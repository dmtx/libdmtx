/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (c) 2008 Mike Laughton

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
 *
 *
 */
extern DmtxImage *
dmtxImageMalloc(int width, int height)
{
   int pixelBufSize, compassBufSize;
   DmtxImage *image;

   image = (DmtxImage *)malloc(sizeof(DmtxImage));
   if(image == NULL) {
      return NULL;
   }

   image->pageCount = 1;
   image->width = width;
   image->height = height;

   pixelBufSize = width * height * sizeof(DmtxPixel);
   compassBufSize = width * height * sizeof(DmtxCompassEdge);

   image->pxl = (DmtxPixel *)malloc(pixelBufSize);
   if(image->pxl == NULL) {
      free(image);
      return NULL;
   }
   memset(image->pxl, 0x00, pixelBufSize);

   image->compass = (DmtxCompassEdge *)malloc(compassBufSize);
   if(image->compass == NULL) {
      free(image->pxl);
      free(image);
      return NULL;
   }
   memset(image->compass, 0x00, compassBufSize);

   return image;
}

/**
 *
 *
 */
extern int
dmtxImageDeInit(DmtxImage **image)
{
   if(*image == NULL)
      return 0; /* Error */

   if((*image)->pxl != NULL)
      free((*image)->pxl);

   if((*image)->compass != NULL)
      free((*image)->compass);

   free(*image);

   *image = NULL;

   return 0;
}

/**
 *
 *
 */
extern int
dmtxImageGetWidth(DmtxImage *image)
{
   if(image == NULL)
      ; /* Error */

   return image->width;
}

/**
 *
 *
 */
extern int
dmtxImageGetHeight(DmtxImage *image)
{
   if(image == NULL)
      ; /* Error */

   return image->height;
}

/**
 *
 *
 */
extern int
dmtxImageGetOffset(DmtxImage *image, DmtxDirection dir, int lineNbr, int offset)
{
   return (dir & DmtxDirHorizontal) ? image->width * lineNbr + offset : image->width * offset + lineNbr;
}
