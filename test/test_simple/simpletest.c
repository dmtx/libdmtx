/*
libdmtx - Data Matrix Encoding/Decoding Library
Copyright (C) 2007 Mike Laughton

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
#include <dmtx.h>

int
main(int argc, char *argv[])
{
   unsigned char str[] = "30Q324343430794<OQQ";
   DmtxEncode    enc;
   DmtxImage    *img;
   DmtxDecode    dec;
   DmtxRegion    reg;
   DmtxMessage  *msg;

   fprintf(stdout, "input:  \"%s\"\n", str);

   /* 1) ENCODE a new Data Matrix barcode image (in memory only) */

   enc = dmtxEncodeStructInit();
   dmtxEncodeDataMatrix(&enc, strlen(str), str, DMTX_SYMBOL_SQUARE_AUTO);

   /* 2) COPY the new image data before freeing encoding memory */

   img = dmtxImageMalloc(enc.image->width, enc.image->height);
   memcpy(img->pxl, enc.image->pxl, img->width * img->height * sizeof(DmtxRgb));

   dmtxEncodeStructDeInit(&enc);

   /* 3) DECODE the Data Matrix barcode from the copied image */

   dec = dmtxDecodeStructInit(img);

   reg = dmtxDecodeFindNextRegion(&dec, NULL);
   if(reg.found != DMTX_REGION_FOUND)
      exit(0);

   msg = dmtxDecodeMatrixRegion(&dec, &reg, -1);
   if(msg != NULL) {
      fputs("output: \"", stdout);
      fwrite(msg->output, sizeof(unsigned char), msg->outputIdx, stdout);
      fputs("\"\n\n", stdout);
      dmtxMessageFree(&msg);
   }

   dmtxDecodeStructDeInit(&dec);
   dmtxImageFree(&img);

   exit(0);
}
