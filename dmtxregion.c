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
 *
 *
 */
extern DmtxRegion
dmtxDecodeFindNextRegion(DmtxDecode *dec, DmtxTime *timeout)
{
   DmtxScanGrid *grid;
   DmtxPixelLoc loc;
   DmtxRegion   reg;

   grid = &(dec->grid);

   /* Continue scanning until we run out of image or run out of time */
   for(;;) {

      /* Check if grid has been completely traversed */
      if(grid->extent < grid->minExtent) {
         reg.found = DMTX_REGION_EOF;
         break;
      }

      /* Pick up where last scan left off */
      loc = GetGridCoordinates(grid);

      /* Increment grid away from this location to prevent repeat visits */
      IncrementPixelProgress(grid);

      /* Scan this pixel for presence of a valid barcode edge */
      reg = dmtxScanPixel(dec, loc);

      /* Found a barcode region */
      if(reg.found == DMTX_REGION_FOUND)
         break;

      /* Ran out of time */
      if(timeout != NULL && dmtxTimeExceeded(*timeout)) {
         reg.found = DMTX_REGION_TIMEOUT;
         break;
      }
   }

   return reg;
}

/**
 * @param dec      pointer to DmtxDecode information struct
 * @param dir      scan direction (DmtxUp|DmtxRight)
 * @param lineNbr  number of image row or column to be scanned
 * @return         number of barcodes scanned
 */
extern DmtxRegion
dmtxScanPixel(DmtxDecode *dec, DmtxPixelLoc loc)
{
   DmtxEdgeSubPixel edgeStart;
   DmtxRay2 ray0, ray1;
   DmtxCompassEdge compassEdge;
   DmtxRegion reg;

   /* Assume region is not found unless scan finds one below */
   memset(&reg, 0x00, sizeof(DmtxRegion));
   reg.found = DMTX_REGION_NOT_FOUND;

   if(loc.X < 0 || loc.X >= dec->image->width ||
         loc.Y < 0 || loc.Y >= dec->image->height)
      return reg;

/* if(dmtxGetMaskProperty(loc.X, loc.Y) == DMTX_DO_NOT_FOLLOW)
      return reg; */

   /* Test whether this pixel is sitting on a raw edge in any direction */
   compassEdge = GetCompassEdge(dec->image, loc.X, loc.Y, DMTX_ALL_COMPASS_DIRS);
   if(compassEdge.magnitude < 60)
      return reg;

   /* If detected edge is strong then find its subpixel location */
   edgeStart = FindZeroCrossing(dec->image, loc.X, loc.Y, compassEdge);
   edgeStart.compass = compassEdge; /* XXX for now ... need to preserve scanDir */
   if(!edgeStart.isEdge)
      return reg;

   /* Next follow the edge to its end in both directions */
   ray0 = FollowEdge(dec->image, loc.X, loc.Y, edgeStart, 1);
   ray1 = FollowEdge(dec->image, loc.X, loc.Y, edgeStart, -1);

   /* Define first edge based on travel limits of detected edge */
   if(MatrixRegionAlignFirstEdge(dec->image, &reg, &edgeStart, ray0, ray1) != DMTX_SUCCESS)
      return reg;

   /* Capture copy of first edge boundaries */
   /* ... if this turns out to be a dead end then we'll mark this single
      edge as 'do not follow' for future passes */
   /* cornerCapture.00 = xyz; cornerCapture.01 = xyz; */

   /* Define second edge based on best match of 4 possible orientations */
   if(MatrixRegionAlignSecondEdge(dec->image, &reg) != DMTX_SUCCESS)
      return reg;

   /* Define right edge */
   if(MatrixRegionAlignRightEdge(dec->image, &reg) != DMTX_SUCCESS)
      return reg;

   /* Define top edge */
   if(MatrixRegionAlignTopEdge(dec->image, &reg) != DMTX_SUCCESS)
      return reg;

   /* Calculate the best fitting symbol size */
   if(MatrixRegionFindSize(dec->image, &reg) != DMTX_SUCCESS)
      return reg; /* mark cornerCapture line as 'no not follow' */

   /* Found a valid matrix region */
   reg.found = DMTX_REGION_FOUND;
   return reg;
}

/**
 *
 *
 */
static void
ClampIntRange(int *value, int min, int max)
{
   assert(*value >= min - 1);
   assert(*value <= max + 1);

   if(*value < min)
      *value = min;
   else if(*value > max)
      *value = max;
}

/**
 *
 *
 */
static DmtxCompassEdge
GetCompassEdge(DmtxImage *image, int x, int y, int edgeScanDirs)
{
   int dirIdx, dirVal[] = { DmtxCompassDirNeg45, DmtxCompassDir0, DmtxCompassDir45, DmtxCompassDir90 };
   int patternIdx, coefficientIdx;
   int xAdjust, yAdjust;
   double mag0, mag90;
   static const double coefficient[] = {  0,  1,  2,  1,  0, -1, -2, -1 };
   static const int patternX[] =       { -1,  0,  1,  1,  1,  0, -1, -1 };
   static const int patternY[] =       { -1, -1, -1,  0,  1,  1,  1,  0 };
   DmtxRgb rgb;
   DmtxCompassEdge edge, maxEdge, *compassCache;
   DmtxColor3 color, black = { 0.0, 0.0, 0.0 }; /* XXX move black to a global scope later */

   assert(edgeScanDirs);

   /* Set maxEdge to invalid state */
   maxEdge.assigned = 0;
   maxEdge.edgeDir = -1;
   maxEdge.scanDir = -1;
   maxEdge.intensity = black;
   maxEdge.magnitude = 0.0;

   if(x <= 0 || x >= image->width - 1 || y <= 0 || y >= image->height - 1)
      return maxEdge; /* XXX should really communicate failure with a dedicated value instead */

   compassCache = &(image->compass[y * image->width + x]);

   if(compassCache->assigned == 1 && (edgeScanDirs & compassCache->edgeDir))
      return *compassCache;

   mag0 = mag90 = -1.0;

   /* Calculate this pixel's edge intensity for each direction (-45, 0, 45, 90) */
   for(dirIdx = 0; dirIdx <= 3; dirIdx++) {

      /* Only scan for edge if this direction was requested */
      if(!(dirVal[dirIdx] & edgeScanDirs))
         continue;

      edge.edgeDir = dirVal[dirIdx];
      edge.scanDir = -1;
      edge.intensity = black;
      edge.magnitude = 0.0;

      /* Set maxEdge on the first iteration */
      if(maxEdge.edgeDir == -1)
         maxEdge = edge;

      /* Add portion from each position in the convolution matrix pattern */
      for(patternIdx = 0; patternIdx < 8; patternIdx++) {
         xAdjust = x + patternX[patternIdx];
         yAdjust = y + patternY[patternIdx];

         /* Accommodate 1 pixel beyond edge of image with nearest neighbor value */
         ClampIntRange(&xAdjust, 0, image->width - 1);
         ClampIntRange(&yAdjust, 0, image->height - 1);

         /* Weight pixel value by appropriate coefficient in convolution matrix */
         coefficientIdx = (patternIdx - dirIdx + 8) % 8;
         dmtxPixelFromImage(rgb, image, xAdjust, yAdjust);
         dmtxColor3FromPixel(&color, rgb);
         dmtxColor3AddTo(&edge.intensity, dmtxColor3ScaleBy(&color, coefficient[coefficientIdx]));
      }
      edge.magnitude = dmtxColor3Mag(&edge.intensity);

      if(edge.edgeDir == DmtxCompassDir0)
         mag0 = edge.magnitude;
      else if(edge.edgeDir == DmtxCompassDir90)
         mag90 = edge.magnitude;

      /* Capture the strongest edge direction and its intensity */
      if(edge.magnitude > maxEdge.magnitude)
         maxEdge = edge;
   }
   if(mag0 > -1.0 && mag90 > -1.0)
      maxEdge.scanDir = (mag0 > mag90) ? DmtxCompassDir0 : DmtxCompassDir90;

   if(edgeScanDirs == DMTX_ALL_COMPASS_DIRS) {
      *compassCache = maxEdge;
      compassCache->assigned = 1;
   }

   return maxEdge;
}

/**
 * We have found an edge candidate whenever we find 3 pixels in a
 * row that share the same strongest direction and the middle one
 * had the highest intensity (or a first-place tie with one of its
 * neighbors)
 */
static DmtxEdgeSubPixel
FindZeroCrossing(DmtxImage *image, int x, int y, DmtxCompassEdge compassStart)
{
   double accelPrev, accelNext, frac;
   DmtxCompassEdge compassPrev, compassNext;
   DmtxEdgeSubPixel subPixel;

   assert(compassStart.scanDir == DmtxCompassDir0 || compassStart.scanDir == DmtxCompassDir90);

   subPixel.isEdge = 0;
   subPixel.xInt = x;
   subPixel.yInt = y;
   subPixel.xFrac = 0.0;
   subPixel.yFrac = 0.0;
   subPixel.compass = GetCompassEdge(image, x, y, compassStart.edgeDir);

   if(subPixel.compass.magnitude < 60)
      return subPixel;

   if(dmtxColor3Dot(&subPixel.compass.intensity, &compassStart.intensity) < 0)
      return subPixel;

   if(compassStart.scanDir == DmtxCompassDir0) {
      compassPrev = GetCompassEdge(image, x-1, y, compassStart.edgeDir);
      compassNext = GetCompassEdge(image, x+1, y, compassStart.edgeDir);
   }
   else { /* DmtxCompassDir90 */
      compassPrev = GetCompassEdge(image, x, y-1, compassStart.edgeDir);
      compassNext = GetCompassEdge(image, x, y+1, compassStart.edgeDir);
   }

   /* Calculate 2nd derivatives left and right of center */
   accelPrev = subPixel.compass.magnitude - compassPrev.magnitude;
   accelNext = compassNext.magnitude - subPixel.compass.magnitude;

   /* If it looks like an edge then interpolate subpixel loc based on 0 crossing */
   if(accelPrev * accelNext < DMTX_ALMOST_ZERO) {
      frac = (fabs(accelNext - accelPrev) > DMTX_ALMOST_ZERO) ?
            (accelPrev / (accelPrev - accelNext)) - 0.5 : 0.0;

      subPixel.isEdge = 1;
      subPixel.xFrac = (compassStart.scanDir == DmtxCompassDir0) ? frac : 0.0;
      subPixel.yFrac = (compassStart.scanDir == DmtxCompassDir90) ? frac : 0.0;
   }

   return subPixel;
}

/**
 *
 *
 */
static DmtxRay2
FollowEdge(DmtxImage *image, int x, int y, DmtxEdgeSubPixel edgeStart, int forward)
{
   int xFollow, yFollow;
   int xIncrement, yIncrement;
   DmtxEdgeSubPixel edge, edge0, edge1;
   DmtxVector2 p, pStart, pSoft, pHard, pStep;
   double strong0, strong1;
   DmtxCompassEdge compass;
   DmtxRay2 ray;

   memset(&ray, 0x00, sizeof(DmtxRay2));

   /* No edge here, thanks for playing */
   if(!edgeStart.isEdge)
      return ray;

   pStart.X = edgeStart.xInt + edgeStart.xFrac;
   pStart.Y = edgeStart.yInt + edgeStart.yFrac;
   pSoft = pHard = pStart;

   edge = edgeStart;
   compass = edgeStart.compass;

   /* If we have a true edge then continue to follow it forward */
   if(compass.scanDir == DmtxCompassDir0) {
      xIncrement = 0;
      yIncrement = forward;
   }
   else {
      xIncrement = forward;
      yIncrement = 0;
   }

   xFollow = x + xIncrement;
   yFollow = y + yIncrement;

   while(edge.isEdge &&
         xFollow >= 0 && xFollow < image->width &&
         yFollow >= 0 && yFollow < image->height) {

      edge = FindZeroCrossing(image, xFollow, yFollow, compass);

      if(!edge.isEdge) {
         edge0 = FindZeroCrossing(image, xFollow + yIncrement, yFollow + xIncrement, compass);
         edge1 = FindZeroCrossing(image, xFollow - yIncrement, yFollow - xIncrement, compass);
         if(edge0.isEdge && edge1.isEdge) {
            strong0 = dmtxColor3Dot(&edge0.compass.intensity, &compass.intensity);
            strong1 = dmtxColor3Dot(&edge1.compass.intensity, &compass.intensity);
            edge = (strong0 > strong1) ? edge0 : edge1;
         }
         else {
            edge = (edge0.isEdge) ? edge0 : edge1;
         }

         if(!edge.isEdge) {
            edge0 = FindZeroCrossing(image, xFollow + 2*yIncrement, yFollow + 2*xIncrement, compass);
            edge1 = FindZeroCrossing(image, xFollow - 2*yIncrement, yFollow - 2*xIncrement, compass);
            if(edge0.isEdge && edge1.isEdge) {
               strong0 = dmtxColor3Dot(&edge0.compass.intensity, &compass.intensity);
               strong1 = dmtxColor3Dot(&edge1.compass.intensity, &compass.intensity);
               edge = (strong0 > strong1) ? edge0 : edge1;
            }
            else {
               edge = (edge0.isEdge) ? edge0 : edge1;
            }
         }
      }

      if(edge.isEdge) {
         p.X = edge.xInt + edge.xFrac;
         p.Y = edge.yInt + edge.yFrac;

         /* Outline of follower in 2nd pane */
         CALLBACK_POINT_PLOT(p, 1, 1, DMTX_DISPLAY_POINT);

         if(edge.compass.magnitude > 0.50 * compass.magnitude)
            pSoft = p;
         if(edge.compass.magnitude > 0.90 * compass.magnitude)
            pHard = p;
      }

      xFollow = (int)(edge.xInt + edge.xFrac + 0.5) + xIncrement;
      yFollow = (int)(edge.yInt + edge.yFrac + 0.5) + yIncrement;
   }

/* CALLBACK_POINT_PLOT(pHard, 1, 4, DMTX_DISPLAY_SQUARE); */

   dmtxVector2Sub(&pStep, &pHard, &pStart);
   if(dmtxVector2Mag(&pStep) < 4)
      return ray;

   ray.isDefined = 1;
   ray.tMin = 0;
   ray.p = pStart;
   dmtxVector2Norm(&pStep);
   ray.v = pStep;
   ray.tMax = dmtxDistanceAlongRay2(&ray, &pSoft);

   return ray;
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

   dmtxVector2Norm(dmtxVector2Sub(&vA, &c1, &c0));
   dmtxVector2Norm(dmtxVector2Sub(&vB, &c2, &c1));

   dmtxMatrix3Rotate(m, M_PI - angle);
   dmtxMatrix3VMultiplyBy(&vA, m);

   return dmtxVector2Dot(&vA, &vB);
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionUpdateXfrms(DmtxImage *image, DmtxRegion *reg)
{
   DmtxVector2 vOT, vOR, vTmp;
   double tx, ty, phi, shx, scx, scy, skx, sky;
   double dimOT, dimOR, dimTX, dimRX, ratio;
   DmtxMatrix3 m, mtxy, mphi, mshx, mscxy, msky, mskx, mTmp;
   DmtxCorners corners;

   assert((reg->corners.known & DmtxCorner00) && (reg->corners.known & DmtxCorner01));

   /* Make copy of known corners to update with temporary values */
   corners = reg->corners;

   if(corners.c00.X < 0.0 || corners.c00.Y < 0.0 ||
      corners.c01.X < 0.0 || corners.c01.Y < 0.0)
      return DMTX_FAILURE;

   if(corners.c00.X > image->width - 1 || corners.c00.Y > image->height - 1 ||
      corners.c01.X > image->width - 1 || corners.c01.Y > image->height - 1)
      return DMTX_FAILURE;

   dimOT = dmtxVector2Mag(dmtxVector2Sub(&vOT, &corners.c01, &corners.c00)); /* XXX could use MagSquared() */
   if(dimOT < 8)
      return DMTX_FAILURE;

   /* Bottom-right corner -- validate if known or create temporary value */
   if(corners.known & DmtxCorner10) {
      if(corners.c10.X < 0.0 || corners.c10.Y < 0.0 ||
            corners.c10.X > image->width - 1 ||
            corners.c10.Y > image->height - 1)
         return DMTX_FAILURE;
   }
   else {
      dmtxMatrix3Rotate(mTmp, -M_PI_2);
      dmtxMatrix3VMultiply(&vTmp, &vOT, mTmp);
      dmtxVector2Add(&corners.c10, &corners.c00, &vTmp);
   }

   dimOR = dmtxVector2Mag(dmtxVector2Sub(&vOR, &corners.c10, &corners.c00)); /* XXX could use MagSquared() */
   if(dimOR < 8)
      return DMTX_FAILURE;

   /* Solid edges are both defined now */
   if(RightAngleTrueness(corners.c01, corners.c00, corners.c10, M_PI_2) < 0.7)
      return DMTX_FAILURE;

   /* Top-right corner -- validate if known or create temporary value */
   if(corners.known & DmtxCorner11) {
      if(corners.c11.X < 0.0 || corners.c11.Y < 0.0 ||
            corners.c11.X > image->width - 1 ||
            corners.c11.Y > image->height - 1)
         return DMTX_FAILURE;
   }
   else {
      dmtxVector2Add(&corners.c11, &corners.c01, &vOR);
   }

   /* Verify that the 4 corners define a reasonably fat quadrilateral */
   dimTX = dmtxVector2Mag(dmtxVector2Sub(&vTmp, &corners.c11, &corners.c01)); /* XXX could use MagSquared() */
   dimRX = dmtxVector2Mag(dmtxVector2Sub(&vTmp, &corners.c11, &corners.c10)); /* XXX could use MagSquared() */
   if(dimTX < 8 || dimRX < 8)
      return DMTX_FAILURE;

   ratio = dimOT / dimRX;
   if(ratio < 0.5 || ratio > 2.0)
      return DMTX_FAILURE;

   ratio = dimOR / dimTX;
   if(ratio < 0.5 || ratio > 2.0)
      return DMTX_FAILURE;

   /* Test top-left corner for trueness */
   if(RightAngleTrueness(corners.c11, corners.c01, corners.c00, M_PI_2) < 0.7)
      return DMTX_FAILURE;

   /* Test bottom-right corner for trueness */
   if(RightAngleTrueness(corners.c00, corners.c10, corners.c11, M_PI_2) < 0.7)
      return DMTX_FAILURE;

   /* Test top-right corner for trueness */
   if(RightAngleTrueness(corners.c10, corners.c11, corners.c01, M_PI_2) < 0.7)
      return DMTX_FAILURE;

   /* Calculate values needed for transformations */
   tx = -1 * corners.c00.X;
   ty = -1 * corners.c00.Y;
   dmtxMatrix3Translate(mtxy, tx, ty);

   phi = atan2(vOT.X, vOT.Y);
   dmtxMatrix3Rotate(mphi, phi);

   /* Update transformation with values known so far */
   dmtxMatrix3Multiply(m, mtxy, mphi);

   dmtxMatrix3VMultiply(&vTmp, &corners.c10, m);

   shx = -vTmp.Y / vTmp.X;
   dmtxMatrix3Shear(mshx, 0.0, shx);

   scx = 1.0/vTmp.X;
   scy = 1.0/dmtxVector2Mag(&vOT);
   dmtxMatrix3Scale(mscxy, scx, scy);

   /* Update transformation with values known so far */
   dmtxMatrix3MultiplyBy(m, mshx);
   dmtxMatrix3MultiplyBy(m, mscxy);

   dmtxMatrix3VMultiply(&vTmp, &corners.c11, m);
   skx = vTmp.X;
   dmtxMatrix3LineSkewSide(mskx, 1.0, skx, 1.0);

   /* Update transformation with values known so far */
   dmtxMatrix3MultiplyBy(m, mskx);

   /* Update transformation with values known so far */
   dmtxMatrix3VMultiply(&vTmp, &corners.c11, m);
   sky = vTmp.Y;
   dmtxMatrix3LineSkewTop(msky, 1.0, sky, 1.0);

   /* Update region with final update */
   dmtxMatrix3Multiply(reg->raw2fit, m, msky);

   /* Create inverse matrix by reverse (avoid straight matrix inversion) */
   dmtxMatrix3LineSkewTopInv(msky, 1.0, sky, 1.0);
   dmtxMatrix3LineSkewSideInv(mskx, 1.0, skx, 1.0);
   dmtxMatrix3Scale(mscxy, 1.0/scx, 1.0/scy);
   dmtxMatrix3Shear(mshx, 0.0, -shx);
   dmtxMatrix3Rotate(mphi, -phi);
   dmtxMatrix3Translate(mtxy, -tx, -ty);

   dmtxMatrix3Multiply(m, msky, mskx);
   dmtxMatrix3MultiplyBy(m, mscxy);
   dmtxMatrix3MultiplyBy(m, mshx);
   dmtxMatrix3MultiplyBy(m, mphi);
   dmtxMatrix3Multiply(reg->fit2raw, m, mtxy);

   return DMTX_SUCCESS;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionAlignFirstEdge(DmtxImage *image, DmtxRegion *reg, DmtxEdgeSubPixel *edgeStart, DmtxRay2 ray0, DmtxRay2 ray1)
{
   DmtxRay2 rayFull;
   DmtxVector2 p0, p1, pTmp;

   if(!ray0.isDefined && !ray1.isDefined) {
      return DMTX_FAILURE;
   }
   else if(ray0.isDefined && ray1.isDefined) {
      /* XXX test if reasonably colinear? */
      dmtxPointAlongRay2(&p0, &ray0, ray0.tMax);
      dmtxPointAlongRay2(&p1, &ray1, ray1.tMax);
      rayFull.isDefined = 1;
      rayFull.p = p1;

      dmtxVector2Sub(&pTmp, &p0, &p1);
      if(dmtxVector2Norm(&pTmp) != DMTX_SUCCESS)
         return DMTX_FAILURE;

      rayFull.v = pTmp;
      rayFull.tMin = 0;
      rayFull.tMax = dmtxDistanceAlongRay2(&rayFull, &p0);
      /* XXX else choose longer of the two */
   }
   else {
      rayFull = (ray0.isDefined) ? ray0 : ray1;
   }

   /* Reject edges shorter than 10 pixels */
   if(rayFull.tMax < 10)
      return DMTX_FAILURE;

   dmtxPointAlongRay2(&pTmp, &rayFull, rayFull.tMax);
   SetCornerLoc(reg, DmtxCorner01, pTmp);
   SetCornerLoc(reg, DmtxCorner00, rayFull.p);

   if(MatrixRegionUpdateXfrms(image, reg) != DMTX_SUCCESS)
      return DMTX_FAILURE;

   /* Top-right pane showing first edge fit */
/* CALLBACK_DECODE_FUNC1(buildMatrixCallback2, dec, reg); */

   return DMTX_SUCCESS;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static void
SetCornerLoc(DmtxRegion *reg, DmtxCornerLoc cornerLoc, DmtxVector2 point)
{
   switch(cornerLoc) {
      case DmtxCorner00:
         reg->corners.c00 = point;
         break;
      case DmtxCorner10:
         reg->corners.c10 = point;
         break;
      case DmtxCorner11:
         reg->corners.c11 = point;
         break;
      case DmtxCorner01:
         reg->corners.c01 = point;
         break;
   }

   reg->corners.known |= cornerLoc;
}

/**
 *    T |
 *      |
 *      |
 *      +-----
 *    O      R
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionAlignSecondEdge(DmtxImage *image, DmtxRegion *reg)
{
   DmtxVector2 p0[4], p1[4], pCorner[4];
   int hitCount[4] = { 0, 0, 0, 0 };
   int weakCount[4] = { 0, 0, 0, 0 };
   DmtxMatrix3 xlate, flip, shear;
   DmtxMatrix3 preFit2Raw, postRaw2Fit;
   DmtxVector2 O, T, R;
   DmtxVector2 fitO, fitT;
   DmtxVector2 oldRawO, oldRawT;
   int i, bestFit;
   DmtxRay2 rayOT, rayNew;
   double ratio, maxRatio;

/*fprintf(stdout, "MatrixRegionAlignSecondEdge()\n"); */

   /* Scan top edge left-to-right (shear only)

      +---   O = intersection of known edges
      |      T = farthest known point along scanned edge
      |      R = old O
   */
   dmtxMatrix3Shear(postRaw2Fit, 0.0, 1.0);
   dmtxMatrix3Shear(preFit2Raw, 0.0, -1.0);

   hitCount[0] = MatrixRegionAlignEdge(image, reg, postRaw2Fit,
         preFit2Raw, &p0[0], &p1[0], &pCorner[0], &weakCount[0]);

   /* Scan top edge right-to-left (horizontal flip and shear)

      ---+   O = intersection of known edges
         |   T = old O
         |   R = farthest known point along scanned edge
   */
   dmtxMatrix3Scale(flip, -1.0, 1.0);
   dmtxMatrix3Shear(shear, 0.0, 1.0);
   dmtxMatrix3Multiply(postRaw2Fit, flip, shear);

   dmtxMatrix3Shear(shear, 0.0, -1.0);
   dmtxMatrix3Scale(flip, -1.0, 1.0);
   dmtxMatrix3Multiply(preFit2Raw, shear, flip);

   hitCount[1] = MatrixRegionAlignEdge(image, reg, postRaw2Fit,
         preFit2Raw, &p0[1], &p1[1], &pCorner[1], &weakCount[1]);

   /* Scan bottom edge left-to-right (vertical flip and shear)

      |      O = intersection of known edges
      |      T = old T
      +---   R = farthest known point along scanned edge
   */
   dmtxMatrix3Scale(flip, 1.0, -1.0);
   dmtxMatrix3Translate(xlate, 0.0, 1.0);
   dmtxMatrix3Shear(shear, 0.0, 1.0);
   dmtxMatrix3Multiply(postRaw2Fit, flip, xlate);
   dmtxMatrix3MultiplyBy(postRaw2Fit, shear);

   dmtxMatrix3Shear(shear, 0.0, -1.0);
   dmtxMatrix3Translate(xlate, 0.0, -1.0);
   dmtxMatrix3Scale(flip, 1.0, -1.0);
   dmtxMatrix3Multiply(preFit2Raw, shear, xlate);
   dmtxMatrix3MultiplyBy(preFit2Raw, flip);

   hitCount[2] = MatrixRegionAlignEdge(image, reg, postRaw2Fit,
         preFit2Raw, &p0[2], &p1[2], &pCorner[2], &weakCount[2]);

   /* Scan bottom edge right-to-left (flip flip shear)

         |   O = intersection of known edges
         |   T = farthest known point along scanned edge
      ---+   R = old T
   */
   dmtxMatrix3Scale(flip, -1.0, -1.0);
   dmtxMatrix3Translate(xlate, 0.0, 1.0);
   dmtxMatrix3Shear(shear, 0.0, 1.0);
   dmtxMatrix3Multiply(postRaw2Fit, flip, xlate);
   dmtxMatrix3MultiplyBy(postRaw2Fit, shear);

   dmtxMatrix3Shear(shear, 0.0, -1.0);
   dmtxMatrix3Translate(xlate, 0.0, -1.0);
   dmtxMatrix3Scale(flip, -1.0, -1.0);
   dmtxMatrix3Multiply(preFit2Raw, shear, xlate);
   dmtxMatrix3MultiplyBy(preFit2Raw, flip);

   hitCount[3] = MatrixRegionAlignEdge(image, reg, postRaw2Fit,
         preFit2Raw, &p0[3], &p1[3], &pCorner[3], &weakCount[3]);

/**
Simplify StepAlongEdge() and avoid complicated error-prone counters.
Instead just find all 4 edges, and then prune out edges according to
early criteria:

   a) Exited based on step limit without touching anything (p0-p1
      length is <8 pixels)

   b) If length (p0-p1) of top is <1/8 of bottom (or vice versa)

For those that remain, choose the winner based on least variation
among module samples and proximity of color to initial edge

   for(each edge orientation option) {
      if(hitCount[i] < 3)
         option[i] = invalid;
      else if(length[p1-p0] < 8 pixels)
         option[i] = invalid;
   }

   if(option[0].valid && option[2].valid) {
      ratio = length[0]/length[2];
      if(ratio > 8)
         option[2] = invalid;
      else if(ration < 1/8)
         option[0] = invalid;
   }
   // XXX then also same thing for option[0] and option[1]

   // Determine winner among remaining options
   winner = NULL;
   currentMin = max or first;
   for(each sizeIdx) {
      for(each edge orientation option) {
         if(edge orientation is ruled out)
            continue;

         if(ColorDevianceSum(sizeIdx, matrix, gradient, &currentMin))
            winner = thisOrientation;
      }
   }
   if(winner == NULL)
      return FAILURE;

     - step along imaginary center (same # steps for each test),
       summing color difference between sample and ON color in known
       gradient. As soon as sum exceeds previous best, then eliminate
       from candidacy. record best minimum difference for each leg
       candidate. candidate with smallest diff wins.

     - maybe round-robin the tests, so the winning leg will get a foot
       in the door sooner, speeding things up significantly
*/

   /* choose orientation with highest hitCount/(weakCount + 1) ratio */
   for(i = 0; i < 4; i++) {
      ratio = (double)hitCount[i]/(weakCount[i] + 1);

      if(i == 0 || ratio > maxRatio) {
         bestFit = i;
         maxRatio = ratio;
      }
   }

   if(hitCount[bestFit] < 5)
      return DMTX_FAILURE;

   fitT.X = 0.0;
   fitT.Y = 1.0;
   dmtxMatrix3VMultiply(&oldRawT, &fitT, reg->fit2raw);

   fitO.X = 0.0;
   fitO.Y = 0.0;
   dmtxMatrix3VMultiply(&oldRawO, &fitO, reg->fit2raw);

   if(bestFit == 0 || bestFit == 1)
      oldRawT = pCorner[bestFit];
   else
      oldRawO = pCorner[bestFit];

   rayOT.p = oldRawO;
   dmtxVector2Sub(&rayOT.v, &oldRawT, &oldRawO);
   if(dmtxVector2Norm(&rayOT.v) != DMTX_SUCCESS)
      return DMTX_FAILURE;

   rayNew.p = p0[bestFit];
   dmtxVector2Sub(&rayNew.v, &p1[bestFit], &p0[bestFit]);
   if(dmtxVector2Norm(&rayNew.v) != DMTX_SUCCESS)
      return DMTX_FAILURE;

   /* New origin is always origin of both known edges */
   dmtxRay2Intersect(&O, &rayOT, &rayNew);

   if(bestFit == 0) {
      T = p1[bestFit];
      R = oldRawO;
   }
   else if(bestFit == 1) {
      T = oldRawO;
      R = p1[bestFit];
   }
   else if(bestFit == 2) {
      T = oldRawT;
      R = p1[bestFit];
   }
   else {
      assert(bestFit == 3);
      T = p1[bestFit];
      R = oldRawT;
   }

   SetCornerLoc(reg, DmtxCorner00, O);
   SetCornerLoc(reg, DmtxCorner01, T);
   SetCornerLoc(reg, DmtxCorner10, R);

   if(MatrixRegionUpdateXfrms(image, reg) != DMTX_SUCCESS)
      return DMTX_FAILURE;

   /* Skewed barcode in the bottom middle pane */
/* CALLBACK_DECODE_FUNC1(buildMatrixCallback4, dec, reg->fit2raw); */

   return DMTX_SUCCESS;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionAlignRightEdge(DmtxImage *image, DmtxRegion *reg)
{
   DmtxMatrix3 rotate, flip, shear, scale;
   DmtxMatrix3 preFit2Raw, postRaw2Fit;

   dmtxMatrix3Rotate(rotate, -M_PI_2);
   dmtxMatrix3Scale(flip, 1.0, -1.0);
   dmtxMatrix3Shear(shear, 0.0, 0.5);
   dmtxMatrix3Scale(scale, 1.25, 1.0);

   dmtxMatrix3Multiply(postRaw2Fit, rotate, flip);
   dmtxMatrix3MultiplyBy(postRaw2Fit, shear);
   dmtxMatrix3MultiplyBy(postRaw2Fit, scale);

   dmtxMatrix3Scale(scale, 0.8, 1.0);
   dmtxMatrix3Shear(shear, 0.0, -0.5);
   dmtxMatrix3Scale(flip, 1.0, -1.0);
   dmtxMatrix3Rotate(rotate, M_PI_2);
   dmtxMatrix3Multiply(preFit2Raw, scale, shear);
   dmtxMatrix3MultiplyBy(preFit2Raw, flip);
   dmtxMatrix3MultiplyBy(preFit2Raw, rotate);

   if(MatrixRegionAlignCalibEdge(image, reg, DmtxEdgeRight, preFit2Raw, postRaw2Fit) != DMTX_SUCCESS)
      return DMTX_FAILURE;

   return DMTX_SUCCESS;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionAlignTopEdge(DmtxImage *image, DmtxRegion *reg)
{
   DmtxMatrix3 preFit2Raw, postRaw2Fit;

   dmtxMatrix3Shear(postRaw2Fit, 0.0, 0.5);
   dmtxMatrix3Shear(preFit2Raw, 0.0, -0.5);

   if(MatrixRegionAlignCalibEdge(image, reg, DmtxEdgeTop, preFit2Raw, postRaw2Fit) != DMTX_SUCCESS)
      return DMTX_FAILURE;

   return DMTX_SUCCESS;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionAlignCalibEdge(DmtxImage *image, DmtxRegion *reg,
      DmtxEdgeLoc edgeLoc, DmtxMatrix3 preFit2Raw, DmtxMatrix3 postRaw2Fit)
{
   DmtxVector2 p0, p1, pCorner;
   double slope;
   int hitCount;
   int weakCount;

   assert(edgeLoc == DmtxEdgeTop || edgeLoc == DmtxEdgeRight);

   hitCount = MatrixRegionAlignEdge(image, reg, postRaw2Fit, preFit2Raw, &p0, &p1, &pCorner, &weakCount);
   if(hitCount < 2)
      return DMTX_FAILURE;

   if(edgeLoc == DmtxEdgeRight) {
      SetCornerLoc(reg, DmtxCorner10, pCorner);
      if(MatrixRegionUpdateXfrms(image, reg) != DMTX_SUCCESS)
         return DMTX_FAILURE;

      dmtxMatrix3VMultiplyBy(&p0, reg->raw2fit);
      dmtxMatrix3VMultiplyBy(&p1, reg->raw2fit);

      assert(fabs(p1.Y - p0.Y) > DMTX_ALMOST_ZERO);
      slope = (p1.X - p0.X) / (p1.Y - p0.Y);

      p0.X = p0.X - slope * p0.Y;
      p0.Y = 0.0;
      p1.X = p0.X + slope;
      p1.Y = 1.0;

      dmtxMatrix3VMultiplyBy(&p0, reg->fit2raw);
      dmtxMatrix3VMultiplyBy(&p1, reg->fit2raw);

      SetCornerLoc(reg, DmtxCorner10, p0);
      SetCornerLoc(reg, DmtxCorner11, p1);
   }
   else {
      SetCornerLoc(reg, DmtxCorner01, pCorner);
      if(MatrixRegionUpdateXfrms(image, reg) != DMTX_SUCCESS)
         return DMTX_FAILURE;

      dmtxMatrix3VMultiplyBy(&p0, reg->raw2fit);
      dmtxMatrix3VMultiplyBy(&p1, reg->raw2fit);

      assert(fabs(p1.X - p0.X) > DMTX_ALMOST_ZERO);
      slope = (p1.Y - p0.Y) / (p1.X - p0.X);

      p0.Y = p0.Y - slope * p0.X;
      p0.X = 0.0;
      p1.Y = p0.Y + slope;
      p1.X = 1.0;

      dmtxMatrix3VMultiplyBy(&p0, reg->fit2raw);
      dmtxMatrix3VMultiplyBy(&p1, reg->fit2raw);

      SetCornerLoc(reg, DmtxCorner01, p0);
      SetCornerLoc(reg, DmtxCorner11, p1);
   }
   if(MatrixRegionUpdateXfrms(image, reg) != DMTX_SUCCESS)
      return DMTX_FAILURE;

   return DMTX_SUCCESS;
}

/**
 * XXX returns 3 points in raw coordinates
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionAlignEdge(DmtxImage *image, DmtxRegion *reg,
      DmtxMatrix3 postRaw2Fit, DmtxMatrix3 preFit2Raw, DmtxVector2 *p0,
      DmtxVector2 *p1, DmtxVector2 *pCorner, int *weakCount)
{
   int hitCount, edgeHit, prevEdgeHit;
   DmtxVector2 c00, c10, c01;
   DmtxMatrix3 sRaw2Fit, sFit2Raw;
   DmtxVector2 forward, lateral;
   DmtxVector2 pFitExact, pFitProgress, pRawProgress, pRawExact, pLast;
   double interceptTest, intercept[8];
   DmtxVector2 adjust[8];
   double slope[8];
   int i;
   int stepsSinceStarAdjust;
   DmtxVector2 pTmp;

/*fprintf(stdout, "MatrixRegionAlignEdge()\n"); */
   dmtxMatrix3Multiply(sRaw2Fit, reg->raw2fit, postRaw2Fit);
   dmtxMatrix3Multiply(sFit2Raw, preFit2Raw, reg->fit2raw);

   /* Draw skewed image in bottom left pane */
/* CALLBACK_DECODE_FUNC1(buildMatrixCallback3, dec, sFit2Raw); */

   /* Set starting point */
   pFitProgress.X = -0.003;
   pFitProgress.Y = 0.9;
   *pCorner = pFitExact = pFitProgress;
   dmtxMatrix3VMultiply(&pRawProgress, &pFitProgress, sFit2Raw);

   /* Initialize star lines */
   for(i = 0; i < 8; i++) {
      slope[i] = tan((M_PI_2/8.0) * i);
      intercept[i] = 0.9;
      adjust[i].X = 0.0;
      adjust[i].Y = 0.9;
   }

   hitCount = 0;
   stepsSinceStarAdjust = 0;
   *weakCount = 0;

   prevEdgeHit = edgeHit = DMTX_EDGE_STEP_EXACT;

   for(;;) {
      /* XXX technically we don't need to recalculate lateral & forward once we have left the finder bar */
      dmtxMatrix3VMultiply(&pTmp, &pRawProgress, sRaw2Fit);

      /* XXX move this outside of this loop ? */
      c00 = pTmp;
      c10.X = c00.X + 1;
      c10.Y = c00.Y;
      c01.X = c00.X - 0.087155743;
      c01.Y = c00.Y + 0.996194698;

      if(dmtxMatrix3VMultiplyBy(&c00, sFit2Raw) != DMTX_SUCCESS)
         return 0;

      if(dmtxMatrix3VMultiplyBy(&c10, sFit2Raw) != DMTX_SUCCESS)
         return 0;

      if(dmtxMatrix3VMultiplyBy(&c01, sFit2Raw) != DMTX_SUCCESS)
         return 0;

      if(RightAngleTrueness(c01, c00, c10, M_PI) < 0.1) {
         /* XXX instead of just failing here, hopefully find what happened
                upstream to trigger this condition. we can probably avoid
                this earlier on, and even avoid assertion failures elsewhere */
         return 0;
      }

      /* Calculate forward and lateral directions in raw coordinates */
      dmtxVector2Sub(&forward, &c10, &c00);
      if(dmtxVector2Norm(&forward) != DMTX_SUCCESS)
         return 0;

      dmtxVector2Sub(&lateral, &c01, &c00);
      if(dmtxVector2Norm(&lateral) != DMTX_SUCCESS)
         return 0;

      prevEdgeHit = edgeHit;
      edgeHit = StepAlongEdge(image, reg, &pRawProgress, &pRawExact, forward, lateral);
      dmtxMatrix3VMultiply(&pFitProgress, &pRawProgress, sRaw2Fit);

      if(edgeHit == DMTX_EDGE_STEP_EXACT) {
/**
         if(prevEdgeHit == DMTX_EDGE_STEP_TOO_WEAK) <-- XXX REVIST LATER ... doesn't work
            hitCount++;
*/
         hitCount++;

         dmtxMatrix3VMultiply(&pFitExact, &pRawExact, sRaw2Fit);

         /* Adjust star lines upward (non-vertical) */
         for(i = 0; i < 8; i++) {
            interceptTest = pFitExact.Y - slope[i] * pFitExact.X;
            if(interceptTest > intercept[i]) {
               intercept[i] = interceptTest;
               adjust[i] = pFitExact;
               stepsSinceStarAdjust = 0;

               /* XXX still "turning corner" but not as bad anymore */
               if(i == 7) {
                  *pCorner = pFitExact;
/*                CALLBACK_DECODE_FUNC4(plotPointCallback, dec, pRawExact, 1, 1, DMTX_DISPLAY_POINT); */
               }

               if(i == 0) {
                  pLast = pFitExact;
/*                CALLBACK_DECODE_FUNC4(plotPointCallback, dec, pRawExact, 1, 1, DMTX_DISPLAY_POINT); */
               }
            }
         }

         /* Draw edge hits along skewed edge in bottom left pane */
/*       CALLBACK_DECODE_FUNC4(xfrmPlotPointCallback, dec, pFitExact, NULL, 4, DMTX_DISPLAY_POINT); */
      }
      else if(edgeHit == DMTX_EDGE_STEP_TOO_WEAK) {
         stepsSinceStarAdjust++;
         if(prevEdgeHit == DMTX_EDGE_STEP_TOO_WEAK)
            (*weakCount)++;
      }

      /* XXX also change stepsSinceNear and use this in the break condition */
      if(hitCount >= 20 && stepsSinceStarAdjust > hitCount)
         break;

      if(pFitProgress.X > 1.0) {
/*       CALLBACK_DECODE_FUNC4(plotPointCallback, dec, pRawProgress, 1, 1, DMTX_DISPLAY_SQUARE); */
         break;
      }

      if(pRawProgress.X < 1 || pRawProgress.X > image->width - 1 ||
         pRawProgress.Y < 1 || pRawProgress.Y > image->height - 1)
         break;
   }

   /* Find lowest available horizontal starline adjustment */
   for(i = 0; i < 8; i++) {
      if(adjust[i].X < 0.1)
         break;
   }
   if(i == -1)
      return 0;

   *p0 = adjust[i];

   /* Find highest available non-horizontal starline adjustment */
   for(i = 7; i > 1; i--) {
      if(adjust[i].X > 0.8)
         break;
   }
   if(i == -1)
      return 0;

   *p1 = adjust[i];

   if(fabs(p0->X - p1->X) < 0.1 || p0->Y < 0.2 || p1->Y < 0.2) {
      return 0;
   }

   dmtxMatrix3VMultiplyBy(pCorner, sFit2Raw);
   dmtxMatrix3VMultiplyBy(p0, sFit2Raw);
   dmtxMatrix3VMultiplyBy(p1, sFit2Raw);

   return hitCount;
}

/**
 * XXX receives and returns 2 points in non-skewed raw pixel coordinates
 *
 * @param
 * @return XXX
 */
static int
StepAlongEdge(DmtxImage *image, DmtxRegion *reg, DmtxVector2 *pProgress,
      DmtxVector2 *pExact, DmtxVector2 forward, DmtxVector2 lateral)
{
   int x, y;
   int xToward, yToward;
   double frac, accelPrev, accelNext;
   DmtxCompassEdge compass, compassPrev, compassNext;
   DmtxVector2 vTmp;

/*fprintf(stdout, "StepAlongEdge()\n"); */
   x = (int)(pProgress->X + 0.5);
   y = (int)(pProgress->Y + 0.5);

   *pExact = *pProgress;
   compass = GetCompassEdge(image, x, y, DMTX_ALL_COMPASS_DIRS);

   /* If pixel shows a weak edge in any direction then advance forward */
   if(compass.magnitude < 60) {
      dmtxVector2AddTo(pProgress, &forward);
/*    CALLBACK_POINT_PLOT(*pProgress, 1, 5, DMTX_DISPLAY_POINT); */
      return DMTX_EDGE_STEP_TOO_WEAK;
   }

   /* forward is toward edge */
   /* lateral is away from edge */

   /* Determine orthagonal step directions */
   if(compass.scanDir == DmtxCompassDir0) {
      yToward = 0;

      if(fabs(forward.X) > fabs(lateral.X))
         xToward = (forward.X > 0) ? 1 : -1;
      else
         xToward = (lateral.X > 0) ? -1 : 1;
   }
   else {
      xToward = 0;

      if(fabs(forward.Y) > fabs(lateral.Y))
         yToward = (forward.Y > 0) ? 1 : -1;
      else
         yToward = (lateral.Y > 0) ? -1 : 1;
   }

   /* Pixel shows edge in perpendicular direction */
   compassPrev = GetCompassEdge(image, x-xToward, y-yToward, compass.edgeDir);
   compassNext = GetCompassEdge(image, x+xToward, y+yToward, compass.edgeDir);

   accelPrev = compass.magnitude - compassPrev.magnitude;
   accelNext = compassNext.magnitude - compass.magnitude;

   /* If we found a strong edge then calculate the zero crossing */
   /* XXX explore expanding this test to allow a little more fudge (without
      screwing up edge placement later) */
   if(accelPrev * accelNext < DMTX_ALMOST_ZERO) {

      dmtxVector2AddTo(pProgress, &lateral);

      frac = (fabs(accelNext - accelPrev) > DMTX_ALMOST_ZERO) ?
            (accelPrev / (accelPrev - accelNext)) - 0.5 : 0.0;

      vTmp.X = xToward;
      vTmp.Y = yToward;
      dmtxVector2ScaleBy(&vTmp, frac);
      dmtxVector2AddTo(pExact, &vTmp);

/*    CALLBACK_POINT_PLOT(*pExact, 2, 1, DMTX_DISPLAY_POINT); */
      return DMTX_EDGE_STEP_EXACT;
   }

   /* Passed edge */
   if(compassPrev.magnitude > compass.magnitude) {
/*    CALLBACK_DECODE_FUNC4(plotPointCallback, dec, *pProgress, 3, 1, DMTX_DISPLAY_POINT); */
      dmtxVector2AddTo(pProgress, &lateral);
      return DMTX_EDGE_STEP_TOO_FAR;
   }

   /* Approaching edge but not there yet */
/* CALLBACK_DECODE_FUNC4(plotPointCallback, dec, *pProgress, 4, 1, DMTX_DISPLAY_POINT); */
   dmtxVector2AddTo(pProgress, &forward);
   return DMTX_EDGE_STEP_NOT_QUITE;
}

/**
 * XXX new experimental version that operates on sizeIdx instead of region
 *
 * @param
 * @return XXX
 */
static DmtxColor3
ReadModuleColor(DmtxImage *image, DmtxRegion *reg, int symbolRow, int symbolCol, int sizeIdx)
{
   int i;
   int symbolRows, symbolCols;
   double sampleX[] = { 0.5, 0.4, 0.5, 0.6, 0.5 };
   double sampleY[] = { 0.5, 0.5, 0.4, 0.5, 0.6 };
   DmtxVector2 p, p0;
   DmtxColor3 cPoint, cAverage;

   cAverage.R = cAverage.G = cAverage.B = 0.0;

   symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, sizeIdx);
   symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, sizeIdx);

   for(i = 0; i < 5; i++) {

      p.X = (1.0/symbolCols) * (symbolCol + sampleX[i]);
      p.Y = (1.0/symbolRows) * (symbolRow + sampleY[i]);

      dmtxMatrix3VMultiply(&p0, &p, reg->fit2raw);
      dmtxColor3FromImage2(&cPoint, image, p0);
/*    dmtxColor3FromImage(&cPoint, image, p0.X, p0.Y); */

      dmtxColor3AddTo(&cAverage, &cPoint);

/*    CALLBACK_DECODE_FUNC4(plotPointCallback, dec, p0, 1, 1, DMTX_DISPLAY_POINT); */
   }
   dmtxColor3ScaleBy(&cAverage, 0.2);

   return cAverage;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionFindSize(DmtxImage *image, DmtxRegion *reg)
{
   int sizeIdx;
   int errors[30] = { 0 };
   int minErrorsSizeIdx;
   int row, col, symbolRows, symbolCols;
   double tOff, tOn, jumpThreshold;
   DmtxColor3 colorOn, colorOff;
   DmtxColor3 colorOnAvg, colorOffAvg;
   DmtxColor3 black = { 0.0, 0.0, 0.0 };
   DmtxGradient gradient, testGradient;
   int sizeIdxAttempts[] = { 23, 22, 21, 20, 19, 18, 17, 16, 15, 14,
                             13, 29, 12, 11, 10, 28, 27,  9, 25,  8,
                             26,  7,  6,  5,  4, 24,  3,  2,  1,  0 };
   int *ptr;

   /* First try all sizes to determine which sizeIdx with best contrast */
   ptr = sizeIdxAttempts;
   gradient.tMin = gradient.tMid = gradient.tMax = 0;
   do {
      sizeIdx = *ptr;
      symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, sizeIdx);
      symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, sizeIdx);

      colorOnAvg = colorOffAvg = black;

      for(row = 0, col = 0; col < symbolCols; col++) {
         colorOn = ReadModuleColor(image, reg, row, col, sizeIdx);
         colorOff = ReadModuleColor(image, reg, row-1, col, sizeIdx);
         dmtxColor3AddTo(&colorOnAvg, &colorOn);
         dmtxColor3AddTo(&colorOffAvg, &colorOff);
      }

      for(row = 0, col = 0; row < symbolRows; row++) {
         colorOn = ReadModuleColor(image, reg, row, col, sizeIdx);
         colorOff = ReadModuleColor(image, reg, row, col-1, sizeIdx);
         dmtxColor3AddTo(&colorOnAvg, &colorOn);
         dmtxColor3AddTo(&colorOffAvg, &colorOff);
      }

      dmtxColor3ScaleBy(&colorOnAvg, 1.0/(symbolRows + symbolCols));
      dmtxColor3ScaleBy(&colorOffAvg, 1.0/(symbolRows + symbolCols));

      testGradient.ray.p = colorOffAvg;
      dmtxColor3Sub(&testGradient.ray.c, &colorOnAvg, &colorOffAvg);
      if(dmtxColor3Mag(&testGradient.ray.c) < 20)
         continue;

      dmtxColor3Norm(&testGradient.ray.c);
      testGradient.tMin = 0;
      testGradient.tMax = dmtxDistanceAlongRay3(&testGradient.ray, &colorOnAvg);
      testGradient.tMid = (testGradient.tMin + testGradient.tMax) / 2.0;

      if(testGradient.tMax > gradient.tMax)
         gradient = testGradient;
   } while(*(ptr++) != 0);

   jumpThreshold = 0.4 * (gradient.tMax - gradient.tMin);
   if(jumpThreshold < 20)
      return DMTX_FAILURE;

   /* Start with largest possible pattern size and work downward.  If done
      in other direction then false positive is possible. */

   ptr = sizeIdxAttempts;
   minErrorsSizeIdx = *ptr;
   do {
      sizeIdx = *ptr;
      symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, sizeIdx);
      symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, sizeIdx);

      /* Test each pair of ON/OFF modules in the calibration bars */

      /* Top calibration row */
      row = symbolRows - 1;
      for(col = 0; col < symbolCols; col += 2) {
         colorOff = ReadModuleColor(image, reg, row, col + 1, sizeIdx);
         tOff = dmtxDistanceAlongRay3(&gradient.ray, &colorOff);
         colorOn = ReadModuleColor(image, reg, row, col, sizeIdx);
         tOn = dmtxDistanceAlongRay3(&gradient.ray, &colorOn);

         if(tOn - tOff < jumpThreshold)
            errors[sizeIdx]++;

         if(errors[sizeIdx] > errors[minErrorsSizeIdx])
            break;
      }

      /* Right calibration column */
      col = symbolCols - 1;
      for(row = 0; row < symbolRows; row += 2) {
         colorOff = ReadModuleColor(image, reg, row + 1, col, sizeIdx);
         tOff = dmtxDistanceAlongRay3(&gradient.ray, &colorOff);
         colorOn = ReadModuleColor(image, reg, row, col, sizeIdx);
         tOn = dmtxDistanceAlongRay3(&gradient.ray, &colorOn);

         if(tOn - tOff < jumpThreshold)
            errors[sizeIdx]++;

         if(errors[sizeIdx] > errors[minErrorsSizeIdx])
            break;
      }

      /* Track of which sizeIdx has the fewest errors */
      if(errors[sizeIdx] < errors[minErrorsSizeIdx])
         minErrorsSizeIdx = sizeIdx;

   } while(*(ptr++) != 0);

   reg->gradient = gradient;
   reg->sizeIdx = minErrorsSizeIdx;

   reg->symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, reg->sizeIdx);
   reg->symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, reg->sizeIdx);
   reg->mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, reg->sizeIdx);
   reg->mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, reg->sizeIdx);

   if(errors[minErrorsSizeIdx] >= 4)
      return DMTX_FAILURE;

   return DMTX_SUCCESS;
}
