<?php

/** @generate-class-entries */

/**
 * The longest side an image may have.
 * @var int
 * @cvalue IMGDEC_MAX_SIDE
 */
const IMGDEC_MAX_SIDE = UNKNOWN;

/**
 * The most pixels one image may hold: 8192 × 8192.
 * @var int
 * @cvalue IMGDEC_MAX_PIXELS
 */
const IMGDEC_MAX_PIXELS = UNKNOWN;

/**
 * ImgdecException code: the bytes are not a readable image.
 * @var int
 * @cvalue IMGDEC_CORRUPT
 */
const IMGDEC_CORRUPT = UNKNOWN;

/**
 * ImgdecException code: a readable image in a layout the rules leave out.
 * @var int
 * @cvalue IMGDEC_UNSUPPORTED
 */
const IMGDEC_UNSUPPORTED = UNKNOWN;

/**
 * ImgdecException code: past IMGDEC_MAX_SIDE or IMGDEC_MAX_PIXELS.
 * @var int
 * @cvalue IMGDEC_TOO_LARGE
 */
const IMGDEC_TOO_LARGE = UNKNOWN;

/** Why an image was not decoded; the code is IMGDEC_CORRUPT, IMGDEC_UNSUPPORTED or IMGDEC_TOO_LARGE. */
class ImgdecException extends Exception {}

/**
 * PNG bytes into straight RGBA8 through libpng: palettes and grey expanded,
 * tRNS made alpha, 16-bit samples cut to their high byte, interlacing undone,
 * no gamma applied.
 *
 * @return array{width: int, height: int, rgba8: string}
 * @throws ImgdecException
 */
function imgdec_png(string $data): array {}

/**
 * JPEG bytes into RGBA8 through libjpeg's default decode: grey made RGB, CMYK
 * as gd converts it (inverted when an Adobe marker says so), alpha 255.
 *
 * @return array{width: int, height: int, rgba8: string}
 * @throws ImgdecException
 */
function imgdec_jpeg(string $data): array {}

/**
 * Classic TIFF bytes into straight RGBA8 through libtiff: the first image,
 * any compression libtiff reads, chunky or in separate planes, orientation
 * top-left; grey of either polarity at 1, 2, 4, 8 or 16 bits, RGB at 8 or 16,
 * a palette at 1, 2, 4 or 8 (colour map entries cut to their high byte);
 * unsigned samples; a first extra sample that is alpha kept (associated
 * alpha divided out: c = round(c × 255 ÷ a), capped at 255).
 *
 * @return array{width: int, height: int, rgba8: string}
 * @throws ImgdecException
 */
function imgdec_tiff(string $data): array {}
