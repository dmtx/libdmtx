/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (c) 2008 Mike Laughton
Copyright (c) 2008 Olivier Guilyardi

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
   int edgeMin;         /* -e, --minimum-edge */
   int squareDevn;      /* -d, --square-deviation */
   int scanGap;         /* -g, --gap */
   int timeoutMS;       /* -m, --milliseconds */
   int newline;         /* -n, --newline */
   char *resolution;    /* -r, --resolution */
   int sizeIdxExpected; /* -s, --symbol-size */
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
   int shrinkMax;       /* -s, --shrink */
   int shrinkMin;       /* -s, --shrink (if range specified) */
   int verbose;         /* -v, --verbose */
} UserOptions;

typedef struct _ImageReader {
   Image *        image;
   ImageInfo *    info;
   ExceptionInfo  exception;
} ImageReader;

static void SetOptionDefaults(UserOptions *opt);
static int HandleArgs(UserOptions *opt, int *fileIndex, int *argcp, char **argvp[]);
static void ShowUsage(int status);
static int ScaleNumberString(char *s, int extent);
static void ListImageFormats(void);
static int OpenImage(ImageReader *reader, char *imagePath, char *resolution);
static DmtxImage *ReadImagePage(ImageReader *reader, int index);
static void CloseImage(ImageReader *reader);
static int PrintDecodedOutput(UserOptions *opt, DmtxImage *image,
                              DmtxRegion *region, DmtxMessage *message, int pageIndex);
static void WriteDiagnosticImage(DmtxDecode *dec, DmtxRegion *reg, char *imagePath);

#endif
