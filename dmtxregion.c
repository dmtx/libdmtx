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
 * TODO: add +1 and -1 offset to FindBestSolidLine()
 *       - break AlignCalib into 2 separate functions. Inner one that simply
 *       follows the line, and outer one that calls inner one over and over until a best hough is found.
 * TODO: try -s2 again after tightening fit logic
 * TODO: Remove status from DmtxPixelLoc
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
   if(offset == DMTX_BAD_OFFSET || dec->image->cache[offset] & 0x40) {
      reg.found = DMTX_REGION_NOT_FOUND;
      return reg;
   }

   /* Test gravitational pull of nearby edges on all color planes */
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

   /* Define top edge */
   if(MatrixRegionAlignCalibEdge(dec, &reg, DmtxEdgeTop) != DMTX_SUCCESS) {
      reg.found = DMTX_REGION_DROPPED_TOP;
      return reg;
   }

   /* Define right edge */
   if(MatrixRegionAlignCalibEdge(dec, &reg, DmtxEdgeRight) != DMTX_SUCCESS) {
      reg.found = DMTX_REGION_DROPPED_RIGHT;
      return reg;
   }

   CALLBACK_MATRIX(&reg);

   /* Calculate the best fitting symbol size */
   if(MatrixRegionFindSize(dec->image, &reg) != DMTX_SUCCESS) {
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
   DmtxBestLine line1x, line2x;
   DmtxBestLine line2n, line2p;
   DmtxFollow fTmp;

   /* Follow to end in both directions */
   BlazeTrail(dec, reg, begin);
   if(reg->stepsTotal < 40) {
      ClearTrail(dec, reg, 0x40);
      return DMTX_FAILURE;
   }

   if((reg->boundMax.X - reg->boundMin.X) * (reg->boundMax.Y - reg->boundMin.Y) < 3000) {
      ClearTrail(dec, reg, 0x40);
      return DMTX_FAILURE;
   }

   line1x = FindBestSolidLine(dec, reg, 0, 0, -1);
   if(line1x.mag < 5) {
      ClearTrail(dec, reg, 0x40);
      return DMTX_FAILURE;
   }

   err = FindTravelLimits(dec, reg, &line1x);
   if(line1x.distSq < 100 || line1x.devn / sqrt(line1x.distSq) > 0.1) {
      ClearTrail(dec, reg, 0x40);
      return DMTX_FAILURE;
   }
   assert(line1x.stepPos >= line1x.stepNeg);

   fTmp = FollowSeek(dec, reg, line1x.stepPos + 5);
   line2p = FindBestSolidLine(dec, reg, fTmp.step, line1x.stepNeg, line1x.angle);

   fTmp = FollowSeek(dec, reg, line1x.stepNeg - 5);
   line2n = FindBestSolidLine(dec, reg, fTmp.step, line1x.stepPos, line1x.angle);
   if(max(line2p.mag, line2n.mag) < 5)
      return DMTX_FAILURE;

   if(line2p.mag > line2n.mag) {
      line2x = line2p;
      err = FindTravelLimits(dec, reg, &line2x);
      if(line2x.distSq < 100 || line2x.devn / sqrt(line2x.distSq) > 0.1)
         return DMTX_FAILURE;

      cross = ((line1x.locPos.X - line1x.locNeg.X) * (line2x.locPos.Y - line2x.locNeg.Y)) -
            ((line1x.locPos.Y - line1x.locNeg.Y) * (line2x.locPos.X - line2x.locNeg.X));
      if(cross > 0) {
         /* Condition 2 */
         reg->polarity = +1;
         reg->locR = line2x.locPos;
         reg->locT = line1x.locNeg;
         reg->leftLoc = line1x.locBeg;
         reg->leftAngle = line1x.angle;
         reg->bottomLoc = line2x.locBeg;
         reg->bottomAngle = line2x.angle;
      }
      else {
         /* Condition 3 */
         reg->polarity = -1;
         reg->locR = line1x.locNeg;
         reg->locT = line2x.locPos;
         reg->leftLoc = line2x.locBeg;
         reg->leftAngle = line2x.angle;
         reg->bottomLoc = line1x.locBeg;
         reg->bottomAngle = line1x.angle;
      }
   }
   else {
      line2x = line2n;
      err = FindTravelLimits(dec, reg, &line2x);
      if(line2x.distSq < 100 || line2x.devn / sqrt(line2x.distSq) > 0.1)
         return DMTX_FAILURE;

      cross = ((line1x.locNeg.X - line1x.locPos.X) * (line2x.locNeg.Y - line2x.locPos.Y)) -
            ((line1x.locNeg.Y - line1x.locPos.Y) * (line2x.locNeg.X - line2x.locPos.X));
      if(cross > 0) {
         /* Condition 1 */
         reg->polarity = -1;
         reg->locR = line2x.locNeg;
         reg->locT = line1x.locPos;
         reg->leftLoc = line1x.locBeg;
         reg->leftAngle = line1x.angle;
         reg->bottomLoc = line2x.locBeg;
         reg->bottomAngle = line2x.angle;
      }
      else {
         /* Condition 4 */
         reg->polarity = +1;
         reg->locR = line1x.locPos; /* follow + */
         reg->locT = line2x.locNeg; /* follow - */
         reg->leftLoc = line2x.locBeg;
         reg->leftAngle = line2x.angle;
         reg->bottomLoc = line1x.locBeg;
         reg->bottomAngle = line1x.angle;
      }
   }
   CALLBACK_POINT_PLOT(reg->locR, 2, 1, DMTX_DISPLAY_SQUARE);
   CALLBACK_POINT_PLOT(reg->locT, 2, 1, DMTX_DISPLAY_SQUARE);

   reg->leftKnown = reg->bottomKnown = 1;

   if(dmtxRegionUpdateXfrms(dec, reg) != DMTX_SUCCESS)
      return DMTX_FAILURE;

   return DMTX_SUCCESS;
}

/**
 *
 *
 */
long
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
   if(dimOT < 8 || dimOR < 8 || dimTX < 8 || dimRX < 8)
      return DMTX_FAILURE;

   /* Verify that the 4 corners define a reasonably fat quadrilateral */
   ratio = dimOT / dimRX;
   if(ratio < 0.5 || ratio > 2.0)
      return DMTX_FAILURE;

   ratio = dimOR / dimTX;
   if(ratio < 0.5 || ratio > 2.0)
      return DMTX_FAILURE;

   /* Verify this is not a bowtie shape */
   if(dmtxVector2Cross(&vOR, &vRX) < 0.0 ||
         dmtxVector2Cross(&vOT, &vTX) > 0.0)
      return DMTX_FAILURE;

   if(RightAngleTrueness(p00, p10, p11, M_PI_2) < dec->squareDevn)
      return DMTX_FAILURE;
   if(RightAngleTrueness(p10, p11, p01, M_PI_2) < dec->squareDevn)
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
static DmtxColor3
ReadModuleColor(DmtxImage *img, DmtxRegion *reg, int symbolRow, int symbolCol, int sizeIdx)
{
   int symbolRows, symbolCols;
   DmtxVector2 p, p0;
   DmtxColor3 cPoint;

   symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, sizeIdx);
   symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, sizeIdx);

   p.X = (symbolCol + 0.5)/symbolCols;
   p.Y = (symbolRow + 0.5)/symbolRows;

   dmtxMatrix3VMultiply(&p0, &p, reg->fit2raw);
   dmtxColor3FromImage2(&cPoint, img, p0);

   return cPoint;
}

/**
 * @brief  XXX
 * @param  image
 * @param  reg
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
MatrixRegionFindSize(DmtxImage *img, DmtxRegion *reg)
{
   int row, col;
   int sizeIdx, bestSizeIdx;
   int symbolRows, symbolCols;
   int jumpCount, errors;
   double contrast, bestContrast;
   DmtxColor3 color, colorOnAvg, colorOffAvg, bestColorOffAvg;
   DmtxColor3 colorDiff, bestColorDiff;
   DmtxColor3 black = { 0.0, 0.0, 0.0 };

   bestSizeIdx = -1;
   contrast = bestContrast = 0;
   colorDiff = bestColorDiff = black;
   colorOffAvg = bestColorOffAvg = black;

   /* Test each barcode size to find best contrast in calibration modules */
   for(sizeIdx = 0; sizeIdx < DMTX_SYMBOL_SQUARE_COUNT + DMTX_SYMBOL_RECT_COUNT; sizeIdx++) {

      symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, sizeIdx);
      symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, sizeIdx);

      colorOnAvg = colorOffAvg = black;

      /* Sum module colors along horizontal calibration bar */
      row = symbolRows - 1;
      for(col = 0; col < symbolCols; col++) {
         color = ReadModuleColor(img, reg, row, col, sizeIdx);
         if(col & 0x01)
            dmtxColor3AddTo(&colorOffAvg, &color);
         else
            dmtxColor3AddTo(&colorOnAvg, &color);
      }

      /* Sum module colors along vertical calibration bar */
      col = symbolCols - 1;
      for(row = 0; row < symbolRows; row++) {
         color = ReadModuleColor(img, reg, row, col, sizeIdx);
         if(row & 0x01)
            dmtxColor3AddTo(&colorOffAvg, &color);
         else
            dmtxColor3AddTo(&colorOnAvg, &color);
      }

      dmtxColor3ScaleBy(&colorOnAvg, 2.0/(symbolRows + symbolCols));
      dmtxColor3ScaleBy(&colorOffAvg, 2.0/(symbolRows + symbolCols));

      contrast = dmtxColor3Mag(dmtxColor3Sub(&colorDiff, &colorOnAvg, &colorOffAvg));
      if(contrast < 20)
         continue;

      if(contrast > bestContrast) {
         bestSizeIdx = sizeIdx;
         bestContrast = contrast;
         bestColorOffAvg = colorOffAvg;
         bestColorDiff = colorDiff;
      }
   }

   /* If no sizes produced acceptable contrast then call it quits */
   if(bestSizeIdx == -1 || bestContrast < 20)
      return DMTX_FAILURE;

   reg->sizeIdx = bestSizeIdx;

   dmtxColor3Norm(&bestColorDiff);
   reg->gradient.ray.c = bestColorDiff;
   reg->gradient.ray.p = bestColorOffAvg;
   reg->gradient.tMin = 0;
   reg->gradient.tMax = bestContrast;

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
   double jumpThreshold;
   double tModule, tPrev;
   DmtxColor3 color;

   assert(xStart == 0 || yStart == 0);
   assert(dir == DmtxDirRight || dir == DmtxDirUp);

   if(dir == DmtxDirRight)
      xInc = 1;
   else
      yInc = 1;

   if(xStart == -1 || xStart == reg->symbolCols ||
         yStart == -1 || yStart == reg->symbolRows)
      state = DMTX_MODULE_OFF;

   jumpThreshold = 0.3 * (reg->gradient.tMax - reg->gradient.tMin);

   color = ReadModuleColor(img, reg, yStart, xStart, reg->sizeIdx);
   tModule = dmtxDistanceAlongRay3(&(reg->gradient.ray), &color);

   for(x = xStart + xInc, y = yStart + yInc;
         (dir == DmtxDirRight && x < reg->symbolCols) ||
         (dir == DmtxDirUp && y < reg->symbolRows);
         x += xInc, y += yInc) {

      tPrev = tModule;
      color = ReadModuleColor(img, reg, y, x, reg->sizeIdx);
      tModule = dmtxDistanceAlongRay3(&(reg->gradient.ray), &color);

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
            (flow[i].mag == flow[strongIdx].mag && ((i & 0x01) == 0))) {
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
 * vaiiiooo
 * --------
 * 0x80 v = visited bit
 * 0x40 a = assigned bit
 * 0x38 u = 3 bits points upstream 0-7
 * 0x07 d = 3 bits points downstream 0-7
 */
static int
BlazeTrail(DmtxDecode *dec, DmtxRegion *reg, DmtxPointFlow flowBegin)
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
         /*if testing downstream (sign < 0) then next upstream is opposite of next arrival*/
         /*if testing upstream (sign > 0) then next downstream is opposite of next arrival*/
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
   clears = ClearTrail(dec, reg, 0x80);
   assert(posAssigns + negAssigns == clears - 1);

   return DMTX_SUCCESS;
}

/**
 *
 *
 */
static int
ClearTrail(DmtxDecode *dec, DmtxRegion *reg, unsigned char clearMask)
{
   int clears;
   DmtxFollow follow;

   /* Clear "visited" bit from trail */
   clears = 0;
   follow = FollowSeek(dec, reg, 0);
   while(abs(follow.step) <= reg->stepsTotal) {
      assert(*follow.ptr & clearMask);
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
FindBestSolidLine(DmtxDecode *dec, DmtxRegion *reg, int step0, int step1, int houghAvoid)
{
   int hough[DMTX_HOUGH_RES] = { 0 };
   int houghMin, houghMax;
   char houghTest[DMTX_HOUGH_RES];
   int i;
   int step;
   int sign;
   int tripSteps;
   int angle;
   int xDiff, yDiff;
   double dH;
   DmtxRay2 rH;
   DmtxFollow follow;
   DmtxBestLine line;
   DmtxPixelLoc rHp;

   memset(&line, 0x00, sizeof(DmtxBestLine));
   memset(&rH, 0x00, sizeof(DmtxRay2));
   angle = 0;

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
         if(dH > -256 && dH < 256)
            hough[i]++;

         /* New angle takes over lead */
         if(hough[i] > hough[angle])
            angle = i;
      }

/*    CALLBACK_POINT_PLOT(follow.loc, (sign > 1) ? 4 : 3, 1, DMTX_DISPLAY_POINT); */

      follow = FollowStep(dec, reg, follow, sign);
   }

   line.angle = angle;
   line.mag = hough[angle];

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
   int posWander, negWander;
   int posTravel, negTravel;
   int posMinDevn, posMinDevnLock;
   int posMaxDevn, posMaxDevnLock;
   int negMinDevn, negMinDevnLock;
   int negMaxDevn, negMaxDevnLock;
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
   posMinDevn = posMinDevnLock = 0;
   posMaxDevn = posMaxDevnLock = 0;
   negMinDevn = negMinDevnLock = 0;
   negMaxDevn = negMaxDevnLock = 0;

   for(i = 0; i < reg->stepsTotal/2; i++) {

      xDiff = followPos.loc.X - loc0.X;
      yDiff = followPos.loc.Y - loc0.Y;
      posTravel = ((cosAngle * xDiff) + (sinAngle * yDiff))/256;
      posWander = ((cosAngle * yDiff) - (sinAngle * xDiff))/256;

      xDiff = followNeg.loc.X - loc0.X;
      yDiff = followNeg.loc.Y - loc0.Y;
      negTravel = ((cosAngle * xDiff) + (sinAngle * yDiff))/256;
      negWander = ((cosAngle * yDiff) - (sinAngle * xDiff))/256;

      if(i > 10 && abs(posWander) > abs(posTravel) && abs(negWander) > abs(negTravel))
         break;

      if(posWander > -3 && posWander < 3) {
         distSq = DistanceSquared(followPos.loc, negMax);
         if(distSq > distSqMax) {
            posMax = followPos.loc;
            distSqMax = distSq;
            line->stepPos = followPos.step;
            line->locPos = followPos.loc;
            posMinDevnLock = posMinDevn;
            posMaxDevnLock = posMaxDevn;
         }
      }
      else {
         posMinDevn = min(posMinDevn, posWander);
         posMaxDevn = max(posMaxDevn, posWander);
      }

      if(negWander > -3 && negWander < 3) {
         distSq = DistanceSquared(followNeg.loc, posMax);
         if(distSq > distSqMax) {
            negMax = followNeg.loc;
            distSqMax = distSq;
            line->stepNeg = followNeg.step;
            line->locNeg = followNeg.loc;
            negMinDevnLock = negMinDevn;
            negMaxDevnLock = negMaxDevn;
         }
      }
      else {
         negMinDevn = min(negMinDevn, negWander);
         negMaxDevn = max(negMaxDevn, negWander);
      }

/*  CALLBACK_POINT_PLOT(followPos.loc, 2, 1, DMTX_DISPLAY_POINT);
    CALLBACK_POINT_PLOT(followNeg.loc, 4, 1, DMTX_DISPLAY_POINT); */

      followPos = FollowStep(dec, reg, followPos, +1);
      followNeg = FollowStep(dec, reg, followNeg, -1);
   }
   line->devn = max(posMaxDevnLock - posMinDevnLock, negMaxDevnLock - negMinDevnLock);
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
MatrixRegionAlignCalibEdge(DmtxDecode *dec, DmtxRegion *reg, int edge)
{
   int i, err;
   int streamDir;
   int angle, bestAngle;
   int strength, bestStrength;
   int inward;
   DmtxVector2 pTmp;
   DmtxPixelLoc locOrigin, loc0, loc1;
   DmtxPixelLoc locFinal, bestFinal;
   DmtxBresLine *startLine, startLineOut, startLineIn;
   DmtxBresLine testLine;

   /* Determine pixel coordinates of origin */
   pTmp.X = 0.0;
   pTmp.Y = 0.0;
   dmtxMatrix3VMultiplyBy(&pTmp, reg->fit2raw);
   locOrigin.X = (int)(pTmp.X + 0.5);
   locOrigin.Y = (int)(pTmp.Y + 0.5);
   locOrigin.status = DMTX_RANGE_GOOD;

   /* Determine locations of test line and start line */
   if(edge == DmtxEdgeTop) {
      streamDir = reg->polarity * -1;
      loc0 = reg->locT;
      startLineOut = BresLineInit(loc0, locOrigin, reg->locR);
      pTmp.X = 0.9;
      pTmp.Y = 0.5;
   }
   else {
      assert(edge == DmtxEdgeRight);
      streamDir = reg->polarity;
      loc0 = reg->locR;
      startLineOut = BresLineInit(loc0, locOrigin, reg->locT);
      pTmp.X = 0.8;
      pTmp.Y = 0.8;
   }
   dmtxMatrix3VMultiplyBy(&pTmp, reg->fit2raw);
   loc1.X = (int)(pTmp.X + 0.5);
   loc1.Y = (int)(pTmp.Y + 0.5);
   loc1.status = DMTX_RANGE_GOOD;

   bestAngle = 0;
   bestStrength = 0;

   startLineIn = startLineOut;
   err = BresLineStep(&startLineIn, +1, 0);

   /* Test for presence of gapped lines at up to N places along start line */
   for(i = 0; i < 10; i++) {
      inward = (i & 0x01);
      startLine = (inward) ? &startLineIn : &startLineOut;

      testLine = BresLineInit(startLine->loc, loc1, locOrigin);
      err = FindBestGappedLine(dec, reg, streamDir, testLine, &angle, &strength, &locFinal);
      if(err == DMTX_FAILURE)
         return DMTX_FAILURE;

      if(i == 0 || strength > bestStrength) {
         bestStrength = strength;
         bestAngle = angle;
         bestFinal = locFinal;
      }
      else if(strength < bestStrength/2) {
         break;
      }

      err = BresLineStep(startLine, ((inward) ? +1 : -1), 0);
   }

   if(edge == DmtxEdgeTop) {
      reg->topKnown = 1;
      reg->topAngle = bestAngle;
      reg->topLoc = locFinal;
   }
   else {
      reg->rightKnown = 1;
      reg->rightAngle = bestAngle;
      reg->rightLoc = locFinal;
   }

   /* Update region transform with new geometry */
   if(dmtxRegionUpdateXfrms(dec, reg) != DMTX_SUCCESS)
      return DMTX_FAILURE;

   return DMTX_SUCCESS;
}

/**
 *
 *
 */
static int
FindBestGappedLine(DmtxDecode *dec, DmtxRegion *reg, int streamDir, DmtxBresLine line,
      int *angle, int *strength, DmtxPixelLoc *locFinal)
{
   int xDiff, yDiff;
   int distSq, totalDistSq;
   int dH;
   int i, bestAngle;
   int hit;
   int hough[DMTX_HOUGH_RES] = { 0 };
   DmtxPixelLoc loc0, final;
   DmtxPointFlow flow;

   loc0 = line.loc0;
   totalDistSq = (line.xDelta * line.xDelta) + (line.yDelta * line.yDelta);

   bestAngle = 0;
   final.X = 0;
   final.Y = 0;
   final.status = DMTX_RANGE_GOOD;

   /* Function follows Bresenham */
   flow = GetPointFlow(dec, reg->flowBegin.plane, loc0, dmtxNeighborNone);

   for(;;) {
      xDiff = flow.loc.X - loc0.X;
      yDiff = flow.loc.Y - loc0.Y;

      distSq = (xDiff * xDiff) + (yDiff * yDiff);
      if(distSq > totalDistSq)
         break;

      /* Flow direction is determined by region polarity */
      flow = FindStrongestNeighbor(dec, flow, streamDir);
      if(flow.mag == -1) {
         return DMTX_FAILURE;
      }
      else if(flow.mag < 20) {
         BresLineStep(&line, 1, 0);
      }
      else {
         hit = BresLineStepHit(&line, flow.loc);
         if(hit == DMTX_SUCCESS) {
            xDiff = line.loc.X - loc0.X;
            yDiff = line.loc.Y - loc0.Y;
            for(i = 0; i < DMTX_HOUGH_RES; i++) {

               dH = (rHvX[i] * yDiff) - (rHvY[i] * xDiff);
               if(dH >= -128 && dH < 128)
                  hough[i]++;

               /* New angle takes over lead */
               if(hough[i] > hough[bestAngle])
                  bestAngle = i;

               if(i == bestAngle)
                  final = flow.loc;
            }
         }
      }

      flow = GetPointFlow(dec, reg->flowBegin.plane, line.loc, dmtxNeighborNone);
   }

   *angle = bestAngle;
   *strength = hough[bestAngle];
   *locFinal = final;

   return DMTX_SUCCESS;
}

/**
 *
 *
 */
static DmtxBresLine
BresLineInit(DmtxPixelLoc loc0, DmtxPixelLoc loc1, DmtxPixelLoc locOrigin)
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
   line.steep = (line.yDelta > line.xDelta);

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
      cp = (((locEnd->X - locBeg->X) * (locOrigin.Y - locEnd->Y)) -
            ((locEnd->Y - locBeg->Y) * (locOrigin.X - locEnd->X)));

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
      cp = (((locEnd->X - locBeg->X) * (locOrigin.Y - locEnd->Y)) -
            ((locEnd->Y - locBeg->Y) * (locOrigin.X - locEnd->X)));

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
 * Try to step us in the direction requested by stepDir, but only
 * using steps forward or outward from region center. If too low,
 * then just advance along travel. If trying to go backward, then
 * just move directly away from the center.
 */
static int
BresLineStepHit(DmtxBresLine *line, DmtxPixelLoc targetLoc)
{
   int travelStep, sideStep;
   DmtxBresLine lineTmp;

   /* Determine necessary steps along and away from Bresenham line */
   lineTmp = *line;
   if(line->steep) {
      travelStep = (line->yStep > 0) ? targetLoc.Y - line->loc.Y : line->loc.Y - targetLoc.Y;
      BresLineStep(&lineTmp, travelStep, 0);
      sideStep = (line->xOut > 0) ? targetLoc.X - lineTmp.loc.X : lineTmp.loc.X - targetLoc.X;
      assert(line->yOut == 0);
   }
   else {
      travelStep = (line->xStep > 0) ? targetLoc.X - line->loc.X : line->loc.X - targetLoc.X;
      BresLineStep(&lineTmp, travelStep, 0);
      sideStep = (line->yOut > 0) ? targetLoc.Y - lineTmp.loc.Y : lineTmp.loc.Y - targetLoc.Y;
      assert(line->xOut == 0);
   }

   if(sideStep < 0) {
      /* Line cannot ratchet inward */
      BresLineStep(line, 1, 0);
/*    CALLBACK_POINT_PLOT(line->loc, 1, 1, DMTX_DISPLAY_POINT); */
      return DMTX_FAILURE;
   }
   else {
      /* travelStep < 0 && sideStep >= 0 */
      if(travelStep < 0) {
         BresLineStep(line, -1, 1);
/*       CALLBACK_POINT_PLOT(line->loc, 2, 1, DMTX_DISPLAY_POINT); */
         return DMTX_FAILURE;
      }
      /* travelStep >= 0 && sideStep >= 0 */
      else {
         BresLineStep(line, travelStep, sideStep);
         if(travelStep == 0 && sideStep > 0) {
            CALLBACK_POINT_PLOT(line->loc, 3, 1, DMTX_DISPLAY_POINT);
            return DMTX_SUCCESS;
         }
         if(travelStep > 0 && sideStep == 0) {
/*          CALLBACK_POINT_PLOT(line->loc, 4, 1, DMTX_DISPLAY_POINT); */
            return DMTX_FAILURE;
         }
         if(travelStep > 0 && sideStep > 0) {
            CALLBACK_POINT_PLOT(line->loc, 5, 1, DMTX_DISPLAY_POINT);
            return DMTX_SUCCESS;
         }
      }
   }

   return DMTX_FAILURE;
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
   double shade;
   FILE *fp;
   DmtxRgb rgb;
   DmtxVector2 p;

   fp = fopen(imagePath, "wb");
   if(fp == NULL) {
      exit(3);
   }

   width = dmtxImageGetProp(dec->image, DmtxPropScaledWidth);
   height = dmtxImageGetProp(dec->image, DmtxPropScaledHeight);

   fprintf(fp, "P6\n%d %d\n255\n", width, height);
   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {

         dmtxImageGetRgb(dec->image, col, row, rgb);

         if(reg != NULL) {
            p.X = col;
            p.Y = row;
            dmtxMatrix3VMultiplyBy(&p, reg->raw2fit);

            if(p.X < 0.0 || p.X > 1.0 || p.Y < 0.0 || p.Y > 1.0)
               shade = 0.7;
            else if(p.X + p.Y < 1.0)
               shade = 0.0;
            else
               shade = 0.4;

            rgb[0] += (shade * (255 - rgb[0]));
            rgb[1] += (shade * (255 - rgb[1]));
            rgb[2] += (shade * (255 - rgb[2]));
         }

         fwrite(rgb, sizeof(char), 3, fp);
      }
   }

   fclose(fp);
}
*/
