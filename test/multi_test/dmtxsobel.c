/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2010 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxsobel.c
 */

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <assert.h>
#include "../../dmtx.h"
#include "multi_test.h"

/**
 *
 *
 */
DmtxSobel *
SobelCreate(DmtxImage *img)
{
   int sWidth, sHeight;
   DmtxSobel *sobel;

   sobel = (DmtxSobel *)calloc(1, sizeof(DmtxSobel));
   if(sobel == NULL)
      return NULL;

   sWidth = dmtxImageGetProp(img, DmtxPropWidth) - 2;
   sHeight = dmtxImageGetProp(img, DmtxPropHeight) - 2;

   sobel->v = dmtxValueGridCreate(sWidth, sHeight, DmtxEdgeVertical, NULL);
   sobel->b = dmtxValueGridCreate(sWidth, sHeight, DmtxEdgeBackslash, NULL);
   sobel->h = dmtxValueGridCreate(sWidth, sHeight, DmtxEdgeHorizontal, NULL);
   sobel->s = dmtxValueGridCreate(sWidth, sHeight, DmtxEdgeSlash, NULL);

   if(sobel->v == NULL || sobel->b == NULL || sobel->h == NULL || sobel->s == NULL)
   {
      SobelDestroy(&sobel);
      return NULL;
   }

   return sobel;
}

/**
 *
 *
 */
DmtxPassFail
SobelDestroy(DmtxSobel **sobel)
{
   if(sobel == NULL || *sobel == NULL)
      return DmtxFail;

   dmtxValueGridDestroy(&((*sobel)->s));
   dmtxValueGridDestroy(&((*sobel)->h));
   dmtxValueGridDestroy(&((*sobel)->b));
   dmtxValueGridDestroy(&((*sobel)->v));

   free(*sobel);
   *sobel = NULL;

   return DmtxPass;
}

/**
 * 3x3 Sobel Kernel
 *
 */
DmtxPassFail
SobelPopulate(DmtxDecode2 *dec)
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
   DmtxSobel *sobel = dec->sobel;
   DmtxImage *img = dec->image;

   assert(dec != NULL);

   sobel = dec->sobel;
   img = dec->image;

   assert(sobel != NULL && img != NULL);

   sWidth = dmtxImageGetProp(img, DmtxPropWidth) - 2;
   sHeight = dmtxImageGetProp(img, DmtxPropHeight) - 2;

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
         sobel->v->value[idx] = vMag;
         sobel->b->value[idx] = bMag;
         sobel->h->value[idx] = hMag;
         sobel->s->value[idx] = sMag;

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

   dec->fn.dmtxValueGridCallback(sobel->v, 0);
   dec->fn.dmtxValueGridCallback(sobel->b, 1);
   dec->fn.dmtxValueGridCallback(sobel->h, 2);
   dec->fn.dmtxValueGridCallback(sobel->s, 3);

   return DmtxPass;
}
