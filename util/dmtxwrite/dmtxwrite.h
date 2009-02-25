/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2008, 2009 Mike Laughton

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

#ifndef __DMTXWRITE_H__
#define __DMTXWRITE_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <wand/magick-wand.h>
#include "../../dmtx.h"
#include "../common/dmtxutil.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _VISUALC_
#include <io.h>
#define snprintf sprintf_s
#endif

#if ENABLE_NLS
# include <libintl.h>
# define _(String) gettext(String)
#else
# define _(String) String
#endif
#define N_(String) String

#define DMTXWRITE_BUFFER_SIZE 4096

typedef struct {
   char *inputPath;
   char *outputPath;
   char *format;
   int codewords;
   int marginSize;
   int moduleSize;
   int scheme;
   int preview;
   int rotate;
   int sizeIdx;
   int color[3];
   int bgColor[3];
   int mosaic;
   int dpi;
   int verbose;
} UserOptions;

static UserOptions GetDefaultOptions(void);
static DmtxPassFail HandleArgs(UserOptions *opt, int *argcp, char **argvp[]);
static void ReadInputData(int *codeBuffer, unsigned char *codeBufferSize, UserOptions *opt);
static void ShowUsage(int status);
static void CleanupMagick(MagickWand **wand, int magickError);
static void ListImageFormats(void);
static char *GetImageFormat(UserOptions *opt);
static DmtxPassFail WriteImageFile(UserOptions *opt, DmtxEncode *enc, char *format);
static DmtxPassFail WriteSvgFile(UserOptions *opt, DmtxEncode *enc, char *format);
static DmtxPassFail WriteAsciiPreview(DmtxEncode *enc);
static DmtxPassFail WriteCodewordList(DmtxEncode *enc);
static DmtxBoolean StrNCmpI(const char *s1, const char *s2, size_t n);

#endif
