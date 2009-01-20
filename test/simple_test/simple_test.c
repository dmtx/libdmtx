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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <dmtx.h>

int
main(int argc, char *argv[])
{
   size_t          width, height, bytesPerPixel;
   unsigned char   str[] = "30Q324343430794<OQQ";
   unsigned char  *pxl;
   DmtxEncode     *enc;
   DmtxImage      *img;
   DmtxDecode     *dec;
   DmtxRegion     *reg;
   DmtxMessage    *msg;

   fprintf(stdout, "input:  \"%s\"\n", str);

   /* 1) ENCODE a new Data Matrix barcode image (in memory only) */

   enc = dmtxEncodeCreate();
   assert(enc != NULL);
   dmtxEncodeDataMatrix(enc, strlen(str), str);

   /* 2) COPY the new image data before releasing encoding memory */

   width = dmtxImageGetProp(enc->image, DmtxPropWidth);
   height = dmtxImageGetProp(enc->image, DmtxPropHeight);
   bytesPerPixel = dmtxImageGetProp(enc->image, DmtxPropBytesPerPixel);

   pxl = (unsigned char *)malloc(width * height * bytesPerPixel);
   assert(pxl != NULL);
   memcpy(pxl, enc->image->pxl, width * height * bytesPerPixel);

   dmtxEncodeDestroy(&enc);

   /* 3) DECODE the Data Matrix barcode from the copied image */

   img = dmtxImageCreate(pxl, width, height, 24, DmtxPackRGB, DmtxFlipNone);
   assert(img != NULL);

   dec = dmtxDecodeCreate(img);
   assert(dec != NULL);

   reg = dmtxRegionFindNext(dec, NULL);
   if(reg != NULL) {
      msg = dmtxDecodeMatrixRegion(dec, reg, -1);
      if(msg != NULL) {
         fputs("output: \"", stdout);
         fwrite(msg->output, sizeof(unsigned char), msg->outputIdx, stdout);
         fputs("\"\n", stdout);
         dmtxMessageDestroy(&msg);
      }
      dmtxRegionDestroy(&reg);
   }

   dmtxDecodeDestroy(&dec);
   dmtxImageDestroy(&img);
   free(pxl);

   exit(0);
}
