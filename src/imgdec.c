/*
 * imgdec: PNG, JPEG and TIFF bytes into straight RGBA8 in C, through libpng,
 * libjpeg and libtiff. The header is imgdec_info(). The pixels are written to
 * an address by imgdec_decode_into(). One exception class, the limits.
 */

#include "core.h"
#include "php_imgdec.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "../stubs/imgdec_arginfo.h"

#include <string.h>

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

static const char *imgdec_format_name(imgdec_format format)
{
	switch (format) {
		case IMGDEC_FMT_PNG: return "png";
		case IMGDEC_FMT_JPEG: return "jpeg";
		case IMGDEC_FMT_TIFF: return "tiff";
	}

	return "image";
}

static void imgdec_throw(const imgdec_error *error, imgdec_status status)
{
	zend_throw_exception(imgdec_ce_exception, error->message, status);
}

PHP_FUNCTION(imgdec_info)
{
	zend_string *bytes;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(bytes)
	ZEND_PARSE_PARAMETERS_END();

	imgdec_facts facts;
	imgdec_error error = {{0}};
	imgdec_status status = imgdec_read_facts((const unsigned char *) ZSTR_VAL(bytes), ZSTR_LEN(bytes), &facts, &error);
	if (status != IMGDEC_OK) {
		imgdec_throw(&error, status);
		RETURN_THROWS();
	}

	array_init_size(return_value, 5);
	add_assoc_string(return_value, "format", imgdec_format_name(facts.format));
	add_assoc_long(return_value, "width", facts.width);
	add_assoc_long(return_value, "height", facts.height);
	add_assoc_bool(return_value, "alpha", facts.alpha);
	add_assoc_long(return_value, "frames", facts.frames);
}

PHP_FUNCTION(imgdec_decode_into)
{
	zend_string *bytes;
	zend_long address, capacity, stride, order, frame = 0;

	ZEND_PARSE_PARAMETERS_START(5, 6)
		Z_PARAM_STR(bytes)
		Z_PARAM_LONG(address)
		Z_PARAM_LONG(capacity)
		Z_PARAM_LONG(stride)
		Z_PARAM_LONG(order)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(frame)
	ZEND_PARSE_PARAMETERS_END();

	if (capacity < 0) {
		zend_argument_value_error(3, "must be at least 0");
		RETURN_THROWS();
	}
	if (stride < 0) {
		zend_argument_value_error(4, "must be at least 0");
		RETURN_THROWS();
	}
	if (order != IMGDEC_RGBA && order != IMGDEC_BGRA) {
		zend_argument_value_error(5, "must be IMGDEC_RGBA or IMGDEC_BGRA");
		RETURN_THROWS();
	}
	if (frame < 0) {
		zend_argument_value_error(6, "must be at least 0");
		RETURN_THROWS();
	}

	imgdec_facts facts;
	imgdec_error error = {{0}};
	const unsigned char *data = (const unsigned char *) ZSTR_VAL(bytes);
	size_t length = ZSTR_LEN(bytes);
	imgdec_status status = imgdec_read_facts(data, length, &facts, &error);
	if (status != IMGDEC_OK) {
		imgdec_throw(&error, status);
		RETURN_THROWS();
	}
	if (address == 0) {
		zend_throw_exception(imgdec_ce_exception, "the address is 0.", IMGDEC_CORRUPT);
		RETURN_THROWS();
	}
	if ((uint32_t) frame >= facts.frames) {
		imgdec_fail(&error, IMGDEC_CORRUPT, "%s image has no frame %u; it has %u.",
			facts.format == IMGDEC_FMT_PNG ? "PNG" : facts.format == IMGDEC_FMT_JPEG ? "JPEG" : "TIFF",
			(unsigned) frame, facts.frames);
		imgdec_throw(&error, IMGDEC_CORRUPT);
		RETURN_THROWS();
	}
	size_t row = (size_t) facts.width * 4;
	if ((size_t) stride < row) {
		imgdec_fail(&error, IMGDEC_CORRUPT, "a row is %zu bytes, and the stride is %ld.", row, (long) stride);
		imgdec_throw(&error, IMGDEC_CORRUPT);
		RETURN_THROWS();
	}
	if (facts.height > 0 && (size_t) stride > SIZE_MAX / facts.height) {
		imgdec_fail(&error, IMGDEC_CORRUPT, "the frame is past the capacity of %ld.", (long) capacity);
		imgdec_throw(&error, IMGDEC_CORRUPT);
		RETURN_THROWS();
	}
	size_t bytes_needed = (size_t) stride * facts.height;
	if (bytes_needed > (size_t) capacity) {
		imgdec_fail(&error, IMGDEC_CORRUPT, "the frame is %zu bytes, past the capacity of %ld.", bytes_needed, (long) capacity);
		imgdec_throw(&error, IMGDEC_CORRUPT);
		RETURN_THROWS();
	}

	imgdec_image image = {0, 0, NULL};
	switch (facts.format) {
		case IMGDEC_FMT_PNG:
			status = imgdec_png(data, length, &image, &error);
			break;
		case IMGDEC_FMT_JPEG:
			status = imgdec_jpeg(data, length, &image, &error);
			break;
		case IMGDEC_FMT_TIFF:
			status = imgdec_tiff(data, length, (uint32_t) frame, &image, &error);
			break;
	}
	if (status != IMGDEC_OK) {
		if (image.rgba8 != NULL) {
			efree(image.rgba8);
		}
		imgdec_throw(&error, status);
		RETURN_THROWS();
	}

	imgdec_store(image.rgba8, image.width, image.height, (unsigned char *) (uintptr_t) address, (size_t) stride, (int) order);
	efree(image.rgba8);

	array_init_size(return_value, 4);
	add_next_index_long(return_value, 0);
	add_next_index_long(return_value, 0);
	add_next_index_long(return_value, image.width);
	add_next_index_long(return_value, image.height);
}

#ifdef IMGDEC_PROBES
typedef struct imgdec_probe {
	unsigned char *bytes;
	size_t size;
	struct imgdec_probe *next;
} imgdec_probe;

static imgdec_probe *imgdec_probes = NULL;

static imgdec_probe *imgdec_probe_find(uintptr_t address)
{
	for (imgdec_probe *probe = imgdec_probes; probe != NULL; probe = probe->next) {
		uintptr_t start = (uintptr_t) probe->bytes;
		if (address >= start && address < start + probe->size) {
			return probe;
		}
	}

	return NULL;
}

PHP_FUNCTION(imgdec_probe_alloc)
{
	zend_long size;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(size)
	ZEND_PARSE_PARAMETERS_END();

	if (size < 1) {
		zend_argument_value_error(1, "must be at least 1");
		RETURN_THROWS();
	}

	imgdec_probe *probe = emalloc(sizeof(imgdec_probe));
	probe->size = (size_t) size;
	probe->bytes = emalloc(probe->size);
	memset(probe->bytes, 0xA5, probe->size);
	probe->next = imgdec_probes;
	imgdec_probes = probe;

	RETURN_LONG((zend_long) (uintptr_t) probe->bytes);
}

PHP_FUNCTION(imgdec_probe_read)
{
	zend_long address, size;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_LONG(address)
		Z_PARAM_LONG(size)
	ZEND_PARSE_PARAMETERS_END();

	imgdec_probe *probe = size < 0 ? NULL : imgdec_probe_find((uintptr_t) address);
	if (probe == NULL || (uintptr_t) address + (size_t) size > (uintptr_t) probe->bytes + probe->size) {
		zend_throw_exception(imgdec_ce_exception, "that address is not a probe allocation.", IMGDEC_CORRUPT);
		RETURN_THROWS();
	}

	RETURN_STRINGL((char *) (uintptr_t) address, (size_t) size);
}

PHP_FUNCTION(imgdec_probe_free)
{
	zend_long address;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(address)
	ZEND_PARSE_PARAMETERS_END();

	imgdec_probe **link = &imgdec_probes;
	while (*link != NULL) {
		if ((uintptr_t) (*link)->bytes == (uintptr_t) address) {
			imgdec_probe *probe = *link;
			*link = probe->next;
			efree(probe->bytes);
			efree(probe);
			return;
		}
		link = &(*link)->next;
	}

	zend_throw_exception(imgdec_ce_exception, "that address is not a probe allocation.", IMGDEC_CORRUPT);
	RETURN_THROWS();
}
#endif

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
