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

#define DMTX_HOUGH_RES 32

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
/* int size, i = 0;
   char imagePath[128]; */

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
      reg = dmtxScanPixel(dec, loc);
/**
      if(reg.found == DMTX_REGION_FOUND || reg.found > DMTX_REGION_DROPPED_2ND) {
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
dmtxScanPixel(DmtxDecode *dec, DmtxPixelLoc loc)
{
   DmtxEdgeSubPixel edgeStart;
   DmtxRay2 ray0, ray1;
   DmtxRegion reg;
   int hough[DMTX_HOUGH_RES] = { 0 };
   int houghStrong;

   memset(&reg, 0x00, sizeof(DmtxRegion));

   /* Assume region is not found unless scan finds one below */
   reg.found = DMTX_REGION_NOT_FOUND;

   if(dmtxImageContainsInt(dec->image, 0, loc.X, loc.Y) == DMTX_FALSE)
      return reg;

   /* Find subpixel location of strongest edge found at this location */
   edgeStart = FindZeroCrossing(dec, loc.X, loc.Y, NULL);
   if(!edgeStart.isEdge) {
      reg.found = DMTX_REGION_DROPPED_EDGE;
      return reg;
   }

   /* Next follow the edge to its end in both directions */
   houghStrong = 0;
   ray0 = FollowEdge(dec, loc.X, loc.Y, edgeStart, 1, hough, &houghStrong);
   ray1 = FollowEdge(dec, loc.X, loc.Y, edgeStart, -1, hough, &houghStrong);
   if(hough[houghStrong] < 8) {
      reg.found = DMTX_REGION_DROPPED_1ST;
      return reg;
   }

   /* Define first edge based on travel limits of detected edge */
   if(MatrixRegionAlignFirstEdge(dec, &reg, &edgeStart, ray0, ray1) != DMTX_SUCCESS) {
      reg.found = DMTX_REGION_DROPPED_1ST;
      return reg;
   }

   /* Define second edge based on best match of 4 possible orientations */
   if(MatrixRegionAlignSecondEdge(dec, &reg) != DMTX_SUCCESS) {
      reg.found = DMTX_REGION_DROPPED_2ND;
      return reg;
   }

   /* Define right edge */
   if(MatrixRegionAlignRightEdge(dec, &reg) != DMTX_SUCCESS) {
      reg.found = DMTX_REGION_DROPPED_RIGHT;
      return reg;
   }

   /* Define top edge */
   if(MatrixRegionAlignTopEdge(dec, &reg) != DMTX_SUCCESS) {
      reg.found = DMTX_REGION_DROPPED_TOP;
      return reg;
   }

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
 * @brief Clamp integer to range min <= value <= max
 * @param value Value to be clamped
 * @param min Minimum range boundary
 * @param max Maximum range boundary
 * @return Clamped value
 */
static int
ClampIntRange(int value, int min, int max)
{
   if(value < min)
      return min;

   if(value > max)
      return max;

   return value;
}

/**
 * @brief Calculate compass edge strength and direction at individual pixel location
 * @param img
 * @param x
 * @param y
 * @param edgeScanDirs
 * @return Compass edge
 */
static DmtxCompassEdge
GetCompassEdge(DmtxImage *img, int x, int y, int edgeScanDirs)
{
   static const int coefficient[] = {  0,  1,  2,  1,  0, -1, -2, -1 };
   static const int patternX[] =    { -1,  0,  1,  1,  1,  0, -1, -1 };
   static const int patternY[] =    { -1, -1, -1,  0,  1,  1,  1,  0 };
   int idx, dirVal[] = { DmtxCompassDirNeg45, DmtxCompassDir0, DmtxCompassDir45, DmtxCompassDir90 };

   int offset, widthScaled, heightScaled;
   int patternIdx, coefficientIdx;
   int xAdjust, yAdjust;
   int rgbCol[3], rgbInt[8][3], *rgbIntPtr;
   int dir, maxDirOrtho, maxDirAll;
   long magSq, maxMagSqOrtho, maxMagSqAll;
   int maxColOrtho[3], maxColAll[3];
   DmtxRgb rgb;
   DmtxCompassEdge maxEdge, *compassCache;
   DmtxColor3 black = { 0.0, 0.0, 0.0 }; /* XXX move black to a global scope later */

   /* Set maxEdge to invalid state */
   maxEdge.dirsTested = DmtxCompassDirNone;
   maxEdge.maxDirAll = maxDirAll = DmtxCompassDirNone;
   maxEdge.maxDirOrtho = maxDirOrtho = DmtxCompassDirNone;
   maxEdge.magnitude = maxMagSqAll = maxMagSqOrtho = 0.0;
   maxEdge.intensity = black;
   memset(maxColAll, 0x00, 3 * sizeof(int));
   memset(maxColOrtho, 0x00, 3 * sizeof(int));

   if(dmtxImageContainsInt(img, 1, x, y) == DMTX_FALSE)
      return maxEdge; /* XXX should really communicate failure with a dedicated value instead */

   widthScaled = img->width/img->scale;   /* dmtxImageGetProp(img, DmtxPropScaledWidth); */
   heightScaled = img->height/img->scale; /* dmtxImageGetProp(img, DmtxPropScaledHeight); */

   /* Cache always holds result from most recent test. Return cached result
      if it matches the request. Otherwise recache and return new result. */

   offset = dmtxImageGetOffset(img, x, y);
   compassCache = &(img->compass[offset]);

   /* Requested test was already performed */
   if(edgeScanDirs == compassCache->dirsTested)
      return *compassCache;

   /* Requested answer is already cached */
   if(edgeScanDirs == compassCache->maxDirAll)
      return *compassCache;

   /* Previously found max ortho by virtue of max all test */
   if(edgeScanDirs == DmtxCompassDirOrtho &&
         compassCache->dirsTested == DmtxCompassDirAll &&
         (compassCache->maxDirAll & DmtxCompassDirOrtho))
      return *compassCache;

   for(patternIdx = 0; patternIdx < 8; patternIdx++) {
      /* Accommodate 1 pixel beyond edge of image with nearest neighbor value */
      xAdjust = ClampIntRange(x + patternX[patternIdx], 0, widthScaled - 1);
      yAdjust = ClampIntRange(y + patternY[patternIdx], 0, heightScaled - 1);

      dmtxImageGetRgb(img, xAdjust, yAdjust, rgb);
      rgbIntPtr = &(rgbInt[patternIdx][0]);
      rgbIntPtr[0] = rgb[0];
      rgbIntPtr[1] = rgb[1];
      rgbIntPtr[2] = rgb[2];
   }

   /* Calculate this pixel's edge intensity for each direction (-45, 0, 45, 90) */
   for(idx = 0; idx < 4; idx++) {

      dir = dirVal[idx];

      /* Only scan for edge if this direction was requested */
      if(!(dir & edgeScanDirs))
         continue;

      memset(rgbCol, 0x00, 3 * sizeof(int));

      /* Add portion from each position in the convolution matrix pattern */
      for(patternIdx = 0; patternIdx < 8; patternIdx++) {

         coefficientIdx = (patternIdx - idx + 8) % 8;
         if(coefficient[coefficientIdx] == 0)
            continue;

         rgbIntPtr = &(rgbInt[patternIdx][0]);

         /* Weight pixel value by appropriate coefficient in convolution matrix */
         switch(coefficient[coefficientIdx]) {
            case 2:
               rgbCol[0] += rgbIntPtr[0];
               rgbCol[1] += rgbIntPtr[1];
               rgbCol[2] += rgbIntPtr[2];
               /* Fall through */
            case 1:
               rgbCol[0] += rgbIntPtr[0];
               rgbCol[1] += rgbIntPtr[1];
               rgbCol[2] += rgbIntPtr[2];
               break;
            case -2:
               rgbCol[0] -= rgbIntPtr[0];
               rgbCol[1] -= rgbIntPtr[1];
               rgbCol[2] -= rgbIntPtr[2];
               /* Fall through */
            case -1:
               rgbCol[0] -= rgbIntPtr[0];
               rgbCol[1] -= rgbIntPtr[1];
               rgbCol[2] -= rgbIntPtr[2];
               break;
         }
      }
      magSq = rgbCol[0] * rgbCol[0] + rgbCol[1] * rgbCol[1] + rgbCol[2] * rgbCol[2];

      /* Capture the strongest edge direction and its intensity */
      if((dir & DmtxCompassDirOrtho) &&
            (maxDirOrtho == DmtxCompassDirNone || magSq > maxMagSqOrtho)) {
         maxDirOrtho = dir;
         maxMagSqOrtho = magSq;
         memcpy(maxColOrtho, rgbCol, 3 * sizeof(int));
      }

      /* Capture the strongest edge direction and its intensity */
      if(maxDirAll == DmtxCompassDirNone || magSq > maxMagSqAll) {
         maxDirAll = dir;
         maxMagSqAll = magSq;
         memcpy(maxColAll, rgbCol, 3 * sizeof(int));
      }
   }

   maxEdge.dirsTested = edgeScanDirs;
   maxEdge.maxDirAll = maxDirAll;
   maxEdge.maxDirOrtho = maxDirOrtho;

   maxEdge.intensity.R = maxColAll[0];
   maxEdge.intensity.G = maxColAll[1];
   maxEdge.intensity.B = maxColAll[2];
   maxEdge.magnitude = dmtxColor3Norm(&maxEdge.intensity);

   *compassCache = maxEdge;

   return *compassCache;
}

/**
 * We have found an edge candidate whenever we find 3 pixels in a
 * row that share the same strongest direction and the middle one
 * had the highest intensity (or a first-place tie with one of its
 * neighbors)
 */
static DmtxEdgeSubPixel
FindZeroCrossing(DmtxDecode *dec, int x, int y, DmtxCompassEdge *compare)
{
   double accelPrev, accelNext, frac;
   DmtxCompassEdge selfStart, compassPrev, compassNext;
   DmtxEdgeSubPixel subPixel;
   DmtxImage *img = dec->image;

   subPixel.isEdge = 0;
   subPixel.xInt = x;
   subPixel.yInt = y;
   subPixel.xFrac = 0.0;
   subPixel.yFrac = 0.0;

   if(compare == NULL) {
      compare = &selfStart;
      selfStart = GetCompassEdge(img, x, y, DmtxCompassDirAll);
      subPixel.compass = selfStart;
   }
   else {
      subPixel.compass = GetCompassEdge(img, x, y, compare->maxDirAll);
      if(dmtxColor3Dot(&subPixel.compass.intensity, &(compare->intensity)) < 0)
         return subPixel;
   }

   if(subPixel.compass.magnitude < dec->edgeThresh * 17.68)
      return subPixel;

   if(compare->maxDirOrtho == DmtxCompassDir0) {
      compassPrev = GetCompassEdge(img, x-1, y, compare->maxDirAll);
      compassNext = GetCompassEdge(img, x+1, y, compare->maxDirAll);
   }
   else { /* DmtxCompassDir90 */
      compassPrev = GetCompassEdge(img, x, y-1, compare->maxDirAll);
      compassNext = GetCompassEdge(img, x, y+1, compare->maxDirAll);
   }

   /* Calculate 2nd derivatives left and right of center */
   accelPrev = subPixel.compass.magnitude - compassPrev.magnitude;
   accelNext = compassNext.magnitude - subPixel.compass.magnitude;

   /* If it looks like an edge then interpolate subpixel loc based on 0 crossing */
   if(accelPrev * accelNext < DMTX_ALMOST_ZERO) {
      frac = (fabs(accelNext - accelPrev) > DMTX_ALMOST_ZERO) ?
            (accelPrev / (accelPrev - accelNext)) - 0.5 : 0.0;

      subPixel.isEdge = 1;
      subPixel.xFrac = (compare->maxDirOrtho == DmtxCompassDir0) ? frac : 0.0;
      subPixel.yFrac = (compare->maxDirOrtho == DmtxCompassDir90) ? frac : 0.0;
   }

   return subPixel;
}

/**
 *
 *
 */
static DmtxRay2
FollowEdge(DmtxDecode *dec, int x, int y, DmtxEdgeSubPixel edgeStart, int forward, int hough[], int *strongIdx)
{
   int xFollow, yFollow;
   int xIncrement, yIncrement;
   DmtxEdgeSubPixel edge, edge0, edge1;
   DmtxVector2 p, pStart, pSoft, pHard, pStep;
   double strong0, strong1;
   double angle;
   int angleIdx;
   DmtxCompassEdge compass;
   DmtxRay2 ray;

   memset(&ray, 0x00, sizeof(DmtxRay2));

   /* No edge here, thanks for playing */
   if(!edgeStart.isEdge)
      return ray;

   pStart.X = edgeStart.xInt + edgeStart.xFrac;
   pStart.Y = edgeStart.yInt + edgeStart.yFrac;
   p = pSoft = pHard = pStart;

   edge = edgeStart;
   compass = edgeStart.compass;

   /* If we have a true edge then continue to follow it forward */
   if(compass.maxDirOrtho == DmtxCompassDir0) {
      xIncrement = 0;
      yIncrement = forward;
   }
   else {
      xIncrement = forward;
      yIncrement = 0;
   }

   xFollow = x + xIncrement;
   yFollow = y + yIncrement;

   while(edge.isEdge && dmtxImageContainsInt(dec->image, 0, xFollow, yFollow) == DMTX_TRUE) {

      edge = FindZeroCrossing(dec, xFollow, yFollow, &compass);

      if(!edge.isEdge) {
         edge0 = FindZeroCrossing(dec, xFollow + yIncrement, yFollow + xIncrement, &compass);
         edge1 = FindZeroCrossing(dec, xFollow - yIncrement, yFollow - xIncrement, &compass);
         if(edge0.isEdge && edge1.isEdge) {
            strong0 = dmtxColor3Dot(&edge0.compass.intensity, &compass.intensity);
            strong1 = dmtxColor3Dot(&edge1.compass.intensity, &compass.intensity);
            edge = (strong0 > strong1) ? edge0 : edge1;
         }
         else {
            edge = (edge0.isEdge) ? edge0 : edge1;
         }

         if(!edge.isEdge) {
            edge0 = FindZeroCrossing(dec, xFollow + 2*yIncrement, yFollow + 2*xIncrement, &compass);
            edge1 = FindZeroCrossing(dec, xFollow - 2*yIncrement, yFollow - 2*xIncrement, &compass);
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

      angle = atan2(p.Y-pStart.Y, p.X-pStart.X) + M_PI;
      angleIdx = (int)(angle * (DMTX_HOUGH_RES/M_PI)) % DMTX_HOUGH_RES;

      hough[angleIdx]++;
      if(hough[angleIdx] > hough[*strongIdx])
         *strongIdx = angleIdx;

      if(hough[*strongIdx] > 8 && hough[angleIdx] < 3)
         break;

      xFollow = (int)(edge.xInt + edge.xFrac + 0.5) + xIncrement;
      yFollow = (int)(edge.yInt + edge.yFrac + 0.5) + yIncrement;
   }

/* CALLBACK_POINT_PLOT(pHard, 1, 4, DMTX_DISPLAY_SQUARE); */
   dmtxVector2Sub(&pStep, &pHard, &pStart);
   if(dmtxVector2Norm(&pStep) < 4.0)
      return ray;

   ray.p = pStart;
   ray.v = pStep;
   ray.tMin = 0;
   ray.tMax = dmtxDistanceAlongRay2(&ray, &pSoft);

   ray.isDefined = (ray.tMax > 4) ? 1 : 0;

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

   dmtxVector2Norm(dmtxVector2Sub(&vA, &c0, &c1));
   dmtxVector2Norm(dmtxVector2Sub(&vB, &c2, &c1));

   dmtxMatrix3Rotate(m, angle);
   dmtxMatrix3VMultiplyBy(&vB, m);

   return dmtxVector2Dot(&vA, &vB);
}

/**
 * @brief Update transformations based on known region attributes
 * @param image
 * @param reg
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
MatrixRegionUpdateXfrms(DmtxDecode *dec, DmtxRegion *reg)
{
   DmtxVector2 vOT, vOR, vTmp;
   double tx, ty, phi, shx, scx, scy, skx, sky;
   double dimOT, dimOR, dimTX, dimRX, ratio;
   DmtxMatrix3 m, mtxy, mphi, mshx, mscx, mscy, mscxy, msky, mskx, mTmp;
   DmtxCorners corners;

   assert((reg->corners.known & DmtxCorner00) && (reg->corners.known & DmtxCorner01));

   /* Make copy of known corners to update with temporary values */
   corners = reg->corners;

   if(dmtxImageContainsFloat(dec->image, corners.c00.X, corners.c00.Y) == DMTX_FALSE)
      return DMTX_FAILURE;

   if(dmtxImageContainsFloat(dec->image, corners.c01.X, corners.c01.Y) == DMTX_FALSE)
      return DMTX_FAILURE;

   dimOT = dmtxVector2Mag(dmtxVector2Sub(&vOT, &corners.c01, &corners.c00)); /* XXX could use MagSquared() */
   if(dimOT < 8)
      return DMTX_FAILURE;

   /* Bottom-right corner -- validate if known or create temporary value */
   if(corners.known & DmtxCorner10) {
      if(dmtxImageContainsFloat(dec->image, corners.c10.X, corners.c10.Y) == DMTX_FALSE)
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
   if(corners.known & DmtxCorner10)
      if(RightAngleTrueness(corners.c01, corners.c00, corners.c10, M_PI_2) < dec->squareDevn)
         return DMTX_FAILURE;

   /* Top-right corner -- validate if known or create temporary value */
   if(corners.known & DmtxCorner11) {
      if(dmtxImageContainsFloat(dec->image, corners.c11.X, corners.c11.Y) == DMTX_FALSE)
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
   if(corners.known & DmtxCorner11)
      if(RightAngleTrueness(corners.c11, corners.c01, corners.c00, M_PI_2) < dec->squareDevn)
         return DMTX_FAILURE;

   if((corners.known & DmtxCorner10) && (corners.known & DmtxCorner11)) {
      /* Test bottom-right corner for trueness */
      if(RightAngleTrueness(corners.c00, corners.c10, corners.c11, M_PI_2) < dec->squareDevn)
         return DMTX_FAILURE;
      /* Test top-right corner for trueness */
      if(RightAngleTrueness(corners.c10, corners.c11, corners.c01, M_PI_2) < dec->squareDevn)
         return DMTX_FAILURE;
   }

   /* Calculate values needed for transformations */
   tx = -1 * corners.c00.X;
   ty = -1 * corners.c00.Y;
   dmtxMatrix3Translate(mtxy, tx, ty);

   phi = atan2(vOT.X, vOT.Y);
   dmtxMatrix3Rotate(mphi, phi);
   dmtxMatrix3Multiply(m, mtxy, mphi);

   dmtxMatrix3VMultiply(&vTmp, &corners.c10, m);
   shx = -vTmp.Y / vTmp.X;
   dmtxMatrix3Shear(mshx, 0.0, shx);
   dmtxMatrix3MultiplyBy(m, mshx);

   scx = 1.0/vTmp.X;
   dmtxMatrix3Scale(mscx, scx, 1.0);
   dmtxMatrix3MultiplyBy(m, mscx);

   dmtxMatrix3VMultiply(&vTmp, &corners.c11, m);
   scy = 1.0/vTmp.Y;
   dmtxMatrix3Scale(mscy, 1.0, scy);
   dmtxMatrix3MultiplyBy(m, mscy);

   dmtxMatrix3VMultiply(&vTmp, &corners.c11, m);
   skx = vTmp.X;
   dmtxMatrix3LineSkewSide(mskx, 1.0, skx, 1.0);
   dmtxMatrix3MultiplyBy(m, mskx);

   dmtxMatrix3VMultiply(&vTmp, &corners.c01, m);
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
 * @brief Align first edge of potential barcode region
 * @param image
 * @param reg
 * @param edgeStart
 * @param ray0
 * @param ray1
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
MatrixRegionAlignFirstEdge(DmtxDecode *dec, DmtxRegion *reg, DmtxEdgeSubPixel *edgeStart, DmtxRay2 ray0, DmtxRay2 ray1)
{
   DmtxRay2 rayFull;
   DmtxVector2 p0, p1, pTmp;
   double ratio;

   if(!ray0.isDefined && !ray1.isDefined) {
      return DMTX_FAILURE;
   }
   else if(ray0.isDefined && ray1.isDefined) {

      /* If rays are relatively colinear then combine them */
      if(dmtxVector2Dot(&ray0.v, &ray1.v) < -0.99) {
         dmtxPointAlongRay2(&p0, &ray0, ray0.tMax);
         dmtxPointAlongRay2(&p1, &ray1, ray1.tMax);
         rayFull.isDefined = 1;
         rayFull.p = p1;

         dmtxVector2Sub(&pTmp, &p0, &p1);
         if(dmtxVector2Norm(&pTmp) < 0.0)
            return DMTX_FAILURE;

         rayFull.v = pTmp;
         rayFull.tMin = 0;
         rayFull.tMax = dmtxDistanceAlongRay2(&rayFull, &p0);
      }
      else { /* Not colinear */
         ratio = ray0.tMax / ray1.tMax;

         assert(ray1.tMax > DMTX_ALMOST_ZERO);

         /* Only use longer ray if shorter one is insignificant */
         if(ratio < 0.2 || ratio > 5)
            rayFull = (ray0.tMax > ray1.tMax) ? ray0 : ray1;
         else
            return DMTX_FAILURE;
      }
   }
   else {
      rayFull = (ray0.isDefined) ? ray0 : ray1;
   }

   /* Reject edges shorter than 12 pixels */
   if(rayFull.tMax < 12)
      return DMTX_FAILURE;

   dmtxPointAlongRay2(&pTmp, &rayFull, rayFull.tMax);
   SetCornerLoc(reg, DmtxCorner01, pTmp);
   SetCornerLoc(reg, DmtxCorner00, rayFull.p);

   if(MatrixRegionUpdateXfrms(dec, reg) != DMTX_SUCCESS)
      return DMTX_FAILURE;

   /* Top-right pane showing first edge fit */
/* CALLBACK_DECODE_FUNC1(buildMatrixCallback2, dec, reg); */

   return DMTX_SUCCESS;
}

/**
 * @brief XXX
 * @param reg
 * @param cornerLoc
 * @param point
 * @return void
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
 * @brief  XXX
 * @param  image
 * @param  reg
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
MatrixRegionAlignSecondEdge(DmtxDecode *dec, DmtxRegion *reg)
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

   /* Corners are named using the following layout:
    *
    *  T |
    *    |
    *    |
    *    +-----
    *  O      R
    *
    */

/**
Simplify StepAlongEdge() and avoid complicated error-prone counters.

Step along all 4 edges.

Eliminate any of the edges that are already known to be bad

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

/*fprintf(stdout, "MatrixRegionAlignSecondEdge()\n"); */

   /* Scan top edge left-to-right (shear only)

      +---   O = intersection of known edges
      |      T = farthest known point along scanned edge
      |      R = old O
   */
   dmtxMatrix3Shear(postRaw2Fit, 0.0, 1.0);
   dmtxMatrix3Shear(preFit2Raw, 0.0, -1.0);

   hitCount[0] = MatrixRegionAlignEdge(dec, reg, postRaw2Fit,
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

   hitCount[1] = MatrixRegionAlignEdge(dec, reg, postRaw2Fit,
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

   hitCount[2] = MatrixRegionAlignEdge(dec, reg, postRaw2Fit,
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

   hitCount[3] = MatrixRegionAlignEdge(dec, reg, postRaw2Fit,
         preFit2Raw, &p0[3], &p1[3], &pCorner[3], &weakCount[3]);

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
   if(dmtxVector2Norm(&rayOT.v) < 0.0)
      return DMTX_FAILURE;

   rayNew.p = p0[bestFit];
   dmtxVector2Sub(&rayNew.v, &p1[bestFit], &p0[bestFit]);
   if(dmtxVector2Norm(&rayNew.v) < 0.0)
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

   if(MatrixRegionUpdateXfrms(dec, reg) != DMTX_SUCCESS)
      return DMTX_FAILURE;

   /* Skewed barcode in the bottom middle pane */
/* CALLBACK_DECODE_FUNC1(buildMatrixCallback4, dec, reg->fit2raw); */

   return DMTX_SUCCESS;
}

/**
 * @brief  XXX
 * @param  image
 * @param  reg
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
MatrixRegionAlignRightEdge(DmtxDecode *dec, DmtxRegion *reg)
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

   if(MatrixRegionAlignCalibEdge(dec, reg, DmtxEdgeRight, preFit2Raw, postRaw2Fit) != DMTX_SUCCESS)
      return DMTX_FAILURE;

   return DMTX_SUCCESS;
}

/**
 * @brief  XXX
 * @param  image
 * @param  reg
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
MatrixRegionAlignTopEdge(DmtxDecode *dec, DmtxRegion *reg)
{
   DmtxMatrix3 preFit2Raw, postRaw2Fit;

   dmtxMatrix3Shear(postRaw2Fit, 0.0, 0.5);
   dmtxMatrix3Shear(preFit2Raw, 0.0, -0.5);

   if(MatrixRegionAlignCalibEdge(dec, reg, DmtxEdgeTop, preFit2Raw, postRaw2Fit) != DMTX_SUCCESS)
      return DMTX_FAILURE;

   return DMTX_SUCCESS;
}

/**
 * @brief  XXX
 * @param  image
 * @param  reg
 * @param  edgeLoc
 * @param  preFit2Raw
 * @param  postRaw2Fit
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
MatrixRegionAlignCalibEdge(DmtxDecode *dec, DmtxRegion *reg,
      DmtxEdgeLoc edgeLoc, DmtxMatrix3 preFit2Raw, DmtxMatrix3 postRaw2Fit)
{
   DmtxVector2 p0, p1, pCorner;
   double slope;
   int hitCount;
   int weakCount;

   assert(edgeLoc == DmtxEdgeTop || edgeLoc == DmtxEdgeRight);

   hitCount = MatrixRegionAlignEdge(dec, reg, postRaw2Fit, preFit2Raw, &p0, &p1, &pCorner, &weakCount);
   if(hitCount < 2)
      return DMTX_FAILURE;

   if(edgeLoc == DmtxEdgeRight) {
      SetCornerLoc(reg, DmtxCorner10, pCorner);
      if(MatrixRegionUpdateXfrms(dec, reg) != DMTX_SUCCESS)
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
      if(MatrixRegionUpdateXfrms(dec, reg) != DMTX_SUCCESS)
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
   if(MatrixRegionUpdateXfrms(dec, reg) != DMTX_SUCCESS)
      return DMTX_FAILURE;

   return DMTX_SUCCESS;
}

/**
 * @brief  XXX
 * @param  image
 * @param  reg
 * @param  postRaw2Fit
 * @param  preFit2Raw
 * @param  p0
 * @param  p1
 * @param  pCorner
 * @param  weakCount
 * @return 3 points in raw coordinates
 */
static int
MatrixRegionAlignEdge(DmtxDecode *dec, DmtxRegion *reg,
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

      dmtxMatrix3VMultiply(&pTmp, &pRawProgress, sRaw2Fit);

      /* Only update forward and lateral vectors before leaving finder bar */
      if(pTmp.X < 0.05) {
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

         /* XXX instead of just failing here, hopefully find what happened
                upstream to trigger this condition. we can probably avoid
                this earlier on, and even avoid assertion failures elsewhere */
         if(RightAngleTrueness(c01, c00, c10, M_PI) < 0.1)
            return 0;

         /* Calculate forward and lateral directions in raw coordinates */
         dmtxVector2Sub(&forward, &c10, &c00);
         if(dmtxVector2Norm(&forward) < 0.0)
            return 0;

         dmtxVector2Sub(&lateral, &c01, &c00);
         if(dmtxVector2Norm(&lateral) < 0.0)
            return 0;
      }

      prevEdgeHit = edgeHit;
      edgeHit = StepAlongEdge(dec, reg, &pRawProgress, &pRawExact, forward, lateral);
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

      if(dmtxImageContainsFloat(dec->image, pRawProgress.X, pRawProgress.Y) == DMTX_FALSE)
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
 * @brief  XXX
 * @param  image
 * @param  reg
 * @param  pProgress
 * @param  pExact
 * @param  forward
 * @param  lateral
 * @return 2 points in non-skewed raw pixel coordinates
 */
static int
StepAlongEdge(DmtxDecode *dec, DmtxRegion *reg, DmtxVector2 *pProgress,
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
   compass = GetCompassEdge(dec->image, x, y, DmtxCompassDirOrtho);

   /* If pixel shows a weak edge in any direction then advance forward */
   if(compass.magnitude < dec->edgeThresh * 17.68) {
      dmtxVector2AddTo(pProgress, &forward);
/*    CALLBACK_POINT_PLOT(*pProgress, 1, 5, DMTX_DISPLAY_POINT); */
      return DMTX_EDGE_STEP_TOO_WEAK;
   }

   /* forward is toward edge */
   /* lateral is away from edge */

   /* Determine orthagonal step directions */
   if(compass.maxDirOrtho == DmtxCompassDir0) {
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
   compassPrev = GetCompassEdge(dec->image, x-xToward, y-yToward, compass.maxDirAll);
   compassNext = GetCompassEdge(dec->image, x+xToward, y+yToward, compass.maxDirAll);

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
   reg->gradient.tMid = bestContrast / 2.0;

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
/* assert(xStart >= -1 && xStart <= reg->symbolCols); */
/* assert(yStart >= -1 && yStart <= reg->symbolRows); */

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
 */
/**
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
