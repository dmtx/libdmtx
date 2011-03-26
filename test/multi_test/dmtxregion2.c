/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2010 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in parent directory for full terms of
 * use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxregion2.c
 */

#include <string.h>
#include <math.h>
#include <assert.h>
#include "../../dmtx.h"
#include "multi_test.h"
#include "kiss_fftr.h"

#define RETURN_FAIL_IF(c) if(c) { return DmtxFail; }

/*
struct Deskew {
   DmtxMatrix3 fit2raw;
   DmtxMatrix3 raw2fit;
}

struct Timing {
   double shift;
   double period;
}

struct AlignmentGrid {
   Deskew align;
   Timing vTiming;
   Timing hTiming;
}
*/

/**
 *
 *
 */
DmtxPassFail
dmtxRegion2FindNext(DmtxDecode2 *dec)
{
   int i, j;
   int phiDiff, phiDiffTmp;
   DmtxBoolean regionFound;
   DmtxPassFail passFail;
   VanishPointSort vPoints;
   DmtxOrient orient;

   vPoints = dmtxFindVanishPoints(dec->hough->vanish);
   dec->fn.vanishPointCallback(&vPoints, 0);

   for(i = 0, regionFound = DmtxFalse; i < vPoints.count && regionFound == DmtxFalse; i++)
   {
      for(j = i + 1; j < vPoints.count; j++)
      {
         phiDiffTmp = abs(vPoints.bucket[i].phi - vPoints.bucket[j].phi);
         phiDiff = (phiDiffTmp < 64) ? phiDiffTmp : 128 - phiDiffTmp;

         /* Reject angle combinations that are too close */
         if(phiDiff < 36)
            continue;

         /* Build oriented (but still untimed) grid from vanish points */
         passFail = OrientRegion(&orient, vPoints.bucket[i], vPoints.bucket[j], dec);
         if(passFail == DmtxFail)
            continue;

         /* Build timed grid from untimed grid and line hough */
/*       align = CalibrateRegion(region, orient, lHough, &passFail);
         if(passFail == DmtxFail)
            continue;
*/
/*
         // timings = dmtxFindGridTiming(dec->hough->line, &vPoints);

         err = dmtxBuildGridFromTimings(&grid, timings.timing[i], timings.timing[j]);
         if(err == DmtxFail)
            continue; // Keep trying

         dec->fn.timingCallback(&timings.timing[i], &timings.timing[j], 1);

         // Hack together raw2fitFull and fit2rawFull outside since we need app data
         AddFullTransforms(&grid);

         err = dmtxFindRegionWithinGrid(&region, &grid, &houghCache, dec, fn);
         regionFound = (err == DmtxPass) ? DmtxTrue : DmtxFalse;

         if(regionFound == DmtxTrue) {
            region.sizeIdx = dmtxGetSizeIdx(region.width, region.height);
            if(region.sizeIdx >= DmtxSymbol10x10 && region.sizeIdx <= DmtxSymbol16x48)
               dmtxDecodeSymbol(&region, dec);
         }

         regionFound = DmtxTrue; // break out of outer loop
         break; // break out of inner loop
*/
      }
   }

   return DmtxPass;
}

/**
 *
 *
 */
DmtxPassFail
OrientRegion(DmtxOrient *orient, DmtxHoughBucket v0, DmtxHoughBucket v1, DmtxDecode2 *dec)
{
   DmtxRay2 v0a, v0b, v1a, v1b;

   v0a = dec->corners[v0.d][v0.phi].lineA;
   v0b = dec->corners[v0.d][v0.phi].lineB;
   v1a = dec->corners[v1.d][v1.phi].lineA;
   v1b = dec->corners[v1.d][v1.phi].lineB;
/*
   RegionFromSides(v0a, v0b, v1a, v1b);
*/
   return DmtxPass;
}

/**
 *
 *
 */
/*
DmtxPassFail
CalibrateRegion(DmtxOrient *orient)
{
input:  orientation (transformation matrix)
        hough line cache

output: timed alignment grid

description:
  fourier transform receives evenly spaced samples to be taken in both directions (v and h)
    -> tranforms into frequency space
    -> major frequency emerges
  determine shift as post-processing step (can't remember how I did it before at the moment)

  need to interpolate values between true locations in line hough because
    "evenly spaced" describes positions in the normalized fitted region whereas
    the hough values are aligned along raw image coordinates

  step size should be determined based on desired fft dimensions (maybe 64
    steps?) ... check what we did for poc

steps:
   for each step 0-63 upward along Y axis
      xFit = 0
      yFit = 1-63
      xRaw,yRaw = VMult(fit2raw,xFit,yFit)
      lineFit = (xRaw - 0, yRaw - 0) (duh)
      phi = lineFit angle (something like atan2)
      d = vector2Mag(lineFit)
      interpolate hough[d][phi] and add to fft
      that was easy

   return DmtxPass;
}
*/

/**
 * Future structure of dmtxRegion2FindNext() (after getting orientation and timing working again)
 *
 * TODO:
 * o Is there a way to name HoughGridPopulate() and HoughGridMerge() to show they are sister functions?
 *
 * hough lines, hough line maxima, and hough vanish can be calculated for entire image at once
 * for each local:
 *    for each combination of vpoint pairs:
 *       step outward
 *          if step takes us into adjacent local
 *              add maxima from adjacent local into existing vpoint hough accumulator
 *              rescan for updated vpoints (using original centerline, adding only portion of region?)
 *              get new timing (?)
 *              continue stepping path where we left off?
 *
 * progress:
 *    level
 *    row
 *    col
 *    vanish point combo (?)
 */
/*
dmtxRegion2FindNext(DmtxDecode2 *dec)
{
   if(starting new level other than the first)
      HoughGridMerge();

   // for each new local
   while(dec->localIdx < dec->localCount)
   {
      // each valid combination of vanish points
      for(dec->vPointIdx < 28)
      {
         vPointI = xyz;
         vPointJ = xyz;

         while(perimeter condition not met)
         {
            stepOutward()

            if(first step in new local, including first)
            {
               add new region to central vanish hough
               update vanish points (shouldn't move too far)
               record that new local was added to stats
               redo timing
            }

            test perimeter for known strip patterns
            if(perimeter says that barcode region is found) {
               save current progress ("?")
               return region;
            }
         }
      }
   }

   return NULL;
}
*/

/**
 *
 *
 */
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
}

/**
 *
 *
 */
void
AddToVanishPointSort(VanishPointSort *sort, DmtxHoughBucket bucket)
{
   int i, startHere;
   int phiDiff, phiDiffTmp;
   DmtxBoolean isFull;
   DmtxHoughBucket *lastBucket;

   isFull = (sort->count == ANGLE_SORT_MAX_COUNT) ? DmtxTrue : DmtxFalse;
   lastBucket = &(sort->bucket[ANGLE_SORT_MAX_COUNT - 1]);

   /* Array is full and incoming bucket is already weakest */
   if(isFull && bucket.val < lastBucket->val)
      return;

   startHere = DmtxUndefined;

   /* If sort already has entry near this angle then either:
    *   a) Overwrite the old one without shifting if stronger
    *   b) Reject the new one completely if weaker
    */
   for(i = 0; i < sort->count; i++)
   {
      phiDiffTmp = abs(bucket.phi - sort->bucket[i].phi);
      phiDiff = (phiDiffTmp < 64) ? phiDiffTmp : 128 - phiDiffTmp;

      if(phiDiff < 10)
      {
         /* Similar angle is already represented with stronger magnitude */
         if(bucket.val < sort->bucket[i].val)
         {
            return;
         }
         /* Found similar-but-weaker angle that will be overwritten */
         else
         {
            sort->bucket[i] = bucket;
            startHere = i;
            break;
         }
      }
   }

   if(startHere == DmtxUndefined)
   {
      if(isFull)
         *lastBucket = bucket;
      else
         sort->bucket[sort->count++] = bucket;

      startHere = sort->count - 1;
   }

   /* Shift weak entries downward */
   for(i = startHere; i > 0; i--)
   {
      if(bucket.val > sort->bucket[i-1].val)
      {
         sort->bucket[i] = sort->bucket[i-1];
         sort->bucket[i-1] = bucket;
      }
      else
      {
         break;
      }
   }
}

/**
 *
 *
 */
VanishPointSort
dmtxFindVanishPoints(DmtxHoughLocal *vHough)
{
   DmtxHoughBucket bucket;
   VanishPointSort sort;

   memset(&sort, 0x00, sizeof(VanishPointSort));

   for(bucket.phi = 0; bucket.phi < 128; bucket.phi++)
   {
      for(bucket.d = 0; bucket.d < 64; bucket.d++)
      {
         bucket.val = vHough->bucket[bucket.d][bucket.phi];
         AddToVanishPointSort(&sort, bucket);
      }
   }

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
 * Return sum of top 8 maximum points (hmmm)
 * btw, can we skip this step entirely?
 */
DmtxHoughBucket
GetAngleSumAtPhi(DmtxHoughLocal *line, int phi)
{
   int i, d;
   int prev, here, next;
   DmtxHoughBucket bucket;
   HoughMaximaSort sort;

   memset(&sort, 0x00, sizeof(HoughMaximaSort));

   /* Handle last condition separately; one sided comparison */
   prev = line->bucket[62][phi];
   here = line->bucket[63][phi];
   if(here > prev)
      AddToMaximaSort(&sort, here);

   /* Handle first condition separately; one sided comparison */
   here = line->bucket[0][phi];
   next = line->bucket[1][phi];
   if(here > next)
      AddToMaximaSort(&sort, here);

   /* Handle remaining conditions as two sided comparisons */
   for(d = 2; d < 64; d++)
   {
      prev = here;
      here = next;
      next = line->bucket[d][phi];

      if(here > 0 && here >= prev && here >= next)
         AddToMaximaSort(&sort, here);
   }

   bucket.d = 0;
   bucket.phi = phi;
   bucket.val = 0;
   for(i = 0; i < 8; i++)
      bucket.val += sort.mag[i];

   return bucket;
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
dmtxFindGridTiming(DmtxHoughLocal *line, VanishPointSort *vPoints)
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

      phi = vPoints->bucket[vSortIdx].phi;

      /* Load FFT input array */
      for(i = 0; i < NFFT; i++) {
         rin[i] = (i < 64) ? line->bucket[i][phi] : 0;
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
            fitMag += line->bucket[y][timing.phi];
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

   RETURN_FAIL_IF(dmtxRay2Intersect(&p00, &(flat->line[0]), &(steep->line[0])) == DmtxFail);
   RETURN_FAIL_IF(dmtxRay2Intersect(&p10, &(flat->line[0]), &(steep->line[1])) == DmtxFail);
   RETURN_FAIL_IF(dmtxRay2Intersect(&p11, &(flat->line[1]), &(steep->line[1])) == DmtxFail);
   RETURN_FAIL_IF(dmtxRay2Intersect(&p01, &(flat->line[1]), &(steep->line[0])) == DmtxFail);
   RETURN_FAIL_IF(RegionUpdateCorners(fit2raw, raw2fit, p00, p10, p11, p01) == DmtxFail);

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
dmtxFindRegionWithinGrid(GridRegion *region, AlignmentGrid *grid, DmtxHoughLocal *line, DmtxDecode *dec, DmtxCallbacks *fn)
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
/*       RegionExpand(&regGrow, sideDir, line, fn); */
         finderSides = DmtxDirNone;
         goodCount = 0;
      }
      else {
         if(innerType == DmtxBarFinder)
            finderSides |= sideDir;

         goodCount++;
      }

/*    fn->perimeterCallback(&regGrow, sideDir, innerType); */
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
   RETURN_FAIL_IF(Vector2Norm(&r.v) == DmtxFail);

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

   RETURN_FAIL_IF(RayFromPoints(&rs.bottom, p00, p10) == DmtxFail);
   RETURN_FAIL_IF(RayFromPoints(&rs.right, p10, p11) == DmtxFail);
   RETURN_FAIL_IF(RayFromPoints(&rs.top, p11, p01) == DmtxFail);
   RETURN_FAIL_IF(RayFromPoints(&rs.left, p01, p00) == DmtxFail);

   *regionSides = rs;

   return DmtxPass;
}

/**
 *
 *
 */
DmtxPassFail
dmtxRegionUpdateFromSides(GridRegion *region, DmtxRegionSides regionSides)
{
   DmtxVector2 p00, p10, p11, p01;

   RETURN_FAIL_IF(dmtxRay2Intersect(&p00, &(regionSides.left), &(regionSides.bottom)) == DmtxFail);
   RETURN_FAIL_IF(dmtxRay2Intersect(&p10, &(regionSides.bottom), &(regionSides.right)) == DmtxFail);
   RETURN_FAIL_IF(dmtxRay2Intersect(&p11, &(regionSides.right), &(regionSides.top)) == DmtxFail);
   RETURN_FAIL_IF(dmtxRay2Intersect(&p01, &(regionSides.top), &(regionSides.left)) == DmtxFail);

   RETURN_FAIL_IF(RegionUpdateCorners(region->grid.fit2rawActive, region->grid.raw2fitActive, p00, p10, p11, p01) == DmtxFail);
   /* need to update fit2rawFull and raw2fitFull here too? */

   return DmtxPass;
}

/**
 *
 *
 */
DmtxPassFail
RegionExpand(GridRegion *region, DmtxDirection sideDir, DmtxHoughLocal *line, DmtxCallbacks *fn)
{
   DmtxRegionSides regionSides;
   DmtxRay2 *sideRay;

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
   RETURN_FAIL_IF(GetOnOffColors(region, dec, &onColor, &offColor) == DmtxFail);

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
         RETURN_FAIL_IF(dmtxRegionUpdateCorners(dec, &reg, p00, p10, p11, p01) == DmtxFail);
         break;
      case (DmtxDirDown | DmtxDirRight):
         RETURN_FAIL_IF(dmtxRegionUpdateCorners(dec, &reg, p10, p11, p01, p00) == DmtxFail);
         break;
      case (DmtxDirRight | DmtxDirUp):
         RETURN_FAIL_IF(dmtxRegionUpdateCorners(dec, &reg, p11, p01, p00, p10) == DmtxFail);
         break;
      case (DmtxDirUp | DmtxDirLeft):
         RETURN_FAIL_IF(dmtxRegionUpdateCorners(dec, &reg, p01, p00, p10, p11) == DmtxFail);
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
