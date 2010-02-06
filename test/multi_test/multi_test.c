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
 * This "multi_test" program is for experimental algorithms. Please
 * consider this code to be untested, unoptimized, and even unstable.
 * If something works here it will make its way into libdmtx "proper"
 * only after being properly written, tuned, and tested.
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
       0,    25,    50,    75,   100,   125,   150,   175,
     200,   224,   249,   273,   297,   321,   345,   369,
     392,   415,   438,   460,   483,   505,   526,   548,
     569,   590,   610,   630,   650,   669,   688,   706,
     724,   742,   759,   775,   792,   807,   822,   837,
     851,   865,   878,   891,   903,   915,   926,   936,
     946,   955,   964,   972,   980,   987,   993,   999,
    1004,  1009,  1013,  1016,  1019,  1021,  1023,  1024,
    1024,  1024,  1023,  1021,  1019,  1016,  1013,  1009,
    1004,   999,   993,   987,   980,   972,   964,   955,
     946,   936,   926,   915,   903,   891,   878,   865,
     851,   837,   822,   807,   792,   775,   759,   742,
     724,   706,   688,   669,   650,   630,   610,   590,
     569,   548,   526,   505,   483,   460,   438,   415,
     392,   369,   345,   321,   297,   273,   249,   224,
     200,   175,   150,   125,   100,    75,    50,    25 };

/* Scaled unit cos */
static int uCos128[] = {
    1024,  1024,  1023,  1021,  1019,  1016,  1013,  1009,
    1004,   999,   993,   987,   980,   972,   964,   955,
     946,   936,   926,   915,   903,   891,   878,   865,
     851,   837,   822,   807,   792,   775,   759,   742,
     724,   706,   688,   669,   650,   630,   610,   590,
     569,   548,   526,   505,   483,   460,   438,   415,
     392,   369,   345,   321,   297,   273,   249,   224,
     200,   175,   150,   125,   100,    75,    50,    25,
       0,   -25,   -50,   -75,  -100,  -125,  -150,  -175,
    -200,  -224,  -249,  -273,  -297,  -321,  -345,  -369,
    -392,  -415,  -438,  -460,  -483,  -505,  -526,  -548,
    -569,  -590,  -610,  -630,  -650,  -669,  -688,  -706,
    -724,  -742,  -759,  -775,  -792,  -807,  -822,  -837,
    -851,  -865,  -878,  -891,  -903,  -915,  -926,  -936,
    -946,  -955,  -964,  -972,  -980,  -987,  -993,  -999,
   -1004, -1009, -1013, -1016, -1019, -1021, -1023, -1024 };

/* Application level functions */
static struct UserOptions GetDefaultOptions(void);
static DmtxPassFail HandleArgs(struct UserOptions *opt, int *argcp, char **argvp[]);
static struct AppState InitAppState(void);
static SDL_Surface *SetWindowSize(int windowWidth, int windowHeight);
static DmtxPassFail HandleEvent(SDL_Event *event, struct AppState *state,
      SDL_Surface *picture, SDL_Surface **screen);
static DmtxPassFail NudgeImage(int windowExtent, int pictureExtent, Sint16 *imageLoc);
/*static void WriteDiagnosticImage(DmtxDecode *dec, char *imagePath);*/

/* Image processing functions */
static void PopulateFlowCache(struct Flow *sFlowCache, struct Flow *bFlowCache,
      struct Flow *hFlowCache, struct Flow *vFlowCache, DmtxImage *img);
static double AdjustOffset(int d, int phiIdx, int extent);
static int GetOffset(int x, int y, int phiIdx, int extent);
static void PopulateHoughCache(struct Hough *pHoughCache, struct Hough *nHoughCache, struct Flow *sFlowCache, struct Flow *bFlowCache, struct Flow *hFlowCache, struct Flow *vFlowCache, int angleBase, int imgExtent);
static void FindHoughMaxima(struct Hough *houghCache, int width, int height);
static void FindBestAngles(struct Hough *pHoughCache, struct Hough *nHoughCache, int phiExtent, int dExtent);

/* Process visualization functions */
static void BlitFlowCache(SDL_Surface *screen, struct Flow *flowCache, int screenX, int screenY, int maxFlowMag);
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
   int                i, pixelCount, maxFlowMag;
   DmtxImage         *img;
   DmtxDecode        *dec;
   int                phi, d, idx;
   struct Flow       *sFlowCache, *bFlowCache, *hFlowCache, *vFlowCache;
   int                spFlowSum, bpFlowSum, hpFlowSum, vpFlowSum, pFlowSumMax;
   int                snFlowSum, bnFlowSum, hnFlowSum, vnFlowSum, nFlowSumMax;
   double             spFlowScale, bpFlowScale, hpFlowScale, vpFlowScale;
   double             snFlowScale, bnFlowScale, hnFlowScale, vnFlowScale;
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
      pixelCount = LOCAL_SIZE * LOCAL_SIZE;
      maxFlowMag = 0;
      for(i = 0; i < pixelCount; i++) {
         if(abs(hFlowCache[i].mag) > maxFlowMag)
            maxFlowMag = abs(hFlowCache[i].mag);

         if(abs(vFlowCache[i].mag) > maxFlowMag)
            maxFlowMag = abs(vFlowCache[i].mag);

         if(abs(sFlowCache[i].mag) > maxFlowMag)
            maxFlowMag = abs(sFlowCache[i].mag);

         if(abs(bFlowCache[i].mag) > maxFlowMag)
            maxFlowMag = abs(bFlowCache[i].mag);
      }

      BlitFlowCache(screen, hFlowCache,   0, 480, maxFlowMag);
      BlitFlowCache(screen, vFlowCache,  64, 480, maxFlowMag);
      BlitFlowCache(screen, sFlowCache, 128, 480, maxFlowMag);
      BlitFlowCache(screen, bFlowCache, 192, 480, maxFlowMag);

      /* Find relative size of hough quadrants */
      PopulateHoughCache(pHoughCache, nHoughCache, sFlowCache, bFlowCache,
            hFlowCache, vFlowCache, 128, LOCAL_SIZE);

      /* Normalize hough quadrants in a slow and inefficient way */
      hpFlowSum = vpFlowSum = spFlowSum = bpFlowSum = 0;
      hnFlowSum = vnFlowSum = snFlowSum = bnFlowSum = 0;
      for(i = 0; i < LOCAL_SIZE * LOCAL_SIZE; i++) {
         if(hFlowCache[i].mag > 0)
            hpFlowSum += hFlowCache[i].mag;
         else
            hnFlowSum += hFlowCache[i].mag;

         if(vFlowCache[i].mag > 0)
            vpFlowSum += vFlowCache[i].mag;
         else
            vnFlowSum += vFlowCache[i].mag;

         if(sFlowCache[i].mag > 0)
            spFlowSum += sFlowCache[i].mag;
         else
            snFlowSum += sFlowCache[i].mag;

         if(bFlowCache[i].mag > 0)
            bpFlowSum += bFlowCache[i].mag;
         else
            bnFlowSum += bFlowCache[i].mag;
      }
      pFlowSumMax = hpFlowSum;
      if(vpFlowSum > pFlowSumMax)
         pFlowSumMax = vpFlowSum;
      if(spFlowSum > pFlowSumMax)
         pFlowSumMax = spFlowSum;
      if(bpFlowSum > pFlowSumMax)
         pFlowSumMax = bpFlowSum;

      nFlowSumMax = hnFlowSum;
      if(vnFlowSum > nFlowSumMax)
         nFlowSumMax = vnFlowSum;
      if(snFlowSum > nFlowSumMax)
         nFlowSumMax = snFlowSum;
      if(bnFlowSum > nFlowSumMax)
         nFlowSumMax = bnFlowSum;

      hpFlowScale = (double)pFlowSumMax/hpFlowSum;
      vpFlowScale = (double)pFlowSumMax/vpFlowSum;
      spFlowScale = (double)pFlowSumMax/spFlowSum;
      bpFlowScale = (double)pFlowSumMax/bpFlowSum;

      hnFlowScale = (double)nFlowSumMax/hnFlowSum;
      vnFlowScale = (double)nFlowSumMax/vnFlowSum;
      snFlowScale = (double)nFlowSumMax/snFlowSum;
      bnFlowScale = (double)nFlowSumMax/bnFlowSum;

      for(phi = 0; phi < 128; phi++) {
         for(d = 0; d < LOCAL_SIZE; d++) {
            idx = d * 128 + phi;
            if(phi < 16 || phi >= 112) {
               pHoughCache[idx].mag = (int)(pHoughCache[idx].mag * vpFlowScale + 0.5);
               nHoughCache[idx].mag = (int)(nHoughCache[idx].mag * vnFlowScale + 0.5);
            }
            else if(phi < 48) {
               pHoughCache[idx].mag = (int)(pHoughCache[idx].mag * bpFlowScale + 0.5);
               nHoughCache[idx].mag = (int)(nHoughCache[idx].mag * bnFlowScale + 0.5);
            }
            else if(phi < 80) {
               pHoughCache[idx].mag = (int)(pHoughCache[idx].mag * hpFlowScale + 0.5);
               nHoughCache[idx].mag = (int)(nHoughCache[idx].mag * hnFlowScale + 0.5);
            }
            else if(phi < 112) {
               pHoughCache[idx].mag = (int)(pHoughCache[idx].mag * spFlowScale + 0.5);
               nHoughCache[idx].mag = (int)(nHoughCache[idx].mag * snFlowScale + 0.5);
            }
         }
      }

      /* Write hough cache images to feedback panes */
      BlitHoughCache(screen, pHoughCache, 256, 480);
      BlitHoughCache(screen, nHoughCache, 256, 544);

      FindHoughMaxima(pHoughCache, 128, LOCAL_SIZE);
      FindHoughMaxima(nHoughCache, 128, LOCAL_SIZE);

      FindBestAngles(pHoughCache, nHoughCache, 128, LOCAL_SIZE);

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
   memset(sFlowCache, 0x00, sizeof(struct Flow) * width * height);
   memset(bFlowCache, 0x00, sizeof(struct Flow) * width * height);
   memset(hFlowCache, 0x00, sizeof(struct Flow) * width * height);
   memset(vFlowCache, 0x00, sizeof(struct Flow) * width * height);

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
         hFlowCache[idx].mag = img->pxl[offsetTL] - img->pxl[offsetBL];
         vFlowCache[idx].mag = img->pxl[offsetBR] - img->pxl[offsetBL];
         sFlowCache[idx].mag = img->pxl[offsetTL] - img->pxl[offsetBR];
         bFlowCache[idx].mag = img->pxl[offsetTR] - img->pxl[offsetBL];

         offsetBL += bytesPerPixel;
      }
   }

   tb = dmtxTimeNow();
/* fprintf(stdout, "PopulateFlowCache time: %ldms\n", (1000000 *
         (tb.sec - ta.sec) + (tb.usec - ta.usec))/1000); */
}

static double
AdjustOffset(int d, int phiIdx, int extent)
{
   int posMax, negMax;
   double scale;

   if(phiIdx < 64) {
      posMax = extent * (uCos128[phiIdx] + uSin128[phiIdx]);
      negMax = 0;
   }
   else {
      posMax = extent * uSin128[phiIdx];
      negMax = extent * uCos128[phiIdx];
   }

   scale = (double)(posMax - negMax) / extent;

   return (d * scale + negMax)/1024.0;

/**
 * original floating point version follows:
 *
   double phiRad;
   double scale;
   double posMax, negMax;

   phiRad = M_PI * phiIdx / 128.0;

   if(phiIdx < 64) {
      posMax = extent * (cos(phiRad) + sin(phiRad));
      negMax = 0.0;
   }
   else {
      posMax = extent * sin(phiRad);
      negMax = extent * cos(phiRad);
   }

   assert(fabs(posMax - negMax) > 0.00001);
   scale = extent / (posMax - negMax);

   return d / scale + negMax;
*/
}

static int
GetOffset(int x, int y, int phiIdx, int extent)
{
   int offset, posMax, negMax;
   double scale;

   if(phiIdx < 64) {
      posMax = extent * (uCos128[phiIdx] + uSin128[phiIdx]);
      negMax = 0;
   }
   else {
      posMax = extent * uSin128[phiIdx];
      negMax = extent * uCos128[phiIdx];
   }

   assert(abs(posMax - negMax) > 0);
   scale = (double)extent / (posMax - negMax);

   offset = (int)((x * uCos128[phiIdx] + y * uSin128[phiIdx] - negMax) * scale + 0.5);

   assert(offset <= 64);

   return offset;

/**
 * original floating point version follows:
 *
   int offset;
   double phiRad;
   double scale;
   double posMax, negMax;

   phiRad = M_PI * phiIdx / 128.0;

   if(phiIdx < 64) {
      posMax = extent * (cos(phiRad) + sin(phiRad));
      negMax = 0.0;
   }
   else {
      posMax = extent * sin(phiRad);
      negMax = extent * cos(phiRad);
   }

   assert(fabs(posMax - negMax) > 0.00001);
   scale = extent / (posMax - negMax);

   offset = (int)((x * cos(phiRad) + y * sin(phiRad) - negMax) * scale + 0.5);
   assert(offset <= 64);

   return offset;
*/
}

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
         if(abs(vFlowCache[idx].mag) > 0) {
            for(phi = 0; phi < 16; phi++) {
               d = GetOffset(x, y, phi, imgExtent);
               if(vFlowCache[idx].mag > 0)
                  pHoughCache[d * angleBase + phi].mag += vFlowCache[idx].mag;
               else
                  nHoughCache[d * angleBase + phi].mag -= vFlowCache[idx].mag;
            }
            for(phi = 112; phi < angleBase; phi++) {
               d = GetOffset(x, y, phi, imgExtent);
               if(vFlowCache[idx].mag > 0)
                  nHoughCache[d * angleBase + phi].mag += vFlowCache[idx].mag;
               else
                  pHoughCache[d * angleBase + phi].mag -= vFlowCache[idx].mag;
            }
         }

         if(abs(bFlowCache[idx].mag) > 0) {
            for(phi = 16; phi < 48; phi++) {
               d = GetOffset(x, y, phi, imgExtent);
               /* Intentional sign inversion to force discontinuity to 0/180 degrees boundary */
               if(bFlowCache[idx].mag > 0)
                  pHoughCache[d * angleBase + phi].mag += bFlowCache[idx].mag;
               else
                  nHoughCache[d * angleBase + phi].mag -= bFlowCache[idx].mag;
            }
         }

         if(abs(hFlowCache[idx].mag) > 0) {
            for(phi = 48; phi < 80; phi++) {
               d = GetOffset(x, y, phi, imgExtent);
               if(hFlowCache[idx].mag > 0)
                  pHoughCache[d * angleBase + phi].mag += hFlowCache[idx].mag;
               else
                  nHoughCache[d * angleBase + phi].mag -= hFlowCache[idx].mag;
            }
         }

         if(abs(sFlowCache[idx].mag) > 0) {
            for(phi = 80; phi < 112; phi++) {
               d = GetOffset(x, y, phi, imgExtent);
               if(sFlowCache[idx].mag > 0)
                  pHoughCache[d * angleBase + phi].mag += sFlowCache[idx].mag;
               else
                  nHoughCache[d * angleBase + phi].mag -= sFlowCache[idx].mag;
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
FindHoughMaxima(struct Hough *houghCache, int width, int height)
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
FindBestAngles(struct Hough *pHoughCache, struct Hough *nHoughCache, int phiExtent, int dExtent)
{
   int phi, d, idx;
   int pIdxBest[128], nIdxBest[128];
   int pMagBest[128], nMagBest[128];
   int magSum, magBest[2] = { 0, 0 };
   int phiDiff, phiBest[2] = { -1, -1 };

   /* Find greatest maximum in p and n caches for each value of phi */
   for(phi = 0; phi < phiExtent; phi++) {

      pIdxBest[phi] = nIdxBest[phi] = phi; /* i.e., (0 * phiExtent + phi) */
      pMagBest[phi] = pHoughCache[phi].mag;
      nMagBest[phi] = nHoughCache[phi].mag;

      for(d = 1; d < dExtent; d++) {

         idx = d * phiExtent + phi;

         if(pHoughCache[idx].isMax && pHoughCache[idx].mag > pMagBest[phi]) {
            pMagBest[phi] = pHoughCache[idx].mag;
            pIdxBest[phi] = idx;
         }
         if(nHoughCache[idx].isMax && nHoughCache[idx].mag > nMagBest[phi]) {
            nMagBest[phi] = nHoughCache[idx].mag;
            nIdxBest[phi] = idx;
         }
      }
   }

   magBest[0] = pHoughCache[0].mag + nHoughCache[0].mag;
   phiBest[0] = 0;
   for(phi = 1; phi < 128; phi++) {

      magSum = pMagBest[phi] + nMagBest[phi];

      if(magSum > magBest[0]) {
         /* If angles are sufficiently different then push down */
         phiDiff = (phi - phiBest[0] + 128) % 128;
         if(phiDiff >= 8 && phiDiff <= 118) {
            magBest[1] = magBest[0];
            phiBest[1] = phiBest[0];
            magBest[0] = magSum;
            phiBest[0] = phi;
         }
         /* Otherwise simply replace */
         else {
            magBest[0] = magSum;
            phiBest[0] = phi;
         }
      }
      else if(magSum > magBest[1]) {
         /* Only update [1] if not in same angle range as [0] */
         phiDiff = (phi - phiBest[0] + 128) % 128;
         if(phiDiff >= 8 && phiDiff <= 118) {
            magBest[1] = magSum;
            phiBest[1] = phi;
         }
      }
   }

   if(phiBest[0] != -1) {
      pHoughCache[pIdxBest[phiBest[0]]].isMax = 2;
      nHoughCache[nIdxBest[phiBest[0]]].isMax = 2;
   }
   if(phiBest[1] != -1) {
      pHoughCache[pIdxBest[phiBest[1]]].isMax = 2;
      nHoughCache[nIdxBest[phiBest[1]]].isMax = 2;
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
FindBestAngles(struct Hough *maximaCache, int width, int height)
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
/* fprintf(stdout, "FindBestAngles time: %ldms\n", (1000000 *
         (tb.sec - ta.sec) + (tb.usec - ta.usec))/1000); */
}
#endif

static void
BlitFlowCache(SDL_Surface *screen, struct Flow *flowCache, int screenX, int screenY, int maxFlowMag)
{
   int row, col;
   unsigned char rgb[3];
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

   for(row = 0; row < height; row++) {
      for(col = 0; col < width; col++) {
         flow = flowCache[row * width + col];
         if(flow.mag > 0) {
            rgb[0] = 0;
            rgb[1] = (int)((abs(flow.mag) * 254.0)/maxFlowMag + 0.5);
            rgb[2] = 0;
         }
         else {
            rgb[0] = (int)((abs(flow.mag) * 254.0)/maxFlowMag + 0.5);
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
         if(maximaCache[d * angles + phi].isMax != 2)
            continue;

         dScaled = AdjustOffset(d, phi, LOCAL_SIZE);

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
