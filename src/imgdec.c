/*
 * imgdec: PNG, JPEG and TIFF bytes into straight RGBA8 in C, through libpng,
 * libjpeg and libtiff. Three functions, one exception class, the limits.
 */

#include "core.h"
#include "php_imgdec.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "../stubs/imgdec_arginfo.h"

#include <png.h>
#include <stdarg.h>
#include <stdio.h>
#include <jpeglib.h>
#include <tiffio.h>

static zend_class_entry *imgdec_ce_exception;

imgdec_status imgdec_fail(imgdec_error *error, imgdec_status status, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vsnprintf(error->message, sizeof(error->message), format, args);
	va_end(args);

	return status;
}

imgdec_status imgdec_check_size(uint64_t width, uint64_t height, imgdec_error *error)
{
	if (width < 1 || height < 1) {
		return imgdec_fail(error, IMGDEC_CORRUPT, "it says it is %llux%llu.", (unsigned long long) width, (unsigned long long) height);
	}
	if (width > IMGDEC_MAX_SIDE || height > IMGDEC_MAX_SIDE || width * height > IMGDEC_MAX_PIXELS) {
		return imgdec_fail(error, IMGDEC_TOO_LARGE, "a %llux%llu image is past the limit: sides up to %d, %u pixels in all.",
			(unsigned long long) width, (unsigned long long) height, IMGDEC_MAX_SIDE, IMGDEC_MAX_PIXELS);
	}

	return IMGDEC_OK;
}

typedef imgdec_status (*imgdec_decoder)(const unsigned char *, size_t, imgdec_image *, imgdec_error *);

static void imgdec_run(INTERNAL_FUNCTION_PARAMETERS, imgdec_decoder decode)
{
	zend_string *data;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(data)
	ZEND_PARSE_PARAMETERS_END();

	imgdec_image image = {0, 0, NULL};
	imgdec_error error = {{0}};
	imgdec_status status = decode((const unsigned char *) ZSTR_VAL(data), ZSTR_LEN(data), &image, &error);
	if (status != IMGDEC_OK) {
		if (image.rgba8 != NULL) {
			zend_string_release(image.rgba8);
		}
		zend_throw_exception(imgdec_ce_exception, error.message, status);
		RETURN_THROWS();
	}

	array_init_size(return_value, 3);
	add_assoc_long(return_value, "width", image.width);
	add_assoc_long(return_value, "height", image.height);
	add_assoc_str(return_value, "rgba8", image.rgba8);
}

PHP_FUNCTION(imgdec_png)
{
	imgdec_run(INTERNAL_FUNCTION_PARAM_PASSTHRU, imgdec_png);
}

PHP_FUNCTION(imgdec_jpeg)
{
	imgdec_run(INTERNAL_FUNCTION_PARAM_PASSTHRU, imgdec_jpeg);
}

PHP_FUNCTION(imgdec_tiff)
{
	imgdec_run(INTERNAL_FUNCTION_PARAM_PASSTHRU, imgdec_tiff);
}

PHP_MINIT_FUNCTION(imgdec)
{
	(void) type;

	register_imgdec_symbols(module_number);
	imgdec_ce_exception = register_class_ImgdecException(zend_ce_exception);

	return SUCCESS;
}

PHP_RINIT_FUNCTION(imgdec)
{
	(void) type;
	(void) module_number;
#if defined(COMPILE_DL_IMGDEC) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}

PHP_MINFO_FUNCTION(imgdec)
{
	char jpeg[16];

	(void) zend_module;
	snprintf(jpeg, sizeof(jpeg), "%d", JPEG_LIB_VERSION);

	php_info_print_table_start();
	php_info_print_table_row(2, "imgdec support", "enabled");
	php_info_print_table_row(2, "Version", PHP_IMGDEC_VERSION);
	php_info_print_table_row(2, "libpng", png_get_libpng_ver(NULL));
	php_info_print_table_row(2, "libjpeg API", jpeg);
	php_info_print_table_row(2, "libtiff", TIFFGetVersion());
	php_info_print_table_end();
}

zend_module_entry imgdec_module_entry = {
	STANDARD_MODULE_HEADER,
	"imgdec",
	ext_functions,
	PHP_MINIT(imgdec),
	NULL,
	PHP_RINIT(imgdec),
	NULL,
	PHP_MINFO(imgdec),
	PHP_IMGDEC_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_IMGDEC
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(imgdec)
#endif
