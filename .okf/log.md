# Log

## 2026-10-09

* `extra.venusian.system` in composer.json: the apt packages `venusian build` installs to compile the extension, the run-time packages a `.deb` carrying it depends on or recommends beyond what `dpkg-shlibdeps` sees, and the Homebrew packages for a dev install.

## 2026-10-04

* Bundle created with ext-imgdec 0.10.0: [decoders](api/decoders.md) (`imgdec_png`, `imgdec_jpeg`, `imgdec_tiff`, `ImgdecException`, 5 constants), [build](runbooks/build.md).
