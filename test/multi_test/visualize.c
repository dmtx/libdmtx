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

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_rotozoom.h>
#include "../../dmtx.h"
#include "multi_test.h"

void ValueGridCallback(DmtxValueGrid *valueGrid, int id)
{
   int x, y;
   int imageFlipY;
   int imageHeight;

   imageHeight = dmtxImageGetProp(gState.dmtxImage, DmtxPropHeight);
   imageFlipY = gState.screen->h - (gState.imageLocY + imageHeight) - 1;

   x = gState.localOffsetX - 1; /* not sure why it needs -1 to align */
   y = gState.localOffsetY;

   switch(id) {
      case 0:
         BlitSobelGrid(gState.screen, valueGrid, x, y, CTRL_ROW2_Y, CTRL_COL1_X);
         break;
      case 1:
         BlitSobelGrid(gState.screen, valueGrid, x, y, CTRL_ROW2_Y, CTRL_COL2_X);
         break;
      case 2:
         BlitSobelGrid(gState.screen, valueGrid, x, y, CTRL_ROW2_Y, CTRL_COL3_X);
         break;
      case 3:
         BlitSobelGrid(gState.screen, valueGrid, x, y, CTRL_ROW2_Y, CTRL_COL4_X);
         break;
      case 4:
         BlitSobelGrid(gState.screen, valueGrid, x, y, CTRL_ROW3_Y, CTRL_COL1_X);
         break;
      case 5:
         BlitSobelGrid(gState.screen, valueGrid, x, y, CTRL_ROW3_Y, CTRL_COL2_X);
         break;
      case 6:
         BlitSobelGrid(gState.screen, valueGrid, x, y, CTRL_ROW3_Y, CTRL_COL4_X);
         break;
      case 7:
         BlitSobelGrid(gState.screen, valueGrid, x, y, CTRL_ROW4_Y, CTRL_COL2_X);
         break;
      case 8:
         BlitSobelGrid(gState.screen, valueGrid, x, y, CTRL_ROW4_Y, CTRL_COL3_X);
         break;
      case 9:
         BlitSobelGrid(gState.screen, valueGrid, x, y, CTRL_ROW4_Y, CTRL_COL4_X);
         break;
      default:
         break;
   }
}

void ZeroCrossingCallback(ZeroCrossing zXing, int id)
{
   int xInt, yInt;

   if(zXing.mag < 50)
      return;

   xInt = gState.imageOffsetX + (int)zXing.x;
   yInt = gState.screen->h - (gState.imageOffsetY + (int)zXing.y) - 1;

   if(xInt < 0 || xInt > CTRL_COL1_X - 2 || yInt < 0 || yInt > gState.screen->h - 1)
      return;

   switch(id) {
      case 0:
         PutPixel(gState.screen, xInt, yInt, 0x00ff0000);
         break;
      default:
         break;
   }
}

void HoughLocalCallback(DmtxHoughLocal *hough, int id)
{
   switch(id) {
      case 0:
         BlitHoughLocal(gState.screen, hough, CTRL_ROW5_Y, CTRL_COL1_X + 1);
         break;
      case 1:
         BlitHoughLocal(gState.screen, hough, CTRL_ROW6_Y, CTRL_COL1_X + 1);
         break;
   }
}

void VanishPointCallback(VanishPointSort *vPoints, int id)
{
   if(gState.displayVanish == DmtxFalse)
      return;

   switch(id) {
      case 0:
         DrawVanishingPoints(gState.screen, vPoints, CTRL_ROW5_Y, CTRL_COL1_X);
         break;
      case 1:
/*       DrawVanishingPoints(gState.screen, vPoints, CTRL_ROW5_Y, CTRL_COL1_X); */
         break;
   }
}

void TimingCallback(Timing *timing0, Timing *timing1, int id)
{
   int i;
   int rowNum;

   rowNum = (id == 0) ? CTRL_ROW6_Y : CTRL_ROW5_Y;

   /* Should this be called before, as soon as local is captured? */
   BlitActiveRegion(gState.screen, gState.local, 2, CTRL_ROW5_Y, CTRL_COL3_X);

   if(gState.displayTiming == DmtxFalse)
      return;

   /* Draw timed and untimed region lines */
   if(gState.displayTiming == DmtxTrue) {
      DrawTimingDots(gState.screen, timing0, rowNum, CTRL_COL1_X);
      DrawTimingDots(gState.screen, timing1, rowNum, CTRL_COL1_X);

      for(i = -64; i <= 64; i++) {
         DrawLine(gState.screen, 64, CTRL_COL3_X, CTRL_ROW5_Y, timing0->phi,
               timing0->shift + (timing0->period * i), 2, 0xff0000ff);
         DrawLine(gState.screen, 64, CTRL_COL3_X, CTRL_ROW5_Y, timing1->phi,
               timing1->shift + (timing1->period * i), 2, 0xff0000ff);
      }
   }
}

void GridCallback(AlignmentGrid *grid, int id)
{
   switch(id) {
      case 0:
         DrawSymbolPreview(gState.screen, gState.imgFull, grid, &gState,
               CTRL_ROW5_Y, CTRL_COL1_X + 1);
         break;
      case 1:
         DrawNormalizedRegion(gState.screen, gState.imgFull, grid, &gState,
               CTRL_ROW5_Y, CTRL_COL3_X);
         break;
   }
}

void PerimeterCallback(GridRegion *region, DmtxDirection side, DmtxBarType type)
{
   DrawPerimeterPatterns(gState.screen, region, &gState, side, type);
}

/******************************************************************************/

void ShowActiveRegion(SDL_Surface *screen, SDL_Surface *active)
{
   BlitActiveRegion(screen, active, 1, CTRL_ROW1_Y, CTRL_COL1_X);
   BlitActiveRegion(screen, active, 1, CTRL_ROW1_Y, CTRL_COL2_X);
   BlitActiveRegion(screen, active, 1, CTRL_ROW1_Y, CTRL_COL3_X);
   BlitActiveRegion(screen, active, 1, CTRL_ROW1_Y, CTRL_COL4_X);
}

/**
 *
 *
 */
void BlitActiveRegion(SDL_Surface *screen, SDL_Surface *active, int zoom, int screenY, int screenX)
{
   SDL_Surface *src;
   SDL_Rect clipRect;

   clipRect.w = LOCAL_SIZE;
   clipRect.h = LOCAL_SIZE;
   clipRect.x = screenX;
   clipRect.y = screenY;

   if(zoom == 1) {
      SDL_BlitSurface(active, NULL, screen, &clipRect);
   }
   else {
      /* DO NOT USE SMOOTHING OPTION -- distorts symbol proportions */
      src = zoomSurface(active, 2.0, 2.0, 0 /* smoothing */);
      SDL_BlitSurface(src, NULL, screen, &clipRect);
      SDL_FreeSurface(src);
   }
}

/**
 *
 *
 */
void BlitSobelGrid(SDL_Surface *screen, DmtxValueGrid *cache, int x, int y, int screenY, int screenX)
{
   int row, col;
   unsigned char rgb[3];
   int width, height;
   int offset;
   int flow;
   unsigned char pixbuf[12288]; /* 64 * 64 * 3 */
   int maxFlowMag = 1020;

   SDL_Surface *surface;
   SDL_Rect clipRect;
   Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
   rmask = 0xff000000;
   gmask = 0x00ff0000;
   bmask = 0x0000ff00;
   amask = 0x000000ff;
#else
   rmask = 0x000000ff;
   gmask = 0x0000ff00;
   bmask = 0x00ff0000;
   amask = 0xff000000;
#endif

   width = height = 64;

   for(row = 0; row < 64; row++)
   {
      for(col = 0; col < 64; col++)
      {
         flow = dmtxValueGridGetValue(cache, col + x, row + y);
         if(flow > 0) {
            rgb[0] = 0;
            rgb[1] = (int)((abs(flow) * 254.0)/maxFlowMag + 0.5);
            rgb[2] = 0;
         }
         else {
            rgb[0] = (int)((abs(flow) * 254.0)/maxFlowMag + 0.5);
            rgb[1] = 0;
            rgb[2] = 0;
         }

         offset = ((height - row - 1) * width + col) * 3;
         pixbuf[offset] = rgb[0];
         pixbuf[offset+1] = rgb[1];
         pixbuf[offset+2] = rgb[2];
      }
   }

   clipRect.w = LOCAL_SIZE;
   clipRect.h = LOCAL_SIZE;
   clipRect.x = screenX;
   clipRect.y = screenY;

   surface = SDL_CreateRGBSurfaceFrom(pixbuf, width, height, 24, width * 3,
         rmask, gmask, bmask, 0);

   SDL_BlitSurface(surface, NULL, screen, &clipRect);
   SDL_FreeSurface(surface);
}

/**
 *
 *
 */
void BlitHoughLocal(SDL_Surface *screen, DmtxHoughLocal *hough, int screenY, int screenX)
{
   int row, col;
   int width, height;
   int maxVal;
   int rgb[3];
   unsigned int cache;
   int offset;
   unsigned char pixbuf[24576]; /* 128 * 64 * 3 */
   SDL_Surface *surface;
   SDL_Rect clipRect;
   Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
   rmask = 0xff000000;
   gmask = 0x00ff0000;
   bmask = 0x0000ff00;
   amask = 0x000000ff;
#else
   rmask = 0x000000ff;
   gmask = 0x0000ff00;
   bmask = 0x00ff0000;
   amask = 0xff000000;
#endif

   width = 128;
   height = LOCAL_SIZE;

   maxVal = 0;
   for(row = 0; row < height; row++)
   {
      for(col = 0; col < width; col++)
      {
         if(hough->bucket[row][col] > maxVal)
            maxVal = hough->bucket[row][col];
      }
   }

   for(row = 0; row < height; row++)
   {
      for(col = 0; col < width; col++)
      {
         cache = hough->bucket[row][col];

         rgb[0] = rgb[1] = rgb[2] = (int)((cache * 254.0)/maxVal + 0.5);

         offset = ((height - row - 1) * width + col) * 3;
         pixbuf[offset] = rgb[0];
         pixbuf[offset+1] = rgb[1];
         pixbuf[offset+2] = rgb[2];
      }
   }

   clipRect.w = width;
   clipRect.h = height;
   clipRect.x = screenX;
   clipRect.y = screenY;

   surface = SDL_CreateRGBSurfaceFrom(pixbuf, width, height, 24, width * 3,
         rmask, gmask, bmask, 0);

   SDL_BlitSurface(surface, NULL, screen, &clipRect);
   SDL_FreeSurface(surface);
}

/**
 *
 *
 */
Uint32
GetPixel(SDL_Surface *surface, int x, int y)
{
   int bpp = surface->format->BytesPerPixel;
   Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

   switch(bpp) {
      case 1:
         return *p;
      case 2:
         return *(Uint16 *)p;
      case 3:
         if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return (p[0] << 16) | (p[1] << 8) | p[2];
         else
            return p[0] | (p[1] << 8) | (p[2] << 16);
      case 4:
         return *(Uint32 *)p;
      default:
         return 0;
   }
}

void
PutPixel(SDL_Surface *surface, int x, int y, Uint32 color)
{
   int bpp = surface->format->BytesPerPixel;
   Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

   switch(bpp) {
   case 1:
      *p = color;
      break;
   case 2:
      *(Uint16 *)p = color;
      break;
   case 3:
      if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
         p[0] = (color >> 16) & 0xff;
         p[1] = (color >> 8) & 0xff;
         p[2] = color & 0xff;
      }
      else {
         p[0] = color & 0xff;
         p[1] = (color >> 8) & 0xff;
         p[2] = (color >> 16) & 0xff;
      }
      break;
   case 4:
      *(Uint32 *)p = color;
      break;
   }
}

/**
 *
 *
 */
int RayIntersect(double *t, DmtxRay2 p0, DmtxRay2 p1)
{
   double numer, denom;
   DmtxVector2 w;

   denom = dmtxVector2Cross(&(p1.v), &(p0.v));
   if(fabs(denom) <= 0.000001)
      return DmtxFail;

   dmtxVector2Sub(&w, &(p1.p), &(p0.p));
   numer = dmtxVector2Cross(&(p1.v), &w);

   *t = numer/denom;

   return DmtxTrue;
}

/**
 *
 *
 */
int IntersectBox(DmtxRay2 ray, DmtxVector2 bb0, DmtxVector2 bb1, DmtxVector2 *p0, DmtxVector2 *p1)
{
   double tTmp, xMin, xMax, yMin, yMax;
   DmtxVector2 p[2];
   int tCount = 0;
   double extent;
   DmtxRay2 rBtm, rTop, rLft, rRgt;
   DmtxVector2 unitX = { 1.0, 0.0 };
   DmtxVector2 unitY = { 0.0, 1.0 };

   if(bb0.X < bb1.X) {
      xMin = bb0.X;
      xMax = bb1.X;
   }
   else {
      xMin = bb1.X;
      xMax = bb0.X;
   }

   if(bb0.Y < bb1.Y) {
      yMin = bb0.Y;
      yMax = bb1.Y;
   }
   else {
      yMin = bb1.Y;
      yMax = bb0.Y;
   }

   extent = xMax - xMin;

   rBtm.p.X = rTop.p.X = rLft.p.X = xMin;
   rRgt.p.X = xMax;

   rBtm.p.Y = rLft.p.Y = rRgt.p.Y = yMin;
   rTop.p.Y = yMax;

   rBtm.v = rTop.v = unitX;
   rLft.v = rRgt.v = unitY;

   if(RayIntersect(&tTmp, rBtm, ray) == DmtxPass && tTmp >= 0.0 && tTmp < extent)
      dmtxPointAlongRay2(&(p[tCount++]), &rBtm, tTmp);

   if(RayIntersect(&tTmp, rTop, ray) == DmtxPass && tTmp >= 0.0 && tTmp < extent)
      dmtxPointAlongRay2(&(p[tCount++]), &rTop, tTmp);

   if(RayIntersect(&tTmp, rLft, ray) == DmtxPass && tTmp >= 0.0 && tTmp < extent)
      dmtxPointAlongRay2(&(p[tCount++]), &rLft, tTmp);

   if(RayIntersect(&tTmp, rRgt, ray) == DmtxPass && tTmp >= 0.0 && tTmp < extent)
      dmtxPointAlongRay2(&(p[tCount++]), &rRgt, tTmp);

   if(tCount != 2)
      return DmtxFail;

   *p0 = p[0];
   *p1 = p[1];

   return DmtxTrue;
}

/**
 *
 *
 */
void DrawActiveBorder(SDL_Surface *screen, int activeExtent)
{
   Sint16 x00, y00;
   Sint16 x10, y10;
   Sint16 x11, y11;
   Sint16 x01, y01;

   x01 = 158;
   y01 = 226;

   x00 = x01;
   y00 = y01 + activeExtent + 1;

   x10 = x00 + activeExtent + 1;
   y10 = y00;

   x11 = x10;
   y11 = y01;

   lineColor(screen, x00, y00, x10, y10, 0x0000ffff);
   lineColor(screen, x10, y10, x11, y11, 0x0000ffff);
   lineColor(screen, x11, y11, x01, y01, 0x0000ffff);
   lineColor(screen, x01, y01, x00, y00, 0x0000ffff);
}

/**
 *
 *
 */
void DrawLine(SDL_Surface *screen, int baseExtent, int screenX, int screenY,
      int phi, double d, int displayScale, Uint32 color)
{
   int scaledExtent;
   DmtxVector2 bb0, bb1;
   DmtxVector2 p0, p1;
   DmtxRay2 rLine;
   DmtxPixelLoc d0, d1;

   scaledExtent = baseExtent * displayScale;
   bb0.X = bb0.Y = 0.0;
   bb1.X = bb1.Y = scaledExtent - 1;

   rLine = HoughCompactToRay(phi, d);
   dmtxVector2ScaleBy(&rLine.p, (double)displayScale);

   p0.X = p0.Y = p1.X = p1.Y = 0.0;

   if(IntersectBox(rLine, bb0, bb1, &p0, &p1) == DmtxFalse)
      return;

   d0.X = (int)(p0.X + 0.5) + screenX;
   d1.X = (int)(p1.X + 0.5) + screenX;

   d0.Y = screenY + (scaledExtent - (int)(p0.Y + 0.5) - 1);
   d1.Y = screenY + (scaledExtent - (int)(p1.Y + 0.5) - 1);

   lineColor(screen, d0.X, d0.Y, d1.X, d1.Y, color);
}

/**
 *
 *
 */
void DrawVanishingPoints(SDL_Surface *screen, VanishPointSort *sort, int screenY, int screenX)
{
   int sortIdx;
   DmtxPixelLoc d0, d1;
   Uint32 rgba;

   for(sortIdx = 0; sortIdx < sort->count; sortIdx++) {
      d0.X = d1.X = screenX + sort->vanishSum[sortIdx].phi;
      d0.Y = screenY;
      d1.Y = d0.Y + 64;

      if(sortIdx < 2)
         rgba = 0xff0000ff;
      else if(sortIdx < 4)
         rgba = 0x007700ff;
      else if(sortIdx < 6)
         rgba = 0x000077ff;
      else
         rgba = 0x000000ff;

      lineColor(screen, d0.X, d0.Y, d1.X, d1.Y, rgba);
   }
}

/**
 *
 *
 */
void DrawTimingDots(SDL_Surface *screen, Timing *timing, int screenY, int screenX)
{
   int i, d;

   for(i = 0; i < 64; i++) {
      d = (int)(i * timing->period + timing->shift);
      if(d >= 64)
         break;

      PutPixel(screen, screenX + timing->phi, screenY + 63 - d, 0x00ff0000);
   }
}

/**
 *
 *
 */
void DrawNormalizedRegion(SDL_Surface *screen, DmtxImage *img,
      AlignmentGrid *region, AppState *state, int screenY, int screenX)
{
   unsigned char pixbuf[49152]; /* 128 * 128 * 3 */
   unsigned char *ptrFit, *ptrRaw;
   SDL_Rect clipRect;
   SDL_Surface *surface;
   Uint32 rmask, gmask, bmask, amask;
   int x, yImage, yDmtx;
   int xRaw, yRaw;
   int extent = 128;
   int modulesToDisplay = 16;
   int dispModExtent = extent/modulesToDisplay;
   int bytesPerRow = extent * 3;
   DmtxVector2 pFit, pRaw, pRawActive, pTmp, pCtr;
   DmtxVector2 gridTest;
   int shiftX, shiftY;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
   rmask = 0xff000000;
   gmask = 0x00ff0000;
   bmask = 0x0000ff00;
   amask = 0x000000ff;
#else
   rmask = 0x000000ff;
   gmask = 0x0000ff00;
   bmask = 0x00ff0000;
   amask = 0xff000000;
#endif

   SDL_LockSurface(state->picture);

   pTmp.X = pTmp.Y = state->activeExtent/2.0;
   dmtxMatrix3VMultiply(&pCtr, &pTmp, region->raw2fitActive);

   for(yImage = 0; yImage < extent; yImage++) {
      for(x = 0; x < extent; x++) {

         yDmtx = (extent - 1) - yImage;

         /* Adjust fitted input so unfitted center is display centered */
         pFit.X = ((x-extent/2) * (double)modulesToDisplay) /
               (region->colCount * extent) + pCtr.X;

         pFit.Y = ((yDmtx-extent/2) * (double)modulesToDisplay) /
               (region->rowCount * extent) + pCtr.Y;

         dmtxMatrix3VMultiply(&pRaw, &pFit, region->fit2rawFull);
         dmtxMatrix3VMultiply(&pRawActive, &pFit, region->fit2rawActive);

         xRaw = (pRaw.X >= 0.0) ? (int)(pRaw.X + 0.5) : (int)(pRaw.X - 0.5);
         yRaw = (pRaw.Y >= 0.0) ? (int)(pRaw.Y + 0.5) : (int)(pRaw.Y - 0.5);

         ptrFit = pixbuf + (yImage * bytesPerRow + x * 3);
         if(xRaw < 0 || xRaw >= img->width || yRaw < 0 || yRaw >= img->height) {
            ptrFit[0] = 0;
            ptrFit[1] = 0;
            ptrFit[2] = 0;
         }
         else {
            ptrRaw = (unsigned char *)img->pxl + dmtxImageGetByteOffset(img, xRaw, yRaw);

            if(pRawActive.X < 0.0 || pRawActive.X >= state->activeExtent - 1 ||
                  pRawActive.Y < 0.0 || pRawActive.Y >= state->activeExtent - 1) {
               ptrFit[0] = ptrRaw[0]/2;
               ptrFit[1] = ptrRaw[1]/2;
               ptrFit[2] = ptrRaw[2]/2;
            }
            else {
               ptrFit[0] = ptrRaw[0];
               ptrFit[1] = ptrRaw[1];
               ptrFit[2] = ptrRaw[2];
            }
         }
      }
   }

   gridTest.X = pCtr.X * region->colCount * dispModExtent;
   gridTest.X += (gridTest.X >= 0.0) ? 0.5 : -0.5;
   shiftX = (int)gridTest.X % dispModExtent;

   gridTest.Y = pCtr.Y * region->rowCount * dispModExtent;
   gridTest.Y += (gridTest.Y >= 0.0) ? 0.5 : -0.5;
   shiftY = (int)gridTest.Y % dispModExtent;

   for(yImage = 0; yImage < extent; yImage++) {

      yDmtx = (extent - 1) - yImage;

      for(x = 0; x < extent; x++) {

         if((yDmtx + shiftY) % dispModExtent != 0 &&
               (x + shiftX) % dispModExtent != 0)
            continue;

         ptrFit = pixbuf + (yImage * bytesPerRow + x * 3);

         /* If reeaally dark then add brightened grid lines */
         if(ptrFit[0] + ptrFit[1] + ptrFit[2] < 60) {
            ptrFit[0] += (255 - ptrFit[0])/8;
            ptrFit[1] += (255 - ptrFit[1])/8;
            ptrFit[2] += (255 - ptrFit[2])/8;
         }
         /* Otherwise add darkened grid lines */
         else {
            ptrFit[0] = (ptrFit[0] * 8)/10;
            ptrFit[1] = (ptrFit[1] * 8)/10;
            ptrFit[2] = (ptrFit[2] * 8)/10;
         }
      }
   }

   clipRect.w = extent;
   clipRect.h = extent;
   clipRect.x = screenX;
   clipRect.y = screenY;

   surface = SDL_CreateRGBSurfaceFrom(pixbuf, extent, extent, 24, extent * 3,
         rmask, gmask, bmask, 0);

   SDL_BlitSurface(surface, NULL, screen, &clipRect);
   SDL_FreeSurface(surface);

   SDL_UnlockSurface(state->picture);
}

/**
 *
 *
 */
Sint16 Clamp(Sint16 x, Sint16 xMin, Sint16 extent)
{
   Sint16 xMax;

   if(x < xMin)
      return xMin;

   xMax = xMin + extent - 1;
   if(x > xMax)
      return xMax;

   return x;
}

/**
 *
 *
 */
void DrawSymbolPreview(SDL_Surface *screen, DmtxImage *img, AlignmentGrid *grid,
      AppState *state, int screenY, int screenX)
{
   DmtxVector2 pTmp, pCtr;
   DmtxVector2 gridTest;
   int shiftX /*, shiftY*/;
/* int rColor, gColor, bColor, color; */
/* Sint16 x1, y1, x2, y2; */
   int extent = 128;
   int modulesToDisplay = 16;
   int dispModExtent = extent/modulesToDisplay;
/* int row, col; */
   int /*rowBeg,*/ colBeg;
   int /*rowEnd,*/ colEnd;

   SDL_LockSurface(state->picture);

   pTmp.X = pTmp.Y = state->activeExtent/2.0;
   dmtxMatrix3VMultiply(&pCtr, &pTmp, grid->raw2fitActive);

   gridTest.X = pCtr.X * grid->colCount * dispModExtent;
   gridTest.X += (gridTest.X >= 0.0) ? 0.5 : -0.5;
   shiftX = 64 - (int)gridTest.X;
   colBeg = (shiftX < 0) ? 0 : -shiftX/8 - 1;
   colEnd = max(colBeg + 17, grid->colCount);

/* something is uninitialized ... verify in cygwin
   fprintf(stdout, "colBeg:%d colEnd:%d\n", colBeg, colEnd); fflush(stdout);

   gridTest.Y = pCtr.Y * grid->rowCount * dispModExtent;
   gridTest.Y += (gridTest.Y >= 0.0) ? 0.5 : -0.5;
   shiftY = 64 - (int)gridTest.Y;
   rowBeg = (shiftY < 0) ? 0 : -shiftY/8 - 1;
   rowEnd = max(rowBeg + 17, grid->rowCount);

   for(row = rowBeg; row < rowEnd; row++) {

      y1 = row * dispModExtent + shiftY;
      y2 = y1 + dispModExtent - 1;

      y1 = (extent - 1 - y1);
      y2 = (extent - 1 - y2);

      y1 = Clamp(y1 + screenY, screenY, 128);
      y2 = Clamp(y2 + screenY, screenY, 128);

      for(col = colBeg; col < colEnd; col++) {
         rColor = dmtxReadModuleColor(img, grid, row, col, 0);
         gColor = dmtxReadModuleColor(img, grid, row, col, 1);
         bColor = dmtxReadModuleColor(img, grid, row, col, 2);
         color = (rColor << 24) | (gColor << 16) | (bColor << 8) | 0xff;

         x1 = col * dispModExtent + shiftX;
         x2 = x1 + dispModExtent - 1;

         x1 = Clamp(x1 + screenX, screenX, 128);
         x2 = Clamp(x2 + screenX, screenX, 128);

         boxColor(screen, x1, y1, x2, y2, color);
      }
   }
*/

   SDL_UnlockSurface(state->picture);
}

/**
 *
 *
 */
void DrawPerimeterPatterns(SDL_Surface *screen, GridRegion *region,
      AppState *state, DmtxDirection side, DmtxBarType type)
{
   DmtxVector2 pTmp, pCtr;
   DmtxVector2 gridTest;
   int shiftX, shiftY;
   int extent = 128;
   int modulesToDisplay = 16;
   int dispModExtent = extent/modulesToDisplay;
   Sint16 x00, y00;
   Sint16 x11, y11;

   pTmp.X = pTmp.Y = state->activeExtent/2.0;
   dmtxMatrix3VMultiply(&pCtr, &pTmp, region->grid.raw2fitActive);

   gridTest.X = pCtr.X * region->grid.colCount * dispModExtent;
   gridTest.X += (gridTest.X >= 0.0) ? 0.5 : -0.5;
   shiftX = 64 - (int)gridTest.X;

   gridTest.Y = pCtr.Y * region->grid.rowCount * dispModExtent;
   gridTest.Y += (gridTest.Y >= 0.0) ? 0.5 : -0.5;
   shiftY = 63 - (int)gridTest.Y;

   /* Calculate corner positions */
   x00 = region->x * dispModExtent + shiftX;
   y00 = region->y * dispModExtent + shiftY;
   x11 = x00 + region->width * dispModExtent;
   y11 = y00 + region->height * dispModExtent;

   DrawPerimeterSide(screen, x00, y00, x11, y11, dispModExtent, side, type);
}

/**
 *
 *
 */
void DrawPerimeterSide(SDL_Surface *screen, int x00, int y00, int x11, int y11,
      int dispModExtent, DmtxDirection side, DmtxBarType type)
{
   Sint16 xBeg, yBeg;
   Sint16 xEnd, yEnd;
   int extent = 128;
   const int screenX = CTRL_COL1_X + 1;
   const int screenY = CTRL_ROW5_Y;
   Uint32 color;

   switch(side) {
      case DmtxDirUp:
         xBeg = x00;
         xEnd = x11;
         yBeg = yEnd = y11 - dispModExtent/2;
         break;
      case DmtxDirLeft:
         yBeg = y00;
         yEnd = y11;
         xBeg = xEnd = x00 + dispModExtent/2;
         break;
      case DmtxDirDown:
         xBeg = x00;
         xEnd = x11;
         yBeg = yEnd = y00 + dispModExtent/2;
         break;
      case DmtxDirRight:
         yBeg = y00;
         yEnd = y11;
         xBeg = xEnd = x11 - dispModExtent/2;
         break;
      default:
         xBeg = x00;
         yBeg = y00;
         xEnd = x11;
         yEnd = y11;
         break;
   }

   yBeg = (extent - 1 - yBeg);
   yEnd = (extent - 1 - yEnd);

   xBeg = Clamp(xBeg + screenX, screenX, 128);
   yBeg = Clamp(yBeg + screenY, screenY, 128);

   xEnd = Clamp(xEnd + screenX, screenX, 128);
   yEnd = Clamp(yEnd + screenY, screenY, 128);

   if(type & DmtxBarFinder)
      color = 0x0000ffff;
   else if(type & DmtxBarTiming)
      color = 0xff0000ff;
   else
      color = 0x00ff00ff;

   lineColor(screen, xBeg, yBeg, xEnd, yEnd, color);
}
