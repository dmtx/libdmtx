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

   valueGrid = (DmtxValueGrid *)calloc(1, sizeof(DmtxValueGrid));
   if(valueGrid == NULL)
      return NULL;

   valueGrid->width = width;
   valueGrid->height = height;
   valueGrid->type = type;
   valueGrid->ref = NULL;

   valueGrid->value = (int *)malloc(width * height * sizeof(int));
   if(valueGrid->value == NULL)
   {
      dmtxValueGridDestroy(&valueGrid);
      return NULL;
   }

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

   if(x < 0 || x >= valueGrid->width || y < 0 || y >= valueGrid->height)
      return 0;

   idx = y * valueGrid->width + x;

   return valueGrid->value[idx];
}

/**
 * 3x3 Sobel Kernel
 */
DmtxPassFail
SobelGridPopulate(DmtxDecode2 *dec)
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
   DmtxImage *img = dec->image;

   sWidth = dmtxImageGetProp(img, DmtxPropWidth) - 2;
   sHeight = dmtxImageGetProp(img, DmtxPropHeight) - 2;

   dec->vSobel = dmtxValueGridCreate(sWidth, sHeight, DmtxEdgeVertical);
   dec->bSobel = dmtxValueGridCreate(sWidth, sHeight, DmtxEdgeBackslash);
   dec->hSobel = dmtxValueGridCreate(sWidth, sHeight, DmtxEdgeHorizontal);
   dec->sSobel = dmtxValueGridCreate(sWidth, sHeight, DmtxEdgeSlash);

   if(dec->vSobel == NULL || dec->bSobel == NULL || dec->hSobel == NULL || dec->sSobel == NULL)
      return DmtxFail; /* Memory cleanup will be handled by caller */

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

   dec->fn.dmtxValueGridCallback(dec->vSobel, 0);
   dec->fn.dmtxValueGridCallback(dec->bSobel, 1);
   dec->fn.dmtxValueGridCallback(dec->hSobel, 2);
   dec->fn.dmtxValueGridCallback(dec->sSobel, 3);

   return DmtxPass;
}

/**
 *
 *
 */
DmtxPassFail
AccelGridPopulate(DmtxDecode2 *dec)
{
   dec->vvAccel = AccelGridCreate(dec->vSobel, DmtxEdgeVertical);
   dec->vbAccel = AccelGridCreate(dec->bSobel, DmtxEdgeVertical);
   dec->vsAccel = AccelGridCreate(dec->sSobel, DmtxEdgeVertical);
   dec->hbAccel = AccelGridCreate(dec->bSobel, DmtxEdgeHorizontal);
   dec->hhAccel = AccelGridCreate(dec->hSobel, DmtxEdgeHorizontal);
   dec->hsAccel = AccelGridCreate(dec->sSobel, DmtxEdgeHorizontal);

   if(dec->vvAccel == NULL || dec->vbAccel == NULL || dec->vsAccel == NULL ||
      dec->hbAccel == NULL || dec->hhAccel == NULL || dec->hsAccel == NULL)
      return DmtxFail; /* Memory cleanup will be handled by caller */

   dec->fn.dmtxValueGridCallback(dec->vvAccel, 4);
   dec->fn.dmtxValueGridCallback(dec->vbAccel, 5);
   dec->fn.dmtxValueGridCallback(dec->vsAccel, 6);
   dec->fn.dmtxValueGridCallback(dec->hbAccel, 7);
   dec->fn.dmtxValueGridCallback(dec->hhAccel, 8);
   dec->fn.dmtxValueGridCallback(dec->hsAccel, 9);

   return DmtxPass;
}

/**
 *
 *
 */
DmtxValueGrid *
AccelGridCreate(DmtxValueGrid *sobel, DmtxEdgeType accelEdgeType)
{
   int x, y;
   int aIdx, sInc, sIdx, sIdxNext;
   int sWidth, sHeight;
   int aWidth, aHeight;
   DmtxValueGrid *accel;

   sWidth = dmtxValueGridGetWidth(sobel);
   sHeight = dmtxValueGridGetHeight(sobel);

   if(accelEdgeType == DmtxEdgeVertical)
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
   if(accel == NULL)
      return NULL;

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
SobelGridGetIndexFromZXing(DmtxValueGrid *sobel, DmtxEdgeType edgeType, int aCol, int aRow)
{
   int sRow, sCol;

/* XXX this doesn't make immediate sense to me when I look at it again ... why is sCol = aCol + 1 ? */
   switch(edgeType) {
      case DmtxEdgeVertical:
         sRow = aRow;
         sCol = aCol + 1;
         break;
      case DmtxEdgeHorizontal:
         sRow = aRow + 1;
         sCol = aCol;
         break;
      default:
         return DmtxUndefined;
   }

   if(sCol < 0 || sCol >= sobel->width || sRow < 0 || sRow >= sobel->height)
      return 0;

   return sRow * dmtxValueGridGetWidth(sobel) + sCol;
}
