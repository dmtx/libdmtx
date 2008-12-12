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
 * @brief  Read image color at location interpolated between neighboring pixels
 * @param  color
 * @param  image
 * @param  p
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

   dmtxImageGetRgb(image, xInt,   yInt,   pxlLL);
   dmtxImageGetRgb(image, xInt+1, yInt,   pxlLR);
   dmtxImageGetRgb(image, xInt,   yInt+1, pxlUL);
   dmtxImageGetRgb(image, xInt+1, yInt+1, pxlUR);

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
 * @brief  Convert integer-based RGB color to float-based equivalent
 * @param  color
 * @param  rgb
 * @return Pixel color
 */
extern DmtxColor3 *
dmtxColor3FromPixel(DmtxColor3 *color, DmtxRgb rgb)
{
   color->R = (double)rgb[0];
   color->G = (double)rgb[1];
   color->B = (double)rgb[2];

   return color;
}

/**
 * @brief  Convert float-based RGB color to integer-based equivalent
 * @param  rgb
 * @param  color
 * @return void
 */
extern void
dmtxPixelFromColor3(DmtxRgb rgb, DmtxColor3 *color)
{
   rgb[0] = (unsigned char)(color->R + 0.5);
   rgb[1] = (unsigned char)(color->G + 0.5);
   rgb[2] = (unsigned char)(color->B + 0.5);
}

/**
 * @brief  Calculate color at specified point along a gradient
 * @param  ray
 * @param  dist
 * @return Extracted color
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
 * @brief  Add RGB channels of one color to another
 * @param  c1
 * @param  c2
 * @return Color sum
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
 * @brief  Add RGB channels of two colors together
 * @param  cOut
 * @param  c1
 * @param  c2
 * @return Color sum
 */
extern DmtxColor3 *
dmtxColor3Add(DmtxColor3 *cOut, DmtxColor3 *c1, DmtxColor3 *c2)
{
   *cOut = *c1;

   return dmtxColor3AddTo(cOut, c2);
}

/**
 * @brief  Subtract RGB channels of one color from another
 * @param  c1
 * @param  c2
 * @return Color difference
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
 * @brief  Subtract RGB channels between two colors
 * @param  cOut
 * @param  c1
 * @param  c2
 * @return Color difference
 */
extern DmtxColor3 *
dmtxColor3Sub(DmtxColor3 *cOut, DmtxColor3 *c1, DmtxColor3 *c2)
{
   *cOut = *c1;

   return dmtxColor3SubFrom(cOut, c2);
}

/**
 * @brief  Scale RGB channels of a color by factor s
 * @param  c
 * @param  s
 * @return Scaled color
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
 * @brief  Scale RGB channels of a color by factor s
 * @param  cOut
 * @param  c
 * @param  s
 * @return Scaled color
 */
extern DmtxColor3 *
dmtxColor3Scale(DmtxColor3 *cOut, DmtxColor3 *c, double s)
{
   *cOut = *c;

   return dmtxColor3ScaleBy(cOut, s);
}

/**
 * @brief  Calculate cross product between two RGB colors
 * @param  cOut
 * @param  c1
 * @param  c2
 * @return Color cross product
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
 * @brief  Normalize vector defined by a color's RGB channels
 * @param  c
 * @return DmtxPass | DmtxFail
 */
extern double
dmtxColor3Norm(DmtxColor3 *c)
{
   double mag;

   mag = dmtxColor3Mag(c);

   if(mag <= DMTX_ALMOST_ZERO)
      return -1.0;

   dmtxColor3ScaleBy(c, 1/mag);

   return mag;
}

/**
 * @brief  Calculate dot product between two RGB colors
 * @param  c1
 * @param  c2
 * @return Color dot product
 */
extern double
dmtxColor3Dot(DmtxColor3 *c1, DmtxColor3 *c2)
{
   return (c1->R * c2->R) + (c1->G * c2->G) + (c1->B * c2->B);
}

/**
 * @brief  Calculate magnitude squared of vector defined by RGB channels
 * @param  c
 * @return Color vector magnitude
 */
extern double
dmtxColor3MagSquared(DmtxColor3 *c)
{
   return (c->R * c->R + c->G * c->G + c->B * c->B);
}

/**
 * @brief  Calculate magnitude of vector defined by a color's RGB channels
 * @param  c
 * @return Color vector magnitude
 */
extern double
dmtxColor3Mag(DmtxColor3 *c)
{
   return sqrt(dmtxColor3MagSquared(c));
}

/**
 * @brief  Calculate the minimum distance between a color and a gradient
 * @param  r
 * @param  q
 * @return Distance from color ray
 */
extern double
dmtxDistanceFromRay3(DmtxRay3 *r, DmtxColor3 *q)
{
   DmtxColor3 cSubTmp;
   DmtxColor3 cCrossTmp;

   /* Assume that ray has a unit length of 1 */
   assert(fabs(1.0 - dmtxColor3Mag(&(r->c))) <= DMTX_ALMOST_ZERO);

   return dmtxColor3Mag(dmtxColor3Cross(&cCrossTmp, &(r->c), dmtxColor3Sub(&cSubTmp, q, &(r->p))));
}

/**
 * @brief  Calculate the distance that a color falls along a gradient
 * @param  r
 * @param  q
 * @return Distance along color ray
 */
extern double
dmtxDistanceAlongRay3(DmtxRay3 *r, DmtxColor3 *q)
{
   DmtxColor3 cSubTmp;

   /* Assume that ray has a unit length of 1 */
   assert(fabs(1.0 - dmtxColor3Mag(&(r->c))) <= DMTX_ALMOST_ZERO);

   return dmtxColor3Dot(dmtxColor3Sub(&cSubTmp, q, &(r->p)), &(r->c));
}

/**
 * @brief  Extract color from specific distance along a gradient
 * @param  point
 * @param  r
 * @param  t
 * @return DmtxPass | DmtxFail
 */
extern DmtxPassFail
dmtxPointAlongRay3(DmtxColor3 *point, DmtxRay3 *r, double t)
{
   DmtxColor3 cTmp;

   /* Assume that ray has a unit length of 1 */
   assert(fabs(1.0 - dmtxColor3Mag(&(r->c))) <= DMTX_ALMOST_ZERO);

   dmtxColor3Scale(&cTmp, &(r->c), t);
   dmtxColor3Add(point, &(r->p), &cTmp);

   return DmtxPass;
}
