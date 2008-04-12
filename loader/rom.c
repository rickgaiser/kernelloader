/* Copyright (c) 2007 Mega Man */
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "rom.h"

#include "romdefinitions.h"

rom_entry_t rom_files[] =
{
	#include "romfilelist.h"
	{ "", NULL, 0}
};

static int rom_initialized = 0;

static void rom_initialize()
{
	int i;

	i = 0;
	#include "rominitialize.h"
}

rom_entry_t *rom_getFile(const char *filename)
{
	int i;

	if (!rom_initialized)
		rom_initialize();

	i = 0;
	while(rom_files[i].start != NULL)
	{
		if (strcmp(rom_files[i].filename, filename) == 0)
			return &rom_files[i];
		i++;
	}
	return NULL;
}

/**
 * @see fopen
 * Opens a file, but use rom version of file if available.
 * If rom version is not available, load file from host.
 * Don't use a prefix like "host:" for file name!
 */
rom_stream_t *rom_open(const char *filename, const char *mode)
{
	rom_entry_t *file;
	rom_stream_t *fd;

	if (!rom_initialized)
		rom_initialize();

	file = rom_getFile(filename);
	if (file != NULL)
	{
		fd = (rom_stream_t *) malloc(sizeof(rom_stream_t));
		if (fd != NULL)
		{
			fd->file = file;
			fd->fin = NULL;
			fd->pos = 0;
			return fd;
		}
		else
		{
			printf("Error: rom_open(): out of memory.\n");
			return NULL;
		}
	}
	else
	{
		FILE *fin;
		char hostfilename[ROM_MAX_FILENAME];

		snprintf(hostfilename, ROM_MAX_FILENAME, "host:%s", filename);

		fin = fopen(hostfilename, mode);
		if (fin != NULL)
		{
			//printf("File opened 0x%08x\n", fin);
			fd = (rom_stream_t *) malloc(sizeof(rom_stream_t));
			if (fd != NULL)
			{
				fd->file = NULL;
				fd->fin = fin;
				return fd;
			}
			else
			{
				fclose(fin);
				printf("Error: rom_open(): out of memory.\n");
				return NULL;
			}
		}
		else
			return NULL;
	}
}

/**
 * @see fread()
 */
int rom_read(rom_stream_t *fd, void *buffer, int size)
{
	if (!rom_initialized)
		rom_initialize();

	if (fd->fin != NULL)
	{
		return fread(buffer, 1, size, fd->fin);
	}
	if (fd->file != NULL)
	{
		int remaining;

		//printf("rom_read(\"%s\", 0x%08x, %d)\n", fd->file->filename, buffer, size);
		//printf("rom_read(): pos = %d\n", fd->pos);

		if (fd->pos >= fd->file->size)
			return -1;
		if (size <= 0)
			return size;

		remaining = fd->file->size - fd->pos;
		if (size > remaining)
			size = remaining;
		if (fd->file->start == NULL)
			return -2;
		if (fd->file->size < 0)
			return -2;
		//printf("rom_read(): size = %d\n", size);
		memcpy(buffer, (void *)(((int)fd->file->start) + fd->pos), size);
		fd->pos += size;
		return size;
	}
	return -1;
}

/**
 * @see fseek()
 */
int rom_seek(rom_stream_t *fd, int offset, int whence)
{
	if (!rom_initialized)
		rom_initialize();

	if (fd->fin != NULL)
		return fseek(fd->fin, offset, whence);
	if (fd->file != NULL)
	{
		int newpos;

		switch (whence)
		{
			case SEEK_SET:
				newpos = offset;
				break;
			case SEEK_END:
				newpos = fd->file->size + offset;
				break;
			case SEEK_CUR:
				newpos = fd->pos + offset;
				break;
			default:
				return -1;
		}
		if (newpos < 0)
			return -1;
		if (newpos > fd->file->size)
			return -1;
		fd->pos = newpos;
		return 0;
	}
	return -1;
}

/**
 * @see ftell()
 */
long rom_tell(rom_stream_t *fd)
{
	if (!rom_initialized)
		rom_initialize();

	if (fd->fin != NULL)
		return ftell(fd->fin);
	if (fd->file != NULL)
		return fd->pos;
	return -1;
}

/**
 * @see fclose()
 */
int rom_close(rom_stream_t *fd)
{
	int ret = -1;

	if (!rom_initialized)
		rom_initialize();

	if (fd->fin != NULL)
	{
		ret = fclose(fd->fin);
		fd->fin = NULL;
	}
	if (fd->file != NULL)
	{
		//free(fd->file); // memory is from array and not allocated!
		fd->file = NULL;
		ret = 0;
	}
	free(fd);
	return ret;
}

int rom_eof(rom_stream_t *fd)
{
	int ret;

	ret = -1;
	if (fd->fin != NULL)
	{
		ret = feof(fd->fin);
	}
	if (fd->file != NULL)
	{
		ret = (fd->pos >= fd->file->size);
	}
	return ret;
}

/**
 * @return True, if transfer is fast and don't need to be cached.
 */
int rom_isFast(rom_stream_t *fd)
{
	return fd->file != NULL;
}

