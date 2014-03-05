/*
 * sbios.c - SBIOS entrypoint and dispatch table.
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 * Copyright (c) 2008 Mega Man
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#include "sbcalls.h"
#include "iopmemdebug.h"
#include "stdio.h"

#define SBCALL_MAX	256

#if defined(SBIOS_DEBUG)
#if 0 /* TBD: Too large for SBIOS. */
static const char *sbiosDescription[] = {
	"GETVER",
	"HALT",
	"SETDVE",
	"PUTCHAR",
	"GETCHAR",
	"SETGSCRT",
	"SETRGBYC",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"REG_DBG",
	"SIFINIT",
	"SIFEXIT",
	"SIFSETDMA",
	"SIFDMASTAT",
	"SIFSETDCHAIN",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"SIFINITCMD",
	"SIFEXITCMD",
	"SIFSENDCMD",
	"SIFCMDINTRHDLR",
	"SIFADDCMDHNDLR",
	"SIFREMOVECMDHDLR",
	"SIFSETCMDBUF",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"SIFINITRPC",
	"SIFEXITRPC",
	"SIFGETOTHERDATA",
	"SIFBINDRPC",
	"SIFCALLRPC",
	"SIFCHECKSTATRPC",
	"SIFSETRPCQUEUE",
	"SIFREGISTERRPC",
	"SIFREMOVERPC",
	"SIFREMOVERPCQUEUE",
	"SIFGETNEXTREQUEST",
	"SIFEXECREQUEST",
	NULL,
	NULL,
	NULL,
	NULL,
	"IOPH_INIT",
	"IOPH_ALLOC",
	"IOPH_FREE",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"PAD_INIT",
	"PAD_END",
	"PAD_PORTOPEN",
	"PAD_PORTCLOSE",
	"PAD_SETMAINMODE",
	"PAD_SETACTDIRECT",
	"PAD_SETACTALIGN",
	"PAD_INFOPRESSMODE",
	"PAD_ENTERPRESSMODE",
	"PAD_EXITPRESSMODE",
	"PAD_READ",
	"PAD_GETSTATE",
	"PAD_GETREQSTATE",
	"PAD_INFOACT",
	"PAD_INFOCOMB",
	"PAD_INFOMODE",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"SND_INIT",
	"SND_END",
	"SND_GREG",
	"SND_SREG",
	"SND_GCOREATTR",
	"SND_SCOREATTR",
	"SND_TRANS",
	"SND_TRANSSTAT",
	"SND_TRANSCALLBACK",
	NULL,
	NULL,
	"SND_REMOTE",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"MC_INIT",
	"MC_OPEN",
	"MC_MKDIR",
	"MC_CLOSE",
	"MC_SEEK",
	"MC_READ",
	"MC_WRITE",
	"MC_GETINFO",
	"MC_GETDIR",
	"MC_FORMAT",
	"MC_DELETE",
	"MC_FLUSH",
	"MC_SETFILEINFO",
	"MC_RENAME",
	"MC_UNFORMAT",
	"MC_GETENTSPACE",
	"MC_CALL",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"CDVD_INIT",
	"CDVD_RESET",
	"CDVD_READY",
	"CDVD_READ",
	"CDVD_STOP",
	"CDVD_GETTOC",
	"CDVD_READRTC",
	"CDVD_WRITERTC",
	"CDVD_MMODE",
	"CDVD_GETERROR",
	"CDVD_GETTYPE",
	"CDVD_TRAYREQ",
	"CDVD_POWERHOOK",
	"CDVD_DASTREAM",
	"CDVD_READSUBQ",
	"CDVD_OPENCONFIG",
	"CDVD_CLOSECONFIG",
	"CDVD_READCONFIG",
	"CDVD_WRITECONFIG",
	"CDVD_RCBYCTL",
	"CDVD_READ_DVD", /* Added by TGE. */
};
#endif
#endif

#ifdef CALLBACK_DEBUG
int sbcall_register_prints_callback(callback_prints_t *callback);
void no_prints(const char *text);
#endif

static void *dispatch[SBCALL_MAX] = {
	/* 0 */
	sbcall_getver,
	/* 1 */
	sbcall_halt,
	/* 2 */
	sbcall_setdve,
	/* 3 */
	sbcall_putc,
	/* 4 */
	sbcall_getc,
	/* 5 */
	sbcall_setgscrt,
	/* 6 */
	sbcall_setrgbyc,
	0, 0, 0, 0, 0, 0, 0, 0,
#ifdef CALLBACK_DEBUG
	/* 15, extension by TGE. Not supported by RTE. */
	sbcall_register_prints_callback,
#else
	0,
#endif
	/* 16 */
	sbcall_sifinit,
	/* 17 */
	sbcall_sifexit,
	/* 18 */
	sbcall_sifsetdma,
	/* 19 */
	sbcall_sifdmastat,
	/* 20 */
	sbcall_sifsetdchain,
	/* 21 */
	sbcall_sifsetreg,
	/* 22 */
	sbcall_sifgetreg,
	/* 23 */
	sbcall_sifstopdma,
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 32 */
	sbcall_sifinitcmd,
	/* 33 */
	sbcall_sifexitcmd,
	/* 34 */
	sbcall_sifsendcmd,
	/* 35 */
	sbcall_sifcmdintrhdlr,
	/* 36 */
	sbcall_sifaddcmdhandler,
	/* 37 */
	sbcall_sifremovecmdhandler,
	/* 38 */
	sbcall_sifsetcmdbuffer,
	/* 39 */
	sbcall_sifsetsreg,
	/* 40 */
	sbcall_sifgetsreg,
	/* 41 */
	sbcall_sifgetdatatable,
	/* 42 */
	sbcall_sifsetsyscmdbuffer,
	0, 0, 0, 0, 0,
	/* 48 */
	sbcall_sifinitrpc,
	/* 49 */
	0,
	/* 50 */
	0,
	/* 51 */
	sbcall_sifbindrpc,
	/* 52 */
	sbcall_sifcallrpc,
	/* 53 */
	0,
	/* 54 */
	0,
	/* 55 */
	0,
	/* 56 */
	0,
	/* 57 */
	0,
	/* 58 */
	0,
	/* 59 */
	0,
	0, 0, 0, 0,
	/* 64 */
	sbcall_iopheapinit,
	/* 65 */
	sbcall_iopaheapalloc,
	/* 66 */
	sbcall_iopheapfree,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 80 */
	sbcall_padinit,
	/* 81 */
	sbcall_padend,
	/* 82 */
	sbcall_padportopen,
	/* 83 */
	sbcall_padportclose,
	/* 84 */
	sbcall_padsetmainmode,
	/* 85 */
	sbcall_padsetactdirect,
	/* 86 */
	sbcall_padsetactalign,
	/* 87 */
	sbcall_padinfopressmode,
	/* 88 */
	sbcall_padenterpressmode,
	/* 89 */
	sbcall_padexitpressmode,
	/* 90 */
	sbcall_padread,
	/* 91 */
	sbcall_padgetstate,
	/* 92 */
	sbcall_padgetreqstate,
	/* 93 */
	sbcall_padinfoact,
	/* 94 */
	0, /* sbcall_padinfocomb */
	/* 95 */
	sbcall_padinfomode,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* Sound is working with RTE libsd.irx and sdrdrv.irx and old versions from game discs. */
	/* 112 */
	sbcall_soundinit,
	/* 113 */
	sbcall_soundend,
	/* 114 */
	sbcall_soundgetreg,
	/* 115 */
	sbcall_soundsetreg,
	/* 116 */
	sbcall_soundgcoreattr,
	/* 117 */
	sbcall_soundscoreattr,
	/* 118 */
	sbcall_soundtrans,
	/* 119 */
	sbcall_soundtransstat,
	/* 120 */
	sbcall_soundtranscallback,
	/* 121 */
	0,
	/* 122 */
	0,
	/* 123 */
	sbcall_soundremote,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 144 */
	sbcall_mcinit,
	/* 145 */
	sbcall_mcopen,
	/* 146 */
	sbcall_mcmkdir,
	/* 147 */
	sbcall_mcclose,
	/* 148 */
	sbcall_mcseek,
	/* 149 */
	sbcall_mcread,
	/* 150 */
	sbcall_mcwrite,
	/* 151 */
	sbcall_mcgetinfo,
	/* 152 */
	sbcall_mcgetdir,
	/* 153 */
	sbcall_mcformat,
	/* 154 */
	sbcall_mcdelete,
	/* 155 */
	sbcall_mcflush,
	/* 156 */
	sbcall_mcsetfileinfo,
	/* 157 */
	sbcall_mcrename,
	/* 158 */
	sbcall_mcunformat,
	/* 159 */
	sbcall_mcgetentspace,
	/* 160 */
	0, /* MC_CALL, What is this? */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 176 */
	sbcall_cdvdinit,
	/* 177 */
	sbcall_cdvdreset,
	/* 178 */
	sbcall_cdvdready,
	/* 179 */
	sbcall_cdvdread,
	/* 180 */
	sbcall_cdvdstop,
	/* 181 */
	sbcall_cdvdgettoc,
	/* 182 */
	sbcall_cdvdreadrtc,
	/* 183 */
	sbcall_cdvdwritertc,
	/* 184 */
	sbcall_cdvdmmode,
	/* 185 */
	sbcall_cdvdgeterror,
	/* 186 */
	sbcall_cdvdgettype,
	/* 187 */
	sbcall_cdvdtrayrequest,
	/* 188 */
	0, /* Installs callback function in RTE, not supported in TGE. */
	/* 189 */
	0,
	/* 190 */
	0,
	/* 191 */
	0,
	/* 192 */
	0,
	/* 193 */
	0,
	/* 194 */
	0,
	/* 195 */
	0,
	/* 196 */
	sbcall_cdvdread_video, /* Not part of RTE, needed to read burned DVD video discs. */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 208 */
	0,
	/* 209 */
	0,
	/* 210 */
	0,
	/* 211 */
	0,
	/* 212 */
	0,
	/* 213 */
	0,
	/* 214 */
	0,
	/* 215 */
	0,
	/* 216 */
	0,
	/* 217 */
	0,
	/* 218 */
	0

};

#ifdef CALLBACK_DEBUG
callback_prints_t *callback_prints = no_prints;

void no_prints(const char *text)
{
	text = text;
}

int sbcall_register_prints_callback(callback_prints_t *callback)
{
	callback_prints = callback;
	printf("Register func 0x%x.\n", (u32) callback);
	return 0;
}
#endif

int sbios(tge_sbcall_t sbcall, void *arg)
{
	int ret;
	int (*sbfunc)(void *) = (((uint32_t) sbcall) < SBCALL_MAX) ? dispatch[sbcall] : NULL;
#if defined(SBIOS_DEBUG)
#if 0 /* TBD: Too large for SBIOS. */
	const char *description = "unknown";

	if (sbcall < sizeof(sbiosDescription) / sizeof(sbiosDescription[0])) {
		if (sbiosDescription[sbcall] != NULL) {
			description = sbiosDescription[sbcall];
		}
	}

	printf("sbios call %d (%s) %s\n", sbcall, description, (sbfunc) ? "implemented" : "not implemented");
#else
	printf("sbios call %d %s\n", sbcall, (sbfunc) ? "implemented" : "not implemented");
#endif
#endif

	if (!sbfunc) {
		return -1;
	}

	ret = sbfunc(arg);

#if defined(SBIOS_DEBUG)
	printf("rv of sbios call %d\n", ret);
#endif

	return ret;
}
