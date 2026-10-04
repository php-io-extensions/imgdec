<?php

declare(strict_types=1);

if (! extension_loaded('imgdec')) {
    throw new RuntimeException('The imgdec extension is not loaded; run pest with -d extension=/path/to/imgdec.so');
}

/** A truecolor gd image of $width x $height from a pixel function (x, y) => [r, g, b, a 0..127]. */
function gdImage(int $width, int $height, Closure $pixel): GdImage
{
    $image = imagecreatetruecolor($width, $height);
    imagealphablending($image, false);
    imagesavealpha($image, true);
    for ($y = 0; $y < $height; $y++) {
        for ($x = 0; $x < $width; $x++) {
            [$r, $g, $b, $a] = $pixel($x, $y);
            imagesetpixel($image, $x, $y, ($a << 24) | ($r << 16) | ($g << 8) | $b);
        }
    }

    return $image;
}

function gdBytes(GdImage $image, string $format, bool $progressive = false): string
{
    imageinterlace($image, $progressive);
    ob_start();
    $format === 'png' ? imagepng($image) : imagejpeg($image, null, 95);

    return ob_get_clean();
}

/**
 * A little-endian, uncompressed, one-strip classic TIFF.
 *
 * @param  array<int, list<int>>  $extra  More tags: number => SHORT values.
 */
function tiff(int $width, int $height, int $photometric, int $bits, int $spp, string $samples, array $extra = []): string
{
    $tags = [256 => [$width], 257 => [$height], 258 => array_fill(0, $spp, $bits), 259 => [1], 262 => [$photometric], 273 => [0], 277 => [$spp], 278 => [$height], 279 => [strlen($samples)]] + $extra;
    ksort($tags);
    $ifd = 8 + strlen($samples) + (strlen($samples) % 2);
    $values = '';
    $entries = '';
    $after = $ifd + 2 + 12 * count($tags) + 4;
    foreach ($tags as $tag => $list) {
        $type = in_array($tag, [256, 257, 273, 278, 279], true) ? 4 : 3;
        $packed = pack($type === 4 ? 'V*' : 'v*', ...$list);
        if ($tag === 273) {
            $packed = pack('V', 8);
        }
        $entries .= pack('vvV', $tag, $type, count($list)).(strlen($packed) <= 4 ? str_pad($packed, 4, "\0") : pack('V', $after + strlen($values)));
        if (strlen($packed) > 4) {
            $values .= $packed;
        }
    }

    return "II*\0".pack('V', $ifd).$samples.(strlen($samples) % 2 ? "\0" : '').pack('v', count($tags)).$entries.pack('V', 0).$values;
}
