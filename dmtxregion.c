/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2008, 2009 Mike Laughton. All rights reserved.
 * Copyright 2012-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 * Copyright 2016 Tim Zaman. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxregion.c
 * \brief Detect barcode regions
 */

#define DMTX_HOUGH_RES 180

/**
 * \brief  Create copy of existing region struct
 * \param  None
 * \return Initialized DmtxRegion struct
 */
extern DmtxRegion *
dmtxRegionCreate(DmtxRegion *reg)
{
   DmtxRegion *regCopy;

   regCopy = (DmtxRegion *)malloc(sizeof(DmtxRegion));
   if(regCopy == NULL)
      return NULL;

   memcpy(regCopy, reg, sizeof(DmtxRegion));

   return regCopy;
}

/**
 * \brief  Destroy region struct
 * \param  reg
 * \return void
 */
extern DmtxPassFail
dmtxRegionDestroy(DmtxRegion **reg)
{
   if(reg == NULL || *reg == NULL)
      return DmtxFail;

   free(*reg);

   *reg = NULL;

   return DmtxPass;
}

/**
 * \brief  Find next barcode region
 * \param  dec Pointer to DmtxDecode information struct
 * \param  timeout Pointer to timeout time (NULL if none)
 * \return Detected region (if found)
 */
extern DmtxRegion *
dmtxRegionFindNext(DmtxDecode *dec, DmtxTime *timeout)
{
   DmtxScanConstraint constraint;

   // Functionally these two branches do the same thing even
   // if timeout were null, but we use the latter form in order
   // to preserve the most short-circuit performance if the
   // caller doesn't specify a timeout.
   if(timeout != NULL) {
      constraint.maxTimeout = timeout;
      constraint.maxIterations = 0;
      return dmtxRegionFindNextDeterministic(dec, &constraint);
   } else {
      return dmtxRegionFindNextDeterministic(dec, NULL);
   }
}

/**
 * \brief  Find next barcode region
 * \param  dec Pointer to DmtxDecode information struct
 * \param  constraint Pointer to constraint (NULL if no constraints)
 *         Constraint is an input/output structure.
 *         Limits will be considered independently. Set to zero/null
 *         to indicate no-constraint. Actual runtime and iterations,
 *         as well as termination reason will be filled in upon return
 *         if constraint is non-null.
 *
 * \return Detected region (if found)
 */
extern DmtxRegion *
dmtxRegionFindNextDeterministic(DmtxDecode *dec, DmtxScanConstraint *constraint)
{
   int locStatus;
   int iterations = 0;
   DmtxPixelLoc loc;
   DmtxRegion   *reg;

   /* Continue until we find a region or run out of chances */
   for(;;) {
      locStatus = PopGridLocation(&(dec->grid), &loc);
      if(locStatus == DmtxRangeEnd) {
         if(constraint != NULL)
            constraint->stopCause = DmtxScanNotFound;
         break;
      }

      /* Iterations counts the number of calls to ScanPixel */
      ++iterations;
      /* Scan location for presence of valid barcode region */
      reg = dmtxRegionScanPixel(dec, loc.X, loc.Y);
      if(reg != NULL) {
         if(constraint != NULL) {
            constraint->iterations = iterations;
            constraint->stopCause = DmtxScanSuccess;
         }
         return reg;
      }

      /* Ran out of iterations? */
      if(constraint != NULL && constraint->maxIterations != 0 && constraint->maxIterations <= iterations) {
         constraint->stopCause = DmtxScanIterLimit;
         break;
      }

      /* Ran out of time? */
      if(constraint != NULL && constraint->maxTimeout != NULL && dmtxTimeExceeded(*constraint->maxTimeout)) {
         constraint->stopCause = DmtxScanTimeLimit;
         break;
      }
   }
   if(constraint)
      constraint->iterations = iterations;

   return NULL;
}

/**
 * \brief  Scan individual pixel for presence of barcode edge
 * \param  dec Pointer to DmtxDecode information struct
 * \param  loc Pixel location
 * \return Detected region (if any)
 */
extern DmtxRegion *
dmtxRegionScanPixel(DmtxDecode *dec, int x, int y)
{
   unsigned char *cache;
   DmtxRegion reg;
   DmtxPointFlow flowBegin;
   DmtxPixelLoc loc;

   loc.X = x;
   loc.Y = y;

   cache = dmtxDecodeGetCache(dec, loc.X, loc.Y);
   if(cache == NULL)
      return NULL;

   if((int)(*cache & 0x80) != 0x00)
      return NULL;

   /* Test for presence of any reasonable edge at this location */
   flowBegin = MatrixRegionSeekEdge(dec, loc);
   if(flowBegin.mag < (int)(dec->edgeThresh * 7.65 + 0.5))
      return NULL;

   memset(&reg, 0x00, sizeof(DmtxRegion));

   /* Determine barcode orientation */
   if(MatrixRegionOrientation(dec, &reg, flowBegin) == DmtxFail)
      return NULL;
   if(dmtxRegionUpdateXfrms(dec, &reg) == DmtxFail)
      return NULL;

   /* Define top edge */
   if(MatrixRegionAlignCalibEdge(dec, &reg, DmtxEdgeTop) == DmtxFail)
      return NULL;
   if(dmtxRegionUpdateXfrms(dec, &reg) == DmtxFail)
      return NULL;

   /* Define right edge */
   if(MatrixRegionAlignCalibEdge(dec, &reg, DmtxEdgeRight) == DmtxFail)
      return NULL;
   if(dmtxRegionUpdateXfrms(dec, &reg) == DmtxFail)
      return NULL;

   CALLBACK_MATRIX(&reg);

   /* Calculate the best fitting symbol size */
   if(MatrixRegionFindSize(dec, &reg) == DmtxFail)
      return NULL;

   /* Found a valid matrix region */
   return dmtxRegionCreate(&reg);
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
   int channelCount;
   DmtxPointFlow flow, flowPlane[3];
   DmtxPointFlow flowPos, flowPosBack;
   DmtxPointFlow flowNeg, flowNegBack;

   channelCount = dec->image->channelCount;

   /* Find whether red, green, or blue shows the strongest edge */
   strongIdx = 0;
   for(i = 0; i < channelCount; i++) {
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
         CALLBACK_POINT_PLOT(flow.loc, 1, 1, 1);
         return flow;
      }
   }

   return dmtxBlankEdge;
}

/**
 *
 *
 */
static DmtxPassFail
MatrixRegionOrientation(DmtxDecode *dec, DmtxRegion *reg, DmtxPointFlow begin)
{
   int cross;
   int minArea;
   int scale;
   int symbolShape;
   int maxDiagonal;
   DmtxPassFail err;
   DmtxBestLine line1x, line2x;
   DmtxBestLine line2n, line2p;
   DmtxFollow fTmp;

   if(dec->sizeIdxExpected == DmtxSymbolSquareAuto ||
         (dec->sizeIdxExpected >= DmtxSymbol10x10 &&
         dec->sizeIdxExpected <= DmtxSymbol144x144))
      symbolShape = DmtxSymbolSquareAuto;
   else if(dec->sizeIdxExpected == DmtxSymbolRectAuto ||
         (dec->sizeIdxExpected >= DmtxSymbol8x18 &&
         dec->sizeIdxExpected <= DmtxSymbol16x48))
      symbolShape = DmtxSymbolRectAuto;
   else
      symbolShape = DmtxSymbolShapeAuto;

   if(dec->edgeMax != DmtxUndefined) {
      if(symbolShape == DmtxSymbolRectAuto)
         maxDiagonal = (int)(1.23 * dec->edgeMax + 0.5); /* sqrt(5/4) + 10% */
      else
         maxDiagonal = (int)(1.56 * dec->edgeMax + 0.5); /* sqrt(2) + 10% */
   }
   else {
      maxDiagonal = DmtxUndefined;
   }

   /* Follow to end in both directions */
   err = TrailBlazeContinuous(dec, reg, begin, maxDiagonal);
   if(err == DmtxFail || reg->stepsTotal < 40) {
      TrailClear(dec, reg, 0x40);
      return DmtxFail;
   }

   /* Filter out region candidates that are smaller than expected */
   if(dec->edgeMin != DmtxUndefined) {
      scale = dmtxDecodeGetProp(dec, DmtxPropScale);

      if(symbolShape == DmtxSymbolSquareAuto)
         minArea = (dec->edgeMin * dec->edgeMin)/(scale * scale);
      else
         minArea = (2 * dec->edgeMin * dec->edgeMin)/(scale * scale);

      if((reg->boundMax.X - reg->boundMin.X) * (reg->boundMax.Y - reg->boundMin.Y) < minArea) {
         TrailClear(dec, reg, 0x40);
         return DmtxFail;
      }
   }

   line1x = FindBestSolidLine(dec, reg, 0, 0, +1, DmtxUndefined);
   if(line1x.mag < 5) {
      TrailClear(dec, reg, 0x40);
      return DmtxFail;
   }

   err = FindTravelLimits(dec, reg, &line1x);
   if(line1x.distSq < 100 || line1x.devn * 10 >= sqrt((double)line1x.distSq)) {
      TrailClear(dec, reg, 0x40);
      return DmtxFail;
   }
   assert(line1x.stepPos >= line1x.stepNeg);

   fTmp = FollowSeek(dec, reg, line1x.stepPos + 5);
   line2p = FindBestSolidLine(dec, reg, fTmp.step, line1x.stepNeg, +1, line1x.angle);

   fTmp = FollowSeek(dec, reg, line1x.stepNeg - 5);
   line2n = FindBestSolidLine(dec, reg, fTmp.step, line1x.stepPos, -1, line1x.angle);
   if(max(line2p.mag, line2n.mag) < 5)
      return DmtxFail;

   if(line2p.mag > line2n.mag) {
      line2x = line2p;
      err = FindTravelLimits(dec, reg, &line2x);
      if(line2x.distSq < 100 || line2x.devn * 10 >= sqrt((double)line2x.distSq))
         return DmtxFail;

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
         return DmtxFail;

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
/* CALLBACK_POINT_PLOT(reg->locR, 2, 1, 1);
   CALLBACK_POINT_PLOT(reg->locT, 2, 1, 1); */

   reg->leftKnown = reg->bottomKnown = 1;

   return DmtxPass;
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
extern DmtxPassFail
dmtxRegionUpdateCorners(DmtxDecode *dec, DmtxRegion *reg, DmtxVector2 p00,
      DmtxVector2 p10, DmtxVector2 p11, DmtxVector2 p01)
{
   double xMax, yMax;
   double tx, ty, phi, shx, scx, scy, skx, sky;
   double dimOT, dimOR, dimTX, dimRX, ratio;
   DmtxVector2 vOT, vOR, vTX, vRX, vTmp;
   DmtxMatrix3 m, mtxy, mphi, mshx, mscx, mscy, mscxy, msky, mskx;

   xMax = (double)(dmtxDecodeGetProp(dec, DmtxPropWidth) - 1);
   yMax = (double)(dmtxDecodeGetProp(dec, DmtxPropHeight) - 1);

   if(p00.X < 0.0 || p00.Y < 0.0 || p00.X > xMax || p00.Y > yMax ||
         p01.X < 0.0 || p01.Y < 0.0 || p01.X > xMax || p01.Y > yMax ||
         p10.X < 0.0 || p10.Y < 0.0 || p10.X > xMax || p10.Y > yMax)
      return DmtxFail;

   dimOT = dmtxVector2Mag(dmtxVector2Sub(&vOT, &p01, &p00)); /* XXX could use MagSquared() */
   dimOR = dmtxVector2Mag(dmtxVector2Sub(&vOR, &p10, &p00));
   dimTX = dmtxVector2Mag(dmtxVector2Sub(&vTX, &p11, &p01));
   dimRX = dmtxVector2Mag(dmtxVector2Sub(&vRX, &p11, &p10));

   /* Verify that sides are reasonably long */
   if(dimOT <= 8.0 || dimOR <= 8.0 || dimTX <= 8.0 || dimRX <= 8.0)
      return DmtxFail;

   /* Verify that the 4 corners define a reasonably fat quadrilateral */
   ratio = dimOT / dimRX;
   if(ratio <= 0.5 || ratio >= 2.0)
      return DmtxFail;

   ratio = dimOR / dimTX;
   if(ratio <= 0.5 || ratio >= 2.0)
      return DmtxFail;

   /* Verify this is not a bowtie shape */
   if(dmtxVector2Cross(&vOR, &vRX) <= 0.0 ||
         dmtxVector2Cross(&vOT, &vTX) >= 0.0)
      return DmtxFail;

   if(RightAngleTrueness(p00, p10, p11, M_PI_2) <= dec->squareDevn)
      return DmtxFail;
   if(RightAngleTrueness(p10, p11, p01, M_PI_2) <= dec->squareDevn)
      return DmtxFail;

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

   return DmtxPass;
}

/**
 *
 *
 */
extern DmtxPassFail
dmtxRegionUpdateXfrms(DmtxDecode *dec, DmtxRegion *reg)
{
   double radians;
   DmtxRay2 rLeft, rBottom, rTop, rRight;
   DmtxVector2 p00, p10, p11, p01;

   assert(reg->leftKnown != 0 && reg->bottomKnown != 0);

   /* Build ray representing left edge */
   rLeft.p.X = (double)reg->leftLoc.X;
   rLeft.p.Y = (double)reg->leftLoc.Y;
   radians = reg->leftAngle * (M_PI/DMTX_HOUGH_RES);
   rLeft.v.X = cos(radians);
   rLeft.v.Y = sin(radians);
   rLeft.tMin = 0.0;
   rLeft.tMax = dmtxVector2Norm(&rLeft.v);

   /* Build ray representing bottom edge */
   rBottom.p.X = (double)reg->bottomLoc.X;
   rBottom.p.Y = (double)reg->bottomLoc.Y;
   radians = reg->bottomAngle * (M_PI/DMTX_HOUGH_RES);
   rBottom.v.X = cos(radians);
   rBottom.v.Y = sin(radians);
   rBottom.tMin = 0.0;
   rBottom.tMax = dmtxVector2Norm(&rBottom.v);

   /* Build ray representing top edge */
   if(reg->topKnown != 0) {
      rTop.p.X = (double)reg->topLoc.X;
      rTop.p.Y = (double)reg->topLoc.Y;
      radians = reg->topAngle * (M_PI/DMTX_HOUGH_RES);
      rTop.v.X = cos(radians);
      rTop.v.Y = sin(radians);
      rTop.tMin = 0.0;
      rTop.tMax = dmtxVector2Norm(&rTop.v);
   }
   else {
      rTop.p.X = (double)reg->locT.X;
      rTop.p.Y = (double)reg->locT.Y;
      radians = reg->bottomAngle * (M_PI/DMTX_HOUGH_RES);
      rTop.v.X = cos(radians);
      rTop.v.Y = sin(radians);
      rTop.tMin = 0.0;
      rTop.tMax = rBottom.tMax;
   }

   /* Build ray representing right edge */
   if(reg->rightKnown != 0) {
      rRight.p.X = (double)reg->rightLoc.X;
      rRight.p.Y = (double)reg->rightLoc.Y;
      radians = reg->rightAngle * (M_PI/DMTX_HOUGH_RES);
      rRight.v.X = cos(radians);
      rRight.v.Y = sin(radians);
      rRight.tMin = 0.0;
      rRight.tMax = dmtxVector2Norm(&rRight.v);
   }
   else {
      rRight.p.X = (double)reg->locR.X;
      rRight.p.Y = (double)reg->locR.Y;
      radians = reg->leftAngle * (M_PI/DMTX_HOUGH_RES);
      rRight.v.X = cos(radians);
      rRight.v.Y = sin(radians);
      rRight.tMin = 0.0;
      rRight.tMax = rLeft.tMax;
   }

   /* Calculate 4 corners, real or imagined */
   if(dmtxRay2Intersect(&p00, &rLeft, &rBottom) == DmtxFail)
      return DmtxFail;

   if(dmtxRay2Intersect(&p10, &rBottom, &rRight) == DmtxFail)
      return DmtxFail;

   if(dmtxRay2Intersect(&p11, &rRight, &rTop) == DmtxFail)
      return DmtxFail;

   if(dmtxRay2Intersect(&p01, &rTop, &rLeft) == DmtxFail)
      return DmtxFail;

   if(dmtxRegionUpdateCorners(dec, reg, p00, p10, p11, p01) != DmtxPass)
      return DmtxFail;

   return DmtxPass;
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
 * \brief  Read color of Data Matrix module location
 * \param  dec
 * \param  reg
 * \param  symbolRow
 * \param  symbolCol
 * \param  sizeIdx
 * \return Averaged module color
 */
static int
ReadModuleColor(DmtxDecode *dec, DmtxRegion *reg, int symbolRow, int symbolCol,
      int sizeIdx, int colorPlane)
{
   int i;
   int symbolRows, symbolCols;
   int color, colorTmp;
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

      //fprintf(stdout, "%dx%d\n", (int)(p.X + 0.5), (int)(p.Y + 0.5));

      dmtxDecodeGetPixelValue(dec, (int)(p.X + 0.5), (int)(p.Y + 0.5),
            colorPlane, &colorTmp);
      color += colorTmp;
   }
   //fprintf(stdout, "\n");
   return color/5;
}

/**
 * \brief  Determine barcode size, expressed in modules
 * \param  image
 * \param  reg
 * \return DmtxPass | DmtxFail
 */
static DmtxPassFail
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
//   DmtxImage *img;

//   img = dec->image;
   bestSizeIdx = DmtxUndefined;
   bestContrast = 0;
   bestColorOnAvg = bestColorOffAvg = 0;

   if(dec->sizeIdxExpected == DmtxSymbolShapeAuto) {
      sizeIdxBeg = 0;
      sizeIdxEnd = DmtxSymbolSquareCount + DmtxSymbolRectCount;
   }
   else if(dec->sizeIdxExpected == DmtxSymbolSquareAuto) {
      sizeIdxBeg = 0;
      sizeIdxEnd = DmtxSymbolSquareCount;
   }
   else if(dec->sizeIdxExpected == DmtxSymbolRectAuto) {
      sizeIdxBeg = DmtxSymbolSquareCount;
      sizeIdxEnd = DmtxSymbolSquareCount + DmtxSymbolRectCount;
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
         color = ReadModuleColor(dec, reg, row, col, sizeIdx, reg->flowBegin.plane);
         if((col & 0x01) != 0x00)
            colorOffAvg += color;
         else
            colorOnAvg += color;
      }

      /* Sum module colors along vertical calibration bar */
      col = symbolCols - 1;
      for(row = 0; row < symbolRows; row++) {
         color = ReadModuleColor(dec, reg, row, col, sizeIdx, reg->flowBegin.plane);
         if((row & 0x01) != 0x00)
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
   if(bestSizeIdx == DmtxUndefined || bestContrast < 20)
      return DmtxFail;

   reg->sizeIdx = bestSizeIdx;
   reg->onColor = bestColorOnAvg;
   reg->offColor = bestColorOffAvg;

   reg->symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, reg->sizeIdx);
   reg->symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, reg->sizeIdx);
   reg->mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, reg->sizeIdx);
   reg->mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, reg->sizeIdx);

   /* Tally jumps on horizontal calibration bar to verify sizeIdx */
   jumpCount = CountJumpTally(dec, reg, 0, reg->symbolRows - 1, DmtxDirRight);
   errors = abs(1 + jumpCount - reg->symbolCols);
   if(jumpCount < 0 || errors > 2)
      return DmtxFail;

   /* Tally jumps on vertical calibration bar to verify sizeIdx */
   jumpCount = CountJumpTally(dec, reg, reg->symbolCols - 1, 0, DmtxDirUp);
   errors = abs(1 + jumpCount - reg->symbolRows);
   if(jumpCount < 0 || errors > 2)
      return DmtxFail;

   /* Tally jumps on horizontal finder bar to verify sizeIdx */
   errors = CountJumpTally(dec, reg, 0, 0, DmtxDirRight);
   if(jumpCount < 0 || errors > 2)
      return DmtxFail;

   /* Tally jumps on vertical finder bar to verify sizeIdx */
   errors = CountJumpTally(dec, reg, 0, 0, DmtxDirUp);
   if(errors < 0 || errors > 2)
      return DmtxFail;

   /* Tally jumps on surrounding whitespace, else fail */
   errors = CountJumpTally(dec, reg, 0, -1, DmtxDirRight);
   if(errors < 0 || errors > 2)
      return DmtxFail;

   errors = CountJumpTally(dec, reg, -1, 0, DmtxDirUp);
   if(errors < 0 || errors > 2)
      return DmtxFail;

   errors = CountJumpTally(dec, reg, 0, reg->symbolRows, DmtxDirRight);
   if(errors < 0 || errors > 2)
      return DmtxFail;

   errors = CountJumpTally(dec, reg, reg->symbolCols, 0, DmtxDirUp);
   if(errors < 0 || errors > 2)
      return DmtxFail;

   return DmtxPass;
}

/**
 * \brief  Count the number of number of transitions between light and dark
 * \param  img
 * \param  reg
 * \param  xStart
 * \param  yStart
 * \param  dir
 * \return Jump count
 */
static int
CountJumpTally(DmtxDecode *dec, DmtxRegion *reg, int xStart, int yStart, DmtxDirection dir)
{
   int x, xInc = 0;
   int y, yInc = 0;
   int state = DmtxModuleOn;
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
      state = DmtxModuleOff;

   darkOnLight = (int)(reg->offColor > reg->onColor);
   jumpThreshold = abs((int)(0.4 * (reg->onColor - reg->offColor) + 0.5));
   color = ReadModuleColor(dec, reg, yStart, xStart, reg->sizeIdx, reg->flowBegin.plane);
   tModule = (darkOnLight) ? reg->offColor - color : color - reg->offColor;

   for(x = xStart + xInc, y = yStart + yInc;
         (dir == DmtxDirRight && x < reg->symbolCols) ||
         (dir == DmtxDirUp && y < reg->symbolRows);
         x += xInc, y += yInc) {

      tPrev = tModule;
      color = ReadModuleColor(dec, reg, y, x, reg->sizeIdx, reg->flowBegin.plane);
      tModule = (darkOnLight) ? reg->offColor - color : color - reg->offColor;

      if(state == DmtxModuleOff) {
         if(tModule > tPrev + jumpThreshold) {
            jumpCount++;
            state = DmtxModuleOn;
         }
      }
      else {
         if(tModule < tPrev - jumpThreshold) {
            jumpCount++;
            state = DmtxModuleOff;
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
   int err;
   int patternIdx, coefficientIdx;
   int compass, compassMax;
   int mag[4] = { 0 };
   int xAdjust, yAdjust;
   int color, colorPattern[8];
   DmtxPointFlow flow;

   for(patternIdx = 0; patternIdx < 8; patternIdx++) {
      xAdjust = loc.X + dmtxPatternX[patternIdx];
      yAdjust = loc.Y + dmtxPatternY[patternIdx];
      err = dmtxDecodeGetPixelValue(dec, xAdjust, yAdjust, colorPlane,
            &colorPattern[patternIdx]);
      if(err == DmtxFail)
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
   int strongIdx;
   int attempt, attemptDiff;
   int occupied;
   unsigned char *cache;
   DmtxPixelLoc loc;
   DmtxPointFlow flow[8];

   attempt = (sign < 0) ? center.depart : (center.depart+4)%8;

   occupied = 0;
   strongIdx = DmtxUndefined;
   for(i = 0; i < 8; i++) {

      loc.X = center.loc.X + dmtxPatternX[i];
      loc.Y = center.loc.Y + dmtxPatternY[i];

      cache = dmtxDecodeGetCache(dec, loc.X, loc.Y);
      if(cache == NULL)
         continue;

      if((int)(*cache & 0x80) != 0x00) {
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

      if(strongIdx == DmtxUndefined || flow[i].mag > flow[strongIdx].mag ||
            (flow[i].mag == flow[strongIdx].mag && ((i & 0x01) != 0))) {
         strongIdx = i;
      }
   }

   return (strongIdx == DmtxUndefined) ? dmtxBlankEdge : flow[strongIdx];
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
   DmtxFollow follow;

   follow.loc = reg->flowBegin.loc;
   follow.step = 0;
   follow.ptr = dmtxDecodeGetCache(dec, follow.loc.X, follow.loc.Y);
   assert(follow.ptr != NULL);
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
FollowSeekLoc(DmtxDecode *dec, DmtxPixelLoc loc)
{
   DmtxFollow follow;

   follow.loc = loc;
   follow.step = 0;
   follow.ptr = dmtxDecodeGetCache(dec, follow.loc.X, follow.loc.Y);
   assert(follow.ptr != NULL);
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
   int patternIdx;
   int stepMod;
   int factor;
   DmtxFollow follow;

   assert(abs(sign) == 1);
   assert((int)(followBeg.neighbor & 0x40) != 0x00);

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
   }

   follow.step = followBeg.step + sign;
   follow.ptr = dmtxDecodeGetCache(dec, follow.loc.X, follow.loc.Y);
   assert(follow.ptr != NULL);
   follow.neighbor = *follow.ptr;

   return follow;
}

/**
 *
 *
 */
static DmtxFollow
FollowStep2(DmtxDecode *dec, DmtxFollow followBeg, int sign)
{
   int patternIdx;
   DmtxFollow follow;

   assert(abs(sign) == 1);
   assert((int)(followBeg.neighbor & 0x40) != 0x00);

   patternIdx = (sign < 0) ? followBeg.neighbor & 0x07 : ((followBeg.neighbor & 0x38) >> 3);
   follow.loc.X = followBeg.loc.X + dmtxPatternX[patternIdx];
   follow.loc.Y = followBeg.loc.Y + dmtxPatternY[patternIdx];

   follow.step = followBeg.step + sign;
   follow.ptr = dmtxDecodeGetCache(dec, follow.loc.X, follow.loc.Y);
   assert(follow.ptr != NULL);
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
static DmtxPassFail
TrailBlazeContinuous(DmtxDecode *dec, DmtxRegion *reg, DmtxPointFlow flowBegin, int maxDiagonal)
{
   int posAssigns, negAssigns, clears;
   int sign;
   int steps;
   unsigned char *cache, *cacheNext, *cacheBeg;
   DmtxPointFlow flow, flowNext;
   DmtxPixelLoc boundMin, boundMax;

   boundMin = boundMax = flowBegin.loc;
   cacheBeg = dmtxDecodeGetCache(dec, flowBegin.loc.X, flowBegin.loc.Y);
   if(cacheBeg == NULL)
      return DmtxFail;
   *cacheBeg = (0x80 | 0x40); /* Mark location as visited and assigned */

   reg->flowBegin = flowBegin;

   posAssigns = negAssigns = 0;
   for(sign = 1; sign >= -1; sign -= 2) {

      flow = flowBegin;
      cache = cacheBeg;

      for(steps = 0; ; steps++) {

         if(maxDiagonal != DmtxUndefined && (boundMax.X - boundMin.X > maxDiagonal ||
               boundMax.Y - boundMin.Y > maxDiagonal))
            break;

         /* Find the strongest eligible neighbor */
         flowNext = FindStrongestNeighbor(dec, flow, sign);
         if(flowNext.mag < 50)
            break;

         /* Get the neighbor's cache location */
         cacheNext = dmtxDecodeGetCache(dec, flowNext.loc.X, flowNext.loc.Y);
         if(cacheNext == NULL)
            break;
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

/*       CALLBACK_POINT_PLOT(flow.loc, (sign > 0) ? 2 : 3, 1, 2); */
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
   if(maxDiagonal != DmtxUndefined && (boundMax.X - boundMin.X > maxDiagonal ||
         boundMax.Y - boundMin.Y > maxDiagonal))
      return DmtxFail;

   return DmtxPass;
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
   DmtxBoolean onEdge;
   int distSq, distSqMax;
   int travel, outward;
   int xDiff, yDiff;
   int steps;
   int stepDir, dirMap[] = { 0, 1, 2, 7, 8, 3, 6, 5, 4 };
   DmtxPassFail err;
   DmtxPixelLoc beforeStep, afterStep;
   DmtxPointFlow flow, flowNext;
   DmtxPixelLoc loc0;
   int xStep, yStep;

   loc0 = line.loc;
   flow = GetPointFlow(dec, reg->flowBegin.plane, loc0, dmtxNeighborNone);
   distSqMax = (line.xDelta * line.xDelta) + (line.yDelta * line.yDelta);
   steps = 0;
   onEdge = DmtxTrue;

   beforeStep = loc0;
   beforeCache = dmtxDecodeGetCache(dec, loc0.X, loc0.Y);
   if(beforeCache == NULL)
      return DmtxFail;
   else
      *beforeCache = 0x00; /* probably should just overwrite one direction */

   do {
      if(onEdge == DmtxTrue) {
         flowNext = FindStrongestNeighbor(dec, flow, streamDir);
         if(flowNext.mag == DmtxUndefined)
            break;

         err = BresLineGetStep(line, flowNext.loc, &travel, &outward);
         if (err == DmtxFail) { return DmtxFail; }

         if(flowNext.mag < 50 || outward < 0 || (outward == 0 && travel < 0)) {
            onEdge = DmtxFalse;
         }
         else {
            BresLineStep(&line, travel, outward);
            flow = flowNext;
         }
      }

      if(onEdge == DmtxFalse) {
         BresLineStep(&line, 1, 0);
         flow = GetPointFlow(dec, reg->flowBegin.plane, line.loc, dmtxNeighborNone);
         if(flow.mag > 50)
            onEdge = DmtxTrue;
      }

      afterStep = line.loc;
      afterCache = dmtxDecodeGetCache(dec, afterStep.X, afterStep.Y);
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

   sign = 0;

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
      if(houghAvoid == DmtxUndefined) {
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

         if((int)houghTest[i] == 0)
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

/*    CALLBACK_POINT_PLOT(follow.loc, (sign > 1) ? 4 : 3, 1, 2); */

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
FindBestSolidLine2(DmtxDecode *dec, DmtxPixelLoc loc0, int tripSteps, int sign, int houghAvoid)
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

   follow = FollowSeekLoc(dec, loc0);
   rHp = line.locBeg = line.locPos = line.locNeg = follow.loc;
   line.stepBeg = line.stepPos = line.stepNeg = 0;

   /* Predetermine which angles to test */
   for(i = 0; i < DMTX_HOUGH_RES; i++) {
      if(houghAvoid == DmtxUndefined) {
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

         if((int)houghTest[i] == 0)
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

/*    CALLBACK_POINT_PLOT(follow.loc, (sign > 1) ? 4 : 3, 1, 2); */

      follow = FollowStep2(dec, follow, sign);
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
static DmtxPassFail
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

      if(posRunning != 0) {
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

      if(negRunning != 0) {
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

/*  CALLBACK_POINT_PLOT(followPos.loc, 2, 1, 2);
    CALLBACK_POINT_PLOT(followNeg.loc, 4, 1, 2); */

      followPos = FollowStep(dec, reg, followPos, +1);
      followNeg = FollowStep(dec, reg, followNeg, -1);
   }
   line->devn = max(posWanderMaxLock - posWanderMinLock, negWanderMaxLock - negWanderMinLock)/256;
   line->distSq = distSqMax;

/* CALLBACK_POINT_PLOT(posMax, 2, 1, 1);
   CALLBACK_POINT_PLOT(negMax, 2, 1, 1); */

   return DmtxPass;
}

/**
 *
 *
 */
static DmtxPassFail
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

   if(dec->sizeIdxExpected == DmtxSymbolSquareAuto ||
         (dec->sizeIdxExpected >= DmtxSymbol10x10 &&
         dec->sizeIdxExpected <= DmtxSymbol144x144))
      symbolShape = DmtxSymbolSquareAuto;
   else if(dec->sizeIdxExpected == DmtxSymbolRectAuto ||
         (dec->sizeIdxExpected >= DmtxSymbol8x18 &&
         dec->sizeIdxExpected <= DmtxSymbol16x48))
      symbolShape = DmtxSymbolRectAuto;
   else
      symbolShape = DmtxSymbolShapeAuto;

   /* Determine end locations of test line */
   if(edgeLoc == DmtxEdgeTop) {
      streamDir = reg->polarity * -1;
      avoidAngle = reg->leftLine.angle;
      follow = FollowSeekLoc(dec, reg->locT);
      pTmp.X = 0.8;
      pTmp.Y = (symbolShape == DmtxSymbolRectAuto) ? 0.2 : 0.6;
   }
   else {
      assert(edgeLoc == DmtxEdgeRight);
      streamDir = reg->polarity;
      avoidAngle = reg->bottomLine.angle;
      follow = FollowSeekLoc(dec, reg->locR);
      pTmp.X = (symbolShape == DmtxSymbolSquareAuto) ? 0.7 : 0.9;
      pTmp.Y = 0.8;
   }

   dmtxMatrix3VMultiplyBy(&pTmp, reg->fit2raw);
   loc1.X = (int)(pTmp.X + 0.5);
   loc1.Y = (int)(pTmp.Y + 0.5);

   loc0 = follow.loc;
   line = BresLineInit(loc0, loc1, locOrigin);
   steps = TrailBlazeGapped(dec, reg, line, streamDir);

   bestLine = FindBestSolidLine2(dec, loc0, steps, streamDir, avoidAngle);
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

   return DmtxPass;
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
   if(line.steep != 0) {
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

/* CALLBACK_POINT_PLOT(loc0, 3, 1, 1);
   CALLBACK_POINT_PLOT(loc1, 3, 1, 1); */

   return line;
}

/**
 *
 *
 */
static DmtxPassFail
BresLineGetStep(DmtxBresLine line, DmtxPixelLoc target, int *travel, int *outward)
{
   /* Determine necessary step along and outward from Bresenham line */
   if(line.steep != 0) {
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

   return DmtxPass;
}

/**
 *
 *
 */
static DmtxPassFail
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
      if(lineNew.steep != 0) {
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
      if(lineNew.steep != 0) {
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

   return DmtxPass;
}

/**
 *
 *
 */
#ifdef NOTDEFINED
static void
WriteDiagnosticImage(DmtxDecode *dec, DmtxRegion *reg, char *imagePath)
{
   int row, col;
   int width, height;
   unsigned char *cache;
   int rgb[3];
   FILE *fp;
   DmtxVector2 p;
   DmtxImage *img;

   assert(reg != NULL);

   fp = fopen(imagePath, "wb");
   if(fp == NULL) {
      exit(3);
   }

   width = dmtxDecodeGetProp(dec, DmtxPropWidth);
   height = dmtxDecodeGetProp(dec->image, DmtxPropHeight);

   img = dmtxImageCreate(NULL, width, height, DmtxPack24bppRGB);

   /* Populate image */
   for(row = 0; row < height; row++) {
      for(col = 0; col < width; col++) {

         cache = dmtxDecodeGetCache(dec, col, row);
         if(cache == NULL) {
            rgb[0] = 0;
            rgb[1] = 0;
            rgb[2] = 128;
         }
         else {
            dmtxDecodeGetPixelValue(dec, col, row, 0, &rgb[0]);
            dmtxDecodeGetPixelValue(dec, col, row, 1, &rgb[1]);
            dmtxDecodeGetPixelValue(dec, col, row, 2, &rgb[2]);

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

   /* Write additional markers */
   rgb[0] = 255;
   rgb[1] = 0;
   rgb[2] = 0;
   dmtxImageSetRgb(img, reg->topLoc.X, reg->topLoc.Y, rgb);
   dmtxImageSetRgb(img, reg->rightLoc.X, reg->rightLoc.Y, rgb);

   /* Write image to PNM file */
   fprintf(fp, "P6\n%d %d\n255\n", width, height);
   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {
         dmtxImageGetRgb(img, col, row, rgb);
         fwrite(rgb, sizeof(char), 3, fp);
      }
   }

   dmtxImageDestroy(&img);

   fclose(fp);
}
#endif
