<?php

declare(strict_types=1);

/*
 * imgdec_info reads the header. imgdec_decode_into writes the pixels at an address.
 * The probe helpers exist only so a test can hold that address; a build without
 * --enable-imgdec-probes cannot run this file.
 */

function placed(string $bytes, int $order = IMGDEC_RGBA, int $frame = 0, ?int $stride = null, ?int $capacity = null): array
{
    $info = imgdec_info($bytes);
    $stride ??= $info['width'] * 4;
    $capacity ??= $stride * $info['height'];
    $address = imgdec_probe_alloc($capacity);
    try {
        $region = imgdec_decode_into($bytes, $address, $capacity, $stride, $order, $frame);
        $raw = imgdec_probe_read($address, $capacity);
    } finally {
        imgdec_probe_free($address);
    }

    return [$info, $region, $raw, $stride];
}

/** Packed rows out of a stride that may be wider, so padding is not part of the pixels. */
function packed(string $raw, int $width, int $height, int $stride): string
{
    $out = '';
    for ($y = 0; $y < $height; $y++) {
        $out .= substr($raw, $y * $stride, $width * 4);
    }

    return $out;
}

it('declares its limits, its orders and its exception', function () {
    expect(IMGDEC_MAX_SIDE)->toBe(65535)
        ->and(IMGDEC_MAX_PIXELS)->toBe(1 << 26)
        ->and([IMGDEC_CORRUPT, IMGDEC_UNSUPPORTED, IMGDEC_TOO_LARGE])->toBe([1, 2, 3])
        ->and([IMGDEC_RGBA, IMGDEC_BGRA])->toBe([0, 1])
        ->and(is_subclass_of(ImgdecException::class, Exception::class))->toBeTrue()
        ->and(phpversion('imgdec'))->toBe('0.11.1')
        ->and(defined('IMGDEC_PROBES'))->toBeTrue();
});

it('writes a PNG at the address in both channel orders, and leaves a wider stride\'s padding untouched', function () {
    $alphas = [0, 1, 64, 127];   // gd's 7-bit scale: 127 is clear, 0 opaque
    $png = gdBytes(gdImage(4, 2, fn (int $x, int $y): array => [$x * 60, $y * 200, 7, $alphas[$x]]), 'png');
    $stride = 4 * 4 + 8;
    [$info, $region, $raw] = placed($png, IMGDEC_RGBA, 0, $stride, $stride * 2 + 16);

    expect($info)->toBe(['format' => 'png', 'width' => 4, 'height' => 2, 'alpha' => true, 'frames' => 1])
        ->and($region)->toBe([0, 0, 4, 2])
        ->and(bin2hex(substr(packed($raw, 4, 2, $stride), 0, 16)))->toBe('000007ff'.'3c0007fd'.'7800077e'.'b4000700');

    $padding = '';
    for ($y = 0; $y < 2; $y++) {
        $padding .= substr($raw, $y * $stride + 16, 8);
    }
    expect($padding)->toBe(str_repeat("\xA5", 16))
        ->and(substr($raw, $stride * 2, 16))->toBe(str_repeat("\xA5", 16));

    [, , $bgra] = placed($png, IMGDEC_BGRA, 0, $stride, $stride * 2);
    expect(bin2hex(substr(packed($bgra, 4, 2, $stride), 0, 4)))->toBe('070000ff');
});

it('answers a PNG header when the file ends after IHDR', function () {
    $png = gdBytes(gdImage(3, 5, fn () => [1, 2, 3, 0]), 'png');
    $ihdr = substr($png, 0, 8 + 8 + 13 + 4);

    expect(imgdec_info($ihdr))->toMatchArray(['format' => 'png', 'width' => 3, 'height' => 5, 'frames' => 1]);
});

it('writes a JPEG, baseline and progressive, opaque and close to the source', function (bool $progressive) {
    $jpeg = gdBytes(gdImage(16, 8, fn (int $x, int $y): array => [$x * 16, $y * 32, 128, 0]), 'jpeg', $progressive);
    [$info, $region, $raw, $stride] = placed($jpeg);
    $pixels = array_chunk(array_values(unpack('C*', packed($raw, 16, 8, $stride))), 4);

    expect($info['format'])->toBe('jpeg')
        ->and($info['alpha'])->toBeFalse()
        ->and($info['frames'])->toBe(1)
        ->and($region)->toBe([0, 0, 16, 8])
        ->and(array_unique(array_column($pixels, 3)))->toBe([255])
        ->and(abs($pixels[16 * 4 + 8][0] - 128))->toBeLessThan(12)
        ->and(abs($pixels[16 * 4 + 8][1] - 128))->toBeLessThan(12);
})->with(['baseline' => false, 'progressive' => true]);

it('answers a JPEG header when the file ends after its SOF marker', function () {
    $jpeg = gdBytes(gdImage(9, 6, fn () => [1, 2, 3, 0]), 'jpeg');
    $sof = strpos($jpeg, "\xFF\xC0");
    if ($sof === false) {
        $sof = strpos($jpeg, "\xFF\xC2");
    }
    expect($sof)->not->toBeFalse();
    $cut = substr($jpeg, 0, $sof + 2 + 8);

    expect(imgdec_info($cut))->toMatchArray(['format' => 'jpeg', 'width' => 9, 'height' => 6, 'alpha' => false, 'frames' => 1]);
});

it('writes TIFF RGB, palette and grey with alpha, and the page that was asked for', function () {
    $rgb = tiff(2, 1, 2, 8, 3, "\x10\x20\x30\x40\x50\x60");
    $palette = tiff(2, 1, 3, 8, 1, "\x01\x00", [320 => [...array_fill(0, 256, 0x1100), ...array_fill(0, 256, 0x2200), ...array_fill(0, 256, 0x33FF)]]);
    $grey = tiff(2, 1, 1, 8, 2, "\x40\x80\xc0\xff", [338 => [2]]);

    $associated = tiff(1, 1, 1, 8, 2, "\x40\x80", [338 => [1]]);
    [, , $rgbRaw, $rgbStride] = placed($rgb);
    [, , $paletteRaw, $paletteStride] = placed($palette);
    [$greyInfo, , $greyRaw, $greyStride] = placed($grey);
    [, , $associatedRaw, $associatedStride] = placed($associated);

    expect(bin2hex(packed($rgbRaw, 2, 1, $rgbStride)))->toBe('102030ff'.'405060ff')
        ->and(bin2hex(packed($paletteRaw, 2, 1, $paletteStride)))->toBe('112233ff'.'112233ff')
        ->and($greyInfo['alpha'])->toBeTrue()
        ->and(bin2hex(packed($greyRaw, 2, 1, $greyStride)))->toBe('40404080'.'c0c0c0ff')
        // 0x40 stored already multiplied by alpha 0x80; straight colour is 0x40 × 255 ÷ 128 = 128.
        ->and(bin2hex(packed($associatedRaw, 1, 1, $associatedStride)))->toBe('80808080');

    $pages = tiffPages();
    expect(imgdec_info($pages)['frames'])->toBe(2);
    [, , $second, $secondStride] = placed($pages, IMGDEC_RGBA, 1);
    expect(bin2hex(packed($second, 1, 1, $secondStride)))->toBe('112233ff');
});

it('refuses before writing a byte when the address, the stride, the capacity or the frame is wrong', function (Closure $call) {
    $png = gdBytes(gdImage(2, 2, fn () => [9, 8, 7, 0]), 'png');
    $info = imgdec_info($png);
    $stride = $info['width'] * 4 + 4;
    $capacity = $stride * $info['height'];
    $address = imgdec_probe_alloc($capacity);
    $before = imgdec_probe_read($address, $capacity);

    try {
        expect($call($png, $address, $capacity, $stride))->toThrow(ImgdecException::class);
        expect(imgdec_probe_read($address, $capacity))->toBe($before);
    } finally {
        imgdec_probe_free($address);
    }
})->with([
    'an address of 0' => [fn (string $png, int $address, int $capacity, int $stride) => fn () => imgdec_decode_into($png, 0, $capacity, $stride, IMGDEC_RGBA)],
    'a stride shorter than a row' => [fn (string $png, int $address, int $capacity, int $stride) => fn () => imgdec_decode_into($png, $address, $capacity, 4, IMGDEC_RGBA)],
    'a capacity shorter than the frame' => [fn (string $png, int $address, int $capacity, int $stride) => fn () => imgdec_decode_into($png, $address, $stride, $stride, IMGDEC_RGBA)],
    'a frame the file does not have' => [fn (string $png, int $address, int $capacity, int $stride) => fn () => imgdec_decode_into($png, $address, $capacity, $stride, IMGDEC_RGBA, 1)],
]);

it('says why it will not, with the code to tell a broken image from an unsupported one', function (Closure $call, int $code, string $message) {
    expect($call)->toThrow(function (ImgdecException $e) use ($code, $message): void {
        expect($e->getCode())->toBe($code)
            ->and($e->getMessage())->toContain($message);
    });
})->with([
    'not a PNG' => [fn () => imgdec_info('plainly not a PNG file'), IMGDEC_CORRUPT, 'not a PNG, JPEG or TIFF'],
    'eight bytes or fewer of a PNG' => [fn () => imgdec_info("\x89PNG\r\n\x1a\n"), IMGDEC_CORRUPT, 'the data stops before the image does'],
    'a cut-off PNG' => [fn () => imgdec_decode_into(substr(gdBytes(gdImage(8, 8, fn () => [1, 2, 3, 0]), 'png'), 0, 60), 1, 1000, 32, IMGDEC_RGBA), IMGDEC_CORRUPT, ''],
    'not a JPEG' => [fn () => imgdec_info("\xFF\xD8"), IMGDEC_CORRUPT, 'the data stops before the image does'],
    'not a TIFF' => [fn () => imgdec_info('nope'), IMGDEC_CORRUPT, 'not a PNG, JPEG or TIFF'],
    'CMYK TIFF' => [fn () => imgdec_decode_into(tiff(1, 1, 5, 8, 4, "\0\0\0\0"), 1, 64, 4, IMGDEC_RGBA), IMGDEC_UNSUPPORTED, 'photometric 5'],
    '12-bit RGB TIFF' => [fn () => imgdec_decode_into(tiff(1, 1, 2, 12, 3, "\0\0\0\0\0"), 1, 64, 4, IMGDEC_RGBA), IMGDEC_UNSUPPORTED, '12-bit RGB'],
    'a turned TIFF' => [fn () => imgdec_decode_into(tiff(1, 1, 2, 8, 3, "\0\0\0", [274 => [3]]), 1, 64, 4, IMGDEC_RGBA), IMGDEC_UNSUPPORTED, 'orientation 3'],
    'float TIFF samples' => [fn () => imgdec_decode_into(tiff(1, 1, 1, 32, 1, "\0\0\0\0", [339 => [3]]), 1, 64, 4, IMGDEC_RGBA), IMGDEC_UNSUPPORTED, 'sample format 3'],
    'a TIFF past the pixel limit' => [fn () => imgdec_info(tiff(9000, 9000, 1, 8, 1, "\0")), IMGDEC_TOO_LARGE, '9000x9000'],
    'a missing TIFF frame' => [fn () => imgdec_decode_into(tiff(1, 1, 2, 8, 3, "\0\0\0"), 1, 64, 4, IMGDEC_RGBA, 2), IMGDEC_CORRUPT, 'no frame 2'],
]);

it('takes only a string', function () {
    expect(fn () => imgdec_info([]))->toThrow(TypeError::class);
});
