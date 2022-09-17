
/* $XConsortium: sun.h,v 5.39.1.1 95/01/05 19:58:43 kaleb Exp $ */
/* $XFree86: xc/programs/Xserver/hw/sun/sun.h,v 3.2 1995/02/12 02:36:21 dawes Exp $ */

/*-
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef _ALPHA_H_ 
#define _ALPHA_H_

/* XXX -- I define USE_WSCONS here, although there should
 *  probably be a little logic as to if to define it.
 */
#ifndef USE_WSCONS
#define USE_WSCONS
#endif

/* X headers */
#include "Xos.h"
#undef index /* don't mangle silly Sun structure member names */
#include "X.h"
#include "Xproto.h"

/* general system headers */
#ifndef NOSTDHDRS
# include <stdlib.h>
#else
# include <malloc.h>
extern char *getenv();
#endif

/* system headers common to both SunOS and Solaris */
#include <sys/param.h>
#include <sys/file.h>
#include <sys/filio.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#define _POSIX_SOURCE
#include <signal.h>
#undef _POSIX_SOURCE
#include <fcntl.h>
#include <errno.h>
#include <memory.h>

#ifdef X_NOT_STDC_ENV
extern int errno;
#endif

#include <dev/wscons/wsconsio.h>

#include <dev/pci/tgareg.h>
#include <dev/tc/sfbreg.h>
#define LK_KLL 8 /* from dev/dec/lk201var.h XXX */

extern int gettimeofday();

/* 
 * Server specific headers
 */
#include "misc.h"
#undef abs /* don't munge function prototypes in headers, sigh */
#include "scrnintstr.h"
#ifdef NEED_EVENTS
# include "inputstr.h"
#endif
#include "input.h"
#include "colormapst.h"
#include "colormap.h"
#include "cursorstr.h"
#include "cursor.h"
#include "dixstruct.h"
#include "dix.h"
#include "opaque.h"
#include "resource.h"
#include "servermd.h"
#include "windowstr.h"

/* 
 * ddx specific headers 
 */
#ifndef PSZ
#define PSZ 8
#endif

#include "mipointer.h"

extern int monitorResolution;

/*
 * MAXEVENTS is the maximum number of events the mouse and keyboard functions
 * will read on a given call to their GetEvents vectors.
 */
#define MAXEVENTS 	32

/*
 * Data private to any alpha keyboard.
 */
typedef struct {
    int		fd;
    int		type;		/* Type of keyboard */
    int		layout;		/* The layout of the keyboard */
    int		click;		/* kbd click save state */
    Leds	leds;		/* last known LED state */
    KeyCode     keys_down[LK_KLL]; /* which keys are down */
} alphaKbdPrivRec, *alphaKbdPrivPtr;

extern alphaKbdPrivRec alphaKbdPriv;

/*
 * Data private to any alpha pointer device.
 */
typedef struct {
    int		fd;
    int		bmask;		/* last known button state */
} alphaPtrPrivRec, *alphaPtrPrivPtr;

extern alphaPtrPrivRec alphaPtrPriv;

typedef struct {
    BYTE	key;
    CARD8	modifiers;
} AlphaModmapRec;

typedef struct {
    int		    width, height;
    Bool	    has_cursor;
    CursorPtr	    pCursor;		/* current cursor */
} alphaCursorRec, *alphaCursorPtr;

typedef struct {
    ColormapPtr	    installedMap;
    CloseScreenProcPtr CloseScreen;
    void	    (*UpdateColormap)();
    alphaCursorRec    hardwareCursor;
    Bool	    hasHardwareCursor;
} alphaScreenRec, *alphaScreenPtr;

#define GetScreenPrivate(s)   ((alphaScreenPtr) ((s)->devPrivates[alphaScreenIndex].ptr))
#define SetupScreen(s)	alphaScreenPtr	pPrivate = GetScreenPrivate(s)

typedef struct {
    unsigned char*  fb;		/* Frame buffer itself */
    int		    fd;		/* frame buffer for ioctl()s, */
    struct wsdisplay_fbinfo info; /* Frame buffer characteristics */
    int		    type;	/* Frame buffer type */
    int		    size;	/* Frame buffer size */
    int		    offset;	/* offset into the fb */
    union {
	    volatile tga_reg_t *tgaregs[4];  /* Registers, and their aliases */
	    volatile sfb_reg_t *sfbregs[4];  /* Registers, and their aliases */
    } regs;
#define tgaregs0 regs.tgaregs[0]
#define tgaregs1 regs.tgaregs[1]
#define tgaregs2 regs.tgaregs[2]
#define tgaregs3 regs.tgaregs[3]
    void	    (*EnterLeave)();/* screen switch */
} fbFd;

typedef Bool (*alphaFbInitProc)(
    int /* screen */,
    ScreenPtr /* pScreen */,
    int /* argc */,
    char** /* argv */
);

typedef struct {
    alphaFbInitProc	init;	/* init procedure for this fb */
    char*		name;	/* /usr/include/fbio names */
} alphaFbDataRec;

#ifdef XKB
extern Bool		noXkbExtension;
#endif

extern alphaFbDataRec	alphaFbData[];
extern int		NalphaFbData;
extern fbFd		alphaFbs[];
extern Bool		alphaActiveZaphod;
extern int		alphaScreenIndex;

extern Bool		alphaTgaAccelerate;
extern Bool		alphaSfbAccelerate;

extern Bool alphaCursorInitialize(
    ScreenPtr /* pScreen */
);

extern void alphaDisableCursor(
    ScreenPtr /* pScreen */
);

extern void alphaEnqueueEvents(
    void
);

extern Bool alphaSaveScreen(
    ScreenPtr /* pScreen */,
    int /* on */
);

extern Bool alphaScreenInit(
    ScreenPtr /* pScreen */
);

extern Bool alphaCloseScreen(
    int /* i */,
    ScreenPtr /* pScreen */
);

extern pointer alphaMemoryMap(
    size_t /* len */,
    off_t /* off */,
    int /* fd */
);

extern Bool alphaScreenAllocate(
    ScreenPtr /* pScreen */
);

extern Bool alphaInitCommon(
    int /* scrn */,
    ScreenPtr /* pScrn */,
    off_t /* offset */,
    Bool (* /* init1 */)(),
    void (* /* init2 */)(),
    Bool (* /* cr_cm */)(),
    Bool (* /* save */)(),
    int /* fb_off */
);

extern struct wscons_event* alphaKbdGetEvents(
    int /* fd */,
    int* /* pNumEvents */,
    Bool* /* pAgain */
);

extern struct wscons_event* alphaMouseGetEvents(
    int /* fd */,
    int* /* pNumEvents */,
    Bool* /* pAgain */
);

extern void alphaKbdEnqueueEvent(
    DeviceIntPtr /* device */,
    struct wscons_event * /* fe */
);

extern void alphaMouseEnqueueEvent(
    DeviceIntPtr /* device */,
    struct wscons_event * /* fe */
);

extern int alphaKbdProc(
    DeviceIntPtr /* pKeyboard */,
    int /* what */
);

extern int alphaMouseProc(
    DeviceIntPtr /* pMouse */,
    int /* what */
);

extern void alphaKbdWait(
    void
);

/*-
 * TVTOMILLI(tv)
 *	Given a struct timeval, convert its time into milliseconds...
 */
#define TVTOMILLI(tv)	(((tv).tv_usec/1000)+((tv).tv_sec*1000))

/*-
 * TSTOMILLI(ts)
 *	Given a struct timespec, convert its time into milliseconds...
 */
#define TSTOMILLI(ts)	(((ts).tv_nsec/1000000)+((ts).tv_sec*1000))

extern Bool alphaCfbSetupScreen(
    ScreenPtr /* pScreen */,
    pointer /* pbits */,	/* pointer to screen bitmap */
    int /* xsize */,		/* in pixels */
    int /* ysize */,
    int /* dpix */,		/* dots per inch */
    int /* dpiy */,		/* dots per inch */
    int /* width */,		/* pixel width of frame buffer */
    int	/* bpp */		/* bits per pixel of root */
);

extern Bool alphaCfbFinishScreenInit(
    ScreenPtr /* pScreen */,
    pointer /* pbits */,	/* pointer to screen bitmap */
    int /* xsize */,		/* in pixels */
    int /* ysize */,
    int /* dpix */,		/* dots per inch */
    int /* dpiy */,		/* dots per inch */
    int /* width */,		/* pixel width of frame buffer */
    int	/* bpp */		/* bits per pixel of root */
);

extern Bool alphaCfbScreenInit(
    ScreenPtr /* pScreen */,
    pointer /* pbits */,	/* pointer to screen bitmap */
    int /* xsize */,		/* in pixels */
    int /* ysize */,
    int /* dpix */,		/* dots per inch */
    int /* dpiy */,		/* dots per inch */
    int /* width */,		/* pixel width of frame buffer */
    int	/* bpp */		/* bits per pixel of root */
);

extern void alphaInstallColormap(
    ColormapPtr /* cmap */
);

extern void alphaUninstallColormap(
    ColormapPtr /* cmap */
);

extern int alphaListInstalledColormaps(
    ScreenPtr /* pScreen */,
    Colormap* /* pCmapList */
);

void 
alphaTgaCopyWindow(
    WindowPtr /* pWin */,
    DDXPointRec /* ptOldOrg */,
    RegionPtr /* prgnSrc */
);

void 
alphaTga32CopyWindow(
    WindowPtr /* pWin */,
    DDXPointRec /* ptOldOrg */,
    RegionPtr /* prgnSrc */
);

Bool
alphaTgaCreateGC(
    register GCPtr /* pGC */
);

Bool
alphaTga32CreateGC(
    register GCPtr /* pGC */
);

void
alphaTgaValidateGC(
    register GCPtr  /* pGC */,
    unsigned long   /* changes */,
    DrawablePtr	    /* pDrawable */
);

void
alphaTga32ValidateGC(
    register GCPtr  /* pGC */,
    unsigned long   /* changes */,
    DrawablePtr	    /* pDrawable */
);

RegionPtr
alphaTgaCopyArea(
    register DrawablePtr /* pSrcDrawable */,
    register DrawablePtr /* pDstDrawable */,
    GCPtr /* pGC */,
    int /* srcx */,
    int /* srcy */,
    int /* width */,
    int /* height */,
    int /* dstx */,
    int /* dsty */
);

RegionPtr
alphaTga32CopyArea(
    register DrawablePtr /* pSrcDrawable */,
    register DrawablePtr /* pDstDrawable */,
    GCPtr /* pGC */,
    int /* srcx */,
    int /* srcy */,
    int /* width */,
    int /* height */,
    int /* dstx */,
    int /* dsty */
);

void
alphaTgaFillSpans(
    DrawablePtr /* pDrawable */,
    GCPtr	/* pGC */,
    int		/* nInit */,
    DDXPointPtr /* pptInit */,
    int*	/* pwidthInit */,
    int 	/* fSorted */
);

void
alphaTga32FillSpans(
    DrawablePtr /* pDrawable */,
    GCPtr	/* pGC */,
    int		/* nInit */,
    DDXPointPtr /* pptInit */,
    int*	/* pwidthInit */,
    int 	/* fSorted */
);

void 
alphaSfbCopyWindow(
    WindowPtr /* pWin */,
    DDXPointRec /* ptOldOrg */,
    RegionPtr /* prgnSrc */
);

void 
alphaSfb32CopyWindow(
    WindowPtr /* pWin */,
    DDXPointRec /* ptOldOrg */,
    RegionPtr /* prgnSrc */
);

Bool
alphaSfbCreateGC(
    register GCPtr /* pGC */
);

Bool
alphaSfb32CreateGC(
    register GCPtr /* pGC */
);

void
alphaSfbValidateGC(
    register GCPtr  /* pGC */,
    unsigned long   /* changes */,
    DrawablePtr	    /* pDrawable */
);

void
alphaSfb32ValidateGC(
    register GCPtr  /* pGC */,
    unsigned long   /* changes */,
    DrawablePtr	    /* pDrawable */
);

RegionPtr
alphaSfbCopyArea(
    register DrawablePtr /* pSrcDrawable */,
    register DrawablePtr /* pDstDrawable */,
    GCPtr /* pGC */,
    int /* srcx */,
    int /* srcy */,
    int /* width */,
    int /* height */,
    int /* dstx */,
    int /* dsty */
);

RegionPtr
alphaSfb32CopyArea(
    register DrawablePtr /* pSrcDrawable */,
    register DrawablePtr /* pDstDrawable */,
    GCPtr /* pGC */,
    int /* srcx */,
    int /* srcy */,
    int /* width */,
    int /* height */,
    int /* dstx */,
    int /* dsty */
);

void
alphaSfbFillSpans(
    DrawablePtr /* pDrawable */,
    GCPtr	/* pGC */,
    int		/* nInit */,
    DDXPointPtr /* pptInit */,
    int*	/* pwidthInit */,
    int 	/* fSorted */
);

void
alphaSfb32FillSpans(
    DrawablePtr /* pDrawable */,
    GCPtr	/* pGC */,
    int		/* nInit */,
    DDXPointPtr /* pptInit */,
    int*	/* pwidthInit */,
    int 	/* fSorted */
);
#endif
