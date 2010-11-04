#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "../../dmtx.h"
#include "multi_test.h"

/**
 *
 *
 */
SobelCache *
SobelCacheCreate(int width, int height)
{
   SobelCache *sobel;
   int dataSizeBytes;

   sobel = (SobelCache *)malloc(sizeof(SobelCache));
   if(sobel == NULL)
      return NULL;

   sobel->width = width;
   sobel->height = height;

   dataSizeBytes = sizeof(int) * sobel->width * sobel->height;
   sobel->vData = (int *)malloc(dataSizeBytes);
   sobel->hData = (int *)malloc(dataSizeBytes);
   sobel->sData = (int *)malloc(dataSizeBytes);
   sobel->bData = (int *)malloc(dataSizeBytes);

   if(sobel->vData == NULL || sobel->hData == NULL ||
         sobel->sData == NULL || sobel->bData == NULL) {
      SobelCacheDestroy(&sobel);
      return NULL;
   }

   return sobel;
}

/**
 *
 *
 */
SobelCache *
SobelCacheFromImage(DmtxImage *image)
{
   int imageWidth = dmtxImageGetProp(image, DmtxPropWidth);
   int imageHeight = dmtxImageGetProp(image, DmtxPropHeight);
   SobelCache *sobel;

   sobel = SobelCacheCreate(imageWidth - 2, imageHeight - 2);
   if(sobel == NULL)
      return NULL;

   SobelCachePopulate(sobel, image);

   return sobel;
}

/**
 *
 *
 */
SobelCache *
PrimeCacheFromSobel(SobelCache *sobel)
{
   int sobelWidth = SobelCacheGetWidth(sobel);
   int sobelHeight = SobelCacheGetHeight(sobel);
   SobelCache *prime;

   prime = SobelCacheCreate(sobelWidth - 1, sobelHeight - 1);
   if(prime == NULL)
      return NULL;

/* SobelCachePopulate(sobel, image); */

   return prime;
}

/**
 *
 *
 */
DmtxPassFail
SobelCacheDestroy(SobelCache **sobel)
{
   if(*sobel == NULL)
      return DmtxFail;

   if((*sobel)->vData != NULL)
      free((*sobel)->vData);

   if((*sobel)->hData != NULL)
      free((*sobel)->hData);

   if((*sobel)->sData != NULL)
      free((*sobel)->sData);

   if((*sobel)->bData != NULL)
      free((*sobel)->bData);

   free(*sobel);
   *sobel = NULL;

   return DmtxPass;
}

/**
 *
 *
 */
int
SobelCacheGetWidth(SobelCache *sobel)
{
   if(sobel == NULL)
      return DmtxUndefined;

   return sobel->width;
}

/**
 *
 *
 */
int
SobelCacheGetHeight(SobelCache *sobel)
{
   if(sobel == NULL)
      return DmtxUndefined;

   return sobel->height;
}

/**
 *
 *
 */
int
SobelCacheGetValue(SobelCache *sobel, DmtxSobelDir dir, int x, int y)
{
   int idx;

   if(sobel == NULL)
      return DmtxUndefined;

   /* bounds checking could go here */

   idx = y * sobel->width + x;

   switch(dir) {
      case DmtxSobelDirVertical:
         return sobel->vData[idx];
      case DmtxSobelDirHorizontal:
         return sobel->hData[idx];
      case DmtxSobelDirSlash:
         return sobel->sData[idx];
      case DmtxSobelDirBackslash:
         return sobel->bData[idx];
   }

   return DmtxUndefined;
}

/**
 * 3x3 Sobel Kernel
 */
DmtxPassFail
SobelCachePopulate(SobelCache *sobel, DmtxImage *img)
{
   int bytesPerPixel, rowSizeBytes, colorPlane;
   int sx, sy;
   int py, pOffset;
   int hMag, vMag, sMag, bMag;
   int colorLoLf, colorLoMd, colorLoRt;
   int colorMdRt, colorHiRt, colorHiMd;
   int colorHiLf, colorMdLf, colorMdMd;
   int idx;

   rowSizeBytes = dmtxImageGetProp(img, DmtxPropRowSizeBytes);
   bytesPerPixel = dmtxImageGetProp(img, DmtxPropBytesPerPixel);
   colorPlane = 1; /* XXX need to make some decisions here */

   for(sy = 0; sy < sobel->height; sy++)
   {
      py = sobel->height - sy;

      pOffset = py * rowSizeBytes + colorPlane;
      colorHiLf = img->pxl[pOffset - rowSizeBytes];
      colorMdLf = img->pxl[pOffset];
      colorLoLf = img->pxl[pOffset + rowSizeBytes];

      pOffset += bytesPerPixel;
      colorHiMd = img->pxl[pOffset - rowSizeBytes];
      colorMdMd = img->pxl[pOffset];
      colorLoMd = img->pxl[pOffset + rowSizeBytes];

      pOffset += bytesPerPixel;
      colorHiRt = img->pxl[pOffset - rowSizeBytes];
      colorMdRt = img->pxl[pOffset];
      colorLoRt = img->pxl[pOffset + rowSizeBytes];

      for(sx = 0; sx < sobel->width; sx++)
      {
         /**
          *   1  2  1
          *   0  0  0
          *  -1 -2 -1
          */
         hMag  = colorHiLf;
         hMag += colorHiMd * 2;
         hMag += colorHiRt;
         hMag -= colorLoLf;
         hMag -= colorLoMd * 2;
         hMag -= colorLoRt;

         /**
          *  -1  0  1
          *  -2  0  2
          *  -1  0  1
          */
         vMag  = colorHiRt;
         vMag += colorMdRt * 2;
         vMag += colorLoRt;
         vMag -= colorHiLf;
         vMag -= colorMdLf * 2;
         vMag -= colorLoLf;

         /**
          *  -2 -1  0
          *  -1  0  1
          *   0  1  2
          */
         sMag  = colorLoMd;
         sMag += colorLoRt * 2;
         sMag += colorMdRt;
         sMag -= colorHiMd;
         sMag -= colorHiLf * 2;
         sMag -= colorMdLf;

         /**
          *   0  1  2
          *  -1  0  1
          *  -2 -1  0
          */
         bMag  = colorMdLf;
         bMag += colorLoLf * 2;
         bMag += colorLoMd;
         bMag -= colorMdRt;
         bMag -= colorHiRt * 2;
         bMag -= colorHiMd;

         /**
          * If implementing these operations using MMX, can load 2
          * registers with 4 doubleword values and subtract (PSUBD).
          */

         idx = sy * sobel->width + sx;
         sobel->hData[idx] = hMag;
         sobel->vData[idx] = vMag;
         sobel->sData[idx] = sMag;
         sobel->bData[idx] = bMag;

         colorHiLf = colorHiMd;
         colorMdLf = colorMdMd;
         colorLoLf = colorLoMd;

         colorHiMd = colorHiRt;
         colorMdMd = colorMdRt;
         colorLoMd = colorLoRt;

         pOffset += bytesPerPixel;
         colorHiRt = img->pxl[pOffset - rowSizeBytes];
         colorMdRt = img->pxl[pOffset];
         colorLoRt = img->pxl[pOffset + rowSizeBytes];
      }
   }

   return DmtxPass;
}
