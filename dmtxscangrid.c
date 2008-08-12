/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (c) 2008 Mike Laughton

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

Contact: mike@dragonflylogic.com
*/

/* $Id$ */

/**
 * @file dmtxscangrid.c
 * @brief Scan grid tracking
 */

/**
 * @brief  XXX
 * @param  p0
 * @param  p1
 * @param  minGapSize
 * @return Initialized grid
 */
static DmtxScanGrid
InitScanGrid(int scanGap, DmtxImage *img)
{
   int xMin, xMax, yMin, yMax;
   int xExtent, yExtent, maxExtent;
   int extent;
   DmtxScanGrid grid;

   memset(&grid, 0x00, sizeof(DmtxScanGrid));

   xMin = dmtxImageGetProp(img, DmtxPropScaledXmin);
   xMax = dmtxImageGetProp(img, DmtxPropScaledXmax);
   yMin = dmtxImageGetProp(img, DmtxPropScaledYmin);
   yMax = dmtxImageGetProp(img, DmtxPropScaledYmax);

   /* Values that get set once */
   xExtent = xMax - xMin;
   yExtent = yMax - yMin;
   maxExtent = (xExtent > yExtent) ? xExtent : yExtent;

   assert(maxExtent > 1);

   for(extent = 1; extent < maxExtent; extent = ((extent + 1) * 2) - 1) {
      if(extent <= scanGap)
         grid.minExtent = extent;
   }
   grid.maxExtent = extent;

   grid.xOffset = (xMin + xMax - grid.maxExtent) / 2;
   grid.yOffset = (yMin + yMax - grid.maxExtent) / 2;

   /* Values that get reset for every level */
   grid.total = 1;
   grid.extent = grid.maxExtent;

   SetDerivedFields(&grid);

   return grid;
}

/**
 * @brief  XXX
 * @param  cross
 * @return void
 */
static void
IncrementPixelProgress(DmtxScanGrid *cross)
{
   cross->pixelCount++;

   /* Increment cross horizontally when go exhaust pixels */
   if(cross->pixelCount >= cross->pixelTotal) {
      cross->pixelCount = 0;
      cross->xCenter += cross->jumpSize;
   }

   /* Increment cross vertically when horizontal step takes us too far */
   if(cross->xCenter > cross->maxExtent) {
      cross->xCenter = cross->startPos;
      cross->yCenter += cross->jumpSize;
   }

   /* Increment level when vertical step takes us too far */
   if(cross->yCenter > cross->maxExtent) {
      cross->total *= 4;
      cross->extent /= 2;
      SetDerivedFields(cross);
   }
}

/**
 * @brief  XXX
 * @param  cross
 * @return void
 */
static void
SetDerivedFields(DmtxScanGrid *cross)
{
   cross->jumpSize = cross->extent + 1;
   cross->pixelTotal = 2 * cross->extent - 1;
   cross->startPos = cross->extent / 2;
   cross->pixelCount = 0;
   cross->xCenter = cross->yCenter = cross->startPos;
}

/**
 * @brief  XXX
 * @param  grid
 * @return Pixel location
 */
static DmtxPixelLoc
GetGridCoordinates(DmtxScanGrid *grid)
{
   int count, half, quarter;
   DmtxPixelLoc loc;

   count = grid->pixelCount;

   assert(count < grid->pixelTotal);

   if(count == grid->pixelTotal - 1) {
      /* center pixel */
      loc.X = grid->xCenter;
      loc.Y = grid->yCenter;
   }
   else {
      half = grid->pixelTotal / 2;
      quarter = half / 2;

      /* horizontal portion */
      if(count < half) {
         loc.X = grid->xCenter + ((count < quarter) ? (count - quarter) : (half - count));
         loc.Y = grid->yCenter;
      }
      /* vertical portion */
      else {
         count -= half;
         loc.X = grid->xCenter;
         loc.Y = grid->yCenter + ((count < quarter) ? (count - quarter) : (half - count));
      }
   }

   loc.X += grid->xOffset;
   loc.Y += grid->yOffset;

   return loc;
}
