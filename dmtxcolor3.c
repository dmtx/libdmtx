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
 * @file dmtxcolor3.c
 * @brief Color handling
 */

/**
 * @brief
 * @param rgb
 * @param image
 * @param x
 * @param y
 * @return void
 */
extern void
dmtxPixelFromImage(DmtxRgb rgb, DmtxImage *image, int x, int y)
{
   if(x >= 0 && x < image->width && y >= 0 && y < image->height) {
      rgb[0] = image->pxl[y * image->width + x][0];
      rgb[1] = image->pxl[y * image->width + x][1];
      rgb[2] = image->pxl[y * image->width + x][2];
   }
   else {
      rgb[0] = 0;
      rgb[1] = 0;
      rgb[2] = 0;
   }
}

/**
 * @brief
 * @param color
 * @param image
 * @param p
 * @return void
 */
extern void
dmtxColor3FromImage2(DmtxColor3 *color, DmtxImage *image, DmtxVector2 p)
{
   int xInt, yInt;
   double xFrac, yFrac;
   DmtxColor3 clrLL, clrLR, clrUL, clrUR;
   DmtxRgb pxlLL, pxlLR, pxlUL, pxlUR;

/* p = dmtxRemoveLensDistortion(p, image, -0.000003, 0.0); */

   xInt = (int)p.X;
   yInt = (int)p.Y;
   xFrac = p.X - xInt;
   yFrac = p.Y - yInt;

   dmtxPixelFromImage(pxlLL, image, xInt,   yInt);
   dmtxPixelFromImage(pxlLR, image, xInt+1, yInt);
   dmtxPixelFromImage(pxlUL, image, xInt,   yInt+1);
   dmtxPixelFromImage(pxlUR, image, xInt+1, yInt+1);

   dmtxColor3ScaleBy(dmtxColor3FromPixel(&clrLL, pxlLL), (1 - xFrac) * (1 - yFrac));
   dmtxColor3ScaleBy(dmtxColor3FromPixel(&clrLR, pxlLR), xFrac * (1 - yFrac));
   dmtxColor3ScaleBy(dmtxColor3FromPixel(&clrUL, pxlUL), (1 - xFrac) * yFrac);
   dmtxColor3ScaleBy(dmtxColor3FromPixel(&clrUR, pxlUR), xFrac * yFrac);

   *color = clrLL;
   dmtxColor3AddTo(color, &clrLR);
   dmtxColor3AddTo(color, &clrUL);
   dmtxColor3AddTo(color, &clrUR);
}

/**
 *
 *
 */
extern DmtxColor3 *
dmtxColor3FromPixel(DmtxColor3 *color, DmtxRgb rgb)
{
 /*double Y;*/

   /* XXX this probably isn't the appropriate place for color-to-grayscale
      conversion but everyone references this function right now */

   color->R = rgb[0];
   color->G = rgb[1];
   color->B = rgb[2];

 /*Y = 0.3*color->R + 0.59*color->G + 0.11*color->B;
   color->R = color->G = color->B = Y;*/

   return color;
}

/**
 *
 *
 */
extern void
dmtxPixelFromColor3(DmtxRgb rgb, DmtxColor3 *color)
{
   rgb[0] = (int)(color->R + 0.5);
   rgb[1] = (int)(color->G + 0.5);
   rgb[2] = (int)(color->B + 0.5);
}

/**
 *
 *
 */
extern DmtxColor3
dmtxColor3AlongRay3(DmtxRay3 *ray, double dist)
{
   DmtxColor3 color;
   DmtxColor3 cTmp;

   color = ray->p;
   dmtxColor3AddTo(&color, dmtxColor3Scale(&cTmp, &(ray->c), dist));

   return(color);
}

/**
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

/**
 *
 *
 */
extern DmtxColor3 *
dmtxColor3Add(DmtxColor3 *cOut, DmtxColor3 *c1, DmtxColor3 *c2)
{
   *cOut = *c1;

   return dmtxColor3AddTo(cOut, c2);
}

/**
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

/**
 *
 *
 */
extern DmtxColor3 *
dmtxColor3Sub(DmtxColor3 *cOut, DmtxColor3 *c1, DmtxColor3 *c2)
{
   *cOut = *c1;

   return dmtxColor3SubFrom(cOut, c2);
}

/**
 *
 *
 */
extern DmtxColor3 *
dmtxColor3ScaleBy(DmtxColor3 *c, double s)
{
   c->R *= s;
   c->G *= s;
   c->B *= s;

   return c;
}

/**
 *
 *
 */
extern DmtxColor3 *
dmtxColor3Scale(DmtxColor3 *cOut, DmtxColor3 *c, double s)
{
   *cOut = *c;

   return dmtxColor3ScaleBy(cOut, s);
}

/**
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

/**
 *
 *
 */
extern int
dmtxColor3Norm(DmtxColor3 *c)
{
   double mag;

   mag = dmtxColor3Mag(c);

   if(mag < DMTX_ALMOST_ZERO)
      return DMTX_FAILURE;

   dmtxColor3ScaleBy(c, 1/mag);

   return DMTX_SUCCESS;
}

/**
 *
 *
 */
extern double
dmtxColor3Dot(DmtxColor3 *c1, DmtxColor3 *c2)
{
   /* XXX double check that this is right */
   return (c1->R * c2->R) + (c1->G * c2->G) + (c1->B * c2->B);
}

/**
 *
 *
 */
extern double
dmtxColor3Mag(DmtxColor3 *c)
{
   return sqrt(c->R * c->R + c->G * c->G + c->B * c->B);
}

/**
 *
 *
 */
extern double
dmtxDistanceFromRay3(DmtxRay3 *r, DmtxColor3 *q)
{
   DmtxColor3 cSubTmp;
   DmtxColor3 cCrossTmp;

   /* Assume that ray has a unit length of 1 */
   assert(fabs(1.0 - dmtxColor3Mag(&(r->c))) < DMTX_ALMOST_ZERO);

   return dmtxColor3Mag(dmtxColor3Cross(&cCrossTmp, &(r->c), dmtxColor3Sub(&cSubTmp, q, &(r->p))));
}

/**
 *
 *
 */
extern double
dmtxDistanceAlongRay3(DmtxRay3 *r, DmtxColor3 *q)
{
   DmtxColor3 cSubTmp;

   /* Assume that ray has a unit length of 1 */
   assert(fabs(1.0 - dmtxColor3Mag(&(r->c))) < DMTX_ALMOST_ZERO);

   return dmtxColor3Dot(dmtxColor3Sub(&cSubTmp, q, &(r->p)), &(r->c));
}

/**
 *
 *
 */
extern int
dmtxPointAlongRay3(DmtxColor3 *point, DmtxRay3 *r, double t)
{
   DmtxColor3 cTmp;

   /* Assume that ray has a unit length of 1 */
   assert(fabs(1.0 - dmtxColor3Mag(&(r->c))) < DMTX_ALMOST_ZERO);

   dmtxColor3Scale(&cTmp, &(r->c), t);
   dmtxColor3Add(point, &(r->p), &cTmp);

   return DMTX_SUCCESS;
}
