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
   DmtxImage *img;

   img = (DmtxImage *)malloc(sizeof(DmtxImage));
   if(img == NULL) {
      return NULL;
   }

   img->pageCount = 1;
   img->width = width;
   img->height = height;
   img->scale = 1;
   img->xMin = 0;
   img->xMax = width - 1;  /* unscaled */
   img->yMin = 0;
   img->yMax = height - 1; /* unscaled */

   pixelBufSize = width * height * sizeof(DmtxRgb);
   compassBufSize = width * height * sizeof(DmtxCompassEdge);

   img->pxl = (DmtxRgb *)malloc(pixelBufSize);
   if(img->pxl == NULL) {
      free(img);
      return NULL;
   }
   memset(img->pxl, 0x00, pixelBufSize);

   img->compass = (DmtxCompassEdge *)malloc(compassBufSize);
   if(img->compass == NULL) {
      free(img->pxl);
      free(img);
      return NULL;
   }
   memset(img->compass, 0x00, compassBufSize);

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
   int width, height;

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
      case DmtxPropXmin:
         img->xMin = value;
         break;
      case DmtxPropXmax:
         img->xMax = value;
         break;
      case DmtxPropYmin: /* Deliberate y-flip */
         img->yMax = dmtxImageGetProp(img, DmtxPropHeight) - value - 1;
         break;
      case DmtxPropYmax: /* Deliberate y-flip */
         img->yMin = dmtxImageGetProp(img, DmtxPropHeight) - value - 1;
         break;
      default:
         return DMTX_FAILURE;
         break;
   }

   /* Specified range has non-positive area */
   if(img->xMin >= img->xMax || img->yMin >= img->yMax)
      return DMTX_FAILURE;

   width = dmtxImageGetProp(img, DmtxPropWidth);
   height = dmtxImageGetProp(img, DmtxPropHeight);

   /* Specified range extends beyond image boundaries */
   if(img->xMin < 0 || img->xMax >= width ||
         img->yMin < 0 || img->yMax >= height)
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
   int scale, widthFull, heightScaled;

   assert(img != NULL);

   widthFull = dmtxImageGetProp(img, DmtxPropWidth);
   heightScaled = dmtxImageGetProp(img, DmtxPropScaledHeight);
   scale = dmtxImageGetProp(img, DmtxPropScale);

   return ((heightScaled - y - 1) * scale * widthFull + (x * scale));
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

   if(dmtxImageContainsInt(img, 0, x, y))
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

   if(dmtxImageContainsInt(img, 0, x, y))
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
   int scale;
   int xMin, xMax, yMin, yMax;

   assert(img != NULL);

   scale = dmtxImageGetProp(img, DmtxPropScale);
   xMin = img->xMin/scale;
   xMax = img->xMax/scale;
   yMin = img->yMin/scale;
   yMax = img->yMax/scale;

   if(x - margin >= xMin && x + margin <= xMax &&
         y - margin >= yMin && y + margin <= yMax) {
      return DMTX_TRUE;
   }

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
/* int width, height; */
   int scale;

   assert(img != NULL);

   scale = dmtxImageGetProp(img, DmtxPropScale);
   x *= scale; /* XXX i think this works ... ideally would scale down xMin, etc... instead for comparison */
   y *= scale;

/* width = dmtxImageGetProp(img, DmtxPropScaledWidth);
   height = dmtxImageGetProp(img, DmtxPropScaledHeight); */

   if(x >= img->xMin && y >= img->yMin && x <= img->xMax && y <= img->yMax)
      return DMTX_TRUE;

   return DMTX_FALSE;
}
