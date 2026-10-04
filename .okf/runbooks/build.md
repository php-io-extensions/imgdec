---
type: Runbook
title: Build, install, test
description: The three libraries, scratch build with gen_stub, the two installers, Pest, the Pi loop.
resource: install-macos.sh
tags: [build, linux, macos, pest]
status: draft
generated: { by: claude-opus/5.5, at: 2026-10-04T17:07:35Z }
sources:
  - id: mac
    resource: install-macos.sh
    title: install-macos.sh
  - id: debian
    resource: install-debian-trixie.sh
    title: install-debian-trixie.sh
  - id: m4
    resource: config.m4
    title: config.m4
---

# Overview

Needs pkg-config, libpng ≥ 1.6, libjpeg, libtiff-4 ≥ 4.5 (`TIFFOpenOptions`). `config.m4` finds all three by pkg-config.[^m4] macOS: `brew install pkg-config libpng jpeg-turbo libtiff`. Debian / Pi OS: `apt install pkg-config libpng-dev libjpeg-dev libtiff-dev`.

Dev loop: copy `config.m4 php_imgdec.h src stubs` to a scratch dir, `phpize`, `php build/gen_stub.php stubs`, `./configure --enable-imgdec --with-php-config=…`, `make CFLAGS="-Wall -Wextra -Wno-unused-parameter"`. Copy `stubs/imgdec_arginfo.h` back after a stub edit. Run Pest with `-d extension=<scratch>/modules/imgdec.so`. No warning from `src/` is acceptable.

macOS (`install-macos.sh`): checks the libraries, builds in a temp copy for php@8.4 and php@8.4-zts, ad-hoc signs, writes `30-imgdec.ini`.[^mac]

Linux (`install-debian-trixie.sh`): checks the libraries, builds in place, installs `imgdec.so`, writes `30-imgdec.ini`, removes build output.[^debian]

Pi from the Mac: tree on the Mac is authoritative, Pi copy disposable. `COPYFILE_DISABLE=1 tar --no-mac-metadata --exclude .git -czf - -C <ext> . | fnk 'tar -xzf - -C ~/imgdec'`, install, Pest, `rm -rf ~/imgdec`.

Surface's `tests/Images` drives the ext through its 'extended' driver against hand-written PNG and TIFF files and holds it to the PHP driver (`ParityTest`); the suite here is the ext on its own.

[^mac]: Same script as ext-rasterize's, names changed, library check added.
[^debian]: Same script as ext-rasterize's, names changed, library check added.
[^m4]: config.m4
