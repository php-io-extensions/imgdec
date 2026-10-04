/*
 * Classic TIFF through libtiff, by the same layout rules (and the same
 * refusal messages) as Surface's PHP reader: the first image; grey of either
 * polarity, RGB or palette; chunky or separate planes; orientation top-left;
 * fill order MSB first; unsigned samples; predictor 1, or 2 on 8 or 16 bits.
 * libtiff decompresses (any codec it has) and undoes the predictor, handing
 * 16-bit samples back in host order; the rest happens here.
 */

#include "core.h"

#include <string.h>
#include <tiffio.h>

typedef struct {
	const unsigned char *data;
	toff_t length;
	toff_t at;
} imgdec_tiff_source;

static tmsize_t imgdec_tiff_read(thandle_t handle, void *into, tmsize_t count)
{
	imgdec_tiff_source *source = (imgdec_tiff_source *) handle;
	toff_t left = source->at >= source->length ? 0 : source->length - source->at;
	tmsize_t take = (toff_t) count > left ? (tmsize_t) left : count;

	memcpy(into, source->data + source->at, (size_t) take);
	source->at += take;

	return take;
}

static tmsize_t imgdec_tiff_write(thandle_t handle, void *from, tmsize_t count)
{
	(void) handle;
	(void) from;
	(void) count;

	return 0;
}

static toff_t imgdec_tiff_seek(thandle_t handle, toff_t offset, int whence)
{
	imgdec_tiff_source *source = (imgdec_tiff_source *) handle;

	switch (whence) {
		case SEEK_SET: source->at = offset; break;
		case SEEK_CUR: source->at += offset; break;
		case SEEK_END: source->at = source->length + offset; break;
		default: return (toff_t) -1;
	}

	return source->at;
}

static int imgdec_tiff_close(thandle_t handle)
{
	(void) handle;

	return 0;
}

static toff_t imgdec_tiff_size(thandle_t handle)
{
	return ((imgdec_tiff_source *) handle)->length;
}

/* libtiff's error handler for this handle: keep the first message. */
static int imgdec_tiff_error(TIFF *tif, void *user, const char *module, const char *format, va_list args)
{
	imgdec_error *error = (imgdec_error *) user;

	(void) tif;
	(void) module;
	if (error->message[0] == '\0') {
		vsnprintf(error->message, sizeof(error->message), format, args);
	}

	return 1;
}

static int imgdec_tiff_warning(TIFF *tif, void *user, const char *module, const char *format, va_list args)
{
	(void) tif;
	(void) user;
	(void) module;
	(void) format;
	(void) args;

	return 1;
}

typedef struct {
	uint16_t photometric;
	uint16_t bits;
	uint16_t spp;
	uint16_t colours;   /* samples that are colour: 1 or 3 */
	uint16_t alpha;     /* 0, or EXTRASAMPLE_ASSOCALPHA (1) or EXTRASAMPLE_UNASSALPHA (2) */
	uint16_t planar;
	uint16_t *map[3];   /* palette colour map, libtiff's */
} imgdec_tiff_layout;

/* The refusals, in Surface's TiffReader order and words. */
static imgdec_status imgdec_tiff_layout_of(TIFF *tif, imgdec_tiff_layout *layout, imgdec_error *error)
{
	uint16_t format = SAMPLEFORMAT_UINT, orientation = ORIENTATION_TOPLEFT, fill = FILLORDER_MSB2LSB, predictor = PREDICTOR_NONE;
	uint16_t extras = 0, *extra = NULL;
	const char *name;
	bool depth_ok;

	TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &layout->spp);
	TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &layout->bits);
	if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &layout->photometric)) {
		return imgdec_fail(error, IMGDEC_CORRUPT, "it lacks tag 262.");
	}
	TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLEFORMAT, &format);
	if (format != SAMPLEFORMAT_UINT) {
		return imgdec_fail(error, IMGDEC_UNSUPPORTED, "sample format %u", format);
	}
	TIFFGetFieldDefaulted(tif, TIFFTAG_ORIENTATION, &orientation);
	if (orientation != ORIENTATION_TOPLEFT) {
		return imgdec_fail(error, IMGDEC_UNSUPPORTED, "orientation %u", orientation);
	}
	TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &layout->planar);
	if (layout->planar != PLANARCONFIG_CONTIG && layout->planar != PLANARCONFIG_SEPARATE) {
		return imgdec_fail(error, IMGDEC_UNSUPPORTED, "planar configuration %u", layout->planar);
	}
	TIFFGetFieldDefaulted(tif, TIFFTAG_FILLORDER, &fill);
	if (fill != FILLORDER_MSB2LSB) {
		return imgdec_fail(error, IMGDEC_UNSUPPORTED, "fill order %u", fill);
	}

	switch (layout->photometric) {
		case PHOTOMETRIC_MINISWHITE:
		case PHOTOMETRIC_MINISBLACK:
			layout->colours = 1;
			name = "grey";
			depth_ok = layout->bits == 1 || layout->bits == 2 || layout->bits == 4 || layout->bits == 8 || layout->bits == 16;
			break;
		case PHOTOMETRIC_RGB:
			layout->colours = 3;
			name = "RGB";
			depth_ok = layout->bits == 8 || layout->bits == 16;
			break;
		case PHOTOMETRIC_PALETTE:
			layout->colours = 1;
			name = "palette";
			depth_ok = layout->bits == 1 || layout->bits == 2 || layout->bits == 4 || layout->bits == 8;
			break;
		default:
			return imgdec_fail(error, IMGDEC_UNSUPPORTED, "photometric %u", layout->photometric);
	}
	if (!depth_ok) {
		return imgdec_fail(error, IMGDEC_UNSUPPORTED, "%u-bit %s", layout->bits, name);
	}
	if (layout->spp < layout->colours) {
		return imgdec_fail(error, IMGDEC_UNSUPPORTED, "%s with %u samples per pixel", name, layout->spp);
	}
	if (layout->bits < 8 && layout->spp > 1) {
		return imgdec_fail(error, IMGDEC_UNSUPPORTED, "%u-bit samples with extra samples", layout->bits);
	}
	if (layout->photometric == PHOTOMETRIC_PALETTE && layout->spp > 1) {
		return imgdec_fail(error, IMGDEC_UNSUPPORTED, "palette with extra samples");
	}
	/* libtiff knows the tag only under a codec that uses it (LZW, Deflate); elsewhere there is none. */
	if (!TIFFGetField(tif, TIFFTAG_PREDICTOR, &predictor) || predictor == 0) {
		predictor = PREDICTOR_NONE;
	}
	if (predictor != PREDICTOR_NONE && (predictor != PREDICTOR_HORIZONTAL || layout->bits < 8)) {
		return imgdec_fail(error, IMGDEC_UNSUPPORTED, "predictor %u on %u-bit samples", predictor, layout->bits);
	}

	layout->alpha = 0;
	if (layout->spp > layout->colours && TIFFGetField(tif, TIFFTAG_EXTRASAMPLES, &extras, &extra) && extras > 0
			&& (extra[0] == EXTRASAMPLE_ASSOCALPHA || extra[0] == EXTRASAMPLE_UNASSALPHA)) {
		layout->alpha = extra[0];
	}
	if (layout->photometric == PHOTOMETRIC_PALETTE
			&& !TIFFGetField(tif, TIFFTAG_COLORMAP, &layout->map[0], &layout->map[1], &layout->map[2])) {
		return imgdec_fail(error, IMGDEC_CORRUPT, "its colour map is missing.");
	}

	return IMGDEC_OK;
}

/* Sample $index of a row of $bits-bit samples (MSB first below a byte, host order at 16). */
static inline uint32_t imgdec_tiff_sample(const unsigned char *row, uint32_t index, uint16_t bits)
{
	switch (bits) {
		case 8: return row[index];
		case 16: return ((const uint16_t *) row)[index];
		default: {
			uint32_t bit = index * bits;
			return (row[bit >> 3] >> (8 - bits - (bit & 7))) & ((1u << bits) - 1);
		}
	}
}

/* An 8-bit value of a sample: the high byte at 16 bits, widened across 0..255 below 8 (as $scale asks). */
static inline unsigned char imgdec_tiff_byte(uint32_t value, uint16_t bits, bool scale)
{
	if (bits == 16) {
		return (unsigned char) (value >> 8);
	}
	if (bits < 8 && scale) {
		return (unsigned char) (value * 255 / ((1u << bits) - 1));
	}

	return (unsigned char) value;
}

/*
 * One image row into RGBA8. $planes holds the row of each plane (one for
 * chunky samples, where every sample of a pixel sits together).
 */
static void imgdec_tiff_row(const imgdec_tiff_layout *layout, unsigned char **planes, uint32_t width, unsigned char *to)
{
	bool chunky = layout->planar == PLANARCONFIG_CONTIG;

	for (uint32_t x = 0; x < width; x++, to += 4) {
		#define SAMPLE(s) (chunky ? imgdec_tiff_sample(planes[0], x * layout->spp + (s), layout->bits) : imgdec_tiff_sample(planes[(s)], x, layout->bits))
		if (layout->photometric == PHOTOMETRIC_PALETTE) {
			uint32_t index = SAMPLE(0);
			to[0] = (unsigned char) (layout->map[0][index] >> 8);
			to[1] = (unsigned char) (layout->map[1][index] >> 8);
			to[2] = (unsigned char) (layout->map[2][index] >> 8);
			to[3] = 0xFF;
			continue;
		}
		if (layout->photometric == PHOTOMETRIC_RGB) {
			to[0] = imgdec_tiff_byte(SAMPLE(0), layout->bits, true);
			to[1] = imgdec_tiff_byte(SAMPLE(1), layout->bits, true);
			to[2] = imgdec_tiff_byte(SAMPLE(2), layout->bits, true);
		} else {
			unsigned char grey = imgdec_tiff_byte(SAMPLE(0), layout->bits, true);
			if (layout->photometric == PHOTOMETRIC_MINISWHITE) {
				grey = 255 - grey;
			}
			to[0] = to[1] = to[2] = grey;
		}
		to[3] = layout->alpha ? imgdec_tiff_byte(SAMPLE(layout->colours), layout->bits, true) : 0xFF;
		if (layout->alpha == EXTRASAMPLE_ASSOCALPHA) {
			unsigned int a = to[3];
			for (int c = 0; c < 3; c++) {
				unsigned int straight = a == 0 ? 0 : (to[c] * 255u + a / 2) / a;
				to[c] = (unsigned char) (straight > 255 ? 255 : straight);
			}
		}
		#undef SAMPLE
	}
}

imgdec_status imgdec_tiff(const unsigned char *data, size_t length, imgdec_image *out, imgdec_error *error)
{
	imgdec_tiff_source source = {data, (toff_t) length, 0};
	imgdec_tiff_layout layout = {0};
	TIFFOpenOptions *options = TIFFOpenOptionsAlloc();
	imgdec_status status;

	TIFFOpenOptionsSetErrorHandlerExtR(options, imgdec_tiff_error, error);
	TIFFOpenOptionsSetWarningHandlerExtR(options, imgdec_tiff_warning, NULL);
	TIFF *tif = TIFFClientOpenExt("imgdec", "rm", (thandle_t) &source, imgdec_tiff_read, imgdec_tiff_write,
		imgdec_tiff_seek, imgdec_tiff_close, imgdec_tiff_size, NULL, NULL, options);
	TIFFOpenOptionsFree(options);
	if (tif == NULL) {
		return imgdec_fail(error, IMGDEC_CORRUPT, "%s", error->message[0] ? error->message : "libtiff could not open it.");
	}

	uint32_t width = 0, height = 0;
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
	if ((status = imgdec_check_size(width, height, error)) != IMGDEC_OK
			|| (status = imgdec_tiff_layout_of(tif, &layout, error)) != IMGDEC_OK) {
		TIFFClose(tif);
		return status;
	}

	uint16_t planes = layout.planar == PLANARCONFIG_SEPARATE ? layout.spp : 1;
	/* Strips are blocks the image's width across; tiles are blocks of their own size. */
	bool tiled = TIFFIsTiled(tif);
	uint32_t block_width = width, block_height = height;
	if (tiled) {
		TIFFGetField(tif, TIFFTAG_TILEWIDTH, &block_width);
		TIFFGetField(tif, TIFFTAG_TILELENGTH, &block_height);
	} else {
		TIFFGetFieldDefaulted(tif, TIFFTAG_ROWSPERSTRIP, &block_height);
		block_height = block_height > height ? height : block_height;
	}
	tmsize_t block_size = tiled ? TIFFTileSize(tif) : TIFFStripSize(tif);
	tmsize_t row_size = tiled ? TIFFTileRowSize(tif) : TIFFScanlineSize(tif);
	size_t image_row = ((size_t) width * (planes > 1 ? 1 : layout.spp) * layout.bits + 7) / 8;
	if (block_size <= 0 || row_size <= 0 || block_width == 0 || block_height == 0) {
		TIFFClose(tif);
		return imgdec_fail(error, IMGDEC_CORRUPT, "its strips or tiles have no size.");
	}

	/* A band of block_height image rows per plane, assembled from strips or tiles, then converted row by row. */
	unsigned char *band = emalloc((size_t) planes * block_height * image_row);
	unsigned char *block = emalloc((size_t) block_size);
	zend_string *pixels = zend_string_alloc((size_t) width * height * 4, 0);
	unsigned char **plane_rows = emalloc(sizeof(unsigned char *) * planes);
	status = IMGDEC_OK;

	for (uint32_t top = 0; top < height && status == IMGDEC_OK; top += block_height) {
		uint32_t rows = top + block_height > height ? height - top : block_height;
		for (uint16_t p = 0; p < planes && status == IMGDEC_OK; p++) {
			unsigned char *plane_band = band + (size_t) p * block_height * image_row;
			for (uint32_t left = 0; left < width; left += block_width) {
				tmsize_t read = tiled
					? TIFFReadEncodedTile(tif, TIFFComputeTile(tif, left, top, 0, p), block, block_size)
					: TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, top, p), block, block_size);
				if (read < (tmsize_t) (tiled ? block_size : row_size * rows)) {
					status = imgdec_fail(error, IMGDEC_CORRUPT, "%s", error->message[0] ? error->message : "a block holds less data than its rows need.");
					break;
				}
				size_t from = (size_t) left * (planes > 1 ? 1 : layout.spp) * layout.bits / 8;
				size_t take = from + (size_t) row_size > image_row ? image_row - from : (size_t) row_size;
				for (uint32_t r = 0; r < rows; r++) {
					memcpy(plane_band + (size_t) r * image_row + from, block + (size_t) r * row_size, take);
				}
			}
		}
		for (uint32_t r = 0; r < rows && status == IMGDEC_OK; r++) {
			for (uint16_t p = 0; p < planes; p++) {
				plane_rows[p] = band + ((size_t) p * block_height + r) * image_row;
			}
			imgdec_tiff_row(&layout, plane_rows, width, (unsigned char *) ZSTR_VAL(pixels) + ((size_t) (top + r) * width * 4));
		}
	}

	efree(plane_rows);
	efree(block);
	efree(band);
	TIFFClose(tif);
	if (status != IMGDEC_OK) {
		zend_string_release(pixels);
		return status;
	}
	ZSTR_VAL(pixels)[ZSTR_LEN(pixels)] = '\0';

	out->width = width;
	out->height = height;
	out->rgba8 = pixels;

	return IMGDEC_OK;
}
