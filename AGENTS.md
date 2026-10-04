# Agent guidance — php-io-extensions/imgdec

1. **Read [`.okf/index.md`](.okf/index.md) first** before changing the API, the C, or packaging. Open only the concepts the change touches.
2. **Three functions, one answer shape.** `imgdec_png()`, `imgdec_jpeg()`, `imgdec_tiff()` take the file's bytes and answer `['width', 'height', 'rgba8']`: straight RGBA8, top-left first. No pixel stores, no framework names, no scaling or colour management. See [`.okf/api/decoders.md`](.okf/api/decoders.md).
3. **The rules are a contract.** Surface's `Images` component holds its PHP reader (`Surface\Images\Native\TiffReader`, `GdReader`) to these same rules and its `ParityTest` to the same bytes. A change to what a format decodes to, or to a refusal's wording, changes there too, in the same run.
4. **Plain C on the Zend API.** No Zephir, no FFI. Links libpng ≥ 1.6, libjpeg and libtiff-4 ≥ 4.5 through pkg-config. Each library's fatal path (libpng and libjpeg longjmp, libtiff's per-handle error handler) frees what the call allocated; no PHP call may leak, crash or keep a stale error.
5. **Limits before pixels.** `IMGDEC_MAX_SIDE` and `IMGDEC_MAX_PIXELS` are checked from the header, before any buffer the size of the image exists.
6. **The stub is the declaration.** Edit `stubs/imgdec.stub.php`, regenerate `stubs/imgdec_arginfo.h` with gen_stub, commit both. Never hand-edit arginfo. See [`.okf/runbooks/build.md`](.okf/runbooks/build.md).
7. **NTS and ZTS.** No module globals. Every change builds with `-Wall -Wextra` without a warning from `src/` and passes Pest under Homebrew `php@8.4` and `php@8.4-zts` and on Linux (the Pi via `fnk`).
8. **Tests are Pest v4 and make their own inputs** (gd for PNG and JPEG, an inline TIFF writer). No fixture files from the network.
9. **Durable facts go in `.okf`.** Update the matching concept, bump its `generated.at`, append `.okf/log.md`, validate with `--strict`. The bundle documents the package, never a session.
10. **Versions**: `composer.json` and `PHP_IMGDEC_VERSION` move together, and only past a published tag.
