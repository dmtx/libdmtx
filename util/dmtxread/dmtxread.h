/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (c) 2008 Mike Laughton

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

#if ENABLE_NLS
# include <libintl.h>
# define _(String) gettext(String)
#else
# define _(String) String
#endif
#define N_(String) String

typedef struct {
   int codewords;       /* -c, --codewords */
   int squareDevn;      /* -d, --square-deviation */
   int scanGap;         /* -g, --gap */
   int timeoutMS;       /* -m, --milliseconds */
   int newline;         /* -n, --newline */
   int shrinkMax;       /* -s, --shrink */
   int shrinkMin;       /* -s, --shrink (if range specified) */
   int edgeThresh;      /* -t, --threshold */
   char *xMin;          /* -x, --x-range-min */
   char *xMax;          /* -X, --x-range-max */
   char *yMin;          /* -y, --y-range-min */
   char *yMax;          /* -Y, --y-range-max */
   int correctionsMax;  /* -C, --corrections-max */
   int diagnose;        /* -D, --diagnose */
   int mosaic;          /* -M, --mosaic */
   int pageNumber;      /* -P, --page-number */
   int corners;         /* -R, --corners */
   int verbose;         /* -v, --verbose */
} UserOptions;

typedef enum {
   ImageFormatUnknown,
   ImageFormatPng,
   ImageFormatTiff
} ImageFormat;

static void SetOptionDefaults(UserOptions *options);
static int HandleArgs(UserOptions *options, int *fileIndex, int *argcp, char **argvp[]);
static void ShowUsage(int status);
static int ScaleNumberString(char *s, int extent);
static ImageFormat GetImageFormat(char *imagePath);
static DmtxImage *LoadImage(char *imagePath, int pageIndex);
static DmtxImage *LoadImagePng(char *imagePath);
static DmtxImage *LoadImageTiff(char *imagePath, int pageIndex);
static int PrintDecodedOutput(UserOptions *options, DmtxImage *image,
      DmtxRegion *region, DmtxMessage *message, int pageIndex);
static void WriteDiagnosticImage(DmtxDecode *dec, DmtxRegion *reg, char *imagePath);

#endif
