---
type: API
title: Decoders
description: "Constants, ImgdecException, imgdec_png/jpeg/tiff: the rules each keeps, what each refuses."
resource: stubs/
tags: [imgdec, png, jpeg, tiff, c]
status: draft
generated: { by: claude-opus/5.5, at: 2026-10-04T17:07:35Z }
sources:
  - id: stubs
    resource: stubs/imgdec.stub.php
    title: imgdec.stub.php
  - id: tiff
    resource: src/tiff.c
    title: The TIFF reader and its refusals
---

# Overview

Bytes in, `['width' => int, 'height' => int, 'rgba8' => string]` out: straight RGBA8, top-left first. No pixel stores, no framework names.[^stubs]

| Function | Library | Rules |
|---|---|---|
| `imgdec_png(string)` | libpng full read API | `png_set_expand` (palette, grey < 8, tRNS → alpha, on 16 bits), `strip_16` (high byte), `gray_to_rgb`, filler 0xFF, interlace handling; no gamma |
| `imgdec_jpeg(string)` | libjpeg | defaults; out RGB, or CMYK for CMYK/YCCK then gd's `(255 − c)(255 − k) / 255`, inverted first under an Adobe marker; alpha 255 |
| `imgdec_tiff(string)` | libtiff, `TIFFClientOpenExt` over the bytes | strips or tiles, chunky or separate planes; conversion here |

Constants: `IMGDEC_MAX_SIDE` 65535, `IMGDEC_MAX_PIXELS` 2²⁶, codes `IMGDEC_CORRUPT` 1, `IMGDEC_UNSUPPORTED` 2, `IMGDEC_TOO_LARGE` 3.

# Refusals

`ImgdecException` (extends `Exception`), code as above, message the reason:

* Corrupt: the library's own message (`Not a PNG file`, `Not a JPEG file: starts with …`, libtiff's first error), `the data stops before the image does`.
* Too large: before decoding, from the header.
* TIFF unsupported, in these words (Surface's PHP reader uses the same): `sample format N`, `orientation N`, `planar configuration N`, `fill order N`, `photometric N`, `N-bit grey|RGB|palette`, `… with N samples per pixel`, `N-bit samples with extra samples`, `palette with extra samples`, `predictor N on M-bit samples`.[^tiff]
* Wrong argument type: `TypeError`.

libtiff knows the Predictor tag only under a codec that uses it (LZW, Deflate); elsewhere none applies. libjpeg and libpng warnings never stop a decode; libtiff's are dropped. Each call collects its own TIFF errors (per-handle handler), so threads do not share messages.

[^stubs]: imgdec.stub.php
[^tiff]: The TIFF reader and its refusals
