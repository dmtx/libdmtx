/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2010 Mike Laughton

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

Contact: mblaughton@users.sourceforge.net
*/

/* $Id$ */

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

   PopulateZones(dec);
/* PopulateVanishBounds(dec); */

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

#define RETURN_FAIL_IF(C) \
   if(C) { \
      decode2ReleaseCacheMemory(dec); \
      return DmtxFail; \
   }

/**
 *
 *
 */
void
PopulateZones(DmtxDecode2 *dec)
{
   int d, phi;
   int dFull, phiFull;
   int x;
   double phiRad, bucketRad, xComp, yComp;
   DmtxOctantType zone0, zone1, zone2;
   unsigned char *zone;

   zone0 = 0;

   /* Calculate distance of each vanish point and capture zone */
   for(d = 0; d < 64; d++)
   {
      dFull = d - 32;

      for(phi = 0; phi < 128; phi++)
      {
         zone = &(dec->zone[d][phi]);

         phiFull = (dFull < 0) ? phi + 128 : phi;
         assert(phiFull >= 0 && phiFull < 256);

         if(phiFull < 32 || phiFull >= 224)
            zone1 = DmtxOctantTop;
         else if(phiFull < 96)
            zone1 = DmtxOctantLeft;
         else if(phiFull < 160)
            zone1 = DmtxOctantBottom;
         else
            zone1 = DmtxOctantRight;

         if(phiFull < 64)
            zone2 = DmtxOctantTopLeft;
         else if(phiFull < 128)
            zone2 = DmtxOctantBottomLeft;
         else if(phiFull < 192)
            zone2 = DmtxOctantBottomRight;
         else
            zone2 = DmtxOctantTopRight;

         /* Infinity */
         if(dFull == 0)
         {
            *zone = zone2;
         }
         else
         {
            bucketRad = abs(dFull) * (M_PI/96.0);
            x = 32.0/tan(bucketRad);

            if(phiFull == 0 || phiFull == 64 || phiFull == 128 || phiFull == 196)
            {
               *zone = (x < 32.0) ? zone0 : zone1;
            }
            else
            {
               phiRad = phi * (M_PI/128.0);
               xComp = fabs(32.0/cos(phiRad));
               yComp = fabs(32.0/sin(phiRad));

               if(x > max(xComp,yComp))
                  *zone = zone2;
               else if(x > min(xComp,yComp))
                  *zone = zone1;
               else
                  *zone = zone0;
            }
         }
      }
   }
}

/**
 *
 *
 */
/*
void
PopulateVanishBounds(DmtxDecode2 *dec)
{
   DmtxHoughBucket vp;

   vp.val = 0; // Unused

   for(vp.phi = 0; vp.phi < 128; vp.phi++)
   {
      for(vp.d = 0; vp.d < 64; vp.d++)
      {
         dec->corners[d][phi] = GetVanishCorners(vp);
      }
   }
}
*/

/**
 *
 *
 */
/*
DmtxCornerPair
GetVanishCorners(DmtxHoughBucket vp)
{
   int zone, zone1, zone2;
   const int zone0 = 0;

   phiRad = vp.phi * (M_PI/128.0);
   dFull = vp.d - 32;
   phiFull = (dFull < 0) ? phi + 128 : phi;
   assert(phiFull >= 0 && phiFull < 256);

   if(phiFull < 64)
      zone2 = DmtxOctantTopLeft;
   else if(phiFull < 128)
      zone2 = DmtxOctantBottomLeft;
   else if(phiFull < 192)
      zone2 = DmtxOctantBottomRight;
   else
      zone2 = DmtxOctantTopRight;

   // Infinity
   if(dFull == 0)
   {
      GetZoneCornerLoc(&(c0.p), &(c1.p), zone2);
      c0.v = c1.v = norm(phiRad);
      return DmtxPass;
   }

   if(phiFull < 32 || phiFull >= 224)
      zone1 = DmtxOctantTop;
   else if(phiFull < 96)
      zone1 = DmtxOctantLeft;
   else if(phiFull < 160)
      zone1 = DmtxOctantBottom;
   else
      zone1 = DmtxOctantRight;

   zone0 = 0;

   bucketRad = abs(dFull) * (M_PI/96.0);
   l = 32.0/tan(bucketRad);

   if(phiFull == 0 || phiFull == 64 || phiFull == 128 || phiFull == 196)
   {
      zone = (l < 32.0) ? zone0 : zone1;
   }
   else
   {
      xComp = fabs(32.0/cos(phiRad));
      yComp = fabs(32.0/sin(phiRad));

      if(l > max(xComp,yComp))
         zone = zone2;
      else if(l > min(xComp,yComp))
         zone = zone1;
      else
         zone = zone0;
   }

   if(zone == zone0)
       zone = zone1; // XXX for now

   GetZoneCornerLoc(&(c0.p), &(c1.p), zone);

   v.x = l * cos(phiRad); <-- careful... phiRad is 0-128 ?
   v.y = l * sin(phiRad);

   c0.v = norm(sub(v, c0.p);
   c1.v = norm(sub(v, c1.p);

   return DmtxPass;
}
*/

/**
 *
 *
 */
/*
DmtxPassFail
GetZoneCornerLoc(DmtxVector2 *p0, DmtxVector2 *p1, DmtxOctantType zone);
{
   const DmtxVector2 p00 = { 0.0, 0.0 };
   const DmtxVector2 p10 = { 1.0, 0.0 };
   const DmtxVector2 p11 = { 1.0, 1.0 };
   const DmtxVector2 p01 = { 0.0, 1.0 };

   switch(zone)
   {
      case DmtxOctantTop:
         *p0 = p11;
         *p1 = p01;
         break;
      case DmtxOctantLeft:
         *p0 = p01;
         *p1 = p00;
         break;
      case DmtxOctantBottom:
         *p0 = p00;
         *p1 = p10;
         break;
      case DmtxOctantRight:
         *p0 = p10;
         *p1 = p11;
         break;
      case DmtxOctantTopLeft:
      case DmtxOctantBottomRight:
         *p0 = p00;
         *p1 = p11;
         break;
      case DmtxOctantBottomLeft:
      case DmtxOctantTopRight:
         *p0 = p10;
         *p1 = p01;
         break;
      default:
         return DmtxFail;
   }

   return DmtxPass;
}
*/

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
