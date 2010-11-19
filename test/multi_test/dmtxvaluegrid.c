#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <assert.h>
#include "../../dmtx.h"
#include "multi_test.h"

/**
 *
 *
 */
DmtxValueGrid *
dmtxValueGridCreate(int width, int height, int type)
{
   DmtxValueGrid *valueGrid;

   valueGrid = (DmtxValueGrid *)malloc(sizeof(DmtxValueGrid));
   if(valueGrid == NULL)
      return NULL;

   valueGrid->width = width;
   valueGrid->height = height;
   valueGrid->type = type;

   valueGrid->value = (int *)malloc(width * height * sizeof(int));
   if(valueGrid->value == NULL)
   {
      dmtxValueGridDestroy(&valueGrid);
      return NULL;
   }

   valueGrid->ref = NULL;

   return valueGrid;
}

/**
 *
 *
 */
DmtxPassFail
dmtxValueGridDestroy(DmtxValueGrid **valueGrid)
{
   if(valueGrid == NULL || *valueGrid == NULL)
      return DmtxFail;

   if((*valueGrid)->value != NULL)
      free((*valueGrid)->value);

   free(*valueGrid);
   *valueGrid = NULL;

   return DmtxPass;
}

/**
 *
 *
 */
int
dmtxValueGridGetWidth(DmtxValueGrid *valueGrid)
{
   if(valueGrid == NULL)
      return DmtxUndefined;

   return valueGrid->width;
}

/**
 *
 *
 */
int
dmtxValueGridGetHeight(DmtxValueGrid *valueGrid)
{
   if(valueGrid == NULL)
      return DmtxUndefined;

   return valueGrid->height;
}

/**
 *
 *
 */
int
dmtxValueGridGetValue(DmtxValueGrid *valueGrid, int x, int y)
{
   int idx;

   if(valueGrid == NULL)
      return DmtxUndefined;

   /* bounds checking could go here */
   idx = y * valueGrid->width + x;

   return valueGrid->value[idx];
}

/**
 * 3x3 Sobel Kernel
 */
DmtxPassFail
SobelCachePopulate(DmtxDecode2 *dec, DmtxImage *img)
{
   int bytesPerPixel, rowSizeBytes, colorPlane;
   int sx, sy;
   int py, pOffset;
   int vMag, bMag, hMag, sMag;
   int colorLoLf, colorLoMd, colorLoRt;
   int colorMdRt, colorHiRt, colorHiMd;
   int colorHiLf, colorMdLf, colorMdMd;
   int idx;
   int sWidth, sHeight;

   sWidth = dmtxImageGetProp(img, DmtxPropWidth) - 2;
   sHeight = dmtxImageGetProp(img, DmtxPropHeight) - 2;

   dec->vSobel = dmtxValueGridCreate(sWidth, sHeight, SobelEdgeVertical);
   dec->bSobel = dmtxValueGridCreate(sWidth, sHeight, SobelEdgeBackslash);
   dec->hSobel = dmtxValueGridCreate(sWidth, sHeight, SobelEdgeHorizontal);
   dec->sSobel = dmtxValueGridCreate(sWidth, sHeight, SobelEdgeSlash);
   if(dec->vSobel == NULL || dec->bSobel == NULL || dec->hSobel == NULL || dec->sSobel == NULL)
      return DmtxFail; /* Cleanup will be handled by caller */

   rowSizeBytes = dmtxImageGetProp(img, DmtxPropRowSizeBytes);
   bytesPerPixel = dmtxImageGetProp(img, DmtxPropBytesPerPixel);
   colorPlane = 1; /* XXX need to make some decisions here */

   for(sy = 0; sy < sHeight; sy++)
   {
      py = sHeight - sy;

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

      for(sx = 0; sx < sWidth; sx++)
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

         idx = sy * sWidth + sx;
         dec->vSobel->value[idx] = vMag;
         dec->bSobel->value[idx] = bMag;
         dec->hSobel->value[idx] = hMag;
         dec->sSobel->value[idx] = sMag;

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
DmtxValueGrid *
AccelCacheCreate(DmtxValueGrid *sobel, AccelEdgeType accelEdgeType)
{
   int x, y;
   int aIdx, sInc, sIdx, sIdxNext;
   int sWidth, sHeight;
   int aWidth, aHeight;
   DmtxValueGrid *accel;

   sWidth = dmtxValueGridGetWidth(sobel);
   sHeight = dmtxValueGridGetHeight(sobel);

   if(accelEdgeType == AccelEdgeVertical)
   {
      aWidth = sWidth - 1;
      aHeight = sHeight;
      sInc = 1;
   }
   else
   {
      aWidth = sWidth;
      aHeight = sHeight - 1;
      sInc = sWidth;
   }

   accel = dmtxValueGridCreate(aWidth, aHeight, accelEdgeType);
   assert(accel != NULL); /* XXX error handling here */

   accel->ref = sobel;

   for(y = 0; y < aHeight; y++)
   {
      sIdx = y * sWidth;
      aIdx = y * aWidth;

      for(x = 0; x < aWidth; x++)
      {
         sIdxNext = sIdx + sInc;
         accel->value[aIdx] = sobel->value[sIdxNext] - sobel->value[sIdx];
         aIdx++;
         sIdx++;
      }
   }

   return accel;
}

/**
 *
 *
 */
int
SobelCacheGetValue(DmtxValueGrid *sobel, int sIdx)
{
   int sValue;

   sValue = sobel->value[sIdx];

   return sValue;
}

/**
 *
 *
 */
int
SobelCacheGetIndexFromZXing(DmtxValueGrid *sobel, AccelEdgeType edgeType, int zCol, int zRow)
{
   int sRow, sCol;

   switch(edgeType) {
      case DmtxDirVertical:
         sRow = zRow;
         sCol = zCol + 1;
         break;
      case DmtxDirHorizontal:
         sRow = zRow + 1;
         sCol = zCol;
         break;
      default:
         return DmtxUndefined;
   }

   assert(sRow >= 0);
   assert(sCol >= 0);

   return sRow * dmtxValueGridGetWidth(sobel) + sCol;
}
