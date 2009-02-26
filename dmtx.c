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

/**
 * @file dmtx.c
 * @brief Main libdmtx source file
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <float.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include "dmtx.h"
#include "dmtxstatic.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_VALUES_H
#include <values.h>
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
 * This use of #include to merge .c files together is deliberate, if unusual.
 * This allows library functions to be grouped in files of like-functionality
 * while still enabling static functions to be accessed across files.
 */

#include "dmtxtime.c"
#include "dmtxscangrid.c"
#include "dmtxmessage.c"
#include "dmtxregion.c"
#include "dmtxdecode.c"
#include "dmtxencode.c"
#include "dmtxplacemod.c"
#include "dmtxfec.c"
#include "dmtxreedsol.c"
#include "dmtxsymbol.c"
#include "dmtxvector2.c"
#include "dmtxmatrix3.c"
#include "dmtximage.c"

extern char *
dmtxVersion(void)
{
   return DmtxVersion;
}
