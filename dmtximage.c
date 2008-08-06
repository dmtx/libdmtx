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
   DmtxImage *img;

   img = (DmtxImage *)calloc(1, sizeof(DmtxImage));
   if(img == NULL) {
      return NULL;
   }

   img->pageCount = 1;
   img->width = img->widthScaled = width;
   img->height = img->heightScaled = height;
   img->scale = 1;
   img->xMin = img->xMinScaled = 0;
   img->xMax = img->xMaxScaled = width - 1;
   img->yMin = img->yMinScaled = 0;
   img->yMax = img->yMaxScaled = height - 1;

   img->pxl = (DmtxRgb *)calloc(width * height, sizeof(DmtxRgb));
   if(img->pxl == NULL) {
      free(img);
      return NULL;
   }

   img->compass = (DmtxCompassEdge *)calloc(width * height, sizeof(DmtxCompassEdge));
   if(img->compass == NULL) {
      free(img->pxl);
      free(img);
      return NULL;
   }

   return img;
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
         img->widthScaled = img->width/img->scale;
         break;
      case DmtxPropHeight:
         img->height = value;
         img->heightScaled = img->height/img->scale;
         break;
      case DmtxPropScale:
         img->scale = value;
         img->widthScaled = img->width/value;
         img->heightScaled = img->height/value;
         img->xMinScaled = img->xMin/value;
         img->xMaxScaled = img->xMax/value;
         img->yMinScaled = img->yMin/value;
         img->yMaxScaled = img->yMax/value;
         break;
      case DmtxPropXmin:
         img->xMin = value;
         img->xMinScaled = img->xMin/img->scale;
         break;
      case DmtxPropXmax:
         img->xMax = value;
         img->xMaxScaled = img->xMax/img->scale;
         break;
      case DmtxPropYmin: /* Deliberate y-flip */
         img->yMax = img->height - value - 1;
         img->yMaxScaled = img->yMax/img->scale;
         break;
      case DmtxPropYmax: /* Deliberate y-flip */
         img->yMin = img->height - value - 1;
         img->yMinScaled = img->yMin/img->scale;
         break;
      default:
         return DMTX_FAILURE;
         break;
   }

   /* Specified range has non-positive area */
   if(img->xMin >= img->xMax || img->yMin >= img->yMax)
      return DMTX_FAILURE;

   /* Specified range extends beyond image boundaries */
   if(img->xMin < 0 || img->xMax >= img->width ||
         img->yMin < 0 || img->yMax >= img->height)
      return DMTX_FAILURE;

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
      case DmtxPropScaledArea:
         return (img->width/img->scale) * (img->height/img->scale);
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
 * @brief  Returns pixel offset for unscaled image
 * @param  img
 * @param  x Scaled x coordinate
 * @param  y Scaled y coordinate
 * @return Unscaled pixel offset
 */
extern int
dmtxImageGetOffset(DmtxImage *img, int x, int y)
{
   assert(img != NULL);

   return ((img->heightScaled - y - 1) * img->scale * img->width + (x * img->scale));
}

/**
 * @brief  XXX
 * @param  img
 * @param  x
 * @param  y
 * @param  rgb
 * @return void
 */
extern int
dmtxImageSetRgb(DmtxImage *img, int x, int y, DmtxRgb rgb)
{
   int offset;

   assert(img != NULL);

   if(dmtxImageContainsInt(img, 0, x, y) == DMTX_FALSE)
      return DMTX_FAILURE;

   offset = dmtxImageGetOffset(img, x, y);
   memcpy(img->pxl[offset], rgb, 3);

   return DMTX_SUCCESS;
}

/**
 * @brief  XXX
 * @param  img
 * @param  x Scaled x coordinate
 * @param  y Scaled y coordinate
 * @param  rgb
 * @return void
 */
extern int
dmtxImageGetRgb(DmtxImage *img, int x, int y, DmtxRgb rgb)
{
   int offset;

   assert(img != NULL);

   if(dmtxImageContainsInt(img, 0, x, y) == DMTX_FALSE)
      return DMTX_FAILURE;

   offset = dmtxImageGetOffset(img, x, y);
   memcpy(rgb, img->pxl[offset], 3);

   return DMTX_SUCCESS;
}

/**
 * @brief  Test whether image contains a coordinate expressed in integers
 * @param  img
 * @param  margin Unscaled margin width
 * @param  x Scaled x coordinate
 * @param  y Scaled y coordinate
 * @return DMTX_TRUE | DMTX_FALSE
 */
extern int
dmtxImageContainsInt(DmtxImage *img, int margin, int x, int y)
{
   assert(img != NULL);

   if(x - margin >= img->xMinScaled && x + margin <= img->xMaxScaled &&
         y - margin >= img->yMinScaled && y + margin <= img->yMaxScaled)
      return DMTX_TRUE;

   return DMTX_FALSE;
}

/**
 * @brief  Test whether image contains a coordinate expressed in floating points
 * @param  img
 * @param  x Scaled x coordinate
 * @param  y Scaled y coordinate
 * @return DMTX_TRUE | DMTX_FALSE
 */
extern int
dmtxImageContainsFloat(DmtxImage *img, double x, double y)
{
   assert(img != NULL);

   if(x >= img->xMinScaled && x <= img->xMaxScaled &&
         y >= img->yMinScaled && y <= img->yMaxScaled)
      return DMTX_TRUE;

   return DMTX_FALSE;
}
