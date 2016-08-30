/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2008, 2009 Mike Laughton. All rights reserved.
 * Copyright 2012-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtximage.c
 * \brief Image handling
 */

/**
 * libdmtx stores image data as a large one-dimensional array of packed pixels,
 * reading from the array when scanning barcodes and writing to it when creating
 * a barcode. Beyond this interaction the calling program is responsible for
 * populating and dispatching pixels between the image array and the outside
 * world, whether that means loading an image from a file, acquiring camera
 * input, displaying output to a screen, saving to disk, etc...
 *
 * By default, libdmtx treats the first pixel of an image array as the top-left
 * corner of the physical image, with the final pixel landing at the bottom-
 * right. However, if mapping a pixel buffer this way produces an inverted
 * image the calling program can specify DmtxFlipY at image creation time to
 * remove the inversion. This has a negligible effect on performance since it
 * only modifies the pixel mapping math, and does not alter any pixel data.
 *
 * Regardless of how an image is stored internally, all libdmtx functions
 * consider coordinate (0,0) to mathematically represent the bottom-left pixel
 * location of an image using a right-handed coordinate system.
 *
 *                (0,HEIGHT-1)        (WIDTH-1,HEIGHT-1)
 *
 *          array pos = 0,1,2,3,...-----------+
 *                      |                     |
 *                      |                     |
 *                      |       libdmtx       |
 *                      |        image        |
 *                      |     coordinates     |
 *                      |                     |
 *                      |                     |
 *                      +---------...,N-2,N-1,N = array pos
 *
 *                    (0,0)              (WIDTH-1,0)
 *
 * Notes:
 *   - OpenGL pixel arrays obtained with glReadPixels() are stored
 *     bottom-to-top; use DmtxFlipY
 *   - Many popular image formats (e.g., PNG, GIF) store rows
 *     top-to-bottom; use DmtxFlipNone
 */

/**
 * \brief  XXX
 * \param  XXX
 * \return XXX
 */
extern DmtxImage *
dmtxImageCreate(unsigned char *pxl, int width, int height, int pack)
{
//   DmtxPassFail err;
   DmtxImage *img;

   if(pxl == NULL || width < 1 || height < 1)
      return NULL;

   img = (DmtxImage *)calloc(1, sizeof(DmtxImage));
   if(img == NULL)
      return NULL;

   img->pxl = pxl;
   img->width = width;
   img->height = height;
   img->pixelPacking = pack;
   img->bitsPerPixel = GetBitsPerPixel(pack);
   img->bytesPerPixel = img->bitsPerPixel/8;
   img->rowPadBytes = 0;
   img->rowSizeBytes = img->width * img->bytesPerPixel + img->rowPadBytes;
   img->imageFlip = DmtxFlipNone;

   /* Leave channelStart[] and bitsPerChannel[] with zeros from calloc */
   img->channelCount = 0;

   switch(pack) {
      case DmtxPackCustom:
         break;
      case DmtxPack1bppK:
         dmtxImageSetChannel(img, 0, 1);
         return NULL; /* unsupported packing order */
/*       break; */
      case DmtxPack8bppK:
         dmtxImageSetChannel(img, 0, 8);
         break;
      case DmtxPack16bppRGB:
      case DmtxPack16bppBGR:
      case DmtxPack16bppYCbCr:
         dmtxImageSetChannel(img,  0, 5);
         dmtxImageSetChannel(img,  5, 5);
         dmtxImageSetChannel(img, 10, 5);
         break;
      case DmtxPack24bppRGB:
      case DmtxPack24bppBGR:
      case DmtxPack24bppYCbCr:
      case DmtxPack32bppRGBX:
      case DmtxPack32bppBGRX:
         dmtxImageSetChannel(img,  0, 8);
         dmtxImageSetChannel(img,  8, 8);
         dmtxImageSetChannel(img, 16, 8);
         break;
      case DmtxPack16bppRGBX:
      case DmtxPack16bppBGRX:
         dmtxImageSetChannel(img,  0, 5);
         dmtxImageSetChannel(img,  5, 5);
         dmtxImageSetChannel(img, 10, 5);
         break;
      case DmtxPack16bppXRGB:
      case DmtxPack16bppXBGR:
         dmtxImageSetChannel(img,  1, 5);
         dmtxImageSetChannel(img,  6, 5);
         dmtxImageSetChannel(img, 11, 5);
         break;
      case DmtxPack32bppXRGB:
      case DmtxPack32bppXBGR:
         dmtxImageSetChannel(img,  8, 8);
         dmtxImageSetChannel(img, 16, 8);
         dmtxImageSetChannel(img, 24, 8);
         break;
      case DmtxPack32bppCMYK:
         dmtxImageSetChannel(img,  0, 8);
         dmtxImageSetChannel(img,  8, 8);
         dmtxImageSetChannel(img, 16, 8);
         dmtxImageSetChannel(img, 24, 8);
         break;
      default:
         return NULL;
   }

   return img;
}

/**
 * \brief  Free libdmtx image memory
 * \param  img pointer to img location
 * \return DmtxFail | DmtxPass
 */
extern DmtxPassFail
dmtxImageDestroy(DmtxImage **img)
{
   if(img == NULL || *img == NULL)
      return DmtxFail;

   free(*img);

   *img = NULL;

   return DmtxPass;
}

/**
 *
 *
 */
extern DmtxPassFail
dmtxImageSetChannel(DmtxImage *img, int channelStart, int bitsPerChannel)
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
 * \brief  Set image property
 * \param  img pointer to image
 * \return image width
 */
extern DmtxPassFail
dmtxImageSetProp(DmtxImage *img, int prop, int value)
{
   if(img == NULL)
      return DmtxFail;

   switch(prop) {
      case DmtxPropRowPadBytes:
         img->rowPadBytes = value;
         img->rowSizeBytes = img->width * (img->bitsPerPixel/8) + img->rowPadBytes;
         break;
      case DmtxPropImageFlip:
         img->imageFlip = value;
         break;
      default:
         break;
   }

   return DmtxPass;
}

/**
 * \brief  Get image width
 * \param  img pointer to image
 * \return image width
 */
extern int
dmtxImageGetProp(DmtxImage *img, int prop)
{
   if(img == NULL)
      return DmtxUndefined;

   switch(prop) {
      case DmtxPropWidth:
         return img->width;
      case DmtxPropHeight:
         return img->height;
      case DmtxPropPixelPacking:
         return img->pixelPacking;
      case DmtxPropBitsPerPixel:
         return img->bitsPerPixel;
      case DmtxPropBytesPerPixel:
         return img->bytesPerPixel;
      case DmtxPropRowPadBytes:
         return img->rowPadBytes;
      case DmtxPropRowSizeBytes:
         return img->rowSizeBytes;
      case DmtxPropImageFlip:
         return img->imageFlip;
      case DmtxPropChannelCount:
         return img->channelCount;
      default:
         break;
   }

   return DmtxUndefined;
}

/**
 * \brief  Returns pixel offset for image
 * \param  img
 * \param  x coordinate
 * \param  y coordinate
 * \return pixel byte offset
 */
extern int
dmtxImageGetByteOffset(DmtxImage *img, int x, int y)
{
   assert(img != NULL);
   assert(!(img->imageFlip & DmtxFlipX)); /* DmtxFlipX is not an option */

   if(dmtxImageContainsInt(img, 0, x, y) == DmtxFalse)
      return DmtxUndefined;

   if(img->imageFlip & DmtxFlipY)
      return (y * img->rowSizeBytes + x * img->bytesPerPixel);

   return ((img->height - y - 1) * img->rowSizeBytes + x * img->bytesPerPixel);
}

/**
 *
 *
 */
extern DmtxPassFail
dmtxImageGetPixelValue(DmtxImage *img, int x, int y, int channel, int *value)
{
   int offset;
/* unsigned char *pixelPtr;
   int pixelValue;
   int mask;
   int bitShift; */

   assert(img != NULL);
   assert(channel < img->channelCount);

   offset = dmtxImageGetByteOffset(img, x, y);
   if(offset == DmtxUndefined)
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
         *value = img->pxl[offset + channel];
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
   int offset;
/* unsigned char *pixelPtr; */
/* int pixelValue; */
/* int mask; */
/* int bitShift; */

   assert(img != NULL);
   assert(channel < img->channelCount);

   offset = dmtxImageGetByteOffset(img, x, y);
   if(offset == DmtxUndefined)
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
         img->pxl[offset + channel] = value;
         break;
   }

   return DmtxPass;
}

/**
 * \brief  Test whether image contains a coordinate expressed in integers
 * \param  img
 * \param  margin width
 * \param  x coordinate
 * \param  y coordinate
 * \return DmtxTrue | DmtxFalse
 */
extern DmtxBoolean
dmtxImageContainsInt(DmtxImage *img, int margin, int x, int y)
{
   assert(img != NULL);

   if(x - margin >= 0 && x + margin < img->width &&
         y - margin >= 0 && y + margin < img->height)
      return DmtxTrue;

   return DmtxFalse;
}

/**
 * \brief  Test whether image contains a coordinate expressed in floating points
 * \param  img
 * \param  x coordinate
 * \param  y coordinate
 * \return DmtxTrue | DmtxFalse
 */
extern DmtxBoolean
dmtxImageContainsFloat(DmtxImage *img, double x, double y)
{
   assert(img != NULL);

   if(x >= 0.0 && x < (double)img->width && y >= 0.0 && y < (double)img->height)
      return DmtxTrue;

   return DmtxFalse;
}

/**
 *
 *
 */
static int
GetBitsPerPixel(int pack)
{
   switch(pack) {
      case DmtxPack1bppK:
         return 1;
      case DmtxPack8bppK:
         return 8;
      case DmtxPack16bppRGB:
      case DmtxPack16bppRGBX:
      case DmtxPack16bppXRGB:
      case DmtxPack16bppBGR:
      case DmtxPack16bppBGRX:
      case DmtxPack16bppXBGR:
      case DmtxPack16bppYCbCr:
         return 16;
      case DmtxPack24bppRGB:
      case DmtxPack24bppBGR:
      case DmtxPack24bppYCbCr:
         return  24;
      case DmtxPack32bppRGBX:
      case DmtxPack32bppXRGB:
      case DmtxPack32bppBGRX:
      case DmtxPack32bppXBGR:
      case DmtxPack32bppCMYK:
         return  32;
      default:
         break;
   }

   return DmtxUndefined;
}
