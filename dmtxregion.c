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

#define DMTX_HOUGH_RES 180

/**
 * @file dmtxregion.c
 * @brief Detect barcode regions
 *
 * This file contains region detection logic.
 */

/**
 * @brief  Find next barcode region
 * @param  dec Pointer to DmtxDecode information struct
 * @param  timeout Pointer to timeout time (NULL if none)
 * @return Detected region (if found)
 */
extern DmtxRegion
dmtxDecodeFindNextRegion(DmtxDecode *dec, DmtxTime *timeout)
{
   DmtxScanGrid *grid;
   DmtxPixelLoc loc, locNext;
   DmtxRegion   reg;
/*
   int size, i = 0;
   char imagePath[128];
*/
   grid = &(dec->grid);

   /* Continue scanning until we run out of time or run out of image */
   for(loc = GetGridCoordinates(grid);; loc = locNext) {

      /* Quit if grid has been completely traversed */
      if(loc.status == DMTX_RANGE_EOF) {
         reg.found = DMTX_REGION_EOF;
         break;
      }

      /* First move away from this location to prevent repeat visits */
      locNext = IncrementPixelProgress(grid);

      /* Scan this pixel for presence of a valid barcode edge */
      reg = dmtxRegionScanPixel(dec, loc);
/*
      if(reg.found == DMTX_REGION_FOUND || reg.found > DMTX_REGION_DROPPED_FINDER) {
         size = snprintf(imagePath, 128, "debug_%06d.pnm", i++);
         if(size >= 128)
            exit(1);
         WriteDiagnosticImage(dec, &reg, imagePath);
      }
*/
      /* Found a barcode region? */
      if(reg.found == DMTX_REGION_FOUND)
         break;

      /* Ran out of time? */
      if(timeout != NULL && dmtxTimeExceeded(*timeout)) {
         reg.found = DMTX_REGION_TIMEOUT;
         break;
      }
   }

   return reg;
}

/**
 * @brief  Scan individual pixel for presence of barcode edge
 * @param  dec Pointer to DmtxDecode information struct
 * @param  loc Pixel location
 * @return Detected region (if any)
 */
extern DmtxRegion
dmtxRegionScanPixel(DmtxDecode *dec, DmtxPixelLoc loc)
{
   int offset;
   DmtxRegion reg;
   DmtxPointFlow flowBegin;

   memset(&reg, 0x00, sizeof(DmtxRegion));

   offset = dmtxImageGetOffset(dec->image, loc.X, loc.Y);
/* if(offset == DMTX_BAD_OFFSET || dec->image->cache[offset] & 0x40) { */
   if(offset == DMTX_BAD_OFFSET) {
      reg.found = DMTX_REGION_NOT_FOUND;
      return reg;
   }

   if(dec->image->cache[offset] & 0x80) {
      reg.found = DMTX_REGION_NOT_FOUND;
      return reg;
   }

   /* Test for presence of any reasonable edge at this location */
   flowBegin = MatrixRegionSeekEdge(dec, loc);
   if(flowBegin.mag < 10) {
      reg.found = DMTX_REGION_DROPPED_EDGE;
      return reg;
   }

   /* Determine barcode orientation */
   if(MatrixRegionOrientation(dec, &reg, flowBegin) != DMTX_SUCCESS) {
      reg.found = DMTX_REGION_DROPPED_FINDER;
      return reg;
   }

   if(dmtxRegionUpdateXfrms(dec, &reg) != DMTX_SUCCESS) {
      reg.found = DMTX_REGION_DROPPED_FINDER;
      return reg;
   }

   /* Define top edge */
   if(MatrixRegionAlignCalibEdge(dec, &reg, DmtxEdgeTop) != DMTX_SUCCESS) {
      reg.found = DMTX_REGION_DROPPED_TOP;
      return reg;
   }
   if(dmtxRegionUpdateXfrms(dec, &reg) != DMTX_SUCCESS) {
      reg.found = DMTX_REGION_DROPPED_TOP;
      return reg;
   }

   /* Define right edge */
   if(MatrixRegionAlignCalibEdge(dec, &reg, DmtxEdgeRight) != DMTX_SUCCESS) {
      reg.found = DMTX_REGION_DROPPED_RIGHT;
      return reg;
   }
   if(dmtxRegionUpdateXfrms(dec, &reg) != DMTX_SUCCESS) {
      reg.found = DMTX_REGION_DROPPED_RIGHT;
      return reg;
   }

   CALLBACK_MATRIX(&reg);

   /* Calculate the best fitting symbol size */
   if(MatrixRegionFindSize(dec, &reg) != DMTX_SUCCESS) {
      reg.found = DMTX_REGION_DROPPED_SIZE;
      return reg;
   }

   /* Found a valid matrix region */
   reg.found = DMTX_REGION_FOUND;
   return reg;
}

/**
 *
 *
 */
static DmtxPointFlow
MatrixRegionSeekEdge(DmtxDecode *dec, DmtxPixelLoc loc)
{
   int i;
   int strongIdx;
   DmtxPointFlow flow, flowPlane[3];
   DmtxPointFlow flowPos, flowPosBack;
   DmtxPointFlow flowNeg, flowNegBack;

   /* Find whether red, green, or blue shows the strongest edge */
   strongIdx = 0;
   for(i = 0; i < 3; i++) {
      flowPlane[i] = GetPointFlow(dec, i, loc, dmtxNeighborNone);
      if(i > 0 && flowPlane[i].mag > flowPlane[strongIdx].mag)
         strongIdx = i;
   }

   if(flowPlane[strongIdx].mag < 10)
      return dmtxBlankEdge;

   flow = flowPlane[strongIdx];

   flowPos = FindStrongestNeighbor(dec, flow, +1);
   flowNeg = FindStrongestNeighbor(dec, flow, -1);
   if(flowPos.mag != 0 && flowNeg.mag != 0) {
      flowPosBack = FindStrongestNeighbor(dec, flowPos, -1);
      flowNegBack = FindStrongestNeighbor(dec, flowNeg, +1);
      if(flowPos.arrive == (flowPosBack.arrive+4)%8 &&
            flowNeg.arrive == (flowNegBack.arrive+4)%8) {
         flow.arrive = dmtxNeighborNone;
         CALLBACK_POINT_PLOT(flow.loc, 1, 1, DMTX_DISPLAY_SQUARE);
         return flow;
      }
   }

   return dmtxBlankEdge;
}

/**
 *
 *
 */
static int
MatrixRegionOrientation(DmtxDecode *dec, DmtxRegion *reg, DmtxPointFlow begin)
{
   int err;
   int cross;
   int minArea;
   int scale;
   int symbolShape;
   int maxDiagonal;
   DmtxBestLine line1x, line2x;
   DmtxBestLine line2n, line2p;
   DmtxFollow fTmp;

   if(dec->sizeIdxExpected == DMTX_SYMBOL_SQUARE_AUTO ||
         (dec->sizeIdxExpected >= DmtxSymbol10x10 &&
         dec->sizeIdxExpected <= DmtxSymbol144x144))
      symbolShape = DMTX_SYMBOL_SQUARE_AUTO;
   else if(dec->sizeIdxExpected == DMTX_SYMBOL_RECT_AUTO ||
         (dec->sizeIdxExpected >= DmtxSymbol8x18 &&
         dec->sizeIdxExpected <= DmtxSymbol16x48))
      symbolShape = DMTX_SYMBOL_RECT_AUTO;
   else
      symbolShape = DMTX_SYMBOL_SHAPE_AUTO;

   if(dec->edgeMax != -1) {
      if(symbolShape == DMTX_SYMBOL_RECT_AUTO)
         maxDiagonal = (int)(1.23 * dec->edgeMax + 0.5); /* sqrt(5/4) + 10% */
      else
         maxDiagonal = (int)(1.56 * dec->edgeMax + 0.5); /* sqrt(2) + 10% */
   }
   else {
      maxDiagonal = -1;
   }

   /* Follow to end in both directions */
   err = TrailBlazeContinuous(dec, reg, begin, maxDiagonal);
   if(err == DMTX_FAILURE || reg->stepsTotal < 40) {
      TrailClear(dec, reg, 0x40);
      return DMTX_FAILURE;
   }

   /* Filter out region candidates that are smaller than expected */
   if(dec->edgeMin != -1) {
      scale = dmtxImageGetProp(dec->image, DmtxPropScale);

      if(symbolShape == DMTX_SYMBOL_SQUARE_AUTO)
         minArea = (dec->edgeMin * dec->edgeMin)/(scale * scale);
      else
         minArea = (2 * dec->edgeMin * dec->edgeMin)/(scale * scale);

      if((reg->boundMax.X - reg->boundMin.X) * (reg->boundMax.Y - reg->boundMin.Y) < minArea) {
         TrailClear(dec, reg, 0x40);
         return DMTX_FAILURE;
      }
   }

   line1x = FindBestSolidLine(dec, reg, 0, 0, +1, -1);
   if(line1x.mag < 5) {
      TrailClear(dec, reg, 0x40);
      return DMTX_FAILURE;
   }

   err = FindTravelLimits(dec, reg, &line1x);
   if(line1x.distSq < 100 || line1x.devn * 10 >= sqrt((double)line1x.distSq)) {
      TrailClear(dec, reg, 0x40);
      return DMTX_FAILURE;
   }
   assert(line1x.stepPos >= line1x.stepNeg);

   fTmp = FollowSeek(dec, reg, line1x.stepPos + 5);
   line2p = FindBestSolidLine(dec, reg, fTmp.step, line1x.stepNeg, +1, line1x.angle);

   fTmp = FollowSeek(dec, reg, line1x.stepNeg - 5);
   line2n = FindBestSolidLine(dec, reg, fTmp.step, line1x.stepPos, -1, line1x.angle);
   if(max(line2p.mag, line2n.mag) < 5)
      return DMTX_FAILURE;

   if(line2p.mag > line2n.mag) {
      line2x = line2p;
      err = FindTravelLimits(dec, reg, &line2x);
      if(line2x.distSq < 100 || line2x.devn * 10 >= sqrt((double)line2x.distSq))
         return DMTX_FAILURE;

      cross = ((line1x.locPos.X - line1x.locNeg.X) * (line2x.locPos.Y - line2x.locNeg.Y)) -
            ((line1x.locPos.Y - line1x.locNeg.Y) * (line2x.locPos.X - line2x.locNeg.X));
      if(cross > 0) {
         /* Condition 2 */
         reg->polarity = +1;
         reg->locR = line2x.locPos;
         reg->stepR = line2x.stepPos;
         reg->locT = line1x.locNeg;
         reg->stepT = line1x.stepNeg;
         reg->leftLoc = line1x.locBeg;
         reg->leftAngle = line1x.angle;
         reg->bottomLoc = line2x.locBeg;
         reg->bottomAngle = line2x.angle;
         reg->leftLine = line1x;
         reg->bottomLine = line2x;
      }
      else {
         /* Condition 3 */
         reg->polarity = -1;
         reg->locR = line1x.locNeg;
         reg->stepR = line1x.stepNeg;
         reg->locT = line2x.locPos;
         reg->stepT = line2x.stepPos;
         reg->leftLoc = line2x.locBeg;
         reg->leftAngle = line2x.angle;
         reg->bottomLoc = line1x.locBeg;
         reg->bottomAngle = line1x.angle;
         reg->leftLine = line2x;
         reg->bottomLine = line1x;
      }
   }
   else {
      line2x = line2n;
      err = FindTravelLimits(dec, reg, &line2x);
      if(line2x.distSq < 100 || line2x.devn / sqrt((double)line2x.distSq) >= 0.1)
         return DMTX_FAILURE;

      cross = ((line1x.locNeg.X - line1x.locPos.X) * (line2x.locNeg.Y - line2x.locPos.Y)) -
            ((line1x.locNeg.Y - line1x.locPos.Y) * (line2x.locNeg.X - line2x.locPos.X));
      if(cross > 0) {
         /* Condition 1 */
         reg->polarity = -1;
         reg->locR = line2x.locNeg;
         reg->stepR = line2x.stepNeg;
         reg->locT = line1x.locPos;
         reg->stepT = line1x.stepPos;
         reg->leftLoc = line1x.locBeg;
         reg->leftAngle = line1x.angle;
         reg->bottomLoc = line2x.locBeg;
         reg->bottomAngle = line2x.angle;
         reg->leftLine = line1x;
         reg->bottomLine = line2x;
      }
      else {
         /* Condition 4 */
         reg->polarity = +1;
         reg->locR = line1x.locPos;
         reg->stepR = line1x.stepPos;
         reg->locT = line2x.locNeg;
         reg->stepT = line2x.stepNeg;
         reg->leftLoc = line2x.locBeg;
         reg->leftAngle = line2x.angle;
         reg->bottomLoc = line1x.locBeg;
         reg->bottomAngle = line1x.angle;
         reg->leftLine = line2x;
         reg->bottomLine = line1x;
      }
   }
/* CALLBACK_POINT_PLOT(reg->locR, 2, 1, DMTX_DISPLAY_SQUARE);
   CALLBACK_POINT_PLOT(reg->locT, 2, 1, DMTX_DISPLAY_SQUARE); */

   reg->leftKnown = reg->bottomKnown = 1;

   return DMTX_SUCCESS;
}

/**
 *
 *
 */
static long
DistanceSquared(DmtxPixelLoc a, DmtxPixelLoc b)
{
   long xDelta, yDelta;

   xDelta = a.X - b.X;
   yDelta = a.Y - b.Y;

   return (xDelta * xDelta) + (yDelta * yDelta);
}


/**
 *
 *
 */
static unsigned char *
GetCacheAddress(DmtxDecode *dec, int x, int y)
{
   int offset;

   offset = dmtxImageGetOffset(dec->image, x, y);
   if(offset == DMTX_BAD_OFFSET)
      return NULL;

   return &(dec->image->cache[offset]);
}

/**
 *
 *
 */
extern int
dmtxRegionUpdateCorners(DmtxDecode *dec, DmtxRegion *reg, DmtxVector2 p00,
      DmtxVector2 p10, DmtxVector2 p11, DmtxVector2 p01)
{
   DmtxVector2 vOT, vOR, vTX, vRX, vTmp;
   double tx, ty, phi, shx, scx, scy, skx, sky;
   double dimOT, dimOR, dimTX, dimRX, ratio;
   DmtxMatrix3 m, mtxy, mphi, mshx, mscx, mscy, mscxy, msky, mskx;

   if(dmtxImageContainsFloat(dec->image, p00.X, p00.Y) == DMTX_FALSE ||
         dmtxImageContainsFloat(dec->image, p01.X, p01.Y) == DMTX_FALSE ||
         dmtxImageContainsFloat(dec->image, p10.X, p10.Y) == DMTX_FALSE)
      return DMTX_FAILURE;

   dimOT = dmtxVector2Mag(dmtxVector2Sub(&vOT, &p01, &p00)); /* XXX could use MagSquared() */
   dimOR = dmtxVector2Mag(dmtxVector2Sub(&vOR, &p10, &p00)); /* XXX could use MagSquared() */
   dimTX = dmtxVector2Mag(dmtxVector2Sub(&vTX, &p11, &p01)); /* XXX could use MagSquared() */
   dimRX = dmtxVector2Mag(dmtxVector2Sub(&vRX, &p11, &p10)); /* XXX could use MagSquared() */

   /* Verify that sides are reasonably long */
   if(dimOT <= 8.0 || dimOR <= 8.0 || dimTX <= 8.0 || dimRX <= 8.0)
      return DMTX_FAILURE;

   /* Verify that the 4 corners define a reasonably fat quadrilateral */
   ratio = dimOT / dimRX;
   if(ratio <= 0.5 || ratio >= 2.0)
      return DMTX_FAILURE;

   ratio = dimOR / dimTX;
   if(ratio <= 0.5 || ratio >= 2.0)
      return DMTX_FAILURE;

   /* Verify this is not a bowtie shape */
   if(dmtxVector2Cross(&vOR, &vRX) <= 0.0 ||
         dmtxVector2Cross(&vOT, &vTX) >= 0.0)
      return DMTX_FAILURE;

   if(RightAngleTrueness(p00, p10, p11, M_PI_2) <= dec->squareDevn)
      return DMTX_FAILURE;
   if(RightAngleTrueness(p10, p11, p01, M_PI_2) <= dec->squareDevn)
      return DMTX_FAILURE;

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
   dmtxMatrix3Multiply(reg->raw2fit, m, msky);

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
   dmtxMatrix3Multiply(reg->fit2raw, m, mtxy);

   return DMTX_SUCCESS;
}

/**
 *
 *
 */
extern int
dmtxRegionUpdateXfrms(DmtxDecode *dec, DmtxRegion *reg)
{
   double radians;
   DmtxRay2 rLeft, rBottom, rTop, rRight;
   DmtxVector2 p00, p10, p11, p01;

   assert(reg->leftKnown != 0 && reg->bottomKnown != 0);

   /* Build ray representing left edge */
   rLeft.p.X = reg->leftLoc.X;
   rLeft.p.Y = reg->leftLoc.Y;
   radians = reg->leftAngle * (M_PI/DMTX_HOUGH_RES);
   rLeft.v.X = cos(radians);
   rLeft.v.Y = sin(radians);
   rLeft.tMin = 0.0;
   rLeft.tMax = dmtxVector2Norm(&rLeft.v);

   /* Build ray representing bottom edge */
   rBottom.p.X = reg->bottomLoc.X;
   rBottom.p.Y = reg->bottomLoc.Y;
   radians = reg->bottomAngle * (M_PI/DMTX_HOUGH_RES);
   rBottom.v.X = cos(radians);
   rBottom.v.Y = sin(radians);
   rBottom.tMin = 0.0;
   rBottom.tMax = dmtxVector2Norm(&rBottom.v);

   /* Build ray representing top edge */
   if(reg->topKnown != 0) {
      rTop.p.X = reg->topLoc.X;
      rTop.p.Y = reg->topLoc.Y;
      radians = reg->topAngle * (M_PI/DMTX_HOUGH_RES);
      rTop.v.X = cos(radians);
      rTop.v.Y = sin(radians);
      rTop.tMin = 0.0;
      rTop.tMax = dmtxVector2Norm(&rTop.v);
   }
   else {
      rTop.p.X = reg->locT.X;
      rTop.p.Y = reg->locT.Y;
      radians = reg->bottomAngle * (M_PI/DMTX_HOUGH_RES);
      rTop.v.X = cos(radians);
      rTop.v.Y = sin(radians);
      rTop.tMin = 0.0;
      rTop.tMax = rBottom.tMax;
   }

   /* Build ray representing right edge */
   if(reg->rightKnown != 0) {
      rRight.p.X = reg->rightLoc.X;
      rRight.p.Y = reg->rightLoc.Y;
      radians = reg->rightAngle * (M_PI/DMTX_HOUGH_RES);
      rRight.v.X = cos(radians);
      rRight.v.Y = sin(radians);
      rRight.tMin = 0.0;
      rRight.tMax = dmtxVector2Norm(&rRight.v);
   }
   else {
      rRight.p.X = reg->locR.X;
      rRight.p.Y = reg->locR.Y;
      radians = reg->leftAngle * (M_PI/DMTX_HOUGH_RES);
      rRight.v.X = cos(radians);
      rRight.v.Y = sin(radians);
      rRight.tMin = 0.0;
      rRight.tMax = rLeft.tMax;
   }

   /* Calculate 4 corners, real or imagined */
   if(dmtxRay2Intersect(&p00, &rLeft, &rBottom) == DMTX_FAILURE)
      return DMTX_FAILURE;

   if(dmtxRay2Intersect(&p10, &rBottom, &rRight) == DMTX_FAILURE)
      return DMTX_FAILURE;

   if(dmtxRay2Intersect(&p11, &rRight, &rTop) == DMTX_FAILURE)
      return DMTX_FAILURE;

   if(dmtxRay2Intersect(&p01, &rTop, &rLeft) == DMTX_FAILURE)
      return DMTX_FAILURE;

   if(dmtxRegionUpdateCorners(dec, reg, p00, p10, p11, p01) != DMTX_SUCCESS)
      return DMTX_FAILURE;

   return DMTX_SUCCESS;
}

/**
 *
 *
 */
static double
RightAngleTrueness(DmtxVector2 c0, DmtxVector2 c1, DmtxVector2 c2, double angle)
{
   DmtxVector2 vA, vB;
   DmtxMatrix3 m;

   dmtxVector2Norm(dmtxVector2Sub(&vA, &c0, &c1));
   dmtxVector2Norm(dmtxVector2Sub(&vB, &c2, &c1));

   dmtxMatrix3Rotate(m, angle);
   dmtxMatrix3VMultiplyBy(&vB, m);

   return dmtxVector2Dot(&vA, &vB);
}

/**
 * @brief  XXX
 * @param  image
 * @param  reg
 * @param  symbolRow
 * @param  symbolCol
 * @param  sizeIdx
 * @return Averaged module color
 */
static int
ReadModuleColor(DmtxImage *img, DmtxRegion *reg, int symbolRow, int symbolCol, int sizeIdx)
{
   int i;
   int symbolRows, symbolCols;
   int color;
   double sampleX[] = { 0.5, 0.4, 0.5, 0.6, 0.5 };
   double sampleY[] = { 0.5, 0.5, 0.4, 0.5, 0.6 };
   DmtxVector2 p;

   symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, sizeIdx);
   symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, sizeIdx);

   color = 0;
   for(i = 0; i < 5; i++) {

      p.X = (1.0/symbolCols) * (symbolCol + sampleX[i]);
      p.Y = (1.0/symbolRows) * (symbolRow + sampleY[i]);

      dmtxMatrix3VMultiplyBy(&p, reg->fit2raw);
      color += dmtxImageGetColor(img, (int)(p.X + 0.5), (int)(p.Y + 0.5), reg->flowBegin.plane);
   }

   return color/5;
}

/**
 * @brief  XXX
 * @param  image
 * @param  reg
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
MatrixRegionFindSize(DmtxDecode *dec, DmtxRegion *reg)
{
   int row, col;
   int sizeIdxBeg, sizeIdxEnd;
   int sizeIdx, bestSizeIdx;
   int symbolRows, symbolCols;
   int jumpCount, errors;
   int color;
   int colorOnAvg, bestColorOnAvg;
   int colorOffAvg, bestColorOffAvg;
   int contrast, bestContrast;
   DmtxImage *img;

   img = dec->image;
   bestSizeIdx = -1;
   bestContrast = 0;
   bestColorOnAvg = bestColorOffAvg = 0;

   if(dec->sizeIdxExpected == DMTX_SYMBOL_SHAPE_AUTO) {
      sizeIdxBeg = 0;
      sizeIdxEnd = DMTX_SYMBOL_SQUARE_COUNT + DMTX_SYMBOL_RECT_COUNT;
   }
   else if(dec->sizeIdxExpected == DMTX_SYMBOL_SQUARE_AUTO) {
      sizeIdxBeg = 0;
      sizeIdxEnd = DMTX_SYMBOL_SQUARE_COUNT;
   }
   else if(dec->sizeIdxExpected == DMTX_SYMBOL_RECT_AUTO) {
      sizeIdxBeg = DMTX_SYMBOL_SQUARE_COUNT;
      sizeIdxEnd = DMTX_SYMBOL_SQUARE_COUNT + DMTX_SYMBOL_RECT_COUNT;
   }
   else {
      sizeIdxBeg = dec->sizeIdxExpected;
      sizeIdxEnd = dec->sizeIdxExpected + 1;
   }

   /* Test each barcode size to find best contrast in calibration modules */
   for(sizeIdx = sizeIdxBeg; sizeIdx < sizeIdxEnd; sizeIdx++) {

      symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, sizeIdx);
      symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, sizeIdx);
      colorOnAvg = colorOffAvg = 0;

      /* Sum module colors along horizontal calibration bar */
      row = symbolRows - 1;
      for(col = 0; col < symbolCols; col++) {
         color = ReadModuleColor(img, reg, row, col, sizeIdx);
         if(col & 0x01)
            colorOffAvg += color;
         else
            colorOnAvg += color;
      }

      /* Sum module colors along vertical calibration bar */
      col = symbolCols - 1;
      for(row = 0; row < symbolRows; row++) {
         color = ReadModuleColor(img, reg, row, col, sizeIdx);
         if(row & 0x01)
            colorOffAvg += color;
         else
            colorOnAvg += color;
      }

      colorOnAvg = (colorOnAvg * 2)/(symbolRows + symbolCols);
      colorOffAvg = (colorOffAvg * 2)/(symbolRows + symbolCols);

      contrast = abs(colorOnAvg - colorOffAvg);
      if(contrast < 20)
         continue;

      if(contrast > bestContrast) {
         bestContrast = contrast;
         bestSizeIdx = sizeIdx;
         bestColorOnAvg = colorOnAvg;
         bestColorOffAvg = colorOffAvg;
      }
   }

   /* If no sizes produced acceptable contrast then call it quits */
   if(bestSizeIdx == -1 || bestContrast < 20)
      return DMTX_FAILURE;

   reg->sizeIdx = bestSizeIdx;
   reg->onColor = bestColorOnAvg;
   reg->offColor = bestColorOffAvg;

   reg->symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, reg->sizeIdx);
   reg->symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, reg->sizeIdx);
   reg->mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, reg->sizeIdx);
   reg->mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, reg->sizeIdx);

   /* Tally jumps on horizontal calibration bar to verify sizeIdx */
   jumpCount = CountJumpTally(img, reg, 0, reg->symbolRows - 1, DmtxDirRight);
   errors = abs(1 + jumpCount - reg->symbolCols);
   if(jumpCount < 0 || errors > 2)
      return DMTX_FAILURE;

   /* Tally jumps on vertical calibration bar to verify sizeIdx */
   jumpCount = CountJumpTally(img, reg, reg->symbolCols - 1, 0, DmtxDirUp);
   errors = abs(1 + jumpCount - reg->symbolRows);
   if(jumpCount < 0 || errors > 2)
      return DMTX_FAILURE;

   /* Tally jumps on horizontal finder bar to verify sizeIdx */
   errors = CountJumpTally(img, reg, 0, 0, DmtxDirRight);
   if(jumpCount < 0 || errors > 2)
      return DMTX_FAILURE;

   /* Tally jumps on vertical finder bar to verify sizeIdx */
   errors = CountJumpTally(img, reg, 0, 0, DmtxDirUp);
   if(errors < 0 || errors > 2)
      return DMTX_FAILURE;

   /* Tally jumps on surrounding whitespace, else fail */
   errors = CountJumpTally(img, reg, 0, -1, DmtxDirRight);
   if(errors < 0 || errors > 2)
      return DMTX_FAILURE;

   errors = CountJumpTally(img, reg, -1, 0, DmtxDirUp);
   if(errors < 0 || errors > 2)
      return DMTX_FAILURE;

   errors = CountJumpTally(img, reg, 0, reg->symbolRows, DmtxDirRight);
   if(errors < 0 || errors > 2)
      return DMTX_FAILURE;

   errors = CountJumpTally(img, reg, reg->symbolCols, 0, DmtxDirUp);
   if(errors < 0 || errors > 2)
      return DMTX_FAILURE;

   return DMTX_SUCCESS;
}

/**
 * @brief  XXX
 * @param  img
 * @param  reg
 * @param  xStart
 * @param  yStart
 * @param  dir
 * @return Jump count
 */
static int
CountJumpTally(DmtxImage *img, DmtxRegion *reg, int xStart, int yStart, DmtxDirection dir)
{
   int x, xInc = 0;
   int y, yInc = 0;
   int state = DMTX_MODULE_ON;
   int jumpCount = 0;
   int jumpThreshold;
   int tModule, tPrev;
   int darkOnLight;
   int color;

   assert(xStart == 0 || yStart == 0);
   assert(dir == DmtxDirRight || dir == DmtxDirUp);

   if(dir == DmtxDirRight)
      xInc = 1;
   else
      yInc = 1;

   if(xStart == -1 || xStart == reg->symbolCols ||
         yStart == -1 || yStart == reg->symbolRows)
      state = DMTX_MODULE_OFF;

   darkOnLight = (int)(reg->offColor > reg->onColor);
   jumpThreshold = abs((int)(0.4 * (reg->onColor - reg->offColor) + 0.5));
   color = ReadModuleColor(img, reg, yStart, xStart, reg->sizeIdx);
   tModule = (darkOnLight) ? reg->offColor - color : color - reg->offColor;

   for(x = xStart + xInc, y = yStart + yInc;
         (dir == DmtxDirRight && x < reg->symbolCols) ||
         (dir == DmtxDirUp && y < reg->symbolRows);
         x += xInc, y += yInc) {

      tPrev = tModule;
      color = ReadModuleColor(img, reg, y, x, reg->sizeIdx);
      tModule = (darkOnLight) ? reg->offColor - color : color - reg->offColor;

      if(state == DMTX_MODULE_OFF) {
         if(tModule > tPrev + jumpThreshold) {
            jumpCount++;
            state = DMTX_MODULE_ON;
         }
      }
      else {
         if(tModule < tPrev - jumpThreshold) {
            jumpCount++;
            state = DMTX_MODULE_OFF;
         }
      }
   }

   return jumpCount;
}

/**
 *
 *
 */
static DmtxPointFlow
GetPointFlow(DmtxDecode *dec, int colorPlane, DmtxPixelLoc loc, int arrive)
{
   static const int coefficient[] = {  0,  1,  2,  1,  0, -1, -2, -1 };
   int patternIdx, coefficientIdx;
   int compass, compassMax;
   int mag[4] = { 0 };
   int xAdjust, yAdjust;
   int color, colorPattern[8];
   DmtxPointFlow flow;

   for(patternIdx = 0; patternIdx < 8; patternIdx++) {
      xAdjust = loc.X + dmtxPatternX[patternIdx];
      yAdjust = loc.Y + dmtxPatternY[patternIdx];
      colorPattern[patternIdx] = dmtxImageGetColor(dec->image, xAdjust, yAdjust, colorPlane);
      if(colorPattern[patternIdx] == -1)
         return dmtxBlankEdge;
   }

   /* Calculate this pixel's flow intensity for each direction (-45, 0, 45, 90) */
   compassMax = 0;
   for(compass = 0; compass < 4; compass++) {

      /* Add portion from each position in the convolution matrix pattern */
      for(patternIdx = 0; patternIdx < 8; patternIdx++) {

         coefficientIdx = (patternIdx - compass + 8) % 8;
         if(coefficient[coefficientIdx] == 0)
            continue;

         color = colorPattern[patternIdx];

         switch(coefficient[coefficientIdx]) {
            case 2:
               mag[compass] += color;
               /* Fall through */
            case 1:
               mag[compass] += color;
               break;
            case -2:
               mag[compass] -= color;
               /* Fall through */
            case -1:
               mag[compass] -= color;
               break;
         }
      }

      /* Identify strongest compass flow */
      if(compass != 0 && abs(mag[compass]) > abs(mag[compassMax]))
         compassMax = compass;
   }

   /* Convert signed compass direction into unique flow directions (0-7) */
   flow.plane = colorPlane;
   flow.arrive = arrive;
   flow.depart = (mag[compassMax] > 0) ? compassMax + 4 : compassMax;
   flow.mag = abs(mag[compassMax]);
   flow.loc = loc;

   return flow;
}

/**
 *
 *
 */
static DmtxPointFlow
FindStrongestNeighbor(DmtxDecode *dec, DmtxPointFlow center, int sign)
{
   int i;
   int offset;
   int strongIdx;
   int attempt, attemptDiff;
   int occupied;
   unsigned char *cache;
   DmtxPixelLoc loc;
   DmtxPointFlow flow[8];

   attempt = (sign < 0) ? center.depart : (center.depart+4)%8;

   occupied = 0;
   strongIdx = -1;
   for(i = 0; i < 8; i++) {

      loc.X = center.loc.X + dmtxPatternX[i];
      loc.Y = center.loc.Y + dmtxPatternY[i];

      offset = dmtxImageGetOffset(dec->image, loc.X, loc.Y);
      if(offset == DMTX_BAD_OFFSET) {
         loc.status = DMTX_RANGE_BAD;
         continue;
      }
      else {
         loc.status = DMTX_RANGE_GOOD;
      }

      cache = &(dec->image->cache[offset]);
      if(*cache & 0x80) {
         if(++occupied > 2)
            return dmtxBlankEdge;
         else
            continue;
      }

      attemptDiff = abs(attempt - i);
      if(attemptDiff > 4)
         attemptDiff = 8 - attemptDiff;
      if(attemptDiff > 1)
         continue;

      flow[i] = GetPointFlow(dec, center.plane, loc, i);

      if(strongIdx == -1 || flow[i].mag > flow[strongIdx].mag ||
            (flow[i].mag == flow[strongIdx].mag && ((i & 0x01) != 0))) {
         strongIdx = i;
      }
   }

   return (strongIdx == -1) ? dmtxBlankEdge : flow[strongIdx];
}

/**
 *
 *
 */
static DmtxFollow
FollowSeek(DmtxDecode *dec, DmtxRegion *reg, int seek)
{
   int i;
   int sign;
   int offset;
   DmtxFollow follow;

   follow.loc = reg->flowBegin.loc;
   offset = dmtxImageGetOffset(dec->image, follow.loc.X, follow.loc.Y);
   assert(offset != DMTX_BAD_OFFSET);

   follow.step = 0;
   follow.ptr = &(dec->image->cache[offset]);
   follow.neighbor = *follow.ptr;

   sign = (seek > 0) ? +1 : -1;
   for(i = 0; i != seek; i += sign) {
      follow = FollowStep(dec, reg, follow, sign);
      assert(follow.ptr != NULL);
      assert(abs(follow.step) <= reg->stepsTotal);
   }

   return follow;
}

/**
 *
 *
 */
static DmtxFollow
FollowSeekLoc(DmtxDecode *dec, DmtxRegion *reg, DmtxPixelLoc loc)
{
   int offset;
   DmtxFollow follow;

   follow.loc = loc;
   offset = dmtxImageGetOffset(dec->image, follow.loc.X, follow.loc.Y);
   assert(offset != DMTX_BAD_OFFSET);

   follow.step = 0;
   follow.ptr = &(dec->image->cache[offset]);
   follow.neighbor = *follow.ptr;

   return follow;
}


/**
 *
 *
 */
static DmtxFollow
FollowStep(DmtxDecode *dec, DmtxRegion *reg, DmtxFollow followBeg, int sign)
{
   int offset;
   int patternIdx;
   int stepMod;
   int factor;
   DmtxFollow follow;

   assert(abs(sign) == 1);
   assert(followBeg.neighbor & 0x40);

   factor = reg->stepsTotal + 1;
   if(sign > 0)
      stepMod = (factor + (followBeg.step % factor)) % factor;
   else
      stepMod = (factor - (followBeg.step % factor)) % factor;

   /* End of positive trail -- magic jump */
   if(sign > 0 && stepMod == reg->jumpToNeg) {
      follow.loc = reg->finalNeg;
   }
   /* End of negative trail -- magic jump */
   else if(sign < 0 && stepMod == reg->jumpToPos) {
      follow.loc = reg->finalPos;
   }
   /* Trail in progress -- normal jump */
   else {
      patternIdx = (sign < 0) ? followBeg.neighbor & 0x07 : ((followBeg.neighbor & 0x38) >> 3);
      follow.loc.X = followBeg.loc.X + dmtxPatternX[patternIdx];
      follow.loc.Y = followBeg.loc.Y + dmtxPatternY[patternIdx];
      follow.loc.status = DMTX_RANGE_GOOD;
   }

   offset = dmtxImageGetOffset(dec->image, follow.loc.X, follow.loc.Y);
   assert(offset != DMTX_BAD_OFFSET);

   follow.step = followBeg.step + sign;
   follow.ptr = &(dec->image->cache[offset]);
   follow.neighbor = *follow.ptr;

   return follow;
}

/**
 *
 *
 */
static DmtxFollow
FollowStep2(DmtxDecode *dec, DmtxRegion *reg, DmtxFollow followBeg, int sign)
{
   int offset;
   int patternIdx;
   DmtxFollow follow;

   assert(abs(sign) == 1);
   assert(followBeg.neighbor & 0x40);

   patternIdx = (sign < 0) ? followBeg.neighbor & 0x07 : ((followBeg.neighbor & 0x38) >> 3);
   follow.loc.X = followBeg.loc.X + dmtxPatternX[patternIdx];
   follow.loc.Y = followBeg.loc.Y + dmtxPatternY[patternIdx];
   follow.loc.status = DMTX_RANGE_GOOD;

   offset = dmtxImageGetOffset(dec->image, follow.loc.X, follow.loc.Y);
   assert(offset != DMTX_BAD_OFFSET);

   follow.step = followBeg.step + sign;
   follow.ptr = &(dec->image->cache[offset]);
   follow.neighbor = *follow.ptr;

   return follow;
}

/**
 * vaiiiooo
 * --------
 * 0x80 v = visited bit
 * 0x40 a = assigned bit
 * 0x38 u = 3 bits points upstream 0-7
 * 0x07 d = 3 bits points downstream 0-7
 */
static int
TrailBlazeContinuous(DmtxDecode *dec, DmtxRegion *reg, DmtxPointFlow flowBegin, int maxDiagonal)
{
   int posAssigns, negAssigns, clears;
   int sign;
   int steps;
   int offset;
   unsigned char *cache, *cacheNext, *cacheBeg;
   DmtxPointFlow flow, flowNext;
   DmtxPixelLoc boundMin, boundMax;

   /* check offset before starting */
   offset = dmtxImageGetOffset(dec->image, flowBegin.loc.X, flowBegin.loc.Y);
   if(offset == DMTX_BAD_OFFSET)
      return DMTX_FAILURE;

   boundMin = boundMax = flowBegin.loc;
   cacheBeg = &(dec->image->cache[offset]);
   *cacheBeg = (0x80 | 0x40); /* Mark location as visited and assigned */

   reg->flowBegin = flowBegin;

   posAssigns = negAssigns = 0;
   for(sign = 1; sign >= -1; sign -= 2) {

      flow = flowBegin;
      cache = cacheBeg;

      for(steps = 0; ; steps++) {

         if(maxDiagonal != -1 && (boundMax.X - boundMin.X > maxDiagonal ||
               boundMax.Y - boundMin.Y > maxDiagonal))
            break;

         /* Find the strongest eligible neighbor */
         flowNext = FindStrongestNeighbor(dec, flow, sign);
         if(flowNext.mag < 50)
            break;

         offset = dmtxImageGetOffset(dec->image, flowNext.loc.X, flowNext.loc.Y);
         if(offset == DMTX_BAD_OFFSET)
            break;

         /* Get the neighbor's cache location */
         cacheNext = &(dec->image->cache[offset]);
         assert(!(*cacheNext & 0x80));

         /* Mark departure from current location. If flowing downstream
          * (sign < 0) then departure vector here is the arrival vector
          * of the next location. Upstream flow uses the opposite rule. */
         *cache |= (sign < 0) ? flowNext.arrive : flowNext.arrive << 3;

         /* Mark known direction for next location */
         /* If testing downstream (sign < 0) then next upstream is opposite of next arrival */
         /* If testing upstream (sign > 0) then next downstream is opposite of next arrival */
         *cacheNext = (sign < 0) ? (((flowNext.arrive + 4)%8) << 3) : ((flowNext.arrive + 4)%8);
         *cacheNext |= (0x80 | 0x40); /* Mark location as visited and assigned */
         if(sign > 0)
            posAssigns++;
         else
            negAssigns++;
         cache = cacheNext;
         flow = flowNext;

         if(flow.loc.X > boundMax.X)
            boundMax.X = flow.loc.X;
         else if(flow.loc.X < boundMin.X)
            boundMin.X = flow.loc.X;
         if(flow.loc.Y > boundMax.Y)
            boundMax.Y = flow.loc.Y;
         else if(flow.loc.Y < boundMin.Y)
            boundMin.Y = flow.loc.Y;

/*       CALLBACK_POINT_PLOT(flow.loc, (sign > 0) ? 2 : 3, 1, DMTX_DISPLAY_POINT); */
      }

      if(sign > 0) {
         reg->finalPos = flow.loc;
         reg->jumpToNeg = steps;
      }
      else {
         reg->finalNeg = flow.loc;
         reg->jumpToPos = steps;
      }
   }
   reg->stepsTotal = reg->jumpToPos + reg->jumpToNeg;
   reg->boundMin = boundMin;
   reg->boundMax = boundMax;

   /* Clear "visited" bit from trail */
   clears = TrailClear(dec, reg, 0x80);
   assert(posAssigns + negAssigns == clears - 1);

   /* XXX clean this up ... redundant test above */
   if(maxDiagonal != -1 && (boundMax.X - boundMin.X > maxDiagonal ||
         boundMax.Y - boundMin.Y > maxDiagonal))
      return DMTX_FAILURE;

   return DMTX_SUCCESS;
}

/**
 * recives bresline, and follows strongest neighbor unless it involves
 * ratcheting bresline inward or backward (although back + outward is allowed).
 *
 */
static int
TrailBlazeGapped(DmtxDecode *dec, DmtxRegion *reg, DmtxBresLine line, int streamDir)
{
   unsigned char *beforeCache, *afterCache;
   int err;
   int onEdge;
   int distSq, distSqMax;
   int travel, outward;
   int xDiff, yDiff;
   int steps;
   int stepDir, dirMap[] = { 0, 1, 2, 7, 8, 3, 6, 5, 4 };
   DmtxPixelLoc beforeStep, afterStep;
   DmtxPointFlow flow, flowNext;
   DmtxPixelLoc loc0;
   int xStep, yStep;

   loc0 = line.loc;
   flow = GetPointFlow(dec, reg->flowBegin.plane, loc0, dmtxNeighborNone);
   distSqMax = (line.xDelta * line.xDelta) + (line.yDelta * line.yDelta);
   steps = 0;
   onEdge = 1;

   beforeStep = loc0;
   beforeCache = GetCacheAddress(dec, loc0.X, loc0.Y);
   if(beforeCache == NULL)
      return DMTX_FAILURE;
   else
      *beforeCache = 0x00; /* probably should just overwrite one direction */

   do {
      if(onEdge) {
         flowNext = FindStrongestNeighbor(dec, flow, streamDir);
         if(flowNext.mag == -1)
            break;

         err = BresLineGetStep(line, flowNext.loc, &travel, &outward);
         if(flowNext.mag < 50 || outward < 0 || (outward == 0 && travel < 0)) {
            onEdge = 0;
         }
         else {
            BresLineStep(&line, travel, outward);
            flow = flowNext;
         }
      }

      if(!onEdge) {
         BresLineStep(&line, 1, 0);
         flow = GetPointFlow(dec, reg->flowBegin.plane, line.loc, dmtxNeighborNone);
         if(flow.mag > 50)
            onEdge = 1;
      }

      afterStep = line.loc;
      afterCache = GetCacheAddress(dec, afterStep.X, afterStep.Y);
      if(afterCache == NULL)
         break;

      /* Determine step direction using pure magic */
      xStep = afterStep.X - beforeStep.X;
      yStep = afterStep.Y - beforeStep.Y;
      assert(abs(xStep) <= 1 && abs(yStep) <= 1);
      stepDir = dirMap[3 * yStep + xStep + 4];
      assert(stepDir != 8);

      if(streamDir < 0) {
         *beforeCache |= (0x40 | stepDir);
         *afterCache = (((stepDir + 4)%8) << 3);
      }
      else {
         *beforeCache |= (0x40 | (stepDir << 3));
         *afterCache = ((stepDir + 4)%8);
      }

      /* Guaranteed to have taken one step since top of loop */
      xDiff = line.loc.X - loc0.X;
      yDiff = line.loc.Y - loc0.Y;
      distSq = (xDiff * xDiff) + (yDiff * yDiff);

      beforeStep = line.loc;
      beforeCache = afterCache;
      steps++;

   } while(distSq < distSqMax);

   return steps;
}

/**
 *
 *
 */
static int
TrailClear(DmtxDecode *dec, DmtxRegion *reg, int clearMask)
{
   int clears;
   DmtxFollow follow;

   assert((clearMask | 0xff) == 0xff);

   /* Clear "visited" bit from trail */
   clears = 0;
   follow = FollowSeek(dec, reg, 0);
   while(abs(follow.step) <= reg->stepsTotal) {
      assert((int)(*follow.ptr & clearMask) != 0x00);
      *follow.ptr &= (clearMask ^ 0xff);
      follow = FollowStep(dec, reg, follow, +1);
      clears++;
   }

   return clears;
}

/**
 *
 *
 */
static DmtxBestLine
FindBestSolidLine(DmtxDecode *dec, DmtxRegion *reg, int step0, int step1, int streamDir, int houghAvoid)
{
   int hough[3][DMTX_HOUGH_RES] = { { 0 } };
   int houghMin, houghMax;
   char houghTest[DMTX_HOUGH_RES];
   int i;
   int step;
   int sign;
   int tripSteps;
   int angleBest;
   int hOffset, hOffsetBest;
   int xDiff, yDiff;
   int dH;
   DmtxRay2 rH;
   DmtxFollow follow;
   DmtxBestLine line;
   DmtxPixelLoc rHp;

   memset(&line, 0x00, sizeof(DmtxBestLine));
   memset(&rH, 0x00, sizeof(DmtxRay2));
   angleBest = 0;
   hOffset = hOffsetBest = 0;

   /* Always follow path flowing away from the trail start */
   if(step0 != 0) {
      if(step0 > 0) {
         sign = +1;
         tripSteps = (step1 - step0 + reg->stepsTotal) % reg->stepsTotal;
      }
      else {
         sign = -1;
         tripSteps = (step0 - step1 + reg->stepsTotal) % reg->stepsTotal;
      }
      if(tripSteps == 0)
         tripSteps = reg->stepsTotal;
   }
   else if(step1 != 0) {
      sign = (step1 > 0) ? +1 : -1;
      tripSteps = abs(step1);
   }
   else if(step1 == 0) {
      sign = +1;
      tripSteps = reg->stepsTotal;
   }
   assert(sign == streamDir);

   follow = FollowSeek(dec, reg, step0);
   rHp = follow.loc;

   line.stepBeg = line.stepPos = line.stepNeg = step0;
   line.locBeg = follow.loc;
   line.locPos = follow.loc;
   line.locNeg = follow.loc;

   /* Predetermine which angles to test */
   for(i = 0; i < DMTX_HOUGH_RES; i++) {
      if(houghAvoid == -1) {
         houghTest[i] = 1;
      }
      else {
         houghMin = (houghAvoid + DMTX_HOUGH_RES/6) % DMTX_HOUGH_RES;
         houghMax = (houghAvoid - DMTX_HOUGH_RES/6 + DMTX_HOUGH_RES) % DMTX_HOUGH_RES;
         if(houghMin > houghMax)
            houghTest[i] = (i > houghMin || i < houghMax) ? 1 : 0;
         else
            houghTest[i] = (i > houghMin && i < houghMax) ? 1 : 0;
      }
   }

   /* Test each angle for steps along path */
   for(step = 0; step < tripSteps; step++) {

      xDiff = follow.loc.X - rHp.X;
      yDiff = follow.loc.Y - rHp.Y;

      /* Increment Hough accumulator */
      for(i = 0; i < DMTX_HOUGH_RES; i++) {

         if(houghTest[i] == 0)
            continue;

         dH = (rHvX[i] * yDiff) - (rHvY[i] * xDiff);
         if(dH >= -384 && dH <= 384) {

            if(dH > 128)
               hOffset = 2;
            else if(dH >= -128)
               hOffset = 1;
            else
               hOffset = 0;

            hough[hOffset][i]++;

            /* New angle takes over lead */
            if(hough[hOffset][i] > hough[hOffsetBest][angleBest]) {
               angleBest = i;
               hOffsetBest = hOffset;
            }
         }
      }

/*    CALLBACK_POINT_PLOT(follow.loc, (sign > 1) ? 4 : 3, 1, DMTX_DISPLAY_POINT); */

      follow = FollowStep(dec, reg, follow, sign);
   }

   line.angle = angleBest;
   line.hOffset = hOffsetBest;
   line.mag = hough[hOffsetBest][angleBest];

   return line;
}

/**
 *
 *
 */
static DmtxBestLine
FindBestSolidLine2(DmtxDecode *dec, DmtxRegion *reg, DmtxPixelLoc loc0, int tripSteps, int sign, int houghAvoid)
{
   int hough[3][DMTX_HOUGH_RES] = { { 0 } };
   int houghMin, houghMax;
   char houghTest[DMTX_HOUGH_RES];
   int i;
   int step;
   int angleBest;
   int hOffset, hOffsetBest;
   int xDiff, yDiff;
   int dH;
   DmtxRay2 rH;
   DmtxBestLine line;
   DmtxPixelLoc rHp;
   DmtxFollow follow;

   memset(&line, 0x00, sizeof(DmtxBestLine));
   memset(&rH, 0x00, sizeof(DmtxRay2));
   angleBest = 0;
   hOffset = hOffsetBest = 0;

   follow = FollowSeekLoc(dec, reg, loc0);
   rHp = line.locBeg = line.locPos = line.locNeg = follow.loc;
   line.stepBeg = line.stepPos = line.stepNeg = 0;

   /* Predetermine which angles to test */
   for(i = 0; i < DMTX_HOUGH_RES; i++) {
      if(houghAvoid == -1) {
         houghTest[i] = 1;
      }
      else {
         houghMin = (houghAvoid + DMTX_HOUGH_RES/6) % DMTX_HOUGH_RES;
         houghMax = (houghAvoid - DMTX_HOUGH_RES/6 + DMTX_HOUGH_RES) % DMTX_HOUGH_RES;
         if(houghMin > houghMax)
            houghTest[i] = (i > houghMin || i < houghMax) ? 1 : 0;
         else
            houghTest[i] = (i > houghMin && i < houghMax) ? 1 : 0;
      }
   }

   /* Test each angle for steps along path */
   for(step = 0; step < tripSteps; step++) {

      xDiff = follow.loc.X - rHp.X;
      yDiff = follow.loc.Y - rHp.Y;

      /* Increment Hough accumulator */
      for(i = 0; i < DMTX_HOUGH_RES; i++) {

         if(houghTest[i] == 0)
            continue;

         dH = (rHvX[i] * yDiff) - (rHvY[i] * xDiff);
         if(dH >= -384 && dH <= 384) {
            if(dH > 128)
               hOffset = 2;
            else if(dH >= -128)
               hOffset = 1;
            else
               hOffset = 0;

            hough[hOffset][i]++;

            /* New angle takes over lead */
            if(hough[hOffset][i] > hough[hOffsetBest][angleBest]) {
               angleBest = i;
               hOffsetBest = hOffset;
            }
         }
      }

/*    CALLBACK_POINT_PLOT(follow.loc, (sign > 1) ? 4 : 3, 1, DMTX_DISPLAY_POINT); */

      follow = FollowStep2(dec, reg, follow, sign);
   }

   line.angle = angleBest;
   line.hOffset = hOffsetBest;
   line.mag = hough[hOffsetBest][angleBest];

   return line;
}

/**
 *
 *
 */
static int
FindTravelLimits(DmtxDecode *dec, DmtxRegion *reg, DmtxBestLine *line)
{
   int i;
   int distSq, distSqMax;
   int xDiff, yDiff;
   int posRunning, negRunning;
   int posTravel, negTravel;
   int posWander, posWanderMin, posWanderMax, posWanderMinLock, posWanderMaxLock;
   int negWander, negWanderMin, negWanderMax, negWanderMinLock, negWanderMaxLock;
   int cosAngle, sinAngle;
   DmtxFollow followPos, followNeg;
   DmtxPixelLoc loc0, posMax, negMax;

   /* line->stepBeg is already known to sit on the best Hough line */
   followPos = followNeg = FollowSeek(dec, reg, line->stepBeg);
   loc0 = followPos.loc;

   cosAngle = rHvX[line->angle];
   sinAngle = rHvY[line->angle];

   distSqMax = 0;
   posMax = negMax = followPos.loc;

   posTravel = negTravel = 0;
   posWander = posWanderMin = posWanderMax = posWanderMinLock = posWanderMaxLock = 0;
   negWander = negWanderMin = negWanderMax = negWanderMinLock = negWanderMaxLock = 0;

   for(i = 0; i < reg->stepsTotal/2; i++) {

      posRunning = (int)(i < 10 || abs(posWander) < abs(posTravel));
      negRunning = (int)(i < 10 || abs(negWander) < abs(negTravel));

      if(posRunning) {
         xDiff = followPos.loc.X - loc0.X;
         yDiff = followPos.loc.Y - loc0.Y;
         posTravel = (cosAngle * xDiff) + (sinAngle * yDiff);
         posWander = (cosAngle * yDiff) - (sinAngle * xDiff);

         if(posWander >= -3*256 && posWander <= 3*256) {
            distSq = DistanceSquared(followPos.loc, negMax);
            if(distSq > distSqMax) {
               posMax = followPos.loc;
               distSqMax = distSq;
               line->stepPos = followPos.step;
               line->locPos = followPos.loc;
               posWanderMinLock = posWanderMin;
               posWanderMaxLock = posWanderMax;
            }
         }
         else {
            posWanderMin = min(posWanderMin, posWander);
            posWanderMax = max(posWanderMax, posWander);
         }
      }
      else if(!negRunning) {
         break;
      }

      if(negRunning) {
         xDiff = followNeg.loc.X - loc0.X;
         yDiff = followNeg.loc.Y - loc0.Y;
         negTravel = (cosAngle * xDiff) + (sinAngle * yDiff);
         negWander = (cosAngle * yDiff) - (sinAngle * xDiff);

         if(negWander >= -3*256 && negWander < 3*256) {
            distSq = DistanceSquared(followNeg.loc, posMax);
            if(distSq > distSqMax) {
               negMax = followNeg.loc;
               distSqMax = distSq;
               line->stepNeg = followNeg.step;
               line->locNeg = followNeg.loc;
               negWanderMinLock = negWanderMin;
               negWanderMaxLock = negWanderMax;
            }
         }
         else {
            negWanderMin = min(negWanderMin, negWander);
            negWanderMax = max(negWanderMax, negWander);
         }
      }
      else if(!posRunning) {
         break;
      }

/*  CALLBACK_POINT_PLOT(followPos.loc, 2, 1, DMTX_DISPLAY_POINT);
    CALLBACK_POINT_PLOT(followNeg.loc, 4, 1, DMTX_DISPLAY_POINT); */

      followPos = FollowStep(dec, reg, followPos, +1);
      followNeg = FollowStep(dec, reg, followNeg, -1);
   }
   line->devn = max(posWanderMaxLock - posWanderMinLock, negWanderMaxLock - negWanderMinLock)/256;
   line->distSq = distSqMax;

/* CALLBACK_POINT_PLOT(posMax, 2, 1, DMTX_DISPLAY_SQUARE);
   CALLBACK_POINT_PLOT(negMax, 2, 1, DMTX_DISPLAY_SQUARE); */

   return DMTX_SUCCESS;
}

/**
 *
 *
 */
static int
MatrixRegionAlignCalibEdge(DmtxDecode *dec, DmtxRegion *reg, int edgeLoc)
{
   int streamDir;
   int steps;
   int avoidAngle;
   int symbolShape;
   DmtxVector2 pTmp;
   DmtxPixelLoc loc0, loc1, locOrigin;
   DmtxBresLine line;
   DmtxFollow follow;
   DmtxBestLine bestLine;

   /* Determine pixel coordinates of origin */
   pTmp.X = 0.0;
   pTmp.Y = 0.0;
   dmtxMatrix3VMultiplyBy(&pTmp, reg->fit2raw);
   locOrigin.X = (int)(pTmp.X + 0.5);
   locOrigin.Y = (int)(pTmp.Y + 0.5);
   locOrigin.status = DMTX_RANGE_GOOD;

   if(dec->sizeIdxExpected == DMTX_SYMBOL_SQUARE_AUTO ||
         (dec->sizeIdxExpected >= DmtxSymbol10x10 &&
         dec->sizeIdxExpected <= DmtxSymbol144x144))
      symbolShape = DMTX_SYMBOL_SQUARE_AUTO;
   else if(dec->sizeIdxExpected == DMTX_SYMBOL_RECT_AUTO ||
         (dec->sizeIdxExpected >= DmtxSymbol8x18 &&
         dec->sizeIdxExpected <= DmtxSymbol16x48))
      symbolShape = DMTX_SYMBOL_RECT_AUTO;
   else
      symbolShape = DMTX_SYMBOL_SHAPE_AUTO;

   /* Determine end locations of test line */
   if(edgeLoc == DmtxEdgeTop) {
      streamDir = reg->polarity * -1;
      avoidAngle = reg->leftLine.angle;
      follow = FollowSeekLoc(dec, reg, reg->locT);
      pTmp.X = 0.8;
      pTmp.Y = (symbolShape == DMTX_SYMBOL_RECT_AUTO) ? 0.2 : 0.6;
   }
   else {
      assert(edgeLoc == DmtxEdgeRight);
      streamDir = reg->polarity;
      avoidAngle = reg->bottomLine.angle;
      follow = FollowSeekLoc(dec, reg, reg->locR);
      pTmp.X = (symbolShape == DMTX_SYMBOL_SQUARE_AUTO) ? 0.7 : 0.9;
      pTmp.Y = 0.8;
   }

   dmtxMatrix3VMultiplyBy(&pTmp, reg->fit2raw);
   loc1.X = (int)(pTmp.X + 0.5);
   loc1.Y = (int)(pTmp.Y + 0.5);
   loc1.status = DMTX_RANGE_GOOD;

   loc0 = follow.loc;
   line = BresLineInit(loc0, loc1, locOrigin);
   steps = TrailBlazeGapped(dec, reg, line, streamDir);

   bestLine = FindBestSolidLine2(dec, reg, loc0, steps, streamDir, avoidAngle);
   if(bestLine.mag < 5) {
      ;
   }

   if(edgeLoc == DmtxEdgeTop) {
      reg->topKnown = 1;
      reg->topAngle = bestLine.angle;
      reg->topLoc = bestLine.locBeg;
   }
   else {
      reg->rightKnown = 1;
      reg->rightAngle = bestLine.angle;
      reg->rightLoc = bestLine.locBeg;
   }

   return DMTX_SUCCESS;
}

/**
 *
 *
 */
static DmtxBresLine
BresLineInit(DmtxPixelLoc loc0, DmtxPixelLoc loc1, DmtxPixelLoc locInside)
{
   int cp;
   DmtxBresLine line;
   DmtxPixelLoc *locBeg, *locEnd;

   /* XXX Verify that loc0 and loc1 are inbounds */

   /* Values that stay the same after initialization */
   line.loc0 = loc0;
   line.loc1 = loc1;
   line.xStep = (loc0.X < loc1.X) ? +1 : -1;
   line.yStep = (loc0.Y < loc1.Y) ? +1 : -1;
   line.xDelta = abs(loc1.X - loc0.X);
   line.yDelta = abs(loc1.Y - loc0.Y);
   line.steep = (int)(line.yDelta > line.xDelta);

   /* Take cross product to determine outward step */
   if(line.steep) {
      /* Point first vector up to get correct sign */
      if(loc0.Y < loc1.Y) {
         locBeg = &loc0;
         locEnd = &loc1;
      }
      else {
         locBeg = &loc1;
         locEnd = &loc0;
      }
      cp = (((locEnd->X - locBeg->X) * (locInside.Y - locEnd->Y)) -
            ((locEnd->Y - locBeg->Y) * (locInside.X - locEnd->X)));

      line.xOut = (cp > 0) ? +1 : -1;
      line.yOut = 0;
   }
   else {
      /* Point first vector left to get correct sign */
      if(loc0.X > loc1.X) {
         locBeg = &loc0;
         locEnd = &loc1;
      }
      else {
         locBeg = &loc1;
         locEnd = &loc0;
      }
      cp = (((locEnd->X - locBeg->X) * (locInside.Y - locEnd->Y)) -
            ((locEnd->Y - locBeg->Y) * (locInside.X - locEnd->X)));

      line.xOut = 0;
      line.yOut = (cp > 0) ? +1 : -1;
   }

   /* Values that change while stepping through line */
   line.loc = loc0;
   line.travel = 0;
   line.outward = 0;
   line.error = (line.steep) ? line.yDelta/2 : line.xDelta/2;

/* CALLBACK_POINT_PLOT(loc0, 3, 1, DMTX_DISPLAY_SQUARE);
   CALLBACK_POINT_PLOT(loc1, 3, 1, DMTX_DISPLAY_SQUARE); */

   return line;
}

/**
 *
 *
 */
static int
BresLineGetStep(DmtxBresLine line, DmtxPixelLoc target, int *travel, int *outward)
{
   /* Determine necessary step along and outward from Bresenham line */
   if(line.steep) {
      *travel = (line.yStep > 0) ? target.Y - line.loc.Y : line.loc.Y - target.Y;
      BresLineStep(&line, *travel, 0);
      *outward = (line.xOut > 0) ? target.X - line.loc.X : line.loc.X - target.X;
      assert(line.yOut == 0);
   }
   else {
      *travel = (line.xStep > 0) ? target.X - line.loc.X : line.loc.X - target.X;
      BresLineStep(&line, *travel, 0);
      *outward = (line.yOut > 0) ? target.Y - line.loc.Y : line.loc.Y - target.Y;
      assert(line.xOut == 0);
   }

   return DMTX_SUCCESS;
}

/**
 *
 *
 */
static int
BresLineStep(DmtxBresLine *line, int travel, int outward)
{
   int i;
   DmtxBresLine lineNew;

   lineNew = *line;

   assert(abs(travel) < 2);
   assert(abs(outward) >= 0);

   /* Perform forward step */
   if(travel > 0) {
      lineNew.travel++;
      if(lineNew.steep) {
         lineNew.loc.Y += lineNew.yStep;
         lineNew.error -= lineNew.xDelta;
         if(lineNew.error < 0) {
            lineNew.loc.X += lineNew.xStep;
            lineNew.error += lineNew.yDelta;
         }
      }
      else {
         lineNew.loc.X += lineNew.xStep;
         lineNew.error -= lineNew.yDelta;
         if(lineNew.error < 0) {
            lineNew.loc.Y += lineNew.yStep;
            lineNew.error += lineNew.xDelta;
         }
      }
   }
   else if(travel < 0) {
      lineNew.travel--;
      if(lineNew.steep) {
         lineNew.loc.Y -= lineNew.yStep;
         lineNew.error += lineNew.xDelta;
         if(lineNew.error >= lineNew.yDelta) {
            lineNew.loc.X -= lineNew.xStep;
            lineNew.error -= lineNew.yDelta;
         }
      }
      else {
         lineNew.loc.X -= lineNew.xStep;
         lineNew.error += lineNew.yDelta;
         if(lineNew.error >= lineNew.xDelta) {
            lineNew.loc.Y -= lineNew.yStep;
            lineNew.error -= lineNew.xDelta;
         }
      }
   }

   for(i = 0; i < outward; i++) {
      /* Outward steps */
      lineNew.outward++;
      lineNew.loc.X += lineNew.xOut;
      lineNew.loc.Y += lineNew.yOut;
   }

   *line = lineNew;

   return DMTX_SUCCESS;
}

/**
 *
 *
 */
/*
static void
WriteDiagnosticImage(DmtxDecode *dec, DmtxRegion *reg, char *imagePath)
{
   int row, col;
   int width, height;
   int offset;
   FILE *fp;
   DmtxRgb rgb;
   DmtxVector2 p;
   DmtxImage *img;

   assert(reg != NULL);

   fp = fopen(imagePath, "wb");
   if(fp == NULL) {
      exit(3);
   }

   width = dmtxImageGetProp(dec->image, DmtxPropScaledWidth);
   height = dmtxImageGetProp(dec->image, DmtxPropScaledHeight);

   img = dmtxImageMalloc(width, height);

   // Populate image
   for(row = 0; row < height; row++) {
      for(col = 0; col < width; col++) {

         offset = dmtxImageGetOffset(dec->image, col, row);
         if(offset == DMTX_BAD_OFFSET) {
            rgb[0] = 0;
            rgb[1] = 0;
            rgb[2] = 128;
         }
         else {
            dmtxImageGetRgb(dec->image, col, row, rgb);

            p.X = col;
            p.Y = row;
            dmtxMatrix3VMultiplyBy(&p, reg->raw2fit);

            if(p.X < 0.0 || p.X > 1.0 || p.Y < 0.0 || p.Y > 1.0) {
               rgb[0] = 0;
               rgb[1] = 0;
               rgb[2] = 128;
            }
            else if(p.X + p.Y > 1.0) {
               rgb[0] += (0.4 * (255 - rgb[0]));
               rgb[1] += (0.4 * (255 - rgb[1]));
               rgb[2] += (0.4 * (255 - rgb[2]));
            }
         }

         dmtxImageSetRgb(img, col, row, rgb);
      }
   }

   // Write additional markers
   rgb[0] = 255;
   rgb[1] = 0;
   rgb[2] = 0;
   dmtxImageSetRgb(img, reg->topLoc.X, reg->topLoc.Y, rgb);
   dmtxImageSetRgb(img, reg->rightLoc.X, reg->rightLoc.Y, rgb);

   // Write image to PNM file
   fprintf(fp, "P6\n%d %d\n255\n", width, height);
   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {
         dmtxImageGetRgb(img, col, row, rgb);
         fwrite(rgb, sizeof(char), 3, fp);
      }
   }

   fclose(fp);
}
*/
