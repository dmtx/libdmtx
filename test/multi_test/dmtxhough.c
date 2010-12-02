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
   int row, col;

   dec->houghGrid = (DmtxHoughGrid *)calloc(1, sizeof(DmtxHoughGrid));
   if(dec->houghGrid == NULL)
      return DmtxFail;

   dec->houghGrid->rows = 1;
   dec->houghGrid->cols = 1;
   dec->houghGrid->count = dec->houghGrid->rows * dec->houghGrid->cols;
   dec->houghGrid->line = (DmtxHoughLocal *)calloc(dec->houghGrid->count, sizeof(DmtxHoughLocal));
   dec->houghGrid->vanish = (DmtxHoughLocal *)calloc(dec->houghGrid->count, sizeof(DmtxHoughLocal));

   RETURN_FAIL_IF(dec->houghGrid->line == NULL || dec->houghGrid->vanish == NULL);

   for(row = 0; row < dec->houghGrid->rows; row++)
   {
      for(col = 0; col < dec->houghGrid->cols; col++)
      {
         RETURN_FAIL_IF(LineHoughAccumulate(dec, col, row) == DmtxFail);
         RETURN_FAIL_IF(VanishHoughAccumulate(dec, col, row) == DmtxFail);

         dec->fn.dmtxHoughLocalCallback(dec->houghGrid->line, 0);
         dec->fn.dmtxHoughLocalCallback(dec->houghGrid->vanish, 1);
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
LineHoughAccumulate(DmtxDecode2 *dec, int gCol, int gRow)
{
   int rRow, rCol;
   int iRow, iCol;
   int iWidth, iHeight;
   int phi;
   DmtxHoughLocal *hRegion;
   ZeroCrossing vvZXing, vbZXing, hbZXing, hhZXing, hsZXing, vsZXing;
   DmtxPassFail vvPassFail, vbPassFail, hbPassFail, hhPassFail, hsPassFail, vsPassFail;

   hRegion = &(dec->houghGrid->line[0]); /* [hCol * width + hRol]; */
   memset(hRegion, 0x00, sizeof(DmtxHoughLocal));

   /* Global coordinate system */
   iWidth = dmtxImageGetProp(dec->image, DmtxPropWidth);
   iHeight = dmtxImageGetProp(dec->image, DmtxPropHeight);

   hRegion->xOrigin = gState.localOffsetX;
   hRegion->yOrigin = gState.localOffsetY;

   /* calculate dOffset ? */

   for(rRow = 0; rRow < 64; rRow++)
   {
      iRow = hRegion->yOrigin + rRow;

      if(iRow >= iHeight)
         continue;

      for(rCol = 0; rCol < 64; rCol++)
      {
         iCol = hRegion->xOrigin + rCol;

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
               HoughLocalAccumulateEdge(hRegion, phi, vvZXing);
            for(phi = 112; phi < 128; phi++)
               HoughLocalAccumulateEdge(hRegion, phi, vvZXing);
         }

         if(vbZXing.mag > 0 && vbPassFail == DmtxPass)
            for(phi = 16; phi < 32; phi++)
               HoughLocalAccumulateEdge(hRegion, phi, vbZXing);

         if(hbZXing.mag > 0 && hbPassFail == DmtxPass)
            for(phi = 32; phi < 48; phi++)
               HoughLocalAccumulateEdge(hRegion, phi, hbZXing);

         if(hhZXing.mag > 0 && hhPassFail == DmtxPass)
            for(phi = 48; phi < 80; phi++)
               HoughLocalAccumulateEdge(hRegion, phi, hhZXing);

         if(hsZXing.mag > 0 && hsPassFail == DmtxPass)
            for(phi = 80; phi < 96; phi++)
               HoughLocalAccumulateEdge(hRegion, phi, hsZXing);

         if(vsZXing.mag > 0 && vsPassFail == DmtxPass)
            for(phi = 96; phi < 112; phi++)
               HoughLocalAccumulateEdge(hRegion, phi, vsZXing);
      }
   }

   return DmtxPass;
}

/**
 *
 *
 */
DmtxPassFail
VanishHoughAccumulate(DmtxDecode2 *dec, int gCol, int gRow)
{
   int lhRow, lhCol;
   int phi, d;
   DmtxHoughLocal *lhRegion, *vhRegion;

   lhRegion = &(dec->houghGrid->line[0]); /* will eventually be [hCol * width + hRol]; */
   vhRegion = &(dec->houghGrid->vanish[0]); /* will eventually be [hCol * width + hRol]; */

   for(lhRow = 0; lhRow < 64; lhRow++)
   {
      for(lhCol = 0; lhCol < 128; lhCol++)
      {
         /* XXX later be sure to flip d in comparisons across 0/127 boundary */
         /* XXX this actually overextends array boundaries I but don't care yet */
         if(lhRegion->bucket[lhRow][lhCol] < lhRegion->bucket[lhRow + 1][lhCol] ||
               lhRegion->bucket[lhRow][lhCol] < lhRegion->bucket[lhRow - 1][lhCol])
            continue;

         for(phi = 0; phi < 128; phi++)
         {
            d = GetVanishBucket(phi, lhCol, lhRow);
            if(d == DmtxUndefined)
               continue;
            vhRegion->bucket[d][phi] += lhRegion->bucket[lhRow][lhCol];
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
   double d;
   double numer, denom;
   double phiBucketRad, phiCompareRad, bucketRad;
   double sinPhiCompare, cosPhiCompare, phiTan;
   DmtxVector2 w;
   DmtxRay2 p0, p1;

   if(phiBucket == phiCompare)
      return 32; /* Infinity */

   phiDelta = phiBucket - phiCompare;
   if(phiDelta < -64)
      phiDelta += 128;
   else if(phiDelta > 64)
      phiDelta -= 128;

   if(abs(phiDelta) > 32)
      return DmtxUndefined; /* Too far from parallel */

   phiTan = tan(phiDelta * (M_PI/128.0));

   p0.p.X = p0.p.Y = 32.0;
   phiBucketRad = phiBucket * (M_PI/128.0);
   p0.v.X = cos(phiBucketRad);
   p0.v.Y = sin(phiBucketRad);

   /* XXX later make helper function to convert lineHough point to DmtxRay2 */
   phiCompareRad = phiCompare * (M_PI/128.0);
   sinPhiCompare = sin(phiCompareRad);
   cosPhiCompare = cos(phiCompareRad);

   d = UncompactOffset(dCompare, phiCompare, 64);
   p1.p.X = d * cosPhiCompare;
   p1.p.Y = d * sinPhiCompare;
   p1.v.X = sinPhiCompare;
   p1.v.Y = -cosPhiCompare;

   denom = dmtxVector2Cross(&(p1.v), &(p0.v));
   assert(fabs(denom) > 0.000001); /* We only compare nearly-parallel converging lines */

   dmtxVector2Sub(&w, &(p1.p), &(p0.p));
   numer = dmtxVector2Cross(&(p1.v), &w);

   bucketRad = atan2(32, (numer/denom)/phiTan);
   if(bucketRad > M_PI_2)
      bucketRad -= M_PI;
   else if(bucketRad < -M_PI_2)
      bucketRad += M_PI;

   /* map -pi/4 -> pi/4 to 0 -> 63 */
   bucket = (int)(bucketRad * (128.0/M_PI)) + 32;

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
