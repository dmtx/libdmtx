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

         vvZXing = GetZeroCrossing(dec->vvAccel, iCol, iRow);
         vbZXing = GetZeroCrossing(dec->vbAccel, iCol, iRow);
         hbZXing = GetZeroCrossing(dec->hbAccel, iCol, iRow);
         hhZXing = GetZeroCrossing(dec->hhAccel, iCol, iRow);
         hsZXing = GetZeroCrossing(dec->hsAccel, iCol, iRow);
         vsZXing = GetZeroCrossing(dec->vsAccel, iCol, iRow);

         if(vvZXing.mag > 0)
         {
            if(gState.displayEdge == 1)
               dec->fn.zeroCrossingCallback(vvZXing, 0);

            for(phi = 0; phi < 16; phi++)
               HoughLocalAccumulateEdge(lhRegion, phi, vvZXing);
            for(phi = 112; phi < 128; phi++)
               HoughLocalAccumulateEdge(lhRegion, phi, vvZXing);
         }

         if(vbZXing.mag > 0)
         {
            if(gState.displayEdge == 2)
               dec->fn.zeroCrossingCallback(vbZXing, 0);

            for(phi = 16; phi < 32; phi++)
               HoughLocalAccumulateEdge(lhRegion, phi, vbZXing);
         }

         if(hbZXing.mag > 0)
         {
            if(gState.displayEdge == 3)
               dec->fn.zeroCrossingCallback(hbZXing, 0);

            for(phi = 32; phi < 48; phi++)
               HoughLocalAccumulateEdge(lhRegion, phi, hbZXing);
         }

         if(hhZXing.mag > 0)
         {
            if(gState.displayEdge == 4)
               dec->fn.zeroCrossingCallback(hhZXing, 0);

            for(phi = 48; phi < 80; phi++)
               HoughLocalAccumulateEdge(lhRegion, phi, hhZXing);
         }

         if(hsZXing.mag > 0)
         {
            if(gState.displayEdge == 5)
               dec->fn.zeroCrossingCallback(hsZXing, 0);

            for(phi = 80; phi < 96; phi++)
               HoughLocalAccumulateEdge(lhRegion, phi, hsZXing);
         }

         if(vsZXing.mag > 0)
         {
            if(gState.displayEdge == 6)
               dec->fn.zeroCrossingCallback(vsZXing, 0);

            for(phi = 96; phi < 112; phi++)
               HoughLocalAccumulateEdge(lhRegion, phi, vsZXing);
         }
      }
   }

   return DmtxPass;
}

/**
 *
 *
 *
 */
DmtxPassFail
MaximaHoughAccumulate(DmtxHoughLocal *mhRegion, DmtxHoughLocal *lhRegion, DmtxDecode2 *dec)
{
   int phi, d;
   int rRow, rCol;
   int iRow, iCol;
   int iWidth, iHeight;
   ZeroCrossing vvZXing, vbZXing, hbZXing, hhZXing, hsZXing, vsZXing;
   DmtxHoughLocal tmpHough;

   /* XXX Neither of the following DmtxHoughLocal initializations are also
          initializing the non-bucket portions */

   memset(mhRegion, 0x00, sizeof(DmtxHoughLocal));

   /* Capture first filter pass in tmpHough */
   for(phi = 0; phi < 128; phi++)
      for(d = 0; d < 64; d++)
         tmpHough.bucket[d][phi] = GetMaximaWeight(lhRegion, phi, d);

*mhRegion = tmpHough;
return DmtxPass;

   /* Global coordinate system */
   iWidth = dmtxImageGetProp(dec->image, DmtxPropWidth);
   iHeight = dmtxImageGetProp(dec->image, DmtxPropHeight);

   /* XXX this is kinda weird ... shouldn't this be set before now? */
   tmpHough.xOrigin = lhRegion->xOrigin = gState.localOffsetX;
   tmpHough.yOrigin = lhRegion->yOrigin = gState.localOffsetY;

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

         vvZXing = GetZeroCrossing(dec->vvAccel, iCol, iRow);
         vbZXing = GetZeroCrossing(dec->vbAccel, iCol, iRow);
         hbZXing = GetZeroCrossing(dec->hbAccel, iCol, iRow);
         hhZXing = GetZeroCrossing(dec->hhAccel, iCol, iRow);
         hsZXing = GetZeroCrossing(dec->hsAccel, iCol, iRow);
         vsZXing = GetZeroCrossing(dec->vsAccel, iCol, iRow);

         InstantRunoff(mhRegion, &tmpHough, &vvZXing, DmtxEdgeVertical);
         InstantRunoff(mhRegion, &tmpHough, &vbZXing, DmtxEdgeBackslash);
         InstantRunoff(mhRegion, &tmpHough, &hbZXing, DmtxEdgeBackslash);
         InstantRunoff(mhRegion, &tmpHough, &hhZXing, DmtxEdgeHorizontal);
         InstantRunoff(mhRegion, &tmpHough, &hsZXing, DmtxEdgeSlash);
         InstantRunoff(mhRegion, &tmpHough, &vsZXing, DmtxEdgeSlash);
      }
   }

   return DmtxPass;
}

/**
 *
 *
 */
void
InstantRunoff(DmtxHoughLocal *maxLineHough, DmtxHoughLocal *lineHough,
      ZeroCrossing *zXing, DmtxEdgeType edgeType)
{
   int x, y;
   int d, phi, val;
   DmtxHoughBucket best = { DmtxUndefined, DmtxUndefined, 0 };

   if(zXing->mag == 0)
      return;

   x = (int)(zXing->x - lineHough->xOrigin + 0.5);
   y = (int)(zXing->y - lineHough->yOrigin + 0.5);

   for(phi = 0; phi < 128; phi++)
   {
      d = HoughGetLocalOffset(x, y, phi);
      if(d < 0 || d > 63)
         continue;

      val = lineHough->bucket[d][phi];

      if(val > best.val)
      {
         best.phi = phi;
         best.d = d;
         best.val = val;
      }
   }

   if(best.phi != DmtxUndefined && best.d != DmtxUndefined)
      maxLineHough->bucket[best.d][best.phi] += best.val;
}

/**
 *
 *
 */
int
GetMaximaWeight(DmtxHoughLocal *line, int phi, int d)
{
   int val, valDn, valUp, valDnDn, valUpUp;
   int weight;

   val = line->bucket[d][phi];
   valDn = (d >= 1) ? line->bucket[d - 1][phi] : 0;
   valUp = (d <= 62) ? line->bucket[d + 1][phi] : 0;

   /* Line is outranked by immediate neigbor in same direction (not a maxima) */
   if(valDn > val || valUp > val)
      return 0;

   valDnDn = (d >= 2) ? line->bucket[d - 2][phi] : 0;
   valUpUp = (d <= 61) ? line->bucket[d + 2][phi] : 0;

   weight = (6 * val) - 2 * (valUp + valDn) - (valUpUp + valDnDn);

   return (weight > 0) ? weight : 0;
}

/**
 *
 *
 */
DmtxPassFail
VanishHoughAccumulate(DmtxHoughLocal *vanish, DmtxHoughLocal *line)
{
   int i, d, phi, val;
   int dLine, phiLine;
   int phi128, phiBeg, phiEnd;
   int dPrev;

   for(dLine = 0; dLine < 64; dLine++)
   {
      for(phiLine = 0; phiLine < 128; phiLine++)
      {
         val = line->bucket[dLine][phiLine];
         if(val == 0)
            continue;

         phiBeg = phiLine - 16;
         phiEnd = phiLine + 16;
         dPrev = DmtxUndefined;

         for(phi = phiBeg; phi <= phiEnd; phi++)
         {
            phi128 = ((phi + 128) & 0x7f);

            d = GetVanishBucket(phi128, phiLine, dLine);
            if(d == DmtxUndefined)
               continue; /* break instead ? */

            /* Flip current d value if in opposite range of phiLine */
            if(phi < 0 || phi > 127)
               d = 63 - d;

            /* Flip previous d value if crossing max phi */
            if(dPrev != DmtxUndefined && phi128 == 0)
               dPrev = 63 - dPrev;

            if(dPrev == DmtxUndefined || dPrev == d)
            {
               vanish->bucket[d][phi128] += val;
            }
            else if(dPrev < d)
            {
               for(i = dPrev + 1; i <= d; i++)
                  vanish->bucket[i][phi128] += val;
            }
            else
            {
               for(i = dPrev - 1; i >= d; i--)
                  vanish->bucket[i][phi128] += val;
            }

            dPrev = d;
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
   bucketF = bucketRad * (96.0/M_PI);
   bucket = (bucketF > 0.0) ? (int)(bucketF + 0.5) : (int)(bucketF - 0.5);

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
GetZeroCrossing(DmtxValueGrid *accel, int iCol, int iRow)
{
   int aInc, aIdx, aIdxNext;
   int aRow, aCol;
   int aWidth, aHeight;
   int aHere, aNext, aPrev;
   double smidge;
   const ZeroCrossing emptyEdge = { 0, 0, 0, 0.0, 0.0 };
   ZeroCrossing edge;

   assert(accel->type == DmtxEdgeVertical || accel->type == DmtxEdgeHorizontal);

   aWidth = dmtxValueGridGetWidth(accel);
   aHeight = dmtxValueGridGetHeight(accel);

   /* XXX add better bounds checking of aIdxNext now that we're comparing diagonals */

   if(accel->type == DmtxEdgeVertical)
   {
      aRow = iRow - 1;
      aCol = iCol - 2;
      aInc = 1;
   }
   else { /* DmtxEdgeHorizontal */
      aRow = iRow - 2;
      aCol = iCol - 1;
      aInc = aWidth;
   }

   aIdx = aRow * aWidth + aCol;
   aIdxNext = aIdx + aInc;
   aHere = accel->value[aIdx];
   aNext = accel->value[aIdxNext];

   edge = emptyEdge;

   if(OPPOSITE_SIGNS(aHere, aNext))
   {
      /* Zero crossing: Neighbors with opposite signs [-10,+10] */
      smidge = abs(aHere/(aHere - aNext));
      edge = SetZeroCrossingFromIndex(accel, iCol, iRow, smidge);
   }
   else if(aHere == 0 && aNext != 0)
   {

      if(!(accel->type == DmtxEdgeVertical && aCol == 0) &&
            !(accel->type == DmtxEdgeHorizontal && aRow == 0))
      {
         aPrev = accel->value[aIdx-aInc];
         if(OPPOSITE_SIGNS(aPrev, aNext))
         {
            /* Zero crossing: Opposite signs separated by zero [-10,0,+10] */
            smidge = 0.0;
            edge = SetZeroCrossingFromIndex(accel, iCol, iRow, smidge);
         }
      }
   }

   return edge;
}

/**
 * 0 < smidge < 1
 *
 */
ZeroCrossing
SetZeroCrossingFromIndex(DmtxValueGrid *accel, int iCol, int iRow, double smidge)
{
   int sCol, sRow, sIdx;
   ZeroCrossing edge;
   DmtxValueGrid *sobel = accel->ref;

   edge.iCol = iCol;
   edge.iRow = iRow;

   if(accel->type == DmtxEdgeVertical)
   {
      edge.x = (double)iCol + smidge;
      edge.y = (double)iRow + 0.5;
   }
   else /* DmtxEdgeHorizontal */
   {
      edge.x = (double)iCol + 0.5;
      edge.y = (double)iRow + smidge;
   }

   sRow = iRow - 1;
   sCol = iCol - 1;

   if(sCol < 0 || sCol >= sobel->width || sRow < 0 || sRow >= sobel->height)
   {
      /* Sobel location out of bounds */
      edge.mag = 0;
   }
   else
   {
      /* Use Sobel value that falls directly between 2 accel locations */
      sIdx = sRow * dmtxValueGridGetWidth(sobel) + sCol;
      edge.mag = abs(sobel->value[sIdx]);
   }

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
