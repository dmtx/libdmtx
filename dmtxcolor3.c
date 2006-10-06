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

/* $Id: dmtxcolor3.c,v 1.2 2006-10-06 21:34:22 mblaughton Exp $ */

/*
 *
 *
 */
extern void
dmtxColor3FromImage(DmtxColor3 *color, DmtxImage *image, int x, int y)
{
   if(x >= 0 && x < image->width && y >= 0 && y < image->height) {
      dmtxColor3FromPixel(color, &(image->pxl[y * image->width + x]));
   }
   else { // XXX this isn't right.  library shouldn't have color to aid graphical visualization
      color->R =  0.0;
      color->G =  0.0;
      color->B = 30.0;
   }
}

/*
 *
 *
 */
extern void
dmtxColor3FromImage2(DmtxColor3 *color, DmtxImage *image, DmtxVector2 p)
{
   int xInt, yInt;
   float xFrac, yFrac;
   DmtxColor3 clrLL, clrLR, clrUL, clrUR;

   xInt = (int)p.X;
   yInt = (int)p.Y;
   xFrac = p.X - xInt;
   yFrac = p.Y - yInt;

   dmtxColor3FromImage(&clrLL, image, xInt,   yInt);
   dmtxColor3FromImage(&clrLR, image, xInt+1, yInt);
   dmtxColor3FromImage(&clrUL, image, xInt,   yInt+1);
   dmtxColor3FromImage(&clrUR, image, xInt+1, yInt+1);

   dmtxColor3ScaleBy(&clrLL, (1 - xFrac) * (1 - yFrac));
   dmtxColor3ScaleBy(&clrLR, xFrac * (1 - yFrac));
   dmtxColor3ScaleBy(&clrUL, (1 - xFrac) * yFrac);
   dmtxColor3ScaleBy(&clrUR, xFrac * yFrac);

   *color = clrLL;
   dmtxColor3AddTo(color, &clrLR);
   dmtxColor3AddTo(color, &clrUL);
   dmtxColor3AddTo(color, &clrUR);
}

/*
 *
 *
 */
extern void
dmtxColor3FromPixel(DmtxColor3 *color, DmtxPixel *pxl)
{
   color->R = pxl->R;
   color->G = pxl->G;
   color->B = pxl->B;
}

/*
 *
 *
 */
extern void
dmtxPixelFromColor3(DmtxPixel *pxl, DmtxColor3 *color)
{
   pxl->R = (int)(color->R + 0.5);
   pxl->G = (int)(color->G + 0.5);
   pxl->B = (int)(color->B + 0.5);
}

/*
 *
 *
 */
extern DmtxColor3
dmtxColor3AlongRay3(DmtxRay3 *ray, float dist)
{
   DmtxColor3 color;
   DmtxColor3 cTmp;

   color = ray->p;
   dmtxColor3AddTo(&color, dmtxColor3Scale(&cTmp, &(ray->c), dist));

   return(color);
}

/*
 *
 *
 */
extern DmtxColor3 *
dmtxColor3AddTo(DmtxColor3 *c1, DmtxColor3 *c2)
{
   c1->R += c2->R;
   c1->G += c2->G;
   c1->B += c2->B;

   return c1;
}

/*
 *
 *
 */
extern DmtxColor3 *
dmtxColor3Add(DmtxColor3 *cOut, DmtxColor3 *c1, DmtxColor3 *c2)
{
   *cOut = *c1;

   return dmtxColor3AddTo(cOut, c2);
}

/*
 *
 *
 */
extern DmtxColor3 *
dmtxColor3SubFrom(DmtxColor3 *c1, DmtxColor3 *c2)
{
   c1->R -= c2->R;
   c1->G -= c2->G;
   c1->B -= c2->B;

   return c1;
}

/*
 *
 *
 */
extern DmtxColor3 *
dmtxColor3Sub(DmtxColor3 *cOut, DmtxColor3 *c1, DmtxColor3 *c2)
{
   *cOut = *c1;

   return dmtxColor3SubFrom(cOut, c2);
}

/*
 *
 *
 */
extern DmtxColor3 *
dmtxColor3ScaleBy(DmtxColor3 *c, float s)
{
   c->R *= s;
   c->G *= s;
   c->B *= s;

   return c;
}

/*
 *
 *
 */
extern DmtxColor3 *
dmtxColor3Scale(DmtxColor3 *cOut, DmtxColor3 *c, float s)
{
   *cOut = *c;

   return dmtxColor3ScaleBy(cOut, s);
}

/*
 *
 *
 */
extern DmtxColor3 *
dmtxColor3Cross(DmtxColor3 *cOut, DmtxColor3 *c1, DmtxColor3 *c2)
{
   cOut->R = c1->G * c2->B - c2->G * c1->B;
   cOut->G = c2->R * c1->B - c1->R * c2->B;
   cOut->B = c1->R * c2->G - c2->R * c1->G;

   return cOut;
}

/*
 *
 *
 */
extern float
dmtxColor3Norm(DmtxColor3 *c)
{
   double mag;

   mag = dmtxColor3Mag(c);

   if(mag == 0)
      ; // Div/0 Error

   dmtxColor3ScaleBy(c, 1/mag);

   return mag;
}

/*
 *
 *
 */
extern float
dmtxColor3Dot(DmtxColor3 *c1, DmtxColor3 *c2)
{
   // XXX double check that this is right
   return (c1->R * c2->R) + (c1->G * c2->G) + (c1->B * c2->B);
}

/*
 *
 *
 */
extern float
dmtxColor3Mag(DmtxColor3 *c)
{
   return sqrt(c->R * c->R + c->G * c->G + c->B * c->B);
}

/*
 *
 *
 */
extern float
dmtxDistanceFromRay3(DmtxRay3 *r, DmtxColor3 *q)
{
   DmtxColor3 cSubTmp;
   DmtxColor3 cCrossTmp;

   // Assume that ray has a unit length of 1
   assert(fabs(1.0 - dmtxColor3Mag(&(r->c))) < DMTX_ALMOST_ZERO);

   return dmtxColor3Mag(dmtxColor3Cross(&cCrossTmp, &(r->c), dmtxColor3Sub(&cSubTmp, q, &(r->p))));
}

/*
 *
 *
 */
extern float
dmtxDistanceAlongRay3(DmtxRay3 *r, DmtxColor3 *q)
{
   DmtxColor3 cSubTmp;

   // Assume that ray has a unit length of 1
   assert(fabs(1.0 - dmtxColor3Mag(&(r->c))) < DMTX_ALMOST_ZERO);

   return dmtxColor3Dot(dmtxColor3Sub(&cSubTmp, q, &(r->p)), &(r->c));
}

/*
 *
 *
 */
extern int
dmtxPointAlongRay3(DmtxColor3 *point, DmtxRay3 *r, float t)
{
   DmtxColor3 cTmp;

   // Assume that ray has a unit length of 1
   assert(fabs(1.0 - dmtxColor3Mag(&(r->c))) < DMTX_ALMOST_ZERO);

   dmtxColor3Scale(&cTmp, &(r->c), t);
   dmtxColor3Add(point, &(r->p), &cTmp);

   return DMTX_SUCCESS;
}
