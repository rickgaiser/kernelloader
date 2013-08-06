/* Copyright (c) 2007 Mega Man */
#include "graphic.h"
#include "gs.h"
#include "rtesif.h"
#include "time.h"
#include "memory.h"
#include "stdio.h"

#define GSSREG_BASE1            KSEG1ADDR(0x12000000)
#define GSSREG_BASE2            KSEG1ADDR(0x12001000)
#define GSSREG1(x)              (GSSREG_BASE1 + ((x) << 4))
#define GSSREG2(x)              (GSSREG_BASE2 + (((x) & 0x0f) << 4))


static uint64_t gssreg[0x10];
static int gs_dx[2], gs_dy[2];

union _dword {
        uint64_t di;
        struct {
#if 1 // CONFIG_CPU_LITTLE_ENDIAN
                uint32_t   lo, hi;
#else
                uint32_t   hi, lo;
#endif
        } si;
};

static inline void store_double(unsigned long addr, unsigned long long val)
{
    union _dword src;

    src.di=val;
    __asm__ __volatile__(
        ".set push\n"
#ifdef PS2_EE
        "       .set mips3\n"
#else
        "       .set arch=r5900\n"
#endif
        "       pextlw         $8,%1,%0\n"
        "       sd             $8,(%2)\n"
        "       .set   pop"
        : : "r"(src.si.lo), "r"(src.si.hi), "r" (addr) : "$8");
}

static inline unsigned long long load_double(unsigned long addr)
{
    union _dword val;

    __asm__ __volatile__(
        ".set   push\n"
        "       .set    mips3\n"
        "       ld      $8,(%2)\n"
                /* 63-32th bits must be same as 31th bit */
        "       dsra    %1,$8,32\n"
        "       dsll    %0,$8,32\n"
        "       dsra    %0,%0,32\n"
        "       .set    pop"
        : "=r" (val.si.lo), "=r" (val.si.hi) : "r" (addr) : "$8");

    return val.di;
}

/*
 *  GS register read / write
 */

int ps2gs_set_gssreg(int reg, uint64_t val)
{
    if (reg >= PS2_GSSREG_PMODE && reg <= PS2_GSSREG_BGCOLOR) {
		gssreg[reg] = val;
		store_double(GSSREG1(reg), val);
    } else if (reg == PS2_GSSREG_CSR) {
		val &= 1 << 8;
		store_double(GSSREG2(reg), val);
    } else if (reg == PS2_GSSREG_SIGLBLID) {
		store_double(GSSREG2(reg), val);
    } else {
		return -1;	/* bad register no. */
	}
    return 0;
}

#if 0
static int ps2gs_set_gssreg_dummy(int reg, uint64_t val)
{
    if (reg >= PS2_GSSREG_PMODE && reg <= PS2_GSSREG_BGCOLOR) {
		gssreg[reg] = val;
    } else {
		return -1;	/* bad register no. */
	}
    return 0;
}
#endif

int ps2gs_get_gssreg(int reg, uint64_t *val)
{
    if (reg == PS2_GSSREG_CSR || reg == PS2_GSSREG_SIGLBLID) {
		/* readable register */
		*val = load_double(GSSREG2(reg));
    } else if (reg >= 0 && reg <= 0x0e) {
		/* write only register .. return saved value */
		*val = gssreg[reg];
    } else {
		return -1;	/* bad register no. */
	}
    return 0;
}

#if 0
int ps2gs_set_gsreg(int reg, uint64_t val)
{
    struct {
    	ps2_giftag giftag; // 128bit
	uint64_t param[2];
    } packet;

    PS2_GIFTAG_CLEAR_TAG(&(packet.giftag));
    packet.giftag.NLOOP = 1;
    packet.giftag.EOP = 1;
    packet.giftag.PRE = 0;
    packet.giftag.PRIM = 0;
    packet.giftag.FLG = PS2_GIFTAG_FLG_PACKED;
    packet.giftag.NREG = 1;
    packet.giftag.REGS0 = PS2_GIFTAG_REGS_AD;
    packet.param[0] = val;
    packet.param[1] = reg;

    ps2sdma_send(DMA_GIF, &packet, sizeof(packet));
    return 0;
}
#endif

/*
 *  PCRTC sync parameters
 */

#define vSMODE1(VHP,VCKSEL,SLCK2,NVCK,CLKSEL,PEVS,PEHS,PVS,PHS,GCONT,SPML,PCK2,XPCK,SINT,PRST,EX,CMOD,SLCK,T1248,LC,RC)	\
	(((uint64_t)(VHP)<<36)   | ((uint64_t)(VCKSEL)<<34) | ((uint64_t)(SLCK2)<<33) | \
	 ((uint64_t)(NVCK)<<32)  | ((uint64_t)(CLKSEL)<<30) | ((uint64_t)(PEVS)<<29)  | \
	 ((uint64_t)(PEHS)<<28)  | ((uint64_t)(PVS)<<27)    | ((uint64_t)(PHS)<<26)   | \
	 ((uint64_t)(GCONT)<<25) | ((uint64_t)(SPML)<<21)   | ((uint64_t)(PCK2)<<19)  | \
	 ((uint64_t)(XPCK)<<18)  | ((uint64_t)(SINT)<<17)   | ((uint64_t)(PRST)<<16)  | \
	 ((uint64_t)(EX)<<15)    | ((uint64_t)(CMOD)<<13)   | ((uint64_t)(SLCK)<<12)  | \
	 ((uint64_t)(T1248)<<10) | ((uint64_t)(LC)<<3)      | ((uint64_t)(RC)<<0))
#define vSYNCH1(HS,HSVS,HSEQ,HBP,HFP)	\
	(((uint64_t)(HS)<<43) | ((uint64_t)(HSVS)<<32) | ((uint64_t)(HSEQ)<<22) | \
	 ((uint64_t)(HBP)<<11) | ((uint64_t)(HFP)<<0))
#define vSYNCH2(HB,HF) \
	(((uint64_t)(HB)<<11) | ((uint64_t)(HF)<<0))
#define vSYNCV(VS,VDP,VBPE,VBP,VFPE,VFP) \
	(((uint64_t)(VS)<<53) | ((uint64_t)(VDP)<<42) | ((uint64_t)(VBPE)<<32) | \
	 ((uint64_t)(VBP)<<20) | ((uint64_t)(VFPE)<<10) | ((uint64_t)(VFP)<<0))

struct rdisplay {
    int magv, magh, dy, dx;
};
#define vDISPLAY(DH,DW,MAGV,MAGH,DY,DX) \
	{ (MAGV), (MAGH), (DY), (DX) }
#define wDISPLAY(DH,DW,MAGV,MAGH,DY,DX) \
	(((uint64_t)(DH)<<44) | ((uint64_t)(DW)<<32) | ((uint64_t)(MAGV)<<27) | \
	 ((uint64_t)(MAGH)<<23) | ((uint64_t)(DY)<<12) | ((uint64_t)(DX)<<0))

struct syncparam {
    int width, height, rheight, dvemode;
    uint64_t smode1, smode2, srfsh, synch1, synch2, syncv;
    struct rdisplay display;
};

static const struct syncparam syncdata0[] = {
    /* 0: NTSC-NI (640x240(224)) */
    { 640, 240, 224, 0,
      vSMODE1(0, 1,1,1,1, 0,0, 0,0,0,4,0,0,1,1,0,2,0, 1,32,4), 0, 8,
      vSYNCH1(254,1462,124,222,64), vSYNCH2(1652,1240),
      vSYNCV(6,480,6,26,6,2), vDISPLAY(239,2559,0,3,25,632) },

    /* 1: NTSC-I (640x480(448)) */
    { 640, 480, 448, 0,
      vSMODE1(0, 1,1,1,1, 0,0, 0,0,0,4,0,0,1,1,0,2,0, 1,32,4), 1, 8,
      vSYNCH1(254,1462,124,222,64), vSYNCH2(1652,1240),
      vSYNCV(6,480,6,26,6,1), vDISPLAY(479,2559,0,3,50,632) },


    /* 2: PAL-NI (640x288(256)) */
    { 640, 288, 256, 1,
      vSMODE1(0, 1,1,1,1, 0,0, 0,0,0,4,0,0,1,1,0,3,0, 1,32,4), 0, 8,
      vSYNCH1(254,1474,127,262,48), vSYNCH2(1680,1212),
      vSYNCV(5,576,5,33,5,4), vDISPLAY(287,2559,0,3,36,652) },

    /* 3: PAL-I (640x576(512)) */
    { 640, 576, 512, 1,
      vSMODE1(0, 1,1,1,1, 0,0, 0,0,0,4,0,0,1,1,0,3,0, 1,32,4), 1, 8,
      vSYNCH1(254,1474,127,262,48), vSYNCH2(1680,1212),
      vSYNCV(5,576,5,33,5,1), vDISPLAY(575,2559,0,3,72,652) },


    /* 4: VESA-1A (640x480 59.940Hz) */
    { 640, 480, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,2,0,0,1,0,0,0,0, 1,15,2), 0, 4,
      vSYNCH1(192,608,192,84,32), vSYNCH2(768,524),
      vSYNCV(2,480,0,33,0,10), vDISPLAY(479,1279,0,1,34,276) },

    /* 5: VESA-1C (640x480 75.000Hz) */
    { 640, 480, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,2,0,0,1,0,0,0,0, 1,28,3), 0, 4,
      vSYNCH1(128,712,128,228,32), vSYNCH2(808,484),
      vSYNCV(3,480,0,16,0,1), vDISPLAY(479,1279,0,1,18,356) },


    /* 6:  VESA-2B (800x600 60.317Hz) */
    { 800, 600, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,2,0,0,1,0,0,0,0, 1,71,6), 0, 4,
      vSYNCH1(256,800,256,164,80), vSYNCH2(976,636),
      vSYNCV(4,600,0,23,0,1), vDISPLAY(599,1599,0,1,26,420) },

    /* 7: VESA-2D (800x600 75.000Hz) */
    { 800, 600, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,2,0,0,1,0,0,0,0, 1,44,3), 0, 4,
      vSYNCH1(160,896,160,308,32), vSYNCH2(1024,588),
      vSYNCV(3,600,0,21,0,1), vDISPLAY(599,1599,0,1,23,468) },


    /* 8: VESA-3B (1024x768 60.004Hz) */
    { 1024, 768, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,2,0,0,1,0,0,0,0, 0,58,6), 0, 4,
      vSYNCH1(272,1072,272,308,48), vSYNCH2(1296,764),
      vSYNCV(6,768,0,29,0,3), vDISPLAY(767,2047,0,1,34,580) },

    /* 9: VESA-3D (1024x768 75.029Hz) */
    { 1024, 768, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,1,0,0,1,0,0,0,0, 1,35,3), 0, 2,
      vSYNCH1(96,560,96,164,16), vSYNCH2(640,396),
      vSYNCV(3,768,0,28,0,1), vDISPLAY(767,1023,0,0,30,260) },


    /* 10: VESA-4A (1280x1024 60.020Hz) */
    { 1280, 1024, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,1,0,0,1,0,0,0,0, 0,8,1), 0, 2,
      vSYNCH1(112,732,112,236,16), vSYNCH2(828,496),
      vSYNCV(3,1024,0,38,0,1), vDISPLAY(1023,1279,0,0,40,348) },

    /* 11: VESA-4B (1280x1024 75.025Hz) */
    { 1280, 1024, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,1,0,0,1,0,0,0,0, 0,10,1), 0, 2,
      vSYNCH1(144,700,144,236,16), vSYNCH2(828,464),
      vSYNCV(3,1024,0,38,0,1), vDISPLAY(1023,1279,0,0,40,380) },


    /* 12: DTV-480P (720x480) */
    { 720, 480, -1, 3,
      vSMODE1(1, 1,1,1,1, 0,0, 0,0,0,2,0,0,1,1,0,0,0, 1,32,4), 0, 4,
      vSYNCH1(128,730,128,104,32), vSYNCH2(826,626),
      vSYNCV(6,483,0,30,0,6), vDISPLAY(479,1439,0,1,35,232) },

    /* 13: DTV-1080I (1920x1080) */
    { 1920, 1080, -1, 4,
      vSMODE1(0, 0,1,1,1, 0,0, 0,0,0,1,0,0,1,0,0,0,0, 1,22,2), 1, 4,
      vSYNCH1(104,1056,44,134,30), vSYNCH2(1064,868),
      vSYNCV(10,1080,2,28,0,5), vDISPLAY(1079,1919,0,0,40,238) },

    /* 14: DTV-720P (1280x720) */
    { 1280, 720, -1, 5,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,1,0,0,1,0,0,0,0, 1,22,2), 0, 4,
      vSYNCH1(104,785,40,198,62), vSYNCH2(763,529),
      vSYNCV(5,720,0,20,0,5), vDISPLAY(719,1279,0,0,24,302) },
};


/* GS rev.19 or later */

static const struct syncparam syncdata1[] = {
    /* 0: NTSC-NI (640x240(224)) */
    { 640, 240, 224, 0,
      vSMODE1(0, 1,1,1,1, 0,0, 0,0,0,4,0,0,1,1,0,2,0, 1,32,4), 0, 8,
      vSYNCH1(254,1462,124,222,64), vSYNCH2(1652,1240),
      vSYNCV(6,480,6,26,6,2), vDISPLAY(239,2559,0,3,25,632) },

    /* 1: NTSC-I (640x480(448)) */
    { 640, 480, 448, 0,
      vSMODE1(0, 1,1,1,1, 0,0, 0,0,0,4,0,0,1,1,0,2,0, 1,32,4), 1, 8,
      vSYNCH1(254,1462,124,222,64), vSYNCH2(1652,1240),
      vSYNCV(6,480,6,26,6,1), vDISPLAY(479,2559,0,3,50,632) },


    /* 2: PAL-NI (640x288(256)) */
    { 640, 288, 256, 1,
      vSMODE1(0, 1,1,1,1, 0,0, 0,0,0,4,0,0,1,1,0,3,0, 1,32,4), 0, 8,
      vSYNCH1(254,1474,127,262,48), vSYNCH2(1680,1212),
      vSYNCV(5,576,5,33,5,4), vDISPLAY(287,2559,0,3,36,652) },

    /* 3: PAL-I (640x576(512)) */
    { 640, 576, 512, 1,
      vSMODE1(0, 1,1,1,1, 0,0, 0,0,0,4,0,0,1,1,0,3,0, 1,32,4), 1, 8,
      vSYNCH1(254,1474,127,262,48), vSYNCH2(1680,1212),
      vSYNCV(5,576,5,33,5,1), vDISPLAY(575,2559,0,3,72,652) },


    /* 4: VESA-1A (640x480 59.940Hz) */
    { 640, 480, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,2,0,0,1,0,0,0,0, 1,15,2), 0, 4,
      vSYNCH1(192,608,192,81,47), vSYNCH2(753,527),
      vSYNCV(2,480,0,33,0,10), vDISPLAY(479,1279,0,1,34,272) },

    /* 5: VESA-1C (640x480 75.000Hz) */
    { 640, 480, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,2,0,0,1,0,0,0,0, 1,28,3), 0, 4,
      vSYNCH1(128,712,128,225,47), vSYNCH2(793,487),
      vSYNCV(3,480,0,16,0,1), vDISPLAY(479,1279,0,1,18,352) },


    /* 6:  VESA-2B (800x600 60.317Hz) */
    { 800, 600, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,2,0,0,1,0,0,0,0, 1,71,6), 0, 4,
      vSYNCH1(256,800,256,161,95), vSYNCH2(961,639),
      vSYNCV(4,600,0,23,0,1), vDISPLAY(599,1599,0,1,26,416) },

    /* 7: VESA-2D (800x600 75.000Hz) */
    { 800, 600, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,2,0,0,1,0,0,0,0, 1,44,3), 0, 4,
      vSYNCH1(160,896,160,305,47), vSYNCH2(1009,591),
      vSYNCV(3,600,0,21,0,1), vDISPLAY(599,1599,0,1,23,464) },


    /* 8: VESA-3B (1024x768 60.004Hz) */
    { 1024, 768, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,2,0,0,1,0,0,0,0, 0,58,6), 0, 4,
      vSYNCH1(272,1072,272,305,63), vSYNCH2(1281,767),
      vSYNCV(6,768,0,29,0,3), vDISPLAY(767,2047,0,1,34,576) },

    /* 9: VESA-3D (1024x768 75.029Hz) */
    { 1024, 768, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,1,0,0,1,0,0,0,0, 1,35,3), 0, 2,
      vSYNCH1(96,560,96,161,31), vSYNCH2(625,399),
      vSYNCV(3,768,0,28,0,1), vDISPLAY(767,1023,0,0,30,256) },


    /* 10: VESA-4A (1280x1024 60.020Hz) */
    { 1280, 1024, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,1,0,0,1,0,0,0,0, 0,8,1), 0, 2,
      vSYNCH1(112,732,112,233,63), vSYNCH2(781,499),
      vSYNCV(3,1024,0,38,0,1), vDISPLAY(1023,1279,0,0,40,344) },

    /* 11: VESA-4B (1280x1024 75.025Hz) */
    { 1280, 1024, -1, 2,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,1,0,0,1,0,0,0,0, 0,10,1), 0, 2,
      vSYNCH1(144,700,144,233,31), vSYNCH2(813,467),
      vSYNCV(3,1024,0,38,0,1), vDISPLAY(1023,1279,0,0,40,376) },


    /* 12: DTV-480P (720x480) */
    { 720, 480, -1, 3,
      vSMODE1(1, 1,1,1,1, 0,0, 0,0,0,2,0,0,1,1,0,0,0, 1,32,4), 0, 4,
      vSYNCH1(128,730,128,101,47), vSYNCH2(811,629),
      vSYNCV(6,483,0,30,0,6), vDISPLAY(479,1439,0,1,35,228) },

    /* 13: DTV-1080I (1920x1080) */
    { 1920, 1080, -1, 4,
      vSMODE1(0, 0,1,1,1, 0,0, 0,0,0,1,0,0,1,0,0,0,0, 1,22,2), 1, 4,
      vSYNCH1(104,1056,44,131,45), vSYNCH2(1012,908),
      vSYNCV(10,1080,2,28,0,5), vDISPLAY(1079,1919,0,0,40,234) },

    /* 14: DTV-720P (1280x720) */
    { 1280, 720, -1, 5,
      vSMODE1(1, 0,1,1,1, 0,0, 0,0,0,1,0,0,1,0,0,0,0, 1,22,2), 0, 4,
      vSYNCH1(104,785,40,195,71), vSYNCH2(715,565),
      vSYNCV(5,720,0,20,0,5), vDISPLAY(719,1279,0,0,24,298) },
};

static const struct syncparam *syncdata = syncdata0;

int ps2_sysconf_video = 0; /* XXX: guessed!!! */

/*
 *  low-level PCRTC initialize
 */

void setcrtc_old(int mode, int ffmd, int noreset)
{
    uint64_t smode1 = syncdata[mode].smode1;

    if (syncdata[mode].dvemode != 2)		/* not VESA */
		smode1 |= (uint64_t)(ps2_sysconf_video & 1) << 25;	/* RGBYC */

    if (!noreset)
		ps2gs_set_gssreg(PS2_GSSREG_SMODE1, smode1 | ((uint64_t)1 << 16));
    ps2gs_set_gssreg(PS2_GSSREG_SYNCH1, syncdata[mode].synch1);
    ps2gs_set_gssreg(PS2_GSSREG_SYNCH2, syncdata[mode].synch2);
    ps2gs_set_gssreg(PS2_GSSREG_SYNCV, syncdata[mode].syncv);
    ps2gs_set_gssreg(PS2_GSSREG_SMODE2, syncdata[mode].smode2 + (ffmd << 1));
    ps2gs_set_gssreg(PS2_GSSREG_SRFSH, syncdata[mode].srfsh);

    if (!noreset &&
	(syncdata[mode].dvemode == 2 ||
	 syncdata[mode].dvemode == 4 ||
	 syncdata[mode].dvemode == 5)) {	/* for VESA, DTV1080I,720P */
		/* PLL on */
		ps2gs_set_gssreg(PS2_GSSREG_SMODE1, smode1 & ~((uint64_t)1 << 16));
		udelay(2500);	/* wait 2.5ms */
    }

    /* sync start */
    ps2gs_set_gssreg(PS2_GSSREG_SMODE1,
		     smode1 & ~((uint64_t)1 << 16) & ~((uint64_t)1 << 17));
    if (!noreset)
		setdve(syncdata[mode].dvemode);

    /* get DISPLAY register offset */
    gs_dx[0] = gs_dx[1] = syncdata[mode].display.dx;
    gs_dy[0] = gs_dy[1] = syncdata[mode].display.dy;
}

// int_mode
#define NON_INTERLACED	0
#define INTERLACED	1

// ntsc_pal
#define NTSC	2
#define PAL	3

// field_mode
#define FRAME	1
#define FIELD	2

#define GS_CSR (0x12001000 | KSEG1)
#define GS_IMR (0x12001010 | KSEG1)

static volatile uint64_t *gs_csr = (unsigned long *) GS_CSR;
static volatile uint64_t *gs_imr = (unsigned long *) GS_IMR;

/** Setup graphic mode. */
uint32_t syscallSetCrtc(int int_mode, int ntsc_pal, int field_mode)
{
	/* gs_set_crtc: */
	extern void setcrtc_old(int mode, int ffmd, int noreset);
	int mode;
	int res;
	int ffmd;

	printf("SetCrtc mode %d\n", int_mode);

	res = 0; /* Guessed!!! */
	ffmd = (res >> 16) & 0x01;

	if (ntsc_pal == NTSC) {
		mode = 0;
	} else if (ntsc_pal == PAL) {
		mode = 2;
	} else {
		printf("gs_set_crtc: unknown mode using PAL.\n");
		mode = 2;
	}
	if (int_mode == INTERLACED) {
		mode++;
	}
	setcrtc_old(mode, ffmd, 0);

	return 0;
}

uint32_t syscallSetGsIMR(uint32_t imr)
{
	*gs_imr = imr;
	return 0;
}

void graphic_init_module(void)
{
	/* Setup interrupts. */
	*gs_imr = 0xff00;
	*gs_csr = 0x00ff;
}

