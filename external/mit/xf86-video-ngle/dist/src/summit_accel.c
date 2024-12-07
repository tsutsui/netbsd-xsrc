/*
 * hardware acceleration for Visualize FX 4
 *
 * Copyright (C) 2024 Michael Lorenz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * MICHAEL LORENZ BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* $NetBSD: summit_accel.c,v 1.1 2024/12/07 10:48:38 macallan Exp $ */

#include <sys/types.h>
#include <dev/ic/summitreg.h>


#include "ngle.h"

//#define DEBUG

#ifdef DEBUG
#define ENTER xf86Msg(X_ERROR, "%s\n", __func__)
#define LEAVE xf86Msg(X_ERROR, "%s done\n", __func__)
#define DBGMSG xf86Msg
#else
#define ENTER
#define DBGMSG if (0) xf86Msg
#define LEAVE
#endif

static void
SummitWaitMarker(ScreenPtr pScreen, int Marker)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	NGLEPtr fPtr = NGLEPTR(pScrn);
	int bail = 10000000, reg;
	ENTER;
	
	do {
		reg = NGLERead4(fPtr, VISFX_STATUS);
		bail--;
		if (bail == 0) {
			xf86Msg(X_ERROR, "%s status %08x\n", __func__, reg);
			return;
		}
	} while (reg != 0);
	LEAVE;
}

static Bool
SummitPrepareCopy
(
    PixmapPtr pSrcPixmap,
    PixmapPtr pDstPixmap,
    int       xdir,
    int       ydir,
    int       alu,
    Pixel     planemask
)
{
	ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
	NGLEPtr fPtr = NGLEPTR(pScrn);
	int srcpitch = exaGetPixmapPitch(pSrcPixmap);
	int srcoff = exaGetPixmapOffset(pSrcPixmap);

	ENTER;

	DBGMSG(X_ERROR, "%s %d %d\n", __func__, srcoff, srcpitch);
	if (alu != GXcopy) return FALSE;
	fPtr->offset = srcoff / srcpitch;
	if (fPtr->hwmode != HW_BLIT) {
		SummitWaitMarker(pSrcPixmap->drawable.pScreen, 0);
		NGLEWrite4(fPtr, VISFX_VRAM_WRITE_MODE, VISFX_WRITE_MODE_PLAIN);
		NGLEWrite4(fPtr, VISFX_VRAM_READ_MODE, VISFX_READ_MODE_COPY);
		fPtr->hwmode = HW_BLIT;
	}
	NGLEWrite4(fPtr, VISFX_PLANE_MASK, planemask);
	LEAVE;
	return TRUE;
}

static void
SummitCopy
(
    PixmapPtr pDstPixmap,
    int       xs,
    int       ys,
    int       xd,
    int       yd,
    int       wi,
    int       he
)
{
	ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
	NGLEPtr fPtr = NGLEPTR(pScrn);
	int dstpitch = exaGetPixmapPitch(pDstPixmap);
	int dstoff = exaGetPixmapOffset(pDstPixmap);

	ENTER;
	SummitWaitMarker(pDstPixmap->drawable.pScreen, 0);
	NGLEWrite4(fPtr, VISFX_COPY_SRC, (xs << 16) | (ys + fPtr->offset));
	NGLEWrite4(fPtr, VISFX_COPY_WH, (wi << 16) | he);
	NGLEWrite4(fPtr, VISFX_COPY_DST, (xd << 16) | (yd + (dstoff / dstpitch)));

	exaMarkSync(pDstPixmap->drawable.pScreen);
	LEAVE;
}

static void
SummitDoneCopy(PixmapPtr pDstPixmap)
{
    ENTER;
    LEAVE;
}

static Bool
SummitPrepareSolid(
    PixmapPtr pPixmap,
    int alu,
    Pixel planemask,
    Pixel fg)
{
	ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
	NGLEPtr fPtr = NGLEPTR(pScrn);

	ENTER;
	if (alu != GXcopy) return FALSE;
	if (fPtr->hwmode != HW_FILL) {
		SummitWaitMarker(pPixmap->drawable.pScreen, 0);
		NGLEWrite4(fPtr, VISFX_VRAM_WRITE_MODE, VISFX_WRITE_MODE_FILL);
		fPtr->hwmode = HW_FILL;
	}
	NGLEWrite4(fPtr, VISFX_FG_COLOUR, fg);
	NGLEWrite4(fPtr, VISFX_PLANE_MASK, planemask);
	LEAVE;
	return TRUE;
}

static void
SummitSolid(
    PixmapPtr pPixmap,
    int x1,
    int y1,
    int x2,
    int y2)
{
	ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
	NGLEPtr fPtr = NGLEPTR(pScrn);
	int wi = x2 - x1, he = y2 - y1;
	int pitch = exaGetPixmapPitch(pPixmap);
	int offset = exaGetPixmapOffset(pPixmap);
	uint32_t mask;

	ENTER;
	
	y1 += offset / pitch;
	
	SummitWaitMarker(pPixmap->drawable.pScreen, 0);
	NGLEWrite4(fPtr, VISFX_START, (x1 << 16) | y1);
	NGLEWrite4(fPtr, VISFX_SIZE, (wi << 16) | he);

	exaMarkSync(pPixmap->drawable.pScreen);
	LEAVE;
}

Bool
SummitPrepareAccess(PixmapPtr pPixmap, int index)
{
	ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
	NGLEPtr fPtr = NGLEPTR(pScrn);

	SummitWaitMarker(pPixmap->drawable.pScreen, 0);
	NGLEWrite4(fPtr, VISFX_VRAM_WRITE_MODE, VISFX_WRITE_MODE_PLAIN);
	NGLEWrite4(fPtr, VISFX_VRAM_READ_MODE, VISFX_READ_MODE_COPY);
	fPtr->hwmode = HW_BLIT;
	
	return TRUE;
}
Bool
SummitInitAccel(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	NGLEPtr fPtr = NGLEPTR(pScrn);
	ExaDriverPtr pExa;
	int lines, bpp = pScrn->bitsPerPixel >> 3;

	pExa = exaDriverAlloc();
	if (!pExa)
		return FALSE;

	fPtr->pExa = pExa;

	pExa->exa_major = EXA_VERSION_MAJOR;
	pExa->exa_minor = EXA_VERSION_MINOR;

	pExa->memoryBase = fPtr->fbmem;
	lines = 1;/* until we figure out how to use more memory */
	DBGMSG(X_ERROR, "lines %d\n", lines);	
	pExa->memorySize = fPtr->fbi.fbi_stride * (fPtr->fbi.fbi_height + 1); //fPtr->fbmem_len;
	pExa->offScreenBase = fPtr->fbi.fbi_stride * fPtr->fbi.fbi_height;
	pExa->pixmapOffsetAlign = fPtr->fbi.fbi_stride;
	pExa->pixmapPitchAlign = fPtr->fbi.fbi_stride;

	pExa->flags = EXA_OFFSCREEN_PIXMAPS | EXA_MIXED_PIXMAPS;

	pExa->maxX = 2048;
	pExa->maxY = 2048;	

	fPtr->hwmode = -1;

	pExa->WaitMarker = SummitWaitMarker;
	pExa->Solid = SummitSolid;
	pExa->DoneSolid = SummitDoneCopy;
	pExa->Copy = SummitCopy;
	pExa->DoneCopy = SummitDoneCopy;
	pExa->PrepareCopy = SummitPrepareCopy;
	pExa->PrepareSolid = SummitPrepareSolid;
	pExa->PrepareAccess = SummitPrepareAccess;
	SummitWaitMarker(pScreen, 0);

	return exaDriverInit(pScreen, pExa);
}
