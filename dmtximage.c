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
 * @brief  Allocate libdmtx image memory
 * @param  width image width
 * @param  height image height
 * @return Address of newly allocated memory
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
   image->scale = 1;

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
 * @brief  Free libdmtx image memory
 * @param  img pointer to img location
 * @return DMTX_FAILURE | DMTX_SUCCESS
 */
extern int
dmtxImageFree(DmtxImage **img)
{
   if(*img == NULL)
      return DMTX_FAILURE;

   if((*img)->pxl != NULL)
      free((*img)->pxl);

   if((*img)->compass != NULL)
      free((*img)->compass);

   free(*img);

   *img = NULL;

   return DMTX_SUCCESS;
}

/**
 * @brief  Set image property
 * @param  img pointer to image
 * @return image width
 */
extern int
dmtxImageSetProp(DmtxImage *img, int prop, int value)
{
   if(img == NULL)
      return DMTX_FAILURE;

   switch(prop) {
      case DmtxPropWidth:
         img->width = value;
         break;
      case DmtxPropHeight:
         img->height = value;
         break;
      case DmtxPropScale:
         img->scale = value;
         break;
      default:
         return DMTX_FAILURE;
         break;
   }

   return DMTX_SUCCESS;
}

/**
 * @brief  Get image width
 * @param  img pointer to image
 * @return image width
 */
extern int
dmtxImageGetProp(DmtxImage *img, int prop)
{
   if(img == NULL)
      return -1;

   switch(prop) {
      case DmtxPropWidth:
         return img->width;
         break;
      case DmtxPropHeight:
         return img->height;
         break;
      case DmtxPropScale:
         return img->scale;
         break;
      case DmtxPropArea:
         return img->width * img->height;
         break;
      case DmtxPropScaledWidth:
         return img->width/img->scale;
         break;
      case DmtxPropScaledHeight:
         return img->height/img->scale;
         break;
   }

   return -1;
}

/**
 * @brief  XXX
 * @param  img
 * @param  x
 * @param  y
 * @return pixel offset
 */
extern int
dmtxImageGetOffset(DmtxImage *img, int x, int y)
{
   assert(img != NULL);

/* return (y * img->width + x); */
   return ((img->height - y - 1) * img->width + x);
}

/**
 * @brief  XXX
 * @param  img
 * @param  x
 * @param  y
 * @param  rgb
 * @return void
 */
extern void
dmtxImageSetRgb(DmtxImage *img, int x, int y, DmtxRgb rgb)
{
   int offset;

   assert(img != NULL);

   offset = dmtxImageGetOffset(img, x, y);

   if(dmtxImageContainsInt(img, 0, x, y))
      memcpy(img->pxl[offset], rgb, 3);
   else
      rgb[0] = rgb[1] = rgb[2] = 0;
}

/**
 * @brief  XXX
 * @param  img
 * @param  x
 * @param  y
 * @param  rgb
 * @return void
 */
extern void
dmtxImageGetRgb(DmtxImage *img, int x, int y, DmtxRgb rgb)
{
   int offset;

   assert(img != NULL);

   offset = dmtxImageGetOffset(img, x, y);

   if(dmtxImageContainsInt(img, 0, x, y))
      memcpy(rgb, img->pxl[offset], 3);
   else
      rgb[0] = rgb[1] = rgb[2] = 0;
}

/**
 * @brief  Test whether image contains a coordinate expressed in integers
 * @param  img
 * @param  margin
 * @param  x
 * @param  y
 * @return DMTX_TRUE | DMTX_FALSE
 */
extern int
dmtxImageContainsInt(DmtxImage *img, int margin, int x, int y)
{
   assert(img != NULL);

   if(margin == 0) {
      if(x >= 0 && y >= 0 && x < img->width && y < img->height)
         return DMTX_TRUE;
   }
   else {
      if(x - margin >= 0 && y - margin >= 0 && x + margin < img->width && y + margin < img->height)
         return DMTX_TRUE;
   }

   return DMTX_FALSE;
}

/**
 * @brief  Test whether image contains a coordinate expressed in floating points
 * @param  img
 * @param  x
 * @param  y
 * @return DMTX_TRUE | DMTX_FALSE
 */
extern int
dmtxImageContainsFloat(DmtxImage *img, double x, double y)
{
   assert(img != NULL);

   if(x >= 0.0 && y >= 0.0 && x < img->width && y < img->height)
      return DMTX_TRUE;

   return DMTX_FALSE;
}
