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
DmtxHoughGrid *
HoughGridCreate(DmtxDecode2 *dec)
{
   int hRow, hCol;
   DmtxHoughGrid *hGrid;
   DmtxHoughLocal *hLocal;

   hGrid = (DmtxHoughGrid *)calloc(1, sizeof(DmtxHoughGrid));
   if(hGrid == NULL)
      return NULL;

   hGrid->rows = 1; /* for now */
   hGrid->cols = 1; /* for now */
   hGrid->count = hGrid->rows * hGrid->cols;

   hGrid->local = (DmtxHoughLocal *)malloc(hGrid->count * sizeof(DmtxHoughLocal));

   for(hRow = 0; hRow < hGrid->rows; hRow++)
   {
      for(hCol = 0; hCol < hGrid->cols; hCol++)
      {
         hLocal = &(hGrid->local[hRow * hGrid->cols + hCol]);
         InitHoughLocal(hLocal, 100, 100);
         HoughLocalAccumulate(hLocal, dec);
      }
   }

   return hGrid;
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
void
InitHoughLocal(DmtxHoughLocal *local, int xOrigin, int yOrigin)
{
   memset(local, 0x00, sizeof(DmtxHoughLocal));

   local->xOrigin = xOrigin;
   local->yOrigin = xOrigin;

   /* calculate dOffset ? */
}

/**
 *
 *
 */
DmtxPassFail
HoughLocalAccumulate(DmtxHoughLocal *local, DmtxDecode2 *dec)
{
   int row, col;
   int iRow, iCol;
   int phi;
   ZeroCrossing hhZXing, hsZXing, vsZXing, vvZXing, vbZXing, hbZXing;

   for(row = 0; row < 64; row++)
   {
      iRow = local->yOrigin + row;

      for(col = 0; col < 64; col++)
      {
         iCol = local->xOrigin + col;

         hhZXing = GetZeroCrossing(dec, iCol, iRow, SobelDirHorizontal, DmtxDirHorizontal);
         hsZXing = GetZeroCrossing(dec, iCol, iRow, SobelDirSlash, DmtxDirHorizontal);
         vsZXing = GetZeroCrossing(dec, iCol, iRow, SobelDirSlash, DmtxDirVertical);
         vvZXing = GetZeroCrossing(dec, iCol, iRow, SobelDirVertical, DmtxDirVertical);
         vbZXing = GetZeroCrossing(dec, iCol, iRow, SobelDirBackslash, DmtxDirVertical);
         hbZXing = GetZeroCrossing(dec, iCol, iRow, SobelDirBackslash, DmtxDirHorizontal);

         if(hhZXing.mag > 0)
         {
            for(phi = 0; phi < 16; phi++)
               HoughLocalAccumulateEdge(local, phi, hhZXing);
            for(phi = 112; phi < 128; phi++)
               HoughLocalAccumulateEdge(local, phi, hhZXing);
         }

         if(hsZXing.mag > 0)
            for(phi = 16; phi < 32; phi++)
               HoughLocalAccumulateEdge(local, phi, hsZXing);

         if(vsZXing.mag > 0)
            for(phi = 32; phi < 48; phi++)
               HoughLocalAccumulateEdge(local, phi, vsZXing);

         if(vvZXing.mag > 0)
            for(phi = 48; phi < 80; phi++)
               HoughLocalAccumulateEdge(local, phi, vvZXing);

         if(vbZXing.mag > 0)
            for(phi = 80; phi < 96; phi++)
               HoughLocalAccumulateEdge(local, phi, vbZXing);

         if(hbZXing.mag > 0)
            for(phi = 96; phi < 112; phi++)
               HoughLocalAccumulateEdge(local, phi, hbZXing);
      }
   }

   return DmtxPass;
}

/**
 *
 *
 */
ZeroCrossing
GetZeroCrossing(DmtxDecode2 *dec, int iCol, int iRow, SobelDirection sobelDir, DmtxDirection edgeDir)
{
   int aInc, aIdx, aIdxNext;
   int aRow, aCol;
   int aWidth, aHeight;
   int aHere, aNext, aPrev;
   int *accelPtr;
   double smidge;
   PixelEdgeCache *accel;
   ZeroCrossing edge = { 0, 0.0, 0.0 };

   assert(edgeDir == DmtxDirVertical || edgeDir == DmtxDirHorizontal);

   if(edgeDir == DmtxDirVertical)
   {
      accel = dec->accelV;
      aInc = 1;
   }
   else
   {
      accel = dec->accelH;
      aInc = aWidth;
   }

   aWidth = PixelEdgeCacheGetWidth(accel);
   aHeight = PixelEdgeCacheGetHeight(accel);
   aRow = iRow - 1;
   aCol = iCol - 1;
   aIdx = aRow * aWidth + aCol;
   aIdxNext = aIdx + aInc;

   switch(sobelDir) {
      case SobelDirVertical:
         accelPtr = accel->v;
         break;
      case SobelDirBackslash:
         accelPtr = accel->b;
         break;
      case SobelDirHorizontal:
         accelPtr = accel->h;
         break;
      case SobelDirSlash:
         accelPtr = accel->s;
         break;
      default:
         return edge;
   }

   aHere = accelPtr[aIdx];
   aNext = accelPtr[aIdxNext];

   if(OPPOSITE_SIGNS(aHere, aNext))
   {
      /* Zero crossing: Neighbors with opposite signs [-10,+10] */
      smidge = abs(aHere/(aHere - aNext));
      edge = SetZeroCrossingFromIndex(dec, aCol, aRow, smidge, sobelDir, edgeDir);
   }
   else if(aHere == 0 && aNext != 0)
   {
      /* No previous value for comparison (beginning of row/col) */
      if(aIdx < aInc)
         return edge;

      aPrev = accelPtr[aIdx-aInc];
      if(OPPOSITE_SIGNS(aPrev, aNext))
      {
         /* Zero crossing: Opposite signs separated by zero [-10,0,+10] */
         edge = SetZeroCrossingFromIndex(dec, aCol, aRow, 0.0, sobelDir, edgeDir);
      }
   }

   if(gState.displayEdge == DmtxUndefined || gState.displayEdge == sobelDir)
   {
      if(edgeDir == DmtxDirVertical && (sobelDir == SobelDirVertical || sobelDir == SobelDirBackslash))
         dec->fn.zeroCrossingCallback(iCol, iRow, edge.mag, 0);
      else if(edgeDir == DmtxDirHorizontal && (sobelDir == SobelDirHorizontal || sobelDir == SobelDirSlash))
         dec->fn.zeroCrossingCallback(iCol, iRow, edge.mag, 0);
   }

   return edge;
}

/**
 * 0 < smidge < 1
 *
 */
ZeroCrossing
SetZeroCrossingFromIndex(DmtxDecode2 *dec, int aCol, int aRow, double smidge,
      SobelDirection sobelDir, DmtxDirection edgeDir)
{
   int sIdx;
   ZeroCrossing edge;

   assert(edgeDir == DmtxDirVertical || edgeDir == DmtxDirHorizontal);

   if(edgeDir == DmtxDirVertical)
   {
      edge.x = (double)aCol + 2.0 + smidge;
      edge.y = (double)aRow + 1.5;
   }
   else
   {
      edge.x = (double)aCol + 1.5;
      edge.y = (double)aRow + 2.0 + smidge;
   }

   sIdx = SobelCacheGetIndexFromZXing(dec->sobel, edgeDir, aCol, aRow);
   edge.mag = abs(SobelCacheGetValue(dec->sobel, sobelDir, sIdx));

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

   d = HoughGetLocalOffset(edge.x, edge.y, phi);

/* local->bucket[(int)d][phi] += edge.mag; */

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
      scale = 64.0 / (sinPhi + cosPhi);
      d = (xLoc * cosPhi + yLoc * sinPhi) * scale;
   }
   else
   {
      scale = 64.0 / (sinPhi - cosPhi);
      d = ((xLoc * cosPhi + yLoc * sinPhi) - cosPhi) * scale;
   }

   return d;
}
