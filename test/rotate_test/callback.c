/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2007, 2008, 2009 Mike Laughton

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

#include <stdlib.h>
#include "rotate_test.h"
#include "callback.h"
#include "display.h"
#include "image.h"
#include "dmtx.h"

/**
 *
 *
 */
void BuildMatrixCallback2(DmtxRegion *region)
{
   int i, j;
   int offset;
   float scale = 1.0/200.0;
   DmtxVector2 point;
   DmtxMatrix3 m0, m1, mInv;
   int rgb[3];

   dmtxMatrix3Translate(m0, -(320 - 200)/2.0, -(320 - 200)/2.0);
   dmtxMatrix3Scale(m1, scale, scale);
   dmtxMatrix3Multiply(mInv, m0, m1);
   dmtxMatrix3MultiplyBy(mInv, region->fit2raw);

   glDisable(GL_TEXTURE_2D);
   glPolygonMode(GL_FRONT, GL_LINE);

   for(i = 0; i < 320; i++) {
      for(j = 0; j < 320; j++) {
         point.X = j;
         point.Y = i;
         dmtxMatrix3VMultiplyBy(&point, mInv);
         dmtxImageGetPixelValue(gImage, (int)(point.X+0.5), (int)(point.Y+0.5), 0, &rgb[0]);
         dmtxImageGetPixelValue(gImage, (int)(point.X+0.5), (int)(point.Y+0.5), 1, &rgb[1]);
         dmtxImageGetPixelValue(gImage, (int)(point.X+0.5), (int)(point.Y+0.5), 2, &rgb[2]);

         offset = (320 * i + j) * 3;
         passTwoPxl[offset + 0] = rgb[0];
         passTwoPxl[offset + 1] = rgb[1];
         passTwoPxl[offset + 2] = rgb[2];
/*       dmtxPixelFromColor3(passTwoPxl[i*320+j], &clr); */
      }
   }

   DrawPane3(NULL, passTwoPxl);

   glViewport(646, 324, 320, 320);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-0.5, 319.5, -0.5, 319.5, -1.0, 10.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glColor3f(0.0, 1.0, 0.0);
   glBegin(GL_QUADS);
   glVertex2f( 60.0,  60.0);
   glVertex2f(260.0,  60.0);
   glVertex2f(260.0, 260.0);
   glVertex2f( 60.0, 260.0);
   glEnd();
}

/**
 *
 *
 */
void BuildMatrixCallback3(DmtxMatrix3 mChainInv)
{
   int i, j;
   int offset;
   float scale = 1.0/100.0;
   DmtxVector2 point;
   DmtxMatrix3 m0, m1, mInv;
   int rgb[3];

   dmtxMatrix3Scale(m0, scale, scale);
   dmtxMatrix3Translate(m1, -(320 - 200)/2.0, -(320 - 200)/2.0);

   dmtxMatrix3Multiply(mInv, m1, m0);
   dmtxMatrix3MultiplyBy(mInv, mChainInv);

   glDisable(GL_TEXTURE_2D);
   glPolygonMode(GL_FRONT, GL_LINE);

   for(i = 0; i < 320; i++) {
      for(j = 0; j < 320; j++) {
         point.X = j;
         point.Y = i;
         dmtxMatrix3VMultiplyBy(&point, mInv);
         dmtxImageGetPixelValue(gImage, (int)(point.X+0.5), (int)(point.Y+0.5), 0, &rgb[0]);
         dmtxImageGetPixelValue(gImage, (int)(point.X+0.5), (int)(point.Y+0.5), 1, &rgb[1]);
         dmtxImageGetPixelValue(gImage, (int)(point.X+0.5), (int)(point.Y+0.5), 2, &rgb[2]);

         offset = (320 * i + j) * 3;
         passTwoPxl[offset + 0] = rgb[0];
         passTwoPxl[offset + 1] = rgb[1];
         passTwoPxl[offset + 2] = rgb[2];
      }
   }

   DrawPane4(NULL, passTwoPxl);

   glViewport(2, 2, 320, 320);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-0.5, 319.5, -0.5, 319.5, -1.0, 10.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glColor3f(0.0, 1.0, 0.0);
   glBegin(GL_QUADS);
   glVertex2f( 60.0,  60.0);
   glVertex2f(160.0,  60.0);
   glVertex2f(160.0, 160.0);
   glVertex2f( 60.0, 160.0);
/**
   glVertex2f( 60.0,  60.0);
   glVertex2f(260.0,  60.0);
   glVertex2f(260.0, 260.0);
   glVertex2f( 60.0, 260.0);
*/
   glEnd();
}

/**
 *
 *
 */
void BuildMatrixCallback4(DmtxMatrix3 mChainInv)
{
   int i, j;
   int offset;
   float scale = 1.0/200.0;
   DmtxVector2 point;
   DmtxMatrix3 m0, m1, mInv;
   int rgb[3];

   dmtxMatrix3Scale(m0, scale, scale);
   dmtxMatrix3Translate(m1, -(320 - 200)/2.0, -(320 - 200)/2.0);

   dmtxMatrix3Multiply(mInv, m1, m0);
   dmtxMatrix3MultiplyBy(mInv, mChainInv);

   glDisable(GL_TEXTURE_2D);
   glPolygonMode(GL_FRONT, GL_LINE);

   for(i = 0; i < 320; i++) {
      for(j = 0; j < 320; j++) {
         point.X = j;
         point.Y = i;
         dmtxMatrix3VMultiplyBy(&point, mInv);
         dmtxImageGetPixelValue(gImage, (int)(point.X+0.5), (int)(point.Y+0.5), 0, &rgb[0]);
         dmtxImageGetPixelValue(gImage, (int)(point.X+0.5), (int)(point.Y+0.5), 1, &rgb[1]);
         dmtxImageGetPixelValue(gImage, (int)(point.X+0.5), (int)(point.Y+0.5), 2, &rgb[2]);

         offset = (320 * i + j) * 3;
         passTwoPxl[offset + 0] = rgb[0];
         passTwoPxl[offset + 1] = rgb[1];
         passTwoPxl[offset + 2] = rgb[2];
      }
   }

   DrawPane5(NULL, passTwoPxl);

   glViewport(324, 2, 320, 320);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-0.5, 319.5, -0.5, 319.5, -1.0, 10.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glColor3f(0.0, 1.0, 0.0);
   glBegin(GL_QUADS);
   glVertex2f( 60.0,  60.0);
   glVertex2f(260.0,  60.0);
   glVertex2f(260.0, 260.0);
   glVertex2f( 60.0, 260.0);
   glEnd();
}

/**
 *
 *
 */
void PlotPointCallback(DmtxPixelLoc loc, int colorInt, int paneNbr, int dispType)
{
   int color;
   DmtxImage *image = NULL;
   DmtxVector2 point;

   point.X = loc.X;
   point.Y = loc.Y;

   switch(paneNbr) {
      case 1:
         glViewport(2, 324, 320, 320);
         break;
      case 2:
         glViewport(324, 324, 320, 320);
/*       image = passOnePxl; */
         break;
      case 3:
         glViewport(646, 324, 320, 320);
         break;
      case 4:
         glViewport(2, 2, 320, 320);
         break;
      case 5:
         glViewport(324, 2, 320, 320);
         break;
      case 6:
         glViewport(646, 2, 320, 320);
         break;
   }

   if(image != NULL) {
      switch(colorInt) {
         case 1:
            color = ColorRed;
            break;
         case 2:
            color = ColorGreen;
            break;
         case 3:
            color = ColorBlue;
            break;
         case 4:
            color = ColorYellow;
            break;
         default:
            color = ColorWhite;
            break;
      }

      plotPoint(image, point.Y, point.X, color);
//    plotPoint(image, point.Y + 1, point.X - 1, color);
//    plotPoint(image, point.Y + 1, point.X + 1, color);
//    plotPoint(image, point.Y - 1, point.X - 1, color);
//    plotPoint(image, point.Y - 1, point.X + 1, color);
   }
   else {
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(-0.5, 319.5, -0.5, 319.5, -1.0, 10.0);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();

      glDisable(GL_TEXTURE_2D);
      glPolygonMode(GL_FRONT, GL_LINE);

      switch(colorInt) {
         case 1:
            glColor3f(1.0, 0.0, 0.0);
            break;
         case 2:
            glColor3f(0.0, 1.0, 0.0);
            break;
         case 3:
            glColor3f(0.0, 0.0, 1.0);
            break;
         case 4:
            glColor3f(1.0, 1.0, 0.0);
            break;
         default:
            glColor3f(1.0, 1.0, 1.0);
            break;
      }

      if(dispType == DMTX_DISPLAY_SQUARE) {
         glBegin(GL_QUADS);
         glVertex2f(point.X - 3, point.Y - 3);
         glVertex2f(point.X + 3, point.Y - 3);
         glVertex2f(point.X + 3, point.Y + 3);
         glVertex2f(point.X - 3, point.Y + 3);
         glEnd();
      }
      else if(dispType == DMTX_DISPLAY_POINT) {
         int x = (int)(point.X + 0.5);
         int y = (int)(point.Y + 0.5);

         glBegin(GL_POINTS);
         glVertex2f(x, y);
         glEnd();
      }
   }
}

/**
 *
 *
 */
void XfrmPlotPointCallback(DmtxVector2 point, DmtxMatrix3 xfrm, int paneNbr, int dispType)
{
   float scale = 100.0;
   DmtxMatrix3 m, m0, m1;

   if(xfrm != NULL) {
      dmtxMatrix3Copy(m, xfrm);
   }
   else {
      dmtxMatrix3Identity(m);
   }

   dmtxMatrix3Scale(m0, scale, scale);
   dmtxMatrix3Translate(m1, (320 - 200)/2.0, (320 - 200)/2.0);
   dmtxMatrix3MultiplyBy(m, m0);
   dmtxMatrix3MultiplyBy(m, m1);

   dmtxMatrix3VMultiplyBy(&point, m);
/* PlotPointCallback(point, 3, paneNbr, dispType); */
}

/**
 *
 *
 */
void PlotModuleCallback(DmtxDecode *info, DmtxRegion *region, int row, int col, int color)
{
   int modSize, halfModsize, padSize;
// float t;

   // Adjust for addition of finder bar
   row++;
   col++;

   halfModsize = (int)(100.0 / (region->mappingCols + 2) + 0.5); // Because 100 == 200/2
   modSize = 2 * halfModsize;
   padSize = (320 - ((region->mappingCols + 2) * modSize))/2;

   // Set for 6th pane
   DrawPaneBorder(645, 1, 322, 322);
   glRasterPos2i(1, 1);

   glPolygonMode(GL_FRONT, GL_FILL);

   // Clamp color to extreme foreground or background color
// t = dmtxDistanceAlongRay3(&(region->gradient.ray), &color);
// t = (t < region->gradient.tMid) ? region->gradient.tMin : region->gradient.tMax;
// dmtxPointAlongRay3(&color, &(region->gradient.ray), t);

   if(color == 1) {
      glColor3f(0.0, 0.0, 0.0);
   }
   else {
      glColor3f(1.0, 1.0, 1.0);
   }

   glBegin(GL_QUADS);
   glVertex2f(modSize*(col+0.5) + padSize - halfModsize, modSize*(row+0.5) + padSize - halfModsize);
   glVertex2f(modSize*(col+0.5) + padSize + halfModsize, modSize*(row+0.5) + padSize - halfModsize);
   glVertex2f(modSize*(col+0.5) + padSize + halfModsize, modSize*(row+0.5) + padSize + halfModsize);
   glVertex2f(modSize*(col+0.5) + padSize - halfModsize, modSize*(row+0.5) + padSize + halfModsize);
   glEnd();
}

/**
 *
 *
 */
void FinalCallback(DmtxDecode *decode, DmtxRegion *region)
{
   int row, col;
   int symbolRows, symbolCols;
   int moduleStatus;
/* DmtxColor3 black = { 0.0, 0.0, 0.0 };
   DmtxColor3 white = { 255.0, 255.0, 255.0 }; */

   symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, region->sizeIdx);
   symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, region->sizeIdx);

   for(row = 0; row < symbolRows; row++) {
      for(col = 0; col < symbolCols; col++) {
/*       moduleStatus = dmtxSymbolModuleStatus(message, region->sizeIdx, row, col); */
         PlotModuleCallback(decode, region, row, col, (moduleStatus & DMTX_MODULE_ON_RGB) ? 1 : 0);
      }
   }
}
