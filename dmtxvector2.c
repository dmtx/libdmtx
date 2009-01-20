/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2008, 2009 Mike Laughton

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
 * @file dmtxvector2.c
 * @brief 2D Vector math
 */

/**
 *
 *
 */
extern DmtxVector2 *
dmtxVector2AddTo(DmtxVector2 *v1, const DmtxVector2 *v2)
{
   v1->X += v2->X;
   v1->Y += v2->Y;

   return v1;
}

/**
 *
 *
 */
extern DmtxVector2 *
dmtxVector2Add(DmtxVector2 *vOut, const DmtxVector2 *v1, const DmtxVector2 *v2)
{
   *vOut = *v1;

   return dmtxVector2AddTo(vOut, v2);
}

/**
 *
 *
 */
extern DmtxVector2 *
dmtxVector2SubFrom(DmtxVector2 *v1, const DmtxVector2 *v2)
{
   v1->X -= v2->X;
   v1->Y -= v2->Y;

   return v1;
}

/**
 *
 *
 */
extern DmtxVector2 *
dmtxVector2Sub(DmtxVector2 *vOut, const DmtxVector2 *v1, const DmtxVector2 *v2)
{
   *vOut = *v1;

   return dmtxVector2SubFrom(vOut, v2);
}

/**
 *
 *
 */
extern DmtxVector2 *
dmtxVector2ScaleBy(DmtxVector2 *v, double s)
{
   v->X *= s;
   v->Y *= s;

   return v;
}

/**
 *
 *
 */
extern DmtxVector2 *
dmtxVector2Scale(DmtxVector2 *vOut, const DmtxVector2 *v, double s)
{
   *vOut = *v;

   return dmtxVector2ScaleBy(vOut, s);
}

/**
 *
 *
 */
extern double
dmtxVector2Cross(const DmtxVector2 *v1, const DmtxVector2 *v2)
{
   return (v1->X * v2->Y) - (v1->Y * v2->X);
}

/**
 *
 *
 */
extern double
dmtxVector2Norm(DmtxVector2 *v)
{
   double mag;

   mag = dmtxVector2Mag(v);

   if(mag <= DMTX_ALMOST_ZERO)
      return -1.0;

   dmtxVector2ScaleBy(v, 1/mag);

   return mag;
}

/**
 *
 *
 */
extern double
dmtxVector2Dot(const DmtxVector2 *v1, const DmtxVector2 *v2)
{
   return (v1->X * v2->X) + (v1->Y * v2->Y);
}

/**
 *
 *
 */
extern double
dmtxVector2Mag(const DmtxVector2 *v)
{
   return sqrt(v->X * v->X + v->Y * v->Y);
}

/**
 *
 *
 */
extern double
dmtxDistanceFromRay2(const DmtxRay2 *r, const DmtxVector2 *q)
{
   DmtxVector2 vSubTmp;

   /* Assumes that v is a unit vector */
   assert(fabs(1.0 - dmtxVector2Mag(&(r->v))) <= DMTX_ALMOST_ZERO);

   return dmtxVector2Cross(&(r->v), dmtxVector2Sub(&vSubTmp, q, &(r->p)));
}

/**
 *
 *
 */
extern double
dmtxDistanceAlongRay2(const DmtxRay2 *r, const DmtxVector2 *q)
{
   DmtxVector2 vSubTmp;

/* Assumes that v is a unit vector */
#ifdef DEBUG
   if(fabs(1.0 - dmtxVector2Mag(v)) > DMTX_ALMOST_ZERO) {
      ; /* XXX big error goes here */
   }
#endif

   return dmtxVector2Dot(dmtxVector2Sub(&vSubTmp, q, &(r->p)), &(r->v));
}

/**
 *
 *
 */
extern int
dmtxRay2Intersect(DmtxVector2 *point, const DmtxRay2 *p0, const DmtxRay2 *p1)
{
   double numer, denom;
   DmtxVector2 w;

   denom = dmtxVector2Cross(&(p1->v), &(p0->v));
   if(fabs(denom) <= DMTX_ALMOST_ZERO)
      return DmtxFail;

   dmtxVector2Sub(&w, &(p1->p), &(p0->p));
   numer = dmtxVector2Cross(&(p1->v), &w);

   return dmtxPointAlongRay2(point, p0, numer/denom);
}

/**
 *
 *
 */
extern DmtxPassFail
dmtxPointAlongRay2(DmtxVector2 *point, const DmtxRay2 *r, double t)
{
   DmtxVector2 vTmp;

   /* Ray should always have unit length of 1 */
   assert(fabs(1.0 - dmtxVector2Mag(&(r->v))) <= DMTX_ALMOST_ZERO);

   dmtxVector2Scale(&vTmp, &(r->v), t);
   dmtxVector2Add(point, &(r->p), &vTmp);

   return DmtxPass;
}

/**
 *
 *
 */
extern DmtxVector2
dmtxRemoveLensDistortion(DmtxVector2 point, DmtxImage *img, double k1, double k2)
{
   int width, height;
   double radiusPow2, radiusPow4;
   double factor;
   DmtxVector2 pointShifted;
   DmtxVector2 correctedPoint;

   /* XXX this function can be rewritten using vector math notation */
   width = dmtxImageGetProp(img, DmtxPropScaledWidth);
   height = dmtxImageGetProp(img, DmtxPropScaledHeight);

   pointShifted.X = point.X - width/2.0;
   pointShifted.Y = point.Y - height/2.0;

   radiusPow2 = pointShifted.X * pointShifted.X + pointShifted.Y * pointShifted.Y;
   radiusPow4 = radiusPow2 * radiusPow2;

   factor = 1 + (k1 * radiusPow2) + (k2 * radiusPow4);

   correctedPoint.X = pointShifted.X * factor + width/2.0;
   correctedPoint.Y = pointShifted.Y * factor + height/2.0;

   return correctedPoint;
}
