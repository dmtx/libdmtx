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

/*
#include <string.h>
#include <math.h>
#include <assert.h>
#include "kiss_fftr.h"
#include "../../dmtx.h"
*/
#include "multi_test.h"

/**
 *
 *
 */
DmtxHoughGrid *
HoughGridCreate(DmtxDecode2 *dec)
{
   int row, col;
   DmtxHoughGrid *grid;
   DmtxHoughLocal *local;

   grid = (DmtxHoughGrid *)calloc(1, sizeof(DmtxHoughGrid));
   if(grid == NULL)
      return NULL;

   grid->rows = 1; /* for now */
   grid->cols = 1; /* for now */
   grid->count = grid->rows * grid->cols;

   grid->local = (DmtxHoughLocal *)malloc(grid->count * sizeof(DmtxHoughLocal));

   for(row = 0; row < grid->rows; row++)
   {
      for(col = 0; col < grid->cols; col++)
      {
         local = &(grid->local[row * grid->cols + col]);
         InitHoughLocal(local, 100, 100);

         FindZeroCrossings(dec, local, DmtxDirVertical);
         FindZeroCrossings(dec, local, DmtxDirHorizontal);
      }
   }

   return grid;
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

   /* calculate dOffset */
}

/**
 * 0 < smidge < 1
 */
DmtxPassFail
RegisterZeroCrossing(DmtxHoughLocal *hough, DmtxDirection edgeType, int zCol,
      int zRow, double smidge, PixelEdgeCache *sobel, int s, DmtxCallbacks *fn)
{
   int sIdx, sValue;
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

   sIdx = SobelCacheGetIndexFromZXing(sobel, edgeType, zCol, zRow);
   sValue = SobelCacheGetValue(sobel, s, sIdx);

   if(gState.displayEdge == DmtxUndefined || gState.displayEdge == s)
   {
      if(edgeType == DmtxDirVertical && (s == 0 || s == 1))
         fn->zeroCrossingCallback(xImg, yImg, sValue, 0);
      else if(edgeType == DmtxDirHorizontal && (s == 2 || s == 3))
         fn->zeroCrossingCallback(xImg, yImg, sValue, 0);
   }

   /* This is where we will accumulate sample into a local hough cache */

   return DmtxPass;
}

/**
 * accel holds acceleration values (vertical or horizontal) that will determine zero crossing locations.
 * sobel contains the actual edge strength, and will only be used if a zero crossing is found
 */
DmtxPassFail
FindZeroCrossings(DmtxDecode2 *dec, DmtxHoughLocal *local, DmtxDirection edgeType)
{
   PixelEdgeCache *sobel = dec->sobel;
   PixelEdgeCache *accel;
   DmtxCallbacks *fn = &(dec->fn);
   int zCol, zRow, s;
   int aInc, aIdx, aIdxNext;
   int aWidth, aHeight;
   int zWidth, zHeight;
   int aPrev, aHere, aNext;
   int *accelPtr;
   double smidge;

   if(edgeType == DmtxDirVertical)
   {
      accel = dec->accelV;
      aWidth = PixelEdgeCacheGetWidth(accel);
      aHeight = PixelEdgeCacheGetHeight(accel);
      zWidth = aWidth - 1;
      zHeight = aHeight;
      aInc = 1;
   }
   else if(edgeType == DmtxDirHorizontal)
   {
      accel = dec->accelH;
      aWidth = PixelEdgeCacheGetWidth(accel);
      aHeight = PixelEdgeCacheGetHeight(accel);
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
               RegisterZeroCrossing(local, edgeType, zCol, zRow, smidge, sobel, s, fn);
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
                  RegisterZeroCrossing(local, edgeType, zCol, zRow, smidge, sobel, s, fn);
               }
            }
         }
      }
   }

   return DmtxPass;
}
