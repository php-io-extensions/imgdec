---
type: API
title: Decoders
description: "imgdec_info and imgdec_decode_into: the rules each format keeps, what a call refuses, and the test-only probes."
resource: stubs/
tags: [imgdec, png, jpeg, tiff, c]
status: draft
generated: { by: grok-4.7, at: 2026-11-17T13:40:00Z }
sources:
  - id: stubs
    resource: stubs/imgdec.stub.php
    title: imgdec.stub.php
  - id: info
    resource: src/info.c
    title: Header read and the copy into the caller's address
  - id: tiff
    resource: src/tiff.c
    title: The TIFF reader and its refusals
---

# Overview

`imgdec_info(string $bytes)` reads the header and returns `format`, `width`, `height`, `alpha`, `frames`. A file that ends after a complete PNG IHDR, a JPEG SOF marker, or the first TIFF IFD still answers. It does not decode pixels.[^info]

`imgdec_decode_into(string $bytes, int $address, int $capacity, int $stride, int $order, int $frame = 0)` writes that frame at `$address` and returns `[0, 0, width, height]`. Colours are straight. Rows are top to bottom. `$order` is `IMGDEC_RGBA` (0) or `IMGDEC_BGRA` (1). Padding inside `$stride`, and bytes past the last row, are not touched. The working pixels are an `emalloc` buffer, copied into the address, then freed. They are not a PHP string.[^stubs]

The call refuses before writing one byte when `$address` is 0, when `$stride` is shorter than `width × 4`, when `$stride × height` is past `$capacity`, and when `$frame` is not in the file.

| Format | Library | Rules |
|---|---|---|
| PNG | libpng full read API | `png_set_expand` (palette, grey < 8, tRNS → alpha, on 16 bits), `strip_16` (high byte), `gray_to_rgb`, filler 0xFF, interlace handling; no gamma |
| JPEG | libjpeg | defaults; out RGB, or CMYK for CMYK/YCCK then gd's `(255 − c)(255 − k) / 255`, inverted first under an Adobe marker; alpha 255 |
| TIFF | libtiff, `TIFFClientOpenExt` over the bytes | the asked frame; strips or tiles, chunky or separate planes; conversion here |

Constants: `IMGDEC_MAX_SIDE` 65535, `IMGDEC_MAX_PIXELS` 2²⁶, codes `IMGDEC_CORRUPT` 1, `IMGDEC_UNSUPPORTED` 2, `IMGDEC_TOO_LARGE` 3, orders `IMGDEC_RGBA` 0 and `IMGDEC_BGRA` 1.

`--enable-imgdec-probes` adds `imgdec_probe_alloc`, `imgdec_probe_read` and `imgdec_probe_free`, and the constant `IMGDEC_PROBES`. A probe allocation is filled with `0xA5`. Those three functions exist so tests can hold an address. A release build does not pass the flag.

# Refusals

`ImgdecException` (extends `Exception`), code as above, message the reason:

* Corrupt: the library's own message (`Not a PNG file`, `Not a JPEG file: starts with …`, libtiff's first error), `the data stops before the image does`, `the address is 0.`, `a row is N bytes, and the stride is M.`, `the frame is N bytes, past the capacity of M.`, `PNG|JPEG|TIFF image has no frame N; it has M.`
* Unknown bytes: `The bytes are not a PNG, JPEG or TIFF image.`
* Too large: before decoding, from the header.
* TIFF unsupported, in these words: `sample format N`, `orientation N`, `planar configuration N`, `fill order N`, `photometric N`, `N-bit grey|RGB|palette`, `… with N samples per pixel`, `N-bit samples with extra samples`, `palette with extra samples`, `predictor N on M-bit samples`.[^tiff]
* Wrong argument type: `TypeError`. An order other than the two constants, or a negative capacity, stride or frame, is `ValueError` via `zend_argument_value_error`.

libtiff knows the Predictor tag only under a codec that uses it (LZW, Deflate); elsewhere none applies. libjpeg and libpng warnings never stop a decode; libtiff's are dropped. Each call collects its own TIFF errors (per-handle handler), so threads do not share messages.

[^stubs]: imgdec.stub.php
[^info]: Header read and the copy into the caller's address
[^tiff]: The TIFF reader and its refusals
