/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2010 Mike Laughton

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

Contact: mblaughton@users.sourceforge.net
*/

/* $Id: dmtxregion2.c 1065 2010-11-11 08:12:30Z mblaughton $ */

#include "../../dmtx.h"
#include "multi_test.h"

/*
#include "kiss_fftr.h"
*/

/**
 *
 *
 */
DmtxDecode2 *
dmtxDecode2Create(DmtxImage *img)
{
   DmtxDecode2 *dec;

   dec = (DmtxDecode2 *)calloc(1, sizeof(DmtxDecode2));
   if(dec == NULL)
      return NULL;

   dec->image = img;

   InitHoughCache2(&(dec->hough));
/*
   also set correct number of local hough caches
   initialize hough cache offsets
*/

   return dec;
}

/**
 *
 *
 */
DmtxPassFail
dmtxDecode2Destroy(DmtxDecode2 **dec)
{
   if(dec == NULL || *dec == NULL)
      return DmtxFail;

   free(*dec);
   *dec = NULL;

   return DmtxPass;
}
