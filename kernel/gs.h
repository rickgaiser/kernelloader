#ifndef __PS2_GS_H
#define __PS2_GS_H

#include "stdint.h"

/* crtmode : mode */
#define PS2_GS_VESA	0
#define PS2_GS_DTV	1
#define PS2_GS_NTSC	2
#define PS2_GS_PAL	3

/* crtmode : res (NTSC, PAL) */
#define PS2_GS_NOINTERLACE	0
#define PS2_GS_INTERLACE	1
#define PS2_GS_FIELD		0x00000
#define PS2_GS_FRAME		0x10000

/* crtmode : res (VESA) */
#define PS2_GS_640x480		0
#define PS2_GS_800x600		1
#define PS2_GS_1024x768		2
#define PS2_GS_1280x1024	3
#define PS2_GS_60Hz		0x0100
#define PS2_GS_75Hz		0x0200

/* crtmode : res (DTV) */
#define PS2_GS_480P	0
#define PS2_GS_1080I	1
#define PS2_GS_720P	2

/* GS pixel format */
#define PS2_GS_PSMCT32		0
#define PS2_GS_PSMCT24		1
#define PS2_GS_PSMCT16		2
#define PS2_GS_PSMCT16S		10
#define PS2_GS_PSMT8		19
#define PS2_GS_PSMT4		20
#define PS2_GS_PSMT8H		27
#define PS2_GS_PSMT4HL		36
#define PS2_GS_PSMT4HH		44
#define PS2_GS_PSMZ32		48
#define PS2_GS_PSMZ24		49
#define PS2_GS_PSMZ16		50
#define PS2_GS_PSMZ16S		58

/* GS registers */
#define PS2_GS_PRIM		0x00
#define PS2_GS_RGBAQ		0x01
#define PS2_GS_ST		0x02
#define PS2_GS_UV		0x03
#define PS2_GS_XYZF2		0x04
#define PS2_GS_XYZ2		0x05
#define PS2_GS_TEX0_1		0x06
#define PS2_GS_TEX0_2		0x07
#define PS2_GS_CLAMP_1		0x08
#define PS2_GS_CLAMP_2		0x09
#define PS2_GS_FOG		0x0a
#define PS2_GS_XYZF3		0x0c
#define PS2_GS_XYZ3		0x0d
#define PS2_GS_TEX1_1		0x14
#define PS2_GS_TEX1_2		0x15
#define PS2_GS_TEX2_1		0x16
#define PS2_GS_TEX2_2		0x17
#define PS2_GS_XYOFFSET_1	0x18
#define PS2_GS_XYOFFSET_2	0x19
#define PS2_GS_PRMODECONT	0x1a
#define PS2_GS_PRMODE		0x1b
#define PS2_GS_TEXCLUT		0x1c
#define PS2_GS_SCANMSK		0x22
#define PS2_GS_MIPTBP1_1	0x34
#define PS2_GS_MIPTBP1_2	0x35
#define PS2_GS_MIPTBP2_1	0x36
#define PS2_GS_MIPTBP2_2	0x37
#define PS2_GS_TEXA		0x3b
#define PS2_GS_FOGCOL		0x3d
#define PS2_GS_TEXFLUSH		0x3f
#define PS2_GS_SCISSOR_1	0x40
#define PS2_GS_SCISSOR_2	0x41
#define PS2_GS_ALPHA_1		0x42
#define PS2_GS_ALPHA_2		0x43
#define PS2_GS_DIMX		0x44
#define PS2_GS_DTHE		0x45
#define PS2_GS_COLCLAMP		0x46
#define PS2_GS_TEST_1		0x47
#define PS2_GS_TEST_2		0x48
#define PS2_GS_PABE		0x49
#define PS2_GS_FBA_1		0x4a
#define PS2_GS_FBA_2		0x4b
#define PS2_GS_FRAME_1		0x4c
#define PS2_GS_FRAME_2		0x4d
#define PS2_GS_ZBUF_1		0x4e
#define PS2_GS_ZBUF_2		0x4f
#define PS2_GS_BITBLTBUF	0x50
#define PS2_GS_TRXPOS		0x51
#define PS2_GS_TRXREG		0x52
#define PS2_GS_TRXDIR		0x53
#define PS2_GS_HWREG		0x54
#define PS2_GS_SIGNAL		0x60
#define PS2_GS_FINISH		0x61
#define PS2_GS_LABEL		0x62
#define PS2_GS_NOP		0x7f

/* GS register setting utils */
#define PS2_GS_SETREG_ALPHA_1	PS2_GS_SET_ALPHA
#define PS2_GS_SETREG_ALPHA_2	PS2_GS_SET_ALPHA
#define PS2_GS_SETREG_ALPHA(a, b, c, d, fix) \
	((uint64_t)(a)       | ((uint64_t)(b) << 2)     | ((uint64_t)(c) << 4) | \
	((uint64_t)(d) << 6) | ((uint64_t)(fix) << 32))

#define PS2_GS_SETREG_BITBLTBUF(sbp, sbw, spsm, dbp, dbw, dpsm) \
	((uint64_t)(sbp)         | ((uint64_t)(sbw) << 16) | \
	((uint64_t)(spsm) << 24) | ((uint64_t)(dbp) << 32) | \
	((uint64_t)(dbw) << 48)  | ((uint64_t)(dpsm) << 56))

#define PS2_GS_SETREG_CLAMP_1	PS2_GS_SET_CLAMP
#define PS2_GS_SETREG_CLAMP_2	PS2_GS_SET_CLAMP
#define PS2_GS_SETREG_CLAMP(wms, wmt, minu, maxu, minv, maxv) \
	((uint64_t)(wms)         | ((uint64_t)(wmt) << 2) | \
	((uint64_t)(minu) << 4)  | ((uint64_t)(maxu) << 14) | \
	((uint64_t)(minv) << 24) | ((uint64_t)(maxv) << 34))

#define PS2_GS_SETREG_COLCLAMP(clamp) ((uint64_t)(clamp))

#define PS2_GS_SETREG_DIMX(dm00, dm01, dm02, dm03, dm10, dm11, dm12, dm13, \
			dm20, dm21, dm22, dm23, dm30, dm31, dm32, dm33) \
	((uint64_t)(dm00)        | ((uint64_t)(dm01) << 4)  | \
	((uint64_t)(dm02) << 8)  | ((uint64_t)(dm03) << 12) | \
	((uint64_t)(dm10) << 16) | ((uint64_t)(dm11) << 20) | \
	((uint64_t)(dm12) << 24) | ((uint64_t)(dm13) << 28) | \
	((uint64_t)(dm20) << 32) | ((uint64_t)(dm21) << 36) | \
	((uint64_t)(dm22) << 40) | ((uint64_t)(dm23) << 44) | \
	((uint64_t)(dm30) << 48) | ((uint64_t)(dm31) << 52) | \
	((uint64_t)(dm32) << 56) | ((uint64_t)(dm33) << 60))

#define PS2_GS_SETREG_DTHE(dthe) ((uint64_t)(dthe))

#define PS2_GS_SETREG_FBA_1	PS2_GS_SETREG_FBA
#define PS2_GS_SETREG_FBA_2	PS2_GS_SETREG_FBA
#define PS2_GS_SETREG_FBA(fba) ((uint64_t)(fba))

#define PS2_GS_SETREG_FOG(f) ((uint64_t)(f) << 56)

#define PS2_GS_SETREG_FOGCOL(fcr, fcg, fcb) \
	((uint64_t)(fcr) | ((uint64_t)(fcg) << 8) | ((uint64_t)(fcb) << 16))

#define PS2_GS_SETREG_FRAME_1	PS2_GS_SETREG_FRAME
#define PS2_GS_SETREG_FRAME_2	PS2_GS_SETREG_FRAME
#define PS2_GS_SETREG_FRAME(fbp, fbw, psm, fbmask) \
	((uint64_t)(fbp)        | ((uint64_t)(fbw) << 16) | \
	((uint64_t)(psm) << 24) | ((uint64_t)(fbmask) << 32))

#define PS2_GS_SETREG_LABEL(id, idmsk) \
	((uint64_t)(id) | ((uint64_t)(idmsk) << 32))

#define PS2_GS_SETREG_MIPTBP1_1	PS2_GS_SETREG_MIPTBP1
#define PS2_GS_SETREG_MIPTBP1_2	PS2_GS_SETREG_MIPTBP1
#define PS2_GS_SETREG_MIPTBP1(tbp1, tbw1, tbp2, tbw2, tbp3, tbw3) \
	((uint64_t)(tbp1)        | ((uint64_t)(tbw1) << 14) | \
	((uint64_t)(tbp2) << 20) | ((uint64_t)(tbw2) << 34) | \
	((uint64_t)(tbp3) << 40) | ((uint64_t)(tbw3) << 54))

#define PS2_GS_SETREG_MIPTBP2_1	PS2_GS_SETREG_MIPTBP2
#define PS2_GS_SETREG_MIPTBP2_2	PS2_GS_SETREG_MIPTBP2
#define PS2_GS_SETREG_MIPTBP2(tbp4, tbw4, tbp5, tbw5, tbp6, tbw6) \
	((uint64_t)(tbp4)        | ((uint64_t)(tbw4) << 14) | \
	((uint64_t)(tbp5) << 20) | ((uint64_t)(tbw5) << 34) | \
	((uint64_t)(tbp6) << 40) | ((uint64_t)(tbw6) << 54))

#define PS2_GS_SETREG_PABE(pabe) ((uint64_t)(pabe))

#define PS2_GS_SETREG_PRIM(prim, iip, tme, fge, abe, aa1, fst, ctxt, fix) \
	((uint64_t)(prim)      | ((uint64_t)(iip) << 3)  | ((uint64_t)(tme) << 4) | \
	((uint64_t)(fge) << 5) | ((uint64_t)(abe) << 6)  | ((uint64_t)(aa1) << 7) | \
	((uint64_t)(fst) << 8) | ((uint64_t)(ctxt) << 9) | ((uint64_t)(fix) << 10))

#define PS2_GS_SETREG_PRMODE(iip, tme, fge, abe, aa1, fst, ctxt, fix) \
	(((uint64_t)(iip) << 3) | ((uint64_t)(tme) << 4)  | \
	 ((uint64_t)(fge) << 5) | ((uint64_t)(abe) << 6)  | ((uint64_t)(aa1) << 7) | \
	 ((uint64_t)(fst) << 8) | ((uint64_t)(ctxt) << 9) | ((uint64_t)(fix) << 10))

#define PS2_GS_SETREG_PRMODECONT(ac) ((uint64_t)(ac))

#define PS2_GS_SETREG_RGBAQ(r, g, b, a, q) \
	((uint64_t)(r)        | ((uint64_t)(g) << 8) | ((uint64_t)(b) << 16) | \
	((uint64_t)(a) << 24) | ((uint64_t)(q) << 32))

#define PS2_GS_SETREG_SCANMSK(msk) ((uint64_t)(msk))

#define PS2_GS_SETREG_SCISSOR_1	PS2_GS_SETREG_SCISSOR
#define PS2_GS_SETREG_SCISSOR_2	PS2_GS_SETREG_SCISSOR
#define PS2_GS_SETREG_SCISSOR(scax0, scax1, scay0, scay1) \
	((uint64_t)(scax0)        | ((uint64_t)(scax1) << 16) | \
	((uint64_t)(scay0) << 32) | ((uint64_t)(scay1) << 48))

#define PS2_GS_SETREG_SIGNAL(id, idmsk) \
	((uint64_t)(id) | ((uint64_t)(idmsk) << 32))

#define PS2_GS_SETREG_ST(s, t) ((uint64_t)(s) |  ((uint64_t)(t) << 32))

#define PS2_GS_SETREG_TEST_1 PS2_GS_SETREG_TEST
#define PS2_GS_SETREG_TEST_2 PS2_GS_SETREG_TEST
#define PS2_GS_SETREG_TEST(ate, atst, aref, afail, date, datm, zte, ztst) \
	((uint64_t)(ate)         | ((uint64_t)(atst) << 1) | \
	((uint64_t)(aref) << 4)  | ((uint64_t)(afail) << 12) | \
	((uint64_t)(date) << 14) | ((uint64_t)(datm) << 15) | \
	((uint64_t)(zte) << 16)  | ((uint64_t)(ztst) << 17))

#define PS2_GS_SETREG_TEX0_1	PS2_GS_SETREG_TEX0
#define PS2_GS_SETREG_TEX0_2	PS2_GS_SETREG_TEX0
#define PS2_GS_SETREG_TEX0(tbp, tbw, psm, tw, th, tcc, tfx, \
			   cbp, cpsm, csm, csa, cld) \
	((uint64_t)(tbp)         | ((uint64_t)(tbw) << 14) | \
	((uint64_t)(psm) << 20)  | ((uint64_t)(tw) << 26) | \
	((uint64_t)(th) << 30)   | ((uint64_t)(tcc) << 34) | \
	((uint64_t)(tfx) << 35)  | ((uint64_t)(cbp) << 37) | \
	((uint64_t)(cpsm) << 51) | ((uint64_t)(csm) << 55) | \
	((uint64_t)(csa) << 56)  | ((uint64_t)(cld) << 61))

#define PS2_GS_SETREG_TEX1_1	PS2_GS_SETREG_TEX1
#define PS2_GS_SETREG_TEX1_2	PS2_GS_SETREG_TEX1
#define PS2_GS_SETREG_TEX1(lcm, mxl, mmag, mmin, mtba, l, k) \
	((uint64_t)(lcm)        | ((uint64_t)(mxl) << 2)  | \
	((uint64_t)(mmag) << 5) | ((uint64_t)(mmin) << 6) | \
	((uint64_t)(mtba) << 9) | ((uint64_t)(l) << 19) | \
	((uint64_t)(k) << 32))

#define PS2_GS_SETREG_TEX2_1	PS2_GS_SETREG_TEX2
#define PS2_GS_SETREG_TEX2_2	PS2_GS_SETREG_TEX2
#define PS2_GS_SETREG_TEX2(psm, cbp, cpsm, csm, csa, cld) \
	(((uint64_t)(psm) << 20) | ((uint64_t)(cbp) << 37) | \
	((uint64_t)(cpsm) << 51) | ((uint64_t)(csm) << 55) | \
	((uint64_t)(csa) << 56)  | ((uint64_t)(cld) << 61))

#define PS2_GS_SETREG_TEXA(ta0, aem, ta1) \
	((uint64_t)(ta0) | ((uint64_t)(aem) << 15) | ((uint64_t)(ta1) << 32))

#define PS2_GS_SETREG_TEXCLUT(cbw, cou, cov) \
	((uint64_t)(cbw) | ((uint64_t)(cou) << 6) | ((uint64_t)(cov) << 12))

#define PS2_GS_SETREG_TRXDIR(xdr) ((uint64_t)(xdr))

#define PS2_GS_SETREG_TRXPOS(ssax, ssay, dsax, dsay, dir) \
	((uint64_t)(ssax)        | ((uint64_t)(ssay) << 16) | \
	((uint64_t)(dsax) << 32) | ((uint64_t)(dsay) << 48) | \
	((uint64_t)(dir) << 59))

#define PS2_GS_SETREG_TRXREG(rrw, rrh) \
	((uint64_t)(rrw) | ((uint64_t)(rrh) << 32))

#define PS2_GS_SETREG_UV(u, v) ((uint64_t)(u) | ((uint64_t)(v) << 16))

#define PS2_GS_SETREG_XYOFFSET_1	PS2_GS_SETREG_XYOFFSET
#define PS2_GS_SETREG_XYOFFSET_2	PS2_GS_SETREG_XYOFFSET
#define PS2_GS_SETREG_XYOFFSET(ofx, ofy) ((uint64_t)(ofx) | ((uint64_t)(ofy) << 32))

#define PS2_GS_SETREG_XYZ3 PS2_GS_SETREG_XYZ
#define PS2_GS_SETREG_XYZ2 PS2_GS_SETREG_XYZ
#define PS2_GS_SETREG_XYZ(x, y, z) \
	((uint64_t)(x) | ((uint64_t)(y) << 16) | ((uint64_t)(z) << 32))

#define PS2_GS_SETREG_XYZF3 PS2_GS_SETREG_XYZF
#define PS2_GS_SETREG_XYZF2 PS2_GS_SETREG_XYZF
#define PS2_GS_SETREG_XYZF(x, y, z, f) \
	((uint64_t)(x) | ((uint64_t)(y) << 16) | ((uint64_t)(z) << 32) | \
	((uint64_t)(f) << 56))

#define PS2_GS_SETREG_ZBUF_1	PS2_GS_SETREG_ZBUF
#define PS2_GS_SETREG_ZBUF_2	PS2_GS_SETREG_ZBUF
#define PS2_GS_SETREG_ZBUF(zbp, psm, zmsk) \
	((uint64_t)(zbp) | ((uint64_t)(psm) << 24) | \
	((uint64_t)(zmsk) << 32))


/* GS special registers */
#define PS2_GSSREG_PMODE	0x00
#define PS2_GSSREG_SMODE1	0x01
#define PS2_GSSREG_SMODE2	0x02
#define PS2_GSSREG_SRFSH	0x03
#define PS2_GSSREG_SYNCH1	0x04
#define PS2_GSSREG_SYNCH2	0x05
#define PS2_GSSREG_SYNCV	0x06
#define PS2_GSSREG_DISPFB1	0x07
#define PS2_GSSREG_DISPLAY1	0x08
#define PS2_GSSREG_DISPFB2	0x09
#define PS2_GSSREG_DISPLAY2	0x0a
#define PS2_GSSREG_EXTBUF	0x0b
#define PS2_GSSREG_EXTDATA	0x0c
#define PS2_GSSREG_EXTWRITE	0x0d
#define PS2_GSSREG_BGCOLOR	0x0e
#define PS2_GSSREG_CSR		0x40
#define PS2_GSSREG_IMR		0x41
#define PS2_GSSREG_BUSDIR	0x44
#define PS2_GSSREG_SIGLBLID	0x48
#define PS2_GSSREG_SYSCNT	0x4f

/* GS register bit assign/define */
#define PS2_GS_CLEAR_GSREG(p)	*(uint64_t *)(p) = 0
/* ALPHA */
typedef struct {
	uint64_t A:      2 __attribute__((packed));
	uint64_t B:      2 __attribute__((packed));
	uint64_t C:      2 __attribute__((packed));
	uint64_t D:      2 __attribute__((packed));
	uint64_t pad8:  24 __attribute__((packed));
	uint64_t FIX:    8 __attribute__((packed));
	uint64_t pad40: 24 __attribute__((packed));
} ps2_gsreg_alpha __attribute__((packed));
#define PS2_GS_ALPHA_A_CS	0
#define PS2_GS_ALPHA_A_CD	1
#define PS2_GS_ALPHA_A_ZERO	2
#define PS2_GS_ALPHA_B_CS	0
#define PS2_GS_ALPHA_B_CD	1
#define PS2_GS_ALPHA_B_ZERO	2
#define PS2_GS_ALPHA_C_AS	0
#define PS2_GS_ALPHA_C_AD	1
#define PS2_GS_ALPHA_C_FIX	2
#define PS2_GS_ALPHA_D_CS	0
#define PS2_GS_ALPHA_D_CD	1
#define PS2_GS_ALPHA_D_ZERO	2

/* BITBLTBUF */
/** use ioctl PS2IOC_{LOAD,SAVE}IMAGE for HOST<->LOCAL xfer **/
typedef struct {
	uint64_t SBP:   14 __attribute__((packed));
	uint64_t pad14:  2 __attribute__((packed));
	uint64_t SBW:    6 __attribute__((packed));
	uint64_t pad22:  2 __attribute__((packed));
	uint64_t SPSM:   6 __attribute__((packed));
	uint64_t pad30:  2 __attribute__((packed));
	uint64_t DBP:   14 __attribute__((packed));
	uint64_t pad46:  2 __attribute__((packed));
	uint64_t DBW:    6 __attribute__((packed));
	uint64_t pad54:  2 __attribute__((packed));
	uint64_t DPSM:   6 __attribute__((packed));
	uint64_t pad62:  2 __attribute__((packed));
} ps2_gsreg_bitbltbuf __attribute__((packed));

/* CLAMP */
typedef struct {
	uint64_t WMS:    2 __attribute__((packed));
	uint64_t WMT:    2 __attribute__((packed));
	uint64_t MINU:  10 __attribute__((packed));
	uint64_t MAXU:  10 __attribute__((packed));
	uint64_t MINV:  10 __attribute__((packed));
	uint64_t MAXV:  10 __attribute__((packed));
	uint64_t pad44: 20 __attribute__((packed));
} ps2_gsreg_clamp __attribute__((packed));
#define PS2_GS_CLAMP_REPEAT		0
#define PS2_GS_CLAMP_CLAMP		1
#define PS2_GS_CLAMP_REGION_CLAMP	2
#define PS2_GS_CLAMP_REGION_REPEAT	3

/* COLCLAMP */
typedef struct {
	uint64_t CLAMP:  1 __attribute__((packed));
	uint64_t pad01: 63 __attribute__((packed));
} ps2_gsreg_colclamp __attribute__((packed));
#define PS2_GS_COLCLAMP_MASK		0
#define PS2_GS_COLCLAMP_CLAMP		1


/* DIMX */
typedef struct {
	uint64_t DM00:  3 __attribute__((packed));
	uint64_t pad03: 1 __attribute__((packed));
	uint64_t DM01:  3 __attribute__((packed));
	uint64_t pad07: 1 __attribute__((packed));
	uint64_t DM02:  3 __attribute__((packed));
	uint64_t pad11: 1 __attribute__((packed));
	uint64_t DM03:  3 __attribute__((packed));
	uint64_t pad15: 1 __attribute__((packed));
	uint64_t DM10:  3 __attribute__((packed));
	uint64_t pad19: 1 __attribute__((packed));
	uint64_t DM11:  3 __attribute__((packed));
	uint64_t pad23: 1 __attribute__((packed));
	uint64_t DM12:  3 __attribute__((packed));
	uint64_t pad27: 1 __attribute__((packed));
	uint64_t DM13:  3 __attribute__((packed));
	uint64_t pad31: 1 __attribute__((packed));
	uint64_t DM20:  3 __attribute__((packed));
	uint64_t pad35: 1 __attribute__((packed));
	uint64_t DM21:  3 __attribute__((packed));
	uint64_t pad39: 1 __attribute__((packed));
	uint64_t DM22:  3 __attribute__((packed));
	uint64_t pad43: 1 __attribute__((packed));
	uint64_t DM23:  3 __attribute__((packed));
	uint64_t pad47: 1 __attribute__((packed));
	uint64_t DM30:  3 __attribute__((packed));
	uint64_t pad51: 1 __attribute__((packed));
	uint64_t DM31:  3 __attribute__((packed));
	uint64_t pad55: 1 __attribute__((packed));
	uint64_t DM32:  3 __attribute__((packed));
	uint64_t pad59: 1 __attribute__((packed));
	uint64_t DM33:  3 __attribute__((packed));
	uint64_t pad63: 1 __attribute__((packed));
} ps2_gsreg_dimx __attribute__((packed));

/* DTHE */
typedef struct {
	uint64_t DTHE:   1 __attribute__((packed));
	uint64_t pad01: 63 __attribute__((packed));
} ps2_gsreg_dthe __attribute__((packed));
#define PS2_GS_DTHE_OFF		0
#define PS2_GS_DTHE_ON		1

/* FBA */
typedef struct {
	uint64_t FBA:   1 __attribute__((packed));
	uint64_t pad01: 63 __attribute__((packed));
} ps2_gsreg_fba __attribute__((packed));

/* FINISH */
typedef struct {
	uint64_t pad00: 64 __attribute__((packed));
} ps2_gsreg_finish __attribute__((packed));

/* FOG */
typedef struct {
	uint64_t pad00: 56 __attribute__((packed));
	uint64_t F:      8 __attribute__((packed));
} ps2_gsreg_fog __attribute__((packed));

/* FOGCOL */
typedef struct {
	uint64_t FCR:    8 __attribute__((packed));
	uint64_t FCG:    8 __attribute__((packed));
	uint64_t FCB:    8 __attribute__((packed));
	uint64_t pad24: 40 __attribute__((packed));
} ps2_gsreg_fogcol __attribute__((packed));

/* FRAME */
typedef struct {
	uint64_t FBP:    9 __attribute__((packed));
	uint64_t pad09:  7 __attribute__((packed));
	uint64_t FBW:    6 __attribute__((packed));
	uint64_t pad22:  2 __attribute__((packed));
	uint64_t PSM:    6 __attribute__((packed));
	uint64_t pad30:  2 __attribute__((packed));
	uint64_t FBMSK: 32 __attribute__((packed));
} ps2_gsreg_frame __attribute__((packed));

/* HWREG */
/** use ioctl PS2IOC_{LOAD,SAVE}IMAGE **/
typedef struct {
	uint64_t DATA: 64 __attribute__((packed));
} ps2_gsreg_hwreg __attribute__((packed));

/* LABEL */
typedef struct {
	uint64_t ID:    32 __attribute__((packed));
	uint64_t IDMSK: 32 __attribute__((packed));
} ps2_gsreg_label __attribute__((packed));

/* MIPTBP1 */
typedef struct {
	uint64_t TBP1:  14 __attribute__((packed));
	uint64_t TBW1:   6 __attribute__((packed));
	uint64_t TBP2:  14 __attribute__((packed));
	uint64_t TBW2:   6 __attribute__((packed));
	uint64_t TBP3:  14 __attribute__((packed));
	uint64_t TBW3:   6 __attribute__((packed));
	uint64_t pad60:  4 __attribute__((packed));
} ps2_gsreg_miptbp1 __attribute__((packed));

/* MIPTBP2 */
typedef struct {
	uint64_t TBP4:  14 __attribute__((packed));
	uint64_t TBW4:   6 __attribute__((packed));
	uint64_t TBP5:  14 __attribute__((packed));
	uint64_t TBW5:   6 __attribute__((packed));
	uint64_t TBP6:  14 __attribute__((packed));
	uint64_t TBW6:   6 __attribute__((packed));
	uint64_t pad60:  4 __attribute__((packed));
} ps2_gsreg_miptbp2 __attribute__((packed));

/* PABE */
typedef struct {
	uint64_t PABE:   1 __attribute__((packed));
	uint64_t pad01: 63 __attribute__((packed));
} ps2_gsreg_pabe __attribute__((packed));
#define PS2_GS_PABE_OFF		0
#define PS2_GS_PABE_ON		1

/* PRIM */
typedef struct {
	uint64_t PRIM:   3 __attribute__((packed));
	uint64_t IIP:    1 __attribute__((packed));
	uint64_t TME:    1 __attribute__((packed));
	uint64_t FGE:    1 __attribute__((packed));
	uint64_t ABE:    1 __attribute__((packed));
	uint64_t AA1:    1 __attribute__((packed));
	uint64_t FST:    1 __attribute__((packed));
	uint64_t CTXT:   1 __attribute__((packed));
	uint64_t FIX:    1 __attribute__((packed));
	uint64_t pad11: 53 __attribute__((packed));
} ps2_gsreg_prim __attribute__((packed));
#define PS2_GS_PRIM_PRIM_POINT		0
#define PS2_GS_PRIM_PRIM_LINE		1
#define PS2_GS_PRIM_PRIM_LINESTRIP	2
#define PS2_GS_PRIM_PRIM_TRIANGLE	3
#define PS2_GS_PRIM_PRIM_TRISTRIP	4
#define PS2_GS_PRIM_PRIM_TRIFAN		5
#define PS2_GS_PRIM_PRIM_SPRITE		6
#define PS2_GS_PRIM_IIP_FLAT		0
#define PS2_GS_PRIM_IIP_GOURAUD		1
#define PS2_GS_PRIM_TME_OFF		0
#define PS2_GS_PRIM_TME_ON		1
#define PS2_GS_PRIM_FGE_OFF		0
#define PS2_GS_PRIM_FGE_ON		1
#define PS2_GS_PRIM_ABE_OFF		0
#define PS2_GS_PRIM_ABE_ON		1
#define PS2_GS_PRIM_AA1_OFF		0
#define PS2_GS_PRIM_AA1_ON		1
#define PS2_GS_PRIM_FST_STQ		0
#define PS2_GS_PRIM_FST_UV		1
#define PS2_GS_PRIM_CTXT_CONTEXT1	0
#define PS2_GS_PRIM_CTXT_CONTEXT2	1
#define PS2_GS_PRIM_FIX_NOFIXDDA	0
#define PS2_GS_PRIM_FIX_FIXDDA		1

/* PRMODE */
typedef struct {
	uint64_t pad00:  3 __attribute__((packed));
	uint64_t IIP:    1 __attribute__((packed));
	uint64_t TME:    1 __attribute__((packed));
	uint64_t FGE:    1 __attribute__((packed));
	uint64_t ABE:    1 __attribute__((packed));
	uint64_t AA1:    1 __attribute__((packed));
	uint64_t FST:    1 __attribute__((packed));
	uint64_t CTXT:   1 __attribute__((packed));
	uint64_t FIX:    1 __attribute__((packed));
	uint64_t pad11: 53 __attribute__((packed));
} ps2_gsreg_prmode __attribute__((packed));
/* use PRIM defines */

/* PRMODECONT */
typedef struct {
	uint64_t AC:     1 __attribute__((packed));
	uint64_t pad01: 63 __attribute__((packed));
} ps2_gsreg_prmodecont __attribute__((packed));
#define PS2_GS_PRMODECONT_REFPRMODE	0
#define PS2_GS_PRMODECONT_REFPRIM	1

/* RGBAQ */
typedef struct {
	uint64_t R: 8 __attribute__((packed));
	uint64_t G: 8 __attribute__((packed));
	uint64_t B: 8 __attribute__((packed));
	uint64_t A: 8 __attribute__((packed));
	float Q    __attribute__((packed));
} ps2_gsreg_rgbaq __attribute__((packed));

/* SCANMSK */
typedef struct {
	uint64_t MSK:    2 __attribute__((packed));
	uint64_t pad02: 62 __attribute__((packed));
} ps2_gsreg_scanmsk __attribute__((packed));
#define PS2_GS_SCANMSK_NOMASK		0
#define PS2_GS_SCANMSK_MASKEVEN		2
#define PS2_GS_SCANMSK_MASKODD		3

/* SCISSOR */
typedef struct {
	uint64_t SCAX0: 11 __attribute__((packed));
	uint64_t pad11:  5 __attribute__((packed));
	uint64_t SCAX1: 11 __attribute__((packed));
	uint64_t pad27:  5 __attribute__((packed));
	uint64_t SCAY0: 11 __attribute__((packed));
	uint64_t pad43:  5 __attribute__((packed));
	uint64_t SCAY1: 11 __attribute__((packed));
	uint64_t pad59:  5 __attribute__((packed));
} ps2_gsreg_scissor __attribute__((packed));

/* SIGNAL */
typedef struct {
	uint64_t ID:    32 __attribute__((packed));
	uint64_t IDMSK: 32 __attribute__((packed));
} ps2_gsreg_signal __attribute__((packed));

/* ST */
typedef struct {
	float S __attribute__((packed));
	float T __attribute__((packed));
} ps2_gsreg_st __attribute__((packed));

/* TEST */
typedef struct {
	uint64_t ATE:    1 __attribute__((packed));
	uint64_t ATST:   3 __attribute__((packed));
	uint64_t AREF:   8 __attribute__((packed));
	uint64_t AFAIL:  2 __attribute__((packed));
	uint64_t DATE:   1 __attribute__((packed));
	uint64_t DATM:   1 __attribute__((packed));
	uint64_t ZTE:    1 __attribute__((packed));
	uint64_t ZTST:   2 __attribute__((packed));
	uint64_t pad19: 45 __attribute__((packed));
} ps2_gsreg_test __attribute__((packed));
#define PS2_GS_TEST_ATE_OFF		0
#define PS2_GS_TEST_ATE_ON		1
#define PS2_GS_TEST_ATST_NEVER		0
#define PS2_GS_TEST_ATST_ALWAYS		1
#define PS2_GS_TEST_ATST_LESS		2
#define PS2_GS_TEST_ATST_LEQUAL		3
#define PS2_GS_TEST_ATST_EQUAL		4
#define PS2_GS_TEST_ATST_GEQUAL		5
#define PS2_GS_TEST_ATST_GREATER	6
#define PS2_GS_TEST_ATST_NOTEQUAL	7
#define PS2_GS_TEST_AFAIL_KEEP		0
#define PS2_GS_TEST_AFAIL_FB_ONLY	1
#define PS2_GS_TEST_AFAIL_ZB_ONLY	2
#define PS2_GS_TEST_AFAIL_RGB_ONLY	3
#define PS2_GS_TEST_DATE_OFF		0
#define PS2_GS_TEST_DATE_ON		1
#define PS2_GS_TEST_DATM_PASS0		0
#define PS2_GS_TEST_DATM_PASS1		1
#define PS2_GS_TEST_ZTE_OFF		0
#define PS2_GS_TEST_ZTE_ON		1
#define PS2_GS_TEST_ZTST_NEVER		0
#define PS2_GS_TEST_ZTST_ALWAYS		1
#define PS2_GS_TEST_ZTST_GEQUAL		2
#define PS2_GS_TEST_ZTST_GREATER	3
#define PS2_GS_ZNEVER		PS2_GS_TEST_ZTST_NEVER
#define PS2_GS_ZALWAYS		PS2_GS_TEST_ZTST_ALWAYS
#define PS2_GS_ZGEQUAL		PS2_GS_TEST_ZTST_GEQUAL
#define PS2_GS_ZGREATER		PS2_GS_TEST_ZTST_GREATER

/* TEX0 */
typedef struct {
	uint64_t TBP0: 14 __attribute__((packed));
	uint64_t TBW:   6 __attribute__((packed));
	uint64_t PSM:   6 __attribute__((packed));
	uint64_t TW:    4 __attribute__((packed));
	uint64_t TH:    4 __attribute__((packed));
	uint64_t TCC:   1 __attribute__((packed));
	uint64_t TFX:   2 __attribute__((packed));
	uint64_t CBP:  14 __attribute__((packed));
	uint64_t CPSM:  4 __attribute__((packed));
	uint64_t CSM:   1 __attribute__((packed));
	uint64_t CSA:   5 __attribute__((packed));
	uint64_t CLD:   3 __attribute__((packed));
} ps2_gsreg_tex0 __attribute__((packed));
#define PS2_GS_TEX_TCC_RGB			0
#define PS2_GS_TEX_TCC_RGBA			1
#define PS2_GS_TEX_TFX_MODULATE			0
#define PS2_GS_TEX_TFX_DECAL			1
#define PS2_GS_TEX_TFX_HIGHLIGHT		2
#define PS2_GS_TEX_TFX_HIGHLIGHT2		3
#define PS2_GS_TEX_CSM_CSM1			0
#define PS2_GS_TEX_CSM_CSM2			1
#define PS2_GS_TEX_CLD_NOUPDATE			0
#define PS2_GS_TEX_CLD_LOAD			1
#define PS2_GS_TEX_CLD_LOAD_COPY0		2
#define PS2_GS_TEX_CLD_LOAD_COPY1		3
#define PS2_GS_TEX_CLD_TEST0_LOAD_COPY0		4
#define PS2_GS_TEX_CLD_TEST1_LOAD_COPY1		5

/* TEX1 */
typedef struct {
	uint64_t LCM:    1 __attribute__((packed));
	uint64_t pad01:  1 __attribute__((packed));
	uint64_t MXL:    3 __attribute__((packed));
	uint64_t MMAG:   1 __attribute__((packed));
	uint64_t MMIN:   3 __attribute__((packed));
	uint64_t MTBA:   1 __attribute__((packed));
	uint64_t pad10:  9 __attribute__((packed));
	uint64_t L:      2 __attribute__((packed));
	uint64_t pad21: 11 __attribute__((packed));
	uint64_t K:     12 __attribute__((packed));
	uint64_t pad44: 20 __attribute__((packed));
} ps2_gsreg_tex1 __attribute__((packed));
#define PS2_GS_TEX1_LCM_CALC				0
#define PS2_GS_TEX1_LCM_K				1
#define PS2_GS_TEX1_MMAG_NEAREST			0
#define PS2_GS_TEX1_MMAG_LINEAR				1
#define PS2_GS_TEX1_MMIN_NEAREST			0
#define PS2_GS_TEX1_MMIN_LINEAR				1
#define PS2_GS_TEX1_MMIN_NEAREST_MIPMAP_NEAREST		2
#define PS2_GS_TEX1_MMIN_NEAREST_MIPMAP_LINEAR		3
#define PS2_GS_TEX1_MMIN_LINEAR_MIPMAP_NEAREST		4
#define PS2_GS_TEX1_MMIN_LINEAR_MIPMAP_LINEAR		5
#define PS2_GS_TEX1_MTBA_NOAUTO				0
#define PS2_GS_TEX1_MTBA_AUTO				1

/* TEX2 */
typedef struct {
	uint64_t pad00: 20 __attribute__((packed));
	uint64_t PSM:    6 __attribute__((packed));
	uint64_t pad26: 11 __attribute__((packed));
	uint64_t CBP:   14 __attribute__((packed));
	uint64_t CPSM:   4 __attribute__((packed));
	uint64_t CSM:    1 __attribute__((packed));
	uint64_t CSA:    5 __attribute__((packed));
	uint64_t CLD:    3 __attribute__((packed));
} ps2_gsreg_tex2 __attribute__((packed));
/* use TEX0 defines */

/* TEXA */
typedef struct {
	uint64_t TA0:    8 __attribute__((packed));
	uint64_t pad08:  7 __attribute__((packed));
	uint64_t AEM:    1 __attribute__((packed));
	uint64_t pad16: 16 __attribute__((packed));
	uint64_t TA1:    8 __attribute__((packed));
	uint64_t pad40: 24 __attribute__((packed));
} ps2_gsreg_texa __attribute__((packed));
#define PS2_GS_TEXA_AEM_NORMAL		0
#define PS2_GS_TEXA_AEM_BLACKTHRU	1

/* TEXCLUT */
typedef struct {
	uint64_t CBW:    6 __attribute__((packed));
	uint64_t COU:    6 __attribute__((packed));
	uint64_t COV:   10 __attribute__((packed));
	uint64_t pad22: 42 __attribute__((packed));
} ps2_gsreg_texclut;

/* TEXFLUSH */
typedef struct {
	uint64_t pad00: 64 __attribute__((packed));
} ps2_gsreg_texflush __attribute__((packed));

/* TRXDIR */
typedef struct {
	uint64_t XDR:    2 __attribute__((packed));
	uint64_t pad02: 62 __attribute__((packed));
} ps2_gsreg_trxdir __attribute__((packed));
#define PS2_GS_TRXDIR_HOST_TO_LOCAL	0
#define PS2_GS_TRXDIR_LOCAL_TO_HOST	1
#define PS2_GS_TRXDIR_LOCAL_TO_LOCAL	2

/* TRXPOS */
typedef struct {
	uint64_t SSAX:  11 __attribute__((packed));
	uint64_t pad11:  5 __attribute__((packed));
	uint64_t SSAY:  11 __attribute__((packed));
	uint64_t pad27:  5 __attribute__((packed));
	uint64_t DSAX:  11 __attribute__((packed));
	uint64_t pad43:  5 __attribute__((packed));
	uint64_t DSAY:  11 __attribute__((packed));
	uint64_t DIR:    2 __attribute__((packed));
	uint64_t pad61:  3 __attribute__((packed));
} ps2_gsreg_trxpos __attribute__((packed));
#define PS2_GS_TRXPOS_DIR_LR_UD		0
#define PS2_GS_TRXPOS_DIR_LR_DU		1
#define PS2_GS_TRXPOS_DIR_RL_UD		2
#define PS2_GS_TRXPOS_DIR_RL_DU		3

/* TRXREG */
typedef struct {
	uint64_t RRW:   12 __attribute__((packed));
	uint64_t pad12: 20 __attribute__((packed));
	uint64_t RRH:   12 __attribute__((packed));
	uint64_t pad44: 20 __attribute__((packed));
} ps2_gsreg_trxreg __attribute__((packed));

/* UV */
typedef struct {
	uint64_t U:     14 __attribute__((packed));
	uint64_t pad14:  2 __attribute__((packed));
	uint64_t V:     14 __attribute__((packed));
	uint64_t pad30: 34 __attribute__((packed));
} ps2_gsreg_uv __attribute__((packed));

/* XYOFFSET */
typedef struct {
	uint64_t OFX:   16 __attribute__((packed));
	uint64_t pad16: 16 __attribute__((packed));
	uint64_t OFY:   16 __attribute__((packed));
	uint64_t pad48: 16 __attribute__((packed));
} ps2_gsreg_xyoffset __attribute__((packed));

/* XYZ2/3 */
typedef struct {
	uint64_t X: 16 __attribute__((packed));
	uint64_t Y: 16 __attribute__((packed));
	uint64_t Z: 32 __attribute__((packed));
} ps2_gsreg_xyz __attribute__((packed));

/* XYZF2/3 */
typedef struct {
	uint64_t X: 16 __attribute__((packed));
	uint64_t Y: 16 __attribute__((packed));
	uint64_t Z: 24 __attribute__((packed));
	uint64_t F:  8 __attribute__((packed));
} ps2_gsreg_xyzf __attribute__((packed));

/* ZBUF */
typedef struct {
	uint64_t ZBP:    9 __attribute__((packed));
	uint64_t pad09: 15 __attribute__((packed));
	uint64_t PSM:    4 __attribute__((packed));
	uint64_t pad28:  4 __attribute__((packed));
	uint64_t ZMSK:   1 __attribute__((packed));
	uint64_t pad33: 31 __attribute__((packed));
} ps2_gsreg_zbuf __attribute__((packed));
#define PS2_GS_ZBUF_ZMSK_NOMASK	0
#define PS2_GS_ZBUF_ZMSK_MASK	1

/* GS special registers */
/* BGCOLOR */
typedef struct {
	uint64_t R:      8 __attribute__((packed));
	uint64_t G:      8 __attribute__((packed));
	uint64_t B:      8 __attribute__((packed));
	uint64_t pad24: 40 __attribute__((packed));
} ps2_gssreg_bgcolor __attribute__((packed));

/* BUSDIR */
/** ioctl, PS2IOC_{LOAD,SAVE}IMAGE **/
/** set always HOST_TO_LOCAL(0) **/

/* CSR */
/** see ps2gs_{en,jp}.txt **/
typedef struct {
	uint64_t SIGNAL:      1 __attribute__((packed)); /* ro */
	uint64_t FINISH:      1 __attribute__((packed)); /* ro */
	uint64_t HSINT:       1 __attribute__((packed)); /* ro */
	uint64_t VSINT:       1 __attribute__((packed)); /* ro */
	uint64_t reserved04:  3 __attribute__((packed)); /* ro */
	uint64_t pad07:       1 __attribute__((packed));
	uint64_t FLUSH:       1 __attribute__((packed)); /* rw */
	uint64_t RESET:       1 __attribute__((packed)); /* N/A */
	uint64_t pad10:       2 __attribute__((packed));
	uint64_t NFIELD:      1 __attribute__((packed)); /* ro */
	uint64_t FIELD:       1 __attribute__((packed)); /* ro */
	uint64_t FIFO:        2 __attribute__((packed)); /* ro */
	uint64_t REV:         8 __attribute__((packed)); /* ro */
	uint64_t ID:          8 __attribute__((packed)); /* ro */
	uint64_t pad32:      32 __attribute__((packed));
} ps2_gssreg_csr __attribute__((packed));
#define PS2_GS_CSR_FLUSH		1
#define PS2_GS_CSR_FIELD_EVEN		0
#define PS2_GS_CSR_FIELD_ODD		1
#define PS2_GS_CSR_FIFO_HALFFULL	0
#define PS2_GS_CSR_FIFO_EMPTY		1
#define PS2_GS_CSR_FIFO_ALMOSTFULL	2

/* DISPFB1/2 */
/** see ps2gs_{en,jp}.txt **/
typedef struct {
	uint64_t FBP:    9 __attribute__((packed));
	uint64_t FBW:    6 __attribute__((packed));
	uint64_t PSM:    5 __attribute__((packed));
	uint64_t pad20: 12 __attribute__((packed));
	uint64_t DBX:   11 __attribute__((packed));
	uint64_t DBY:   11 __attribute__((packed));
	uint64_t pad54: 10 __attribute__((packed));
} ps2_gssreg_dispfb __attribute__((packed));

/* DISPLAY1/2 */
/** see ps2gs_{en,jp}.txt **/
typedef struct {
	uint64_t DX:    12 __attribute__((packed));
	uint64_t DY:    11 __attribute__((packed));
	uint64_t MAGH:   4 __attribute__((packed));
	uint64_t MAGV:   2 __attribute__((packed));
	uint64_t pad29:  3 __attribute__((packed));
	uint64_t DW:    12 __attribute__((packed));
	uint64_t DH:    11 __attribute__((packed));
	uint64_t pad55:  9 __attribute__((packed));
} ps2_gssreg_display __attribute__((packed));

/* EXTBUF */
typedef struct {
	uint64_t EXBP:  14 __attribute__((packed));
	uint64_t EXBW:   6 __attribute__((packed));
	uint64_t FBIN:   2 __attribute__((packed));
	uint64_t WFFMD:  1 __attribute__((packed));
	uint64_t EMODA:  2 __attribute__((packed));
	uint64_t EMODC:  2 __attribute__((packed));
	uint64_t pad27:  5 __attribute__((packed));
	uint64_t WDX:   11 __attribute__((packed));
	uint64_t WDY:   11 __attribute__((packed));
	uint64_t pad54: 10 __attribute__((packed));
} ps2_gssreg_extbuf __attribute__((packed));
#define PS2_GS_EXTBUF_FBIN_OUT1		0
#define PS2_GS_EXTBUF_FBIN_OUT2		1
#define PS2_GS_EXTBUF_WFFMD_FIELD	0
#define PS2_GS_EXTBUF_WFFMD_FRAME	1
#define PS2_GS_EXTBUF_EMODA_THURU	0
#define PS2_GS_EXTBUF_EMODA_Y		1
#define PS2_GS_EXTBUF_EMODA_Y2		2
#define PS2_GS_EXTBUF_EMODA_ZERO	3
#define PS2_GS_EXTBUF_EMODC_THURU	0
#define PS2_GS_EXTBUF_EMODC_MONO	1
#define PS2_GS_EXTBUF_EMODC_YCbCr	2
#define PS2_GS_EXTBUF_EMODC_ALPHA	3

/* EXTDATA */
typedef struct {
	uint64_t SX:    12 __attribute__((packed));
	uint64_t SY:    11 __attribute__((packed));
	uint64_t SMPH:   4 __attribute__((packed));
	uint64_t SMPV:   2 __attribute__((packed));
	uint64_t pad29:  3 __attribute__((packed));
	uint64_t WW:    12 __attribute__((packed));
	uint64_t WH:    11 __attribute__((packed));
	uint64_t pad55:  9 __attribute__((packed));
} ps2_gssreg_extdata __attribute__((packed));

/* EXTWRITE */
typedef struct {
	uint64_t EXTWRITE:  1 __attribute__((packed));
	uint64_t pad01: 63 __attribute__((packed));
} ps2_gsreg_extwrite __attribute__((packed));
#define PS2_GS_EXTWRITE_STOP	0
#define PS2_GS_EXTWRITE_START	1

/* IMR */
/** see ps2event_{en,jp}.txt **/
typedef struct {
	uint64_t pad00:      8 __attribute__((packed));
	uint64_t SIGMSK:     1 __attribute__((packed)); /* ro */
	uint64_t FINISHMSK:  1 __attribute__((packed)); /* ro */
	uint64_t HSMSK:      1 __attribute__((packed)); /* ro */
	uint64_t VSMSK:      1 __attribute__((packed)); /* ro */
	uint64_t reserve12:  1 __attribute__((packed)); /* ro */
	uint64_t reserve13:  1 __attribute__((packed)); /* ro */
	uint64_t reserve14:  1 __attribute__((packed)); /* ro */
	uint64_t pad15:     49 __attribute__((packed));
} ps2_gsreg_imr __attribute__((packed));

/* PMODE */
/** see ps2gs_{en,jp}.txt **/
typedef struct {
	uint64_t EN1:        1 __attribute__((packed));
	uint64_t EN2:        1 __attribute__((packed));
	uint64_t CRTMD:      3 __attribute__((packed));
	uint64_t MMOD:       1 __attribute__((packed));
	uint64_t AMOD:       1 __attribute__((packed));
	uint64_t SLBG:       1 __attribute__((packed));
	uint64_t ALP:        8 __attribute__((packed));
	uint64_t reserve16: 17 __attribute__((packed));
	uint64_t pad33:     31 __attribute__((packed));
} ps2_gsreg_pmode __attribute__((packed));
#define PS2_GS_PMODE_EN_OFF		0
#define PS2_GS_PMODE_EN_ON		1
#define PS2_GS_PMODE_MMOD_PORT1		0
#define PS2_GS_PMODE_MMOD_ALP		1
#define PS2_GS_PMODE_AMOD_PORT1		0
#define PS2_GS_PMODE_AMOD_PORT2		1
#define PS2_GS_PMODE_SLBG_BLEND2	0
#define PS2_GS_PMODE_SLBG_BLENDBG	1

/* SIGLBLID */
typedef struct {
	uint64_t SIGID: 32 __attribute__((packed));
	uint64_t LBLID: 32 __attribute__((packed));
} ps2_gsreg_siglblid __attribute__((packed));

/* SMODE2 */
/** see ps2gs_{en,jp}.txt **/
typedef struct {
	uint64_t INT:    1 __attribute__((packed));
	uint64_t FFMD:   1 __attribute__((packed));
	uint64_t DPMS:   2 __attribute__((packed));
	uint64_t pad04: 60 __attribute__((packed));
} ps2_gsreg_smode2 __attribute__((packed));
#define PS2_GS_SMODE2_INT_NOINTERLACE	0
#define PS2_GS_SMODE2_INT_INTERLACE	1
#define PS2_GS_SMODE2_FFMD_FIELD	0
#define PS2_GS_SMODE2_FFMD_FRAME	1
#define PS2_GS_SMODE2_DPMS_ON		0
#define PS2_GS_SMODE2_DPMS_STANDBY	1
#define PS2_GS_SMODE2_DPMS_SUSPEND	2
#define PS2_GS_SMODE2_DPMS_OFF		3

#endif /* __PS2_GS_H */
