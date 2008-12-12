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

#include <magick/api.h>

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
   int stopAfter;       /* -N, --stop-after */
   int pageNumber;      /* -P, --page-number */
   int corners;         /* -R, --corners */
   int shrinkMax;       /* -s, --shrink */
   int shrinkMin;       /* -s, --shrink (if range specified) */
   int verbose;         /* -v, --verbose */
} UserOptions;

/* Functions */
static void SetOptionDefaults(UserOptions *opt);
static DmtxPassFail HandleArgs(UserOptions *opt, int *fileIndex, int *argcp, char **argvp[]);
static void ShowUsage(int status);
static Image *OpenImageList(ImageInfo **gmInfo, char *imagePath, char *resolution);
static void CleanupMagick(Image **gmImage, ImageInfo **gmInfo);
static void WritePixelsToBuffer(unsigned char *pxl, Image *gmPage);
static DmtxPassFail SetDecodeOptions(DmtxDecode *dec, DmtxImage *img, UserOptions *opt);
static DmtxPassFail PrintDecodedOutput(UserOptions *opt, DmtxImage *image,
      DmtxRegion *region, DmtxMessage *message, int pageIndex);
static void WriteDiagnosticImage(DmtxDecode *dec, DmtxRegion *reg, char *imagePath);
static void ListImageFormats(void);
static int ScaleNumberString(char *s, int extent);

typedef Image                     GmImage;
typedef ImageInfo                 GmImageInfo;
typedef ExceptionInfo             GmExceptionInfo;

#define Image                     CALL_WITH_GM_PREFIX
#define ImageInfo                 CALL_WITH_GM_PREFIX
#define ExceptionInfo             CALL_WITH_GM_PREFIX

/* XXX find a way to make these illegal
#define CatchException            gmPrefixMissing
#define CloneImageInfo            gmPrefixMissing
#define DestroyExceptionInfo      gmPrefixMissing
#define DestroyImage              gmPrefixMissing
#define DestroyImageInfo          gmPrefixMissing
#define DestroyMagick             gmPrefixMissing
#define DispatchImage             gmPrefixMissing
#define GetExceptionInfo          gmPrefixMissing
#define GetImageFromList          gmPrefixMissing
#define GetImageListLength        gmPrefixMissing
#define GetMagickInfoArray        gmPrefixMissing
#define InitializeMagick          gmPrefixMissing
#define ReadImage                 gmPrefixMissing
*/

#define gmCatchException          CatchException
#define gmCloneImageInfo          CloneImageInfo
#define gmDestroyExceptionInfo    DestroyExceptionInfo
#define gmDestroyImage            DestroyImage
#define gmDestroyImageInfo        DestroyImageInfo
#define gmDestroyMagick           DestroyMagick
#define gmDispatchImage           DispatchImage
#define gmGetExceptionInfo        GetExceptionInfo
#define gmGetImageFromList        GetImageFromList
#define gmGetImageListLength      GetImageListLength
#define gmGetMagickInfoArray      GetMagickInfoArray
#define gmInitializeMagick        InitializeMagick
#define gmReadImage               ReadImage

#endif
