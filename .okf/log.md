# Log

## 2026-10-28

* **Version**: 0.11.0 in `PHP_IMGDEC_VERSION` (`php_imgdec.h`); `branch-alias` `dev-main` is `0.11.x-dev`. Promoted with the Venusian 0.11 ecosystem. No API or behaviour change. Built and loaded on Homebrew PHP 8.4 NTS and ZTS and the Pi on 8.4 ZTS: 16 passed on each. An x86_64 8.4 NTS box without ext-gd loads it and passes 12; the 4 others need gd, which `tests/Pest.php` uses to make the fixture images.

## 2026-10-09


## 2026-10-04

* Bundle created with ext-imgdec 0.10.0: [decoders](api/decoders.md) (`imgdec_png`, `imgdec_jpeg`, `imgdec_tiff`, `ImgdecException`, 5 constants), [build](runbooks/build.md).

## 2026-10-13

* Version 0.10.5: `PHP_IMGDEC_VERSION` and the version test move from 0.10.0. README rewritten to the php-io-extensions template.
