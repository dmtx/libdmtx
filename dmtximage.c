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

   /* Consider moving scaling factor to decode as "lens" factors or
      something ... otherwise image needs to be remalloced whenever
      decode is reinitialized ...*/

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

   img->cache = (unsigned char *)malloc(width * height * sizeof(unsigned char));
   if(img->cache == NULL) {
      free(img->pxl);
      free(img);
      return NULL;
   }

   return img;
}

/**
 * @brief  xxx
 * @param  xxx
 * @return xxx
 */
extern DmtxImage
dmtxImageSet(unsigned char *pxlBuf, int width, int height, int flip, int packing)
{
   int err;
   DmtxImage img;

   memset(&img, 0x00, sizeof(DmtxImage));

   img.pxlnew = pxlBuf;
   img.width = width;
   img.height = height;
   img.flip = flip; /* 0=NO_FLIP, 1=X_FLIP, 2=Y_FLIP, 3=(X_FLIP|Y_FLIP) */

   switch(packing) {
      case DmtxPack1bppK:
         err = dmtxImageAddChannel(&img,  0, 1);
         break;

      case DmtxPack16bppRGB:
         err = dmtxImageAddChannel(&img,  0, 5);
         err = dmtxImageAddChannel(&img,  5, 5);
         err = dmtxImageAddChannel(&img, 10, 5);
         break;

      case DmtxPack8bppK:
      case DmtxPack24bppYCbCr:
         err = dmtxImageAddChannel(&img,  0, 8);
         break;

      case DmtxPack24bppRGB:
      case DmtxPack32bppRGBA:
         err = dmtxImageAddChannel(&img,  0, 8);
         err = dmtxImageAddChannel(&img,  8, 8);
         err = dmtxImageAddChannel(&img, 16, 8);
         break;

      case DmtxPack32bppCMYK:
         err = dmtxImageAddChannel(&img,  0, 8);
         err = dmtxImageAddChannel(&img,  8, 8);
         err = dmtxImageAddChannel(&img, 16, 8);
         err = dmtxImageAddChannel(&img, 24, 8);
         break;

      default:
         break;
   }

   return img;
}

/**
 *
 *
 */
extern int
dmtxImageAddChannel(DmtxImage *img, int channelStart, int bitsPerChannel)
{
   if(img->channelCount >= 4) /* IMAGE_MAX_CHANNEL */
      return DMTX_FAILURE;

   /* New channel extends beyond pixel data */
/* if(channelStart + bitsPerChannel > img->bitsPerPixel)
      return DMTX_FAILURE; */

   img->bitsPerChannel[img->channelCount] = bitsPerChannel;
   img->channelStart[img->channelCount] = channelStart;
   (img->channelCount)++;

   return DMTX_SUCCESS;
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

   if((*img)->cache != NULL)
      free((*img)->cache);

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
      case DmtxPropHeight:
         return img->height;
      case DmtxPropArea:
         return img->width * img->height;
      case DmtxPropXmin:
         return img->xMin;
      case DmtxPropXmax:
         return img->xMax;
      case DmtxPropYmin:
         return img->yMin;
      case DmtxPropYmax:
         return img->yMax;
      case DmtxPropScale:
         return img->scale;
      case DmtxPropScaledWidth:
         return img->width/img->scale;
      case DmtxPropScaledHeight:
         return img->height/img->scale;
      case DmtxPropScaledArea:
         return (img->width/img->scale) * (img->height/img->scale);
      case DmtxPropScaledXmin:
         return img->xMinScaled;
      case DmtxPropScaledXmax:
         return img->xMaxScaled;
      case DmtxPropScaledYmin:
         return img->yMinScaled;
      case DmtxPropScaledYmax:
         return img->yMaxScaled;
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

   if(dmtxImageContainsInt(img, 0, x, y) == DMTX_FALSE)
      return DMTX_BAD_OFFSET;

   return ((img->heightScaled - y - 1) * img->scale * img->width + (x * img->scale));
}
/*
 * new implementation
 *
extern int
dmtxImageGetOffset(DmtxImage *img, int x, int y)
{
   int offset;

   if(img == NULL); // maybe check boundaries too?
      return -1;

   switch(img->originType) {
      case DmtxOriginTopLeft:
         offset = ((img->heightScaled - y - 1) * img->scale * img->width + (x * img->scale));
         break;
      case DmtxOriginBottomLeft:
         offset = img->scale * (y * img->width + x);
         break;
      default:
         return -1;
         break;
   }

   return offset;
}
*/

/**
 * @brief  Assign pixel RGB values
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

   offset = dmtxImageGetOffset(img, x, y);
   if(offset == DMTX_BAD_OFFSET)
      return DMTX_FAILURE;

   memcpy(img->pxl[offset], rgb, 3);

   return DMTX_SUCCESS;
}

/**
 * @brief  Retrieve RGB values of a specific pixel location
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

   offset = dmtxImageGetOffset(img, x, y);
   if(offset == DMTX_BAD_OFFSET)
      return DMTX_FAILURE;

   memcpy(rgb, img->pxl[offset], 3);

   return DMTX_SUCCESS;
}

/**
 * @brief  Retrieve the value of a specific color channel and pixel location
 * @param  img
 * @param  x Scaled x coordinate
 * @param  y Scaled y coordinate
 * @param  rgb
 * @return void
 */
extern int
dmtxImageGetColor(DmtxImage *img, int x, int y, int colorPlane)
{
   int offset;

   assert(img != NULL);

   offset = dmtxImageGetOffset(img, x, y);
   if(offset == DMTX_BAD_OFFSET)
      return -1;

   return img->pxl[offset][colorPlane];
}
/*
 * new implementation
extern int
dmtxImageGetPixelValue(DmtxImage *img, int x, int y, int channel, int *value)
{
   int offset;

   if(img == NULL || channel >= img->channelCount);
      return DMTX_FAILURE;

   if(dmtxImageContainsInt(img, 0, x, y) == DMTX_FALSE)
      return DMTX_FAILURE;

   offset = dmtxImageGetOffset(img, x, y);
   if(offset < 0)
      return DMTX_FAILURE;

   switch(img->bitsPerChannel[channel]) {
      case 1:
         assert(img->bitsPerPixel == 1);
         mask = 0x01 << (7 - offset%8);
         *value = (img->pxl[offset/8] & mask) ? 255 : 0;
         break;
      case 5:
         // XXX might be expensive if we want to scale perfect 0x00-0xff range
         assert(img->bitsPerPixel == 16);
         pixel = img->pxl[offset * (img->bitsPerPixel/8)];
         pixelValue = (*pixel << 8) | (*(pixel+1));
         bitShift = img->bitsPerPixel - 5 - img->channelStart[channel];
         mask = 0x1f << bitShift;
         value = ((pixelValue & mask) >> bitShift) << 3;
         break;
      case 8:
         assert(img->channelStart % 8 == 0);
         *value = img->pxl[offset * (img->bitsPerPixel/8) + (img->bitsPerChannel[channel]/8)];
         break;
   }

   return DMTX_SUCCESS;
}
*/

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

   if(x >= (double)img->xMinScaled && x <= (double)img->xMaxScaled &&
         y >= (double)img->yMinScaled && y <= (double)img->yMaxScaled)
      return DMTX_TRUE;

   return DMTX_FALSE;
}
