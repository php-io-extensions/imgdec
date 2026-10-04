PHP_ARG_ENABLE([imgdec],
  [whether to enable imgdec support],
  [AS_HELP_STRING([--enable-imgdec], [Enable the imgdec image decoding extension (needs libpng, libjpeg and libtiff)])],
  [no])

if test "$PHP_IMGDEC" != "no"; then
  PKG_CHECK_MODULES([IMGDEC_PNG], [libpng >= 1.6])
  PKG_CHECK_MODULES([IMGDEC_JPEG], [libjpeg])
  PKG_CHECK_MODULES([IMGDEC_TIFF], [libtiff-4 >= 4.5])

  PHP_EVAL_INCLINE([$IMGDEC_PNG_CFLAGS $IMGDEC_JPEG_CFLAGS $IMGDEC_TIFF_CFLAGS])
  PHP_EVAL_LIBLINE([$IMGDEC_PNG_LIBS $IMGDEC_JPEG_LIBS $IMGDEC_TIFF_LIBS], [IMGDEC_SHARED_LIBADD])
  PHP_SUBST([IMGDEC_SHARED_LIBADD])

  PHP_NEW_EXTENSION([imgdec],
    [src/imgdec.c src/png.c src/jpeg.c src/tiff.c],
    [$ext_shared],, [-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1])
  PHP_ADD_BUILD_DIR([$ext_builddir/src])
fi
