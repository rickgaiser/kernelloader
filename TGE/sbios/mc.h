/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# (c) 2007 Mega Man
#
# $Id$
# Macro's, structures & function prototypes for mclib.
*/


/*
	NOTE: These functions will work with the MCMAN/MCSERV or XMCMAN/XMCSERV
	modules stored in rom0. To determine which one you are using, send the
	appropriate arg to the mcInit() function (MC_TYPE_MC or MC_TYPE_XMC)
        
        NOTE: These functions seem to work for both psx and ps2 memcards
        
        to use memcards:
        1) first load modules (sio2man then mcman/mcserv)
        2) call mcInit(MC_TYPE)
        3) use mcGetInfo() to see if memcards are connected
        4) use mcSync to check that the function has finished
        
        all mc* functions except mcInit() are asynchronous and require mcSync()
        usage to test when they are done
*/

#ifndef _MCLIB_H_
#define _MCLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MC_WAIT			0
#define MC_NOWAIT		1

#define MC_TYPE_PSX     1
#define MC_TYPE_PS2     2
#define MC_TYPE_POCKET  3
#define MC_TYPE_NONE    0

// Problem: Format is always 0 with ROM irx's
#define MC_FORMATTED    -1
#define MC_UNFORMATTED  0

// Valid bits in memcard file attributes (mctable.AttrFile)
#define MC_ATTR_READABLE        0x0001
#define MC_ATTR_WRITEABLE       0x0002
#define MC_ATTR_EXECUTABLE      0x0004
#define MC_ATTR_PROTECTED       0x0008
#define MC_ATTR_FILE            0x0010  //Set for any file on MC
#define MC_ATTR_SUBDIR          0x0020
#define MC_ATTR_OBJECT          0x0030  //Mask to find either folder or file
#define MC_ATTR_CLOSED          0x0080
#define MC_ATTR_PDAEXEC         0x0800
#define MC_ATTR_PSX             0x1000

// function numbers returned by mcSync in the 'cmd' pointer
#define MC_FUNC_GET_INFO	0x01
#define MC_FUNC_OPEN		0x02
#define MC_FUNC_CLOSE		0x03
#define MC_FUNC_SEEK		0x04
#define MC_FUNC_READ		0x05
#define MC_FUNC_WRITE		0x06
#define MC_FUNC_FLUSH		0x0A
#define MC_FUNC_MK_DIR		0x0B
#define MC_FUNC_CH_DIR		0x0C
#define MC_FUNC_GET_DIR		0x0D
#define MC_FUNC_SET_INFO	0x0E
#define MC_FUNC_DELETE		0x0F
#define MC_FUNC_FORMAT		0x10
#define MC_FUNC_UNFORMAT	0x11
#define MC_FUNC_GET_ENT		0x12
#define MC_FUNC_RENAME		0x13
#define MC_FUNC_CHG_PRITY	0x14
//#define MC_FUNC_UNKNOWN_1	0x5A	// mcserv version
//#define MC_FUNC_UNKNOWN_2	0x5C	// mcserv version

typedef int iconIVECTOR[4];
typedef float iconFVECTOR[4];

typedef struct
{
    unsigned char  head[4];     // header = "PS2D"
    unsigned short unknown1;    // unknown
    unsigned short nlOffset;    // new line pos within title name
    unsigned unknown2;          // unknown
    unsigned trans;             // transparency
    iconIVECTOR bgCol[4];       // background color for each of the four points
    iconFVECTOR lightDir[3];    // directions of three light sources
    iconFVECTOR lightCol[3];    // colors of each of these sources
    iconFVECTOR lightAmbient;   // ambient light
    unsigned short title[34];   // application title - NOTE: stored in sjis, NOT normal ascii
    unsigned char view[64];     // list icon filename
    unsigned char copy[64];     // copy icon filename
    unsigned char del[64];      // delete icon filename
    unsigned char unknown3[512];// unknown
} mcIcon;


typedef struct
{
    struct
    {
        unsigned char unknown1;
        unsigned char sec;      // Entry creation date/time (second)
        unsigned char min;      // Entry creation date/time (minute)
        unsigned char hour;     // Entry creation date/time (hour)
        unsigned char day;      // Entry creation date/time (day)
        unsigned char month;    // Entry creation date/time (month)
        unsigned short year;    // Entry creation date/time (year)
    } _create;

    struct
    {
        unsigned char unknown2;
        unsigned char sec;      // Entry modification date/time (second)
        unsigned char min;      // Entry modification date/time (minute)
        unsigned char hour;     // Entry modification date/time (hour)
        unsigned char day;      // Entry modification date/time (day)
        unsigned char month;    // Entry modification date/time (month)
        unsigned short year;    // Entry modification date/time (year)
    } _modify;

    unsigned fileSizeByte;      // File size (bytes). For a directory entry: 0
    unsigned short attrFile;    // File attribute
    unsigned short unknown3;
    unsigned unknown4[2];
    unsigned char name[32];         //Entry name
} mcTable __attribute__((aligned (64)));


// values to send to mcInit() to use either mcserv or xmcserv
#define MC_TYPE_MC	0
#define MC_TYPE_XMC	1
#define MC_TYPE_RESET	0xdead


// init memcard lib
// 
// args:	MC_TYPE_MC  = use MCSERV/MCMAN
//			MC_TYPE_XMC = use XMCSERV/XMCMAN
// returns:	0   = successful
//			< 0 = error
int mcInit(tge_sbcall_rpc_arg_t *carg);

#ifdef __cplusplus
}
#endif

#endif // _MCLIB_H_
