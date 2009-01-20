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

/**
 * @file dmtximage.c
 * @brief Image handling
 */

/**
 * libdmtx handles each image as a single 1D array of packed pixel data.
 * All image operations will read or write to this array, but will never
 * access a filesystem or other external target directly. Instead, libdmtx
 * relies on the calling program to transfer data between this array and
 * the outside world (e.g., saving to disk, acquiring camera input,
 * etc...), and therefore requires an approach for efficiently sharing
 * pixel data. Calling programs have two options:
 *
 *  1) libdmtx can use an existing array of pixels that was previously
 *     allocated by the calling program. Data in this array will not be
 *     altered by the barcode scanning process, so libdmtx can safely use
 *     display memory and pixel buffers to achieve fast performance. Pixel
 *     memory used with this option will not be freed when the DmtxImage
 *     struct is released by dmtxImageDestroy().
 *
 *  2) libdmtx can also allocate the pixel array itself. When scanning a
 *     barcode, this option requires the calling program to write a copy
 *     of its pixel data to the newly allocated array (more work than #1
 *     above). This memory will be freed automatically when the DmtxImage
 *     struct is released by dmtxImageDestroy().
 *
 * When calling dmtxImageCreate() the program must also specify certain
 * parameters to instruct libdmtx about the underlying image structure.
 * This allows the library to use a large number of image structures (i.e.,
 * row orders, packing format, color depths) while still presenting a
 * consistent pixel coordinate scheme to the caller.
 *
 * Regardless of how an image is stored internally, libdmtx always
 * considers (x=0,y=0) to represent the bottom-left pixel location of an
 * image. Care must be taken to ensure that images are properly flipped
 * (or not flipped) for this behavior to work correctly.
 *
 * By default libdmtx treats the first pixel in arrays as the bottom-left
 * location of an image, with horizontal rows working upward to the final
 * pixel at the top-right corner. If mapping a pixel buffer this way
 * produces an inverted image, then specify DmtxFlipY at image creation
 * time to remove the inversion. Note that DmtxFlipY has no significant
 * affect on performance since it only modifies the pixel mapping logic,
 * and does not alter any pixel data. If the image appears correctly
 * without any flips then specify DmtxFlipNone.
 *
 *                (0,HEIGHT-1)        (WIDTH-1,HEIGHT-1)
 *                      +---------------------+
 *                      |                     |
 *                      |                     |
 *                      |       libdmtx       |
 *                      |        image        |
 *                      |     coordinates     |
 *                      |                     |
 *                      |                     |
 *                      +---------------------+
 *                    (0,0)              (WIDTH-1,0)
 *
 * Note:
 *   - OpenGL pixel arrays obtained with glReadPixels() are stored
 *     bottom-to-top; use DmtxFlipNone
 *   - Many popular image formats (e.g., PNG, GIF) store rows
 *     top-to-bottom; use DmtxFlipY
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

   img->bitsPerPixel = bpp;
   img->pack = pack;
   img->flip = flip;

   img->width = img->widthScaled = width;
   img->height = img->heightScaled = height;
   img->scale = 1;
   img->xMin = img->xMinScaled = 0;
   img->xMax = img->xMaxScaled = width - 1;
   img->yMin = img->yMinScaled = 0;
   img->yMax = img->yMaxScaled = height - 1;

   if(pxl != NULL) {
      img->mallocByDmtx = DmtxFalse;
      img->pxl = pxl;
   }
   else {
      img->mallocByDmtx = DmtxTrue;

      /* Pixels must fall along byte boundaries for auto malloc option */
      if(img->bitsPerPixel % 8 != 0)
         return NULL;

      bytesPerPixel = img->bitsPerPixel/8;
      img->pxl = malloc(bytesPerPixel * width * height * sizeof(unsigned char));
      if(img->pxl == NULL) {
         free(img);
         return NULL;
      }
   }

   switch(pack) {
      case DmtxPackK:
         if(bpp == 1 || bpp == 8)
            err = dmtxImageAddChannel(img, 0, bpp);
         else
            return NULL;
         break;
      case DmtxPackRGB:
      case DmtxPackBGR:
      case DmtxPackYCbCr:
         if(bpp == 16) {
            err = dmtxImageAddChannel(img,  0, 5);
            err = dmtxImageAddChannel(img,  5, 5);
            err = dmtxImageAddChannel(img, 10, 5);
         }
         else if(bpp == 24) {
            err = dmtxImageAddChannel(img,  0, 8);
            err = dmtxImageAddChannel(img,  8, 8);
            err = dmtxImageAddChannel(img, 16, 8);
         }
         else {
            return NULL;
         }
         break;
      case DmtxPackRGBX:
      case DmtxPackBGRX:
         if(bpp == 16) {
            err = dmtxImageAddChannel(img,  0, 5);
            err = dmtxImageAddChannel(img,  5, 5);
            err = dmtxImageAddChannel(img, 10, 5);
         }
         else if(bpp == 32) {
            err = dmtxImageAddChannel(img,  0, 8);
            err = dmtxImageAddChannel(img,  8, 8);
            err = dmtxImageAddChannel(img, 16, 8);
         }
         else {
            return NULL;
         }
         break;
      case DmtxPackXRGB:
      case DmtxPackXBGR:
         if(bpp == 16) {
            err = dmtxImageAddChannel(img,  1, 5);
            err = dmtxImageAddChannel(img,  6, 5);
            err = dmtxImageAddChannel(img, 11, 5);
         }
         else if(bpp == 32) {
            err = dmtxImageAddChannel(img,  8, 8);
            err = dmtxImageAddChannel(img, 16, 8);
            err = dmtxImageAddChannel(img, 24, 8);
         }
         else {
            return NULL;
         }
         break;
      case DmtxPackCMYK:
         err = dmtxImageAddChannel(img,  0, 8);
         err = dmtxImageAddChannel(img,  8, 8);
         err = dmtxImageAddChannel(img, 16, 8);
         err = dmtxImageAddChannel(img, 24, 8);
         break;
      case DmtxPackCustom:
         break;
      default:
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
         return img->bitsPerPixel;
      case DmtxPropBytesPerPixel:
         return img->bitsPerPixel/8;
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
 *
 *
 */
extern DmtxPassFail
dmtxImageGetPixelValue(DmtxImage *img, int x, int y, int channel, int *value)
{
   unsigned char *pixelPtr;
   int pixelValue;
   int offset;
   int mask;
   int bitShift;
   int bytesPerPixel;

   assert(img != NULL);
   assert(channel < img->channelCount);

   offset = dmtxImageGetPixelOffset(img, x, y);
   if(offset == DMTX_BAD_OFFSET)
      return DmtxFail;

   switch(img->bitsPerChannel[channel]) {
      case 1:
         assert(img->bitsPerPixel == 1);
         mask = 0x01 << (7 - offset%8);
         *value = (img->pxl[offset/8] & mask) ? 255 : 0;
         break;
      case 5:
         /* XXX might be expensive if we want to scale perfect 0-255 range */
         assert(img->bitsPerPixel == 16);
         pixelPtr = img->pxl + (offset * (img->bitsPerPixel/8));
         pixelValue = (*pixelPtr << 8) | (*(pixelPtr+1));
         bitShift = img->bitsPerPixel - 5 - img->channelStart[channel];
         mask = 0x1f << bitShift;
         *value = (((pixelValue & mask) >> bitShift) << 3);
         break;
      case 8:
         assert(img->channelStart[channel] % 8 == 0);
         assert(img->bitsPerPixel % 8 == 0);
         bytesPerPixel = img->bitsPerPixel / 8;
         *value = img->pxl[offset * bytesPerPixel + channel];
         break;
   }

   return DmtxPass;
}

/**
 *
 *
 */
extern DmtxPassFail
dmtxImageSetPixelValue(DmtxImage *img, int x, int y, int channel, int value)
{
/* unsigned char *pixelPtr; */
/* int pixelValue; */
   int offset;
/* int mask; */
/* int bitShift; */
   int bytesPerPixel;

   assert(img != NULL);
   assert(channel < img->channelCount);

   offset = dmtxImageGetPixelOffset(img, x, y);
   if(offset == DMTX_BAD_OFFSET)
      return DmtxFail;

   switch(img->bitsPerChannel[channel]) {
      case 1:
/*       assert(img->bitsPerPixel == 1);
         mask = 0x01 << (7 - offset%8);
         *value = (img->pxl[offset/8] & mask) ? 255 : 0; */
         break;
      case 5:
         /* XXX might be expensive if we want to scale perfect 0-255 range */
/*       assert(img->bitsPerPixel == 16);
         pixelPtr = img->pxl + (offset * (img->bitsPerPixel/8));
         pixelValue = (*pixelPtr << 8) | (*(pixelPtr+1));
         bitShift = img->bitsPerPixel - 5 - img->channelStart[channel];
         mask = 0x1f << bitShift;
         *value = (((pixelValue & mask) >> bitShift) << 3); */
         break;
      case 8:
         assert(img->channelStart[channel] % 8 == 0);
         assert(img->bitsPerPixel % 8 == 0);
         bytesPerPixel = img->bitsPerPixel / 8;
         img->pxl[offset * bytesPerPixel + channel] = value;
         break;
   }

   return DmtxPass;
}

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
