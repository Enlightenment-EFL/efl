#include "evas_common_private.h"
#include "evas_convert_yuv.h"

#ifdef BUILD_MMX
# include "evas_mmx.h"
#endif

#ifdef HAVE_ALTIVEC_H
# include <altivec.h>
#ifdef CONFIG_DARWIN
#define AVV(x...) (x)
#else
#define AVV(x...) {x}
#endif

#endif

static void _evas_yuv_init         (void);
// Broken atm - the sse and mmx get math.. wrong :(
//static void _evas_yv12_709torgb_sse(unsigned char **yuv, unsigned char *rgb, int w, int h);
static void _evas_yv12torgb_sse    (unsigned char **yuv, unsigned char *rgb, int w, int h);
// Broken atm - the sse and mmx get math.. wrong :(
//static void _evas_yv12_709torgb_mmx(unsigned char **yuv, unsigned char *rgb, int w, int h);
static void _evas_yv12torgb_mmx    (unsigned char **yuv, unsigned char *rgb, int w, int h);
static void _evas_yv12_709torgb_raster(unsigned char **yuv, unsigned char *rgb, int w, int h);
static void _evas_yv12torgb_raster (unsigned char **yuv, unsigned char *rgb, int w, int h);
static void _evas_yuy2torgb_raster (unsigned char **yuv, unsigned char *rgb, int w, int h);
static void _evas_nv12torgb_raster (unsigned char **yuv, unsigned char *rgb, int w, int h);
static void _evas_nv12tiledtorgb_raster(unsigned char **yuv, unsigned char *rgb, int w, int h);

#define CRV    104595
#define CBU    132251
#define CGU    25624
#define CGV    53280

#define YMUL   76283
#define OFF    32768
#define BITRES 16

#define CRV709 117504
#define CBU709 138607
#define CGU709  13959
#define CGV709  34996


/* calculation float resolution in bits */
/* ie RES = 6 is 10.6 fixed point */
/*    RES = 8 is 8.8 fixed point */
/*    RES = 4 is 12.4 fixed point */
/* NB: going above 6 will lead to overflow... :( */
#define RES    6

#define RZ(i)  (i >> (BITRES - RES))
#define FOUR(i) {i, i, i, i}

#ifdef BUILD_MMX
__attribute__ ((aligned (8))) const volatile unsigned short _const_crvcrv[4] = FOUR(RZ(CRV));
__attribute__ ((aligned (8))) const volatile unsigned short _const_cbucbu[4] = FOUR(RZ(CBU));
__attribute__ ((aligned (8))) const volatile unsigned short _const_cgucgu[4] = FOUR(RZ(CGU));
__attribute__ ((aligned (8))) const volatile unsigned short _const_cgvcgv[4] = FOUR(RZ(CGV));
__attribute__ ((aligned (8))) const volatile unsigned short _const_ymul  [4] = FOUR(RZ(YMUL));
__attribute__ ((aligned (8))) const volatile unsigned short _const_128   [4] = FOUR(128);
__attribute__ ((aligned (8))) const volatile unsigned short _const_32    [4] = FOUR(RZ(OFF));
__attribute__ ((aligned (8))) const volatile unsigned short _const_16    [4] = FOUR(16);
__attribute__ ((aligned (8))) const volatile unsigned short _const_ff    [4] = FOUR(-1);

__attribute__ ((aligned (8))) const volatile unsigned short _const_crvcrv709[4] = FOUR(RZ(CRV709));
__attribute__ ((aligned (8))) const volatile unsigned short _const_cbucbu709[4] = FOUR(RZ(CBU709));
__attribute__ ((aligned (8))) const volatile unsigned short _const_cgucgu709[4] = FOUR(RZ(CGU709));
__attribute__ ((aligned (8))) const volatile unsigned short _const_cgvcgv709[4] = FOUR(RZ(CGV709));

#define CONST_CRVCRV *_const_crvcrv
#define CONST_CBUCBU *_const_cbucbu
#define CONST_CGUCGU *_const_cgucgu
#define CONST_CGVCGV *_const_cgvcgv
#define CONST_YMUL   *_const_ymul
#define CONST_128    *_const_128
#define CONST_32     *_const_32
#define CONST_16     *_const_16
#define CONST_FF     *_const_ff

#define CONST_CRVCRV709 *_const_crvcrv709
#define CONST_CBUCBU709 *_const_cbucbu709
#define CONST_CGUCGU709 *_const_cgucgu709
#define CONST_CGVCGV709 *_const_cgvcgv709

/* for C non aligned cleanup */
const int _crv = RZ(CRV);   /* 1.596 */
const int _cbu = RZ(CBU);   /* 2.018 */
const int _cgu = RZ(CGU);   /* 0.391 */
const int _cgv = RZ(CGV);   /* 0.813 */

const int _crv709 = RZ(CRV709);   /* 1.793 */
const int _cbu709 = RZ(CBU709);   /* 2.115 */
const int _cgu709 = RZ(CGU709);   /* 0.213 */
const int _cgv709 = RZ(CGV709);   /* 0.534 */

#endif

/* shortcut speedup lookup-tables */
static short _v1164[256];
static short _v1596[256];
static short _v813[256];
static short _v391[256];
static short _v2018[256];

static short _v1793[256];
static short _v534[256];
static short _v213[256];
static short _v2115[256];

static unsigned char _clip_lut[1024];
#define LUT_CLIP(i) ((_clip_lut+384)[(i)])

#define CMP_CLIP(i) ((i&256)? (~(i>>10)) : i);

static int initted = 0;

void
evas_common_convert_yuv_422p_709_rgba(DATA8 **src, DATA8 *dst, int w, int h)
{
   if (!initted) _evas_yuv_init();
   initted = 1;
/* Broken atm - the sse and mmx get math.. wrong :(
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX2))
     _evas_yv12_709torgb_sse(src, dst, w, h);
   else if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     _evas_yv12_709torgb_mmx(src, dst, w, h);
   else
 */
     _evas_yv12_709torgb_raster(src, dst, w, h);
}


void
evas_common_convert_yuv_422p_601_rgba(DATA8 **src, DATA8 *dst, int w, int h)
{
   if (!initted) _evas_yuv_init();
   initted = 1;
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX2))
     _evas_yv12torgb_sse(src, dst, w, h);
   else if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     _evas_yv12torgb_mmx(src, dst, w, h);
   else
     _evas_yv12torgb_raster(src, dst, w, h);
}

/* Thanks to Diz for this code. i've munged it a little and turned it into */
/* inline macros. I tried beating it with a different algorithm using MMX */
/* but failed. So here we are. This is the fastest YUV->RGB i know of for */
/* x86. It has an issue that it doesn't convert colours accurately so the */
/* image looks a little "yellowy". This is a result of only 10.6 fixed point */
/* resolution as opposed to 16.16 in the C code. This could be fixed by */
/* processing half the number of pixels per cycle and going up to 32bits */
/* per element during compute, but it would all but negate the speedup */
/* from mmx I think :( It might be possible to use SSE and SSE2 here, but */
/* I haven't tried yet. Let's see. */

/* NB: XviD has almost the same code in it's assembly YV12->RGB code. same */
/* algorithm, same constants, same all over actually, except it actually */
/* does a few extra memory accesses that this one doesn't, so in theory */
/* this code should be faster. In the end it's all just an mmx version of */
/* the reference implimentation done with fixed point math */

/*
static void
_evas_yv12_709torgb_sse(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
#ifdef BUILD_MMX
   int xx, yy;
   register unsigned char *yp1, *up, *vp;
   unsigned char *dp1;

   dp1 = rgb;

   for (yy = 0; yy < h; yy++)
     {
	yp1 = yuv[yy];
	up = yuv[h + (yy / 2)];
	vp = yuv[h + (h / 2) + (yy / 2)];
	for (xx = 0; xx < (w - 807); xx += 8)
	  {
	     movd_m2r(*up, mm3);
	     movd_m2r(*vp, mm2);
	     movq_m2r(*yp1, mm0);

	     pxor_r2r(mm7, mm7);
	     punpcklbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm7, mm3);

	     movq_r2r(mm0, mm1);
	     psrlw_i2r(8, mm0);
	     psllw_i2r(8, mm1);
	     psrlw_i2r(8, mm1);

	     movq_m2r(CONST_16, mm4);
	     psubsw_r2r(mm4, mm0);
	     psubsw_r2r(mm4, mm1);

	     movq_m2r(CONST_128, mm5);
	     psubsw_r2r(mm5, mm2);
	     psubsw_r2r(mm5, mm3);

	     movq_m2r(CONST_YMUL, mm4);
	     pmullw_r2r(mm4, mm0);
	     pmullw_r2r(mm4, mm1);

	     movq_m2r(CONST_CRVCRV709, mm7);
	     pmullw_r2r(mm3, mm7);
	     movq_m2r(CONST_CBUCBU709, mm6);
	     pmullw_r2r(mm2, mm6);
	     movq_m2r(CONST_CGUCGU709, mm5);
	     pmullw_r2r(mm2, mm5);
	     movq_m2r(CONST_CGVCGV709, mm4);
	     pmullw_r2r(mm3, mm4);

	     movq_r2r(mm0, mm2);
	     paddsw_r2r(mm7, mm2);
	     paddsw_r2r(mm1, mm7);

	     psraw_i2r(RES, mm2);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm2);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm2, mm3);
	     punpckhbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm3, mm7);
	     por_r2r(mm7, mm2);

	     movq_r2r(mm0, mm3);
	     psubsw_r2r(mm5, mm3);
	     psubsw_r2r(mm4, mm3);
	     paddsw_m2r(CONST_32, mm3);

	     movq_r2r(mm1, mm7);
	     psubsw_r2r(mm5, mm7);
	     psubsw_r2r(mm4, mm7);
	     paddsw_m2r(CONST_32, mm7);

	     psraw_i2r(RES, mm3);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm3);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm3, mm4);
	     punpckhbw_r2r(mm7, mm3);
	     punpcklbw_r2r(mm4, mm7);
	     por_r2r(mm7, mm3);

	     movq_m2r(CONST_32, mm4);
	     paddsw_r2r(mm6, mm0);
	     paddsw_r2r(mm6, mm1);
	     paddsw_r2r(mm4, mm0);
	     paddsw_r2r(mm4, mm1);
	     psraw_i2r(RES, mm0);
	     psraw_i2r(RES, mm1);
	     packuswb_r2r(mm1, mm0);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm0, mm5);
	     punpckhbw_r2r(mm7, mm0);
	     punpcklbw_r2r(mm5, mm7);
	     por_r2r(mm7, mm0);

	     movq_m2r(CONST_FF, mm1);
	     movq_r2r(mm0, mm5);
	     movq_r2r(mm3, mm6);
	     movq_r2r(mm2, mm7);
	     punpckhbw_r2r(mm3, mm2);
	     punpcklbw_r2r(mm6, mm7);
	     punpckhbw_r2r(mm1, mm0);
	     punpcklbw_r2r(mm1, mm5);

	     movq_r2r(mm7, mm1);
	     punpckhwd_r2r(mm5, mm7);
	     punpcklwd_r2r(mm5, mm1);

	     movq_r2r(mm2, mm4);
	     punpckhwd_r2r(mm0, mm2);
	     punpcklwd_r2r(mm0, mm4);

	     movntq_r2m(mm1, *(dp1));
	     movntq_r2m(mm7, *(dp1 + 8));
	     movntq_r2m(mm4, *(dp1 + 16));
	     movntq_r2m(mm2, *(dp1 + 24));

	     yp1 += 8;
	     up += 4;
	     vp += 4;
	     dp1 += 8 * 4;
	  }

	if (xx < w)
	  {
	     int y, u, v, r, g, b;

	     for (; xx < w; xx += 2)
	       {
		  u = (*up++) - 128;
		  v = (*vp++) - 128;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv709 * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu709 * u) - (_cgv709 * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu709 * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv709 * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu709 * u) - (_cgv709 * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu709 * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;
	       }
	  }
     }
   emms();
#else
   _evas_yv12_709torgb_mmx(yuv, rgb, w, h);
#endif
}
*/

static void
_evas_yv12torgb_sse(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
#ifdef BUILD_MMX
   int xx, yy;
   register unsigned char *yp1, *up, *vp;
   unsigned char *dp1;

   /* destination pointers */
   dp1 = rgb;

   for (yy = 0; yy < h; yy++)
     {
	/* plane pointers */
	yp1 = yuv[yy];
	up = yuv[h + (yy / 2)];
	vp = yuv[h + (h / 2) + (yy / 2)];
	for (xx = 0; xx < (w - 7); xx += 8)
	  {
	     movd_m2r(*up, mm3);
	     movd_m2r(*vp, mm2);
	     movq_m2r(*yp1, mm0);

	     pxor_r2r(mm7, mm7);
	     punpcklbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm7, mm3);

	     movq_r2r(mm0, mm1);
	     psrlw_i2r(8, mm0);
	     psllw_i2r(8, mm1);
	     psrlw_i2r(8, mm1);

	     movq_m2r(CONST_16, mm4);
	     psubsw_r2r(mm4, mm0);
	     psubsw_r2r(mm4, mm1);

	     movq_m2r(CONST_128, mm5);
	     psubsw_r2r(mm5, mm2);
	     psubsw_r2r(mm5, mm3);

	     movq_m2r(CONST_YMUL, mm4);
	     pmullw_r2r(mm4, mm0);
	     pmullw_r2r(mm4, mm1);

	     movq_m2r(CONST_CRVCRV, mm7);
	     pmullw_r2r(mm3, mm7);
	     movq_m2r(CONST_CBUCBU, mm6);
	     pmullw_r2r(mm2, mm6);
	     movq_m2r(CONST_CGUCGU, mm5);
	     pmullw_r2r(mm2, mm5);
	     movq_m2r(CONST_CGVCGV, mm4);
	     pmullw_r2r(mm3, mm4);

	     movq_r2r(mm0, mm2);
	     paddsw_r2r(mm7, mm2);
	     paddsw_r2r(mm1, mm7);

	     psraw_i2r(RES, mm2);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm2);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm2, mm3);
	     punpckhbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm3, mm7);
	     por_r2r(mm7, mm2);

	     movq_r2r(mm0, mm3);
	     psubsw_r2r(mm5, mm3);
	     psubsw_r2r(mm4, mm3);
	     paddsw_m2r(CONST_32, mm3);

	     movq_r2r(mm1, mm7);
	     psubsw_r2r(mm5, mm7);
	     psubsw_r2r(mm4, mm7);
	     paddsw_m2r(CONST_32, mm7);

	     psraw_i2r(RES, mm3);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm3);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm3, mm4);
	     punpckhbw_r2r(mm7, mm3);
	     punpcklbw_r2r(mm4, mm7);
	     por_r2r(mm7, mm3);

	     movq_m2r(CONST_32, mm4);
	     paddsw_r2r(mm6, mm0);
	     paddsw_r2r(mm6, mm1);
	     paddsw_r2r(mm4, mm0);
	     paddsw_r2r(mm4, mm1);
	     psraw_i2r(RES, mm0);
	     psraw_i2r(RES, mm1);
	     packuswb_r2r(mm1, mm0);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm0, mm5);
	     punpckhbw_r2r(mm7, mm0);
	     punpcklbw_r2r(mm5, mm7);
	     por_r2r(mm7, mm0);

	     movq_m2r(CONST_FF, mm1);
	     movq_r2r(mm0, mm5);
	     movq_r2r(mm3, mm6);
	     movq_r2r(mm2, mm7);
	     punpckhbw_r2r(mm3, mm2);
	     punpcklbw_r2r(mm6, mm7);
	     punpckhbw_r2r(mm1, mm0);
	     punpcklbw_r2r(mm1, mm5);

	     movq_r2r(mm7, mm1);
	     punpckhwd_r2r(mm5, mm7);
	     punpcklwd_r2r(mm5, mm1);

	     movq_r2r(mm2, mm4);
	     punpckhwd_r2r(mm0, mm2);
	     punpcklwd_r2r(mm0, mm4);

	     movntq_r2m(mm1, *(dp1));
	     movntq_r2m(mm7, *(dp1 + 8));
	     movntq_r2m(mm4, *(dp1 + 16));
	     movntq_r2m(mm2, *(dp1 + 24));

	     yp1 += 8;
	     up += 4;
	     vp += 4;
	     dp1 += 8 * 4;
	  }
	/* cleanup pixles that arent a multiple of 8 pixels wide */
	if (xx < w)
	  {
	     int y, u, v, r, g, b;

	     for (; xx < w; xx += 2)
	       {
		  u = (*up++) - 128;
		  v = (*vp++) - 128;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu * u) - (_cgv * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu * u) - (_cgv * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;
	       }
	  }
     }
   emms();
#else
   _evas_yv12torgb_mmx(yuv, rgb, w, h);
#endif
}

/*
static void
_evas_yv12_709torgb_mmx(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
#ifdef BUILD_MMX
   int xx, yy;
   register unsigned char *yp1, *up, *vp;
   unsigned char *dp1;

   dp1 = rgb;

   for (yy = 0; yy < h; yy++)
     {
	yp1 = yuv[yy];
	up = yuv[h + (yy / 2)];
	vp = yuv[h + (h / 2) + (yy / 2)];
	for (xx = 0; xx < (w - 7); xx += 8)
	  {
	     movd_m2r(*up, mm3);
	     movd_m2r(*vp, mm2);
	     movq_m2r(*yp1, mm0);

	     pxor_r2r(mm7, mm7);
	     punpcklbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm7, mm3);

	     movq_r2r(mm0, mm1);
	     psrlw_i2r(8, mm0);
	     psllw_i2r(8, mm1);
	     psrlw_i2r(8, mm1);

	     movq_m2r(CONST_16, mm4);
	     psubsw_r2r(mm4, mm0);
	     psubsw_r2r(mm4, mm1);

	     movq_m2r(CONST_128, mm5);
	     psubsw_r2r(mm5, mm2);
	     psubsw_r2r(mm5, mm3);

	     movq_m2r(CONST_YMUL, mm4);
	     pmullw_r2r(mm4, mm0);
	     pmullw_r2r(mm4, mm1);

	     movq_m2r(CONST_CRVCRV709, mm7);
	     pmullw_r2r(mm3, mm7);
	     movq_m2r(CONST_CBUCBU709, mm6);
	     pmullw_r2r(mm2, mm6);
	     movq_m2r(CONST_CGUCGU709, mm5);
	     pmullw_r2r(mm2, mm5);
	     movq_m2r(CONST_CGVCGV709, mm4);
	     pmullw_r2r(mm3, mm4);

	     movq_r2r(mm0, mm2);
	     paddsw_r2r(mm7, mm2);
	     paddsw_r2r(mm1, mm7);

	     psraw_i2r(RES, mm2);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm2);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm2, mm3);
	     punpckhbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm3, mm7);
	     por_r2r(mm7, mm2);

	     movq_r2r(mm0, mm3);
	     psubsw_r2r(mm5, mm3);
	     psubsw_r2r(mm4, mm3);
	     paddsw_m2r(CONST_32, mm3);

	     movq_r2r(mm1, mm7);
	     psubsw_r2r(mm5, mm7);
	     psubsw_r2r(mm4, mm7);
	     paddsw_m2r(CONST_32, mm7);

	     psraw_i2r(RES, mm3);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm3);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm3, mm4);
	     punpckhbw_r2r(mm7, mm3);
	     punpcklbw_r2r(mm4, mm7);
	     por_r2r(mm7, mm3);

	     movq_m2r(CONST_32, mm4);
	     paddsw_r2r(mm6, mm0);
	     paddsw_r2r(mm6, mm1);
	     paddsw_r2r(mm4, mm0);
	     paddsw_r2r(mm4, mm1);
	     psraw_i2r(RES, mm0);
	     psraw_i2r(RES, mm1);
	     packuswb_r2r(mm1, mm0);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm0, mm5);
	     punpckhbw_r2r(mm7, mm0);
	     punpcklbw_r2r(mm5, mm7);
	     por_r2r(mm7, mm0);

	     movq_m2r(CONST_FF, mm1);
	     movq_r2r(mm0, mm5);
	     movq_r2r(mm3, mm6);
	     movq_r2r(mm2, mm7);
	     punpckhbw_r2r(mm3, mm2);
	     punpcklbw_r2r(mm6, mm7);
	     punpckhbw_r2r(mm1, mm0);
	     punpcklbw_r2r(mm1, mm5);

	     movq_r2r(mm7, mm1);
	     punpckhwd_r2r(mm5, mm7);
	     punpcklwd_r2r(mm5, mm1);

	     movq_r2r(mm2, mm4);
	     punpckhwd_r2r(mm0, mm2);
	     punpcklwd_r2r(mm0, mm4);

	     movq_r2m(mm1, *(dp1));
	     movq_r2m(mm7, *(dp1 + 8));
	     movq_r2m(mm4, *(dp1 + 16));
	     movq_r2m(mm2, *(dp1 + 24));

	     yp1 += 8;
	     up += 4;
	     vp += 4;
	     dp1 += 8 * 4;
	  }
	if (xx < w)
	  {
	     int y, u, v, r, g, b;

	     for (; xx < w; xx += 2)
	       {
		  u = (*up++) - 128;
		  v = (*vp++) - 128;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv709 * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu709 * u) - (_cgv709 * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu709 * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv709 * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu709 * u) - (_cgv709 * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu709 * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;
	       }
	  }
     }
   emms();
#else
   _evas_yv12torgb_raster(yuv, rgb, w, h);
#endif
}
*/

static void
_evas_yv12torgb_mmx(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
#ifdef BUILD_MMX
   int xx, yy;
   register unsigned char *yp1, *up, *vp;
   unsigned char *dp1;

   /* destination pointers */
   dp1 = rgb;

   for (yy = 0; yy < h; yy++)
     {
	/* plane pointers */
	yp1 = yuv[yy];
	up = yuv[h + (yy / 2)];
	vp = yuv[h + (h / 2) + (yy / 2)];
	for (xx = 0; xx < (w - 7); xx += 8)
	  {
	     movd_m2r(*up, mm3);
	     movd_m2r(*vp, mm2);
	     movq_m2r(*yp1, mm0);

	     pxor_r2r(mm7, mm7);
	     punpcklbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm7, mm3);

	     movq_r2r(mm0, mm1);
	     psrlw_i2r(8, mm0);
	     psllw_i2r(8, mm1);
	     psrlw_i2r(8, mm1);

	     movq_m2r(CONST_16, mm4);
	     psubsw_r2r(mm4, mm0);
	     psubsw_r2r(mm4, mm1);

	     movq_m2r(CONST_128, mm5);
	     psubsw_r2r(mm5, mm2);
	     psubsw_r2r(mm5, mm3);

	     movq_m2r(CONST_YMUL, mm4);
	     pmullw_r2r(mm4, mm0);
	     pmullw_r2r(mm4, mm1);

	     movq_m2r(CONST_CRVCRV, mm7);
	     pmullw_r2r(mm3, mm7);
	     movq_m2r(CONST_CBUCBU, mm6);
	     pmullw_r2r(mm2, mm6);
	     movq_m2r(CONST_CGUCGU, mm5);
	     pmullw_r2r(mm2, mm5);
	     movq_m2r(CONST_CGVCGV, mm4);
	     pmullw_r2r(mm3, mm4);

	     movq_r2r(mm0, mm2);
	     paddsw_r2r(mm7, mm2);
	     paddsw_r2r(mm1, mm7);

	     psraw_i2r(RES, mm2);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm2);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm2, mm3);
	     punpckhbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm3, mm7);
	     por_r2r(mm7, mm2);

	     movq_r2r(mm0, mm3);
	     psubsw_r2r(mm5, mm3);
	     psubsw_r2r(mm4, mm3);
	     paddsw_m2r(CONST_32, mm3);

	     movq_r2r(mm1, mm7);
	     psubsw_r2r(mm5, mm7);
	     psubsw_r2r(mm4, mm7);
	     paddsw_m2r(CONST_32, mm7);

	     psraw_i2r(RES, mm3);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm3);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm3, mm4);
	     punpckhbw_r2r(mm7, mm3);
	     punpcklbw_r2r(mm4, mm7);
	     por_r2r(mm7, mm3);

	     movq_m2r(CONST_32, mm4);
	     paddsw_r2r(mm6, mm0);
	     paddsw_r2r(mm6, mm1);
	     paddsw_r2r(mm4, mm0);
	     paddsw_r2r(mm4, mm1);
	     psraw_i2r(RES, mm0);
	     psraw_i2r(RES, mm1);
	     packuswb_r2r(mm1, mm0);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm0, mm5);
	     punpckhbw_r2r(mm7, mm0);
	     punpcklbw_r2r(mm5, mm7);
	     por_r2r(mm7, mm0);

	     movq_m2r(CONST_FF, mm1);
	     movq_r2r(mm0, mm5);
	     movq_r2r(mm3, mm6);
	     movq_r2r(mm2, mm7);
	     punpckhbw_r2r(mm3, mm2);
	     punpcklbw_r2r(mm6, mm7);
	     punpckhbw_r2r(mm1, mm0);
	     punpcklbw_r2r(mm1, mm5);

	     movq_r2r(mm7, mm1);
	     punpckhwd_r2r(mm5, mm7);
	     punpcklwd_r2r(mm5, mm1);

	     movq_r2r(mm2, mm4);
	     punpckhwd_r2r(mm0, mm2);
	     punpcklwd_r2r(mm0, mm4);

	     movq_r2m(mm1, *(dp1));
	     movq_r2m(mm7, *(dp1 + 8));
	     movq_r2m(mm4, *(dp1 + 16));
	     movq_r2m(mm2, *(dp1 + 24));

	     yp1 += 8;
	     up += 4;
	     vp += 4;
	     dp1 += 8 * 4;
	  }
	/* cleanup pixles that arent a multiple of 8 pixels wide */
	if (xx < w)
	  {
	     int y, u, v, r, g, b;

	     for (; xx < w; xx += 2)
	       {
		  u = (*up++) - 128;
		  v = (*vp++) - 128;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu * u) - (_cgv * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu * u) - (_cgv * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;
	       }
	  }
     }
   emms();
#else
   _evas_yv12torgb_raster(yuv, rgb, w, h);
#endif
}

static void
_evas_yuv_init(void)
{
   int i;

   for (i = 0; i < 256; i++)
     {
	_v1164[i] = (int)(((float)(i - 16 )) * 1.164);

	_v1596[i] = (int)(((float)(i - 128)) * 1.596);
	_v813[i]  = (int)(((float)(i - 128)) * 0.813);

	_v391[i]  = (int)(((float)(i - 128)) * 0.391);
	_v2018[i] = (int)(((float)(i - 128)) * 2.018);

//////////////////////////////////////////////////////////////////////////
        _v1793[i] = (int)(((float)(i - 128)) * 1.793);
        _v534[i]  = (int)(((float)(i - 128)) * 0.534);
        _v213[i]  = (int)(((float)(i - 128)) * 0.213);
        _v2115[i] = (int)(((float)(i - 128)) * 2.115);
     }

   for (i = -384; i < 640; i++)
     {
	_clip_lut[i+384] = i < 0 ? 0 : (i > 255) ? 255 : i;
     }
}

static void
_evas_yv12_709torgb_raster(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
   int xx, yy;
   int y, u, v;
   unsigned char *yp1, *yp2, *up, *vp;
   unsigned char *dp1, *dp2;

   /* destination pointers */
   dp1 = rgb;
   dp2 = rgb + (w * 4);

   for (yy = 0; yy < h; yy += 2)
     {
	/* plane pointers */
	yp1 = yuv[yy];
	yp2 = yuv[yy + 1];
	up = yuv[h + (yy / 2)];
	vp = yuv[h + (h / 2) + (yy / 2)];
	for (xx = 0; xx < w; xx += 2)
	  {
	     int vmu;

	     /* collect u & v for 2x2 pixel block */
	     u = *up++;
	     v = *vp++;

	     /* save lookups */
	     vmu = _v534[v] + _v213[u];
	     u = _v2115[u];
	     v = _v1793[v];

             /* do the top 2 pixels of the 2x2 block which shared u & v */
	     /* yuv to rgb */
	     y = _v1164[*yp1++];
	     *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp1 += 4;

	     /* yuv to rgb */
	     y = _v1164[*yp1++];
	     *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp1 += 4;

	     /* do the bottom 2 pixels */
	     /* yuv to rgb */
	     y = _v1164[*yp2++];
	     *((DATA32 *) dp2) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp2 += 4;

	     /* yuv to rgb */
	     y = _v1164[*yp2++];
	     *((DATA32 *) dp2) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp2 += 4;
	  }
	/* jump down one line since we are doing 2 at once */
	dp1 += (w * 4);
	dp2 += (w * 4);
     }
}

static void
_evas_yv12torgb_raster(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
   int xx, yy;
   int y, u, v;
   unsigned char *yp1, *yp2, *up, *vp;
   unsigned char *dp1, *dp2;

   /* destination pointers */
   dp1 = rgb;
   dp2 = rgb + (w * 4);

   for (yy = 0; yy < h; yy += 2)
     {
	/* plane pointers */
	yp1 = yuv[yy];
	yp2 = yuv[yy + 1];
	up = yuv[h + (yy / 2)];
	vp = yuv[h + (h / 2) + (yy / 2)];
	for (xx = 0; xx < w; xx += 2)
	  {
	     int vmu;

	     /* collect u & v for 2x2 pixel block */
	     u = *up++;
	     v = *vp++;

	     /* save lookups */
	     vmu = _v813[v] + _v391[u];
	     u = _v2018[u];
	     v = _v1596[v];

             /* do the top 2 pixels of the 2x2 block which shared u & v */
	     /* yuv to rgb */
	     y = _v1164[*yp1++];
	     *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp1 += 4;

	     /* yuv to rgb */
	     y = _v1164[*yp1++];
	     *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp1 += 4;

	     /* do the bottom 2 pixels */
	     /* yuv to rgb */
	     y = _v1164[*yp2++];
	     *((DATA32 *) dp2) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp2 += 4;

	     /* yuv to rgb */
	     y = _v1164[*yp2++];
	     *((DATA32 *) dp2) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp2 += 4;
	  }
	/* jump down one line since we are doing 2 at once */
	dp1 += (w * 4);
	dp2 += (w * 4);
     }
}

void
evas_common_convert_yuv_422_601_rgba(DATA8 **src, DATA8 *dst, int w, int h)
{
   if (!initted) _evas_yuv_init();
   initted = 1;
   _evas_yuy2torgb_raster(src, dst, w, h);
}

void
evas_common_convert_yuv_420_601_rgba(DATA8 **src, DATA8 *dst, int w, int h)
{
   if (!initted) _evas_yuv_init();
   initted = 1;
   _evas_nv12torgb_raster(src, dst, w, h);
}

void
evas_common_convert_yuv_420T_601_rgba(DATA8 **src, DATA8 *dst, int w, int h)
{
   if (initted) _evas_yuv_init();
   initted = 1;
   _evas_nv12tiledtorgb_raster(src, dst, w, h);
}

static void
_evas_yuy2torgb_raster(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
   int xx, yy;
   int y, u, v;
   unsigned char *yp1, *yp2, *up, *vp;
   unsigned char *dp1;

   dp1 = rgb;

   /* destination pointers */
   for (yy = 0; yy < h; yy++)
     {
        /* plane pointers */
        unsigned char *line;

        line = yuv[yy];
        yp1 = line + 0;
        up = line + 1;
        yp2 = line + 2;
        vp = line + 3;

        for (xx = 0; xx < w; xx += 2)
          {
             int vmu;

             /* collect u & v for 2 pixels block */
             u = *up;
             v = *vp;

             /* save lookups */
             vmu = _v813[v] + _v391[u];
             u = _v2018[u];
             v = _v1596[v];

             /* do the top 2 pixels of the 2x2 block which shared u & v */
	     /* yuv to rgb */
	     y = _v1164[*yp1];
	     *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp1 += 4;

	     /* yuv to rgb */
	     y = _v1164[*yp2];
	     *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

             dp1 += 4;

	     yp1 += 4; yp2 += 4; up += 4; vp += 4;
	  }
     }
}

static inline void
_evas_yuv2rgb_420_raster(unsigned char *yp1, unsigned char *yp2, unsigned char *up, unsigned char *vp,
                         unsigned char *dp1, unsigned char *dp2)
{
   int y, u, v;
   int vmu;
   int rgb;

   /* collect u & v for 4 pixels block */
   u = *up;
   v = *vp;

   /* save lookups */
#ifdef MEM_BP
   vmu = _v813[v] + _v391[u];
   u = _v2018[u];
   v = _v1596[v];
#else
   u -= 128;
   v -= 128;
   vmu = v * CGV + u * CGU;
   u = u * CBU;
   v = v * CRV;
#endif

   /* do the top 2 pixels of the 2x2 block which shared u & v */
   /* yuv to rgb */
#ifdef MEM_BP
   y = _v1164[*yp1];
   rgb = RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));
#else
   y = (*yp1 - 16 ) * YMUL;
   rgb = RGB_JOIN(LUT_CLIP(((y + v) >> 16)),
                  LUT_CLIP(((y - vmu + OFF) >> 16)),
                  LUT_CLIP(((y + u + OFF) >> 16)));
#endif
   *((DATA32 *) dp1) = 0xff000000 + rgb;

   dp1 += 4; yp1++;

   /* yuv to rgb */
#ifdef MEM_BP
   y = _v1164[*yp1];
   rgb = RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));
#else
   y = (*yp1 - 16 ) * YMUL;
   rgb = RGB_JOIN(LUT_CLIP(((y + v) >> 16)),
                  LUT_CLIP(((y - vmu + OFF) >> 16)),
                  LUT_CLIP(((y + u + OFF) >> 16)));
#endif
   *((DATA32 *) dp1) = 0xff000000 + rgb;

   /* do the bottom 2 pixels of the 2x2 block which shared u & v */
   /* yuv to rgb */
#ifdef MEM_BP
   y = _v1164[*yp2];
   rgb = RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));
#else
   y = (*yp2 - 16 ) * YMUL;
   rgb = RGB_JOIN(LUT_CLIP(((y + v) >> 16)),
                  LUT_CLIP(((y - vmu + OFF) >> 16)),
                  LUT_CLIP(((y + u + OFF) >> 16)));
#endif
   *((DATA32 *) dp2) = 0xff000000 + rgb;

   dp2 += 4; yp2++;

   /* yuv to rgb */
#ifdef MEM_BP
   y = _v1164[*yp2];
   rgb = RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));
#else
   y = (*yp2 - 16 ) * YMUL;
   rgb = RGB_JOIN(LUT_CLIP(((y + v) >> 16)),
                  LUT_CLIP(((y - vmu + OFF) >> 16)),
                  LUT_CLIP(((y + u + OFF) >> 16)));
#endif
   *((DATA32 *) dp2) = 0xff000000 + rgb;
}

static void
_evas_nv12tiledtorgb_raster(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
#define HANDLE_MACROBLOCK(YP1, YP2, UP, VP, DP1, DP2)                   \
   {                                                                    \
     int i;                                                             \
     int j;                                                             \
                                                                        \
     for (i = 0; i < 32; i += 2)                                        \
       {                                                                \
          for (j = 0; j < 64; j += 2)                                   \
            {                                                           \
               _evas_yuv2rgb_420_raster(YP1, YP2, UP, VP, DP1, DP2);    \
                                                                        \
               /* the previous call just rendered 2 pixels per lines */ \
               DP1 += 8; DP2 += 8;                                      \
                                                                        \
               /* and took for that 2 lines with 2 Y, 1 U and 1 V. Don't forget U & V are in the same plane */ \
               YP1 += 2; YP2 += 2; UP += 2; VP += 2;                    \
            }                                                           \
                                                                        \
          DP1 += sizeof (int) * ((w << 1) - 64);			\
          DP2 += sizeof (int) * ((w << 1) - 64);			\
          YP1 += 64;                                                    \
          YP2 += 64;                                                    \
       }                                                                \
   }

   /* One macro block is 32 lines of Y and 16 lines of UV */
   const int offset_value[2] = { 0, 64 * 16 };
   int mb_x, mb_y, mb_w, mb_h;
   int base_h;
   int uv_x, uv_step;
   int stride;

   /* Idea iterate over each macroblock and convert each of them using _evas_nv12torgb_raster */

   /* The layout of the Y macroblock order in RGB non tiled space : */
   /* --------------------------------------------------- */
   /* | 0  | 1  | 6  | 7  | 8  | 9  | 14 | 15 | 16 | 17 | */
   /* --------------------------------------------------- */
   /* | 2  | 3  | 4  | 5  | 10 | 11 | 12 | 13 | 18 | 19 | */
   /* --------------------------------------------------- */
   /* | 20 | 21 | 26 | 27 | 28 | 29 | 34 | 35 | 36 | 37 | */
   /* --------------------------------------------------- */
   /* | 22 | 23 | 24 | 25 | 30 | 31 | 32 | 33 | 38 | 39 | */
   /* --------------------------------------------------- */
   /* | 40 | 41 | 42 | 43 | 44 | 45 | 46 | 47 | 48 | 49 | */
   /* --------------------------------------------------- */
   /* The layout of the UV macroblock order in the same RGB non tiled space : */
   /* --------------------------------------------------- */
   /* |    |    |    |    |    |    |    |    |    |    | */
   /* - 0  - 1  - 6  - 7  - 8  - 9  - 14 - 15 - 16 - 17 - */
   /* |    |    |    |    |    |    |    |    |    |    | */
   /* --------------------------------------------------- */
   /* |    |    |    |    |    |    |    |    |    |    | */
   /* - 2  - 3  - 4  - 5  - 10 - 11 - 12 - 13 - 18 - 19 - */
   /* |    |    |    |    |    |    |    |    |    |    | */
   /* --------------------------------------------------- */
   /* |    |    |    |    |    |    |    |    |    |    | */
   /* - 20 - 21 - 22 - 22 - 23 - 24 - 25 - 26 - 27 - 28 - */

   /* the number of macroblock should be a multiple of 64x32 */
   mb_w = w / 64;
   mb_h = h / 32;

   base_h = (mb_h >> 1) + (mb_h & 0x1);
   stride = w * sizeof (int);

   uv_x = 0;

   /* In this format we linearize macroblock on two line to form a Z and it's invert */
   for (mb_y = 0; mb_y < (mb_h >> 1); mb_y++)
     {
        int step = 2;
        int offset = 0;
        int x = 0;
	int rmb_x = 0;
	int ry[2];

	ry[0] = mb_y * 2 * 32 * stride;
	ry[1] = ry[0] + 32 * stride;

	uv_step = (mb_y & 0x1) == 0 ? 4 : 0;
	uv_x = (mb_y & 0x1) == 0 ? 0 : 2 * 64 * 32;

	for (mb_x = 0; mb_x < mb_w * 2; mb_x++, rmb_x += 64 * 32)
	  {
	    unsigned char *yp1, *yp2, *up, *vp;
	    unsigned char *dp1, *dp2;

	    dp1 = rgb + x + ry[offset];
	    dp2 = dp1 + stride;

	    yp1 = yuv[mb_y] + rmb_x;
	    yp2 = yp1 + 64;

	    /* UV plane is two time less bigger in pixel count, but it old two bytes each times */
	    up = yuv[(mb_y >> 1) + base_h] + uv_x + offset_value[offset];
	    vp = up + 1;

	    HANDLE_MACROBLOCK(yp1, yp2, up, vp, dp1, dp2);

	    step++;
	    if ((step & 0x3) == 0)
	      {
		offset = 1 - offset;
		x -= 64 * sizeof (int);
		uv_x -= 64 * 32;
	      }
	    else
	      {
		x += 64 * sizeof (int);
		uv_x += 64 * 32;
	      }

	    uv_step++;
	    if (uv_step == 8)
	      {
		uv_step = 0;
		uv_x += 4 * 64 * 32;
	      }
	  }
     }

   if (mb_h & 0x1)
     {
        int x = 0;
	int ry;

	ry = mb_y << 1;

	uv_step = 0;
	uv_x = 0;

        for (mb_x = 0; mb_x < mb_w; mb_x++, x++, uv_x++)
          {
             unsigned char *yp1, *yp2, *up, *vp;
             unsigned char *dp1, *dp2;

             dp1 = rgb + (x * 64 + (ry * 32 * w)) * sizeof (int);
             dp2 = dp1 + sizeof (int) * w;

             yp1 = yuv[mb_y] + mb_x * 64 * 32;
             yp2 = yp1 + 64;

             up = yuv[mb_y / 2 + base_h] + uv_x * 64 * 32;
             vp = up + 1;

             HANDLE_MACROBLOCK(yp1, yp2, up, vp, dp1, dp2);
          }
     }
}

static void
_evas_nv12torgb_raster(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
   int xx, yy;
   unsigned char *yp1, *yp2, *up, *vp;
   unsigned char *dp1;
   unsigned char *dp2;
   int stride = sizeof(DATA32) * w;

   dp1 = rgb;
   dp2 = dp1 + stride;

   for (yy = 0; yy < h; yy++)
     {
        yp1 = yuv[yy++];
        yp2 = yuv[yy];

        up = yuv[h + (yy >> 1)];
        vp = up + 1;

        for (xx = 0; xx < w; xx += 2)
          {
             _evas_yuv2rgb_420_raster(yp1, yp2, up, vp, dp1, dp2);

             /* the previous call just rendered 2 pixels per lines */
             dp1 += 8; dp2 += 8;

             /* and took for that 2 lines with 2 Y, 1 U and 1 V. Don't forget U & V are in the same plane */
             yp1 += 2; yp2 += 2; up += 2; vp += 2;
          }

        /* jump one line */
        dp1 += stride;
        dp2 += stride;
     }
}

