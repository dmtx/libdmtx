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
   DmtxPassFail status;

   if(dec == NULL)
      return DmtxFail;

   dec->image = img;

   status = decode2ReleaseCacheMemory(dec);
   RETURN_FAIL_IF(status == DmtxFail);

   status = SobelGridPopulate(dec);
   RETURN_FAIL_IF(status == DmtxFail);

   status = AccelGridPopulate(dec);
   RETURN_FAIL_IF(status == DmtxFail);

   status = HoughGridPopulate(dec);
   RETURN_FAIL_IF(status == DmtxFail);

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

   HoughGridDestroy(&(dec->houghGrid));
   dmtxValueGridDestroy(&(dec->hsAccel));
   dmtxValueGridDestroy(&(dec->hhAccel));
   dmtxValueGridDestroy(&(dec->hbAccel));
   dmtxValueGridDestroy(&(dec->vsAccel));
   dmtxValueGridDestroy(&(dec->vbAccel));
   dmtxValueGridDestroy(&(dec->vvAccel));
   dmtxValueGridDestroy(&(dec->sSobel));
   dmtxValueGridDestroy(&(dec->hSobel));
   dmtxValueGridDestroy(&(dec->bSobel));
   dmtxValueGridDestroy(&(dec->vSobel));

   return DmtxPass;
}
