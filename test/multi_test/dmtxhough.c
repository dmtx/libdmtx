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

#include <string.h>
#include <assert.h>
#include <math.h>
#include "multi_test.h"

#define RETURN_FAIL_IF(C) \
   if(C) { \
      HoughGridDestroy(&(dec->houghGrid)); \
      return DmtxFail; \
   }

/**
 *
 *
 */
DmtxPassFail
HoughGridPopulate(DmtxDecode2 *dec)
{
   int row, col, idx;
   DmtxHoughLocal *line, *maxima, *vanish;

   dec->houghGrid = (DmtxHoughGrid *)calloc(1, sizeof(DmtxHoughGrid));
   if(dec->houghGrid == NULL)
      return DmtxFail;

   dec->houghGrid->rows = 1;
   dec->houghGrid->cols = 1;
   dec->houghGrid->count = dec->houghGrid->rows * dec->houghGrid->cols;
   dec->houghGrid->line = (DmtxHoughLocal *)calloc(dec->houghGrid->count, sizeof(DmtxHoughLocal));
   dec->houghGrid->maxima = (DmtxHoughLocal *)calloc(dec->houghGrid->count, sizeof(DmtxHoughLocal));
   dec->houghGrid->vanish = (DmtxHoughLocal *)calloc(dec->houghGrid->count, sizeof(DmtxHoughLocal));

   RETURN_FAIL_IF(dec->houghGrid->line == NULL ||
         dec->houghGrid->maxima == NULL || dec->houghGrid->vanish == NULL);

   for(row = 0; row < dec->houghGrid->rows; row++)
   {
      for(col = 0; col < dec->houghGrid->cols; col++)
      {
         idx = 0; /* will eventually be [hCol * width + hRol]; */

         line = &(dec->houghGrid->line[idx]);
         maxima = &(dec->houghGrid->maxima[idx]);
         vanish = &(dec->houghGrid->vanish[idx]);

         RETURN_FAIL_IF(LineHoughAccumulate(line, dec) == DmtxFail);
         dec->fn.dmtxHoughLocalCallback(line, 0);

         RETURN_FAIL_IF(MaximaHoughAccumulate(maxima, line, dec) == DmtxFail);
         dec->fn.dmtxHoughLocalCallback(maxima, 1);

         RETURN_FAIL_IF(VanishHoughAccumulate(vanish, maxima) == DmtxFail);
         dec->fn.dmtxHoughLocalCallback(vanish, 2);
      }
   }

   return DmtxPass;
}

#undef RETURN_FAIL_IF

/**
 *
 *
 */
DmtxPassFail
HoughGridDestroy(DmtxHoughGrid **grid)
{
   if(grid == NULL || *grid == NULL)
      return DmtxFail;

   if((*grid)->vanish != NULL)
      free((*grid)->vanish);

   if((*grid)->maxima != NULL)
      free((*grid)->maxima);

   if((*grid)->line != NULL)
      free((*grid)->line);

   free(*grid);
   *grid = NULL;

   return DmtxPass;
}

/**
 *
 *
 */
DmtxPassFail
LineHoughAccumulate(DmtxHoughLocal *lhRegion, DmtxDecode2 *dec)
{
   int rRow, rCol;
   int iRow, iCol;
   int iWidth, iHeight;
   int phi;
   ZeroCrossing vvZXing, vbZXing, hbZXing, hhZXing, hsZXing, vsZXing;
   DmtxPassFail vvPassFail, vbPassFail, hbPassFail, hhPassFail, hsPassFail, vsPassFail;

   memset(lhRegion, 0x00, sizeof(DmtxHoughLocal));

   /* Global coordinate system */
   iWidth = dmtxImageGetProp(dec->image, DmtxPropWidth);
   iHeight = dmtxImageGetProp(dec->image, DmtxPropHeight);

   lhRegion->xOrigin = gState.localOffsetX;
   lhRegion->yOrigin = gState.localOffsetY;

   /* calculate dOffset ? */

   for(rRow = 0; rRow < 64; rRow++)
   {
      iRow = lhRegion->yOrigin + rRow;

      if(iRow >= iHeight)
         continue;

      for(rCol = 0; rCol < 64; rCol++)
      {
         iCol = lhRegion->xOrigin + rCol;

         if(iCol >= iWidth)
            continue;

         vvZXing = GetZeroCrossing(dec->vvAccel, iCol, iRow, &vvPassFail);
         vbZXing = GetZeroCrossing(dec->vbAccel, iCol, iRow, &vbPassFail);
         hbZXing = GetZeroCrossing(dec->hbAccel, iCol, iRow, &hbPassFail);
         hhZXing = GetZeroCrossing(dec->hhAccel, iCol, iRow, &hhPassFail);
         hsZXing = GetZeroCrossing(dec->hsAccel, iCol, iRow, &hsPassFail);
         vsZXing = GetZeroCrossing(dec->vsAccel, iCol, iRow, &vsPassFail);

         if(gState.displayEdge == 1 && vvPassFail == DmtxPass)
            dec->fn.zeroCrossingCallback(vvZXing, 0);
         else if(gState.displayEdge == 2 && vbPassFail == DmtxPass)
            dec->fn.zeroCrossingCallback(vbZXing, 0);
         else if(gState.displayEdge == 3 && hbPassFail == DmtxPass)
            dec->fn.zeroCrossingCallback(hbZXing, 0);
         else if(gState.displayEdge == 4 && hhPassFail == DmtxPass)
            dec->fn.zeroCrossingCallback(hhZXing, 0);
         else if(gState.displayEdge == 5 && hsPassFail == DmtxPass)
            dec->fn.zeroCrossingCallback(hsZXing, 0);
         else if(gState.displayEdge == 6 && vsPassFail == DmtxPass)
            dec->fn.zeroCrossingCallback(vsZXing, 0);

         if(vvZXing.mag > 0 && vvPassFail == DmtxPass)
         {
            for(phi = 0; phi < 16; phi++)
               HoughLocalAccumulateEdge(lhRegion, phi, vvZXing);
            for(phi = 112; phi < 128; phi++)
               HoughLocalAccumulateEdge(lhRegion, phi, vvZXing);
         }

         if(vbZXing.mag > 0 && vbPassFail == DmtxPass)
            for(phi = 16; phi < 32; phi++)
               HoughLocalAccumulateEdge(lhRegion, phi, vbZXing);

         if(hbZXing.mag > 0 && hbPassFail == DmtxPass)
            for(phi = 32; phi < 48; phi++)
               HoughLocalAccumulateEdge(lhRegion, phi, hbZXing);

         if(hhZXing.mag > 0 && hhPassFail == DmtxPass)
            for(phi = 48; phi < 80; phi++)
               HoughLocalAccumulateEdge(lhRegion, phi, hhZXing);

         if(hsZXing.mag > 0 && hsPassFail == DmtxPass)
            for(phi = 80; phi < 96; phi++)
               HoughLocalAccumulateEdge(lhRegion, phi, hsZXing);

         if(vsZXing.mag > 0 && vsPassFail == DmtxPass)
            for(phi = 96; phi < 112; phi++)
               HoughLocalAccumulateEdge(lhRegion, phi, vsZXing);
      }
   }

   return DmtxPass;
}

/**
 *
 *
 */
DmtxPassFail
MaximaHoughAccumulate(DmtxHoughLocal *mhRegion, DmtxHoughLocal *lhRegion, DmtxDecode2 *dec)
{
   int rRow, rCol;
   int phi, phiMax;
   int d, dMax;
   int val, valMax;

   for(rRow = 0; rRow < 64; rRow++)
   {
      for(rCol = 0; rCol < 64; rCol++)
      {
         dMax = DmtxUndefined;
         phiMax = DmtxUndefined;
         valMax = 0;

         for(phi = 0; phi < 128; phi++)
         {
            d = (int)(HoughGetLocalOffset(rCol, rRow, phi) + 0.5);

            val = lhRegion->bucket[d][phi];
            if(val > valMax)
            {
               dMax = d;
               phiMax = phi;
               valMax = val;
            }
         }

         if(dMax != DmtxUndefined && phiMax != DmtxUndefined)
         {
            mhRegion->bucket[dMax][phiMax] += 1;
         }
      }
   }

   return DmtxPass;
}

/**
 *
 *
 */
DmtxPassFail
VanishHoughAccumulate(DmtxHoughLocal *vhRegion, DmtxHoughLocal *lhRegion)
{
   int lhRow, lhCol;
   int phi, d, i;
   int val, valCompare[8], valMin;

   for(lhRow = 0; lhRow < 64; lhRow++)
   {
      for(lhCol = 0; lhCol < 128; lhCol++)
      {
         /* XXX later be sure to flip d in comparisons across 0/127 boundary */
         /* XXX this actually overextends array boundaries I but don't care yet */
         val = lhRegion->bucket[lhRow][lhCol];
         valCompare[0] = lhRegion->bucket[lhRow+1][lhCol-1];
         valCompare[1] = lhRegion->bucket[lhRow+1][lhCol  ];
         valCompare[2] = lhRegion->bucket[lhRow+1][lhCol+1];
         valCompare[3] = lhRegion->bucket[lhRow  ][lhCol-1];
         valCompare[4] = lhRegion->bucket[lhRow  ][lhCol+1];
         valCompare[5] = lhRegion->bucket[lhRow-1][lhCol-1];
         valCompare[6] = lhRegion->bucket[lhRow-1][lhCol  ];
         valCompare[7] = lhRegion->bucket[lhRow-1][lhCol+1];

         valMin = val;
         for(i = 0; i < 8; i++)
         {
            if(valCompare[i] >= val)
            {
               val = DmtxUndefined;
               break;
            }
            if(valCompare[i] < valMin)
               valMin = valCompare[i];
         }
         if(val == DmtxUndefined)
            continue;

         for(phi = 0; phi < 128; phi++)
         {
            d = GetVanishBucket(phi, lhCol, lhRow);
            if(d == DmtxUndefined)
               continue;
            vhRegion->bucket[d][phi] += val;
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
GetVanishBucket(int phiBucket, int phiCompare, int dCompare)
{
   int bucket;
   int phiDelta;
   double d, u, x;
   double bucketRad, phiDeltaRad, phiCompareRad;
   double bucketF;

   if(phiBucket == phiCompare)
      return 32; /* Infinity */

   phiDelta = phiBucket - phiCompare;
   if(phiDelta < -64)
      phiDelta += 128;
   else if(phiDelta > 64)
      phiDelta -= 128;

   if(abs(phiDelta) > 32)
      return DmtxUndefined; /* Too far from parallel */

   phiCompareRad = phiCompare * (M_PI/128.0);
   phiDeltaRad = phiDelta * (M_PI/128.0);

   d = UncompactOffset(dCompare, phiCompare, 64);
   u = 32.0 * (cos(phiCompareRad) + sin(phiCompareRad));
   x = fabs((d - u)/sin(phiDeltaRad));

   if(x < 0.0001)
      return DmtxUndefined;

   bucketRad = atan(32.0/x);
   assert(bucketRad > 0.0);

   /* map 0 -> pi/2 to 0 -> 64 */
   bucketF = bucketRad * (64.0/M_PI);
   bucketF *= (bucketF + 30.0)/62.0;
   bucket = (int)bucketF;

   if(phiDelta * (d - u) < 0.0)
      bucket = -bucket;

   bucket += 32;

   if(bucket < 0)
      bucket = DmtxUndefined;
   else if(bucket > 63)
      bucket = DmtxUndefined;

   return bucket;
}

/**
 *
 *
 */
ZeroCrossing
GetZeroCrossing(DmtxValueGrid *accel, int iCol, int iRow, DmtxPassFail *passFail)
{
   int aInc, aIdx, aIdxNext;
   int aRow, aCol;
   int aWidth, aHeight;
   int aHere, aNext, aPrev;
   double smidge;
   const ZeroCrossing emptyEdge = { 0, 0.0, 0.0 };
   ZeroCrossing edge = emptyEdge;
   DmtxPassFail childPassFail;

   aWidth = dmtxValueGridGetWidth(accel);
   aHeight = dmtxValueGridGetHeight(accel);

   /* XXX add better bounds checking of aIdxNext now that we're comparing diagonals */

   switch(accel->type) {
      case DmtxEdgeVertical:
         aRow = iRow - 1;
         aCol = iCol - 2;
         aInc = 1;
         break;

      case DmtxEdgeHorizontal:
         aRow = iRow - 2;
         aCol = iCol - 1;
         aInc = aWidth;
         break;

      default:
         *passFail = DmtxFail;
         return emptyEdge; /* Fail: Illegal edge direction */
   }

   aIdx = aRow * aWidth + aCol;
   aIdxNext = aIdx + aInc;

   aHere = accel->value[aIdx];
   aNext = accel->value[aIdxNext];

   if(OPPOSITE_SIGNS(aHere, aNext))
   {
      /* Zero crossing: Neighbors with opposite signs [-10,+10] */
      smidge = abs(aHere/(aHere - aNext));
      edge = SetZeroCrossingFromIndex(accel, aCol, aRow, smidge, &childPassFail);
   }
   else if(aHere == 0 && aNext != 0)
   {
      if((accel->type == DmtxEdgeVertical && aCol == 0) ||
            (accel->type == DmtxEdgeHorizontal && aRow == 0))
      {
         /* No previous value for comparison (beginning of row/col) */
         *passFail = DmtxFail;
         return emptyEdge;
      }

      aPrev = accel->value[aIdx-aInc];
      if(OPPOSITE_SIGNS(aPrev, aNext))
      {
         /* Zero crossing: Opposite signs separated by zero [-10,0,+10] */
         edge = SetZeroCrossingFromIndex(accel, aCol, aRow, 0.0, &childPassFail);
      }
   }

   *passFail = childPassFail;
   return edge;
}

/**
 * 0 < smidge < 1
 *
 */
ZeroCrossing
SetZeroCrossingFromIndex(DmtxValueGrid *accel, int aCol, int aRow, double smidge, DmtxPassFail *passFail)
{
   int sCol, sRow, sIdx;
   const ZeroCrossing emptyEdge = { 0, 0.0, 0.0 };
   ZeroCrossing edge;
   DmtxValueGrid *sobel = accel->ref;

   switch(accel->type) {
      case DmtxEdgeVertical:
         edge.x = (double)aCol + 2.0 + smidge;
         edge.y = (double)aRow + 1.5;
         sRow = aRow;
         sCol = aCol + 1;
         break;

      case DmtxEdgeHorizontal:
         edge.x = (double)aCol + 1.5;
         edge.y = (double)aRow + 2.0 + smidge;
         sRow = aRow + 1;
         sCol = aCol;
         break;

      default:
         /* Illegal edge type */
         *passFail = DmtxFail;
         return emptyEdge;
   }

   /* Fail:  Sobel location out of bounds */
   if(sCol < 0 || sCol >= sobel->width || sRow < 0 || sRow >= sobel->height)
   {
      *passFail = DmtxFail;
      return emptyEdge;
   }

   /* Use Sobel value that falls directly between 2 accel locations */
   sIdx = sRow * dmtxValueGridGetWidth(sobel) + sCol;
   edge.mag = abs(sobel->value[sIdx]);

   *passFail = DmtxPass;
   return edge;
}

/**
 *
 *
 */
DmtxPassFail
HoughLocalAccumulateEdge(DmtxHoughLocal *line, int phi, ZeroCrossing edge)
{
   double d;
   int dInt;

   d = HoughGetLocalOffset(edge.x - line->xOrigin, edge.y - line->yOrigin, phi);
   dInt = (int)d;

if(dInt > 63)
   return DmtxFail;

   assert(dInt >= 0);
   assert(dInt < 64);
   assert(phi >= 0 && phi < 128);

   line->bucket[dInt][phi] += edge.mag;

   return DmtxPass;
}

/**
 *
 *
 */
double
HoughGetLocalOffset(double xLoc, double yLoc, int phi)
{
   double phiRad, sinPhi, cosPhi;
   double scale, d;

   phiRad = (phi * M_PI)/128.0;
   sinPhi = sin(phiRad);
   cosPhi = cos(phiRad);

   if(phi <= 64)
   {
      scale = 1.0 / (sinPhi + cosPhi);
      d = (xLoc * cosPhi + yLoc * sinPhi) * scale;
   }
   else
   {
      scale = 1.0 / (sinPhi - cosPhi);
      d = ((xLoc * cosPhi + yLoc * sinPhi) - (cosPhi * 64.0)) * scale;
   }

   return d;
}
