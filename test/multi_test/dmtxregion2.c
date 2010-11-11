/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2010 Mike Laughton

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

Contact: mblaughton@users.sourceforge.net
*/

/* $Id$ */

/** TODO
 * o Refactor new code to use iRow, iCol, sRow, sCol, aRow, aCol, zRow, and
 *   zCol for index like values and x,y for coordinates (and iIdx, aIdx, etc...)
 * o Create functions to convert between index schemes (accel -> sobel, etc...)
 * o Consider switching PixelEdgeCache to use [4] instead of ->v, ->h, etc...
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <SDL/SDL.h>
#include "../../dmtx.h"
#include "multi_test.h"
#include "kiss_fftr.h"

#define RETURN_IF_FAIL(X) if(X == DmtxFail) return DmtxFail

/* Scaled unit sin */
static int uSin128[] = {
       0,    25,    50,    75,   100,   125,   150,   175,
     200,   224,   249,   273,   297,   321,   345,   369,
     392,   415,   438,   460,   483,   505,   526,   548,
     569,   590,   610,   630,   650,   669,   688,   706,
     724,   742,   759,   775,   792,   807,   822,   837,
     851,   865,   878,   891,   903,   915,   926,   936,
     946,   955,   964,   972,   980,   987,   993,   999,
    1004,  1009,  1013,  1016,  1019,  1021,  1023,  1024,
    1024,  1024,  1023,  1021,  1019,  1016,  1013,  1009,
    1004,   999,   993,   987,   980,   972,   964,   955,
     946,   936,   926,   915,   903,   891,   878,   865,
     851,   837,   822,   807,   792,   775,   759,   742,
     724,   706,   688,   669,   650,   630,   610,   590,
     569,   548,   526,   505,   483,   460,   438,   415,
     392,   369,   345,   321,   297,   273,   249,   224,
     200,   175,   150,   125,   100,    75,    50,    25 };

/* Scaled unit cos */
static int uCos128[] = {
    1024,  1024,  1023,  1021,  1019,  1016,  1013,  1009,
    1004,   999,   993,   987,   980,   972,   964,   955,
     946,   936,   926,   915,   903,   891,   878,   865,
     851,   837,   822,   807,   792,   775,   759,   742,
     724,   706,   688,   669,   650,   630,   610,   590,
     569,   548,   526,   505,   483,   460,   438,   415,
     392,   369,   345,   321,   297,   273,   249,   224,
     200,   175,   150,   125,   100,    75,    50,    25,
       0,   -25,   -50,   -75,  -100,  -125,  -150,  -175,
    -200,  -224,  -249,  -273,  -297,  -321,  -345,  -369,
    -392,  -415,  -438,  -460,  -483,  -505,  -526,  -548,
    -569,  -590,  -610,  -630,  -650,  -669,  -688,  -706,
    -724,  -742,  -759,  -775,  -792,  -807,  -822,  -837,
    -851,  -865,  -878,  -891,  -903,  -915,  -926,  -936,
    -946,  -955,  -964,  -972,  -980,  -987,  -993,  -999,
   -1004, -1009, -1013, -1016, -1019, -1021, -1023, -1024 };

void
dmtxScanImage(DmtxDecode *dec, DmtxImage *imgActive, DmtxCallbacks *fn)
{
   int i, j;
   int phiDiff;
   DmtxBoolean     regionFound;
   DmtxPassFail    err;
   DmtxEdgeCache   sobelCache;
   DmtxEdgeCache   crossingCache;
   DmtxHoughCache  houghCache;
   VanishPointSort vPoints;
   DmtxTimingSort  timings;
   AlignmentGrid   grid;
   GridRegion      region;

   /* XXX change DmtxEdgeCache into DmtxSobelCache? */
   dmtxBuildSobelCache(&sobelCache, imgActive);
   fn->edgeCacheCallback(&sobelCache, 0);

   dmtxBuildCrossingCache(&crossingCache, &sobelCache);
   fn->edgeCacheCallback(&crossingCache, 1);

   dmtxBuildHoughCache(&houghCache, &sobelCache);
   dmtxNormalizeHoughCache(&houghCache, &sobelCache);
   fn->houghCacheCallback(&houghCache, 0);

   dmtxMarkHoughMaxima(&houghCache);
   fn->houghCacheCallback(&houghCache, 1);

   vPoints = dmtxFindVanishPoints(&houghCache);
   fn->vanishPointCallback(&vPoints, 0);

   timings = dmtxFindGridTiming(&houghCache, &vPoints);

   regionFound = DmtxFalse;
   for(i = 0; regionFound == DmtxFalse && i < timings.count; i++) {
      for(j = i+1; j < timings.count; j++) {
         phiDiff = abs(timings.timing[i].phi - timings.timing[j].phi);

         /* Reject combinations that deviate from right angle (phi == 64) */
         if(abs(64 - phiDiff) > 28) /* within +- ~40 deg */
            continue;

         err = dmtxBuildGridFromTimings(&grid, timings.timing[i], timings.timing[j]);
         if(err == DmtxFail)
            continue; /* Keep trying */

         fn->timingCallback(&timings.timing[i], &timings.timing[j], 0);

         /* Hack together raw2fitFull and fit2rawFull outside since we need app data */
         AddFullTransforms(&grid);

         err = dmtxFindRegionWithinGrid(&region, &grid, &houghCache, dec, fn);
         regionFound = (err == DmtxPass) ? DmtxTrue : DmtxFalse;

         fn->gridCallback(&(region.grid), 0);
         fn->gridCallback(&(region.grid), 1);

         if(regionFound == DmtxTrue) {
            region.sizeIdx = dmtxGetSizeIdx(region.width, region.height);
            if(region.sizeIdx >= DmtxSymbol10x10 && region.sizeIdx <= DmtxSymbol16x48)
               dmtxDecodeSymbol(&region, dec);
         }

         regionFound = DmtxTrue; /* break out of outer loop */
         break; /* break out of inner loop */
      }
   }
}

#define RETURN_FAIL_IF(C) if(C) { PixelEdgeCacheDestroy(&sobel); PixelEdgeCacheDestroy(&accelV); PixelEdgeCacheDestroy(&accelH); return DmtxFail; }
/**
 *
 *
 */
DmtxPassFail
dmtxScanImage2(DmtxImage *dmtxImage, DmtxCallbacks *fn)
{
   PixelEdgeCache *sobel = NULL;
   PixelEdgeCache *accelV = NULL;
   PixelEdgeCache *accelH = NULL;

  /* perform bulk operations on entire image, regardless of position in window */

   sobel = SobelCacheCreate(dmtxImage);
   RETURN_FAIL_IF(sobel == NULL);
   fn->pixelEdgeCacheCallback(sobel, 0);

   accelV = AccelCacheCreate(sobel, DmtxDirVertical);
   RETURN_FAIL_IF(accelV == NULL);
   fn->pixelEdgeCacheCallback(accelV, 1);

   accelH = AccelCacheCreate(sobel, DmtxDirHorizontal);
   RETURN_FAIL_IF(accelH == NULL);
   fn->pixelEdgeCacheCallback(accelH, 2);

   /* XXX Implementation strategy:
    * 1) Change "local" area calculate to occur every time image position moves,
    *    and store local offset in gAppState
    * 2) Calculate zero crossings and plot results to main image
    * 3) Calculate zero crossings and add to Hough accumulator
    */

   /* initialize hough accumulator */

   /* later FindZeroCrossings will also receive a hough cache to receive zero crossing hits */
   FindZeroCrossings(accelV, sobel, DmtxDirVertical, fn);
   FindZeroCrossings(accelH, sobel, DmtxDirHorizontal, fn);

   /* find barcode here */

   PixelEdgeCacheDestroy(&accelH);
   PixelEdgeCacheDestroy(&accelV);
   PixelEdgeCacheDestroy(&sobel);

   return DmtxPass;
}

/**
 * 0 < smidge < 1
 */
DmtxPassFail
RegisterZeroCrossing(DmtxDirection edgeType, int zCol, int zRow, double smidge,
      int sValue, DmtxCallbacks *fn)
{
   double xImg, yImg;

   if(edgeType == DmtxDirVertical)
   {
      xImg = (double)zCol + 2.0 + smidge;
      yImg = (double)zRow + 1.5;
   }
   else if(edgeType == DmtxDirHorizontal)
   {
      xImg = (double)zCol + 1.5;
      yImg = (double)zRow + 2.0 + smidge;
   }
   else
   {
      return DmtxFail;
   }

   fn->zeroCrossingCallback(xImg, yImg, sValue, 0);

   /* This is where we will accumulate sample into a local hough cache */

   return DmtxPass;
}

/**
 * accel holds acceleration values (vertical or horizontal) that will determine zero crossing locations.
 * sobel contains the actual edge strength, and will only be used if a zero crossing is found
 */
DmtxPassFail
FindZeroCrossings(PixelEdgeCache *accel, PixelEdgeCache *sobel, DmtxDirection edgeType, DmtxCallbacks *fn)
{
   int zCol, zRow, s;
   int aInc, aIdx, aIdxNext, sIdx;
   int aWidth, aHeight;
   int zWidth, zHeight;
   int aPrev, aHere, aNext;
   int *accelPtr;
   int sValue;
   double smidge;

   aWidth = PixelEdgeCacheGetWidth(accel);
   aHeight = PixelEdgeCacheGetHeight(accel);

   if(edgeType == DmtxDirVertical)
   {
      zWidth = aWidth - 1;
      zHeight = aHeight;
      aInc = 1;
   }
   else if(edgeType == DmtxDirHorizontal)
   {
      zWidth = aWidth;
      zHeight = aHeight - 1;
      aInc = aWidth;
   }
   else
   {
      return DmtxFail;
   }

   for(zRow = 0; zRow < zHeight; zRow++)
   {
      aIdx = zRow * aWidth;

      for(zCol = 0; zCol < zWidth; zCol++, aIdx++)
      {
         aIdxNext = aIdx + aInc;

         for(s = 0; s < 4; s++)
         {
            switch(s) {
               case 0:
                  accelPtr = accel->v;
                  break;
               case 1:
                  accelPtr = accel->b;
                  break;
               case 2:
                  accelPtr = accel->h;
                  break;
               case 3:
                  accelPtr = accel->s;
                  break;
               default:
                  return DmtxFail;
            }
            aHere = accelPtr[aIdx];
            aNext = accelPtr[aIdxNext];

            if(OPPOSITE_SIGNS(aHere, aNext))
            {
               /* Zero crossing: Neighbors with opposite signs [-10,+10] */
               smidge = abs(aHere/(aHere - aNext));
               sIdx = SobelCacheGetIndexFromZXing(sobel, edgeType, zCol, zRow);
               if(sIdx != DmtxUndefined)
               {
                  sValue = SobelCacheGetValue(sobel, s, sIdx);
                  RegisterZeroCrossing(edgeType, zCol, zRow, smidge, sValue, fn);
               }
            }
            else if(aHere == 0 && aNext != 0)
            {
               /* No previous value for comparison (beginning of row/col) */
               if(aIdx < aInc)
                  continue;

               aPrev = accelPtr[aIdx-aInc];
               if(OPPOSITE_SIGNS(aPrev, aNext))
               {
                  /* Zero crossing: Opposite signs separated by zero [-10,0,+10] */
                  smidge = 0.0;
                  sIdx = SobelCacheGetIndexFromZXing(sobel, edgeType, zCol, zRow);
                  if(sIdx != DmtxUndefined)
                  {
                     sValue = SobelCacheGetValue(sobel, s, sIdx);
                     RegisterZeroCrossing(edgeType, zCol, zRow, smidge, sValue, fn);
                  }
               }
            }
         }
      }
   }

   return DmtxPass;
}

/**
 * 3x3 Sobel Kernel
 */
DmtxPassFail
dmtxBuildSobelCache(DmtxEdgeCache *sobelCache, DmtxImage *img)
{
   int width, height;
   int bytesPerPixel, rowSizeBytes, colorPlane;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
   int hMag, vMag, sMag, bMag;
   int colorLoLf, colorLoMd, colorLoRt;
   int colorMdRt, colorHiRt, colorHiMd;
   int colorHiLf, colorMdLf, colorMdMd;
   int offset, offsetLo, offsetMd, offsetHi;
   int idx;

   memset(sobelCache, 0x00, sizeof(DmtxEdgeCache));

   width = dmtxImageGetProp(img, DmtxPropWidth);
   height = dmtxImageGetProp(img, DmtxPropHeight);
   rowSizeBytes = dmtxImageGetProp(img, DmtxPropRowSizeBytes);
   bytesPerPixel = dmtxImageGetProp(img, DmtxPropBytesPerPixel);
   colorPlane = 1; /* XXX need to make some decisions here */

   xBeg = 1;
   xEnd = width - 2;
   yBeg = 1;
   yEnd = height - 2;

   for(y = yBeg; y <= yEnd; y++) {

      /* Pixel data first pixel = top-left; everything else bottom-left */
      offsetMd = ((height - y - 1) * rowSizeBytes) + colorPlane;
      offsetHi = offsetMd - rowSizeBytes;
      offsetLo = offsetMd + rowSizeBytes;

      colorHiLf = img->pxl[offsetHi];
      colorMdLf = img->pxl[offsetMd];
      colorLoLf = img->pxl[offsetLo];

      offset = bytesPerPixel;

      colorHiMd = img->pxl[offsetHi + offset];
      colorMdMd = img->pxl[offsetMd + offset];
      colorLoMd = img->pxl[offsetLo + offset];

      offset += bytesPerPixel;

      colorHiRt = img->pxl[offsetHi + offset];
      colorMdRt = img->pxl[offsetMd + offset];
      colorLoRt = img->pxl[offsetLo + offset];

      for(x = xBeg; x <= xEnd; x++) {

         idx = y * width + x;

         /**
          * Calculate horizontal edge flow
          *   1  2  1
          *   0  0  0
          *  -1 -2 -1
          */
         hMag  =  colorHiLf;
         hMag += (colorHiMd << 1);
         hMag +=  colorHiRt;
         hMag -=  colorLoLf;
         hMag -= (colorLoMd << 1);
         hMag -=  colorLoRt;

         /**
          * Calculate vertical edge flow
          *  -1  0  1
          *  -2  0  2
          *  -1  0  1
          */
         vMag  =  colorHiRt;
         vMag += (colorMdRt << 1);
         vMag +=  colorLoRt;
         vMag -=  colorHiLf;
         vMag -= (colorMdLf << 1);
         vMag -=  colorLoLf;

         /**
          * Calculate "slash" edge flow
          *  -2 -1  0
          *  -1  0  1
          *   0  1  2
          */
         sMag  =  colorLoMd;
         sMag += (colorLoRt << 1);
         sMag +=  colorMdRt;
         sMag -=  colorHiMd;
         sMag -= (colorHiLf << 1);
         sMag -=  colorMdLf;

         /**
          * Calculate "backslash" edge flow
          *   0  1  2
          *  -1  0  1
          *  -2 -1  0
          */
         bMag  =  colorMdLf;
         bMag += (colorLoLf << 1);
         bMag +=  colorLoMd;
         bMag -=  colorMdRt;
         bMag -= (colorHiRt << 1);
         bMag -=  colorHiMd;

         /**
          * If implementing these operations using MMX, can load 2
          * registers with 4 doubleword values and subtract (PSUBD).
          */

         sobelCache->hDir[idx] = hMag;
         sobelCache->vDir[idx] = vMag;
         sobelCache->sDir[idx] = sMag;
         sobelCache->bDir[idx] = bMag;

         colorHiLf = colorHiMd;
         colorMdLf = colorMdMd;
         colorLoLf = colorLoMd;

         colorHiMd = colorHiRt;
         colorMdMd = colorMdRt;
         colorLoMd = colorLoRt;

         offset += bytesPerPixel;

         colorHiRt = img->pxl[offsetHi + offset];
         colorMdRt = img->pxl[offsetMd + offset];
         colorLoRt = img->pxl[offsetLo + offset];
      }
   }

   return DmtxPass;
}

/**
 *
 */
DmtxPassFail
dmtxBuildCrossingCache(DmtxEdgeCache *crossingCache, DmtxEdgeCache *sobelCache)
{
   /* In reality if this works we will skip this step entirely and instead just
    * add the crossings directly into the Hough cache without ever storing an
    * actual crossing cache. But let's see it working first before taking the
    * plunge. */

   int x, y;

   memset(crossingCache, 0x00, sizeof(DmtxEdgeCache));

   for(y = 0; y < 64; y++) {
      for(x = 0; x < 64; x++) {
         ;
      }
   }

   return DmtxPass;
}

/**
 *
 */
int
GetCompactOffset(int x, int y, int phiIdx, int extent)
{
   int offset, posMax, negMax;
   double scale;

   if(phiIdx < 64) {
      posMax = extent * (uCos128[phiIdx] + uSin128[phiIdx]);
      negMax = 0;
   }
   else {
      posMax = extent * uSin128[phiIdx];
      negMax = extent * uCos128[phiIdx];
   }

   assert(abs(posMax - negMax) > 0);
   scale = (double)extent / (posMax - negMax);

   offset = (int)((x * uCos128[phiIdx] + y * uSin128[phiIdx] - negMax) * scale + 0.5);

   return offset;
}

double
UncompactOffset(double compactedOffset, int phiIdx, int extent)
{
   double phiRad;
   double scale;
   double posMax, negMax;

   phiRad = M_PI * phiIdx / 128.0;

   if(phiIdx < 64) {
      posMax = extent * (cos(phiRad) + sin(phiRad));
      negMax = 0.0;
   }
   else {
      posMax = extent * sin(phiRad);
      negMax = extent * cos(phiRad);
   }

   assert(extent > 0);
   scale = (posMax - negMax) / extent;

   return (compactedOffset * scale) + negMax;

/* fixed point version:
   int posMax, negMax;
   double scale;

   if(phiIdx < 64) {
      posMax = extent * (uCos128[phiIdx] + uSin128[phiIdx]);
      negMax = 0;
   }
   else {
      posMax = extent * uSin128[phiIdx];
      negMax = extent * uCos128[phiIdx];
   }

   scale = (double)(posMax - negMax) / extent;

   return (d * scale + negMax)/1024.0;
*/
}

/**
 *
 *
 */
DmtxPassFail
dmtxBuildHoughCache(DmtxHoughCache *hough, DmtxEdgeCache *sobelCache)
{
   int idx, phi, d;
   int angleBase, imgExtent;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;

   hough->offExtent = HOUGH_D_EXTENT;
   hough->phiExtent = HOUGH_PHI_EXTENT;
   memset(hough->isMax, 0x01, sizeof(char) * HOUGH_D_EXTENT * HOUGH_PHI_EXTENT);
   memset(&(hough->mag), 0x00, sizeof(int) * HOUGH_D_EXTENT * HOUGH_PHI_EXTENT);

   imgExtent = hough->offExtent;
   angleBase = hough->phiExtent;

   xBeg = 2;
   xEnd = imgExtent - 3;
   yBeg = 2;
   yEnd = imgExtent - 3;

   for(y = yBeg; y <= yEnd; y++) {
      for(x = xBeg; x <= xEnd; x++) {

         idx = y * imgExtent + x;

         /* After finalizing algorithm, precalculate for 32x32 square:
          *   each product (x*uCos[phi]) and (y*uSin[phi]) (FAST) (8kB)
          *   or, map each (x,y,phi) to its (d,phi) location (FASTER) (262kB)
          *
          * That should provide a huge speedup.
          */
         if(abs(sobelCache->vDir[idx]) > 0) {
            for(phi = 0; phi < 16; phi++) {
               d = GetCompactOffset(x, y, phi, imgExtent);
               if(d == -1) continue;
               hough->mag[d * angleBase + phi] += abs(sobelCache->vDir[idx]);
            }
            for(phi = 112; phi < angleBase; phi++) {
               d = GetCompactOffset(x, y, phi, imgExtent);
               if(d == -1) continue;
               hough->mag[d * angleBase + phi] += abs(sobelCache->vDir[idx]);
            }
         }

         if(abs(sobelCache->bDir[idx]) > 0) {
            for(phi = 16; phi < 48; phi++) {
               d = GetCompactOffset(x, y, phi, imgExtent);
               if(d == -1) continue;
               hough->mag[d * angleBase + phi] += abs(sobelCache->bDir[idx]);
            }
         }

         if(abs(sobelCache->hDir[idx]) > 0) {
            for(phi = 48; phi < 80; phi++) {
               d = GetCompactOffset(x, y, phi, imgExtent);
               if(d == -1) continue;
               hough->mag[d * angleBase + phi] += abs(sobelCache->hDir[idx]);
            }
         }

         if(abs(sobelCache->sDir[idx]) > 0) {
            for(phi = 80; phi < 112; phi++) {
               d = GetCompactOffset(x, y, phi, imgExtent);
               if(d == -1) continue;
               hough->mag[d * angleBase + phi] += abs(sobelCache->sDir[idx]);
            }
         }
      }
   }

   return DmtxPass;
}

/**
 * Normalize hough quadrants in a very slow and inefficient way
 *
 */
DmtxPassFail
dmtxNormalizeHoughCache(DmtxHoughCache *hough, DmtxEdgeCache *sobelCache)
{
   int          pixelCount;
   int          i, idx, phi, d;
   unsigned int sFlowSum, bFlowSum, hFlowSum, vFlowSum;
   double       sFlowScale, bFlowScale, hFlowScale, vFlowScale;
   double       normScale, phiScale;

   hFlowSum = vFlowSum = sFlowSum = bFlowSum = 0;

   pixelCount = hough->offExtent * hough->offExtent;

   for(i = 0; i < pixelCount; i++) {
      hFlowSum += abs(sobelCache->hDir[i]);
      vFlowSum += abs(sobelCache->vDir[i]);
      sFlowSum += abs(sobelCache->sDir[i]);
      bFlowSum += abs(sobelCache->bDir[i]);
   }

   hFlowScale = (double)65536/hFlowSum;
   vFlowScale = (double)65536/vFlowSum;
   sFlowScale = (double)65536/sFlowSum;
   bFlowScale = (double)65536/bFlowSum;

   for(phi = 0; phi < 128; phi++) {
      for(d = 0; d < LOCAL_SIZE; d++) {
         idx = d * 128 + phi;
         if(phi < 16 || phi >= 112)
            normScale = vFlowScale;
         else if(phi < 48)
            normScale = bFlowScale;
         else if(phi < 80)
            normScale = hFlowScale;
         else if(phi < 112)
            normScale = sFlowScale;
         else
            normScale = 1;

         phiScale = ((phi < 32 || phi >= 96) ? abs(uCos128[phi]) : uSin128[phi])/1024.0;

         hough->mag[idx] = (int)(hough->mag[idx] * normScale * phiScale + 0.5);
      }
   }

   return DmtxPass;
}

/**
 *
 *
 */
void
dmtxMarkHoughMaxima(DmtxHoughCache *hough)
{
   int phi, offset;
   int idx0, idx1;

   /* Find local maxima for each angle */
   for(phi = 0; phi < hough->phiExtent; phi++) {
      for(offset = 0; offset < hough->offExtent - 1; offset++) {

         idx0 = offset * hough->phiExtent + phi;
         idx1 = idx0 + hough->phiExtent;

         if(hough->mag[idx0] == 0)
            hough->isMax[idx0] = 0;
         else if(hough->mag[idx0] > hough->mag[idx1])
            hough->isMax[idx1] = 0;
         else if(hough->mag[idx1] > hough->mag[idx0])
            hough->isMax[idx0] = 0;
      }
   }
}

/**
 *
 *
 */
void
AddToVanishPointSort(VanishPointSort *sort, VanishPointSum vanishSum)
{
   int i, startHere;
   int phiDiff;
   DmtxBoolean willGrow;

   /* Special case: first addition */
   if(sort->count == 0) {
      sort->vanishSum[sort->count++] = vanishSum;
      return;
   }

   willGrow = (sort->count < ANGLE_SORT_MAX_COUNT) ? DmtxTrue : DmtxFalse;
   startHere = sort->count - 1; /* Sort normally starts at weakest element */

   /* If sort already has entry for this angle (or close) then either:
    *   a) Overwrite the old one without shifting (if stronger), or
    *   b) Reject the new one completely (if weaker)
    */
   for(i = 0; i < sort->count; i++) {
      phiDiff = abs(vanishSum.phi - sort->vanishSum[i].phi);

      if(phiDiff < 8 || phiDiff > 119) {
         /* Similar angle is already represented with stronger magnitude */
         if(vanishSum.mag < sort->vanishSum[i].mag) {
            return;
         }
         /* Found similar-but-weaker angle that will be overwritten */
         else {
            sort->vanishSum[i] = vanishSum;
            willGrow = DmtxFalse; /* Non-growing re-sort required */
            startHere = i - 1;
            break;
         }
      }
   }

   /* Shift weak entries downward */
   for(i = startHere; i >= 0; i--) {
      if(vanishSum.mag > sort->vanishSum[i].mag) {
         if(i + 1 < ANGLE_SORT_MAX_COUNT)
            sort->vanishSum[i+1] = sort->vanishSum[i];
         sort->vanishSum[i] = vanishSum;
      }
   }

   /* Count changes only if shift occurs */
   if(willGrow == DmtxTrue)
      sort->count++;
}

/**
 *
 *
 */
VanishPointSort
dmtxFindVanishPoints(DmtxHoughCache *hough)
{
   int phi;
   VanishPointSort sort;

   memset(&sort, 0x00, sizeof(VanishPointSort));

   /* Add strongest line at each angle to sort */
   for(phi = 0; phi < hough->phiExtent; phi++)
      AddToVanishPointSort(&sort, GetAngleSumAtPhi(hough, phi));

   return sort;
}

/**
 *
 *
 */
void
AddToMaximaSort(HoughMaximaSort *sort, int maximaMag)
{
   int i;

   /* If new entry would be weakest (or only) one in list, then append */
   if(sort->count == 0 || maximaMag < sort->mag[sort->count - 1]) {
      if(sort->count + 1 < MAXIMA_SORT_MAX_COUNT)
         sort->mag[sort->count++] = maximaMag;
      return;
   }

   /* Otherwise shift the weaker entries downward */
   for(i = sort->count - 1; i >= 0; i--) {
      if(maximaMag > sort->mag[i]) {
         if(i + 1 < MAXIMA_SORT_MAX_COUNT)
            sort->mag[i+1] = sort->mag[i];
         sort->mag[i] = maximaMag;
      }
   }

   if(sort->count < MAXIMA_SORT_MAX_COUNT)
      sort->count++;
}

/**
 *
 *
 */
VanishPointSum
GetAngleSumAtPhi(DmtxHoughCache *hough, int phi)
{
   int offset, i;
   VanishPointSum vanishSum;
   HoughMaximaSort sort;

   memset(&sort, 0x00, sizeof(HoughMaximaSort));

   for(offset = 0; offset < hough->offExtent; offset++) {
      i = offset * hough->phiExtent + phi;
      if(hough->isMax[i])
         AddToMaximaSort(&sort, hough->mag[i]);
   }

   vanishSum.phi = phi;
   vanishSum.mag = 0;
   for(i = 0; i < 8; i++)
      vanishSum.mag += sort.mag[i];

   return vanishSum;
}

/**
 *
 *
 */
void
AddToTimingSort(DmtxTimingSort *sort, Timing timing)
{
   int i;

   if(timing.mag < 1.0) /* XXX or some minimum threshold */
      return;

   /* If new entry would be weakest (or only) one in list, then append */
   if(sort->count == 0 || timing.mag < sort->timing[sort->count - 1].mag) {
      if(sort->count + 1 < TIMING_SORT_MAX_COUNT)
         sort->timing[sort->count++] = timing;
      return;
   }

   /* Otherwise shift the weaker entries downward */
   for(i = sort->count - 1; i >= 0; i--) {
      if(timing.mag > sort->timing[i].mag) {
         if(i + 1 < TIMING_SORT_MAX_COUNT)
            sort->timing[i+1] = sort->timing[i];
         sort->timing[i] = timing;
      }
   }

   if(sort->count < TIMING_SORT_MAX_COUNT)
      sort->count++;
}

/**
 *
 *
 */
DmtxTimingSort
dmtxFindGridTiming(DmtxHoughCache *hough, VanishPointSort *vPoints)
{
   int x, y, fitMag, fitMax, fitOff, attempts, iter;
   int i, vSortIdx, phi;
   kiss_fftr_cfg   cfg = NULL;
   kiss_fft_scalar rin[NFFT];
   kiss_fft_cpx    sout[NFFT/2+1];
   kiss_fft_scalar mag[NFFT/2+1];
   int maxIdx;
   Timing timing;
   DmtxTimingSort timings;

   memset(&timings, 0x00, sizeof(DmtxTimingSort));

   for(vSortIdx = 0; vSortIdx < vPoints->count; vSortIdx++) {

      phi = vPoints->vanishSum[vSortIdx].phi;

      /* Load FFT input array */
      for(i = 0; i < NFFT; i++) {
         rin[i] = (i < 64) ? hough->mag[i * hough->phiExtent + phi] : 0;
      }

      /* Execute FFT */
      memset(sout, 0x00, sizeof(kiss_fft_cpx) * (NFFT/2 + 1));
      cfg = kiss_fftr_alloc(NFFT, 0, 0, 0);
      kiss_fftr(cfg, rin, sout);
      free(cfg);

      /* Select best result */
      maxIdx = NFFT/9-1;
      for(i = 0; i < NFFT/9-1; i++)
         mag[i] = 0.0;
      for(i = NFFT/9-1; i < NFFT/2+1; i++) {
         mag[i] = sout[i].r * sout[i].r + sout[i].i * sout[i].i;
         if(mag[i] > mag[maxIdx])
            maxIdx = i;
      }

      timing.phi = phi;
      timing.period = NFFT / (double)maxIdx;
      timing.mag = mag[maxIdx];

      /* Find best offset */
      fitOff = fitMax = 0;
      attempts = (int)timing.period + 1;
      for(x = 0; x < attempts; x++) {
         fitMag = 0;
         for(iter = 0; ; iter++) {
            y = x + (int)(iter * timing.period);
            if(y >= 64)
               break;
            fitMag += hough->mag[y * hough->phiExtent + timing.phi];
         }
         if(x == 0 || fitMag > fitMax) {
            fitMax = fitMag;
            fitOff = x;
         }
      }
      timing.shift = fitOff;

      AddToTimingSort(&timings, timing);
   }

   return timings;
}

/**
 *
 *
 */
DmtxRay2
HoughCompactToRay(int phi, double d)
{
   double phiRad;
   double dScaled;
   DmtxRay2 rStart, rLine;

   memset(&rStart, 0x00, sizeof(DmtxRay2));
   memset(&rLine, 0x00, sizeof(DmtxRay2));

   rStart.p.X = rStart.p.Y = 0.0;

   phiRad = phi * M_PI/128.0;

   rStart.v.X = cos(phiRad);
   rStart.v.Y = sin(phiRad);

   rLine.v.X = -rStart.v.Y;
   rLine.v.Y = rStart.v.X;

   dScaled = UncompactOffset(d, phi, LOCAL_SIZE);

   dmtxPointAlongRay2(&(rLine.p), &rStart, dScaled);

   return rLine;
}

/**
 *
 *
 *
 */
DmtxPassFail
dmtxBuildGridFromTimings(AlignmentGrid *grid, Timing vp0, Timing vp1)
{
   RegionLines rl0, rl1, *flat, *steep;
   DmtxVector2 p00, p10, p11, p01;
   DmtxMatrix3 fit2raw, raw2fit, mScale;

   /* (1) -- later compare all possible combinations for strongest pair */
   rl0.timing = vp0;
   rl0.gridCount = (int)((64.0 - vp0.shift)/vp0.period);
   rl0.dA = vp0.shift;
   rl0.dB = vp0.shift + vp0.period * rl0.gridCount; /* doesn't work but whatever */

   rl1.timing = vp1;
   rl1.gridCount = (int)((64.0 - vp1.shift)/vp1.period);
   rl1.dA = vp1.shift;
   rl1.dB = vp1.shift + vp1.period * rl1.gridCount; /* doesn't work but whatever */

   /* flat[0] is the bottom flat line */
   /* flat[1] is the top flat line */
   /* steep[0] is the left steep line */
   /* steep[1] is the right line */

   /* Line with angle closest to horizontal is flatest */
   if(abs(64 - rl0.timing.phi) < abs(64 - rl1.timing.phi)) {
      flat = &rl0;
      steep = &rl1;
   }
   else {
      flat = &rl1;
      steep = &rl0;
   }

   flat->line[0] = HoughCompactToRay(flat->timing.phi, flat->dA);
   flat->line[1] = HoughCompactToRay(flat->timing.phi, flat->dB);

   if(steep->timing.phi < 64) {
      steep->line[0] = HoughCompactToRay(steep->timing.phi, steep->dA);
      steep->line[1] = HoughCompactToRay(steep->timing.phi, steep->dB);
   }
   else {
      steep->line[0] = HoughCompactToRay(steep->timing.phi, steep->dB);
      steep->line[1] = HoughCompactToRay(steep->timing.phi, steep->dA);
   }

   RETURN_IF_FAIL(dmtxRay2Intersect(&p00, &(flat->line[0]), &(steep->line[0])));
   RETURN_IF_FAIL(dmtxRay2Intersect(&p10, &(flat->line[0]), &(steep->line[1])));
   RETURN_IF_FAIL(dmtxRay2Intersect(&p11, &(flat->line[1]), &(steep->line[1])));
   RETURN_IF_FAIL(dmtxRay2Intersect(&p01, &(flat->line[1]), &(steep->line[0])));
   RETURN_IF_FAIL(RegionUpdateCorners(fit2raw, raw2fit, p00, p10, p11, p01));

   grid->rowCount = flat->gridCount;
   grid->colCount = steep->gridCount;

   /* raw2fit: Final transformation fits single origin module */
   dmtxMatrix3Identity(mScale);
   dmtxMatrix3Multiply(grid->raw2fitActive, raw2fit, mScale);

   /* fit2raw: Abstract away display nuances of multi_test application */
   dmtxMatrix3Identity(mScale);
   dmtxMatrix3Multiply(grid->fit2rawActive, mScale, fit2raw);

   return DmtxPass;
}

/**
 *
 *
 */
StripStats
GenStripPatternStats(unsigned char *strip, int stripLength, int startState, int contrast)
{
   int i, jumpAmount, jumpThreshold;
   int finderLow, finderHigh, timingLow, timingHigh;
   int surpriseCount, jumpCount, contrastSum;
   int newState, currentState;
   StripStats stats;

   assert(startState == MODULE_HIGH || startState == MODULE_LOW);

   jumpThreshold = (contrast*40)/100;
   finderLow = finderHigh = timingLow = timingHigh = 0;
   surpriseCount = jumpCount = contrastSum = 0;
   currentState = startState;
   memset(&stats, 0x00, sizeof(StripStats));

   for(i = 0; i < stripLength; i++) {

      if(i > 0) {
         jumpAmount = strip[i] - strip[i-1];

         /* Tally jump statistics if occurred */
         if(abs(jumpAmount) > jumpThreshold) {
            jumpCount++;
            contrastSum += abs(jumpAmount);
            newState = (jumpAmount > 0) ? MODULE_HIGH : MODULE_LOW;

            /* Surprise! We jumped but landed in the same state */
            if(newState == currentState)
               surpriseCount++;
            else
               currentState = newState;
         }
      }

      /* Increment appropriate finder pattern */
      if(currentState == MODULE_HIGH)
         finderHigh++;
      else
         finderLow++;

      /* Increment appropriate timing pattern */
      if(currentState ^ (i & 0x01))
         timingHigh++;
      else
         timingLow++;
   }

   stats.jumps = jumpCount;
   stats.surprises = surpriseCount;
   stats.finderErrors = (finderHigh < finderLow) ? finderHigh : finderLow;
   stats.timingErrors = (timingHigh < timingLow) ? timingHigh : timingLow;

   if(jumpCount > 0) {
      stats.contrast = (int)((double)contrastSum/jumpCount + 0.5);
      stats.finderBest = (finderHigh > finderLow) ? MODULE_HIGH : MODULE_LOW;
      stats.timingBest = (timingHigh > timingLow) ? MODULE_HIGH : MODULE_LOW;
   }
   else {
      stats.contrast = 0;
      stats.finderBest = MODULE_UNKNOWN;
      stats.timingBest = MODULE_UNKNOWN;
   }

   return stats;
}

/**
 *
 *
 */
DmtxPassFail
dmtxFindRegionWithinGrid(GridRegion *region, AlignmentGrid *grid, DmtxHoughCache *houghCache, DmtxDecode *dec, DmtxCallbacks *fn)
{
   int goodCount;
   int finderSides;
   DmtxDirection sideDir;
   DmtxBarType innerType, outerType;
   GridRegion regGrow;

   memset(&regGrow, 0x00, sizeof(GridRegion));

   regGrow.grid = *grid; /* Capture local copy of grid for tweaking */
   regGrow.x = regGrow.grid.colCount / 2;
   regGrow.y = regGrow.grid.rowCount / 2;
   regGrow.width = 2;
   regGrow.height = 2;
   regGrow.sizeIdx = DmtxUndefined;
   regGrow.onColor = regGrow.offColor = 0;
   regGrow.contrast = 20; /* low initial value */

   /* Assume the starting region will be far enough away from any side that
    * the expansion rules won't need to work at the very smallest sizes */

   /* Grow region */
   finderSides = DmtxDirNone;
   for(goodCount = 0, sideDir = DmtxDirDown; goodCount < 4; sideDir = RotateCW(sideDir)) {
      if(regGrow.width > 26 || regGrow.height > 26)
         return DmtxFail;

      innerType = TestSideForPattern(&regGrow, dec->image, sideDir, 0);
      outerType = TestSideForPattern(&regGrow, dec->image, sideDir, 1);

      /**
       * Make smarter ... maybe:
       * 1) Check that next-farther out strips are consistent (easy)
       * 2) Check that the colors are all consistent (not as easy)
       */

      if(innerType == DmtxBarNone || outerType == DmtxBarNone) {
         RegionExpand(&regGrow, sideDir, houghCache, fn);
         finderSides = DmtxDirNone;
         goodCount = 0;
      }
      else {
         if(innerType == DmtxBarFinder)
            finderSides |= sideDir;

         goodCount++;
      }

      fn->perimeterCallback(&regGrow, sideDir, innerType);
   }

   regGrow.finderSides = finderSides;
   *region = regGrow;

   return DmtxPass;
}

/**
 *
 *
 */
int dmtxReadModuleColor(DmtxImage *img, AlignmentGrid *grid, int symbolRow,
      int symbolCol, int colorPlane)
{
   int err;
   int i;
   int color, colorTmp;
   double sampleX[] = { 0.5, 0.4, 0.5, 0.6, 0.5 };
   double sampleY[] = { 0.5, 0.5, 0.4, 0.5, 0.6 };
   DmtxVector2 p;

   color = 0;
   for(i = 0; i < 5; i++) {

      p.X = (1.0/grid->colCount) * (symbolCol + sampleX[i]);
      p.Y = (1.0/grid->rowCount) * (symbolRow + sampleY[i]);

      dmtxMatrix3VMultiplyBy(&p, grid->fit2rawFull);

      /* Should use dmtxDecodeGetPixelValue() later to properly handle pixel skipping */
      err = dmtxImageGetPixelValue(img, p.X, p.Y, colorPlane, &colorTmp);
      if(err == DmtxFail)
         return 0;

      color += colorTmp;
   }

   return color/5;
}

/**
 *
 *
 */
DmtxBarType
TestSideForPattern(GridRegion *region, DmtxImage *img, DmtxDirection side, int offset)
{
   int i;
   int col, colBeg;
   int row, rowBeg;
   int extent;
   unsigned char colorStrip[26] = { 0 };
   StripStats *stats, statsHigh, statsLow;

   assert(region->width <= 26 && region->height <= 26);
   assert(side == DmtxDirUp || side == DmtxDirLeft ||
         side == DmtxDirDown || side == DmtxDirRight);

   /* Note opposite meaning (DmtxDirUp means "top", extent is horizontal) */
   extent = (side & DmtxDirVertical) ? region->width : region->height;
   if(extent < 3)
      return DmtxBarNone;

   switch(side) {
      case DmtxDirUp:
         colBeg = region->x;
         rowBeg = (region->y + region->height - 1) + offset;
         break;
      case DmtxDirLeft:
         colBeg = region->x - offset;
         rowBeg = region->y;
         break;
      case DmtxDirDown:
         colBeg = region->x;
         rowBeg = region->y - offset;
         break;
      case DmtxDirRight:
         colBeg = (region->x + region->width - 1) + offset;
         rowBeg = region->y;
         break;
      default:
         return DmtxUndefined;
   }

   /* Sample and hold colors at each module along both inner and outer edges */
   col = colBeg;
   row = rowBeg;

   for(i = 0; i < extent; i++) {
      colorStrip[i] = dmtxReadModuleColor(img, &(region->grid), row, col, 0);

      if(side & DmtxDirVertical)
         col++;
      else
         row++;
   }

   statsHigh = GenStripPatternStats(colorStrip, extent, MODULE_HIGH, region->contrast);
   statsLow = GenStripPatternStats(colorStrip, extent, MODULE_LOW, region->contrast);
   stats = (statsHigh.surprises > statsLow.surprises) ? &statsLow : &statsHigh;

   if(stats->contrast > region->contrast) {
      region->contrast = stats->contrast;
   }

   if(stats->finderErrors < stats->timingErrors) {
      if(stats->finderErrors < 2) {
         return DmtxBarFinder;
      }
   }
   else if(stats->finderErrors > stats->timingErrors) {
      if(stats->timingErrors < 2) {
         return DmtxBarTiming;
      }
   }

   return DmtxBarNone;
}

#define DmtxAlmostZero          0.000001

/**
 *
 *
 */
DmtxPassFail
Vector2Norm(DmtxVector2 *v)
{
   double mag;

   mag = dmtxVector2Mag(v);

   if(mag <= DmtxAlmostZero)
      return DmtxFail;

   dmtxVector2ScaleBy(v, 1/mag);

   return DmtxTrue;
}

/**
 *
 *
 */
DmtxPassFail
RayFromPoints(DmtxRay2 *ray, DmtxVector2 p0, DmtxVector2 p1)
{
   DmtxRay2 r;

   r.tMin = 0.0;
   r.tMax = 1.0;
   r.p = p0;

   dmtxVector2Sub(&r.v, &p1, &p0);
   RETURN_IF_FAIL(Vector2Norm(&r.v));

   *ray = r;

   return DmtxPass;
}

/**
 * Not a generic function. Notice that it uses fit2rawActive.
 *
 */
DmtxPassFail
dmtxRegionToSides(GridRegion *region, DmtxRegionSides *regionSides)
{
   DmtxVector2 p00, p10, p11, p01;
   DmtxRegionSides rs;

   p00.X = p01.X = region->x * (1.0/region->grid.colCount);
   p10.X = p11.X = (region->x + region->width) * (1.0/region->grid.colCount);
   p00.Y = p10.Y = region->y * (1.0/region->grid.rowCount);
   p01.Y = p11.Y = (region->y + region->height) * (1.0/region->grid.rowCount);

   dmtxMatrix3VMultiplyBy(&p00, region->grid.fit2rawActive);
   dmtxMatrix3VMultiplyBy(&p10, region->grid.fit2rawActive);
   dmtxMatrix3VMultiplyBy(&p11, region->grid.fit2rawActive);
   dmtxMatrix3VMultiplyBy(&p01, region->grid.fit2rawActive);

   RETURN_IF_FAIL(RayFromPoints(&rs.bottom, p00, p10));
   RETURN_IF_FAIL(RayFromPoints(&rs.right, p10, p11));
   RETURN_IF_FAIL(RayFromPoints(&rs.top, p11, p01));
   RETURN_IF_FAIL(RayFromPoints(&rs.left, p01, p00));

   *regionSides = rs;

   return DmtxPass;
}

/**
 *
 *
 */
DmtxHoughCompact
dmtxRayToHoughCompact(DmtxRay2 ray)
{
   double phiRad;
   DmtxHoughCompact houghCompact;

   if(ray.v.X > 0.0)
      phiRad = atan2(ray.v.Y, ray.v.X) + M_PI_2;
   else
      phiRad = atan2(-ray.v.Y, -ray.v.X) + M_PI_2;

   houghCompact.phi = (int)(phiRad/M_PI * 128.0 + 0.5);

   assert(houghCompact.phi >= 0);
   assert(houghCompact.phi <= 128);

   houghCompact.d = GetCompactOffset((int)(ray.p.X + 0.5), (int)(ray.p.Y + 0.5), houghCompact.phi, 64);

   return houghCompact;
}

/**
 *
 *
 */
DmtxBoolean
dmtxHoughNudgeStronger(DmtxHoughCache *houghCache, DmtxHoughCompact *houghCompact)
{
   return DmtxTrue;
}

/**
 *
 *
 */
DmtxPassFail
dmtxRegionUpdateFromSides(GridRegion *region, DmtxRegionSides regionSides)
{
   DmtxVector2 p00, p10, p11, p01;

   RETURN_IF_FAIL(dmtxRay2Intersect(&p00, &(regionSides.left), &(regionSides.bottom)));
   RETURN_IF_FAIL(dmtxRay2Intersect(&p10, &(regionSides.bottom), &(regionSides.right)));
   RETURN_IF_FAIL(dmtxRay2Intersect(&p11, &(regionSides.right), &(regionSides.top)));
   RETURN_IF_FAIL(dmtxRay2Intersect(&p01, &(regionSides.top), &(regionSides.left)));

   RETURN_IF_FAIL(RegionUpdateCorners(region->grid.fit2rawActive, region->grid.raw2fitActive, p00, p10, p11, p01));
   /* need to update fit2rawFull and raw2fitFull here too? */

   return DmtxPass;
}

/**
 *
 *
 */
DmtxPassFail
RegionExpand(GridRegion *region, DmtxDirection sideDir, DmtxHoughCache *houghCache, DmtxCallbacks *fn)
{
   DmtxRegionSides regionSides;
   DmtxRay2 *sideRay;
   DmtxHoughCompact sideHoughCompact;

   switch(sideDir) {
      case DmtxDirDown:
         region->y--;
         region->height++;
         sideRay = &(regionSides.bottom);
         break;
      case DmtxDirUp:
         region->height++;
         sideRay = &(regionSides.top);
         break;
      case DmtxDirLeft:
         region->x--;
         region->width++;
         sideRay = &(regionSides.left);
         break;
      case DmtxDirRight:
         region->width++;
         sideRay = &(regionSides.right);
         break;
      default:
         return DmtxFail;
   }

   /* Nudge expanded side for a better fit */
   if(gState.autoNudge == DmtxTrue)
   {
      RETURN_IF_FAIL(dmtxRegionToSides(region, &regionSides));
      sideHoughCompact = dmtxRayToHoughCompact(*sideRay);

      if(sideHoughCompact.d >= 0 && sideHoughCompact.d < 64)
      {
         fn->houghCompactCallback(sideHoughCompact, 0);

         /* If line is nudged to a stronger location, update region geometry */
         if(dmtxHoughNudgeStronger(houghCache, &sideHoughCompact) == DmtxTrue)
         {
            *sideRay = HoughCompactToRay(sideHoughCompact.phi, sideHoughCompact.d);
/*
            dmtxRegionUpdateFromSides(region, regionSides);
*/
         }
      }
   }

   return DmtxPass;
}

/**
 *
 *
 */
int
dmtxGetSizeIdx(int a, int b)
{
   int i, rows, cols;
   const int totalSymbolSizes = 30; /* is there a better way to determine this? */

   for(i = 0; i < totalSymbolSizes; i++) {
      rows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, i);
      cols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, i);

      if((rows == a && cols == b) || (rows == b && cols == a))
         return i;
   }

   return DmtxUndefined;
}

/**
 *
 *
 */
DmtxPassFail
RegionUpdateCorners(DmtxMatrix3 fit2raw, DmtxMatrix3 raw2fit, DmtxVector2 p00,
      DmtxVector2 p10, DmtxVector2 p11, DmtxVector2 p01)
{
/* double xMax, yMax; */
   double tx, ty, phi, shx, scx, scy, skx, sky;
   double dimOT, dimOR, dimTX, dimRX; /* , ratio; */
   DmtxVector2 vOT, vOR, vTX, vRX, vTmp;
   DmtxMatrix3 m, mtxy, mphi, mshx, mscx, mscy, mscxy, msky, mskx;

/* xMax = (double)(dmtxDecodeGetProp(dec, DmtxPropWidth) - 1);
   yMax = (double)(dmtxDecodeGetProp(dec, DmtxPropHeight) - 1);

   if(p00.X < 0.0 || p00.Y < 0.0 || p00.X > xMax || p00.Y > yMax ||
         p01.X < 0.0 || p01.Y < 0.0 || p01.X > xMax || p01.Y > yMax ||
         p10.X < 0.0 || p10.Y < 0.0 || p10.X > xMax || p10.Y > yMax)
      return DmtxFail; */
   dimOT = dmtxVector2Mag(dmtxVector2Sub(&vOT, &p01, &p00)); /* XXX could use MagSquared() */
   dimOR = dmtxVector2Mag(dmtxVector2Sub(&vOR, &p10, &p00));
   dimTX = dmtxVector2Mag(dmtxVector2Sub(&vTX, &p11, &p01));
   dimRX = dmtxVector2Mag(dmtxVector2Sub(&vRX, &p11, &p10));

   /* Verify that sides are reasonably long */
/* if(dimOT <= 8.0 || dimOR <= 8.0 || dimTX <= 8.0 || dimRX <= 8.0)
      return DmtxFail; */

   /* Verify that the 4 corners define a reasonably fat quadrilateral */
/* ratio = dimOT / dimRX;
   if(ratio <= 0.5 || ratio >= 2.0)
      return DmtxFail; */

/* ratio = dimOR / dimTX;
   if(ratio <= 0.5 || ratio >= 2.0)
      return DmtxFail; */

   /* Verify this is not a bowtie shape */
/*
   if(dmtxVector2Cross(&vOR, &vRX) <= 0.0 ||
         dmtxVector2Cross(&vOT, &vTX) >= 0.0)
      return DmtxFail; */

/* if(RightAngleTrueness(p00, p10, p11, M_PI_2) <= dec->squareDevn)
      return DmtxFail;
   if(RightAngleTrueness(p10, p11, p01, M_PI_2) <= dec->squareDevn)
      return DmtxFail; */

   /* Calculate values needed for transformations */
   tx = -1 * p00.X;
   ty = -1 * p00.Y;
   dmtxMatrix3Translate(mtxy, tx, ty);

   phi = atan2(vOT.X, vOT.Y);
   dmtxMatrix3Rotate(mphi, phi);
   dmtxMatrix3Multiply(m, mtxy, mphi);

   dmtxMatrix3VMultiply(&vTmp, &p10, m);
   shx = -vTmp.Y / vTmp.X;
   dmtxMatrix3Shear(mshx, 0.0, shx);
   dmtxMatrix3MultiplyBy(m, mshx);

   scx = 1.0/vTmp.X;
   dmtxMatrix3Scale(mscx, scx, 1.0);
   dmtxMatrix3MultiplyBy(m, mscx);

   dmtxMatrix3VMultiply(&vTmp, &p11, m);
   scy = 1.0/vTmp.Y;
   dmtxMatrix3Scale(mscy, 1.0, scy);
   dmtxMatrix3MultiplyBy(m, mscy);

   dmtxMatrix3VMultiply(&vTmp, &p11, m);
   skx = vTmp.X;
   dmtxMatrix3LineSkewSide(mskx, 1.0, skx, 1.0);
   dmtxMatrix3MultiplyBy(m, mskx);

   dmtxMatrix3VMultiply(&vTmp, &p01, m);
   sky = vTmp.Y;
   dmtxMatrix3LineSkewTop(msky, sky, 1.0, 1.0);
   dmtxMatrix3Multiply(raw2fit, m, msky);

   /* Create inverse matrix by reverse (avoid straight matrix inversion) */
   dmtxMatrix3LineSkewTopInv(msky, sky, 1.0, 1.0);
   dmtxMatrix3LineSkewSideInv(mskx, 1.0, skx, 1.0);
   dmtxMatrix3Multiply(m, msky, mskx);

   dmtxMatrix3Scale(mscxy, 1.0/scx, 1.0/scy);
   dmtxMatrix3MultiplyBy(m, mscxy);

   dmtxMatrix3Shear(mshx, 0.0, -shx);
   dmtxMatrix3MultiplyBy(m, mshx);

   dmtxMatrix3Rotate(mphi, -phi);
   dmtxMatrix3MultiplyBy(m, mphi);

   dmtxMatrix3Translate(mtxy, -tx, -ty);
   dmtxMatrix3Multiply(fit2raw, m, mtxy);

   return DmtxPass;
}

/**
 *
 *
 */
DmtxPassFail
dmtxDecodeSymbol(GridRegion *region, DmtxDecode *dec)
{
   static int prefix = 0;
   int onColor, offColor;
   DmtxVector2 p00, p10, p11, p01;
   DmtxRegion reg;
   DmtxMessage *msg;

   /* Since we now hold 2 adjacent timing bars, find colors */
   RETURN_IF_FAIL(GetOnOffColors(region, dec, &onColor, &offColor));

   p00.X = p01.X = region->x * (1.0/region->grid.colCount);
   p10.X = p11.X = (region->x + region->width) * (1.0/region->grid.colCount);
   p00.Y = p10.Y = region->y * (1.0/region->grid.rowCount);
   p01.Y = p11.Y = (region->y + region->height) * (1.0/region->grid.rowCount);

   dmtxMatrix3VMultiplyBy(&p00, region->grid.fit2rawFull);
   dmtxMatrix3VMultiplyBy(&p10, region->grid.fit2rawFull);
   dmtxMatrix3VMultiplyBy(&p11, region->grid.fit2rawFull);
   dmtxMatrix3VMultiplyBy(&p01, region->grid.fit2rawFull);

   /* Update DmtxRegion with detected corners */
   switch(region->finderSides) {
      case (DmtxDirLeft | DmtxDirDown):
         RETURN_IF_FAIL(dmtxRegionUpdateCorners(dec, &reg, p00, p10, p11, p01));
         break;
      case (DmtxDirDown | DmtxDirRight):
         RETURN_IF_FAIL(dmtxRegionUpdateCorners(dec, &reg, p10, p11, p01, p00));
         break;
      case (DmtxDirRight | DmtxDirUp):
         RETURN_IF_FAIL(dmtxRegionUpdateCorners(dec, &reg, p11, p01, p00, p10));
         break;
      case (DmtxDirUp | DmtxDirLeft):
         RETURN_IF_FAIL(dmtxRegionUpdateCorners(dec, &reg, p01, p00, p10, p11));
         break;
      default:
         return DmtxFail;
   }

   /* Populate old-style region */
   reg.flowBegin.plane = 0; /* or 1, or 2 (0 = red, 1 = green, etc...) */
   reg.onColor = onColor;
   reg.offColor = offColor;
   reg.sizeIdx = region->sizeIdx;
   reg.symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, region->sizeIdx);
   reg.symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, region->sizeIdx);
   reg.mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, region->sizeIdx);
   reg.mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, region->sizeIdx);

   msg = dmtxDecodeMatrixRegion(dec, &reg, DmtxUndefined);
   if(msg == NULL)
      return DmtxFail;

   fprintf(stdout, "%d: ", prefix);
   prefix = (prefix == 9) ? 0 : prefix + 1;
   fwrite(msg->output, sizeof(char), msg->outputIdx, stdout);
   fputc('\n', stdout);
   fflush(stdout);

   return DmtxPass;
}

/**
 *
 *
 */
DmtxPassFail
GetOnOffColors(GridRegion *region, const DmtxDecode *dec, int *onColor, int *offColor)
{
   int colBeg, rowBeg;
   DmtxDirection d0, d1;
   ColorTally t0, t1;

   /* add assertion to guarantee 2 adjancent timing bars */

   /* Start tally at intersection of timing bars */
   colBeg = (region->finderSides & DmtxDirLeft) ? region->x + region->width - 1 : region->x;
   rowBeg = (region->finderSides & DmtxDirDown) ? region->y + region->height - 1 : region->y;

   switch(region->finderSides) {
      case (DmtxDirLeft | DmtxDirDown):
         d0 = DmtxDirLeft;
         d1 = DmtxDirDown;
         break;
      case (DmtxDirDown | DmtxDirRight):
         d0 = DmtxDirDown;
         d1 = DmtxDirRight;
         break;
      case (DmtxDirRight | DmtxDirUp):
         d0 = DmtxDirRight;
         d1 = DmtxDirUp;
         break;
      case (DmtxDirUp | DmtxDirLeft):
         d0 = DmtxDirUp;
         d1 = DmtxDirLeft;
         break;
      default:
         return DmtxFail;
   }

   t0 = GetTimingColors(region, dec, colBeg, rowBeg, d0);
   t1 = GetTimingColors(region, dec, colBeg, rowBeg, d1);

   if(t0.evnCount + t1.evnCount == 0 || t0.oddCount + t1.oddCount == 0)
      return DmtxFail;

   *onColor = (t0.oddColor + t1.oddColor)/(t0.oddCount + t1.oddCount);
   *offColor = (t0.evnColor + t1.evnColor)/(t0.evnCount + t1.evnCount);

   return DmtxPass;
}

/**
 *
 *
 */
ColorTally
GetTimingColors(GridRegion *region, const DmtxDecode *dec, int colBeg, int rowBeg,
      DmtxDirection dir)
{
   int i, row, col, extent;
   int sample;
   ColorTally colors;

   colors.evnColor = 0;
   colors.evnCount = 0;

   colors.oddColor = 0;
   colors.oddCount = 0;

   /* Note opposite meaning (DmtxDirUp means "top", extent is horizontal) */
   extent = (dir & DmtxDirVertical) ? region->width : region->height;

   col = colBeg;
   row = rowBeg;
   for(i = 0; i < extent; i++) {
      sample = dmtxReadModuleColor(dec->image, &(region->grid), row, col, 0);

      if(i & 0x01) {
         colors.oddColor += sample;
         colors.oddCount++;
      }
      else {
         colors.evnColor += sample;
         colors.evnCount++;
      }

      switch(dir) {
         case DmtxDirUp:
            row++;
            break;
         case DmtxDirLeft:
            col--;
            break;
         case DmtxDirDown:
            row--;
            break;
         case DmtxDirRight:
            col++;
            break;
         default:
            return colors; /* XXX should be an error condition */
      }
   }

    /* consider having PerimeterEdgeTest() call this function when ready? */

   return colors;
}
