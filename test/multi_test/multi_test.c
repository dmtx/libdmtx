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

/* $Id: multi_test.c 561 2008-12-28 16:28:58Z mblaughton $ */

/**
 * Try experimenting with specialized flow detection. For example, detected
 * edge is at center of this pattern:
 *
 *  +-+-+
 *  |a|b|
 *  +-o-+
 *  |c|d|
 *  +-+-+
 *
 *  vEdge = ((b-a)+(c-d))/2
 *  hEdge = ((a-c)+(b-d))/2
 *  sEdge is (a-d) reduced/increased toward zero by abs(b-c)
 *  bEdge is (b-c) reduced/increased toward zero by abs(a-d)
 *
 * Use bilinear (tri-?) when oversampling to fix vertical and horizonatal
 * diagonal problem
 *
 * For presentation purposes, scale flow panes with single scale factor
 * instead of individually
 *
 * Consider positive and negative hough peaks together for each angle when
 * determining strongest line
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <assert.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_gfxPrimitives.h>
#include "../../dmtx.h"

#define LOCAL_SIZE 64

struct UserOptions {
   const char *imagePath;
};

struct AppState {
   int         windowWidth;
   int         windowHeight;
   Sint16      imageLocX;
   Sint16      imageLocY;
   Uint8       leftButton;
   Uint8       rightButton;
   Uint16      pointerX;
   Uint16      pointerY;
   DmtxBoolean quit;
};

struct Flow {
   int mag;
};

struct Hough {
   char isMax;
   unsigned int mag;
};

/* Scaled unit sin */
static int uSin128[] = {
       0,    25,    50,    75,    99,   123,   147,   169,
     191,   212,   233,   252,   270,   288,   304,   320,
     334,   348,   361,   373,   385,   396,   407,   417,
     427,   437,   446,   456,   467,   477,   488,   500,
     512,   525,   539,   553,   569,   585,   602,   620,
     639,   658,   678,   699,   720,   742,   764,   786,
     808,   829,   850,   871,   891,   911,   929,   946,
     961,   975,   988,   999,  1008,  1015,  1020,  1023,
    1024,  1023,  1020,  1015,  1008,   999,   988,   975,
     961,   946,   929,   911,   891,   871,   850,   829,
     808,   786,   764,   742,   720,   699,   678,   658,
     639,   620,   602,   585,   569,   553,   539,   525,
     512,   500,   488,   477,   467,   456,   446,   437,
     427,   417,   407,   396,   385,   373,   361,   348,
     334,   320,   304,   288,   270,   252,   233,   212,
     191,   169,   147,   123,    99,    75,    50,    25 };

/* Scaled unit cos */
static int uCos128[] = {
    1024,  1023,  1020,  1015,  1008,   999,   988,   975,
     961,   946,   929,   911,   891,   871,   850,   829,
     808,   786,   764,   742,   720,   699,   678,   658,
     639,   620,   602,   585,   569,   553,   539,   525,
     512,   500,   488,   477,   467,   456,   446,   437,
     427,   417,   407,   396,   385,   373,   361,   348,
     334,   320,   304,   288,   270,   252,   233,   212,
     191,   169,   147,   123,    99,    75,    50,    25,
       0,   -25,   -50,   -75,   -99,  -123,  -147,  -169,
    -191,  -212,  -233,  -252,  -270,  -288,  -304,  -320,
    -334,  -348,  -361,  -373,  -385,  -396,  -407,  -417,
    -427,  -437,  -446,  -456,  -467,  -477,  -488,  -500,
    -512,  -525,  -539,  -553,  -569,  -585,  -602,  -620,
    -639,  -658,  -678,  -699,  -720,  -742,  -764,  -786,
    -808,  -829,  -850,  -871,  -891,  -911,  -929,  -946,
    -961,  -975,  -988,  -999, -1008, -1015, -1020, -1023 };

static struct UserOptions GetDefaultOptions(void);
static DmtxPassFail HandleArgs(struct UserOptions *opt, int *argcp, char **argvp[]);
static struct AppState InitAppState(void);
static SDL_Surface *SetWindowSize(int windowWidth, int windowHeight);
static DmtxPassFail HandleEvent(SDL_Event *event, struct AppState *state,
      SDL_Surface *picture, SDL_Surface **screen);
static DmtxPassFail NudgeImage(int windowExtent, int pictureExtent, Sint16 *imageLoc);
/*static void WriteDiagnosticImage(DmtxDecode *dec, char *imagePath);*/
static void PopulateFlowCache(struct Flow *sFlowCache, struct Flow *bFlowCache,
      struct Flow *hFlowCache, struct Flow *vFlowCache, DmtxImage *img);
static double AdjustOffset(int d, int phiIdx, int width, int height);
static int GetOffset(int x, int y, int phiIdx, int width, int height);
static void PopulateHoughCache(struct Hough *pHoughCache, struct Hough *nHoughCache, struct Flow *sFlowCache, struct Flow *bFlowCache, struct Flow *hFlowCache, struct Flow *vFlowCache, int angleBase, int imgExtent);
static void PopulateMaximaCache(struct Hough *houghCache, int width, int height);
static void NarrowMaximaRange(struct Hough *pMaximaCache, struct Hough *nMaximaCache, int phiExtent, int dExtent);
static void BlitFlowCache(SDL_Surface *screen, struct Flow *flowCache, int screenX, int screenY);
static void BlitHoughCache(SDL_Surface *screen, struct Hough *houghCache, int screenX, int screenY);
static void BlitActiveRegion(SDL_Surface *screen, SDL_Surface *active, int screenX, int screenY);
/*static void PlotPixel(SDL_Surface *surface, int x, int y);*/
static int Ray2Intersect(double *t, DmtxRay2 p0, DmtxRay2 p1);
static int IntersectBox(DmtxRay2 ray, DmtxVector2 bb0, DmtxVector2 bb1, DmtxVector2 *p0, DmtxVector2 *p1);
static void DrawActiveBorder(SDL_Surface *screen, int activeExtent);
static void DrawGridLines(SDL_Surface *screen, struct Hough *maximaCache, int angles, int diag, int screenX, int screenY);

int
main(int argc, char *argv[])
{
   struct UserOptions opt;
   struct AppState    state;
   SDL_Surface       *screen;
   SDL_Surface       *picture;
   SDL_Event          event;
   SDL_Rect           imageLoc;
   Uint32             bgColorB, bgColorK;
   DmtxImage         *img;
   DmtxDecode        *dec;
   struct Flow       *sFlowCache, *bFlowCache, *hFlowCache, *vFlowCache;
   struct Hough      *pHoughCache, *nHoughCache;
   SDL_Rect           clipRect;
   SDL_Surface       *local;

   opt = GetDefaultOptions();

   if(HandleArgs(&opt, &argc, &argv) == DmtxFail) {
      exit(1);
/*    ShowUsage(1); */
   }

   state = InitAppState();

   /* Load image */
   picture = IMG_Load(opt.imagePath);
   if(picture == NULL) {
      fprintf(stderr, "Unable to load image \"%s\": %s\n", opt.imagePath, SDL_GetError());
      exit(1);
   }

   /* Allocate memory for the flow caches */
   sFlowCache = (struct Flow *)calloc(LOCAL_SIZE * LOCAL_SIZE, sizeof(struct Flow));
   assert(sFlowCache != NULL);
   bFlowCache = (struct Flow *)calloc(LOCAL_SIZE * LOCAL_SIZE, sizeof(struct Flow));
   assert(bFlowCache != NULL);
   hFlowCache = (struct Flow *)calloc(LOCAL_SIZE * LOCAL_SIZE, sizeof(struct Flow));
   assert(hFlowCache != NULL);
   vFlowCache = (struct Flow *)calloc(LOCAL_SIZE * LOCAL_SIZE, sizeof(struct Flow));
   assert(vFlowCache != NULL);

   pHoughCache = (struct Hough *)calloc(128 * LOCAL_SIZE, sizeof(struct Hough));
   assert(pHoughCache != NULL);
   nHoughCache = (struct Hough *)calloc(128 * LOCAL_SIZE, sizeof(struct Hough));
   assert(nHoughCache != NULL);

   atexit(SDL_Quit);

   /* Initialize SDL library */
   if(SDL_Init(SDL_INIT_VIDEO) < 0) {
      fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
      exit(1);
   }

   screen = SetWindowSize(state.windowWidth, state.windowHeight);
   NudgeImage(state.windowWidth, picture->w, &state.imageLocX);
   NudgeImage(state.windowHeight, picture->h, &state.imageLocY);

   bgColorB = SDL_MapRGBA(screen->format, 0, 0, 64, 255);
   bgColorK = SDL_MapRGBA(screen->format, 0, 0, 0, 255);

   /* Create surface to hold image pixels to be scanned */
   local = SDL_CreateRGBSurface(SDL_SWSURFACE, LOCAL_SIZE, LOCAL_SIZE, 32, 0, 0, 0, 0);
   switch(local->pitch / local->w) {
      case 1:
/* XXX Should be creating image with stride/row padding */
         img = dmtxImageCreate(local->pixels, local->w, local->h, DmtxPack8bppK);
         break;
      case 3:
         img = dmtxImageCreate(local->pixels, local->w, local->h, DmtxPack24bppRGB);
         break;
      case 4:
         img = dmtxImageCreate(local->pixels, local->w, local->h, DmtxPack32bppXRGB);
         break;
      default:
         exit(1);
   }
   assert(img != NULL);

   dec = dmtxDecodeCreate(img, 1);
   assert(dec != NULL);

   for(;;) {
      SDL_Delay(10);

      while(SDL_PollEvent(&event))
         HandleEvent(&event, &state, picture, &screen);

      if(state.quit == DmtxTrue)
         break;

      imageLoc.x = state.imageLocX;
      imageLoc.y = state.imageLocY;

      /* Capture portion of image that falls within highlighted region */
      clipRect.x = (screen->w - LOCAL_SIZE)/2 - imageLoc.x;
      clipRect.y = (screen->h - LOCAL_SIZE)/2 - imageLoc.y;
      clipRect.w = LOCAL_SIZE;
      clipRect.h = LOCAL_SIZE;
      SDL_FillRect(local, NULL, bgColorK);
      SDL_BlitSurface(picture, &clipRect, local, NULL);

      /* Start with blank canvas */
      SDL_FillRect(screen, NULL, bgColorB);

      /* Draw image to main canvas area */
      clipRect.w = 640;
      clipRect.h = 480;
      clipRect.x = 0;
      clipRect.y = 0;
      SDL_SetClipRect(screen, &clipRect);
      SDL_BlitSurface(picture, NULL, screen, &imageLoc);
      SDL_SetClipRect(screen, NULL);

      DrawActiveBorder(screen, LOCAL_SIZE);

      SDL_LockSurface(local);
      PopulateFlowCache(sFlowCache, bFlowCache, hFlowCache, vFlowCache, dec->image);
      SDL_UnlockSurface(local);

      /* Write flow cache images to feedback panes */
      BlitFlowCache(screen, hFlowCache,   0, 480);
      BlitFlowCache(screen, vFlowCache,  64, 480);
      BlitFlowCache(screen, sFlowCache, 128, 480);
      BlitFlowCache(screen, bFlowCache, 192, 480);

      PopulateHoughCache(pHoughCache, nHoughCache, sFlowCache, bFlowCache,
            hFlowCache, vFlowCache, 128, LOCAL_SIZE);

      /* Write hough cache images to feedback panes */
      BlitHoughCache(screen, pHoughCache, 256, 480);
      BlitHoughCache(screen, nHoughCache, 256, 544);

      PopulateMaximaCache(pHoughCache, 128, LOCAL_SIZE);
      PopulateMaximaCache(nHoughCache, 128, LOCAL_SIZE);

      NarrowMaximaRange(pHoughCache, nHoughCache, 128, LOCAL_SIZE);

      /* Write maxima cache images to feedback panes */
      BlitHoughCache(screen, pHoughCache, 384, 480);
      BlitHoughCache(screen, nHoughCache, 384, 544);

      /* Draw positive hough lines to feedback panes */
      BlitActiveRegion(screen, local, 512, 480);
      DrawGridLines(screen, pHoughCache, 128, LOCAL_SIZE, 512, 480);

      /* Draw negative hough lines to feedback panes */
      BlitActiveRegion(screen, local, 512, 544);
      DrawGridLines(screen, nHoughCache, 128, LOCAL_SIZE, 512, 544);

      SDL_Flip(screen);
   }

   SDL_FreeSurface(local);

   free(nHoughCache);
   free(pHoughCache);
   free(vFlowCache);
   free(hFlowCache);
   free(bFlowCache);
   free(sFlowCache);

   dmtxDecodeDestroy(&dec);
   dmtxImageDestroy(&img);

   exit(0);
}

/**
 *
 *
 */
static struct UserOptions
GetDefaultOptions(void)
{
   struct UserOptions opt;

   memset(&opt, 0x00, sizeof(struct UserOptions));

   opt.imagePath = NULL;

   return opt;
}

/**
 *
 *
 */
static DmtxPassFail
HandleArgs(struct UserOptions *opt, int *argcp, char **argvp[])
{
   if(*argcp < 2) {
      fprintf(stdout, "argument required\n");
      return DmtxFail;
   }
   else {
      opt->imagePath = (*argvp)[1];
   }

   return DmtxPass;
}

/**
 *
 *
 */
static struct AppState
InitAppState(void)
{
   struct AppState state;

   state.windowWidth = 640;
   state.windowHeight = 480 + 2 * LOCAL_SIZE;
   state.imageLocX = 0;
   state.imageLocY = 0;
   state.leftButton = SDL_RELEASED;
   state.rightButton = SDL_RELEASED;
   state.pointerX = 0;
   state.pointerY = 0;
   state.quit = DmtxFalse;

   return state;
}

/**
 *
 *
 */
static SDL_Surface *
SetWindowSize(int windowWidth, int windowHeight)
{
   SDL_Surface *screen;

   screen = SDL_SetVideoMode(windowWidth, windowHeight, 32,
         SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE);

   if(screen == NULL) {
      fprintf(stderr, "Couldn't set %dx%dx32 video mode: %s\n", windowWidth,
            windowHeight, SDL_GetError());
      exit(1);
   }

   return screen;
}

/**
 *
 *
 */
static DmtxPassFail
HandleEvent(SDL_Event *event, struct AppState *state, SDL_Surface *picture, SDL_Surface **screen)
{
   int nudgeRequired = DmtxFalse;

   switch(event->type) {
      case SDL_KEYDOWN:
         switch(event->key.keysym.sym) {
            case SDLK_ESCAPE:
               state->quit = DmtxTrue;
            default:
               break;
         }
         break;

      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
         if(event->button.button == SDL_BUTTON_LEFT)
            state->leftButton = event->button.state;
         else if(event->button.button == SDL_BUTTON_RIGHT)
            state->rightButton = event->button.state;
         break;

      case SDL_MOUSEMOTION:
         state->pointerX = event->motion.x;
         state->pointerY = event->motion.y;

         if(state->leftButton == SDL_PRESSED) {
            state->imageLocX += event->motion.xrel;
            state->imageLocY += event->motion.yrel;
            nudgeRequired = DmtxTrue;
         }
         break;

      case SDL_QUIT:
         state->quit = DmtxTrue;
         break;

      case SDL_VIDEORESIZE:
         state->windowWidth = event->resize.w;
         state->windowHeight = event->resize.h;
         *screen = SetWindowSize(state->windowWidth, state->windowHeight);
         nudgeRequired = DmtxTrue;
         break;

      default:
         break;
   }

   if(nudgeRequired == DmtxTrue) {
      NudgeImage(state->windowWidth, picture->w, &(state->imageLocX));
      NudgeImage(state->windowHeight, picture->h, &(state->imageLocY));
   }

   return DmtxPass;
}

/**
 *
 *
 */
static DmtxPassFail
NudgeImage(int windowExtent, int pictureExtent, Sint16 *imageLoc)
{
   int minReveal = 16;

   if(*imageLoc < minReveal - pictureExtent)
      *imageLoc = minReveal - pictureExtent;
   else if(*imageLoc > windowExtent - minReveal)
      *imageLoc = windowExtent - minReveal;

   return DmtxPass;
}

/**
 *
 *
 */
/*
static void
WriteDiagnosticImage(DmtxDecode *dec, char *imagePath)
{
   int totalBytes, headerBytes;
   int bytesWritten;
   unsigned char *pnm;
   FILE *fp;

   fp = fopen(imagePath, "wb");
   if(fp == NULL)
      exit(1);

   pnm = dmtxDecodeCreateDiagnostic(dec, &totalBytes, &headerBytes, 0);
   if(pnm == NULL)
      exit(1);

   bytesWritten = fwrite(pnm, sizeof(unsigned char), totalBytes, fp);
   if(bytesWritten != totalBytes)
      exit(1);

   free(pnm);
   fclose(fp);
}
*/

static void
PopulateFlowCache(struct Flow *sFlowCache, struct Flow *bFlowCache,
      struct Flow *hFlowCache, struct Flow *vFlowCache,
      DmtxImage *img)
{
   int width, height;
   int bytesPerPixel, rowSizeBytes, colorPlane;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
   int sTmp, bTmp, sEdge, bEdge, hEdge, vEdge;
   int hEdge2, vEdge2;
   int offsetBL, offsetTL, offsetBR, offsetTR;
   int idx;
   DmtxTime ta, tb;

   width = dmtxImageGetProp(img, DmtxPropWidth);
   height = dmtxImageGetProp(img, DmtxPropHeight);
   rowSizeBytes = dmtxImageGetProp(img, DmtxPropRowSizeBytes);
   bytesPerPixel = dmtxImageGetProp(img, DmtxPropBytesPerPixel);
   colorPlane = 0; /* XXX need to make some decisions here */

   xBeg = 1;
   xEnd = width - 2;
   yBeg = 1;
   yEnd = height - 2;

   ta = dmtxTimeNow();

/* XXX should be accessing image with access methods or at least respecting stride */

   for(y = yBeg; y <= yEnd; y++) {
      offsetBL = ((height - y - 1) * rowSizeBytes) + bytesPerPixel + colorPlane;

      for(x = xBeg; x <= xEnd; x++) {

         /* Pixel data first pixel = top-left; everything else bottom-left */
         offsetTL = offsetBL - rowSizeBytes;
         offsetBR = offsetBL + bytesPerPixel;
         offsetTR = offsetTL + bytesPerPixel;

         idx = y * width + x;

         /**
          * VV B+ +H  +S
          * -+ -B -H  S-
          */
         hEdge = img->pxl[offsetTL] - img->pxl[offsetBL];
         hEdge2 = img->pxl[offsetTR] - img->pxl[offsetBR];
         vEdge = img->pxl[offsetBR] - img->pxl[offsetBL];
         vEdge2 = img->pxl[offsetTR] - img->pxl[offsetTL];
/*       sEdge = ((img->pxl[offsetTR] - img->pxl[offsetBR]) - vEdge)/2;
         bEdge = (hEdge + vEdge)/2; */
         sTmp = img->pxl[offsetTL] - img->pxl[offsetBR];
         bTmp = img->pxl[offsetTR] - img->pxl[offsetBL];

         if(sTmp < 0) {
            sEdge = (sTmp + abs(bTmp) > 0) ? 0 : sTmp + abs(bTmp);
         }
         else {
            sEdge = (sTmp - abs(bTmp) < 0) ? 0 : sTmp - abs(bTmp);
         }

         if(bTmp < 0) {
            bEdge = (bTmp + abs(sTmp) > 0) ? 0 : bTmp + abs(sTmp);
         }
         else {
            bEdge = (bTmp - abs(sTmp) < 0) ? 0 : bTmp - abs(sTmp);
         }

         sFlowCache[idx].mag = sEdge;
         bFlowCache[idx].mag = bEdge;
/*       hFlowCache[idx].mag = hEdge;
         vFlowCache[idx].mag = vEdge; */
         hFlowCache[idx].mag = (hEdge + hEdge2)/2;
         vFlowCache[idx].mag = (vEdge + vEdge2)/2;

         offsetBL += bytesPerPixel;
      }
   }

   tb = dmtxTimeNow();
/* fprintf(stdout, "PopulateFlowCache time: %ldms\n", (1000000 *
         (tb.sec - ta.sec) + (tb.usec - ta.usec))/1000); */
}

#define TRANS 0.146446609406726

static double
AdjustOffset(int d, int phiIdx, int width, int height)
{
   double phiRad;
   double scale;
   double posMax, negMax, extMax;

   phiRad = M_PI * phiIdx / 128.0;

   extMax = (height > width) ? height : width;

   if(phiIdx < 64) {
      posMax = width * cos(phiRad) + height * sin(phiRad);
      negMax = 0.0;
   }
   else {
      posMax = height * sin(phiRad);
      negMax = width * cos(phiRad);
   }

   assert(fabs(posMax - negMax) > 0.00001);
   scale = extMax / (posMax - negMax);

   return d / scale + negMax;
}

static int
GetOffset(int x, int y, int phiIdx, int width, int height)
{
   double phiRad;
   double scale;
   double posMax, negMax, extMax;

   phiRad = M_PI * phiIdx / 128.0;

   extMax = (height > width) ? height : width;

   if(phiIdx < 64) {
      posMax = width * cos(phiRad) + height * sin(phiRad);
      negMax = 0.0;
   }
   else {
      posMax = height * sin(phiRad);
      negMax = width * cos(phiRad);
   }

   assert(fabs(posMax - negMax) > 0.00001);
   scale = extMax / (posMax - negMax);

   assert((int)((x * cos(phiRad) + y * sin(phiRad) - negMax) * scale + 0.5) <= 64);

   return (int)((x * cos(phiRad) + y * sin(phiRad) - negMax) * scale + 0.5);
}
#undef TRANS

/**
 *
 *
 */
static void
PopulateHoughCache(struct Hough *pHoughCache, struct Hough *nHoughCache, struct Flow *sFlowCache, struct Flow *bFlowCache, struct Flow *hFlowCache, struct Flow *vFlowCache, int angleBase, int imgExtent)
{
   int idx, phi, d;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
/* double scale; */
   DmtxTime ta, tb;
   int width, height;

   width = height = imgExtent;

   xBeg = 2;
   xEnd = width - 3;
   yBeg = 2;
   yEnd = height - 3;

   ta = dmtxTimeNow();

   memset(pHoughCache, 0x00, sizeof(struct Hough) * angleBase * imgExtent);
   memset(nHoughCache, 0x00, sizeof(struct Hough) * angleBase * imgExtent);

   for(phi = 0; phi < angleBase; phi++) {
      for(d = 0; d < LOCAL_SIZE; d++) {
         pHoughCache[d * angleBase + phi].isMax = 1;
         nHoughCache[d * angleBase + phi].isMax = 1;
      }
   }

   for(y = yBeg; y <= yEnd; y++) {
      for(x = xBeg; x <= xEnd; x++) {

         idx = y * width + x;

         /* After finalizing algorithm, precalculate for 32x32 square:
          *   each product (x*uCos[phi]) and (y*uSin[phi]) (FAST) (8kB)
          *   or, map each (x,y,phi) to its (d,phi) location (FASTER) (262kB)
          *
          * This should provide a huge speedup.
          */

         if(abs(vFlowCache[idx].mag) > 5) {
            for(phi = 0; phi < 16; phi++) {
               d = GetOffset(x, y, phi, width, height);
               if(vFlowCache[idx].mag > 0)
                  pHoughCache[d * angleBase + phi].mag += vFlowCache[idx].mag;
               else
                  nHoughCache[d * angleBase + phi].mag -= vFlowCache[idx].mag;
            }
            for(phi = 112; phi < angleBase; phi++) {
               d = GetOffset(x, y, phi, width, height);
               if(vFlowCache[idx].mag > 0)
                  nHoughCache[d * angleBase + phi].mag += vFlowCache[idx].mag;
               else
                  pHoughCache[d * angleBase + phi].mag -= vFlowCache[idx].mag;
            }
         }

         if(abs(bFlowCache[idx].mag) > 5) {
            for(phi = 16; phi < 48; phi++) {
               d = GetOffset(x, y, phi, width, height);
               /* Intentional sign inversion to force discontinuity to 0/180 degrees boundary */
               if(bFlowCache[idx].mag > 0)
                  pHoughCache[d * angleBase + phi].mag += (int)(bFlowCache[idx].mag * 1.41 + 0.5);
/*                pHoughCache[d * angleBase + phi].mag += (int)(bFlowCache[idx].mag + 0.5);*/
               else
                  nHoughCache[d * angleBase + phi].mag -= (int)(bFlowCache[idx].mag * 1.41 + 0.5);
/*                nHoughCache[d * angleBase + phi].mag -= (int)(bFlowCache[idx].mag + 0.5);*/
            }
         }

         if(abs(hFlowCache[idx].mag) > 5) {
            for(phi = 48; phi < 80; phi++) {
               d = GetOffset(x, y, phi, width, height);
               if(hFlowCache[idx].mag > 0)
                  pHoughCache[d * angleBase + phi].mag += hFlowCache[idx].mag;
               else
                  nHoughCache[d * angleBase + phi].mag -= hFlowCache[idx].mag;
            }
         }

         if(abs(sFlowCache[idx].mag) > 5) {
            for(phi = 80; phi < 112; phi++) {
               d = GetOffset(x, y, phi, width, height);
               if(sFlowCache[idx].mag > 0)
                  pHoughCache[d * angleBase + phi].mag += (int)(sFlowCache[idx].mag * 0.71 + 0.5);
/*                pHoughCache[d * angleBase + phi].mag += (int)(sFlowCache[idx].mag + 0.5);*/
               else
                  nHoughCache[d * angleBase + phi].mag -= (int)(sFlowCache[idx].mag * 0.71 + 0.5);
/*                nHoughCache[d * angleBase + phi].mag -= (int)(sFlowCache[idx].mag + 0.5);*/
            }
         }
      }
   }

   tb = dmtxTimeNow();
/* fprintf(stdout, "PopulateHough time: %ldms\n", (1000000 *
         (tb.sec - ta.sec) + (tb.usec - ta.usec))/1000); */
}

/**
 *
 *
 */
static void
PopulateMaximaCache(struct Hough *houghCache, int width, int height)
{
   int x, y;
   int idx0, idx1;
   DmtxTime ta, tb;

   ta = dmtxTimeNow();

   /* Find local maxima for each angle */
   for(x = 0; x < width; x++) {
      for(y = 0; y < height - 1; y++) {

         idx0 = y * width + x;
         idx1 = idx0 + width;

         if(houghCache[idx0].mag == 0)
            houghCache[idx0].isMax = 0;
         else if(houghCache[idx0].mag > houghCache[idx1].mag)
            houghCache[idx1].isMax = 0;
         else if(houghCache[idx1].mag > houghCache[idx0].mag)
            houghCache[idx0].isMax = 0;
      }
   }

   tb = dmtxTimeNow();
/* fprintf(stdout, "PopulateMaxima time: %ldms\n", (1000000 *
         (tb.sec - ta.sec) + (tb.usec - ta.usec))/1000); */
}

static void
NarrowMaximaRange(struct Hough *pMaximaCache, struct Hough *nMaximaCache, int phiExtent, int dExtent)
{
   int phi, d, idx;
   int pIdxMax, nIdxMax;
   int pIdxBest, nIdxBest;
   int sumMagMax, pMagMax, nMagMax;

   pIdxMax = nIdxMax = -1;
   pIdxBest = nIdxBest = -1;
   sumMagMax = 0;

   /* For each phi */
   for(phi = 0; phi < phiExtent; phi++) {

      /* For each offset, find greatest maximum in p and n caches */
      pMagMax = nMagMax = 0;
      for(d = 0; d < dExtent; d++) {
         idx = d * phiExtent + phi;

         if(pMaximaCache[idx].isMax && pMaximaCache[idx].mag > pMagMax) {
            pMagMax = pMaximaCache[idx].mag;
            pIdxMax = idx;
         }

         if(nMaximaCache[idx].isMax && nMaximaCache[idx].mag > nMagMax) {
            nMagMax = nMaximaCache[idx].mag;
            nIdxMax = idx;
         }
      }

      if(pMagMax + nMagMax > sumMagMax) {
         pIdxBest = pIdxMax;
         nIdxBest = nIdxMax;
         sumMagMax = pMagMax + nMagMax;
      }
   }

   if(pIdxMax != -1 && nIdxMax != -1) {
      pMaximaCache[pIdxBest].isMax = 2;
      nMaximaCache[nIdxBest].isMax = 2;
   }
}

/**
 *
 *
 */
#ifndef min
#define min(x,y) ((x < y) ? x : y)
#endif
#ifdef IGNOREME
static void
NarrowMaximaRange(struct Hough *maximaCache, int width, int height)
{
   int xMid, xUse;
   int xPeak, xPeakPrev, xPeakNext;
   int zPeak, zPeakPrev, zPeakNext;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
   int i, magCompare;
   int maximaCount, maximaTally;
   int maxValueCap, currentMaxValue, currentMaxOccurrences;
   int usableOccurrences;
   int range[128] = { 0 };
   DmtxTime ta, tb;

   xBeg = 0;
   xEnd = width - 1;
   yBeg = 0;
   yEnd = height - 1;

   ta = dmtxTimeNow();

   /* for each grouping of 3 angles */
   for(xMid = xBeg; xMid <= xEnd; xMid++) {

      maximaCount = 0;
      maximaTally = 0;
      maxValueCap = -1; /* unknown */
      currentMaxValue = 1; /* start with lowest non-zero maxima */
      currentMaxOccurrences = 0;

      /* execute at most 8 times */
      for(i = 0; i < 8; i++) {

         /* find next largest value and count occurrences */
         for(x = xMid - 1; x <= xMid + 1; x++) {
            for(y = yBeg; y <= yEnd; y++) {

               xUse = (x + 128) % 128;

               magCompare = maximaCache[y * width + xUse].mag;

               /* already lost, skip */
               if(magCompare < currentMaxValue)
                  continue;

               /* already counted this location, skip */
               if(maxValueCap != -1 && magCompare >= maxValueCap)
                  continue;

               if(magCompare > currentMaxValue) {
                  /* found new max less than cap -- reset counters */
                  currentMaxValue = magCompare;
                  currentMaxOccurrences = 1;
               }
               else if(magCompare == currentMaxValue) {
                  /* repeat occurrence -- increment */
                  currentMaxOccurrences++;
               }
            }
         }

         usableOccurrences = min(8 - maximaCount, currentMaxOccurrences);
         maximaCount += currentMaxOccurrences;
         maximaTally += (usableOccurrences * currentMaxValue);
         maxValueCap = currentMaxValue;

         if(maximaCount >= 8 || currentMaxValue == 1)
            break;
      }
      range[xMid] = maximaTally;
   }

   /* Find maximum maxima tally */
   currentMaxValue = range[0];
   xPeak = 0;
   for(x = 0; x < 128; x++) {
      if(range[x] > currentMaxValue) {
         currentMaxValue = range[x];
         xPeak = x;
      }
   }

   xPeakPrev = (xPeak + 128 - 1)%128;
   xPeakNext = (xPeak + 128 + 1)%128;

   zPeak = (xPeak + 64) % 128;
   zPeakPrev = (zPeak + 128 - 1) % 128;
   zPeakNext = (zPeak + 128 + 1) % 128;

   /* Erase all values that are not in maximum */
   for(x = 0; x < 128; x++) {
      if(x != xPeak && x != xPeakPrev && x != xPeakNext &&
            x != zPeak && x != zPeakPrev && x != zPeakNext) {
         for(y = yBeg; y < yEnd; y++)
            maximaCache[y * width + x].mag = 0;
      }
   }

   tb = dmtxTimeNow();
/* fprintf(stdout, "NarrowMaximaRange time: %ldms\n", (1000000 *
         (tb.sec - ta.sec) + (tb.usec - ta.usec))/1000); */
}
#endif

static void
BlitFlowCache(SDL_Surface *screen, struct Flow *flowCache, int screenX, int screenY)
{
   int row, col;
   unsigned char rgb[3];
   int flowVal, maxVal;
   int width, height;
   int offset;
   struct Flow flow;
   unsigned char pixbuf[12288]; /* 64 * 64 * 3 */
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

   width = LOCAL_SIZE;
   height = LOCAL_SIZE;

   maxVal = 0;
   for(row = 0; row < height; row++) {
      for(col = 0; col < width; col++) {
         flowVal = abs(flowCache[row * width + col].mag);
         if(flowVal > maxVal)
            maxVal = flowVal;
      }
   }

   for(row = 0; row < height; row++) {
      for(col = 0; col < width; col++) {
         flow = flowCache[row * width + col];
         if(flow.mag > 0) {
            rgb[0] = 0;
            rgb[1] = (int)((abs(flow.mag) * 254.0)/maxVal + 0.5);
            rgb[2] = 0;
         }
         else {
            rgb[0] = (int)((abs(flow.mag) * 254.0)/maxVal + 0.5);
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

static void
BlitHoughCache(SDL_Surface *screen, struct Hough *houghCache, int screenX, int screenY)
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
   for(row = 0; row < height; row++) {
      for(col = 0; col < width; col++) {
         if(houghCache[row * width + col].isMax == 0)
            continue;

         if(houghCache[row * width + col].mag > maxVal)
            maxVal = houghCache[row * width + col].mag;
      }
   }

   for(row = 0; row < height; row++) {
      for(col = 0; col < width; col++) {

         cache = houghCache[row * width + col].mag;

         if(houghCache[row * width + col].isMax == 2) {
            rgb[0] = 255;
            rgb[1] = rgb[2] = 0;
         }
         else if(houghCache[row * width + col].isMax == 1) {
            rgb[0] = rgb[1] = rgb[2] = (int)((cache * 254.0)/maxVal + 0.5);
         }
         else {
            rgb[0] = rgb[1] = rgb[2] = 0;
         }

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

static void
BlitActiveRegion(SDL_Surface *screen, SDL_Surface *active, int screenX, int screenY)
{
   SDL_Rect clipRect;

   clipRect.w = LOCAL_SIZE;
   clipRect.h = LOCAL_SIZE;
   clipRect.x = screenX;
   clipRect.y = screenY;

   SDL_BlitSurface(active, NULL, screen, &clipRect);
}

#ifdef NOTDEFINED
/**
 *
 *
 */
static void
PlotPixel(SDL_Surface *surface, int x, int y)
{
   char *ptr;
   Uint32 col;

   ptr = (char *)surface->pixels;
   col = SDL_MapRGB(surface->format, 255, 0, 0);

   memcpy(ptr + surface->pitch * y + surface->format->BytesPerPixel * x,
         &col, surface->format->BytesPerPixel);
}
#endif

static int
Ray2Intersect(double *t, DmtxRay2 p0, DmtxRay2 p1)
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

static int
IntersectBox(DmtxRay2 ray, DmtxVector2 bb0, DmtxVector2 bb1, DmtxVector2 *p0, DmtxVector2 *p1)
{
   double xMin, xMax, yMin, yMax;
   double tTmp, t[4], tMin[2], tMax[2];
   double tMinStart, tMaxStart;
   int i;
   int tCount = 0;
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

   rBtm.p.X = rTop.p.X = rLft.p.X = xMin;
   rRgt.p.X = xMax;

   rBtm.p.Y = rLft.p.Y = rRgt.p.Y = yMin;
   rTop.p.Y = yMax;

   rBtm.v = rTop.v = unitX;
   rLft.v = rRgt.v = unitY;

   if(Ray2Intersect(&tTmp, ray, rBtm) == DmtxPass)
      t[tCount++] = tTmp;

   if(Ray2Intersect(&tTmp, ray, rTop) == DmtxPass)
      t[tCount++] = tTmp;

   if(Ray2Intersect(&tTmp, ray, rLft) == DmtxPass)
      t[tCount++] = tTmp;

   if(Ray2Intersect(&tTmp, ray, rRgt) == DmtxPass)
      t[tCount++] = tTmp;

   if(tCount < 2)
      return DmtxFail;

   tMinStart = tMaxStart = t[0];
   for(i = 1; i < tCount; i++) {
      if(t[i] < tMinStart)
         tMinStart = t[i];
      if(t[i] > tMaxStart)
         tMaxStart = t[i];
   }

   /* tMin[1] is the smallest and tMin[0] is second smallest */
   tMin[0] = tMin[1] = tMaxStart;
   for(i = 0; i < tCount; i++) {
      if(t[i] < tMin[1]) {
         tMin[0] = tMin[1];
         tMin[1] = t[i];
      }
      else if(t[i] < tMin[0]) {
         tMin[0] = t[i];
      }
   }

   /* tMax[1] is the larget and tMax[0] is second largest */
   tMax[0] = tMax[1] = tMinStart;
   for(i = 0; i < tCount; i++) {
      if(t[i] > tMax[1]) {
         tMax[0] = tMax[1];
         tMax[1] = t[i];
      }
      else if(t[i] > tMax[0]) {
         tMax[0] = t[i];
      }
   }

   dmtxPointAlongRay2(p0, &ray, tMin[0]);
   dmtxPointAlongRay2(p1, &ray, tMax[0]);

   return DmtxTrue;
}

static void
DrawActiveBorder(SDL_Surface *screen, int activeExtent)
{
   Sint16 x00, y00;
   Sint16 x10, y10;
   Sint16 x11, y11;
   Sint16 x01, y01;

   x01 = (screen->w - activeExtent)/2 - 1;
   y01 = (screen->h - activeExtent)/2 - 1;

   x00 = x01;
   y00 = y01 + activeExtent + 1;

   x10 = x00 + activeExtent + 1;
   y10 = y00;

   x11 = x10;
   y11 = y01;

   lineColor(screen, x00, y00, x10, y10, 0xffff00ff);
   lineColor(screen, x10, y10, x11, y11, 0xffff00ff);
   lineColor(screen, x11, y11, x01, y01, 0xffff00ff);
   lineColor(screen, x01, y01, x00, y00, 0xffff00ff);
}

static void
DrawGridLines(SDL_Surface *screen, struct Hough *maximaCache, int angles,
      int diag, int screenX, int screenY)
{
   int phi, d;
   double phiRad;
   double dScaled;
   DmtxVector2 bb0, bb1;
   DmtxVector2 p0, p1;
   DmtxRay2 rStart, rLine;
   DmtxPixelLoc d0, d1;

   bb0.X = bb0.Y = 0.0;
   bb1.X = bb1.Y = LOCAL_SIZE;

   rStart.p.X = rStart.p.Y = 0.0;

   for(phi = 0; phi < angles; phi++) {

      phiRad = phi * M_PI/128.0;

      rStart.v.X = cos(phiRad);
      rStart.v.Y = sin(phiRad);

      rLine.v.X = -rStart.v.Y;
      rLine.v.Y = rStart.v.X;

      for(d = 0; d < diag; d++) {

/*       if(maximaCache[d * angles + phi].mag > 0) { */
         if(maximaCache[d * angles + phi].isMax == 2) {

            dScaled = AdjustOffset(d, phi, LOCAL_SIZE, LOCAL_SIZE);

            dmtxPointAlongRay2(&(rLine.p), &rStart, dScaled);

            p0.X = p0.Y = p1.X = p1.Y = 0.0;

            if(IntersectBox(rLine, bb0, bb1, &p0, &p1) == DmtxFalse)
               continue;

            d0.X = (int)(p0.X + 0.5) + screenX;
            d1.X = (int)(p1.X + 0.5) + screenX;

            d0.Y = screenY + (LOCAL_SIZE - (int)(p0.Y + 0.5) - 1);
            d1.Y = screenY + (LOCAL_SIZE - (int)(p1.Y + 0.5) - 1);

            lineColor(screen, d0.X, d0.Y, d1.X, d1.Y, 0xff0000ff);
         }
      }
   }
}
