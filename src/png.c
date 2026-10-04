/*
 * PNG through libpng's full read API: palettes and grey expanded, tRNS made
 * alpha (matched on all 16 bits, before the cut), 16-bit samples cut to their
 * high byte, interlacing undone, no gamma applied, opaque images filled with
 * alpha 255. libpng reports a fatal problem by longjmp out of its error hook.
 */

#include "core.h"

#include <png.h>
#include <setjmp.h>
#include <string.h>

typedef struct {
	const unsigned char *data;
	size_t length;
	size_t at;
	imgdec_error *error;
} imgdec_png_source;

static void imgdec_png_read(png_structp png, png_bytep into, png_size_t count)
{
	imgdec_png_source *source = png_get_io_ptr(png);

	if (count > source->length - source->at) {
		png_error(png, "the data stops before the image does");
	}
	memcpy(into, source->data + source->at, count);
	source->at += count;
}

static void imgdec_png_error(png_structp png, png_const_charp message)
{
	imgdec_png_source *source = png_get_error_ptr(png);

	imgdec_fail(source->error, IMGDEC_CORRUPT, "%s.", message);
	png_longjmp(png, 1);
}

static void imgdec_png_warning(png_structp png, png_const_charp message)
{
	(void) png;
	(void) message;
}

imgdec_status imgdec_png(const unsigned char *data, size_t length, imgdec_image *out, imgdec_error *error)
{
	imgdec_png_source source = {data, length, 0, error};
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, &source, imgdec_png_error, imgdec_png_warning);
	png_infop info = png == NULL ? NULL : png_create_info_struct(png);
	png_bytep *volatile rows = NULL;
	zend_string *volatile pixels = NULL;
	imgdec_status status = IMGDEC_OK;

	if (info == NULL) {
		png_destroy_read_struct(&png, NULL, NULL);
		return imgdec_fail(error, IMGDEC_CORRUPT, "libpng could not start.");
	}

	if (setjmp(png_jmpbuf(png))) {
		if (rows != NULL) {
			efree(rows);
		}
		if (pixels != NULL) {
			zend_string_release(pixels);
		}
		png_destroy_read_struct(&png, &info, NULL);
		return IMGDEC_CORRUPT;
	}

	png_set_read_fn(png, &source, imgdec_png_read);
	png_read_info(png, info);

	png_uint_32 width = png_get_image_width(png, info);
	png_uint_32 height = png_get_image_height(png, info);
	if ((status = imgdec_check_size(width, height, error)) != IMGDEC_OK) {
		png_destroy_read_struct(&png, &info, NULL);
		return status;
	}

	int colour = png_get_color_type(png, info);
	png_set_expand(png);
	png_set_strip_16(png);
	if (colour == PNG_COLOR_TYPE_GRAY || colour == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png);
	}
	if (!(colour & PNG_COLOR_MASK_ALPHA) && !png_get_valid(png, info, PNG_INFO_tRNS)) {
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
	}
	png_set_interlace_handling(png);
	png_read_update_info(png, info);
	if (png_get_rowbytes(png, info) != (png_size_t) width * 4) {
		png_destroy_read_struct(&png, &info, NULL);
		return imgdec_fail(error, IMGDEC_UNSUPPORTED, "colour type %d at %d bits did not come out as RGBA8", colour, png_get_bit_depth(png, info));
	}

	pixels = zend_string_alloc((size_t) width * height * 4, 0);
	rows = emalloc(sizeof(png_bytep) * height);
	for (png_uint_32 y = 0; y < height; y++) {
		rows[y] = (png_bytep) ZSTR_VAL(pixels) + (size_t) y * width * 4;
	}
	png_read_image(png, rows);
	png_read_end(png, NULL);

	efree(rows);
	ZSTR_VAL(pixels)[ZSTR_LEN(pixels)] = '\0';
	png_destroy_read_struct(&png, &info, NULL);

	out->width = width;
	out->height = height;
	out->rgba8 = pixels;

	return IMGDEC_OK;
}
