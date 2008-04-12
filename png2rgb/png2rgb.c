/* Copyright (c) 2007 Mega Man */
#include <png.h>
#include <stdint.h>
#include <malloc.h>
#include <ctype.h>
#include <stdlib.h>

 /* The png_jmpbuf() macro, used in error handling, became available in
  * libpng version 1.0.6.  If you want to be able to run your code with older
  * versions of libpng, you must define the macro yourself (but only if it
  * is not already defined by libpng!).
  */

#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

/* Read a PNG file. */
uint8_t *loadPng(const char *file_name, uint16_t * width, uint16_t * height,
	uint16_t * depth)

/* We need to open the file */
{
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	FILE *fp;
	int x, y;
	uint8_t *dest;

	if ((fp = fopen(file_name, "rb")) == NULL)
		return NULL;

	/* Create and initialize the png_struct with the desired error handler
	 * functions. Use the default stderr and longjump method.
	 */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL) {
		fclose(fp);
		return NULL;
	}

	/* Allocate/initialize the memory for image information. */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}

	/* Set error handling if you are using the setjmp/longjmp method (this is
	 * the normal method of doing things with libpng).
	 */
	if (setjmp(png_jmpbuf(png_ptr))) {
		/* Free all of the memory associated with the png_ptr and info_ptr */
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		fclose(fp);
		/* If we get here, we had a problem reading the file */
		return NULL;
	}

	/* Set up the input control if you are using standard C streams */
	png_init_io(png_ptr, fp);

	/* If we have already read some of the signature */
	png_set_sig_bytes(png_ptr, sig_read);

	/* Read entire image. */
	/* XXX: Assuming host is litle endian system like ps2 or x86. */
	png_read_png(png_ptr, info_ptr,
		PNG_TRANSFORM_SWAP_ENDIAN /* png_transforms */ , png_voidp_NULL);

	/* At this point you have read the entire image */
	printf("Width: %d Height: %d Depth: %d\n", (int) info_ptr->width,
		(int) info_ptr->height, (int) (info_ptr->pixel_depth / 8));

	*width = info_ptr->width;
	*height = info_ptr->height;
	*depth = info_ptr->pixel_depth / 8;
	if (info_ptr->color_type == PNG_COLOR_TYPE_RGBA) {
		if (info_ptr->pixel_depth != 32) {
			fprintf(stderr, "Color depth wrong %d (32 required).\n", info_ptr->pixel_depth);

			/* clean up after the read, and free any memory allocated - REQUIRED */
			png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
			return NULL;
		}
	} else {
		if (info_ptr->pixel_depth != 24) {
			fprintf(stderr, "Color depth wrong %d (24 required).\n", info_ptr->pixel_depth);

			/* clean up after the read, and free any memory allocated - REQUIRED */
			png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
			return NULL;
		}
	}

	/* close the file */
	fclose(fp);

	dest = malloc(info_ptr->width * info_ptr->height * *depth);
	memset(dest, 0, info_ptr->width * info_ptr->height * *depth);

	if (dest != NULL) {
		/* Copy picture into destination memory and fix alpha channel. */
		for (y = 0; y < info_ptr->height; y++) {
			unsigned char *p;

			p = info_ptr->row_pointers[y];
			for (x = 0; x < info_ptr->width; x++) {
				unsigned char alpha;
				int pos;

				if (info_ptr->color_type == PNG_COLOR_TYPE_RGBA) {
					/* Value range 0 .. 255 changed to range 0 .. 128. */
					alpha = p[3] >> 1;
					if (alpha != 0)
						alpha++;
				} else {
					alpha = 0x80;
				}
				alpha = 0x80 - alpha;

				pos = *depth * (y * info_ptr->width + x);
				/* XXX: Assuming host is litle endian system like ps2 or x86. */
				dest[pos + 0] = p[0];
				dest[pos + 1] = p[1];
				dest[pos + 2] = p[2];
				if (info_ptr->color_type == PNG_COLOR_TYPE_RGBA) {
					dest[pos + 3] = alpha;
					p += 4;
				} else {
					p += 3;
				}
			}
		}
	}

	/* clean up after the read, and free any memory allocated - REQUIRED */
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

	/* that's it */
	return dest;
}

int main(int argc, char *argv[])
{
	FILE *fout = NULL;
	char *prefix = NULL;
	int i;
	uint8_t *buffer = NULL;
	uint16_t width = 0;
	uint16_t height = 0;
	uint16_t depth = 0;
	int size;

	if (argc != 5) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr,
			"%s [png input file] [raw outputfile] [output header file] [Macro prefix]\n",
			argv[0]);
		fprintf(stderr, "\n");
		fprintf(stderr, "Convert PNG image to raw binary data.\n");
		return -99;
	}

	prefix = argv[4];
	i = 0;
	while (prefix[i] != 0) {
		prefix[i] = toupper(prefix[i]);
		i++;
	}
	buffer = loadPng(argv[1], &width, &height, &depth);
	if (buffer != NULL) {
	fout = fopen(argv[2], "wb");
	if (fout != NULL) {
		size = depth * width * height;
		if (fwrite(buffer, size, 1, fout) != 1) {
			fclose(fout);
			fprintf(stderr, "Error: Failed to read image data.\n");
			return -7;
		}
		fclose(fout);

		fout = fopen(argv[3], "wt");
		if (fout != NULL) {
			fprintf(fout, "/* File generated by %s. */\n", argv[0]);
			fprintf(fout, "#ifndef _%s_H_\n", prefix);
			fprintf(fout, "#define _%s_H_\n", prefix);
			fprintf(fout, "\n");
			fprintf(fout, "/** Image width. */\n");
			fprintf(fout, "#define %s_WIDTH %d\n", prefix, width);
			fprintf(fout, "/** Image height. */\n");
			fprintf(fout, "#define %s_HEIGHT %d\n", prefix, height);
			fprintf(fout, "/** Color depth, maximum color value. */\n");
			fprintf(fout, "#define %s_DEPTH %d\n", prefix, depth);
			fprintf(fout, "\n");
			fprintf(fout, "#endif /* _%s_H_ */\n", prefix);
			fclose(fout);
		} else {
			fprintf(stderr, "Can't write to file \"%s\".\n", argv[3]);
			return -2;
		}
		return 0;
	} else {
		fprintf(stderr, "Can't write to file \"%s\".\n", argv[2]);
		return -2;
	}
	} else {
		fprintf(stderr, "Failed to read file \"%s\".\n", argv[1]);
		return -2;
	}
	return 0;
}
