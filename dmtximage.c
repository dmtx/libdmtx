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
 * @file dmtximage.c
 * @brief Image handling
 */

/**
 * @fn DmtxImage *dmtxImageMalloc(int width, int height)
 * @brief allocate libdmtx image memory
 * @param width image width
 * @param height image height
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

   pixelBufSize = width * height * sizeof(DmtxRgb);
   compassBufSize = width * height * sizeof(DmtxCompassEdge);

   image->pxl = (DmtxRgb *)malloc(pixelBufSize);
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
 * @fn int dmtxImageFree(DmtxImage **img)
 * @brief free libdmtx image memory
 * @param img pointer to img location
 */
extern int
dmtxImageFree(DmtxImage **img)
{
   if(*img == NULL)
      return 0; /* Error */

   if((*img)->pxl != NULL)
      free((*img)->pxl);

   if((*img)->compass != NULL)
      free((*img)->compass);

   free(*img);

   *img = NULL;

   return 0;
}

/**
 * @fn int dmtxImageGetWidth(DmtxImage *img)
 * @brief get image width
 * @param img pointer to image
 */
extern int
dmtxImageGetWidth(DmtxImage *img)
{
   if(img == NULL)
      ; /* Error */

   return img->width;
}

/**
 * @fn int dmtxImageGetHeight(DmtxImage *img)
 * @brief get image height
 * @param img pointer to image
 */
extern int
dmtxImageGetHeight(DmtxImage *img)
{
   if(img == NULL)
      ; /* Error */

   return img->height;
}

/**
 * @fn int dmtxImageGetOffset(DmtxImage *img, DmtxDirection dir, int lineNbr, int offset)
 * @brief get pixel offset
 */
extern int
dmtxImageGetOffset(DmtxImage *img, DmtxDirection dir, int lineNbr, int offset)
{
   return (dir & DmtxDirHorizontal) ? img->width * lineNbr + offset : img->width * offset + lineNbr;
}
