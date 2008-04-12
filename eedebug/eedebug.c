/* Copyright (c) 2007 Mega Man */
#include <types.h>
#include <thbase.h>
#include <thsemap.h>
#include <sysclib.h>
#include <stdio.h>
#include <ioman.h>
#include <intrman.h>
#include <loadcore.h>
#include <sifcmd.h>

#include "iopprintdata.h"

static eePrint(const char *text);
static int ttyMount(void);

/** IOP Module entry point, install io driver redirecting every printf() to EE. */
int _start(int argc, char **argv)
{
	iop_thread_t param;
	int th;

	if (!sceSifCheckInit())
		sceSifInit();
	sceSifInitRpc(0);

	printf("Started eedebug!\n");

	eePrint("EE debug start\n");

	ttyMount();
	return 0;
}

static eePrint(const char *text)
{
	static iop_text_data_t text_data __attribute__((aligned(64)));
	strncpy(text_data.text, text, 80);
	text_data.text[79] = 0;
	if (!sceSifSendCmd(0x00000010, &text_data, 64, NULL,
                NULL, 0)) {
		printf("Failed to send message.\n");
	}
}


////////////////////////////////////////////////////////////////////////

static int tty_sema = -1;

static char ttyname[] = "tty";

////////////////////////////////////////////////////////////////////////
static int dummy()
{
    return -5;
}

////////////////////////////////////////////////////////////////////////
static int dummy0()
{
    return 0;
}

////////////////////////////////////////////////////////////////////////
static int ttyInit(iop_device_t *driver)
{
    iop_sema_t sema_info;

    sema_info.attr       = 0;
    sema_info.initial = 1;	/* Unlocked.  */
    sema_info.max  = 1;
    if ((tty_sema = CreateSema(&sema_info)) < 0)
	    return -1;

	return 1;
}

////////////////////////////////////////////////////////////////////////
static int ttyOpen( int fd, char *name, int mode)
{
    return 1;
}

////////////////////////////////////////////////////////////////////////
static int ttyClose( int fd)
{
    return 1;
}

////////////////////////////////////////////////////////////////////////
static int ttyWrite(iop_file_t *file, char *buf, int size)
{
	static iop_text_data_t text_data __attribute__((aligned(64)));
    int res;
	int len;
	int s;
	int pos;

    WaitSema(tty_sema);

	len = size;
	pos = 0;
	do {
		s = len;
		if (len > 79) {
			s = 79;
		}
		memcpy(&text_data.text, &buf[pos], s);
		text_data.text[s] = 0;
		pos += s;
		len -= s;
		while (!sceSifSendCmd(0x00000010, &text_data, sizeof(text_data), NULL,
                NULL, 0)) {
			//printf("Failed to send message.\n");
		}
	} while(len > 0);

    SignalSema(tty_sema);
    return res;
}

iop_device_ops_t tty_functarray = { ttyInit, dummy0, (void *)dummy,
	(void *)ttyOpen, (void *)ttyClose, (void *)dummy,
	(void *)ttyWrite, (void *)dummy, (void *)dummy,
	(void *)dummy, (void *)dummy, (void *)dummy,
	(void *)dummy, (void *)dummy, (void *)dummy,
	(void *)dummy, (void *)dummy };

iop_device_t tty_driver = { ttyname, 3, 1, "TTY via Udp",
							&tty_functarray };

////////////////////////////////////////////////////////////////////////
// Entry point for mounting the file system
static int ttyMount(void)
{
    close(0);
    close(1);
    DelDrv(ttyname);
    AddDrv(&tty_driver);
    if(open("tty00:", O_RDONLY) != 0) while(1);
    if(open("tty00:", O_WRONLY) != 1) while(1);

    return 0;
}
