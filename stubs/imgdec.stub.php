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

/**
 * Channel order for imgdec_decode_into(): red, green, blue, alpha.
 * @var int
 * @cvalue IMGDEC_RGBA
 */
const IMGDEC_RGBA = UNKNOWN;

/**
 * Channel order for imgdec_decode_into(): blue, green, red, alpha.
 * @var int
 * @cvalue IMGDEC_BGRA
 */
const IMGDEC_BGRA = UNKNOWN;

/** Why an image was not decoded; the code is IMGDEC_CORRUPT, IMGDEC_UNSUPPORTED or IMGDEC_TOO_LARGE. */
class ImgdecException extends Exception {}

/**
 * The header, and nothing past it: format (png, jpeg or tiff), width, height,
 * whether a pixel can be partly transparent, and how many frames the file holds.
 *
 * @return array{format: string, width: int, height: int, alpha: bool, frames: int}
 * @throws ImgdecException
 */
function imgdec_info(string $bytes): array {}

/**
 * One frame written at $address, straight alpha, rows top to bottom, $stride
 * bytes from one row to the next. $order is IMGDEC_RGBA or IMGDEC_BGRA. Bytes
 * outside the rows, and past the last row, are left as they were. The answer
 * is the region written, [x, y, width, height], which is the whole frame at (0, 0).
 *
 * Refuses, before writing one byte, when $address is 0, when $stride is shorter
 * than a packed row, when $stride × height is past $capacity, and when $frame
 * is not in the file.
 *
 * @return array{int, int, int, int}
 * @throws ImgdecException
 */
function imgdec_decode_into(string $bytes, int $address, int $capacity, int $stride, int $order, int $frame = 0): array {}
