/*
 * JPEG through libjpeg with its defaults (the decode gd makes): RGB out,
 * grey widened to RGB by libjpeg, CMYK and YCCK out as CMYK and converted as
 * gd converts it, inverted first when an Adobe marker says the samples are.
 * libjpeg reports a fatal problem by calling error_exit, which longjmps.
 * Warnings (a cut-off scan among them) do not stop a decode.
 */

#include "core.h"

#include <stdio.h>
#include <setjmp.h>
#include <jpeglib.h>

typedef struct {
	struct jpeg_error_mgr manager;
	jmp_buf jump;
	imgdec_error *error;
} imgdec_jpeg_errors;

static void imgdec_jpeg_exit(j_common_ptr cinfo)
{
	imgdec_jpeg_errors *errors = (imgdec_jpeg_errors *) cinfo->err;
	char message[JMSG_LENGTH_MAX];

	cinfo->err->format_message(cinfo, message);
	imgdec_fail(errors->error, IMGDEC_CORRUPT, "%s.", message);
	longjmp(errors->jump, 1);
}

static void imgdec_jpeg_quiet(j_common_ptr cinfo)
{
	(void) cinfo;
}

imgdec_status imgdec_jpeg(const unsigned char *data, size_t length, imgdec_image *out, imgdec_error *error)
{
	struct jpeg_decompress_struct cinfo;
	imgdec_jpeg_errors errors;
	JSAMPLE *volatile row = NULL;
	zend_string *volatile pixels = NULL;

	cinfo.err = jpeg_std_error(&errors.manager);
	errors.manager.error_exit = imgdec_jpeg_exit;
	errors.manager.output_message = imgdec_jpeg_quiet;
	errors.error = error;

	if (setjmp(errors.jump)) {
		if (row != NULL) {
			efree(row);
		}
		if (pixels != NULL) {
			zend_string_release(pixels);
		}
		jpeg_destroy_decompress(&cinfo);
		return IMGDEC_CORRUPT;
	}

	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, data, (unsigned long) length);
	jpeg_read_header(&cinfo, TRUE);

	imgdec_status status = imgdec_check_size(cinfo.image_width, cinfo.image_height, error);
	if (status != IMGDEC_OK) {
		jpeg_destroy_decompress(&cinfo);
		return status;
	}

	bool cmyk = cinfo.jpeg_color_space == JCS_CMYK || cinfo.jpeg_color_space == JCS_YCCK;
	cinfo.out_color_space = cmyk ? JCS_CMYK : JCS_RGB;
	jpeg_start_decompress(&cinfo);

	uint32_t width = cinfo.output_width;
	uint32_t height = cinfo.output_height;
	int channels = cinfo.output_components;
	bool inverted = cinfo.saw_Adobe_marker;
	pixels = zend_string_alloc((size_t) width * height * 4, 0);
	row = emalloc((size_t) width * channels);

	unsigned char *to = (unsigned char *) ZSTR_VAL(pixels);
	while (cinfo.output_scanline < height) {
		JSAMPROW rows[1] = {row};
		jpeg_read_scanlines(&cinfo, rows, 1);
		for (uint32_t x = 0; x < width; x++, to += 4) {
			const JSAMPLE *p = row + (size_t) x * channels;
			if (cmyk) {
				int c = p[0], m = p[1], y = p[2], k = p[3];
				if (inverted) {
					c = 255 - c;
					m = 255 - m;
					y = 255 - y;
					k = 255 - k;
				}
				to[0] = (unsigned char) ((255 - c) * (255 - k) / 255);
				to[1] = (unsigned char) ((255 - m) * (255 - k) / 255);
				to[2] = (unsigned char) ((255 - y) * (255 - k) / 255);
			} else {
				to[0] = p[0];
				to[1] = p[1];
				to[2] = p[2];
			}
			to[3] = 0xFF;
		}
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	efree(row);
	ZSTR_VAL(pixels)[ZSTR_LEN(pixels)] = '\0';

	out->width = width;
	out->height = height;
	out->rgba8 = pixels;

	return IMGDEC_OK;
}
