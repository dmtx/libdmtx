/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2008, 2009 Mike Laughton
Copyright (C) 2008 Olivier Guilyardi

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Contact: mike@dragonflylogic.com
*/

/* $Id$ */

#ifndef __DMTXREAD_H__
#define __DMTXREAD_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <assert.h>
#include <wand/magick-wand.h>
#include "../../dmtx.h"
#include "../common/dmtxutil.h"

#if ENABLE_NLS
# include <libintl.h>
# define _(String) gettext(String)
#else
# define _(String) String
#endif
#define N_(String) String

typedef struct {
   int codewords;       /* -c, --codewords */
   int edgeMin;         /* -e, --minimum-edge */
   int edgeMax;         /* -E, --maximum-edge */
   int scanGap;         /* -g, --gap */
   int timeoutMS;       /* -m, --milliseconds */
   int newline;         /* -n, --newline */
   int page;            /* -p, --page */
   int squareDevn;      /* -q, --square-deviation */
   int dpi;             /* -r, --resolution */
   int sizeIdxExpected; /* -s, --symbol-size */
   int edgeThresh;      /* -t, --threshold */
   char *xMin;          /* -x, --x-range-min */
   char *xMax;          /* -X, --x-range-max */
   char *yMin;          /* -y, --y-range-min */
   char *yMax;          /* -Y, --y-range-max */
   int correctionsMax;  /* -C, --corrections-max */
   int diagnose;        /* -D, --diagnose */
   int mosaic;          /* -M, --mosaic */
   int stopAfter;       /* -N, --stop-after */
   int pageNumbers;     /* -P, --page-numbers */
   int corners;         /* -R, --corners */
   int shrinkMax;       /* -S, --shrink */
   int shrinkMin;       /* -S, --shrink (if range specified) */
   int unicode;         /* -U, --unicode */
   int verbose;         /* -v, --verbose */
} UserOptions;

/* Functions */
static UserOptions GetDefaultOptions(void);
static DmtxPassFail HandleArgs(UserOptions *opt, int *fileIndex, int *argcp, char **argvp[]);
static void ShowUsage(int status);
static DmtxPassFail SetDecodeOptions(DmtxDecode *dec, DmtxImage *img, UserOptions *opt);
static DmtxPassFail PrintStats(DmtxMessage *msg, DmtxImage *image,
      DmtxRegion *reg, int imgPageIndex, UserOptions *opt);
static DmtxPassFail PrintMessage(DmtxMessage *msg, UserOptions *opt);
static void CleanupMagick(MagickWand **wand, int magicError);
static void ListImageFormats(void);
static void WriteDiagnosticImage(DmtxDecode *dec, char *imagePath);
static int ScaleNumberString(char *s, int extent);

#endif
