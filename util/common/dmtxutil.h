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

/* $Id: dmtxread.h 124 2008-04-13 01:38:03Z mblaughton $ */

#ifndef __DMTXUTIL_H__
#define __DMTXUTIL_H__

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

extern DmtxPassFail StringToInt(int *numberInt, char *numberString, char **terminate);
extern void FatalError(int errorCode, char *fmt, ...);
extern char *Basename(char *path);

static char *symbolSizes[] = {
      "10x10", "12x12",   "14x14",   "16x16",   "18x18",   "20x20",
      "22x22", "24x24",   "26x26",   "32x32",   "36x36",   "40x40",
      "44x44", "48x48",   "52x52",   "64x64",   "72x72",   "80x80",
      "88x88", "96x96", "104x104", "120x120", "132x132", "144x144",
       "8x18",  "8x32",   "12x26",   "12x36",   "16x36",   "16x48"
};

#endif
