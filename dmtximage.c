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
 * @brief  xxx
 * @param  xxx
 * @return xxx
 */
extern DmtxImage *
dmtxImageCreate(unsigned char *pxl, int width, int height, int bpp, int pack, int flip)
{
   int bytesPerPixel;
   DmtxPassFail err;
   DmtxImage *img;

   img = (DmtxImage *)calloc(1, sizeof(DmtxImage));
   if(img == NULL)
      return NULL;

   img->bpp = bpp;
   img->pack = pack;
   img->flip = flip;

   img->width = img->widthScaled = width;
   img->height = img->heightScaled = height;
   img->scale = 1;
   img->xMin = img->xMinScaled = 0;
   img->xMax = img->xMaxScaled = width - 1;
   img->yMin = img->yMinScaled = 0;
   img->yMax = img->yMaxScaled = height - 1;

   /* XXX TEMPORARY */
   img->cache = (unsigned char *)malloc(width * height * sizeof(unsigned char));
   if(img->cache == NULL) {
      free(img);
      return NULL;
   }

   if(pxl != NULL) {
      img->mallocByDmtx = DmtxFalse;
      img->pxl = pxl;
   }
   else {
      img->mallocByDmtx = DmtxTrue;

      /* Pixels must fall along byte boundaries for auto malloc option */
      if(img->bpp % 8 != 0)
         return NULL;

      bytesPerPixel = img->bpp/8;
      img->pxl = malloc(bytesPerPixel * width * height * sizeof(unsigned char));
      if(img->pxl == NULL) {
         free(img);
         return NULL;
      }
   }

   if(img->bpp == 1) {
      err = dmtxImageAddChannel(img,  0, 1);
   }
   else if(pack == DmtxPackRGB && img->bpp == 16) {
      err = dmtxImageAddChannel(img,  0, 5);
      err = dmtxImageAddChannel(img,  5, 5);
      err = dmtxImageAddChannel(img, 10, 5);
   }
   else if(pack == DmtxPackYCbCr && img->bpp == 24) {
      err = dmtxImageAddChannel(img,  0, 8);
   }
   else if(pack == DmtxPackRGB && (img->bpp == 24 || img->bpp == 32)) {
      err = dmtxImageAddChannel(img,  0, 8);
      err = dmtxImageAddChannel(img,  8, 8);
      err = dmtxImageAddChannel(img, 16, 8);
   }
   else if(pack == DmtxPackCMYK && img->bpp == 32) {
      err = dmtxImageAddChannel(img,  0, 8);
      err = dmtxImageAddChannel(img,  8, 8);
      err = dmtxImageAddChannel(img, 16, 8);
      err = dmtxImageAddChannel(img, 24, 8);
   }
   else if(pack != DmtxPackCustom) {
      return NULL;
   }

   return img;
}

/**
 * @brief  Free libdmtx image memory
 * @param  img pointer to img location
 * @return DmtxFail | DmtxPass
 */
extern DmtxPassFail
dmtxImageDestroy(DmtxImage **img)
{
   if(img == NULL || *img == NULL)
      return DmtxFail;

   if((*img)->mallocByDmtx == DmtxTrue && (*img)->pxl != NULL)
      free((*img)->pxl);

   if((*img)->cache != NULL)
      free((*img)->cache);

   free(*img);

   *img = NULL;

   return DmtxPass;
}

/**
 *
 *
 */
extern DmtxPassFail
dmtxImageAddChannel(DmtxImage *img, int channelStart, int bitsPerChannel)
{
   if(img->channelCount >= 4) /* IMAGE_MAX_CHANNEL */
      return DmtxFail;

   /* New channel extends beyond pixel data */
/* if(channelStart + bitsPerChannel > img->bitsPerPixel)
      return DmtxFail; */

   img->bitsPerChannel[img->channelCount] = bitsPerChannel;
   img->channelStart[img->channelCount] = channelStart;
   (img->channelCount)++;

   return DmtxPass;
}

/**
 * @brief  Set image property
 * @param  img pointer to image
 * @return image width
 */
extern DmtxPassFail
dmtxImageSetProp(DmtxImage *img, int prop, int value)
{
   if(img == NULL)
      return DmtxFail;

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
         return DmtxFail;
   }

   /* Specified range has non-positive area */
   if(img->xMin >= img->xMax || img->yMin >= img->yMax)
      return DmtxFail;

   /* Specified range extends beyond image boundaries */
   if(img->xMin < 0 || img->xMax >= img->width ||
         img->yMin < 0 || img->yMax >= img->height)
      return DmtxFail;

   return DmtxPass;
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
      case DmtxPropBitsPerPixel:
         return img->bpp;
      case DmtxPropBytesPerPixel:
         return img->bpp/8;
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
dmtxImageGetPixelOffset(DmtxImage *img, int x, int y)
{
   int offset;

   assert(img != NULL);

   if(dmtxImageContainsInt(img, 0, x, y) == DmtxFalse)
      return DMTX_BAD_OFFSET;

   if(img->flip & DmtxFlipY)
      offset = ((img->heightScaled - y - 1) * img->scale * img->width + (x * img->scale));
   else
      offset = img->scale * (y * img->width + x);

   return offset;
}

/**
 * @brief  Assign pixel RGB values
 * @param  img
 * @param  x
 * @param  y
 * @param  rgb
 * @return void
 */
extern DmtxPassFail
dmtxImageSetRgb(DmtxImage *img, int x, int y, DmtxRgb rgb)
{
   int pixelOffset;
   int byteOffset;

   assert(img != NULL);

   pixelOffset = dmtxImageGetPixelOffset(img, x, y);
   if(pixelOffset == DMTX_BAD_OFFSET)
      return DmtxFail;

   byteOffset = pixelOffset * (img->bpp/8);
   img->pxl[byteOffset + 0] = rgb[0];
   img->pxl[byteOffset + 1] = rgb[1];
   img->pxl[byteOffset + 2] = rgb[2];

   return DmtxPass;
}

/**
 * @brief  Retrieve RGB values of a specific pixel location
 * @param  img
 * @param  x Scaled x coordinate
 * @param  y Scaled y coordinate
 * @param  rgb
 * @return void
 */
extern DmtxPassFail
dmtxImageGetRgb(DmtxImage *img, int x, int y, DmtxRgb rgb)
{
   int pixelOffset;
   int byteOffset;

   assert(img != NULL);

   pixelOffset = dmtxImageGetPixelOffset(img, x, y);
   if(pixelOffset == DMTX_BAD_OFFSET)
      return DmtxFail;

   byteOffset = pixelOffset * (img->bpp/8);
   rgb[0] = img->pxl[byteOffset + 0];
   rgb[1] = img->pxl[byteOffset + 1];
   rgb[2] = img->pxl[byteOffset + 2];

   return DmtxPass;
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
   int pixelOffset;
   int byteOffset;

   assert(img != NULL);

   pixelOffset = dmtxImageGetPixelOffset(img, x, y);
   if(pixelOffset == DMTX_BAD_OFFSET)
      return -1;

   byteOffset = pixelOffset * (img->bpp/8);

   return img->pxl[byteOffset + colorPlane];
}
/*
 * new implementation
extern DmtxPassFail
dmtxImageGetPixelValue(DmtxImage *img, int x, int y, int channel, int *value)
{
   int offset;

   if(img == NULL || channel >= img->channelCount);
      return DmtxFail;

   if(dmtxImageContainsInt(img, 0, x, y) == DmtxFalse)
      return DmtxFail;

   offset = dmtxImageGetPixelOffset(img, x, y);
   if(offset < 0)
      return DmtxFail;

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

   return DmtxPass;
}
*/

/**
 * @brief  Test whether image contains a coordinate expressed in integers
 * @param  img
 * @param  margin Unscaled margin width
 * @param  x Scaled x coordinate
 * @param  y Scaled y coordinate
 * @return DmtxTrue | DmtxFalse
 */
extern DmtxBoolean
dmtxImageContainsInt(DmtxImage *img, int margin, int x, int y)
{
   assert(img != NULL);

   if(x - margin >= img->xMinScaled && x + margin <= img->xMaxScaled &&
         y - margin >= img->yMinScaled && y + margin <= img->yMaxScaled)
      return DmtxTrue;

   return DmtxFalse;
}

/**
 * @brief  Test whether image contains a coordinate expressed in floating points
 * @param  img
 * @param  x Scaled x coordinate
 * @param  y Scaled y coordinate
 * @return DmtxTrue | DmtxFalse
 */
extern DmtxBoolean
dmtxImageContainsFloat(DmtxImage *img, double x, double y)
{
   assert(img != NULL);

   if(x >= (double)img->xMinScaled && x <= (double)img->xMaxScaled &&
         y >= (double)img->yMinScaled && y <= (double)img->yMaxScaled)
      return DmtxTrue;

   return DmtxFalse;
}
