/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2008, 2009 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in parent directory for full terms of
 * use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 */

/**
 * \file dmtx.c
 * \brief Main libdmtx source file
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <math.h>
#include "dmtx.h"
#include "dmtxstatic.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef CALLBACK_POINT_PLOT
#define CALLBACK_POINT_PLOT(a,b,c,d)
#endif

#ifndef CALLBACK_POINT_XFRM
#define CALLBACK_POINT_XFRM(a,b,c,d)
#endif

#ifndef CALLBACK_MODULE
#define CALLBACK_MODULE(a,b,c,d,e)
#endif

#ifndef CALLBACK_MATRIX
#define CALLBACK_MATRIX(a)
#endif

#ifndef CALLBACK_FINAL
#define CALLBACK_FINAL(a,b)
#endif

/**
 * Use #include to merge the individual .c source files into a single
 * combined file during preprocessing. This enables us to organize the
 * project into files of like functionality while still presenting a
 * clean namespace. More specifically, internal functions can be made
 * static without losing the ability to access them "externally" from
 * the other files in this list.
 */

#include "dmtxtime.c"
#include "dmtxscangrid.c"
#include "dmtxmessage.c"
#include "dmtxregion.c"
#include "dmtxdecode.c"
#include "dmtxencode.c"
#include "dmtxplacemod.c"
#include "dmtxreedsol.c"
#include "dmtxsymbol.c"
#include "dmtxvector2.c"
#include "dmtxmatrix3.c"
#include "dmtximage.c"
#include "dmtxbytelist.c"
#include "dmtxencodestream.c"
#include "dmtxencodesingle.c"
#include "dmtxencodeoptimize.c"
#include "dmtxencodeascii.c"
#include "dmtxencodec40textx12.c"
#include "dmtxencodeedifact.c"
#include "dmtxencodebase256.c"

extern char *
dmtxVersion(void)
{
   return DmtxVersion;
}
