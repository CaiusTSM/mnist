/*
 * Usage:
 *
 * In ONE of your source files define the macro MNIST_IMPL before
 * including this header like so:
 * 
 * #define MNIST_IMPL
 * #include "mnist.h"
 *
 * Everywhere else you can simply include the header (do not define MNIST_IMPL again).
 * See the API section for the available functions.
 *
 * ---- Example code: ----
 *
 * #define MNIST_IMPL
 * #include "mnist.h"
 *
 * int main(int argc, char *argv[]) {
 *	mnist_labels labels = mnist_load_labels("../../mnist/train-labels-idx1-ubyte");
 *  if (!labels.valid) {
 *    return 1;
 *  }
 *  mnist_images images = mnist_load_images("../../mnist/train-images-idx3-ubyte");
 *  if (!images.valid) {
 *    return 1;
 *  }
 *  println("{int}", mnist_get_label(labels, 0));
 *  mnist_print_image(mnist_get_image(images, 0), 127);
 * }
 *
 * -----------------------
 *
 * Notes:
 *
 * The label and image files are stored in big endiann (MSB first).
 * They both start with unsigned 32 bit magic numbers:
 * 0x00000801 (2049) - label file magic number
 * 0x00000803 (2051) - image file magic number
 *
 * The MNIST images have 0 as white and 255 as black.
 * This loader does not automatically flip that to be 255 for white
 * and 0 for black. The 255 for black is due to black pen on white paper (how much ink, not the value).
 *
 */

#ifndef MNIST_INCLUDE_HEADER
#define MNIST_INCLUDE_HEADER

#ifndef u32
#define u32 unsigned int
#endif

typedef struct mnist_labels {
	int count;
	unsigned char *data;
	int valid;
} mnist_labels;

typedef struct mnist_image {
	int rows;
	int columns;
	unsigned char *data;
} mnist_image;

typedef struct mnist_images {
	int count;
	int rows;
	int columns;
	unsigned char *data;
	int valid;
} mnist_images;

// ------------------- API -------------------
// Returns a contiguous array of single byte labels.
mnist_labels mnist_load_labels(const char *filepath);
// Returns a label from the list of labels given.
unsigned char mnist_get_label(mnist_labels labels, int index);
// Returns image data as a contiguous array of grayscale images (0 is white, 255 is black).
mnist_images mnist_load_images(const char *filepath);
// Returns an image from the list of images given.
mnist_image mnist_get_image(mnist_images images, int index);
// Prints an mnist digit based on the given threshold.
void mnist_print_image(mnist_image image, unsigned char threshold);
// -------------------------------------------

#ifdef MNIST_IMPL

#include <stdio.h>
#include <stdlib.h>

#ifndef MAGIC_NUMBER_LABELS
#define MAGIC_NUMBER_LABELS ((u32)0x00000801)
#endif

#ifndef MAGIC_NUMBER_IMAGES
#define MAGIC_NUMBER_IMAGES ((u32)0x00000803)
#endif

typedef struct mnist_labels_header {
	u32 magic_number;
	u32 number_of_items;
} mnist_labels_header;

typedef struct mnist_images_header {
	u32 magic_number;
	u32 number_of_images;
	u32 number_of_rows;
	u32 number_of_columns;
} mnist_images_header;

static int is_big_endian() {
	int v = 1;
	/*
	 * v =
	 * +---+---+---+---+
	 * |0x1|0x0|0x0|0x0|
	 * +---+---+---+---+
	 * when big endian
	 *
	 * v =
	 * +---+---+---+---+
	 * |0x0|0x0|0x0|0x1|
	 * +---+---+---+---+
	 * when little endian
	 *
	 * "*(char *)(&v)" gets the first (left)
	 * byte of the above sequence
	 *
	 */
	return ((*(char *)(&v)) == 0);
}

static int swap_endianness_u32(u32 v) {
	int ret = 0;

	ret |= v >> 24; // byte 0 to byte 3
	ret |= (v & 0xFF0000) >> 8; // byte 1 to byte 2
	ret |= (v & 0xFF00) << 8; // byte 2 to byte 1
	ret |= v << 24; // byte 3 to byte 0

	return ret;
}

mnist_labels mnist_load_labels(const char *filepath) {
	mnist_labels labels = {0};
	unsigned char *label_data = 0;
	FILE *fp = 0;
	mnist_labels_header header = {0};

	if (filepath == 0) {
		return labels;
	}

	fp = fopen(filepath, "rb");

	if (!fp) {
		return labels;
	}

	if (!fread(&header.magic_number, sizeof(u32), 1, fp)) {
		fclose(fp);
		return labels;
	}

	if (!fread(&header.number_of_items, sizeof(u32), 1, fp)) {
		fclose(fp);
		return labels;
	}

	if (!is_big_endian()) {
		header.magic_number = swap_endianness_u32(header.magic_number);
		header.number_of_items = swap_endianness_u32(header.number_of_items);
	}

	if (header.magic_number != MAGIC_NUMBER_LABELS) {
		fclose(fp);
		return labels;
	}

	label_data = (unsigned char *)malloc(sizeof(unsigned char) * header.number_of_items);

	if (label_data == 0) {
		return labels;
	}

	if (fread(label_data, sizeof(unsigned char), header.number_of_items, fp) != header.number_of_items) {
		fclose(fp);
		free(label_data);
		return labels;
	}

	fclose(fp);

	labels.count = header.number_of_items;
	labels.data = label_data;
	labels.valid = 1;
	return labels;
}

unsigned char mnist_get_label(mnist_labels labels, int index) {
	return labels.data[index];
}

mnist_images mnist_load_images(const char *filepath) {
	mnist_images images = {0};
	unsigned char *image_data = 0;
	FILE *fp = 0;
	mnist_images_header header = {0};

	if (filepath == 0) {
		return images;
	}

	fp = fopen(filepath, "rb");

	if (!fp) {
		return images;
	}

	if (!fread(&header.magic_number, sizeof(u32), 1, fp)) {
		fclose(fp);
		return images;
	}

	if (!fread(&header.number_of_images, sizeof(u32), 1, fp)) {
		fclose(fp);
		return images;
	}

	if (!fread(&header.number_of_rows, sizeof(u32), 1, fp)) {
		fclose(fp);
		return images;
	}

	if (!fread(&header.number_of_columns, sizeof(u32), 1, fp)) {
		fclose(fp);
		return images;
	}

	if (!is_big_endian()) {
		header.magic_number = swap_endianness_u32(header.magic_number);
		header.number_of_images = swap_endianness_u32(header.number_of_images);
		header.number_of_rows = swap_endianness_u32(header.number_of_rows);
		header.number_of_columns = swap_endianness_u32(header.number_of_columns);
	}

	if (header.magic_number != MAGIC_NUMBER_IMAGES) {
		fclose(fp);
		return images;
	}

	{
		int size = header.number_of_images * header.number_of_rows * header.number_of_columns;

		image_data = (unsigned char *)malloc(sizeof(unsigned char) * size);

		if (image_data == 0) {
			return images;
		}

		if (fread(image_data, sizeof(unsigned char), size, fp) != size) {
			fclose(fp);
			free(image_data);
			return images;
		}
	}

	fclose(fp);

	images.count = header.number_of_images;
	images.rows = header.number_of_rows;
	images.columns = header.number_of_columns;
	images.data = image_data;
	images.valid = 1;
	return images;
}

mnist_image mnist_get_image(mnist_images images, int index) {
	mnist_image image;
	image.rows = images.rows;
	image.columns = images.columns;
	image.data = &images.data[images.rows * images.columns * index];
	return image;
}

void mnist_print_image(mnist_image image, unsigned char threshold) {
	for (int row = 0; row < image.rows; ++row) {
		for (int col = 0; col < image.columns; ++col) {
			unsigned char v = image.data[col + row * image.columns];
			if (v >= threshold) {
				printf("##");
			}
			else {
				printf("  ");
			}
		}
		printf("\n");
	}
	printf("\n");
}

#endif

#endif
