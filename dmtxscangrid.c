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
 * @param  xMin
 * @param  xMax
 * @param  yMin
 * @param  yMax
 * @param  smallestFeature
 * @return Initialized grid
 */
static DmtxScanGrid
InitScanGrid(DmtxImage *img, int smallestFeature)
{
   int xExtent, yExtent, maxExtent;
   int extent;
   DmtxScanGrid grid;

   memset(&grid, 0x00, sizeof(DmtxScanGrid));

   grid.xMin = dmtxImageGetProp(img, DmtxPropScaledXmin);
   grid.xMax = dmtxImageGetProp(img, DmtxPropScaledXmax);
   grid.yMin = dmtxImageGetProp(img, DmtxPropScaledYmin);
   grid.yMax = dmtxImageGetProp(img, DmtxPropScaledYmax);

   /* Values that get set once */
   xExtent = grid.xMax - grid.xMin;
   yExtent = grid.yMax - grid.yMin;
   maxExtent = (xExtent > yExtent) ? xExtent : yExtent;

   assert(maxExtent > 1);

   for(extent = 1; extent < maxExtent; extent = ((extent + 1) * 2) - 1)
      if(extent <= smallestFeature)
         grid.minExtent = extent;

   grid.maxExtent = extent;

   grid.xOffset = (grid.xMin + grid.xMax - grid.maxExtent) / 2;
   grid.yOffset = (grid.yMin + grid.yMax - grid.maxExtent) / 2;

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
static DmtxPixelLoc
IncrementPixelProgress(DmtxScanGrid *cross)
{
   DmtxPixelLoc loc;

   /* Loop until a good location is found or the grid is completely traversed */
   do {

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

      loc = GetGridCoordinates(cross);

   } while(loc.status != DMTX_RANGE_GOOD && loc.status != DMTX_RANGE_EOF);

   return loc;
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

   if(grid->extent == 0 || grid->extent < grid->minExtent) {
      loc.X = loc.Y = -1;
      loc.status = DMTX_RANGE_EOF;
      return loc;
   }

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

   if(loc.X < grid->xMin || loc.X > grid->xMax ||
         loc.Y < grid->yMin || loc.Y > grid->yMax)
      loc.status = DMTX_RANGE_BAD;
   else
      loc.status = DMTX_RANGE_GOOD;

   return loc;
}
