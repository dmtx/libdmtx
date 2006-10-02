/*
libdmtx - Data Matrix Encoding/Decoding Library
Copyright (C) 2006 Mike Laughton

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

/* $Id: dmtxregion.c,v 1.6 2006-10-02 03:10:15 mblaughton Exp $ */

/**
 * Scans through a line (vertical or horizontal) of the source image to
 * find and decode valid Data Matrix barcode regions.
 *
 * @param decode   pointer to DmtxDecode information struct
 * @param dir      scan direction (DmtxUp|DmtxRight)
 * @param lineNbr  number of image row or column to be scanned
 * @return         number of barcodes scanned
 */
extern int
dmtxScanLine(DmtxDecode *decode, DmtxDirection dir, int lineNbr)
{
   int              success;
   DmtxJumpScan     jumpScan;
   DmtxEdgeScan     edgeScan;
   DmtxEdgeFollower followLeft, followRight;
   DmtxMatrixRegion matrixRegion;

   assert(decode->image.width > 0 && decode->image.height > 0);
   assert(dir == DmtxDirUp || dir == DmtxDirRight);

   // The outer loop steps through a full row or column of the source
   // image looking for regions that alternate between 2 colors.  This
   // alternating pattern is what you would expect if you took a pixel-wide
   // slice of a Data Matrix barcode image.  libdmtx refers to these
   // regions as "jump regions" since the color value is seen as jumping
   // back and forth across an imaginary middle color (or "edge").

   // Initialize jump scan to cover entire line
   JumpScanInit(&jumpScan, dir, lineNbr, 0, (dir & DmtxDirHorizontal) ?
         decode->image.width : decode->image.height);

   // Loop once for each jump region candidate found on this line
   while(JumpScanNextRegion(&jumpScan, decode) != DMTX_END_OF_RANGE) {

      // Does this jump region meet necessary requirements?
      if(!JumpRegionValid(&jumpScan.region))
         continue;

      // We now have a sufficiently-long region of 2 alternating colors.
      // Next inspect each color boundary (or "edge") in this region to
      // uncover finder bar candidates.

      // Initialize edge scan to search within current jump region
      EdgeScanInit(&edgeScan, &jumpScan);

      // Loop once for each color edge found in this jump region
      while(EdgeScanNextEdge(&edgeScan, decode, &(jumpScan.region.gradient)) != DMTX_END_OF_RANGE) {

         // Finder bars are detected by tracing (or "following") an edge to
         // the left and to the right of the current scan direction.  If
         // the followers find edges that are sufficiently straight and
         // long then they are pursued as valid finder bar candidates.

         EdgeFollowerInit(&followLeft, &edgeScan, &(jumpScan.region.gradient), TurnCorner(dir, DMTX_TURN_CCW));
         EdgeFollowerInit(&followRight, &edgeScan, &(jumpScan.region.gradient), TurnCorner(dir, DMTX_TURN_CW));

         EdgeFollowerFollow(&followLeft, decode);
         EdgeFollowerFollow(&followRight, decode);

         // We now hold a rough trace of the finder bars that border 2
         // adjacent sides of a Data Matrix region.  The following steps
         // will refine that region by building a transformation matrix
         // that maps input pixel coordinates to a rectangular (usually
         // square) 2D space of predetermined dimensions.  This is done
         // by capturing and tweaking a "chain" of input values that are
         // inputs to the transformation matrix and its inverse.

         MatrixRegionInit(&matrixRegion, &(jumpScan.region.gradient));

         success = MatrixRegionAlignFinderBars(&matrixRegion, decode, &followLeft, &followRight);
         if(!success)
            continue;

         success = MatrixRegionAlignTop(&matrixRegion, decode);
         if(!success)
            continue;

         success = MatrixRegionAlignSide(&matrixRegion, decode);
         if(!success)
            continue;

         // XXX Consider splitting remaining functionality into function stored in dmtxdecode.c

         // XXX When adding clipping functionality against previously
         // scanned barcodes, this is a good place to add a test for
         // collisions.  Although we tested for this early on in the jump
         // region scan, the subsequent follower and alignment steps may
         // have moved us into a collision with another matrix region.  A
         // collision at this point is grounds for immediate failure.

         success = MatrixRegionRead(&matrixRegion, decode);
         if(!success)
            continue;

         PopulateArrayFromImage(&matrixRegion, decode);

         ModulePlacementEcc200(&matrixRegion);

         success = CheckErrors(&matrixRegion);
         if(!success) {
            fprintf(stderr, "Rejected due to ECC validation\n");
            continue;
         }

         DataStreamDecode(&matrixRegion);

         // We are now holding a valid, fully decoded Data Matrix barcode.
         // Add this to the list of valid barcodes and continue searching
         // for more.

         if(decode && decode->finalCallback)
            (*(decode->finalCallback))(&matrixRegion);

         decode->matrix[decode->matrixCount++] = matrixRegion;

         if((decode->option & DmtxSingleScanOnly) && decode->matrixCount > 0)
            break;
      }
   }

   return decode->matrixCount;
}

/**
 * Sets the location and boundaries of a region to be scanned.
 *
 * @param range    Pointer to target DmtxScanRange
 * @param dir      Line direction (DmtxDirUp|DmtxDirLeft|DmtxDirDown|DmtxDirRight)
 * @param lineNbr  Number of row or column in image
 * @param firstPos Offset of first pixel in scan range
 * @param lastPos  Offset of last pixel in scan range
 * @return void
 */
static void
ScanRangeSet(DmtxScanRange *range, DmtxDirection dir, int lineNbr, int firstPos, int lastPos)
{
   memset(range, 0x00, sizeof(DmtxScanRange));

   range->dir = dir;
   range->lineNbr = lineNbr;
   range->firstPos = firstPos;
   range->lastPos = lastPos;
}

/**
 * XXX
 * @param dir XXX
 * @param turn XXX
 * @return XXX
 */
static DmtxDirection
TurnCorner(DmtxDirection dir, int turn)
{
   DmtxDirection newDir;

   newDir = (turn == DMTX_TURN_CW) ? 0x0f & (dir >> 1) : 0x0f & (dir << 1);

   return (newDir) ? newDir : (dir ^ 0x0f) & 0x09;
}

/**
 * Initializes jump scan variable boundaries and starting point.  Should
 * be called once per input line (row or column) prior to searching for
 * jump regions.
 *
 * @param jumpScan XXX
 * @param dir      XXX
 * @param lineNbr  XXX
 * @param start    XXX
 * @param length   XXX
 * @return void
 */
static void
JumpScanInit(DmtxJumpScan *jumpScan, DmtxDirection dir, int lineNbr, int start, int length)
{
   memset(jumpScan, 0x00, sizeof(DmtxJumpScan));
   ScanRangeSet(&(jumpScan->range), dir, lineNbr, start, start + length - 1);

   // Initialize "current" region to end at start of range
   jumpScan->region.anchor2 = start;
}

/**
 * Zeros out *jumpRegion and repoints location to start the next region.
 * JumpScanInit() must be called before first call to JumpRegionIncrement();
 *
 * @param jumpRegion XXX
 * @param range Pass range if initializing, NULL if incrementing
 * @return void
 */
static void
JumpRegionIncrement(DmtxJumpRegion *jumpRegion)
{
   int anchorTmp;

   // NOTE: First time here will show jump->region.gradient.isDefined ==
   // DMTX_FALSE due to initialization in JumpScanInit()

   // Reuse the trailing same-colored pixels from the previous region as
   // the leading part of this region if a gradient was established
   anchorTmp = (jumpRegion->gradient.isDefined) ? jumpRegion->lastJump : jumpRegion->anchor2;
   memset(jumpRegion, 0x00, sizeof(DmtxJumpRegion));
   jumpRegion->anchor1 = anchorTmp;
}

/**
 * XXX
 *
 * @param jumpScan XXX
 * @param decode   XXX
 * @return offset | DMTX_END_OF_RANGE
 */
// XXX THIS FUNCTION HAS NOT BEEN REWRITTEN YET
static int
JumpScanNextRegion(DmtxJumpScan *jumpScan, DmtxDecode *decode)
{
   DmtxJumpRegion  *region;
   DmtxScanRange   *range;
   int             anchor1Offset, anchor2Offset;
   float           colorDist;
   float           offGradient, alongGradient;
   DmtxVector3     vDist;
   int             minMaxFlag;
   float           aThird;

   // XXX When adding clipping against previously scanned barcodes you will do a check:
   // if first point is in an existing region then fast forward to after the existing region
   // if non-first point hits a region then this defines the end of your jump region

   region = &(jumpScan->region);
   range = &(jumpScan->range);

   if(region->anchor2 == range->lastPos)
      return DMTX_END_OF_RANGE;

   JumpRegionIncrement(region);

   anchor1Offset = dmtxImageGetOffset(&(decode->image), jumpScan->range.dir, range->lineNbr, region->anchor1);
   dmtxColorFromPixel(&(region->gradient.color), &(decode->image.pxl[anchor1Offset]));

   // For each pixel in the range
   for(region->anchor2 = region->anchor1 + 1; region->anchor2 != range->lastPos; region->anchor2++) {

      anchor2Offset = dmtxImageGetOffset(&(decode->image), jumpScan->range.dir, range->lineNbr, region->anchor2);

      // Capture previous and current pixel color
      region->gradient.colorPrev = region->gradient.color;
      dmtxColorFromPixel(&(region->gradient.color), &(decode->image.pxl[anchor2Offset]));

      // Measure color distance from previous pixel color
      dmtxVector3Sub(&vDist, &(region->gradient.color), &(region->gradient.colorPrev));
      colorDist = dmtxVector3Mag(&vDist);

      // If color distance is larger than image noise
      if(colorDist > DMTX_MIN_JUMP_DISTANCE) {

         // If gradient line does not exist then
         if(!region->gradient.isDefined) {

            // Create gradient line
            region->gradient.ray.p = region->gradient.colorPrev;
            region->gradient.ray.v = vDist;
            dmtxVector3Norm(&(region->gradient.ray.v));

            // Update tMax, tMin, and derived values
            region->gradient.isDefined = 1;
            region->gradient.tMin = 0; // XXX technically this is not necessary (already initialized)
            region->gradient.tMax = dmtxDistanceAlongRay3(&(region->gradient.ray), &(region->gradient.color));
            region->gradient.tMid = (region->gradient.tMin + region->gradient.tMax)/2.0;
            minMaxFlag = 1; // Latest hit was on the high side
            region->lastJump = region->anchor2; // XXX see, this is why the logic shouldn't be in two places
         }
         else { // Gradient line does exist
            // Measure distance from current gradient line
            offGradient = dmtxDistanceFromRay3(&(region->gradient.ray), &(region->gradient.color));

            // If distance from gradient line is large
            if(offGradient > DMTX_MAX_COLOR_DEVN) {

               if(decode && decode->stepScanCallback)
                  (*(decode->stepScanCallback))(decode, range, jumpScan);

               return DMTX_SUCCESS;
            }
            else { // Distance from gradient line is small
               // Update tMax, tMin (distance along gradient lines), and derived values if necessary
               alongGradient = dmtxDistanceAlongRay3(&(region->gradient.ray), &(region->gradient.color));

               if(alongGradient < region->gradient.tMin)
                  region->gradient.tMin = alongGradient;
               else if(alongGradient > region->gradient.tMax)
                  region->gradient.tMax = alongGradient;
               region->gradient.tMid = (region->gradient.tMin + region->gradient.tMax)/2.0;

               // Record that another big jump occurred
               // XXX this should probably record gradient direction shifts rather than big jumps
               aThird = (region->gradient.tMax - region->gradient.tMin)/2.0;
               if(((minMaxFlag == 1 && alongGradient < region->gradient.tMin + aThird) ||
                     (minMaxFlag == -1 && alongGradient > region->gradient.tMax - aThird)) &&
                     aThird > 5.0) {
                  region->jumpCount++;
                  region->lastJump = region->anchor2;
                  minMaxFlag *= -1;
               }
            }
         }
      }
   }

   if(decode && decode->stepScanCallback)
      (*(decode->stepScanCallback))(decode, range, jumpScan);

   return DMTX_SUCCESS;
}

/**
 * Returns true if a jump region is determined to be valid, else false.
 *
 * @param jumpRegion XXX
 * @return jump region validity (true|false)
 */
static int
JumpRegionValid(DmtxJumpRegion *jumpRegion)
{
   return (jumpRegion->jumpCount > DMTX_MIN_JUMP_COUNT &&
           abs(jumpRegion->anchor2 - jumpRegion->anchor1) > DMTX_MIN_STEP_RANGE);
}

/**
 * XXX
 *
 * @param
 * @return void
 */
static void
EdgeScanInit(DmtxEdgeScan *edgeScan, DmtxJumpScan *jumpScan)
{
   memset(edgeScan, 0x00, sizeof(DmtxEdgeScan));

   // Set edge scan range by starting with a copy of the jump scan's range
   edgeScan->range = jumpScan->range;
   edgeScan->range.firstPos = jumpScan->region.anchor1;
   edgeScan->range.lastPos = jumpScan->region.anchor2 - 1;

   // Initialize "current" edge to end at start of full jump scan range
   edgeScan->edge.offset = edgeScan->edgeNext.offset = edgeScan->range.firstPos;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
EdgeScanNextEdge(DmtxEdgeScan *edgeScan, DmtxDecode *decode, DmtxGradient *gradient)
{
   int pxlOffset;
   float lower, upper;

   if(edgeScan->edge.offset == edgeScan->range.lastPos)
      return DMTX_END_OF_RANGE;

   while(edgeScan->edge.offset != edgeScan->range.lastPos) {
      edgeScan->edge = edgeScan->edgeNext;
      (edgeScan->edgeNext.offset)++;

      // XXX pretty ugly test... should happen as part of structure with no test needed
      if(edgeScan->edge.offset == edgeScan->range.firstPos) {
         pxlOffset = dmtxImageGetOffset(&(decode->image), edgeScan->range.dir,
               edgeScan->range.lineNbr, edgeScan->edge.offset);
         dmtxColorFromPixel(&(edgeScan->edge.color), &(decode->image.pxl[pxlOffset]));
         edgeScan->edge.t = dmtxDistanceAlongRay3(&(gradient->ray), &(edgeScan->edge.color));
      }

      pxlOffset = dmtxImageGetOffset(&(decode->image), edgeScan->range.dir,
            edgeScan->range.lineNbr, edgeScan->edgeNext.offset);
      dmtxColorFromPixel(&(edgeScan->edgeNext.color), &(decode->image.pxl[pxlOffset]));
      edgeScan->edgeNext.t = dmtxDistanceAlongRay3(&(gradient->ray), &(edgeScan->edgeNext.color));

      // XXX This test should not be necessary, but loop boundaries are currently sketchy
      if(dmtxDistanceFromRay3(&(gradient->ray), &(edgeScan->edgeNext.color)) > 10.0)
         return DMTX_END_OF_RANGE;

      lower = gradient->tMid - edgeScan->edge.t;
      upper = gradient->tMid - edgeScan->edgeNext.t;

      // edge and edgeNext have the same color
      if(fabs(lower - upper) < DMTX_ALMOST_ZERO) {
         continue;
      }
      // Boundary color falls between edge and edgeNext
      else if(lower * upper <= 0) {
         edgeScan->subPixelOffset = lower / (lower - upper);

         if(decode && decode->crossScanCallback)
            (*(decode->crossScanCallback))(&(edgeScan->range), gradient, edgeScan);

         return DMTX_SUCCESS;
      }
   }

   if(decode && decode->crossScanCallback)
      (*(decode->crossScanCallback))(&(edgeScan->range), gradient, edgeScan);

   return DMTX_SUCCESS;
}

/**
 * XXX
 *
 * @param follow   XXX
 * @param edgeScan XXX
 * @param gradient XXX
 * @param dir      XXX
 * @return XXX
 */
static void
EdgeFollowerInit(DmtxEdgeFollower *follow, DmtxEdgeScan *edgeScan, DmtxGradient *gradient, DmtxDirection dir)
{
   memset(follow, 0x00, sizeof(DmtxEdgeFollower));

   follow->slope = (edgeScan->edgeNext.t > edgeScan->edge.t) ? 1 : -1;
   follow->paraOffset = edgeScan->range.lineNbr;
   follow->perpOffset = edgeScan->edge.offset + edgeScan->subPixelOffset;
   follow->tMin = gradient->tMin;
   follow->tMid = gradient->tMid;
   follow->tMax = gradient->tMax;
   follow->ray = gradient->ray;
   follow->dir = dir;
}

/**
 * XXX
 *
 * @param follow XXX
 * @return XXX
 */
static DmtxVector2
EdgeFollowerGetLoc(DmtxEdgeFollower *follow)
{
   DmtxVector2 loc;

   if(follow->dir & DmtxDirVertical) {
      loc.X = follow->perpOffset;
      loc.Y = follow->paraOffset;
   }
   else {
      loc.X = follow->paraOffset;
      loc.Y = follow->perpOffset;
   }

   return loc;
}

/**
 * XXX
 *
 * @param follow XXX
 * @param decode XXX
 * @return XXX
 */
static int
EdgeFollowerFollow(DmtxEdgeFollower *follow, DmtxDecode *decode)
{
   // initialize line0 and line1 to "unset" and zero strength
   int i;
   int anchorStrength = 0;
   float offPath, offAngle;
   DmtxVector2 vTmp, anchorStep, fStep[DMTX_FOLLOW_STEPS];
   DmtxRay2 rayAnchor, rayNext;

   memset(&(follow->line0), 0x00, sizeof(DmtxRay2));
   memset(&(follow->line1), 0x00, sizeof(DmtxRay2));

   // Build first vector by incrementing 5 steps
   anchorStep = fStep[0] = EdgeFollowerGetLoc(follow);
   for(i = 1; i < DMTX_FOLLOW_STEPS; i++) {
      if(EdgeFollowerIncrement(follow, decode) != DMTX_EDGE_FOUND) {
         if(decode && decode->plotPointCallback)
            (*(decode->plotPointCallback))(EdgeFollowerGetLoc(follow), 1, 2, DMTX_DISPLAY_SQUARE);
         return 0; // Lost edge before completing first vector
      }
      fStep[i] = EdgeFollowerGetLoc(follow);
   }
   dmtxVector2Sub(&vTmp, &(fStep[DMTX_FOLLOW_STEPS-1]), &(fStep[0]));
   dmtxVector2Norm(&vTmp); // XXX potential problem here if follower ends up in starting point (div/0)
   rayAnchor.p = fStep[0];
   rayAnchor.v = vTmp;

   // Continue while we can still find an edge
   while(EdgeFollowerIncrement(follow, decode) == DMTX_EDGE_FOUND) {

      if(follow->turnCount > 2) { // Reached maximum number of turns

         if(decode && decode->plotPointCallback) // Plot a red X where edge was lost
            (*(decode->plotPointCallback))(fStep[0], 1, 2, DMTX_DISPLAY_SQUARE);

          return 0; // XXX this should probably reflect the edge status (e.g. lost, max_turns, etc...)
      }

      // incrementProgress(follow, anchorVector); (using current followVector)
      for(i = 0; i < DMTX_FOLLOW_STEPS - 1; i++) {
         fStep[i] = fStep[i+1]; // XXX innefficient, but simple
      }
      fStep[i] = EdgeFollowerGetLoc(follow);
      dmtxVector2Sub(&vTmp, &(fStep[DMTX_FOLLOW_STEPS-1]), &(fStep[0]));
      dmtxVector2Norm(&vTmp);
      rayNext.p = fStep[0];
      rayNext.v = vTmp;

      offPath = fabs(dmtxDistanceFromRay2(&rayAnchor, &(fStep[0])));
      offAngle = fabs(dmtxVector2Dot(&(rayAnchor.v), &(rayNext.v)));

      // Step is in direction of predicted path
      if(offAngle > 0.984) {
         anchorStrength++;
      }
      else { // Step is in wrong direction

         // Current line is done -- fixate
         if(anchorStrength > 10) { // if line reached strength threshhold, but is now turning a corner

            // line0 gets set first
            if(follow->line0.isDefined == 0) {

               if(decode && decode->plotPointCallback)
                  (*(decode->plotPointCallback))(fStep[0], 2, 2, DMTX_DISPLAY_SQUARE);

               // Update should build ray based on min/max points on the edge, NOT anchorVector
               // XXXXXXXXX IMPORTANT: Revisit this... results should be adequate using anchorVector here
               follow->line0.isDefined = 1;

               dmtxVector2Sub(&vTmp, &(fStep[0]), &anchorStep);
               dmtxVector2Norm(&vTmp);
               follow->line0.v = vTmp;
               follow->line0.p = anchorStep;

               follow->line0.tMin = 0;
               follow->line0.tMax = dmtxDistanceAlongRay2(&(follow->line0), &(fStep[0]));
            }
            // line1 gets set if line0 is already set
            else {

               if(decode && decode->plotPointCallback)
                  (*(decode->plotPointCallback))(fStep[0], 3, 2, DMTX_DISPLAY_SQUARE);

               // Update should build ray based on min/max points on the edge, NOT anchorVector
               // XXXXXXXXX IMPORTANT: Revisit this... results should be adequate using anchorVector here
               follow->line1.isDefined = 1;

               dmtxVector2Sub(&vTmp, &(fStep[0]), &anchorStep);
               dmtxVector2Norm(&vTmp);
               follow->line1.v = vTmp;
               follow->line1.p = anchorStep;

               follow->line1.tMin = 0;
               follow->line1.tMax = dmtxDistanceAlongRay2(&(follow->line1), &(fStep[0]));

               return 0; // return "full"
            }
         }
         else {
            // line never reached strength threshhold
            if(decode && decode->plotPointCallback)
               (*(decode->plotPointCallback))(fStep[0], 4, 2, DMTX_DISPLAY_SQUARE);
         }

         rayAnchor = rayNext;
         anchorStep = fStep[0];
         anchorStrength = 0;
      }
   }

   // return edge_lost condition (range/right/left)
   if(decode && decode->plotPointCallback)
      (*(decode->plotPointCallback))(EdgeFollowerGetLoc(follow), 5, 2, DMTX_DISPLAY_SQUARE);

   return 0;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
EdgeFollowerIncrement(DmtxEdgeFollower *follow, DmtxDecode *decode)
{
   int turnDir;
   int increment;
   int offset0, offset1, endOffset;
   int dir;
   float t0, t1;
   float tmpOffset;
   DmtxVector3 color0, color1;

   // XXX this first calc of offset0 might be done smarter by extrapolating current "line"
   offset0 = (int)(follow->perpOffset + 0.5);

   if(((follow->dir & DmtxDirHorizontal) && (offset0 + 1 >= decode->image.height - 1)) ||
         ((follow->dir & DmtxDirVertical) && (offset0 + 1 >= decode->image.width - 1)) ||
         offset0 - 1 <= 0)
      return DMTX_END_OF_RANGE;

   increment = (follow->dir & DmtxDirRightUp) ? 1 : -1;
   follow->paraOffset += increment;

   if(((follow->dir & DmtxDirHorizontal) && (follow->paraOffset >= decode->image.width)) ||
         ((follow->dir & DmtxDirVertical) && (follow->paraOffset >= decode->image.height)) ||
         follow->paraOffset <= 0)
      return DMTX_END_OF_RANGE;

   if(follow->dir & DmtxDirVertical)
      dmtxColorFromPixel(&color0, &(decode->image.pxl[decode->image.width * follow->paraOffset + offset0]));
   else
      dmtxColorFromPixel(&color0, &(decode->image.pxl[decode->image.width * offset0 + follow->paraOffset]));

   t0 = dmtxDistanceAlongRay3(&(follow->ray), &color0);

   dir = (follow->slope * (follow->tMid - t0) >= 0) ? 1 : -1;

   // Find adjacent pixels whose values "surround" tMid
   endOffset = offset0 + 3*dir;
   for(offset1 = offset0; offset1 != endOffset; offset1 += dir) {

      if(((follow->dir & DmtxDirHorizontal) && (offset1 + 1 >= decode->image.height - 1)) ||
            ((follow->dir & DmtxDirVertical) && (offset1 + 1 >= decode->image.width - 1)) ||
            offset1 - 1 <= 0)
         return DMTX_END_OF_RANGE;

      if(follow->dir & DmtxDirVertical)
         dmtxColorFromPixel(&color1, &(decode->image.pxl[decode->image.width * follow->paraOffset + offset1]));
      else
         dmtxColorFromPixel(&color1, &(decode->image.pxl[decode->image.width * offset1 + follow->paraOffset]));

      t1 = dmtxDistanceAlongRay3(&(follow->ray), &color1);

      // Check that follower hasn't wandered off of edge following unlike colors
      if(dmtxDistanceFromRay3(&(follow->ray), &color1) > DMTX_MAX_COLOR_DEVN) {
         return DMTX_END_OF_RANGE; // XXX not really DMTX_END_OF_RANGE, but works for now
      }

      if(fabs(t1 - follow->tMid) < DMTX_ALMOST_ZERO ||
            ((follow->tMid - t0) * (follow->tMid - t1) < 0)) {

         if(fabs(t1 - follow->tMid) < DMTX_ALMOST_ZERO) {
            follow->perpOffset = offset1;
         }
         else if(fabs(follow->tMid - t0) < fabs(follow->tMid - t1)) {
            follow->perpOffset = offset0 + follow->slope * ((follow->tMid - t0)/fabs(t0 - t1));
         }
         else {
            follow->perpOffset = offset1 + follow->slope * ((follow->tMid - t1)/fabs(t0 - t1));
         }

         if(decode && decode->followScanCallback)
            (*(decode->followScanCallback))(follow);

         return DMTX_EDGE_FOUND;
      }
      else {
         offset0 = offset1;
         t0 = t1;
      }
   }

   tmpOffset = follow->paraOffset;
   follow->paraOffset = follow->perpOffset;
   follow->perpOffset = tmpOffset;

   if(follow->dir & (DmtxDirUp | DmtxDirLeft))
      turnDir = (dir > 0) ? DMTX_TURN_CW : DMTX_TURN_CCW;
   else
      turnDir = (dir > 0) ? DMTX_TURN_CCW : DMTX_TURN_CW;

   // XXX Whoa, this one was hard to figure out
   if(follow->dir & DmtxDirVertical && turnDir == DMTX_TURN_CW)
      follow->slope *= -1;
   else if(follow->dir & DmtxDirHorizontal && turnDir == DMTX_TURN_CCW)
      follow->slope *= -1;

   follow->turnCount++;
   follow->dir = TurnCorner(follow->dir, turnDir);

   return DMTX_EDGE_FOUND;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static void
MatrixRegionInit(DmtxMatrixRegion *matrixRegion, DmtxGradient *gradient)
{
   memset(matrixRegion, 0x00, sizeof(DmtxMatrixRegion));

   matrixRegion->gradient = *gradient;

   // XXX this stuff just became a lot less important since I'm initializing it locally in "chain" and then copying
   matrixRegion->chain.tx = 0.0;
   matrixRegion->chain.ty = 0.0;
   matrixRegion->chain.phi = 0.0;
   matrixRegion->chain.shx = 0.0;
   matrixRegion->chain.scx = 1.0;
   matrixRegion->chain.scy = 1.0;
   matrixRegion->chain.bx0 = 100.0;
   matrixRegion->chain.bx1 = 100.0;
   matrixRegion->chain.by0 = 100.0;
   matrixRegion->chain.by1 = 100.0;
   matrixRegion->chain.sz = 100.0;

   MatrixRegionUpdateXfrms(matrixRegion);
}

/**
 * XXX
 * TODO: this function should really be static --- move "decode" functions into this file
 *
 * @param
 * @return XXX
 */
extern void
dmtxMatrixRegionDeInit(DmtxMatrixRegion *matrixRegion)
{
   if(matrixRegion->array != NULL)
      free(matrixRegion->array);

   if(matrixRegion->code != NULL)
      free(matrixRegion->code);

   if(matrixRegion->output != NULL)
      free(matrixRegion->output);

   memset(matrixRegion, 0x00, sizeof(DmtxMatrixRegion));
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static void
Matrix3ChainXfrm(DmtxMatrix3 m, DmtxChain *chain)
{
   DmtxMatrix3 mtxy, mphi, mshx, mscxy, msky, mskx;

   dmtxMatrix3Translate(mtxy, chain->tx, chain->ty);
   dmtxMatrix3Rotate(mphi, chain->phi);
   dmtxMatrix3Shear(mshx, chain->shx, 0.0);
   dmtxMatrix3Scale(mscxy, chain->scx * chain->sz, chain->scy * chain->sz);
   dmtxMatrix3LineSkewTop(msky, chain->by0, chain->by1, chain->sz);
   dmtxMatrix3LineSkewSide(mskx, chain->bx0, chain->bx1, chain->sz);

   dmtxMatrix3Multiply(m, mtxy, mphi);
   dmtxMatrix3MultiplyBy(m, mshx);
   dmtxMatrix3MultiplyBy(m, mscxy);
   dmtxMatrix3MultiplyBy(m, msky);
   dmtxMatrix3MultiplyBy(m, mskx);
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static void
Matrix3ChainXfrmInv(DmtxMatrix3 m, DmtxChain *chain)
{
   DmtxMatrix3 mskx, msky, mscxy, mshx, mphi, mtxy;

   dmtxMatrix3LineSkewSideInv(mskx, chain->bx0, chain->bx1, chain->sz);
   dmtxMatrix3LineSkewTopInv(msky, chain->by0, chain->by1, chain->sz);
   dmtxMatrix3Scale(mscxy, 1.0/(chain->scx * chain->sz), 1.0/(chain->scy * chain->sz));
   dmtxMatrix3Shear(mshx, -chain->shx, 0.0);
   dmtxMatrix3Rotate(mphi, -chain->phi);
   dmtxMatrix3Translate(mtxy, -chain->tx, -chain->ty);

   dmtxMatrix3Multiply(m, mskx, msky);
   dmtxMatrix3MultiplyBy(m, mscxy);
   dmtxMatrix3MultiplyBy(m, mshx);
   dmtxMatrix3MultiplyBy(m, mphi);
   dmtxMatrix3MultiplyBy(m, mtxy);
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static void
MatrixRegionUpdateXfrms(DmtxMatrixRegion *matrixRegion)
{
   Matrix3ChainXfrm(matrixRegion->raw2fit, &(matrixRegion->chain));
   Matrix3ChainXfrmInv(matrixRegion->fit2raw, &(matrixRegion->chain));
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionAlignFinderBars(DmtxMatrixRegion *matrixRegion, DmtxDecode *decode,
      DmtxEdgeFollower *f0, DmtxEdgeFollower *f1)
{
   int             success;
   float           v1Length;
   float           v2Length;
   DmtxFinderBar   bar;
   DmtxEdgeFollower *fTmp;
   DmtxRay2        rPrimary, rSecondary;
   DmtxVector2     pMin, pTmp;
   DmtxVector2     v1, v2;
   DmtxMatrix3     m;
   DmtxChain       chain;

/* XXX comment these cases better
count   1st    2nd
         0      0
            return false

         1      1
            colinear?
            yes: decide which is primary (longer secondary)
                 verify other finder bar is co-linear
            no:  finder bars are known

         1      0
         11     0
            primary is known
            verify that secondary is long enough

         0      1
         0     11
            primary is known
            verify that secondary is long enough
*/
   // Ensure that no secondary line is defined unless primary line is also defined
   assert((f0->line1.isDefined) ? f0->line0.isDefined : 1);
   assert((f1->line1.isDefined) ? f1->line0.isDefined : 1);

   // We need at least 2 finder bar candidates to proceed
   if(f0->line0.isDefined + f0->line1.isDefined + f1->line0.isDefined + f1->line1.isDefined < 2) {
      return DMTX_FALSE;
   }
   // One direction contains both possible finder bar candidates (rare)
   else if(f0->line0.isDefined + f1->line0.isDefined == 1) {
      fTmp = (f0->line0.isDefined) ? f0 : f1;
      rPrimary = fTmp->line0;   // Set primary to the defined one
      rSecondary = fTmp->line1; // Set secondary to the defined one's dog-leg
   }
   // Each direction contains a finder bar candidate
   else {
      // Close to -1.0 means colinear (but opposite)
      if(dmtxVector2Dot(&(f0->line0.v), &(f1->line0.v)) < -0.99) {

         if(f0->line1.isDefined * (f0->line1.tMax - f0->line1.tMin) >
               f1->line1.isDefined * (f1->line1.tMax - f1->line1.tMin)) {
            // Primary is combination of both line0's (careful about direction / tMax handling)
            rPrimary = f1->line0;
            dmtxPointAlongRay2(&pMin, &(f0->line0), f0->line0.tMax);
            rPrimary.tMin = dmtxDistanceAlongRay2(&rPrimary, &pMin);
            rSecondary = f0->line1;
         }
         else {
            // Primary is combination of both line0's (careful about direction / tMax handling)
            rPrimary = f0->line0;
            dmtxPointAlongRay2(&pMin, &(f1->line0), f1->line0.tMax);
            rPrimary.tMin = dmtxDistanceAlongRay2(&rPrimary, &pMin);
            rSecondary = f1->line1;
         }
      }
      else {
         // Set primary and secondary to line0's arbitrarily
         rPrimary = f0->line0;
         rSecondary = f1->line0;
      }
   }

   // Check if we have to valid lines
   if(rPrimary.isDefined == 0 || rSecondary.isDefined == 0) {
      return DMTX_FALSE;
   }
   else if(rPrimary.tMax - rPrimary.tMin < 15 || rSecondary.tMax - rSecondary.tMin < 15) {
      return DMTX_FALSE;
   }

   // Can't check lengths yet because length should be calculated from the
   // true intersection of both lines

   // Find intersection of 2 chosen rays
   success = dmtxRay2Intersect(&(bar.p0), &rPrimary, &rSecondary);
   if(!success) {
      return DMTX_FALSE;
   }

   dmtxPointAlongRay2(&(bar.p2), &rPrimary, rPrimary.tMax);
   dmtxPointAlongRay2(&(bar.p1), &rSecondary, rSecondary.tMax);

   dmtxVector2Sub(&v1, &bar.p1, &bar.p0);
   dmtxVector2Sub(&v2, &bar.p2, &bar.p0);

   v1Length = dmtxVector2Mag(&v1);
   v2Length = dmtxVector2Mag(&v2);

   // Check that both lines are at least 10 pixels in length
   if(v1Length < 10 || v2Length < 10) {
//    fprintf(stdout, "Reject: At least one finder bar is too short.\n");
      return DMTX_FALSE;
   }
   // Check matrix is not too "flat"
   else if(min(v1Length, v2Length) / max(v1Length, v2Length) < 0.2) {
//    fprintf(stdout, "Reject: Candidate region is too \"flat\" (%g).\n",
//          min(v1Length, v2Length) / max(v1Length, v2Length));
      return DMTX_FALSE;
   }

   // normalize(dir1) and normalize(dir2)
   dmtxVector2Norm(&v1);
   dmtxVector2Norm(&v2);

   // Check that finder bars are not colinear
   if(fabs(dmtxVector2Dot(&v1, &v2)) > cos(20.0*M_PI/180.0)) {
//    fprintf(stdout, "Reject: Finder bars are too colinear.\n");
      return DMTX_FALSE;
   }

   // XXX This is where we draw the 3 squares on the first pane: Replace with plotPoint callback
   if(decode && decode->plotPointCallback) {
      (*(decode->plotPointCallback))(bar.p0, 1, 1, DMTX_DISPLAY_SQUARE);
      (*(decode->plotPointCallback))(bar.p1, 1, 1, DMTX_DISPLAY_SQUARE);
      (*(decode->plotPointCallback))(bar.p2, 1, 1, DMTX_DISPLAY_SQUARE);
   }

   // Check for clockwise/ccw order of points and flip if necessary
   if(dmtxVector2Cross(&v2, &v1) < 0.0) {
      pTmp = bar.p1;
      bar.p1 = bar.p2;
      bar.p2 = pTmp;
   }

   chain.tx = -bar.p0.X;
   chain.ty = -bar.p0.Y;
   chain.phi = 0.0;
   chain.shx = 0.0;
   chain.scx = chain.scy = 1.0;
   chain.bx0 = chain.bx1 = chain.by0 = chain.by1 = chain.sz = 1.0;

   Matrix3ChainXfrm(m, &chain);
   dmtxMatrix3VMultiply(&pTmp, &bar.p2, m);
   chain.phi = -atan2(pTmp.Y, pTmp.X);

   Matrix3ChainXfrm(m, &chain);
   dmtxMatrix3VMultiply(&pTmp, &bar.p2, m);
   chain.scx = 1.0/pTmp.X;
   dmtxMatrix3VMultiply(&pTmp, &bar.p1, m);
   assert(pTmp.Y > DMTX_ALMOST_ZERO);
   chain.shx = -pTmp.X / pTmp.Y;
   chain.scy = 1.0/pTmp.Y;

   chain.bx0 = chain.bx1 = chain.by0 = chain.by1 = chain.sz = 100.0;

   // Update transformations now that finders are set
   matrixRegion->chain = chain;
   MatrixRegionUpdateXfrms(matrixRegion);

   // Now matrixRegion contains our best guess.  Next tighten it up.
   if(decode && decode->buildMatrixCallback2)
      (*(decode->buildMatrixCallback2))(&bar, matrixRegion);

   return DMTX_TRUE;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionAlignTop(DmtxMatrixRegion *matrixRegion, DmtxDecode *decode)
{
   float t, m;
   float currentCalibGap, maxCalibGap;
   DmtxVector2 p0, px0, pTmp;
   DmtxVector2 calibTopP0, calibTopP1;
   DmtxVector3 color;
   DmtxMatrix3 s, sInv, sReg, sRegInv, m0, m1;
   DmtxVector2 prevHit, prevStep, highHit, highHitX, prevCalibHit;

   dmtxMatrix3LineSkewTop(m0, 100.0, 75.0, 100.0);
   dmtxMatrix3Scale(m1, 1.25, 1.0);
   dmtxMatrix3Multiply(s, m0, m1);
   dmtxMatrix3Multiply(sReg, matrixRegion->raw2fit, s);

   dmtxMatrix3LineSkewTop(m0, 100.0, 400.0/3.0, 100.0);
   dmtxMatrix3Scale(m1, 0.8, 1.0);
   dmtxMatrix3Multiply(sInv, m1, m0);
   dmtxMatrix3Multiply(sRegInv, sInv, matrixRegion->fit2raw);

   if(decode && decode->buildMatrixCallback3)
      (*(decode->buildMatrixCallback3))(sRegInv);

   p0.X = 0.0;
   p0.Y = 100.0;
   prevStep = prevHit = prevCalibHit = p0;
   currentCalibGap = maxCalibGap = 0;

   highHit.X = highHit.Y = 100.0; // XXX add this for safety in case it's not found

   while(p0.X < 100.0 && p0.Y < 300.0) { // XXX cap rise at 300.0 to prevent infinite loops
      dmtxMatrix3VMultiply(&px0, &p0, sRegInv);
      dmtxColorFromImage(&color, &(decode->image), px0.X, px0.Y);
      t = dmtxDistanceAlongRay3(&(matrixRegion->gradient.ray), &color);

//    if(decode && decode->xfrmPlotPointCallback)
//       (*(decode->xfrmPlotPointCallback))(px0, sReg, 4, DMTX_DISPLAY_POINT);

      // Bad notation whereby:
      //    prevStep captures every little step
      //    prevHit captures the most recent edge boundary detection

      // Need to move upward
      if(t > matrixRegion->gradient.tMid) {

         if(p0.X - prevStep.X < DMTX_ALMOST_ZERO) {

            currentCalibGap = p0.X - prevCalibHit.X;
            if(currentCalibGap > maxCalibGap) {
               maxCalibGap = currentCalibGap;

               calibTopP0 = prevCalibHit;
               calibTopP1 = p0;

               dmtxMatrix3VMultiply(&pTmp, &prevCalibHit, sRegInv);
               if(decode && decode->xfrmPlotPointCallback)
                  (*(decode->xfrmPlotPointCallback))(pTmp, sReg, 4, DMTX_DISPLAY_POINT);

               if(decode && decode->xfrmPlotPointCallback)
                  (*(decode->xfrmPlotPointCallback))(px0, sReg, 4, DMTX_DISPLAY_POINT);
            }
            prevCalibHit = p0;
         }

         prevStep = p0;
         p0.Y += 0.1;
      }
      // Need to advance to the right
      else {
         // If higher than previous step
         if(fabs(p0.Y - prevStep.Y) > DMTX_ALMOST_ZERO) {

            // If it has been a while since previous hit
            if(p0.X - prevHit.X >= 3) {
               highHit = p0;
               highHitX = px0; // XXX note: capturing transformed point... not useful except for displaying
            }
            // Recent had a hit
            else {
               prevHit = p0;
            }
         }

         prevStep = p0;
         p0.X += 0.1;
      }
   }

   dmtxMatrix3VMultiplyBy(&prevHit, sInv);
   dmtxMatrix3VMultiplyBy(&highHit, sInv);

   // Store calibration points in raw image coordinate system
   dmtxMatrix3VMultiply(&(matrixRegion->calibTopP0), &calibTopP0, sRegInv);
   dmtxMatrix3VMultiply(&(matrixRegion->calibTopP1), &calibTopP1, sRegInv);

   if(decode && decode->xfrmPlotPointCallback) {
      (*(decode->xfrmPlotPointCallback))(matrixRegion->calibTopP0, sReg, 4, DMTX_DISPLAY_SQUARE);
      (*(decode->xfrmPlotPointCallback))(matrixRegion->calibTopP1, sReg, 4, DMTX_DISPLAY_SQUARE);
   }

   m = (highHit.Y - prevHit.Y)/(highHit.X - prevHit.X);
   matrixRegion->chain.by0 = (m * -prevHit.X) + prevHit.Y;
   matrixRegion->chain.by1 = (m * (100.0 - prevHit.X)) + prevHit.Y;

   if(matrixRegion->chain.by0 < 0 || matrixRegion->chain.by1 < 0) {
      return DMTX_FALSE;
   }

   MatrixRegionUpdateXfrms(matrixRegion);

   if(decode && decode->xfrmPlotPointCallback) {
      (*(decode->xfrmPlotPointCallback))(highHitX, sReg, 4, DMTX_DISPLAY_SQUARE);
   }

   return DMTX_TRUE;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionAlignSide(DmtxMatrixRegion *matrixRegion, DmtxDecode *decode)
{
   float t, m;
   DmtxVector2 p0, px0;
   DmtxVector3 color;
   DmtxMatrix3 s, sInv, sReg, sRegInv, m0, m1;
   DmtxVector2 prevHit, prevStep, highHit, highHitX;

   dmtxMatrix3LineSkewSide(m0, 100.0, 75.0, 100.0);
   dmtxMatrix3Scale(m1, 1.0, 1.25);
   dmtxMatrix3Multiply(s, m0, m1);
   dmtxMatrix3Multiply(sReg, matrixRegion->raw2fit, s);

   dmtxMatrix3LineSkewSide(m0, 100.0, 400.0/3.0, 100.0);
   dmtxMatrix3Scale(m1, 1.0, 0.8);
   dmtxMatrix3Multiply(sInv, m1, m0);
   dmtxMatrix3Multiply(sRegInv, sInv, matrixRegion->fit2raw);

// if(decode && decode->buildMatrixCallback4)
//    (*(decode->buildMatrixCallback4))(sRegInv);

   p0.X = 100.0;
   p0.Y = 0.0;
   prevStep = prevHit = p0;

   highHit.X = highHit.Y = 100.0; // XXX add this for safety in case it's not found

   while(p0.Y < 100.0 && p0.X < 300.0) { // XXX 300.0 caps rise to prevent infinite loops
// XXX infinite loop problem here... don't know why yet
      dmtxMatrix3VMultiply(&px0, &p0, sRegInv);
      dmtxColorFromImage(&color, &(decode->image), px0.X, px0.Y);
      t = dmtxDistanceAlongRay3(&(matrixRegion->gradient.ray), &color);

      if(t >= matrixRegion->gradient.tMid) {
         prevStep = p0;
         p0.X += 0.1;
      }
      else {
         if(fabs(p0.X - prevStep.X) > DMTX_ALMOST_ZERO) {
            if(p0.Y - prevHit.Y < 3) {
               prevHit = p0;

//             if(decode && decode->xfrmPlotPointCallback)
//                (*(decode->xfrmPlotPointCallback))(px0, sReg, 5, DMTX_DISPLAY_POINT);
            }
            else {
               highHit = p0;
               highHitX = px0; // XXX note: capturing transformed point... not useful except for displaying
            }
         }

         prevStep = p0;
         p0.Y += 0.1;
      }
   }

   dmtxMatrix3VMultiplyBy(&prevHit, sInv);
   dmtxMatrix3VMultiplyBy(&highHit, sInv);

   m = (highHit.X - prevHit.X)/(highHit.Y - prevHit.Y);
   matrixRegion->chain.bx0 = (m * -prevHit.Y) + prevHit.X;
   matrixRegion->chain.bx1 = (m * (100.0 - prevHit.Y)) + prevHit.X;

   if(matrixRegion->chain.bx0 < 0 || matrixRegion->chain.bx1 < 0) {
      return DMTX_FALSE;
   }

   MatrixRegionUpdateXfrms(matrixRegion);

// if(decode && decode->xfrmPlotPointCallback)
//    (*(decode->xfrmPlotPointCallback))(highHitX, sReg, 5, DMTX_DISPLAY_SQUARE);

   return DMTX_TRUE;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionEstimateSize(DmtxMatrixRegion *matrixRegion, DmtxDecode *decode)
{
   int matrixSize;
   float gapX;
   DmtxVector2 p0, p1;

   dmtxMatrix3VMultiply(&p0, &(matrixRegion->calibTopP0), matrixRegion->raw2fit);
   dmtxMatrix3VMultiply(&p1, &(matrixRegion->calibTopP1), matrixRegion->raw2fit);

   gapX = 200.0/(p1.X - p0.X);

   // Round to nearest even number
   matrixSize = 2 * (int)(gapX/2.0 + 0.5);

   if(matrixSize < 8)
      matrixSize = 8;

   return matrixSize;
}
