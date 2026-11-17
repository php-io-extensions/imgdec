# Security Policy

## Supported versions

ext-imgdec is pre-1.0. No 0.x release receives security fixes or advisories; fixes land in the
next release line. Security support starts with 1.0.

| Version | Security fixes |
|---------|----------------|
| < 1.0   | No             |

## Reporting a vulnerability

Please don't open a public issue for a security problem.

Report it privately through GitHub: the **Report a vulnerability** button on the
[php-io-extensions/imgdec](https://github.com/php-io-extensions/imgdec) repository's **Security** tab.
If that isn't available, email **info@projectsaturnstudios.com**.

Include what you found, the affected version, your platform, PHP build (NTS or ZTS) and the
library versions (`php --ri imgdec` shows libpng, the libjpeg API and libtiff), the input that
triggers it if you can share it, and steps to reproduce. Reports are read and weighed for the
release line in development; before 1.0 there is no response-time commitment.

A defect in libpng, libjpeg or libtiff itself belongs with those projects. Report it here as well
when ext-imgdec should guard against it.

## Security model

ext-imgdec parses image files, which commonly arrive from the network: map tiles, uploads,
downloads. It hands the bytes to libpng, libjpeg and libtiff, so it is exactly as robust as the
library versions it is linked against. Keep them patched; the installers link whatever
pkg-config finds.

- **Size limits.** Width and height are read from the header and refused past
  `IMGDEC_MAX_SIDE` (65535) or `IMGDEC_MAX_PIXELS` (2²⁶) before any pixel buffer exists.
  `imgdec_decode_into()` also refuses, before writing a byte, when the caller's stride is
  shorter than a row or when stride × height is past the capacity it was given.
- **Memory.** Input is read from the PHP string in place; nothing reads past its end (each
  library's reader is bounded by the string's length). A decode allocates its working buffer
  with `emalloc` and frees it before returning. The pixels are copied into the address the
  caller passed, which is not a PHP string. When a library gives up mid-image, the call frees
  what it allocated and throws `ImgdecException` without writing that address.
- **Errors.** libtiff errors are collected per call through a handle-scoped handler, so threads
  of a ZTS build do not see each other's messages. Library warnings never reach output.
- **No I/O.** The functions take bytes; they open no files and make no network requests.

A report is in scope when ext-imgdec itself reads or writes memory it should not, leaks, or
crashes on any input string: a buffer sized from a header that can disagree with the data, a
row copied past its block, an error path that frees twice.
