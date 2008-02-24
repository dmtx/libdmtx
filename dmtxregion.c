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
 * This function returns either when a region is found or when the file has been completely read
 * returns: DMTX_REGION_FOUND
 *          DMTX_REGION_EOF
 */
extern int
dmtxFindNextRegion(DmtxDecode *decode)
{
   DmtxPixelLoc loc;
   DmtxScanGrid *grid;

   grid = &(decode->grid);

   /* Pick up where we left off after last scan */
   while(grid->extent >= grid->minExtent) {

      /* Extract pixel location of current progress from scan grid */
      loc = GetGridCoordinates(grid);

      /* Increment early to avoid any chance of revisiting this location */
      IncrementPixelProgress(grid);

      /* Scan this pixel for the presence of a valid barcode edge */
      /* XXX remember that later this function will not decode the actual barcode yet */
      if(dmtxScanPixel(decode, loc) == DMTX_REGION_FOUND)
         return DMTX_REGION_FOUND;
   }

   return DMTX_REGION_EOF;
}

/**
 * @param decode   pointer to DmtxDecode information struct
 * @param dir      scan direction (DmtxUp|DmtxRight)
 * @param lineNbr  number of image row or column to be scanned
 * @return         number of barcodes scanned
 */
extern int
dmtxScanPixel(DmtxDecode *decode, DmtxPixelLoc loc)
{
   int success;
   DmtxEdgeSubPixel edgeStart;
   DmtxRay2 ray0, ray1;
   DmtxCompassEdge compassEdge;

   if(loc.X < 0 || loc.X >= decode->image->width ||
         loc.Y < 0 || loc.Y >= decode->image->height)
      return DMTX_REGION_NOT_FOUND; /* XXX change to DMTX_REGION_out_of_bounds */

   /* Test whether this pixel is sitting on an edge of any direction */
   compassEdge = GetCompassEdge(decode->image, loc.X, loc.Y, DMTX_ALL_COMPASS_DIRS);
   if(compassEdge.magnitude < 60)
      return DMTX_REGION_NOT_FOUND;

   /* If the edge is strong then find its subpixel location */
   edgeStart = FindZeroCrossing(decode->image, loc.X, loc.Y, compassEdge);
   edgeStart.compass = compassEdge; /* XXX for now ... need to preserve scanDir */
   if(!edgeStart.isEdge)
      return DMTX_REGION_NOT_FOUND;

   /* Next follow it to the end in both directions */
   ray0 = FollowEdge(decode->image, loc.X, loc.Y, edgeStart, 1, decode);
   ray1 = FollowEdge(decode->image, loc.X, loc.Y, edgeStart, -1, decode);

   memset(&(decode->region), 0x00, sizeof(DmtxRegion));
   decode->region.valid = 0;

   success = MatrixRegionAlignFirstEdge(decode, &edgeStart, ray0, ray1);
   if(!success)
      return DMTX_REGION_NOT_FOUND;

   success = MatrixRegionAlignSecondEdge(decode);
   if(!success)
      return DMTX_REGION_NOT_FOUND;

   success = MatrixRegionAlignRightEdge(decode);
   if(!success)
      return DMTX_REGION_NOT_FOUND;

   success = MatrixRegionAlignTopEdge(decode);
   if(!success)
      return DMTX_REGION_NOT_FOUND;

   /* XXX When adding clipping functionality against previously
      scanned barcodes, this is a good place to add a test for
      collisions.  Although we tested for this early on in the jump
      region scan, the subsequent follower and alignment steps may
      have moved us into a collision with another matrix region.  A
      collision at this point is grounds for immediate failure. */

   success = MatrixRegionFindSize(decode);
   if(!success)
      return DMTX_REGION_NOT_FOUND;

   /* XXX logic below this point should be broken off into a separate
      decode function (possibly in dmtxdecode.c) because:

      * it has nothing to do with region detection
      * it needs to be called by different parts of code (mosaic vs matrix)
      * it needs to be called multiple times during mosaic decoding

      .. but for now just make an ugly hack ...
   */

   if(decode->mosaic) {
      success = AllocateStorage(decode);
      if(!success)
         return DMTX_REGION_FOUND;

      success = PopulateArrayFromMosaic(decode);
      if(!success)
         return DMTX_REGION_FOUND;
/**
      success = DecodeMosaicRegion(&region);
      if(!success) {
         decode->region.status = DMTX_STATUS_INVALID;
         return DMTX_REGION_NOT_FOUND;
      }
*/
   }
   else {
      success = AllocateStorage(decode);
      if(!success)
         return DMTX_REGION_FOUND;

      success = PopulateArrayFromMatrix(decode);
      if(!success)
         return DMTX_REGION_FOUND;

      success = DecodeMatrixRegion(&(decode->region));
      if(!success)
         return DMTX_REGION_FOUND;
   }

   decode->region.valid = 1;
   return DMTX_REGION_FOUND;
}

/**
 * XXX
 * TODO: this function should really be static --- move "decode" functions into this file
 *
 * @param
 * @return XXX
 */
extern void
dmtxMatrixRegionDeInit(DmtxRegion *region)
{
   if(region->array != NULL)
      free(region->array);

   if(region->code != NULL)
      free(region->code);

   if(region->output != NULL)
      free(region->output);

   memset(region, 0x00, sizeof(DmtxRegion));
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
   DmtxPixel pixel;
   DmtxCompassEdge edge, maxEdge;
   DmtxColor3 color, black = { 0.0, 0.0, 0.0 }; /* XXX move black to a global scope later */

   assert(edgeScanDirs);

   /* Set maxEdge to invalid state */
   maxEdge.edgeDir = -1;
   maxEdge.scanDir = -1;
   maxEdge.intensity = black;
   maxEdge.magnitude = 0.0;

   mag0 = mag90 = -1.0;

   if(x <= 0 || x >= image->width - 1 || y <= 0 || y >= image->height - 1)
      return maxEdge; /* XXX should really communicate failure with a dedicated value instead */

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

         /* Accomodate 1 pixel beyond edge of image with nearest neighbor value */
         ClampIntRange(&xAdjust, 0, image->width - 1);
         ClampIntRange(&yAdjust, 0, image->height - 1);

         /* Weight pixel value by appropriate coefficient in convolution matrix */
         coefficientIdx = (patternIdx - dirIdx + 8) % 8;
         pixel = dmtxPixelFromImage(image, xAdjust, yAdjust);
         dmtxColor3FromPixel(&color, &pixel);
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
FollowEdge(DmtxImage *image, int x, int y, DmtxEdgeSubPixel edgeStart, int forward, DmtxDecode *decode)
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
/*       CALLBACK_DECODE_FUNC4(plotPointCallback, decode, p, 1, 1, DMTX_DISPLAY_POINT); */

         if(edge.compass.magnitude > 0.50 * compass.magnitude)
            pSoft = p;
         if(edge.compass.magnitude > 0.90 * compass.magnitude)
            pHard = p;
      }

      xFollow = (int)(edge.xInt + edge.xFrac + 0.5) + xIncrement;
      yFollow = (int)(edge.yInt + edge.yFrac + 0.5) + yIncrement;
   }

/* CALLBACK_DECODE_FUNC4(plotPointCallback, decode, pHard, 1, 1, DMTX_DISPLAY_SQUARE); */

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
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionUpdateXfrms(DmtxRegion *region, DmtxImage *image)
{
   DmtxVector2 v01, vTmp, vCenter, pCenter;
   double tx, ty, phi, shx, scx, scy, skx, sky;
   double dimOT, dimOX, dimOR, dimRT, dimTX, dimRX;
   DmtxMatrix3 m, mtxy, mphi, mshx, mscxy, msky, mskx;
   DmtxCorners corners;

   assert((region->corners.known & DmtxCorner00) && (region->corners.known & DmtxCorner01));

   /* Make copy of known corners to update with temporary values */
   corners = region->corners;

   if(corners.c00.X < 0.0 || corners.c00.Y < 0.0 ||
      corners.c01.X < 0.0 || corners.c01.Y < 0.0)
      return DMTX_FAILURE;

   if(corners.c00.X > image->width - 1 || corners.c00.Y > image->height - 1 ||
      corners.c01.X > image->width - 1 || corners.c01.Y > image->height - 1)
      return DMTX_FAILURE;

   dimOT = dmtxVector2Mag(dmtxVector2Sub(&v01, &corners.c01, &corners.c00)); /* XXX could use MagSquared() */
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
      vTmp.X = v01.Y;
      vTmp.Y = -v01.X;
      dmtxVector2Add(&corners.c10, &corners.c00, &vTmp);

      /* Choose direction that points toward center of image */
      pCenter.X = image->width/2.0;
      pCenter.Y = image->height/2.0;
      dmtxVector2Sub(&vCenter, &pCenter, &corners.c00);

      if(dmtxVector2Dot(&vTmp, &vCenter) < 0.0)
         dmtxVector2ScaleBy(&vTmp, -1.0);
   }

   /* Top-right corner -- validate if known or create temporary value */
   if(corners.known & DmtxCorner11) {
      if(corners.c11.X < 0.0 || corners.c11.Y < 0.0 ||
            corners.c11.X > image->width - 1 ||
            corners.c11.Y > image->height - 1)
         return DMTX_FAILURE;
   }
   else {
      dmtxVector2Add(&corners.c11, &corners.c10, &v01);
   }

   /* Verify that the 4 corners define a reasonably fat quadrilateral */
   dimOX = dmtxVector2Mag(dmtxVector2Sub(&vTmp, &corners.c11, &corners.c00)); /* XXX could use MagSquared() */
   dimOR = dmtxVector2Mag(dmtxVector2Sub(&vTmp, &corners.c10, &corners.c00)); /* XXX could use MagSquared() */
   dimRT = dmtxVector2Mag(dmtxVector2Sub(&vTmp, &corners.c01, &corners.c10)); /* XXX could use MagSquared() */
   dimTX = dmtxVector2Mag(dmtxVector2Sub(&vTmp, &corners.c11, &corners.c01)); /* XXX could use MagSquared() */
   dimRX = dmtxVector2Mag(dmtxVector2Sub(&vTmp, &corners.c11, &corners.c10)); /* XXX could use MagSquared() */

   if(dimOR < 8 || dimTX < 8 || dimRX < 8 || dimOX < 11 || dimRT < 11)
      return DMTX_FAILURE;

   if(min(dimOX,dimRT)/max(dimOX,dimRT) < 0.5 ||
      min(dimOT,dimRX)/max(dimOT,dimRX) < 0.5 ||
      min(dimOR,dimTX)/max(dimOR,dimTX) < 0.5)
      return DMTX_FAILURE;

   /* Calculate values needed for transformations */
   tx = -1 * corners.c00.X;
   ty = -1 * corners.c00.Y;
   dmtxMatrix3Translate(mtxy, tx, ty);

   phi = atan2(v01.X, v01.Y);
   dmtxMatrix3Rotate(mphi, phi);

   /* Update transformation with values known so far */
   dmtxMatrix3Multiply(m, mtxy, mphi);

   dmtxMatrix3VMultiply(&vTmp, &corners.c10, m);
   assert(vTmp.X > DMTX_ALMOST_ZERO);
   shx = -vTmp.Y / vTmp.X;
   dmtxMatrix3Shear(mshx, 0.0, shx);

   scx = 1.0/vTmp.X;
   scy = 1.0/dmtxVector2Mag(&v01);
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
   dmtxMatrix3Multiply(region->raw2fit, m, msky);

   /* Create inverse tranformation fit2raw (true matrix inverse is unnecessary) */
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
   dmtxMatrix3Multiply(region->fit2raw, m, mtxy);

   return DMTX_SUCCESS;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionAlignFirstEdge(DmtxDecode *decode, DmtxEdgeSubPixel *edgeStart, DmtxRay2 ray0, DmtxRay2 ray1)
{
   int success;
   DmtxRay2 rayFull;
   DmtxVector2 p0, p1, pTmp;
   DmtxRegion *region;

   region = &(decode->region);

/*fprintf(stdout, "MatrixRegionAlignFirstEdge()\n"); */
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
      dmtxVector2Norm(&pTmp);
      rayFull.v = pTmp;
      rayFull.tMin = 0;
      rayFull.tMax = dmtxDistanceAlongRay2(&rayFull, &p0);
      /* XXX else choose longer of the two */
   }
   else {
      rayFull = (ray0.isDefined) ? ray0 : ray1;
   }

   /* Reject edges shorter than 10 pixels */
   if(rayFull.tMax < 10) {
      return DMTX_FAILURE;
   }

   SetCornerLoc(region, DmtxCorner00, rayFull.p);
   SetCornerLoc(region, DmtxCorner01, p0);

   success = MatrixRegionUpdateXfrms(region, decode->image);
   if(!success)
      return DMTX_FAILURE;

   /* Top-right pane showing first edge fit */
   CALLBACK_DECODE_FUNC1(buildMatrixCallback2, decode, region);

   return DMTX_SUCCESS;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static void
SetCornerLoc(DmtxRegion *region, DmtxCornerLoc cornerLoc, DmtxVector2 point)
{
   switch(cornerLoc) {
      case DmtxCorner00:
         region->corners.c00 = point;
         break;
      case DmtxCorner10:
         region->corners.c10 = point;
         break;
      case DmtxCorner11:
         region->corners.c11 = point;
         break;
      case DmtxCorner01:
         region->corners.c01 = point;
         break;
   }

   region->corners.known |= cornerLoc;
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
MatrixRegionAlignSecondEdge(DmtxDecode *decode)
{
   int success;
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
   DmtxRegion *region;

/*fprintf(stdout, "MatrixRegionAlignSecondEdge()\n"); */
   region = &(decode->region);

   /* Scan top edge left-to-right (shear only)

      +---   O = intersection of known edges
      |      T = farthest known point along scanned edge
      |      R = old O
   */
   dmtxMatrix3Shear(postRaw2Fit, 0.0, 1.0);
   dmtxMatrix3Shear(preFit2Raw, 0.0, -1.0);

   hitCount[0] = MatrixRegionAlignEdge(decode, postRaw2Fit, preFit2Raw,
         &p0[0], &p1[0], &pCorner[0], &weakCount[0]);

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

   hitCount[1] = MatrixRegionAlignEdge(decode, postRaw2Fit, preFit2Raw,
         &p0[1], &p1[1], &pCorner[1], &weakCount[1]);

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

   hitCount[2] = MatrixRegionAlignEdge(decode, postRaw2Fit, preFit2Raw,
         &p0[2], &p1[2], &pCorner[2], &weakCount[2]);

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

   hitCount[3] = MatrixRegionAlignEdge(decode, postRaw2Fit, preFit2Raw,
         &p0[3], &p1[3], &pCorner[3], &weakCount[3]);

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
   dmtxMatrix3VMultiply(&oldRawT, &fitT, region->fit2raw);

   fitO.X = 0.0;
   fitO.Y = 0.0;
   dmtxMatrix3VMultiply(&oldRawO, &fitO, region->fit2raw);

   if(bestFit == 0 || bestFit == 1)
      oldRawT = pCorner[bestFit];
   else
      oldRawO = pCorner[bestFit];

   rayOT.p = oldRawO;
   dmtxVector2Sub(&rayOT.v, &oldRawT, &oldRawO);
   dmtxVector2Norm(&rayOT.v);

   rayNew.p = p0[bestFit];
   dmtxVector2Sub(&rayNew.v, &p1[bestFit], &p0[bestFit]);
   dmtxVector2Norm(&rayNew.v);

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

   SetCornerLoc(region, DmtxCorner00, O);
   SetCornerLoc(region, DmtxCorner01, T);
   SetCornerLoc(region, DmtxCorner10, R);

   success = MatrixRegionUpdateXfrms(region, decode->image);

   /* Skewed barcode in the bottom middle pane */
/* CALLBACK_DECODE_FUNC1(buildMatrixCallback4, decode, region->fit2raw); */

   return success;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionAlignRightEdge(DmtxDecode *decode)
{
   int success;
   DmtxMatrix3 rotate, flip, skew, scale;
   DmtxMatrix3 preFit2Raw, postRaw2Fit;
   DmtxRegion *region;

   region = &(decode->region);

   dmtxMatrix3Rotate(rotate, -M_PI_2);
   dmtxMatrix3Scale(flip, 1.0, -1.0);
   dmtxMatrix3LineSkewTop(skew, 1.0, 0.5, 1.0);
   dmtxMatrix3Scale(scale, 1.25, 1.0);

   dmtxMatrix3Multiply(postRaw2Fit, rotate, flip);
   dmtxMatrix3MultiplyBy(postRaw2Fit, skew);
   dmtxMatrix3MultiplyBy(postRaw2Fit, scale);

   dmtxMatrix3Scale(scale, 0.8, 1.0);
   dmtxMatrix3LineSkewTopInv(skew, 1.0, 0.5, 1.0);
   dmtxMatrix3Scale(flip, 1.0, -1.0);
   dmtxMatrix3Rotate(rotate, M_PI_2);
   dmtxMatrix3Multiply(preFit2Raw, scale, skew);
   dmtxMatrix3MultiplyBy(preFit2Raw, flip);
   dmtxMatrix3MultiplyBy(preFit2Raw, rotate);

   success = MatrixRegionAlignCalibEdge(decode, DmtxEdgeRight, preFit2Raw, postRaw2Fit);

   return success;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionAlignTopEdge(DmtxDecode *decode)
{
   int success;
   DmtxMatrix3 preFit2Raw, postRaw2Fit;

   dmtxMatrix3LineSkewTop(postRaw2Fit, 1.0, 0.5, 1.0);
   dmtxMatrix3LineSkewTopInv(preFit2Raw, 1.0, 0.5, 1.0);

   success = MatrixRegionAlignCalibEdge(decode, DmtxEdgeTop, preFit2Raw, postRaw2Fit);

   return success;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionAlignCalibEdge(DmtxDecode *decode, DmtxEdgeLoc edgeLoc, DmtxMatrix3 preFit2Raw, DmtxMatrix3 postRaw2Fit)
{
   DmtxVector2 p0, p1, pCorner;
   DmtxVector2 cFit, cBefore, cAfter, cTmp;
   int success;
   double slope;
   int hitCount;
   int weakCount;
   DmtxRegion *region;

   assert(edgeLoc == DmtxEdgeTop || edgeLoc == DmtxEdgeRight);

   region = &(decode->region);

   hitCount = MatrixRegionAlignEdge(decode, postRaw2Fit, preFit2Raw, &p0, &p1, &pCorner, &weakCount);
   if(hitCount < 2)
      return DMTX_FAILURE;

   /* Update pCorner first, tracking value before and after update */
   if(edgeLoc == DmtxEdgeRight) {
      cFit.X = 1.0;
      cFit.Y = 0.0;
      dmtxMatrix3VMultiply(&cBefore, &cFit, region->fit2raw);
/* XXX we are probably doing a few extra ops here */
      SetCornerLoc(region, DmtxCorner10, pCorner);
      success = MatrixRegionUpdateXfrms(region, decode->image);
      if(!success)
         return DMTX_FAILURE;

      dmtxMatrix3VMultiply(&cAfter, &cFit, region->fit2raw);
   }
   else {
      cFit.X = 0.0;
      cFit.Y = 1.0;
      dmtxMatrix3VMultiply(&cBefore, &cFit, region->fit2raw);

      SetCornerLoc(region, DmtxCorner01, pCorner);
      success = MatrixRegionUpdateXfrms(region, decode->image);
      if(!success)
         return DMTX_FAILURE;

      dmtxMatrix3VMultiply(&cBefore, &cFit, region->fit2raw);
   }

   /* If pCorner's change was significant then it probably affected edge
      fit quality.  Since pCorner is now correct, a second edge alignment
      should give accurate results. */
   if(dmtxVector2Mag(dmtxVector2Sub(&cTmp, &cAfter, &cBefore)) > 20.0) {
      hitCount = MatrixRegionAlignEdge(decode, postRaw2Fit, preFit2Raw, &p0, &p1, &pCorner, &weakCount);
      if(hitCount < 2)
         return DMTX_FAILURE;
   }

   /* With reliable edge fit results now update remaining corners */
   if(edgeLoc == DmtxEdgeRight) {
      dmtxMatrix3VMultiplyBy(&p0, region->raw2fit);
      dmtxMatrix3VMultiplyBy(&p1, region->raw2fit);

      assert(fabs(p1.Y - p0.Y) > DMTX_ALMOST_ZERO);
      slope = (p1.X - p0.X) / (p1.Y - p0.Y);

      p0.X = p0.X - slope * p0.Y;
      p0.Y = 0.0;
      p1.X = p0.X + slope;
      p1.Y = 1.0;

      dmtxMatrix3VMultiplyBy(&p0, region->fit2raw);
      dmtxMatrix3VMultiplyBy(&p1, region->fit2raw);

      SetCornerLoc(region, DmtxCorner10, p0);
      SetCornerLoc(region, DmtxCorner11, p1);
   }
   else {
      dmtxMatrix3VMultiplyBy(&p0, region->raw2fit);
      dmtxMatrix3VMultiplyBy(&p1, region->raw2fit);

      assert(fabs(p1.X - p0.X) > DMTX_ALMOST_ZERO);
      slope = (p1.Y - p0.Y) / (p1.X - p0.X);

      p0.Y = p0.Y - slope * p0.X;
      p0.X = 0.0;
      p1.Y = p0.Y + slope;
      p1.X = 1.0;

      dmtxMatrix3VMultiplyBy(&p0, region->fit2raw);
      dmtxMatrix3VMultiplyBy(&p1, region->fit2raw);

      SetCornerLoc(region, DmtxCorner01, p0);
      SetCornerLoc(region, DmtxCorner11, p1);
   }
   success = MatrixRegionUpdateXfrms(region, decode->image);
   if(!success)
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
MatrixRegionAlignEdge(DmtxDecode *decode, DmtxMatrix3 postRaw2Fit, DmtxMatrix3 preFit2Raw, DmtxVector2 *p0, DmtxVector2 *p1, DmtxVector2 *pCorner, int *weakCount)
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
   DmtxRegion *region;

   region = &(decode->region);

/*fprintf(stdout, "MatrixRegionAlignEdge()\n"); */
   dmtxMatrix3Multiply(sRaw2Fit, region->raw2fit, postRaw2Fit);
   dmtxMatrix3Multiply(sFit2Raw, preFit2Raw, region->fit2raw);

   /* Draw skewed image in bottom left pane */
/* CALLBACK_DECODE_FUNC1(buildMatrixCallback3, decode, sFit2Raw); */

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

      /* XXX still not happy with this */
      c00 = pTmp;
      c10.X = c00.X + 0.1;
      c10.Y = c00.Y;
      c01.X = c00.X - 0.0087155743; /* sin(5deg) */
      c01.Y = c00.Y + 0.0996194698; /* cos(5deg) */

      dmtxMatrix3VMultiplyBy(&c00, sFit2Raw);
      dmtxMatrix3VMultiplyBy(&c10, sFit2Raw);
      dmtxMatrix3VMultiplyBy(&c01, sFit2Raw);

      /* Calculate forward and lateral directions in raw coordinates */
      dmtxVector2Sub(&forward, &c10, &c00);
      /* XXX modify dmtxVector2Norm() to return failure without assert */
      if(dmtxVector2Mag(&forward) < DMTX_ALMOST_ZERO)
         return 0;
      dmtxVector2Norm(&forward);

      dmtxVector2Sub(&lateral, &c01, &c00);
      if(dmtxVector2Mag(&lateral) < DMTX_ALMOST_ZERO)
         return 0;
      dmtxVector2Norm(&lateral);

      /* Don't allow forward and lateral to point in same direction
         (extreme matrix could otherwise create infinite loop) */
      if(dmtxVector2Dot(&forward, &lateral) > 0.0)
         return 0;

      prevEdgeHit = edgeHit;
      edgeHit = StepAlongEdge(decode->image, region, &pRawProgress, &pRawExact, forward, lateral, decode);
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
/*                CALLBACK_DECODE_FUNC4(plotPointCallback, decode, pRawExact, 1, 1, DMTX_DISPLAY_POINT); */
               }

               if(i == 0) {
                  pLast = pFitExact;
                  CALLBACK_DECODE_FUNC4(plotPointCallback, decode, pRawExact, 1, 1, DMTX_DISPLAY_POINT);
               }
            }
         }

         /* Draw edge hits along skewed edge in bottom left pane */
         CALLBACK_DECODE_FUNC4(xfrmPlotPointCallback, decode, pFitExact, NULL, 4, DMTX_DISPLAY_POINT);
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
/*       CALLBACK_DECODE_FUNC4(plotPointCallback, decode, pRawProgress, 1, 1, DMTX_DISPLAY_SQUARE); */
         break;
      }

      if(pRawProgress.X < 1 || pRawProgress.X > decode->image->width - 1 ||
         pRawProgress.Y < 1 || pRawProgress.Y > decode->image->height - 1)
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
StepAlongEdge(DmtxImage *image, DmtxRegion *region, DmtxVector2 *pProgress, DmtxVector2 *pExact, DmtxVector2 forward, DmtxVector2 lateral, DmtxDecode *decode)
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
/*    CALLBACK_DECODE_FUNC4(plotPointCallback, decode, *pProgress, 1, 1, DMTX_DISPLAY_POINT); */
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

/*    CALLBACK_DECODE_FUNC4(plotPointCallback, decode, *pExact, 2, 1, DMTX_DISPLAY_POINT); */
      return DMTX_EDGE_STEP_EXACT;
   }

   /* Passed edge */
   if(compassPrev.magnitude > compass.magnitude) {
/*    CALLBACK_DECODE_FUNC4(plotPointCallback, decode, *pProgress, 3, 1, DMTX_DISPLAY_POINT); */
      dmtxVector2AddTo(pProgress, &lateral);
      return DMTX_EDGE_STEP_TOO_FAR;
   }

   /* Approaching edge but not there yet */
/* CALLBACK_DECODE_FUNC4(plotPointCallback, decode, *pProgress, 4, 1, DMTX_DISPLAY_POINT); */
   dmtxVector2AddTo(pProgress, &forward);
   return DMTX_EDGE_STEP_NOT_QUITE;
}

/**
 * Determine pattern size by testing calibration bars for alternating
 * foreground and background color modules.  All barcode regions are
 * treated as one large pattern during this step, even multiregion
 * barcodes, since we are only looking at the outside perimeter.
 *
 * @param
 * @return XXX
 */
static int
AllocateStorage(DmtxDecode *decode)
{
   DmtxRegion *region;

   region = &(decode->region);

   region->symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, region->sizeIdx);
   region->symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, region->sizeIdx);
   region->mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, region->sizeIdx);
   region->mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, region->sizeIdx);

   region->arraySize = sizeof(unsigned char) * region->mappingRows * region->mappingCols;

   region->array = (unsigned char *)malloc(region->arraySize);
   if(region->array == NULL) {
      perror("Malloc failed");
      exit(2); /* XXX find better error handling here */
   }
   memset(region->array, 0x00, region->arraySize);

   region->codeSize = sizeof(unsigned char) *
         dmtxGetSymbolAttribute(DmtxSymAttribDataWordLength, region->sizeIdx) +
         dmtxGetSymbolAttribute(DmtxSymAttribErrorWordLength, region->sizeIdx);

   region->code = (unsigned char *)malloc(region->codeSize);
   if(region->code == NULL) {
      perror("Malloc failed");
      exit(2); /* XXX find better error handling here */
   }
   memset(region->code, 0x00, region->codeSize);

   /* XXX not sure if this is the right place or even the right approach.
      Trying to allocate memory for the decoded data stream and will
      initially assume that decoded data will not be larger than 2x encoded data */
   region->outputSize = sizeof(unsigned char) * region->codeSize * 10;
   region->output = (unsigned char *)malloc(region->outputSize);
   if(region->output == NULL) {
      perror("Malloc failed");
      exit(2); /* XXX find better error handling here */
   }
   memset(region->output, 0x00, region->outputSize);

   return DMTX_SUCCESS; /* XXX good read */
}

/**
 * XXX new experimental version that operates on sizeIdx instead of region
 *
 * @param
 * @return XXX
 */
static DmtxColor3
ReadModuleColor(DmtxDecode *decode, int symbolRow, int symbolCol, int sizeIdx, DmtxMatrix3 fit2raw)
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

      dmtxMatrix3VMultiply(&p0, &p, fit2raw);
      dmtxColor3FromImage2(&cPoint, decode->image, p0);
/*    dmtxColor3FromImage(&cPoint, decode->image, p0.X, p0.Y); */

      dmtxColor3AddTo(&cAverage, &cPoint);

/*    CALLBACK_DECODE_FUNC4(plotPointCallback, decode, p0, 1, 1, DMTX_DISPLAY_POINT); */
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
MatrixRegionFindSize(DmtxDecode *decode)
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
   DmtxRegion *region;

   region = &(decode->region);

   /* First try all sizes to determine which sizeIdx with best contrast */
   ptr = sizeIdxAttempts;
   gradient.tMin = gradient.tMid = gradient.tMax = 0;
   do {
      sizeIdx = *ptr;
      symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, sizeIdx);
      symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, sizeIdx);

      colorOnAvg = colorOffAvg = black;

      for(row = 0, col = 0; col < symbolCols; col++) {
         colorOn = ReadModuleColor(decode, row, col, sizeIdx, region->fit2raw);
         colorOff = ReadModuleColor(decode, row-1, col, sizeIdx, region->fit2raw);
         dmtxColor3AddTo(&colorOnAvg, &colorOn);
         dmtxColor3AddTo(&colorOffAvg, &colorOff);
      }

      for(row = 0, col = 0; row < symbolRows; row++) {
         colorOn = ReadModuleColor(decode, row, col, sizeIdx, region->fit2raw);
         colorOff = ReadModuleColor(decode, row, col-1, sizeIdx, region->fit2raw);
         dmtxColor3AddTo(&colorOnAvg, &colorOn);
         dmtxColor3AddTo(&colorOffAvg, &colorOff);
      }

      dmtxColor3ScaleBy(&colorOnAvg, 1.0/(symbolRows + symbolCols));
      dmtxColor3ScaleBy(&colorOffAvg, 1.0/(symbolRows + symbolCols));

      testGradient.ray.p = colorOffAvg;
      dmtxColor3Sub(&testGradient.ray.c, &colorOnAvg, &colorOffAvg);
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
         colorOff = ReadModuleColor(decode, row, col + 1, sizeIdx, region->fit2raw);
         tOff = dmtxDistanceAlongRay3(&gradient.ray, &colorOff);
         colorOn = ReadModuleColor(decode, row, col, sizeIdx, region->fit2raw);
         tOn = dmtxDistanceAlongRay3(&gradient.ray, &colorOn);

         if(tOn - tOff < jumpThreshold)
            errors[sizeIdx]++;

         if(errors[sizeIdx] > errors[minErrorsSizeIdx])
            break;
      }

      /* Right calibration column */
      col = symbolCols - 1;
      for(row = 0; row < symbolRows; row += 2) {
         colorOff = ReadModuleColor(decode, row + 1, col, sizeIdx, region->fit2raw);
         tOff = dmtxDistanceAlongRay3(&gradient.ray, &colorOff);
         colorOn = ReadModuleColor(decode, row, col, sizeIdx, region->fit2raw);
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

   region->gradient = gradient;
   region->sizeIdx = minErrorsSizeIdx;

   if(errors[minErrorsSizeIdx] >= 4)
      return DMTX_FAILURE;

   return DMTX_SUCCESS;
}

/*
 *
 *
 */
static int
PopulateArrayFromMatrix(DmtxDecode *decode)
{
   int weightFactor;
   int mapWidth, mapHeight;
   int xRegionTotal, yRegionTotal;
   int xRegionCount, yRegionCount;
   int xOrigin, yOrigin;
   int mapCol, mapRow;
   int colTmp, rowTmp, idx;
   int tally[24][24]; /* Large enough to map largest single region */
   DmtxRegion *region;

   region = &(decode->region);

   memset(region->array, 0x00, region->arraySize);

   /* Capture number of regions present in barcode */
   xRegionTotal = dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, region->sizeIdx);
   yRegionTotal = dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, region->sizeIdx);

   /* Capture region dimensions (not including border modules) */
   mapWidth = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionCols, region->sizeIdx);
   mapHeight = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionRows, region->sizeIdx);

   weightFactor = 2 * (mapHeight + mapWidth + 2);
   assert(weightFactor > 0);

   /* Tally module changes for each region in each direction */
   for(yRegionCount = 0; yRegionCount < yRegionTotal; yRegionCount++) {

      /* Y location of mapping region origin in symbol coordinates */
      yOrigin = yRegionCount * (mapHeight + 2) + 1;

      for(xRegionCount = 0; xRegionCount < xRegionTotal; xRegionCount++) {

         /* X location of mapping region origin in symbol coordinates */
         xOrigin = xRegionCount * (mapWidth + 2) + 1;

         memset(tally, 0x00, 24 * 24 * sizeof(int));
         /* XXX see if there's a clean way to remove decode from this call */
         TallyModuleJumps(decode, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirUp);
         TallyModuleJumps(decode, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirLeft);
         TallyModuleJumps(decode, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirDown);
         TallyModuleJumps(decode, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirRight);

         /* Decide module status based on final tallies */
         for(mapRow = 0; mapRow < mapHeight; mapRow++) {
            for(mapCol = 0; mapCol < mapWidth; mapCol++) {

               rowTmp = (yRegionCount * mapHeight) + mapRow;
               rowTmp = yRegionTotal * mapHeight - rowTmp - 1;
               colTmp = (xRegionCount * mapWidth) + mapCol;
               idx = (rowTmp * xRegionTotal * mapWidth) + colTmp;

               if(tally[mapRow][mapCol]/(double)weightFactor > 0.5)
                  region->array[idx] = DMTX_MODULE_ON_RGB;
               else
                  region->array[idx] = DMTX_MODULE_OFF;

               region->array[idx] |= DMTX_MODULE_ASSIGNED;
            }
         }
      }
   }

   return 1;
}

/*
 *
 *
 */
static void
TallyModuleJumps(DmtxDecode *decode, int tally[][24], int xOrigin, int yOrigin, int mapWidth, int mapHeight, int direction)
{
   int extent, weight;
   int travelStep;
   int symbolRow, symbolCol;
   int mapRow, mapCol;
   int lineStart, lineStop;
   int travelStart, travelStop;
   int *line, *travel;
   double jumpThreshold;
   DmtxColor3 color;
   DmtxRegion *region;
   int statusPrev, statusModule;
   double tPrev, tModule;
/* double debug[24][24]; */

   assert(direction == DmtxDirUp || direction == DmtxDirLeft ||
         direction == DmtxDirDown || direction == DmtxDirRight);

   region = &(decode->region);

   travelStep = (direction == DmtxDirUp || direction == DmtxDirRight) ? 1 : -1;

   /* Abstract row and column progress using pointers to allow grid
      traversal in all 4 directions using same logic */

   if(direction & DmtxDirHorizontal) {
      line = &symbolRow;
      travel = &symbolCol;
      extent = mapWidth;
      lineStart = yOrigin;
      lineStop = yOrigin + mapHeight;
      travelStart = (travelStep == 1) ? xOrigin - 1 : xOrigin + mapWidth;
      travelStop = (travelStep == 1) ? xOrigin + mapWidth : xOrigin - 1;
   }
   else {
      assert(direction & DmtxDirVertical);
      line = &symbolCol;
      travel = &symbolRow;
      extent = mapHeight;
      lineStart = xOrigin;
      lineStop = xOrigin + mapWidth;
      travelStart = (travelStep == 1) ? yOrigin - 1: yOrigin + mapHeight;
      travelStop = (travelStep == 1) ? yOrigin + mapHeight : yOrigin - 1;
   }

   jumpThreshold = 0.3 * (region->gradient.tMax - region->gradient.tMin);

   assert(jumpThreshold > 0);

   for(*line = lineStart; *line < lineStop; (*line)++) {

      /* Capture tModule for each leading border module as normal but
         decide status based on predictable barcode border pattern */

      *travel = travelStart;
      color = ReadModuleColor(decode, symbolRow, symbolCol, region->sizeIdx, region->fit2raw);
      tModule = dmtxDistanceAlongRay3(&(region->gradient.ray), &color);

      statusModule = (travelStep == 1 || !(*line & 0x01)) ? DMTX_MODULE_ON_RGB : DMTX_MODULE_OFF;

      weight = extent;

      while((*travel += travelStep) != travelStop) {

         tPrev = tModule;
         statusPrev = statusModule;

         /* For normal data-bearing modules capture color and decide
            module status based on comparison to previous "known" module */

         color = ReadModuleColor(decode, symbolRow, symbolCol, region->sizeIdx, region->fit2raw);
         tModule = dmtxDistanceAlongRay3(&(region->gradient.ray), &color);

         if(statusPrev == DMTX_MODULE_ON_RGB) {
            if(tModule < tPrev - jumpThreshold)
               statusModule = DMTX_MODULE_OFF;
            else
               statusModule = DMTX_MODULE_ON_RGB;
         }
         else if(statusPrev == DMTX_MODULE_OFF) {
            if(tModule > tPrev + jumpThreshold)
               statusModule = DMTX_MODULE_ON_RGB;
            else
               statusModule = DMTX_MODULE_OFF;
         }

         mapRow = symbolRow - yOrigin;
         mapCol = symbolCol - xOrigin;
         assert(mapRow < 24 && mapCol < 24);

         if(statusModule == DMTX_MODULE_ON_RGB)
            tally[mapRow][mapCol] += (2 * weight);
/*       else if(statusModule == DMTX_MODULE_UNSURE)
            tally[mapRow][mapCol] += weight; */

/*       debug[mapRow][mapCol] = tModule - tPrev; */

         weight--;
      }

      assert(weight == 0);
   }

/* XXX begin temporary dump */
/*
for(mapRow = mapHeight - 1; mapRow >= 0; mapRow--) {
   for(mapCol = 0; mapCol < mapWidth; mapCol++) {
      fprintf(stdout, "%0.2g ", debug[mapRow][mapCol]);
   }
   fprintf(stdout, "\n");
}
fprintf(stdout, "\n");
*/
/* XXX end temporary dump */
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
PopulateArrayFromMosaic(DmtxDecode *decode)
{
   int col, row, rowTmp;
   int symbolRow, symbolCol;
   int dataRegionRows, dataRegionCols;
   DmtxColor3 color;
   DmtxRegion *region;

   region = &(decode->region);

   dataRegionRows = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionRows, region->sizeIdx);
   dataRegionCols = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionCols, region->sizeIdx);

   memset(region->array, 0x00, region->arraySize);

   for(row = 0; row < region->mappingRows; row++) {

      /* Transform mapping row to symbol row (Swap because the array's
         origin is top-left and everything else is bottom-left) */
      rowTmp = region->mappingRows - row - 1;
      symbolRow = rowTmp + 2 * (rowTmp / dataRegionRows) + 1;

      for(col = 0; col < region->mappingCols; col++) {

         /* Transform mapping col to symbol col */
         symbolCol = col + 2 * (col / dataRegionCols) + 1;

         color = ReadModuleColor(decode, symbolRow, symbolCol, region->sizeIdx, region->fit2raw);

         /* Value has been assigned, but not visited */
         if(color.R < 50)
            region->array[row*region->mappingCols+col] |= DMTX_MODULE_ON_RED;
         if(color.G < 50)
            region->array[row*region->mappingCols+col] |= DMTX_MODULE_ON_GREEN;
         if(color.B < 50)
            region->array[row*region->mappingCols+col] |= DMTX_MODULE_ON_BLUE;

         region->array[row*region->mappingCols+col] |= DMTX_MODULE_ASSIGNED;
      }
   }

   /* Ideal barcode drawn in lower-right (final) window pane */
   CALLBACK_DECODE_FUNC2(finalCallback, decode, decode, region);

   return DMTX_SUCCESS;
}
