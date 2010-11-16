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

/* $Id$ */

#include <assert.h>
#include "../../dmtx.h"
#include "multi_test.h"

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

   decode2ReleaseCacheMemory(*dec);

   free(*dec);
   *dec = NULL;

   return DmtxPass;
}

#define RETURN_FAIL_IF(C) \
   if(C) { \
      decode2ReleaseCacheMemory(dec); \
      return DmtxFail; \
   }

/**
 *
 *
 */
DmtxPassFail
dmtxDecode2SetImage(DmtxDecode2 *dec, DmtxImage *img)
{
   int houghCol, houghRow;

   if(dec == NULL)
      return DmtxFail;

   dec->image = img;

   decode2ReleaseCacheMemory(dec);

   dec->sobel = SobelCacheCreate(dec->image);
   RETURN_FAIL_IF(dec->sobel == NULL);
   dec->fn.pixelEdgeCacheCallback(dec->sobel, 0);

   dec->accelV = AccelCacheCreate(dec->sobel, DmtxDirVertical);
   RETURN_FAIL_IF(dec->accelV == NULL);
   dec->fn.pixelEdgeCacheCallback(dec->accelV, 1);

   dec->accelH = AccelCacheCreate(dec->sobel, DmtxDirHorizontal);
   RETURN_FAIL_IF(dec->accelH == NULL);
   dec->fn.pixelEdgeCacheCallback(dec->accelH, 2);

/*
   InitHoughCache2(&(dec->hough));
   also set correct number of local hough caches
   initialize hough cache offsets
*/

   houghCol = houghRow = 0; /* XXX jimmy crack corn */

   FindZeroCrossings(dec, houghCol, houghRow, DmtxDirVertical);
   FindZeroCrossings(dec, houghCol, houghRow, DmtxDirHorizontal);

   return DmtxPass;
}

/**
 *
 *
 */
DmtxPassFail
decode2ReleaseCacheMemory(DmtxDecode2 *dec)
{
   if(dec == NULL)
      return DmtxFail;

   /* XXX release hough cache too */
   PixelEdgeCacheDestroy(&(dec->sobel));
   PixelEdgeCacheDestroy(&(dec->accelV));
   PixelEdgeCacheDestroy(&(dec->accelH));

   return DmtxPass;
}


