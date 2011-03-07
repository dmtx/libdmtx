/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2010 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in parent directory for full terms of
 * use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 */

/**
 * \file dmtxdecode2.c
 */

#include <math.h>
#include <assert.h>
#include "../../dmtx.h"
#include "multi_test.h"

/**
 *
 *
 */
DmtxDecode2 *
dmtxDecode2Create(DmtxImage *img)
{
   DmtxDecode2 *dec;

   dec = (DmtxDecode2 *)calloc(1, sizeof(DmtxDecode2));
   if(dec == NULL)
      return NULL;

   PopulateVanishBounds(dec);

   return dec;
}

/**
 *
 *
 */
DmtxPassFail
dmtxDecode2Destroy(DmtxDecode2 **dec)
{
   if(dec == NULL || *dec == NULL)
      return DmtxFail;

   decode2ReleaseCacheMemory(*dec);

   free(*dec);
   *dec = NULL;

   return DmtxPass;
}

#define RETURN_FAIL_IF(c) \
   if(c) { \
      decode2ReleaseCacheMemory(dec); \
      return DmtxFail; \
   }

/**
 *
 *
 */
void
PopulateVanishBounds(DmtxDecode2 *dec)
{
   int d, phi;

   for(phi = 0; phi < 128; phi++)
      for(d = 0; d < 64; d++)
         dec->corners[d][phi] = GetVanishCorners(d, phi);
}

/**
 *
 *
 */
DmtxVanishCorners
GetVanishCorners(int d, int phi)
{
   DmtxVanishCorners vBound;
   DmtxVectorPair locs, dirs;
   int zone;
   int dFull, phiFull;
   double l, phiRad, bucketRad;
   DmtxVector2 v;

   dFull = d - 32;
   phiFull = (dFull < 0) ? phi + 128 : phi;
   assert(phiFull >= 0 && phiFull < 256);
   phiRad = phi * (M_PI/128.0);

   /* Infinity */
   if(dFull == 0)
   {
      zone = GetZone(phiFull, NULL);
      locs = GetZoneCornerLocs(zone);

      dirs.a.X = dirs.b.X = cos(phiRad); /* XXX does phiRad point in this direction, or right angle? */
      dirs.a.Y = dirs.b.Y = sin(phiRad);
   }
   else
   {
      bucketRad = abs(dFull) * (M_PI/96.0);
      l = 32/tan(bucketRad);
      zone = GetZone(phiFull, &l);
      locs = GetZoneCornerLocs(zone);

      v.X = l * cos(phiRad);
      v.Y = l * sin(phiRad); /* XXX remember phiRad may not point in direction you think */

      dmtxVector2Sub(&dirs.a, &v, &locs.a);
      dmtxVector2Sub(&dirs.b, &v, &locs.b);

      dmtxVector2Norm(&dirs.a); /* I think this is necessary */
      dmtxVector2Norm(&dirs.b);
   }

   vBound.zone = zone;
   vBound.lineA.p = locs.a;
   vBound.lineA.v = dirs.a;
   vBound.lineB.p = locs.b;
   vBound.lineB.v = dirs.b;

   return vBound;
}

/**
 *
 *
 */
int
GetZone(int phiFull, double *distance)
{
   int zone0 = 0;
   int zone1, zone2;
   double phiRad, xComp, yComp;

   if(phiFull < 32 || phiFull >= 224)
      zone1 = DmtxOctantTop;
   else if(phiFull < 96)
      zone1 = DmtxOctantLeft;
   else if(phiFull < 160)
      zone1 = DmtxOctantBottom;
   else
      zone1 = DmtxOctantRight;

   /* Orthagonal directions */
   if(phiFull == 0 || phiFull == 64 || phiFull == 128 || phiFull == 196)
      return (distance != NULL && *distance < 32.0) ? zone0 : zone1;

   if(phiFull < 64)
      zone2 = DmtxOctantTopLeft;
   else if(phiFull < 128)
      zone2 = DmtxOctantBottomLeft;
   else if(phiFull < 192)
      zone2 = DmtxOctantBottomRight;
   else
      zone2 = DmtxOctantTopRight;

   /* Non-orthagonal vanishing point at infinity */
   if(distance == NULL)
      return zone2;

   /* Must be a finite non-orthagonal vanishing point */
   phiRad = phiFull * (M_PI/128.0);
   xComp = fabs(32.0/cos(phiRad)); /* remember phiRad may not point in direction you think */
   yComp = fabs(32.0/sin(phiRad));

   if(*distance > max(xComp,yComp))
      return zone2;
   else if(*distance > min(xComp,yComp))
      return zone1;

   return zone0;
}

/**
 *
 *
 */
DmtxVectorPair
GetZoneCornerLocs(DmtxOctantType zone)
{
   const DmtxVector2 p00 = { 0.0, 0.0 }; /* should be { -32.0, -32.0 } ? */
   const DmtxVector2 p10 = { 1.0, 0.0 };
   const DmtxVector2 p11 = { 1.0, 1.0 };
   const DmtxVector2 p01 = { 0.0, 1.0 };
   DmtxVectorPair locs;

   switch(zone)
   {
      case DmtxOctantTop:
         locs.a = p11;
         locs.b = p01;
         break;
      case DmtxOctantLeft:
         locs.a = p01;
         locs.b = p00;
         break;
      case DmtxOctantBottom:
         locs.a = p00;
         locs.b = p10;
         break;
      case DmtxOctantRight:
         locs.a = p10;
         locs.b = p11;
         break;
      case DmtxOctantTopLeft:
      case DmtxOctantBottomRight:
         locs.a = p00;
         locs.b = p11;
         break;
      case DmtxOctantBottomLeft:
      case DmtxOctantTopRight:
      default: /* XXX this feels wrong */
         locs.a = p10;
         locs.b = p01;
         break;
   }

   return locs;
}

/**
 *
 *
 */
DmtxPassFail
dmtxDecode2SetImage(DmtxDecode2 *dec, DmtxImage *img)
{
   if(dec == NULL)
      return DmtxFail;

   dec->image = img;

   /* XXX decide here how big and how small to scale the image, and what level to go to */
   /*     store it in the decode struct */

   /* Free existing buffers if sized incorrectly */
   /* if(buffers are allocated but sized incorrectly) */
   RETURN_FAIL_IF(decode2ReleaseCacheMemory(dec) == DmtxFail);

   /* Allocate new buffers if necessary */
   /* if(buffers are not allocated) */
   dec->sobel = SobelCreate(dec->image);
   RETURN_FAIL_IF(dec->sobel == NULL);

   dec->accel = AccelCreate(dec->sobel);
   RETURN_FAIL_IF(dec->accel == NULL);

   dec->hough = HoughCreate(1,1);
   RETURN_FAIL_IF(dec->hough == NULL);

   /* Necessary to zero out buffers? */

   RETURN_FAIL_IF(SobelPopulate(dec) == DmtxFail);
   RETURN_FAIL_IF(AccelPopulate(dec) == DmtxFail);
   RETURN_FAIL_IF(HoughPopulate(dec) == DmtxFail);

   return DmtxPass;
}

#undef RETURN_FAIL_IF

/**
 *
 *
 */
DmtxPassFail
decode2ReleaseCacheMemory(DmtxDecode2 *dec)
{
   if(dec == NULL)
      return DmtxFail;

   HoughDestroy(&(dec->hough));
   AccelDestroy(&(dec->accel));
   SobelDestroy(&(dec->sobel));

   return DmtxPass;
}
