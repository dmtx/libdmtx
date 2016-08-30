/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2008, 2009 Mike Laughton. All rights reserved.
 * Copyright 2012-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxvector2.c
 * \brief 2D Vector math
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

   if(mag <= DmtxAlmostZero)
      return -1.0; /* XXX this doesn't look clean */

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
   assert(fabs(1.0 - dmtxVector2Mag(&(r->v))) <= DmtxAlmostZero);

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

#ifdef DEBUG
   /* Assumes that v is a unit vector */
   if(fabs(1.0 - dmtxVector2Mag(&(r->v))) > DmtxAlmostZero) {
      ; /* XXX big error goes here */
   }
#endif

   return dmtxVector2Dot(dmtxVector2Sub(&vSubTmp, q, &(r->p)), &(r->v));
}

/**
 *
 *
 */
extern DmtxPassFail
dmtxRay2Intersect(DmtxVector2 *point, const DmtxRay2 *p0, const DmtxRay2 *p1)
{
   double numer, denom;
   DmtxVector2 w;

   denom = dmtxVector2Cross(&(p1->v), &(p0->v));
   if(fabs(denom) <= DmtxAlmostZero)
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
   assert(fabs(1.0 - dmtxVector2Mag(&(r->v))) <= DmtxAlmostZero);

   dmtxVector2Scale(&vTmp, &(r->v), t);
   dmtxVector2Add(point, &(r->p), &vTmp);

   return DmtxPass;
}
