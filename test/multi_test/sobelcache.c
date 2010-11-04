#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <assert.h>
#include "../../dmtx.h"
#include "multi_test.h"

/**
 *
 *
 */
PixelEdgeCache *
PixelEdgeCacheCreate(int cacheWidth, int cacheHeight)
{
   PixelEdgeCache *cache;
   int dataSizeBytes;

   cache = (PixelEdgeCache *)malloc(sizeof(PixelEdgeCache));
   if(cache == NULL)
      return NULL;

   cache->width = cacheWidth;
   cache->height = cacheHeight;

   dataSizeBytes = sizeof(int) * cache->width * cache->height;
   cache->v = (int *)malloc(dataSizeBytes);
   cache->b = (int *)malloc(dataSizeBytes);
   cache->h = (int *)malloc(dataSizeBytes);
   cache->s = (int *)malloc(dataSizeBytes);

   if(cache->v == NULL || cache->h == NULL || cache->s == NULL || cache->b == NULL) {
      PixelEdgeCacheDestroy(&cache);
      return NULL;
   }

   return cache;
}

/**
 *
 *
 */
DmtxPassFail
PixelEdgeCacheDestroy(PixelEdgeCache **cache)
{
   if(*cache == NULL)
      return DmtxFail;

   if((*cache)->v != NULL)
      free((*cache)->v);

   if((*cache)->b != NULL)
      free((*cache)->b);

   if((*cache)->h != NULL)
      free((*cache)->h);

   if((*cache)->s != NULL)
      free((*cache)->s);

   free(*cache);
   *cache = NULL;

   return DmtxPass;
}

/**
 *
 *
 */
int
PixelEdgeCacheGetWidth(PixelEdgeCache *cache)
{
   if(cache == NULL)
      return DmtxUndefined;

   return cache->width;
}

/**
 *
 *
 */
int
PixelEdgeCacheGetHeight(PixelEdgeCache *cache)
{
   if(cache == NULL)
      return DmtxUndefined;

   return cache->height;
}

/**
 *
 *
 */
int
PixelEdgeCacheGetValue(PixelEdgeCache *cache, DmtxSobelDir dir, int x, int y)
{
   int idx;

   if(cache == NULL)
      return DmtxUndefined;

   /* bounds checking could go here */

   idx = y * cache->width + x;

   switch(dir) {
      case DmtxSobelDirVertical:
         return cache->v[idx];
      case DmtxSobelDirBackslash:
         return cache->b[idx];
      case DmtxSobelDirHorizontal:
         return cache->h[idx];
      case DmtxSobelDirSlash:
         return cache->s[idx];
   }

   return DmtxUndefined;
}

/**
 *
 *
 */
PixelEdgeCache *
SobelCacheCreate(DmtxImage *img)
{
   int sobelWidth, sobelHeight;
   PixelEdgeCache *sobel;

   sobelWidth = dmtxImageGetProp(img, DmtxPropWidth) - 2;
   sobelHeight = dmtxImageGetProp(img, DmtxPropHeight) - 2;

   sobel = PixelEdgeCacheCreate(sobelWidth, sobelHeight);
   if(sobel == NULL)
      return NULL;

   SobelCachePopulate(sobel, img);

   return sobel;
}

/**
 * 3x3 Sobel Kernel
 */
DmtxPassFail
SobelCachePopulate(PixelEdgeCache *sobel, DmtxImage *img)
{
   int bytesPerPixel, rowSizeBytes, colorPlane;
   int sx, sy;
   int py, pOffset;
   int vMag, bMag, hMag, sMag;
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
          * If implementing these operations using MMX, can load 2
          * registers with 4 doubleword values and subtract (PSUBD).
          */

         idx = sy * sobel->width + sx;
         sobel->v[idx] = vMag;
         sobel->b[idx] = bMag;
         sobel->h[idx] = hMag;
         sobel->s[idx] = sMag;

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

/**
 *
 *
 */
PixelEdgeCache *
AccelCacheCreate(PixelEdgeCache *sobel, DmtxDirection edgeType)
{
   int accelWidth, accelHeight;
   PixelEdgeCache *accel;

   assert(edgeType == DmtxDirVertical || edgeType == DmtxDirHorizontal);

   if(edgeType == DmtxDirVertical) {
      accelWidth = PixelEdgeCacheGetWidth(sobel) - 1;
      accelHeight = PixelEdgeCacheGetHeight(sobel);
   }
   else {
      accelWidth = PixelEdgeCacheGetWidth(sobel);
      accelHeight = PixelEdgeCacheGetHeight(sobel) - 1;
   }

   accel = PixelEdgeCacheCreate(accelWidth, accelHeight);
   if(accel == NULL)
      return NULL;

   /* populate cache here */

   return accel;
}
