<?php

declare(strict_types=1);

it('declares its limits and its exception', function () {
    expect(IMGDEC_MAX_SIDE)->toBe(65535)
        ->and(IMGDEC_MAX_PIXELS)->toBe(1 << 26)
        ->and([IMGDEC_CORRUPT, IMGDEC_UNSUPPORTED, IMGDEC_TOO_LARGE])->toBe([1, 2, 3])
        ->and(is_subclass_of(ImgdecException::class, Exception::class))->toBeTrue()
        ->and(phpversion('imgdec'))->toBe('0.10.0');
});

it('decodes PNG to straight RGBA8, alpha exactly as the file holds it', function () {
    $alphas = [0, 1, 64, 127];   // gd's 7-bit scale: 127 is clear, 0 opaque
    $png = gdBytes(gdImage(4, 2, fn (int $x, int $y): array => [$x * 60, $y * 200, 7, $alphas[$x]]), 'png');
    $image = imgdec_png($png);

    expect([$image['width'], $image['height']])->toBe([4, 2])
        ->and(strlen($image['rgba8']))->toBe(4 * 2 * 4)
        ->and(bin2hex(substr($image['rgba8'], 0, 16)))->toBe('000007ff'.'3c0007fd'.'7800077e'.'b4000700');   // gd wrote alphas 0, 1, 64, 127 of 127 as 255, 253, 126, 0
});

it('decodes JPEG, baseline and progressive, opaque and close to the source', function (bool $progressive) {
    $jpeg = gdBytes(gdImage(16, 8, fn (int $x, int $y): array => [$x * 16, $y * 32, 128, 0]), 'jpeg', $progressive);
    $image = imgdec_jpeg($jpeg);
    $pixels = array_chunk(array_values(unpack('C*', $image['rgba8'])), 4);

    expect([$image['width'], $image['height']])->toBe([16, 8])
        ->and(array_unique(array_column($pixels, 3)))->toBe([255])
        ->and(abs($pixels[16 * 4 + 8][0] - 128))->toBeLessThan(12)
        ->and(abs($pixels[16 * 4 + 8][1] - 128))->toBeLessThan(12);
})->with(['baseline' => false, 'progressive' => true]);

it('decodes TIFF: RGB, palette and grey with alpha', function () {
    $rgb = imgdec_tiff(tiff(2, 1, 2, 8, 3, "\x10\x20\x30\x40\x50\x60"));
    $palette = imgdec_tiff(tiff(2, 1, 3, 8, 1, "\x01\x00", [320 => [...array_fill(0, 256, 0x1100), ...array_fill(0, 256, 0x2200), ...array_fill(0, 256, 0x33FF)]]));
    $grey = imgdec_tiff(tiff(2, 1, 1, 8, 2, "\x40\x80\xc0\xff", [338 => [2]]));

    expect(bin2hex($rgb['rgba8']))->toBe('102030ff'.'405060ff')
        ->and(bin2hex($palette['rgba8']))->toBe('112233ff'.'112233ff')
        ->and(bin2hex($grey['rgba8']))->toBe('40404080'.'c0c0c0ff');
});

it('says why it will not, with the code to tell a broken image from an unsupported one', function (Closure $call, int $code, string $message) {
    expect($call)->toThrow(function (ImgdecException $e) use ($code, $message): void {
        expect($e->getCode())->toBe($code)
            ->and($e->getMessage())->toContain($message);
    });
})->with([
    'not a PNG' => [fn () => imgdec_png('plainly not a PNG file'), IMGDEC_CORRUPT, 'Not a PNG file'],
    'eight bytes or fewer' => [fn () => imgdec_png('nope'), IMGDEC_CORRUPT, 'the data stops before the image does'],
    'a cut-off PNG' => [fn () => imgdec_png(substr(gdBytes(gdImage(8, 8, fn () => [1, 2, 3, 0]), 'png'), 0, 60)), IMGDEC_CORRUPT, ''],
    'not a JPEG' => [fn () => imgdec_jpeg('nope'), IMGDEC_CORRUPT, 'Not a JPEG file'],
    'not a TIFF' => [fn () => imgdec_tiff('nope'), IMGDEC_CORRUPT, ''],
    'CMYK TIFF' => [fn () => imgdec_tiff(tiff(1, 1, 5, 8, 4, "\0\0\0\0")), IMGDEC_UNSUPPORTED, 'photometric 5'],
    '12-bit RGB TIFF' => [fn () => imgdec_tiff(tiff(1, 1, 2, 12, 3, "\0\0\0\0\0")), IMGDEC_UNSUPPORTED, '12-bit RGB'],
    'a turned TIFF' => [fn () => imgdec_tiff(tiff(1, 1, 2, 8, 3, "\0\0\0", [274 => [3]])), IMGDEC_UNSUPPORTED, 'orientation 3'],
    'float TIFF samples' => [fn () => imgdec_tiff(tiff(1, 1, 1, 32, 1, "\0\0\0\0", [339 => [3]])), IMGDEC_UNSUPPORTED, 'sample format 3'],
    'a TIFF past the pixel limit' => [fn () => imgdec_tiff(tiff(9000, 9000, 1, 8, 1, "\0")), IMGDEC_TOO_LARGE, '9000x9000'],
]);

it('takes only a string', function () {
    expect(fn () => imgdec_png([]))->toThrow(TypeError::class);
});
