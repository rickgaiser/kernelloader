/* Copyright (c) 2007 Mega Man */
#ifndef __ROM_H__
#define __ROM_H__

#define ROM_MAX_FILENAME 200

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rom_entry
{
	unsigned char filename[ROM_MAX_FILENAME];
	const void *start;
	int size;
	int width;
	int height;
	int depth;
} rom_entry_t;

typedef struct rom_stream
{
	const rom_entry_t *file;
	FILE *fin;
	int pos;
} rom_stream_t;

rom_stream_t *rom_open(const char *filename, const char *mode);
int rom_read(rom_stream_t *fd, void *buffer, int size);
int rom_seek(rom_stream_t *fd, int offset, int whence);
long rom_tell(rom_stream_t *fd);
int rom_close(rom_stream_t *fd);
int rom_isFast(rom_stream_t *fd);
int rom_eof(rom_stream_t *fd);

const rom_entry_t *rom_getFile(const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* __ROM_H__ */
