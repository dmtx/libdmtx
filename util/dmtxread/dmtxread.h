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
   int codewords;
   int scanGap;
   int newline;
   char *xRangeMin;
   char *xRangeMax;
   char *yRangeMin;
   char *yRangeMax;
   int msec;
   int verbose;
   int maxCorrections;
   int diagnose;
   int mosaic;
   int pageNumber;
   int corners;
} UserOptions;

typedef enum {
   ImageFormatUnknown,
   ImageFormatPng,
   ImageFormatTiff
} ImageFormat;

static void SetOptionDefaults(UserOptions *options);
static int HandleArgs(UserOptions *options, int *fileIndex, int *argcp, char **argvp[]);
static int SetRangeLimit(int *target, char *optionString, int minMax, int limit);
static int SetScanRegion(DmtxPixelLoc *p0, DmtxPixelLoc *p1, UserOptions *options, DmtxImage *image);
static void ShowUsage(int status);
static ImageFormat GetImageFormat(char *imagePath);
static DmtxImage *LoadImage(char *imagePath, int pageIndex);
static DmtxImage *LoadImagePng(char *imagePath);
static DmtxImage *LoadImageTiff(char *imagePath, int pageIndex);
static int PrintDecodedOutput(UserOptions *options, DmtxImage *image,
      DmtxRegion *region, DmtxMessage *message, int pageIndex);
static void WriteImagePnm(UserOptions *options, DmtxDecode *decode,
      DmtxMessage *message, int sizeIdx, char *imagePath);

#endif
