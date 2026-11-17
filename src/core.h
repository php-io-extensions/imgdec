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

typedef enum {
	IMGDEC_FMT_PNG = 1,
	IMGDEC_FMT_JPEG = 2,
	IMGDEC_FMT_TIFF = 3,
} imgdec_format;

typedef struct {
	imgdec_format format;
	uint32_t width;
	uint32_t height;
	uint32_t frames;
	bool alpha;
} imgdec_facts;

typedef struct {
	uint32_t width;
	uint32_t height;
	unsigned char *rgba8;    /* emalloc'd width × height × 4, straight RGBA8; NULL on failure */
} imgdec_image;

typedef struct {
	char message[256];
} imgdec_error;

#define IMGDEC_RGBA 0
#define IMGDEC_BGRA 1

imgdec_status imgdec_read_facts(const unsigned char *data, size_t length, imgdec_facts *out, imgdec_error *error);
imgdec_status imgdec_png(const unsigned char *data, size_t length, imgdec_image *out, imgdec_error *error);
imgdec_status imgdec_jpeg(const unsigned char *data, size_t length, imgdec_image *out, imgdec_error *error);
imgdec_status imgdec_tiff(const unsigned char *data, size_t length, uint32_t frame, imgdec_image *out, imgdec_error *error);

/* Packed RGBA8 rows copied onto $dest, one every $stride bytes. $order is IMGDEC_RGBA or IMGDEC_BGRA. */
void imgdec_store(const unsigned char *rgba, uint32_t width, uint32_t height, unsigned char *dest, size_t stride, int order);

/* IMGDEC_TOO_LARGE with the message, or IMGDEC_OK. */
imgdec_status imgdec_check_size(uint64_t width, uint64_t height, imgdec_error *error);

/* Format a message into the error and answer the status, for one-line returns. */
imgdec_status imgdec_fail(imgdec_error *error, imgdec_status status, const char *format, ...);

#endif
