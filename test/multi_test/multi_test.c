/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2009 Mike Laughton

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

/*
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <assert.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "../../dmtx.h"

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
      struct Flow *hFlowCache, struct Flow *vFlowCache, DmtxImage *img, int width, int height);
static void PopulateHoughCache(struct Hough *pHoughCache, struct Hough *nHoughCache, struct Flow *sFlowCache, struct Flow *bFlowCache, struct Flow *hFlowCache, struct Flow *vFlowCache, int width, int height, int diag);
static void PopulateTightCache(struct Hough *tight, struct Hough *pHoughCache, struct Hough *nHoughCache, int width, int height);
static void WriteFlowCacheImage(struct Flow *flow, int width, int height, char *imagePath);
static void WriteHoughCacheImage(struct Hough *hough, int width, int height, char *imagePath);
/*static void PlotPixel(SDL_Surface *surface, int x, int y);*/

int main(int argc, char *argv[])
{
   struct UserOptions opt;
   struct AppState    state;
   int                scanX, scanY;
   int                width, height;
   int                diag;
   SDL_Surface       *screen;
   SDL_Surface       *picture;
   SDL_Event          event;
   SDL_Rect           imageLoc;
   DmtxImage         *img;
   DmtxDecode        *dec;
   struct Flow       *sFlowCache, *bFlowCache, *hFlowCache, *vFlowCache;
   struct Hough      *pHoughCache, *nHoughCache, *tight;

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

   switch(picture->pitch / picture->w) {
      case 1:
         img = dmtxImageCreate(picture->pixels, picture->w, picture->h, DmtxPack8bppK);
         break;
      case 3:
         img = dmtxImageCreate(picture->pixels, picture->w, picture->h, DmtxPack24bppRGB);
         break;
      case 4:
         img = dmtxImageCreate(picture->pixels, picture->w, picture->h, DmtxPack32bppXRGB);
         break;
      default:
         exit(1);
   }
   assert(img != NULL);

   dec = dmtxDecodeCreate(img, 1);
   assert(dec != NULL);

   width = dmtxImageGetProp(img, DmtxPropWidth);
   height = dmtxImageGetProp(img, DmtxPropHeight);

   sFlowCache = (struct Flow *)calloc(width * height, sizeof(struct Flow));
   assert(sFlowCache != NULL);
   bFlowCache = (struct Flow *)calloc(width * height, sizeof(struct Flow));
   assert(bFlowCache != NULL);
   hFlowCache = (struct Flow *)calloc(width * height, sizeof(struct Flow));
   assert(hFlowCache != NULL);
   vFlowCache = (struct Flow *)calloc(width * height, sizeof(struct Flow));
   assert(vFlowCache != NULL);

/* diag = (int)(sqrt(width * width + height * height) + 0.5); */
/* diagIdx = (int)(atan2(height, width) * 180/M_PI + 0.5); */
   diag = (int)(sqrt(width * width + height * height) + 0.5);

   pHoughCache = (struct Hough *)calloc(128 * diag, sizeof(struct Hough));
   assert(pHoughCache != NULL);

   nHoughCache = (struct Hough *)calloc(128 * diag, sizeof(struct Hough));
   assert(nHoughCache != NULL);

   tight = (struct Hough *)calloc(128 * diag, sizeof(struct Hough));
   assert(tight != NULL);

   SDL_LockSurface(picture);
   PopulateFlowCache(sFlowCache, bFlowCache, hFlowCache, vFlowCache, dec->image, width, height);
   SDL_UnlockSurface(picture);

   PopulateHoughCache(pHoughCache, nHoughCache, sFlowCache, bFlowCache,
         hFlowCache, vFlowCache, width, height, diag);
/* PopulateTightCache(tight, pHoughCache, nHoughCache, 128, diag); */

   WriteFlowCacheImage(sFlowCache, width, height, "sFlowCache.pnm");
   WriteFlowCacheImage(bFlowCache, width, height, "bFlowCache.pnm");
   WriteFlowCacheImage(hFlowCache, width, height, "hFlowCache.pnm");
   WriteFlowCacheImage(vFlowCache, width, height, "vFlowCache.pnm");
   WriteHoughCacheImage(pHoughCache, 128, diag, "pHoughCache.pnm");
   WriteHoughCacheImage(nHoughCache, 128, diag, "nHoughCache.pnm");
   WriteHoughCacheImage(tight, 128, diag, "tHoughCache.pnm");

   atexit(SDL_Quit);

   /* Initialize SDL library */
   if(SDL_Init(SDL_INIT_VIDEO) < 0) {
      fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
      exit(1);
   }

   screen = SetWindowSize(state.windowWidth, state.windowHeight);
   NudgeImage(state.windowWidth, picture->w, &state.imageLocX);
   NudgeImage(state.windowHeight, picture->h, &state.imageLocY);

   for(;;) {
      SDL_Delay(10);

      while(SDL_PollEvent(&event))
         HandleEvent(&event, &state, picture, &screen);

      if(state.quit == DmtxTrue)
         break;

      imageLoc.x = state.imageLocX;
      imageLoc.y = state.imageLocY;

      SDL_FillRect(screen, NULL, 0xff000050);
      SDL_BlitSurface(picture, NULL, screen, &imageLoc);

      if(state.rightButton == SDL_PRESSED) {

         scanX = state.pointerX - state.imageLocX;
         scanY = state.pointerY - state.imageLocY;

         /* Highlight a small box of edges */
/*
         SDL_LockSurface(screen);
         for(y = -10; y <= 10; y++) {
            for(x = -10; x <= 10; x++) {
               if(scanY + y < 1 || scanY + y > height - 2)
                  continue;
               if(scanX + x < 1 || scanX + x > width - 2)
                  continue;
               if(!(edgeCache[(scanY + y) * width + (scanX + x)].dir & ArriveConnected))
                  continue;
               if(!(edgeCache[(scanY + y) * width + (scanX + x)].dir & DepartConnected))
                  continue;

               PlotPixel(screen, state.imageLocX + scanX + x, state.imageLocY + scanY + y);
            }
         }
         SDL_UnlockSurface(screen);
*/
      }

      SDL_Flip(screen);
   }

   free(tight);
   free(nHoughCache);
   free(pHoughCache);
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
   state.windowHeight = 480;
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
   int marginA, marginB;

   marginA = *imageLoc;
   marginB = *imageLoc + pictureExtent - windowExtent;

   /* Image falls completely within window */
   if(pictureExtent <= windowExtent) {
      *imageLoc = (marginA - marginB)/2;
   }
   /* One edge inside and one edge outside window */
   else if(marginA * marginB > 0) {
      if((pictureExtent - windowExtent) * marginA < 0)
         *imageLoc -= marginB;
      else
         *imageLoc -= marginA;
   }

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
      DmtxImage *img, int width, int height)
{
   int bytesPerPixel, rowSizeBytes, colorPlane;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
   int sEdge, bEdge, hEdge, vEdge;
   int offsetBL, offsetTL, offsetBR, offsetTR;
   int idx;
   DmtxTime ta, tb;

   rowSizeBytes = dmtxImageGetProp(img, DmtxPropRowSizeBytes);
   bytesPerPixel = dmtxImageGetProp(img, DmtxPropBytesPerPixel);
   colorPlane = 0; /* XXX need to make some decisions here */

   xBeg = 1;
   xEnd = width - 2;
   yBeg = 1;
   yEnd = height - 2;

   ta = dmtxTimeNow();

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
         sEdge = img->pxl[offsetTL] - img->pxl[offsetBR];
         bEdge = img->pxl[offsetTR] - img->pxl[offsetBL];
         hEdge = img->pxl[offsetTL] - img->pxl[offsetBL];
         vEdge = img->pxl[offsetBR] - img->pxl[offsetBL];

         sFlowCache[idx].mag = sEdge;
         bFlowCache[idx].mag = bEdge;
         hFlowCache[idx].mag = hEdge;
         vFlowCache[idx].mag = vEdge;

         offsetBL += bytesPerPixel;
      }
   }

   tb = dmtxTimeNow();
   fprintf(stdout, "PopulateFlowCache time: %ldms\n", (1000000 *
         (tb.sec - ta.sec) + (tb.usec - ta.usec))/1000);
}

#define TRANS 0.146446609406726

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

   return (int)((x * cos(phiRad) + y * sin(phiRad) - negMax) * scale + 0.5);

/* d = ((x * uCos128[phi] + y * uSin128[phi]) >> 10); */
/*
   scale = cos(4 * phiRad) * TRANS + 1 - TRANS;

   negMax = (phiIdx < 64) ? 0 : (int)(32 * cos(phiRad) + 0.5);

   return (int)((x * cos(phiRad) + y * sin(phiRad)) * scale + 0.5) - negMax;
*/
}
#undef TRANS

/**
 * 12.0 -----    6
 * 10.0   -----  5
 *  8.0 -----    4
 *  6.0   -----  3
 *  5.0 -----    2
 *  4.0 -----    2
 *  2.0   -----  1
 *  0.0 -----    0
 * -2.0   ----- -3
 * -4.0 -----   -3
 * -5.0 -----   -3
 * -6.0   ----- -4
 *
 * algorithm for ranking contrast
 * is fabs() faster than x * -1 ?
 * is there a fast math way to flip sign?
 */
static void
PopulateHoughCache(struct Hough *pHoughCache, struct Hough *nHoughCache, struct Flow *sFlowCache, struct Flow *bFlowCache, struct Flow *hFlowCache, struct Flow *vFlowCache, int width, int height, int diag)
{
   int idx, phi, d;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
/* double scale; */
   DmtxTime ta, tb;

   xBeg = 2;
   xEnd = width - 3;
   yBeg = 2;
   yEnd = height - 3;

   ta = dmtxTimeNow();

   for(y = yBeg; y <= yEnd; y++) {
      for(x = xBeg; x <= xEnd; x++) {

         idx = y * width + x;

         /* After finalizing algorithm, precalculate for 32x32 square:
          *   each product (x*uCos[phi]) and (y*uSin[phi]) (FAST) (8kB)
          *   or, map each (x,y,phi) to its (d,phi) location (FASTER) (262kB)
          *
          * This should provide a huge speedup.
          */

/*             d = (x * cos(M_PI*phi/128.0) + y * sin(M_PI*phi/128.0))/2; */
/*             assert(d < 2 * diag); */

         if(abs(vFlowCache[idx].mag) > 5) {
            for(phi = 0; phi < 16; phi++) {
/*             d = ((x * uCos128[phi] + y * uSin128[phi]) >> 10);
               d = (x * cos(M_PI*phi/128.0) + y * sin(M_PI*phi/128.0)) * scale; */
               d = GetOffset(x, y, phi, width, height);
               if(vFlowCache[idx].mag > 0)
                  pHoughCache[d * 128 + phi].mag += vFlowCache[idx].mag;
               else
                  nHoughCache[d * 128 + phi].mag -= vFlowCache[idx].mag;
            }
            for(phi = 112; phi < 128; phi++) {
/*             d = ((x * uCos128[phi] + y * uSin128[phi]) >> 10);
               scale = (phi >= 32 && phi < 80) ? fabs(sin(M_PI*phi/128.0)) : fabs(cos(M_PI*phi/128.0));
               d = (x * cos(M_PI*phi/128.0) + y * sin(M_PI*phi/128.0)) * scale; */
               d = GetOffset(x, y, phi, width, height);
               if(vFlowCache[idx].mag > 0)
                  nHoughCache[d * 128 + phi].mag += vFlowCache[idx].mag;
               else
                  pHoughCache[d * 128 + phi].mag -= vFlowCache[idx].mag;
            }
         }

         if(abs(bFlowCache[idx].mag) > 5) {
            for(phi = 16; phi < 48; phi++) {
/*             d = ((x * uCos128[phi] + y * uSin128[phi]) >> 10);
               scale = (phi >= 32 && phi < 80) ? fabs(sin(M_PI*phi/128.0)) : fabs(cos(M_PI*phi/128.0));
               d = (x * cos(M_PI*phi/128.0) + y * sin(M_PI*phi/128.0)) * scale; */
               d = GetOffset(x, y, phi, width, height);
               if(bFlowCache[idx].mag > 0)
                  pHoughCache[d * 128 + phi].mag += bFlowCache[idx].mag;
               else
                  nHoughCache[d * 128 + phi].mag -= bFlowCache[idx].mag;
            }
         }

         if(abs(hFlowCache[idx].mag) > 5) {
            for(phi = 48; phi < 80; phi++) {
/*             d = ((x * uCos128[phi] + y * uSin128[phi]) >> 10);
               scale = (phi >= 32 && phi < 80) ? fabs(sin(M_PI*phi/128.0)) : fabs(cos(M_PI*phi/128.0));
               d = (x * cos(M_PI*phi/128.0) + y * sin(M_PI*phi/128.0)) * scale; */
               d = GetOffset(x, y, phi, width, height);
               if(hFlowCache[idx].mag > 0)
                  pHoughCache[d * 128 + phi].mag += hFlowCache[idx].mag;
               else
                  nHoughCache[d * 128 + phi].mag -= hFlowCache[idx].mag;
            }
         }

         if(abs(sFlowCache[idx].mag) > 5) {
            for(phi = 80; phi < 112; phi++) {
/*             d = ((x * uCos128[phi] + y * uSin128[phi]) >> 10);
               scale = (phi >= 32 && phi < 80) ? fabs(sin(M_PI*phi/128.0)) : fabs(cos(M_PI*phi/128.0));
               d = (x * cos(M_PI*phi/128.0) + y * sin(M_PI*phi/128.0)) * scale; */
               d = GetOffset(x, y, phi, width, height);
               if(sFlowCache[idx].mag > 0)
                  pHoughCache[d * 128 + phi].mag += sFlowCache[idx].mag;
               else
                  nHoughCache[d * 128 + phi].mag -= sFlowCache[idx].mag;
            }
         }
      }
   }

   tb = dmtxTimeNow();
   fprintf(stdout, "PopulateHough time: %ldms\n", (1000000 *
         (tb.sec - ta.sec) + (tb.usec - ta.usec))/1000);
}

/**
 *
 *
 */
static void
PopulateTightCache(struct Hough *tight, struct Hough *pHoughCache, struct Hough *nHoughCache, int width, int height)
{
   int idxMid, idxTop, idxBtm;
   int diffTop, diffBtm, yStrong;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
   DmtxTime ta, tb;

   xBeg = 0;
   xEnd = width - 1;
   yBeg = 1;
   yEnd = height - 2;

   ta = dmtxTimeNow();

   /* Find local maxima for each angle */
   for(x = xBeg; x <= xEnd; x++) {
      for(y = yBeg; y <= yEnd; y++) {

         idxMid = y * width + x;
         idxTop = idxMid + width;
         idxBtm = idxMid - width;

         if(pHoughCache[idxMid].mag > 0) {
            diffTop = pHoughCache[idxMid].mag - pHoughCache[idxTop].mag;
            diffBtm = pHoughCache[idxMid].mag - pHoughCache[idxBtm].mag;
            if(diffTop >= 0 && diffBtm >= 0)
               tight[idxMid].mag = (diffTop + diffBtm);
/*             tight[idxMid].mag = pHoughCache[idxMid].mag; */
         }

         if(nHoughCache[idxMid].mag > 0) {
            diffTop = nHoughCache[idxMid].mag - nHoughCache[idxTop].mag;
            diffBtm = nHoughCache[idxMid].mag - nHoughCache[idxBtm].mag;
            if(diffTop >= 0 && diffBtm >= 0) {
               tight[idxMid].mag += (diffTop + diffBtm);
/*             tight[idxMid].mag += nHoughCache[idxMid].mag; */
            }
         }
      }
   }

   /* Find strongest maxima for each angle */
   for(x = xBeg; x <= xEnd; x++) {
      yStrong = yBeg;
      for(y = yBeg; y <= yEnd; y++) {
         idxMid = y * width + x;
         if(tight[idxMid].mag > tight[yStrong].mag)
            yStrong = idxMid;
      }

      if(yStrong != yBeg)
         tight[yStrong].mag *= 2;
   }

   tb = dmtxTimeNow();
   fprintf(stdout, "PopulateTight time: %ldms\n", (1000000 *
         (tb.sec - ta.sec) + (tb.usec - ta.usec))/1000);
}

/**
 *
 *
 */
static void
WriteFlowCacheImage(struct Flow *flowCache, int width, int height, char *imagePath)
{
   int row, col;
   int rgb[3];
   int flowVal, maxVal;
   struct Flow flow;
   FILE *fp;

   fp = fopen(imagePath, "wb");
   if(fp == NULL)
      exit(1);

   fprintf(fp, "P6\n%d %d\n255\n", width, height);

   maxVal = 0;
   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {
         flowVal = abs(flowCache[row * width + col].mag);
         if(flowVal > maxVal)
            maxVal = flowVal;
      }
   }

   for(row = height - 1; row >= 0; row--) {
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

         fputc(rgb[0], fp);
         fputc(rgb[1], fp);
         fputc(rgb[2], fp);
      }
   }

   fclose(fp);
}

/**
 *
 *
 */
static void
WriteHoughCacheImage(struct Hough *houghCache, int width, int height, char *imagePath)
{
   int row, col;
   int maxVal;
   int rgb[3];
   unsigned int cache;
   FILE *fp;

   fp = fopen(imagePath, "wb");
   if(fp == NULL)
      exit(1);

   assert(width == 128);

   fprintf(fp, "P6\n%d %d\n255\n", width, height);

   maxVal = 0;
   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {
         if(houghCache[row * width + col].mag > maxVal)
            maxVal = houghCache[row * width + col].mag;
      }
   }

   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {
         cache = houghCache[row * width + col].mag;
/*
         if((col >= 16 && col < 48) || (col >= 80 && col < 112)) {
            cache = (int)(cache * 0.7071 + 0.5);
            cache = (int)(cache * 0.5 + 0.5);
         }
*/
         rgb[0] = rgb[1] = rgb[2] = (int)((cache * 254.0)/maxVal + 0.5);
         fputc(rgb[0], fp);
         fputc(rgb[1], fp);
         fputc(rgb[2], fp);
      }
   }

   fclose(fp);
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
