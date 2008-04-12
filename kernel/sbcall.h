/*
 * sbcall.h
 *
 *	Copyright (C) 2000-2002  Sony Computer Entertainment Inc.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License Version 2. See the file "COPYING" in the main
 * directory of this archive for more details.
 *
 * $Id$
 *
 */
/*
 * Chage history:
 *
 * Version 2.57         Mar 13, 2003
 *	added:
 *		SB_SETRGBYC
 * Version 2.56         Nov 26, 2002
 *	added:
 *		SBR_REMOCON2_INIT
 *		SBR_REMOCON2_END
 *		SBR_REMOCON2_PORTOPEN
 *		SBR_REMOCON2_PORTCLOSE
 *		SBR_REMOCON2_FEATURE
 *		SBR_REMOCON2_IGNORE
 *		SB_REMOCON2_READ
 * Version 2.55		Jan 09, 2002
 *	added SBR_SOUND_REMOTE command definitions
 * Version 2.54		Jan 08, 2002
 *	deleted:
 *		SBR_SOUND_VOICE_TRANS
 *		SBR_SOUND_VOICE_TRANSSTAT
 * Version 2.53		Dec 19, 2001
 *	added:
 *		SBR_SOUND_REMOTE
 * Version 2.52		Dec 04, 2001
 *	added:
 *		SBR_CDVD_OPENCONFIG
 *		SBR_CDVD_CLOSECONFIG
 *		SBR_CDVD_READCONFIG
 *		SBR_CDVD_WRITECONFIG
 * Version 2.51		Nov 19, 2001
 *	added:
 *		SBR_REMOCON_INIT
 *		SBR_REMOCON_END
 *		SBR_REMOCON_PORTOPEN
 *		SBR_REMOCON_PORTCLOSE
 *		SB_REMOCON_READ
 */

#ifndef __ASM_PS2_SBCALL_H
#define __ASM_PS2_SBCALL_H

#define SBIOS_VERSION	0x0257

#define SB_GETVER		0
#define SB_HALT			1
struct sb_halt_arg {
    int mode;
#define SB_HALT_MODE_PWROFF	0
#define SB_HALT_MODE_HALT	1
#define SB_HALT_MODE_RESTART	2
};
#define SB_SETDVE		2
struct sb_setdve_arg {
    int mode;
};
#define SB_PUTCHAR		3
struct sb_putchar_arg {
    char c;
};
#define SB_GETCHAR		4
#define SB_SETGSCRT		5
struct sb_setgscrt_arg {
    int inter;
    int omode;
    int ffmode;
    int *dx1, *dy1, *dx2, *dy2;
};
#define SB_SETRGBYC		6
struct sb_setrgbyc_arg {
    int rgbyc;
};

/*
 *  SIF DMA services
 */

#define SB_SIFINIT		16
#define SB_SIFEXIT		17
#define SB_SIFSETDMA		18
struct sb_sifsetdma_arg {
    void *sdd;
    int len;
};
#define SB_SIFDMASTAT		19
struct sb_sifdmastat_arg {
    int id;
};
#define SB_SIFSETDCHAIN		20
/* 21-23: reserved */

/*
 *  SIF CMD services
 */

#define SB_SIFINITCMD		32
#define SB_SIFEXITCMD		33
#define SB_SIFSENDCMD		34
struct sb_sifsendcmd_arg {
    uint32_t fid;
    void *pp;
    int ps;
    void *src;
    void *dest;
    int size;
};
#define SB_SIFCMDINTRHDLR	35
#define SB_SIFADDCMDHANDLER	36
struct sb_sifaddcmdhandler_arg {
    uint32_t fid;
    void *func;
    void *data;
};
#define SB_SIFREMOVECMDHANDLER	37
struct sb_sifremovecmdhandler_arg {
    uint32_t fid;
};
#define SB_SIFSETCMDBUFFER	38
struct sb_sifsetcmdbuffer_arg {
    void *db;
    int size;
};
/* 39-42: reserved */

/*
 *  SIF RPC services
 */

#define SB_SIFINITRPC		48
#define SB_SIFEXITRPC		49
#define SB_SIFGETOTHERDATA	50
struct sb_sifgetotherdata_arg {
    void *rd;
    void *src;
    void *dest;
    int size;
    uint32_t mode;
    void *func;
    void *para;
};
#define SB_SIFBINDRPC		51
struct sb_sifbindrpc_arg {
    void *bd;
    uint32_t command;
    uint32_t mode;
    void *func;
    void *para;
};
#define SB_SIFCALLRPC		52
struct sb_sifcallrpc_arg {
    void *bd;
    uint32_t fno;
    uint32_t mode;
    void *send;
    int ssize;
    void *receive;
    int rsize;
    void *func;
    void *para;
};
#define SB_SIFCHECKSTATRPC	53
struct sb_sifcheckstatrpc_arg {
    void *cd;
};
#define SB_SIFSETRPCQUEUE	54
struct sb_sifsetrpcqueue_arg {
    void *pSrqd;
    void *callback;
    void *arg;
};
#define SB_SIFREGISTERRPC	55
struct sb_sifregisterrpc_arg {
    void *pr;
    uint32_t command;
    void *func;
    void *buff;
    void *cfunc;
    void *cbuff;
    void *pq;
};
#define SB_SIFREMOVERPC		56
struct sb_sifremoverpc_arg {
    void *pr;
    void *pq;
};
#define SB_SIFREMOVERPCQUEUE	57
struct sb_sifremoverpcqueue_arg {
    void *pSrqd;
};
#define SB_SIFGETNEXTREQUEST	58
struct sb_sifgetnextrequest_arg {
    void *qd;
};
#define SB_SIFEXECREQUEST	59
struct sb_sifexecrequest_arg {
    void *rdp;
};

/*
 *  device services
 */

/* RPC common argument */

struct sbr_common_arg {
    int result;
    void *arg;
    void (*func)(void *, int);
    void *para;
};

/* IOP heap */

#define SBR_IOPH_INIT		64
#define SBR_IOPH_ALLOC		65
struct sbr_ioph_alloc_arg {
    int size;
};
#define SBR_IOPH_FREE		66
struct sbr_ioph_free_arg {
    void *addr;
};

/* pad device */

#define SBR_PAD_INIT		80
struct sbr_pad_init_arg {
    int mode;
};
#define SBR_PAD_END		81
#define SBR_PAD_PORTOPEN	82
struct sbr_pad_portopen_arg {
    int port;
    int slot;
    void *addr;
};
#define SBR_PAD_PORTCLOSE	83
struct sbr_pad_portclose_arg {
    int port;
    int slot;
};
#define SBR_PAD_SETMAINMODE	84
struct sbr_pad_setmainmode_arg {
    int port;
    int slot;
    int offs;
    int lock;
};
#define SBR_PAD_SETACTDIRECT	85
struct sbr_pad_setactdirect_arg {
    int port;
    int slot;
    const unsigned char *data;
};
#define SBR_PAD_SETACTALIGN	86
struct sbr_pad_setactalign_arg {
    int port;
    int slot;
    const unsigned char *data;
};
#define SBR_PAD_INFOPRESSMODE	87
struct sbr_pad_pressmode_arg {
    int port;
    int slot;
};
#define SBR_PAD_ENTERPRESSMODE	88
#define SBR_PAD_EXITPRESSMODE	89


#define SB_PAD_READ		90
struct sb_pad_read_arg {
    int port;
    int slot;
    unsigned char *rdata;
};
#define SB_PAD_GETSTATE		91
struct sb_pad_getstate_arg {
    int port;
    int slot;
};
#define SB_PAD_GETREQSTATE	92
struct sb_pad_getreqstate_arg {
    int port;
    int slot;
};
#define SB_PAD_INFOACT		93
struct sb_pad_infoact_arg {
    int port;
    int slot;
    int actno;
    int term;
};
#define SB_PAD_INFOCOMB		94
struct sb_pad_infocomb_arg {
    int port;
    int slot;
    int listno;
    int offs;
};
#define SB_PAD_INFOMODE		95
struct sb_pad_infomode_arg {
    int port;
    int slot;
    int term;
    int offs;
};

/* sound */

#define SBR_SOUND_INIT		112
struct sbr_sound_init_arg {
#define SB_SOUND_INIT_COLD	0
#define SB_SOUND_INIT_HOT	1
    int flag;
};
#define SBR_SOUND_END		113
#define SB_SOUND_GREG		114
#define SB_SOUND_SREG		115
struct sb_sound_reg_arg {
    uint32_t idx;
#define SB_SOUND_REG_MADR(core)		(0 + (core))
#define SB_SOUND_REG_BCR(core)		(2 + (core))
#define SB_SOUND_REG_BTCR(core)		(4 + (core))
#define SB_SOUND_REG_CHCR(core)		(6 + (core))

#define SB_SOUND_REG_MMIX(core)		(8 + (core))
#define SB_SOUND_REG_DMAMOD(core)	(10 + (core))
#define SB_SOUND_REG_MVOLL(core)	(12 + (core))
#define SB_SOUND_REG_MVOLR(core)	(14 + (core))
#define SB_SOUND_REG_EVOLL(core)	(16 + (core))
#define SB_SOUND_REG_EVOLR(core)	(18 + (core))
#define SB_SOUND_REG_AVOLL(core)	(20 + (core))
#define SB_SOUND_REG_AVOLR(core)	(22 + (core))
#define SB_SOUND_REG_BVOLL(core)	(24 + (core))
#define SB_SOUND_REG_BVOLR(core)	(26 + (core))
    uint32_t data;
};
#define SBR_SOUND_GCOREATTR	116
#define SBR_SOUND_SCOREATTR	117
struct sbr_sound_coreattr_arg {
    uint32_t idx;
#define SB_SOUND_CA_EFFECT_ENABLE	(1<<1)
#define SB_SOUND_CA_IRQ_ENABLE		(2<<1)
#define SB_SOUND_CA_MUTE_ENABLE		(3<<1)
#define SB_SOUND_CA_NOISE_CLK		(4<<1)
#define SB_SOUND_CA_SPDIF_MODE		(5<<1)
    uint32_t data;
};
#define SBR_SOUND_TRANS		118
struct sbr_sound_trans_arg {
    int channel;
    uint32_t mode;
#define SB_SOUND_TRANS_MODE_WRITE	0
#define SB_SOUND_TRANS_MODE_READ	1
#define SB_SOUND_TRANS_MODE_STOP	2
#define SB_SOUND_TRANS_MODE_DMA		(0<<3)
#define SB_SOUND_TRANS_MODE_PIO		(1<<3)
#define SB_SOUND_TRANS_MODE_ONCE	(0<<4)
#define SB_SOUND_TRANS_MODE_LOOP	(1<<4)
    uint32_t addr;
    uint32_t size;
    uint32_t start_addr;
};
#define SBR_SOUND_TRANSSTAT	119
struct sbr_sound_trans_stat_arg {
    int channel;
#define SB_SOUND_TRANSSTAT_WAIT		1
#define SB_SOUND_TRANSSTAT_CHECK	0
    int flag;
};
#define SBR_SOUND_TRANSCALLBACK	120
struct sbr_sound_trans_callback_arg {
    int channel;
    int (*func)(void*, int);
    void *data;
    int (*oldfunc)(void*, int);
    void *olddata;
};
#define SBR_SOUND_REMOTE	123
/*
 * XXX, 
 * struct sbr_sound_remote_arg in asm/mips/ps2/sbcall.h and
 * struct ps2sd_command must have common members and alignments.
 * double check if you change this structure.
 */
struct sbr_sound_remote_arg {
    int command;
#define PS2SDCTL_COMMAND_QUIT      0x1020
#define PS2SDCTL_COMMAND_QUIT2     0x010000d0
#define PS2SDCTL_COMMAND_OPEN1     0x1030
#define PS2SDCTL_COMMAND_OPEN2     0x1040
#define PS2SDCTL_COMMAND_OPEN3     0x1050
#define PS2SDCTL_COMMAND_OPEN4     0x01009050
#define PS2SDCTL_COMMAND_WRITE     0x1060
#define PS2SDCTL_COMMAND_WRITE2    0x01000070
#define PS2SDCTL_COMMAND_WRITE3    0x01000040
#define PS2SDCTL_COMMAND_READ      0x1070
    int args[126];
};

/* memory card */

#define SBR_MC_INIT		144
#define SBR_MC_OPEN		145
struct sbr_mc_open_arg {
    int port;
    int slot;
    const char *name;
    int mode;
};
#define SBR_MC_MKDIR		146
struct sbr_mc_mkdir_arg {
    int port;
    int slot;
    const char *name;
};
#define SBR_MC_CLOSE		147
struct sbr_mc_close_arg {
    int fd;
};
#define SBR_MC_SEEK		148
struct sbr_mc_seek_arg {
    int fd;
    int offset;
    int mode;
};
#define SBR_MC_READ		149
struct sbr_mc_read_arg {
    int fd;
    void *buff;
    int size;
};
#define SBR_MC_WRITE		150
struct sbr_mc_write_arg {
    int fd;
    void *buff;
    int size;
};
#define SBR_MC_GETINFO		151
struct sbr_mc_getinfo_arg {
    int port;
    int slot;
    int *type;
    int *free;
    int *format;
};
#define SBR_MC_GETDIR		152
struct sbr_mc_getdir_arg {
    int port;
    int slot;
    const char *name;
    unsigned int mode;
    int maxent;
    void *table;
};
#define SBR_MC_FORMAT		153
struct sbr_mc_format_arg {
    int port;
    int slot;
};
#define SBR_MC_DELETE		154
struct sbr_mc_delete_arg {
    int port;
    int slot;
    const char *name;
};
#define SBR_MC_FLUSH		155
struct sbr_mc_flush_arg {
    int fd;
};
#define SBR_MC_SETFILEINFO	156
struct sbr_mc_setfileinfo_arg {
    int port;
    int slot;
    const char *name;
    const char *info;
    unsigned int valid;
};
#define SBR_MC_RENAME		157
struct sbr_mc_rename_arg {
    int port;
    int slot;
    const char *org;
    const char *new;
};
#define SBR_MC_UNFORMAT		158
struct sbr_mc_unformat_arg {
    int port;
    int slot;
};
#define SBR_MC_GETENTSPACE	159
struct sbr_mc_getentspace_arg {
    int port;
    int slot;
    const char *path;
};
#define SBR_MC_CALL		160

/*
 * CD/DVD
 */
#define SBR_CDVD_INIT		176
#define SBR_CDVD_RESET		177
#define SBR_CDVD_READY		178
struct sbr_cdvd_ready_arg {
    int mode;
};

#define SBR_CDVD_READ		179
struct sbr_cdvd_read_arg {
    uint32_t lbn;
    uint32_t sectors;
    void *buf;
    struct sceCdRMode *rmode;
};

#define SBR_CDVD_STOP		180
#define SBR_CDVD_GETTOC		181
struct sbr_cdvd_gettoc_arg {
    uint8_t *buf;
    int len;
    int media;
};

#define SBR_CDVD_READRTC	182
#define SBR_CDVD_WRITERTC	183
struct sbr_cdvd_rtc_arg {
	uint8_t stat;		/* status */
	uint8_t second;		/* second */
	uint8_t minute;		/* minute */
	uint8_t hour;		/* hour   */

	uint8_t pad;		/* pad    */
	uint8_t day;		/* day    */
	uint8_t month;		/* 1900 or 2000 and  month  */
	uint8_t year;		/* year   */
};
#define SBR_CDVD_MMODE		184
struct sbr_cdvd_mmode_arg {
    int media;
};

#define SBR_CDVD_GETERROR	185
#define SBR_CDVD_GETTYPE	186
#define SBR_CDVD_TRAYREQ	187
struct sbr_cdvd_trayreq_arg {
    int req;
    int traycount;
};

#define SB_CDVD_POWERHOOK	188
struct sb_cdvd_powerhook_arg {
  void (*func)(void *);
  void *arg;
};

#define SBR_CDVD_DASTREAM	189
struct sbr_cdvd_dastream_arg {
    uint32_t command;
    uint32_t lbn;
    int size;
    void *buf;
    struct sceCdRMode *rmode;
};

#define SBR_CDVD_READSUBQ	190
struct sbr_cdvd_readsubq_arg {
    int stat;
    uint8_t data[10];
    uint8_t reserved[2];
};

#define SBR_CDVD_OPENCONFIG	191
#define SBR_CDVD_CLOSECONFIG	192
#define SBR_CDVD_READCONFIG	193
#define SBR_CDVD_WRITECONFIG	194
struct sbr_cdvd_config_arg {
    int dev;
#define SB_CDVD_CFG_OSD		0x01    /* OSD    */
    int mode;
#define SB_CDVD_CFG_READ	0x00    /* READ   */
#define SB_CDVD_CFG_WRITE	0x01    /* WRITE  */
    int blk;
#define SB_CDVD_CFG_BLKSIZE	15
    uint8_t *data;
    int stat;
#define SB_CDVD_CFG_STAT_CMDERR	(1<<7)
#define SB_CDVD_CFG_STAT_BUSY	(1<<0)
};

#define SBR_CDVD_RCBYCTL        195
struct sbr_cdvd_rcbyctl_arg {
    int param;
    int stat;
};

/*
 * Remote Controller
 */
#define SBR_REMOCON_INIT	208
struct sbr_remocon_init_arg {
    int mode;
#define SBR_REMOCON_INIT_MODE	0
};

#define SBR_REMOCON_END		209
#define SBR_REMOCON_PORTOPEN	210
#define SBR_REMOCON_PORTCLOSE	211
struct sbr_remocon_portopen_arg {
    int port;
    int slot;
};

#define SB_REMOCON_READ		212
struct sb_remocon_read_arg {
    int port;
    int slot;
    int len;
#define SB_REMOCON_MAXDATASIZE	64
    unsigned char *buf;
};

#define SBR_REMOCON2_INIT	213
#define SBR_REMOCON2_END	214
#define SBR_REMOCON2_PORTOPEN	215
#define SBR_REMOCON2_PORTCLOSE	216
#define SB_REMOCON2_READ	217

#define SBR_REMOCON2_IRFEATURE	218
struct sbr_remocon2_feature_arg {
    unsigned char feature;
};

#endif /* __ASM_PS2_SBCALL_H */
