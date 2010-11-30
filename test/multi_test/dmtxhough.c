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

/**
 *
 *
 */
DmtxPassFail
HoughGridPopulate(DmtxDecode2 *dec)
{
   int row, col;
   DmtxHoughGrid *grid;

   dec->houghGrid = (DmtxHoughGrid *)calloc(1, sizeof(DmtxHoughGrid));
   if(dec->houghGrid == NULL)
      return DmtxFail;

   grid = dec->houghGrid;
   grid->rows = 1;
   grid->cols = 1;
   grid->count = grid->rows * grid->cols;
   grid->local = (DmtxHoughLocal *)malloc(grid->count * sizeof(DmtxHoughLocal));
   if(grid->local == NULL)
   {
      free(dec->houghGrid);
      dec->houghGrid = NULL;
      return DmtxFail;
   }

   for(row = 0; row < grid->rows; row++)
   {
      for(col = 0; col < grid->cols; col++)
      {
         HoughLocalAccumulate(dec, col, row);
      }
   }

   dec->fn.dmtxHoughLocalCallback(dec->houghGrid->local, 0);

   return DmtxPass;
}

/**
 *
 *
 */
DmtxPassFail
HoughGridDestroy(DmtxHoughGrid **grid)
{
   if(grid == NULL || *grid == NULL)
      return DmtxFail;

   if((*grid)->local != NULL)
      free((*grid)->local);

   free(*grid);
   *grid = NULL;

   return DmtxPass;
}

/**
 *
 *
 */
DmtxPassFail
HoughLocalAccumulate(DmtxDecode2 *dec, int gCol, int gRow)
{
   int rRow, rCol;
   int iRow, iCol;
   int iWidth, iHeight;
   int phi;
   DmtxHoughLocal *hRegion;
   ZeroCrossing vvZXing, vbZXing, hbZXing, hhZXing, hsZXing, vsZXing;
   DmtxPassFail vvPassFail, vbPassFail, hbPassFail, hhPassFail, hsPassFail, vsPassFail;

   hRegion = &(dec->houghGrid->local[0]); /* [hCol * width + hRol]; */
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

         if(gState.displayEdge == 1)
            dec->fn.zeroCrossingCallback(vvZXing, 0);
         else if(gState.displayEdge == 2)
            dec->fn.zeroCrossingCallback(vbZXing, 0);
         else if(gState.displayEdge == 3)
            dec->fn.zeroCrossingCallback(hbZXing, 0);
         else if(gState.displayEdge == 4)
            dec->fn.zeroCrossingCallback(hhZXing, 0);
         else if(gState.displayEdge == 5)
            dec->fn.zeroCrossingCallback(hsZXing, 0);
         else if(gState.displayEdge == 6)
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

   *passFail = DmtxFail;

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
      if(aIdx < aInc)
         return emptyEdge; /* Fail: No previous value for comparison (beginning of row/col) */

      aPrev = accel->value[aIdx-aInc];
      if(OPPOSITE_SIGNS(aPrev, aNext))
      {
         /* Zero crossing: Opposite signs separated by zero [-10,0,+10] */
         edge = SetZeroCrossingFromIndex(accel, aCol, aRow, 0.0, &childPassFail);
      }
   }

   /* XXX I'm not crazy about handling of edge here ... maybe should use passFail
      to allow parent to determine if edge was detected instead of relying on edge.mag > 0.0 */

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

   *passFail = DmtxFail;

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
         return emptyEdge; /* Fail: Illegal edge type */
   }

   /* Fail:  Sobel location out of bounds */
   if(sCol < 0 || sCol >= sobel->width || sRow < 0 || sRow >= sobel->height)
      return emptyEdge;

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
HoughLocalAccumulateEdge(DmtxHoughLocal *local, int phi, ZeroCrossing edge)
{
   double d;
   int dInt;

   d = HoughGetLocalOffset(edge.x - local->xOrigin, edge.y - local->yOrigin, phi);
   dInt = (int)d;

if(dInt > 63)
   return DmtxFail;

   assert(dInt >= 0);
   assert(dInt < 64);
   assert(phi >= 0 && phi < 128);

   local->bucket[dInt][phi] += edge.mag;

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
