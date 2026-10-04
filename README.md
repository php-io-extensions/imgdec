# ext-imgdec

[![Latest Version on Packagist](https://img.shields.io/packagist/v/php-io-extensions/imgdec.svg)](https://packagist.org/packages/php-io-extensions/imgdec)
[![License](https://img.shields.io/packagist/l/php-io-extensions/imgdec.svg)](LICENSE)

PNG, JPEG and TIFF bytes into straight (not premultiplied) RGBA8 pixels, in C, through
[libpng](http://www.libpng.org/pub/png/libpng.html), [libjpeg](https://libjpeg-turbo.org/) and
[libtiff](https://libtiff.gitlab.io/libtiff/). Written against the Zend API.

PHP's gd decodes images too, but it keeps alpha in 7 bits, doesn't read TIFF, and gets its pixels
out only through `imagecolorat()`, one call per pixel. ext-imgdec answers every pixel as one string,
in one call, ready to hand to a framebuffer or a GPU texture. A 512 × 512 map tile takes about 1 ms
instead of 25–55 ms through gd.

```
ext-imgdec                          bytes → RGBA8                         ← this package
  → venusian-surface/images         app('images')->decode(): a framebuffer, 'extended' driver
    → canvases, panels, GPU engines draw it
```

## Requirements

- PHP 8.4 or newer, NTS or ZTS
- Linux or macOS
- libpng 1.6+, libjpeg (libjpeg-turbo) and libtiff 4.5+, with their headers, found through `pkg-config`:
  - macOS: `brew install pkg-config libpng jpeg-turbo libtiff`
  - Debian, Raspberry Pi OS, Ubuntu: `sudo apt install pkg-config libpng-dev libjpeg-dev libtiff-dev`

## Installation

With [PIE](https://github.com/php/pie):

```bash
pie install php-io-extensions/imgdec
```

From a checkout, with the bundled installers. Each one checks for the three libraries, builds,
installs `imgdec.so` into the PHP's `extension_dir`, writes `30-imgdec.ini` into its conf.d
directory, and checks that the extension loads:

```bash
./install-macos.sh                     # Homebrew php@8.4 and php@8.4-zts
./install-debian-trixie.sh             # the php on PATH: Debian trixie, Raspberry Pi OS, Ubuntu 24.04+
./install-macos.sh /path/to/bin/php    # specific PHP binaries
```

By hand:

```bash
phpize && ./configure --enable-imgdec && make && make install
echo 'extension=imgdec' > "$(php -r 'echo PHP_CONFIG_FILE_SCAN_DIR;')/30-imgdec.ini"
```

## Usage

### Decode a file

```php
['width' => $width, 'height' => $height, 'rgba8' => $pixels] = imgdec_png(file_get_contents('legend.png'));

// Four bytes a pixel, rows top to bottom: the pixel at (x, y) is
$red = ord($pixels[($y * $width + $x) * 4]);
$alpha = ord($pixels[($y * $width + $x) * 4 + 3]);
```

### Decode what a server sends

```php
$jpeg = file_get_contents('https://gibs.earthdata.nasa.gov/wmts/epsg4326/best/MODIS_Terra_CorrectedReflectance_TrueColor/default/2021-09-21/250m/2/1/2.jpeg');
$tile = imgdec_jpeg($jpeg);
// ['width' => 512, 'height' => 512, 'rgba8' => <1048576 bytes>]
```

Pick the function by the file's first bytes: `\x89PNG` for PNG, `\xFF\xD8\xFF` for JPEG, `II*\0`
or `MM\0*` for TIFF.

### Tell a broken file from an unsupported one

```php
try {
    imgdec_tiff($bytes);
} catch (ImgdecException $e) {
    match ($e->getCode()) {
        IMGDEC_CORRUPT => 'not a readable image',           // the library's own message
        IMGDEC_UNSUPPORTED => 'a layout the rules leave out', // e.g. "photometric 5" for CMYK TIFF
        IMGDEC_TOO_LARGE => 'past the size limits',
    };
}
```

## Functions

| Function | Reads | Answers |
|---|---|---|
| `imgdec_png(string $data): array` | PNG, through libpng | `['width' => int, 'height' => int, 'rgba8' => string]` |
| `imgdec_jpeg(string $data): array` | JPEG, through libjpeg | the same |
| `imgdec_tiff(string $data): array` | classic TIFF, through libtiff | the same |

`rgba8` is `width × height × 4` bytes: red, green, blue, alpha, straight alpha, rows top to bottom.

## Rules

- **PNG**: palettes and grey expanded to RGB, tRNS made alpha (matched on all 16 bits), 16-bit
  samples cut to their high byte, interlacing undone, no gamma applied, opaque images given alpha
  255.
- **JPEG**: libjpeg's default decode; grey made RGB; CMYK and YCCK converted as gd converts them
  (`(255 − c) × (255 − k) / 255`), the samples inverted first when an Adobe marker says so; alpha
  255. A warning, such as data that stops early, does not stop a decode.
- **TIFF**: the first image; any compression libtiff reads; chunky or separate planes; strips or
  tiles; orientation top-left; grey of either polarity at 1, 2, 4, 8 or 16 bits, RGB at 8 or 16, a
  palette at 1, 2, 4 or 8 (colour map entries cut to their high byte); unsigned samples. A first
  extra sample that is alpha is kept; associated alpha is divided out
  (`c = round(c × 255 ÷ a)`, capped at 255). Anything else is `IMGDEC_UNSUPPORTED`, with the reason.

## Exceptions and constants

`ImgdecException` extends `Exception`; its code is one of:

| Constant | Value | Meaning |
|---|---|---|
| `IMGDEC_CORRUPT` | 1 | The bytes are not a readable image |
| `IMGDEC_UNSUPPORTED` | 2 | A readable image in a layout the rules leave out |
| `IMGDEC_TOO_LARGE` | 3 | Past one of the limits below |
| `IMGDEC_MAX_SIDE` | 65535 | The longest side an image may have |
| `IMGDEC_MAX_PIXELS` | 67108864 | The most pixels one image may hold (8192 × 8192) |

The limits are checked from the header, before any pixel buffer exists. A wrong argument type is a
`TypeError`.

## Upgrading

This is the first release.

## Testing

```bash
composer install
php -d extension=/path/to/modules/imgdec.so vendor/bin/pest
```

The suite makes its own images with gd and an inline TIFF writer. Surface's `tests/Images` also
holds the extension to its PHP decoder, byte for byte.

## Security

ext-imgdec parses image files, which often come from the network, through libpng, libjpeg and
libtiff. See [SECURITY.md](SECURITY.md) for what it guards against and how to report a
vulnerability.

## License

MIT. See [LICENSE](LICENSE).
