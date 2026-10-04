/*
 * The decoders: PNG, JPEG or TIFF bytes into straight RGBA8, top-left first.
 * Each fills an imgdec_image (pixels in a zend_string it allocates) or says
 * why not in an imgdec_error. They share the size limits and the rules below.
 */

#ifndef IMGDEC_CORE_H
#define IMGDEC_CORE_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include <stdint.h>

#define IMGDEC_MAX_SIDE   65535
#define IMGDEC_MAX_PIXELS (1u << 26)

typedef enum {
	IMGDEC_OK = 0,
	IMGDEC_CORRUPT = 1,      /* the bytes are not a readable image */
	IMGDEC_UNSUPPORTED = 2,  /* a readable image in a layout the rules leave out */
	IMGDEC_TOO_LARGE = 3,    /* past IMGDEC_MAX_SIDE or IMGDEC_MAX_PIXELS */
} imgdec_status;

typedef struct {
	uint32_t width;
	uint32_t height;
	zend_string *rgba8;      /* width × height × 4 bytes; NULL on failure */
} imgdec_image;

typedef struct {
	char message[256];
} imgdec_error;

imgdec_status imgdec_png(const unsigned char *data, size_t length, imgdec_image *out, imgdec_error *error);
imgdec_status imgdec_jpeg(const unsigned char *data, size_t length, imgdec_image *out, imgdec_error *error);
imgdec_status imgdec_tiff(const unsigned char *data, size_t length, imgdec_image *out, imgdec_error *error);

/* IMGDEC_TOO_LARGE with the message, or IMGDEC_OK. */
imgdec_status imgdec_check_size(uint64_t width, uint64_t height, imgdec_error *error);

/* Format a message into the error and answer the status, for one-line returns. */
imgdec_status imgdec_fail(imgdec_error *error, imgdec_status status, const char *format, ...);

#endif
