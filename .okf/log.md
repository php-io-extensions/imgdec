# Log

## 2026-11-17

* **Version**: 0.11.1. `imgdec_png`, `imgdec_jpeg` and `imgdec_tiff` are gone. `imgdec_info` reads the header without decoding pixels. `imgdec_decode_into` writes one frame at an address the caller owns (RGBA or BGRA, a stride, a capacity) and returns the region. A refusal writes nothing. `--enable-imgdec-probes` builds the three test helpers and is not a release flag. Arginfo is hand-written. 23 passed on Homebrew PHP 8.4 NTS and ZTS. A 512×512 JPEG decoded in 2.4 ms and a 512×512 PNG in 0.7 ms. Three thousand mutated files under guard malloc did not crash, and a failed decode wrote nothing.

## 2026-10-28

* **Version**: 0.11.0 in `PHP_IMGDEC_VERSION` (`php_imgdec.h`); `branch-alias` `dev-main` is `0.11.x-dev`. Promoted with the Venusian 0.11 ecosystem. No API or behaviour change. Built and loaded on Homebrew PHP 8.4 NTS and ZTS and the Pi on 8.4 ZTS: 16 passed on each. An x86_64 8.4 NTS box without ext-gd loads it and passes 12; the 4 others need gd, which `tests/Pest.php` uses to make the fixture images.

## 2026-10-09


## 2026-10-04

* Bundle created with ext-imgdec 0.10.0: [decoders](api/decoders.md) (`imgdec_png`, `imgdec_jpeg`, `imgdec_tiff`, `ImgdecException`, 5 constants), [build](runbooks/build.md).

## 2026-10-13

* Version 0.10.5: `PHP_IMGDEC_VERSION` and the version test move from 0.10.0. README rewritten to the php-io-extensions template.
