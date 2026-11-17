/*
 * The header, and nothing past it. A file that ends once the header is present
 * still answers: PNG once IHDR is whole, JPEG once a SOF marker is whole, TIFF
 * once the first directory is whole. Pixel data is not read.
 */

#include "core.h"

#include <string.h>

static uint16_t imgdec_u16(const unsigned char *p, int be)
{
	return be ? (uint16_t) ((p[0] << 8) | p[1]) : (uint16_t) (p[0] | (p[1] << 8));
}

static uint32_t imgdec_u32(const unsigned char *p, int be)
{
	return be
		? ((uint32_t) p[0] << 24) | ((uint32_t) p[1] << 16) | ((uint32_t) p[2] << 8) | p[3]
		: (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16) | ((uint32_t) p[3] << 24);
}

static imgdec_status imgdec_facts_png(const unsigned char *data, size_t length, imgdec_facts *out, imgdec_error *error)
{
	static const unsigned char signature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};

	if (length < 8 || memcmp(data, signature, 8) != 0) {
		return imgdec_fail(error, IMGDEC_CORRUPT, "Not a PNG file.");
	}
	if (length < 8 + 8 + 13 || memcmp(data + 12, "IHDR", 4) != 0) {
		return imgdec_fail(error, IMGDEC_CORRUPT, "the data stops before the image does.");
	}

	uint32_t width = imgdec_u32(data + 16, 1);
	uint32_t height = imgdec_u32(data + 20, 1);
	unsigned char colour = data[25];
	imgdec_status status = imgdec_check_size(width, height, error);
	if (status != IMGDEC_OK) {
		return status;
	}

	bool alpha = colour == 4 || colour == 6;
	size_t at = 8;
	while (at + 8 <= length) {
		uint32_t count = imgdec_u32(data + at, 1);
		if (at + 12 + (size_t) count > length) {
			break;
		}
		if (memcmp(data + at + 4, "tRNS", 4) == 0) {
			alpha = true;
		}
		if (memcmp(data + at + 4, "IDAT", 4) == 0 || memcmp(data + at + 4, "IEND", 4) == 0) {
			break;
		}
		at += 12 + (size_t) count;
	}

	out->format = IMGDEC_FMT_PNG;
	out->width = width;
	out->height = height;
	out->frames = 1;
	out->alpha = alpha;

	return IMGDEC_OK;
}

static imgdec_status imgdec_facts_jpeg(const unsigned char *data, size_t length, imgdec_facts *out, imgdec_error *error)
{
	if (length < 2 || data[0] != 0xFF || data[1] != 0xD8) {
		return imgdec_fail(error, IMGDEC_CORRUPT, "Not a JPEG file.");
	}

	size_t i = 2;
	while (i + 1 < length) {
		if (data[i] != 0xFF) {
			return imgdec_fail(error, IMGDEC_CORRUPT, "Not a JPEG file.");
		}
		while (i < length && data[i] == 0xFF) {
			i++;
		}
		if (i >= length) {
			break;
		}
		unsigned char marker = data[i++];
		if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD9)) {
			continue;
		}
		if (i + 2 > length) {
			break;
		}
		unsigned int segment = ((unsigned int) data[i] << 8) | data[i + 1];
		int sof = marker >= 0xC0 && marker <= 0xCF && marker != 0xC4 && marker != 0xC8 && marker != 0xCC;
		if (sof) {
			if (segment < 8 || i + 6 >= length) {
				return imgdec_fail(error, IMGDEC_CORRUPT, "the data stops before the image does.");
			}
			uint32_t height = ((uint32_t) data[i + 3] << 8) | data[i + 4];
			uint32_t width = ((uint32_t) data[i + 5] << 8) | data[i + 6];
			imgdec_status status = imgdec_check_size(width, height, error);
			if (status != IMGDEC_OK) {
				return status;
			}
			out->format = IMGDEC_FMT_JPEG;
			out->width = width;
			out->height = height;
			out->frames = 1;
			out->alpha = false;

			return IMGDEC_OK;
		}
		if (segment < 2 || i + segment > length) {
			return imgdec_fail(error, IMGDEC_CORRUPT, "the data stops before the image does.");
		}
		i += segment;
	}

	return imgdec_fail(error, IMGDEC_CORRUPT, "the data stops before the image does.");
}

/* One SHORT or LONG from an IFD entry. A value that does not fit in the entry is read at its offset. */
static int imgdec_tiff_number(const unsigned char *data, size_t length, const unsigned char *entry, int be, uint32_t *number)
{
	uint16_t type = imgdec_u16(entry + 2, be);
	uint32_t count = imgdec_u32(entry + 4, be);
	unsigned int width = type == 3 ? 2u : type == 4 ? 4u : 0u;

	if (width == 0 || count < 1) {
		return 0;
	}
	if (count * width <= 4) {
		*number = width == 2 ? imgdec_u16(entry + 8, be) : imgdec_u32(entry + 8, be);
		return 1;
	}
	uint32_t offset = imgdec_u32(entry + 8, be);
	if ((size_t) offset + width > length) {
		return 0;
	}
	*number = width == 2 ? imgdec_u16(data + offset, be) : imgdec_u32(data + offset, be);

	return 1;
}

static imgdec_status imgdec_facts_tiff(const unsigned char *data, size_t length, imgdec_facts *out, imgdec_error *error)
{
	int be;

	if (length >= 4 && memcmp(data, "II*\0", 4) == 0) {
		be = 0;
	} else if (length >= 4 && memcmp(data, "MM\0*", 4) == 0) {
		be = 1;
	} else {
		return imgdec_fail(error, IMGDEC_CORRUPT, "Not a TIFF file.");
	}
	if (length < 8) {
		return imgdec_fail(error, IMGDEC_CORRUPT, "the data stops before the image does.");
	}

	uint32_t ifd = imgdec_u32(data + 4, be);
	uint32_t frames = 0;
	uint32_t width = 0, height = 0;
	bool alpha = false;

	while (ifd != 0 && frames < 1024) {
		if ((size_t) ifd + 2 > length) {
			if (frames == 0) {
				return imgdec_fail(error, IMGDEC_CORRUPT, "the data stops before the image does.");
			}
			break;
		}
		uint16_t count = imgdec_u16(data + ifd, be);
		size_t next_at = (size_t) ifd + 2 + (size_t) count * 12;
		if (next_at + 4 > length) {
			if (frames == 0) {
				return imgdec_fail(error, IMGDEC_CORRUPT, "the data stops before the image does.");
			}
			break;
		}
		if (frames == 0) {
			uint32_t extra = 0;
			bool saw_extra = false;
			for (uint16_t n = 0; n < count; n++) {
				const unsigned char *entry = data + ifd + 2 + (size_t) n * 12;
				uint16_t tag = imgdec_u16(entry, be);
				uint32_t number = 0;
				if (!imgdec_tiff_number(data, length, entry, be, &number)) {
					continue;
				}
				if (tag == 256) {
					width = number;
				} else if (tag == 257) {
					height = number;
				} else if (tag == 338) {
					extra = number;
					saw_extra = true;
				}
			}
			imgdec_status status = imgdec_check_size(width, height, error);
			if (status != IMGDEC_OK) {
				return status;
			}
			alpha = saw_extra && (extra == 1 || extra == 2);
		}
		frames++;
		uint32_t next = imgdec_u32(data + next_at, be);
		if (next == ifd) {
			break;
		}
		ifd = next;
	}
	if (frames == 0) {
		return imgdec_fail(error, IMGDEC_CORRUPT, "Not a TIFF file.");
	}

	out->format = IMGDEC_FMT_TIFF;
	out->width = width;
	out->height = height;
	out->frames = frames;
	out->alpha = alpha;

	return IMGDEC_OK;
}

imgdec_status imgdec_read_facts(const unsigned char *data, size_t length, imgdec_facts *out, imgdec_error *error)
{
	if (length >= 8 && data[0] == 0x89 && data[1] == 'P') {
		return imgdec_facts_png(data, length, out, error);
	}
	if (length >= 2 && data[0] == 0xFF && data[1] == 0xD8) {
		return imgdec_facts_jpeg(data, length, out, error);
	}
	if (length >= 4 && (memcmp(data, "II*\0", 4) == 0 || memcmp(data, "MM\0*", 4) == 0)) {
		return imgdec_facts_tiff(data, length, out, error);
	}

	return imgdec_fail(error, IMGDEC_CORRUPT, "The bytes are not a PNG, JPEG or TIFF image.");
}

void imgdec_store(const unsigned char *rgba, uint32_t width, uint32_t height, unsigned char *dest, size_t stride, int order)
{
	for (uint32_t y = 0; y < height; y++) {
		unsigned char *row = dest + (size_t) y * stride;
		const unsigned char *from = rgba + (size_t) y * width * 4;
		if (order == IMGDEC_RGBA) {
			memcpy(row, from, (size_t) width * 4);
			continue;
		}
		for (uint32_t x = 0; x < width; x++) {
			row[x * 4] = from[x * 4 + 2];
			row[x * 4 + 1] = from[x * 4 + 1];
			row[x * 4 + 2] = from[x * 4];
			row[x * 4 + 3] = from[x * 4 + 3];
		}
	}
}
