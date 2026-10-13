<div align="center" id="header">

<h1>ext-imgdec</h1>

<h3>A small PHP8.4+ library for decoding PNG, JPEG and TIFF images into RGBA8 format.</h3>

[![CI](https://img.shields.io/github/actions/workflow/status/php-io-extensions/imgdec/ci.yml?style=flat-square&label=CI)](https://github.com/php-io-extensions/imgdec/actions/workflows/ci.yml)
[![Packagist](https://img.shields.io/packagist/v/php-io-extensions/imgdec?style=flat-square)](https://packagist.org/packages/php-io-extensions/imgdec)
[![PHP](https://img.shields.io/packagist/dependency-v/php-io-extensions/imgdec/php?style=flat-square&label=PHP)](https://packagist.org/packages/php-io-extensions/imgdec)
[![License](https://img.shields.io/github/license/php-io-extensions/imgdec?style=flat-square)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-linux_%7C_macos-blue?style=flat-square&logo=linux&logoColor=white)](#requirements)
[![OKF](https://img.shields.io/badge/OKF-0.2-6f42c1?style=flat-square&label=knowledge%20bundle)](.okf/index.md)
[![PIE](https://img.shields.io/badge/PIE-installable-4f5b93?style=flat-square&logo=php&logoColor=white)](https://github.com/php/pie)

</div>

Anything that draws an image itself, be it a framebuffer, a GPU texture, a display panel, or a compositor, needs the image as plain pixels: every pixel's red, green, blue and alpha channel in a known order. Files don't arrive that way. A map tile comes as JPEG, an icon as PNG with a palette, a scan as a TIFF in strips or tiles, and each format has its own compression, color models and alpha rules to undo first.

<b>The only problem?</b> PHP's built-in answer is gd, but gd was made to edit images, not to hand their pixels over. It keeps alpha in 7 bits, it can't read TIFF, and it gives its pixels out through `imagecolorat()`, one call per pixel. Getting a 512 × 512 JPEG tile into an RGBA8 string that way takes about 22 ms on an average 4-5 year old MBP. gd decodes the tile in about 1 ms; the other 21 are the 262,144 `imagecolorat()` calls plus the packing logic around them.

<b>ext-imgdec</b> decodes in C. Three functions take a file's bytes and answer `['width', 'height', 'rgba8']`: every pixel as one string, straight alpha, rows top to bottom. The same tile takes about 1 ms. PNG goes through libpng, JPEG through libjpeg, TIFF through libtiff, each with its formats' quirks settled by fixed, written-down rules: palettes expanded, transparency made alpha, 16-bit samples cut to 8, CMYK converted, associated alpha divided out.


## Basic usage

Most ways you will interface with ext-imgdec are in two parts -

Get the bytes and pick the decoder. The function is chosen by the file's first bytes, not its name, so a misnamed file still decodes.

```php
function decode(string $bytes): array
{
    return match (true) {
        str_starts_with($bytes, "\x89PNG") => imgdec_png($bytes),
        str_starts_with($bytes, "\xFF\xD8\xFF") => imgdec_jpeg($bytes),
        str_starts_with($bytes, "II*\0"), str_starts_with($bytes, "MM\0*") => imgdec_tiff($bytes),
        default => throw new InvalidArgumentException('not a PNG, JPEG or TIFF'),
    };
}

['width' => $width, 'height' => $height, 'rgba8' => $pixels] = decode(file_get_contents('legend.png'));
```

Use the pixels. `rgba8` is `width × height × 4` bytes, red, green, blue, alpha, rows top to bottom, so the pixel at `(x, y)` starts at byte `(y × width + x) × 4`. Hand the string as it is to whatever draws.

```php
echo "{$width} × {$height}, ", strlen($pixels), " bytes\n";

$pixel = fn (int $x, int $y): string => bin2hex(substr($pixels, ($y * $width + $x) * 4, 4));
echo $pixel(0, 0), ' ', $pixel(3, 1), ' ', $pixel(1, 0), "\n";
```

```console
$ php legend.php
4 × 2, 32 bytes
ff0000ff 0000ff81 00000000
```

Opaque red, half-transparent blue, and a clear pixel: straight alpha, so a clear pixel's colour is whatever the file stored.

### Examples

#### Decode what a server sends

The functions take bytes, so an image fetched over HTTP decodes without touching the disk.

```php
$jpeg = file_get_contents('https://gibs.earthdata.nasa.gov/wmts/epsg4326/best/MODIS_Terra_CorrectedReflectance_TrueColor/default/2021-09-21/250m/2/1/2.jpeg');

['width' => $width, 'height' => $height, 'rgba8' => $tile] = imgdec_jpeg($jpeg);
echo "{$width} × {$height}, ", strlen($tile), " bytes\n";
```

```console
512 × 512, 1048576 bytes
```

#### Tell a broken file from an unsupported one

Every refusal is an `ImgdecException`, and its code says which kind: the bytes aren't a readable image, the image is in a layout the rules leave out, or it is past the size limits. The message is the library's reason, or the rule that refused it.

```php
foreach (['scan.tif', 'poster.tif', 'cut.png'] as $path) {
    try {
        decode(file_get_contents($path));
    } catch (ImgdecException $e) {
        echo $path, ': ', match ($e->getCode()) {
            IMGDEC_CORRUPT => 'not a readable image',
            IMGDEC_UNSUPPORTED => 'a layout imgdec leaves out',
            IMGDEC_TOO_LARGE => 'past the size limits',
        }, ' (', $e->getMessage(), ")\n";
    }
}
```

```console
scan.tif: a layout imgdec leaves out (photometric 5)
poster.tif: past the size limits (a 9000x9000 image is past the limit: sides up to 65535, 67108864 pixels in all.)
cut.png: not a readable image (the data stops before the image does.)
```

`scan.tif` is CMYK, photometric 5 in TIFF's terms; `poster.tif` declares 81 million pixels; `cut.png` stops after its first 40 bytes.

## Requirements

| | |
|---|---|
| OS | Linux or macOS. `composer.json` lists those two families. |
| PHP | 8.4 or newer, NTS or ZTS. |
| Libraries | libpng 1.6 or newer, libjpeg (libjpeg-turbo is the usual one) and libtiff 4.5 or newer, with their headers, found through `pkg-config`. |

To build it you need a C compiler, `pkg-config`, the three libraries' development files, and the development files of the PHP you intend to load it into:

```bash
# Debian, Ubuntu, Raspberry Pi OS
sudo apt install build-essential pkg-config libpng-dev libjpeg-dev libtiff-dev php8.4-dev

# macOS: Xcode Command Line Tools, and Homebrew's PHP ships phpize and php-config
xcode-select --install
brew install pkg-config libpng jpeg-turbo libtiff php@8.4
```

## Installation

### With PHP PIE (recommended)

[PIE](https://github.com/php/pie) is the PHP Foundation's installer for extensions. It downloads the tagged source from Packagist, builds it against the PHP on your `PATH`, installs the module and enables it. `sudo` where your PHP's extension directory is root-owned, as on Debian; not on a Homebrew Mac. Install the libraries above first:

```bash
pie install php-io-extensions/imgdec
```

### Manually (install script)

The repository ships two installers, one per platform. Each one checks that `pkg-config` finds the three libraries, builds for each PHP it targets, installs `imgdec.so` into that PHP's `extension_dir`, writes `30-imgdec.ini` into its `conf.d` scan directory, and checks the module loads. The macOS one builds in a temporary copy of the checkout and ad-hoc signs the module so the system loads it on first use; the Debian one builds in the checkout, writes the ini into the CLI, FPM and Apache `conf.d` of that PHP, and cleans the build products out afterwards.

```bash
git clone https://github.com/php-io-extensions/imgdec.git
cd imgdec

./install-debian-trixie.sh             # Debian Trixie, Ubuntu 24.04+, Raspberry Pi OS: the php on PATH
./install-macos.sh                     # Homebrew php@8.4 and php@8.4-zts
./install-macos.sh /path/to/bin/php    # the macOS script takes specific PHP binaries
```

### Manually (phpize)

```bash
git clone https://github.com/php-io-extensions/imgdec.git
cd imgdec
phpize
./configure --enable-imgdec
make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu)"
sudo make install
```

Then enable it: add `extension=imgdec.so` to your `php.ini`, or drop that line into a file in the scan directory that `php --ini` reports.

### Verify

`php --ri imgdec` also names the library versions it was built against:

```console
$ php --ri imgdec

imgdec

imgdec support => enabled
Version => 0.10.x
libpng => 1.6.59
libjpeg API => 80
libtiff => LIBTIFF, Version 4.7.2
```

## API

Three functions, one exception class and five constants, declared in [`stubs/imgdec.stub.php`](stubs/imgdec.stub.php). Each function takes the whole file as a string and answers `['width' => int, 'height' => int, 'rgba8' => string]`: `width × height × 4` bytes of straight-alpha RGBA8, rows top to bottom. Every failure is an `ImgdecException` with one of the three codes below; no partial image is ever returned. An argument that is not a string is a `TypeError`.

### Functions

#### `imgdec_png(string $data): array`

Decodes PNG through libpng.

* Palettes and grey are expanded to RGB, and an image with no alpha gets alpha 255.
* A `tRNS` chunk becomes alpha, matched on all 16 bits of a 16-bit image.
* 16-bit samples are cut to their high byte, and interlacing is undone.
* No gamma is applied: the samples are the file's.

---

#### `imgdec_jpeg(string $data): array`

Decodes JPEG, baseline or progressive, through libjpeg's default decode. Alpha is 255.

* Grey is made RGB.
* CMYK and YCCK are converted as gd converts them, `(255 − c) × (255 − k) / 255` per channel, with the samples inverted first when an Adobe marker says so.
* A warning, such as data that stops early, does not stop a decode.

---

#### `imgdec_tiff(string $data): array`

Decodes the first image of a classic TIFF through libtiff, in any compression libtiff reads, with chunky or separate planes, in strips or tiles.

* Accepted: grey of either polarity at 1, 2, 4, 8 or 16 bits; RGB at 8 or 16; a palette at 1, 2, 4 or 8, its colour map entries cut to their high byte. Unsigned samples, orientation top-left.
* A first extra sample that is alpha is kept. Associated alpha is divided out, `c = round(c × 255 ÷ a)` capped at 255.
* Anything else is `IMGDEC_UNSUPPORTED`, with the reason in the message: `photometric 5` for CMYK, `orientation 3` for a turned image, `sample format 3` for floats, `12-bit RGB`.

### Errors and limits

#### `ImgdecException`

Extends `Exception`. Its code is one of:

| Constant | Value | Meaning |
|---|---|---|
| `IMGDEC_CORRUPT` | `1` | The bytes are not a readable image. The message is the library's reason. |
| `IMGDEC_UNSUPPORTED` | `2` | A readable image in a layout the rules above leave out. The message names it. |
| `IMGDEC_TOO_LARGE` | `3` | Past one of the limits below. |

| Constant | Value | Meaning |
|---|---|---|
| `IMGDEC_MAX_SIDE` | `65535` | The longest side an image may have. |
| `IMGDEC_MAX_PIXELS` | `67108864` | The most pixels one image may hold, 8192 × 8192: a 256 MB result. |

The limits are read from the header and checked before any pixel buffer exists, so a file that only claims to be enormous costs nothing to refuse.

## Extensions you might use with this one

* [ext-fb](https://github.com/php-io-extensions/fb): a pixel store in your display's byte layout. `FbBuffer::blitRgba8()` maps the `rgba8` string into it, and `paintRgba8()` places it scaled, turned or blended.
* [ext-pcurl](https://github.com/php-io-extensions/pcurl): libcurl's multi socket API, for fetching many tiles at once and decoding each as it lands.

## AI-assisted development

This codebase was built with LLM assistance under human direction. Prompts, reviews, and architectural decisions were owned by a professional software engineer — the model accelerates drafting and exploration; it does not replace judgment on APIs, dependency direction, or what ships.

## Agentic Documentation

This repository carries an [Open Knowledge Format](https://github.com/GoogleCloudPlatform/knowledge-catalog/blob/main/okf/SPEC.md) bundle at [`.okf/`](.okf/): Markdown with YAML frontmatter, readable by you and loadable by a coding agent. It records what the README doesn't: the library calls behind each decoder's rules, the exact wording of every refusal, how each library's warnings and errors are handled per call, the libraries the build finds through pkg-config, the scratch build and both installers, and a log of every change that altered behaviour.

Start at [`.okf/index.md`](.okf/index.md). [`AGENTS.md`](AGENTS.md) is the short rule sheet an agent reads before touching the C, the stub, or packaging; each rule points into the bundle for the reason behind it.

Point your agent at the repo and it has the same context the maintainer has. Point yourself at it when the README's answer is "it does X" and you want "because".

## Resources

* [libpng](http://www.libpng.org/pub/png/libpng.html), [libjpeg-turbo](https://libjpeg-turbo.org/) and [libtiff](https://libtiff.gitlab.io/libtiff/): the libraries every decode goes through.
* [PIE](https://github.com/php/pie): the PHP Foundation's extension installer.
* [`gen_stub.php`](https://github.com/php/php-src/blob/master/build/gen_stub.php): the php-src stub format. [`stubs/imgdec.stub.php`](stubs/imgdec.stub.php) is the API declaration; `imgdec_arginfo.h` is generated from it.
* [Open Knowledge Format](https://github.com/GoogleCloudPlatform/knowledge-catalog/blob/main/okf/SPEC.md): the spec the [`.okf/`](.okf/) bundle follows.

## Running the test suite

The suite is Pest v4 and makes its own images: PNG and JPEG through gd, TIFF through an inline writer, so it needs no image files and no network. It covers each format's rules, alpha exactly as the file holds it, baseline and progressive JPEG, palette and grey TIFF with alpha, and every refusal with its code. It runs on Linux and macOS, and needs ext/gd: built into Homebrew's PHP, `php8.4-gd` on Debian. Build the module in the clone, then point `php` at it:

```bash
git clone https://github.com/php-io-extensions/imgdec.git
cd imgdec
phpize && ./configure --enable-imgdec && make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu)"
composer install
php -n -d extension="$PWD/modules/imgdec.so" vendor/bin/pest
```

```console
Tests:    16 passed (50 assertions)
Duration: 0.05s
```

`-n` skips your `php.ini`, so the suite tests the clone and not an installed `imgdec.so`; without it, a PHP that already loads imgdec warns `Module "imgdec" is already loaded` and keeps the installed one. Where gd is a shared extension rather than built in, as on Debian packaging, add `-d extension=gd` before the imgdec one, since `-n` dropped the ini that loads it. A module only loads in the PHP whose `phpize` built it: for Homebrew's ZTS build, run `phpize` and pass `--with-php-config` from `/opt/homebrew/opt/php@8.4-zts/bin`.

## Contributing

Open an issue first for anything beyond a typo. Pull requests change `stubs/imgdec.stub.php` and the C under `src/` together, regenerate `stubs/imgdec_arginfo.h` with gen_stub, and add a Pest test for the behaviour. The decoding rules and the wording of each refusal are a contract other code relies on: a change to either needs its reason in the pull request. CI must be green on every cell of the matrix. Read [`AGENTS.md`](AGENTS.md) for the rules the code follows.

## Security

ext-imgdec parses image files, which often come from the network, through libpng, libjpeg and libtiff. See [SECURITY.md](SECURITY.md) for what it guards against and how to report a vulnerability.

## License

MIT. See [LICENSE](LICENSE).
