/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# (C)2001, Gustavo Scotti (gustavo@scotti.com)
# (c) 2003 Marcus R. Brown (mrbrown@0xd6.org)
# (c) 2007 Mega Man
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id$
# EE FILE IO handling
*/

#ifndef _FILEIO_H
#define _FILEIO_H

#define FIO_PATH_MAX	256

#define FIO_WAIT		0
#define FIO_NOWAIT		1

#define FIO_COMPLETE	1
#define FIO_INCOMPLETE	0

#define	EOF	(-1)

#define O_RDONLY	0x0001
#define O_WRONLY	0x0002
#define O_RDWR		0x0003
#define O_NBLOCK	0x0010
#define O_APPEND	0x0100
#define O_CREAT		0x0200
#define O_TRUNC		0x0400
#define O_NOWAIT	0x8000

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	unsigned int mode;
	unsigned int attr;
	unsigned int size;
	unsigned char ctime[8];
	unsigned char atime[8];
	unsigned char mtime[8];
	unsigned int hisize;
} fio_stat_t;

typedef struct {
	fio_stat_t stat;
	char name[256];
	unsigned int unknown;
} fio_dirent_t;

int fioInit(void);
void fioExit(void);
int fioOpen(const char *fname, int mode);
int fioClose( int fd);
int fioRead( int fd, void *buff, int buff_size);
int fioWrite( int fd, const void *buff, int buff_size);
int fioLseek( int fd, int offset, int whence);
int fioMkdir(const char* dirname);
int fioPutc(int fd,int c);
int fioGetc(int fd);
int fioGets(int fd, char* buff, int n);
void fioSetBlockMode(int blocking);
int fioSync(int mode, int *retVal);

// Yet to be properly tested..
int fioIoctl(int fd, int request, void *data);
int fioDopen(const char *name);
int fioDclose(int fd);
int fioDread(int fd, fio_dirent_t *buf);
int fioGetstat(const char *name, fio_stat_t *buf);
int fioChstat(const char *name, fio_stat_t *buf, unsigned int cbit);
int fioRemove(const char *name);
int fioFormat(const char *name);
int fioRmdir(const char* dirname);

#ifdef __cplusplus
}
#endif

#endif // _FILEIO_H
