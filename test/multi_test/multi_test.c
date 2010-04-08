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

/**
 * Next:
 * o Change logic to use strongest mag as starting point, then find grid from there
 * o Track multiple strongest points so best grid can be used
 * o Find grid at +-90 degress
 * o Display normalized view in realtime
 *
 * Approach:
 *   1) Calculate s/b/h/v flow caches (edge intensity)
 *   2) Accumulate and normalize Hough cache
 *   3) Detect high contrast vanishing points (skip for now ... assume infinity)
 *   4) Normalize region by shifting vanishing points to infinity (skip for now)
 *   5) Detect N strongest angles within Hough
 *   6) Detect M strongest offsets within each of N angles
 *   7) Find grid pattern for N*M angle/offset combos
 *   8) Test for presence of barcode
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
#include <SDL/SDL_rotozoom.h>
#include "../../dmtx.h"

#define LOCAL_SIZE   64
#define LINE_SORT_MAX 8

#define CTRL_COL1_X 511
#define CTRL_COL2_X 576

#define CTRL_ROW1_Y   0
#define CTRL_ROW2_Y  65
#define CTRL_ROW3_Y 130
#define CTRL_ROW4_Y 195
#define CTRL_ROW5_Y 260
#define CTRL_ROW6_Y 325

struct UserOptions {
   const char *imagePath;
};

struct AppState {
   int         windowWidth;
   int         windowHeight;
   int         activeExtent;
   int         displayDots;
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

#define HOUGH_D_EXTENT 64
#define HOUGH_PHI_EXTENT 128

struct HoughCache {
   int dExtent;
   int phiExtent;
   char isMax[HOUGH_D_EXTENT * HOUGH_PHI_EXTENT];
   unsigned int mag[HOUGH_D_EXTENT * HOUGH_PHI_EXTENT];
};

struct Hough {
   char isMax;
   unsigned int mag;
};

struct HoughLine {
   int d;
   int phi;
   int mag;
};

struct HoughLineSort {
   struct HoughLine lines[LINE_SORT_MAX];
   int count;
   int maxCount;
};

struct Timing {
   int angle;
   int shift;
   int periodScaled;
   int scale;
   int mag;
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
static void PopulateHoughCache(struct HoughCache *houghCache, struct Flow *sFlowCache, struct Flow *bFlowCache, struct Flow *hFlowCache, struct Flow *vFlowCache);
static void NormalizeHoughCache(struct HoughCache *houghCache, struct Flow *sFlowCache, struct Flow *bFlowCache, struct Flow *hFlowCache, struct Flow *vFlowCache);
static void MarkHoughMaxima(struct HoughCache *houghCache);
static void AddToAngleSort(struct HoughLineSort *stack, struct HoughLine line);
static struct HoughLineSort FindBestAngles(struct HoughCache *houghCache);
static void SetTimingPattern(int periodScaled, int scale, int center, char pattern[], int patternSize);
static struct Timing FindGridTiming(struct HoughCache *houghCache, struct HoughLineSort *sort);
static struct Timing FindGridTimingAtLine(struct HoughCache *houghCache, struct HoughLine line);

/* Process visualization functions */
static void BlitFlowCache(SDL_Surface *screen, struct Flow *flowCache, int maxFlowMag, int screenX, int screenY);
static void BlitHoughCache(SDL_Surface *screen, struct HoughCache *houghCache, int screenX, int screenY);
static void BlitActiveRegion(SDL_Surface *screen, SDL_Surface *active, int zoom, int screenX, int screenY);
static void PlotPixel(SDL_Surface *surface, int x, int y);
static int Ray2Intersect(double *t, DmtxRay2 p0, DmtxRay2 p1);
static int IntersectBox(DmtxRay2 ray, DmtxVector2 bb0, DmtxVector2 bb1, DmtxVector2 *p0, DmtxVector2 *p1);
static void DrawActiveBorder(SDL_Surface *screen, int activeExtent);
static void DrawLine(SDL_Surface *screen, int baseExtent, int screenX, int screenY, int phi, int d, int displayScale);
static void DrawPhiBox(SDL_Surface *screen, int extent, int screenX, int screenY, int phi, int d);
static void DrawTimingLines(SDL_Surface *screen, struct Timing timing, int scale, int screenX, int screenY);
static void DrawTimingDots(SDL_Surface *screen, struct Timing timing, int screenX, int screenY);

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
   int                i, j;
   int                pixelCount, maxFlowMag;
   int                displayCol;
   DmtxImage         *img;
   DmtxDecode        *dec;
   struct Flow       *sFlowCache, *bFlowCache, *hFlowCache, *vFlowCache;
   struct HoughLine   line;
   struct HoughCache  houghCache;
   struct HoughLineSort lineSort;
   struct Timing      timing;
   SDL_Rect           clipRect;
   SDL_Surface       *local, *localTmp;

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

   atexit(SDL_Quit);

   /* Initialize SDL library */
   if(SDL_Init(SDL_INIT_VIDEO) < 0) {
      fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
      exit(1);
   }

   screen = SetWindowSize(state.windowWidth, state.windowHeight);
   NudgeImage(state.windowWidth, picture->w, &state.imageLocX);
   NudgeImage(state.windowHeight, picture->h, &state.imageLocY);

   bgColorB = SDL_MapRGBA(screen->format, 0, 0, 50, 255);
   bgColorK = SDL_MapRGBA(screen->format, 0, 0, 0, 255);

   /* Create surface to hold image pixels to be scanned */
   local = SDL_CreateRGBSurface(SDL_SWSURFACE, LOCAL_SIZE, LOCAL_SIZE, 32, 0, 0, 0, 0);

   /* Create another surface for scaling purposes */
   localTmp = SDL_CreateRGBSurface(SDL_SWSURFACE, LOCAL_SIZE, LOCAL_SIZE, 32, 0, 0, 0, 0);

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
      /* Use blitsurface if 1:1, otherwise scale */
      SDL_FillRect(local, NULL, bgColorK);
      if(state.activeExtent == 64) {
         clipRect.x = (screen->w - state.activeExtent)/2 - imageLoc.x;
         clipRect.y = (screen->h - state.activeExtent)/2 - imageLoc.y;
         clipRect.w = LOCAL_SIZE;
         clipRect.h = LOCAL_SIZE;
         SDL_BlitSurface(picture, &clipRect, local, NULL);
      }
      else if(state.activeExtent == 32) {
         Uint8 localBpp;
         Uint8 *writePixel, *readTL, *readTR, *readBL, *readBR;

         /* first blit, then expand */
         clipRect.x = (screen->w - state.activeExtent)/2 - imageLoc.x;
         clipRect.y = (screen->h - state.activeExtent)/2 - imageLoc.y;
         clipRect.w = LOCAL_SIZE;
         clipRect.h = LOCAL_SIZE;
         SDL_BlitSurface(picture, &clipRect, localTmp, NULL);

         localBpp = local->format->BytesPerPixel;
         SDL_LockSurface(local);
         SDL_LockSurface(localTmp);
         for(i = 0; i < 64; i++) {
            for(j = 0; j < 64; j++) {
               readTL = (Uint8 *)localTmp->pixels + ((i/2) * 64 + (j/2)) * localBpp;
               readTR = readTL + localBpp;
               readBL = readTL + (64 * localBpp);
               readBR = readBL + localBpp;
               writePixel = (Uint8 *)local->pixels + ((i * 64 + j) * localBpp);

               /* memcpy(writePixel, readTL, localBpp); nearest neighbor */
               if(!(i & 0x01) && !(j & 0x01)) {
                  memcpy(writePixel, readTL, localBpp);
               }
               else if((i & 0x01) && !(j & 0x01)) {
                  writePixel[0] = ((Uint16)readTL[0] + (Uint16)readBL[0])/2;
                  writePixel[1] = ((Uint16)readTL[1] + (Uint16)readBL[1])/2;
                  writePixel[2] = ((Uint16)readTL[2] + (Uint16)readBL[2])/2;
                  writePixel[3] = ((Uint16)readTL[3] + (Uint16)readBL[3])/2;
               }
               else if(!(i & 0x01) && (j & 0x01)) {
                  writePixel[0] = ((Uint16)readTL[0] + (Uint16)readTR[0])/2;
                  writePixel[1] = ((Uint16)readTL[1] + (Uint16)readTR[1])/2;
                  writePixel[2] = ((Uint16)readTL[2] + (Uint16)readTR[2])/2;
                  writePixel[3] = ((Uint16)readTL[3] + (Uint16)readTR[3])/2;
               }
               else {
                  writePixel[0] = ((Uint16)readTL[0] + (Uint16)readTR[0] +
                        (Uint16)readBL[0] + (Uint16)readBR[0])/4;
                  writePixel[1] = ((Uint16)readTL[1] + (Uint16)readTR[1] +
                        (Uint16)readBL[1] + (Uint16)readBR[1])/4;
                  writePixel[2] = ((Uint16)readTL[2] + (Uint16)readTR[2] +
                        (Uint16)readBL[2] + (Uint16)readBR[2])/4;
                  writePixel[3] = ((Uint16)readTL[3] + (Uint16)readTR[3] +
                        (Uint16)readBL[3] + (Uint16)readBR[3])/4;
               }
            }
         }
         SDL_UnlockSurface(localTmp);
         SDL_UnlockSurface(local);
      }
      else if(state.activeExtent == 16) {
         Uint8 localBpp;
         Uint8 *writePixel, *readTL;

         /* first blit, then expand */
         clipRect.x = (screen->w - state.activeExtent)/2 - imageLoc.x;
         clipRect.y = (screen->h - state.activeExtent)/2 - imageLoc.y;
         clipRect.w = LOCAL_SIZE;
         clipRect.h = LOCAL_SIZE;
         SDL_BlitSurface(picture, &clipRect, localTmp, NULL);

         localBpp = local->format->BytesPerPixel;
         SDL_LockSurface(local);
         SDL_LockSurface(localTmp);
         for(i = 0; i < 64; i++) {
            for(j = 0; j < 64; j++) {
               readTL = (Uint8 *)localTmp->pixels + ((i/4) * 64 + (j/4)) * localBpp;
               writePixel = (Uint8 *)local->pixels + ((i * 64 + j) * localBpp);
               memcpy(writePixel, readTL, localBpp);
            }
         }
         SDL_UnlockSurface(localTmp);
         SDL_UnlockSurface(local);
      }

      /* Start with blank canvas */
      SDL_FillRect(screen, NULL, bgColorB);

      /* Draw image to main canvas area */
      clipRect.w = 510;
      clipRect.h = 453;
      clipRect.x = 0;
      clipRect.y = 0;
      SDL_SetClipRect(screen, &clipRect);
      SDL_BlitSurface(picture, NULL, screen, &imageLoc);
      SDL_SetClipRect(screen, NULL);

      DrawActiveBorder(screen, state.activeExtent);

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

      BlitFlowCache(screen, hFlowCache, maxFlowMag, CTRL_COL1_X, CTRL_ROW1_Y);
      BlitFlowCache(screen, vFlowCache, maxFlowMag, CTRL_COL2_X, CTRL_ROW1_Y);
      BlitFlowCache(screen, sFlowCache, maxFlowMag, CTRL_COL1_X, CTRL_ROW2_Y);
      BlitFlowCache(screen, bFlowCache, maxFlowMag, CTRL_COL2_X, CTRL_ROW2_Y);

      /* Find relative size of hough quadrants */
      PopulateHoughCache(&houghCache, sFlowCache, bFlowCache, hFlowCache, vFlowCache);
      NormalizeHoughCache(&houghCache, sFlowCache, bFlowCache, hFlowCache, vFlowCache);
      BlitHoughCache(screen, &houghCache, CTRL_COL1_X, CTRL_ROW3_Y);

      MarkHoughMaxima(&houghCache);

      /* Draw strongest lines over Hough and original view feedback panes */
      lineSort = FindBestAngles(&houghCache);

      BlitActiveRegion(screen, local, 1, CTRL_COL1_X, CTRL_ROW4_Y);
      BlitActiveRegion(screen, local, 1, CTRL_COL2_X, CTRL_ROW4_Y);
      BlitActiveRegion(screen, local, 2, CTRL_COL1_X, CTRL_ROW5_Y);

      if(state.displayDots == DmtxTrue) {
         for(i = 0; i < lineSort.count; i++) {
            line = lineSort.lines[i];

            if(i < 2) {
               displayCol = CTRL_COL1_X;
               DrawLine(screen, 128, displayCol, CTRL_ROW5_Y, line.phi, line.d, 2);
            }
            else {
               displayCol = CTRL_COL2_X;
            }
            DrawLine(screen, 64, displayCol, CTRL_ROW4_Y, line.phi, line.d, 1);
            DrawPhiBox(screen, 64, CTRL_COL1_X, CTRL_ROW3_Y, line.phi, line.d);
         }
      }

      /* Draw timing lines */
      timing = FindGridTiming(&houghCache, &lineSort);
      if(state.displayDots == DmtxTrue) {
/*       DrawTimingLines(screen, timing, 2, CTRL_COL1_X, CTRL_ROW5_Y); */
         DrawTimingDots(screen, timing, CTRL_COL1_X, CTRL_ROW3_Y);
      }

      SDL_Flip(screen);
   }

   SDL_FreeSurface(localTmp);
   SDL_FreeSurface(local);

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
   state.windowHeight = 453;
   state.activeExtent = 64;
   state.displayDots = DmtxTrue;
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
               break;
            case SDLK_d:
               state->displayDots = (state->displayDots == DmtxTrue) ? DmtxFalse : DmtxTrue;
               break;
            default:
               break;
         }
         break;

      case SDL_MOUSEBUTTONDOWN:
         switch(event->button.button) {
            case SDL_BUTTON_LEFT:
               state->leftButton = event->button.state;
               break;
            case SDL_BUTTON_RIGHT:
               state->rightButton = event->button.state;
               break;
            case SDL_BUTTON_WHEELDOWN:
               if(state->activeExtent < 64)
                  state->activeExtent *= 2;
               break;
            case SDL_BUTTON_WHEELUP:
               if(state->activeExtent > 16)
                  state->activeExtent /= 2;
               break;
         }
         break;

      case SDL_MOUSEBUTTONUP:
         switch(event->button.button) {
            case SDL_BUTTON_LEFT:
               state->leftButton = event->button.state;
               break;
            case SDL_BUTTON_RIGHT:
               state->rightButton = event->button.state;
               break;
         }
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

/**
 * 2x2 Kernel
 */
#ifdef IGNOREME
static void
PopulateFlowCache(struct Flow *sFlowCache, struct Flow *bFlowCache,
      struct Flow *hFlowCache, struct Flow *vFlowCache, DmtxImage *img)
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
#endif

/**
 * 3x3 Sobel Kernel
 */
static void
PopulateFlowCache(struct Flow *sFlowCache, struct Flow *bFlowCache,
      struct Flow *hFlowCache, struct Flow *vFlowCache, DmtxImage *img)
{
   int width, height;
   int bytesPerPixel, rowSizeBytes, colorPlane;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
   int hMag, vMag, sMag, bMag;
   int colorLoLf, colorLoMd, colorLoRt;
   int colorMdRt, colorHiRt, colorHiMd;
   int colorHiLf, colorMdLf, colorMdMd;
   int offset, offsetLo, offsetMd, offsetHi;
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

   for(y = yBeg; y <= yEnd; y++) {

      /* Pixel data first pixel = top-left; everything else bottom-left */
      offsetMd = ((height - y - 1) * rowSizeBytes) + bytesPerPixel + colorPlane;
      offsetHi = offsetMd - rowSizeBytes;
      offsetLo = offsetMd + rowSizeBytes;

      colorHiLf = img->pxl[offsetHi];
      colorMdLf = img->pxl[offsetMd];
      colorLoLf = img->pxl[offsetLo];

      offset = bytesPerPixel;

      colorHiMd = img->pxl[offsetHi + offset];
      colorMdMd = img->pxl[offsetMd + offset];
      colorLoMd = img->pxl[offsetLo + offset];

      offset += bytesPerPixel;

      colorHiRt = img->pxl[offsetHi + offset];
      colorMdRt = img->pxl[offsetMd + offset];
      colorLoRt = img->pxl[offsetLo + offset];

      for(x = xBeg; x <= xEnd; x++) {

         idx = y * width + x;

         /**
          * Calculate horizontal edge flow
          *   1  2  1
          *   0  0  0
          *  -1 -2 -1
          */
         hMag  =  colorHiLf;
         hMag += (colorHiMd << 1);
         hMag +=  colorHiRt;
         hMag -=  colorLoLf;
         hMag -= (colorLoMd << 1);
         hMag -=  colorLoRt;

         /**
          * Calculate vertical edge flow
          *  -1  0  1
          *  -2  0  2
          *  -1  0  1
          */
         vMag  =  colorHiRt;
         vMag += (colorMdRt << 1);
         vMag +=  colorLoRt;
         vMag -=  colorHiLf;
         vMag -= (colorMdLf << 1);
         vMag -=  colorLoLf;

         /**
          * Calculate "slash" edge flow
          *  -2 -1  0
          *  -1  0  1
          *   0  1  2
          */
         sMag  =  colorLoMd;
         sMag += (colorLoRt << 1);
         sMag +=  colorMdRt;
         sMag -=  colorHiMd;
         sMag -= (colorHiLf << 1);
         sMag -=  colorMdLf;

         /**
          * Calculate "backslash" edge flow
          *   0  1  2
          *  -1  0  1
          *  -2 -1  0
          */
         bMag  =  colorMdLf;
         bMag += (colorLoLf << 1);
         bMag +=  colorLoMd;
         bMag -=  colorMdRt;
         bMag -= (colorHiRt << 1);
         bMag -=  colorHiMd;

         /**
          * If implementing these operations using MMX, can load 2
          * registers with 4 doubleword values and subtract (PSUBD).
          */

         hFlowCache[idx].mag = hMag;
         vFlowCache[idx].mag = vMag;
         sFlowCache[idx].mag = sMag;
         bFlowCache[idx].mag = bMag;

         colorHiLf = colorHiMd;
         colorMdLf = colorMdMd;
         colorLoLf = colorLoMd;

         colorHiMd = colorHiRt;
         colorMdMd = colorMdRt;
         colorLoMd = colorLoRt;

         offset += bytesPerPixel;

         colorHiRt = img->pxl[offsetHi + offset];
         colorMdRt = img->pxl[offsetMd + offset];
         colorLoRt = img->pxl[offsetLo + offset];
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
PopulateHoughCache(struct HoughCache *houghCache, struct Flow *sFlowCache,
      struct Flow *bFlowCache, struct Flow *hFlowCache, struct Flow *vFlowCache)
{
   int idx, phi, d;
   int angleBase, imgExtent;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;

   houghCache->dExtent = HOUGH_D_EXTENT;
   houghCache->phiExtent = HOUGH_PHI_EXTENT;
   memset(houghCache->isMax, 0x01, sizeof(char) * HOUGH_D_EXTENT * HOUGH_PHI_EXTENT);
   memset(&(houghCache->mag), 0x00, sizeof(int) * HOUGH_D_EXTENT * HOUGH_PHI_EXTENT);

   imgExtent = houghCache->dExtent;
   angleBase = houghCache->phiExtent;

   xBeg = 2;
   xEnd = imgExtent - 3;
   yBeg = 2;
   yEnd = imgExtent - 3;

   for(y = yBeg; y <= yEnd; y++) {
      for(x = xBeg; x <= xEnd; x++) {

         idx = y * imgExtent + x;

         /* After finalizing algorithm, precalculate for 32x32 square:
          *   each product (x*uCos[phi]) and (y*uSin[phi]) (FAST) (8kB)
          *   or, map each (x,y,phi) to its (d,phi) location (FASTER) (262kB)
          *
          * This should provide a huge speedup.
          */
         if(abs(vFlowCache[idx].mag) > 0) {
            for(phi = 0; phi < 16; phi++) {
               d = GetOffset(x, y, phi, imgExtent);
               if(d == -1) continue;
               houghCache->mag[d * angleBase + phi] += abs(vFlowCache[idx].mag);
            }
            for(phi = 112; phi < angleBase; phi++) {
               d = GetOffset(x, y, phi, imgExtent);
               if(d == -1) continue;
               houghCache->mag[d * angleBase + phi] += abs(vFlowCache[idx].mag);
            }
         }

         if(abs(bFlowCache[idx].mag) > 0) {
            for(phi = 16; phi < 48; phi++) {
               d = GetOffset(x, y, phi, imgExtent);
               if(d == -1) continue;
               houghCache->mag[d * angleBase + phi] += abs(bFlowCache[idx].mag);
            }
         }

         if(abs(hFlowCache[idx].mag) > 0) {
            for(phi = 48; phi < 80; phi++) {
               d = GetOffset(x, y, phi, imgExtent);
               if(d == -1) continue;
               houghCache->mag[d * angleBase + phi] += abs(hFlowCache[idx].mag);
            }
         }

         if(abs(sFlowCache[idx].mag) > 0) {
            for(phi = 80; phi < 112; phi++) {
               d = GetOffset(x, y, phi, imgExtent);
               if(d == -1) continue;
               houghCache->mag[d * angleBase + phi] += abs(sFlowCache[idx].mag);
            }
         }
      }
   }
}

/**
 * Normalize hough quadrants in a very slow and inefficient way
 *
 */
static void
NormalizeHoughCache(struct HoughCache *houghCache,
      struct Flow *sFlowCache, struct Flow *bFlowCache,
      struct Flow *hFlowCache, struct Flow *vFlowCache)
{
   int          pixelCount;
   int          i, idx, phi, d;
   unsigned int sFlowSum, bFlowSum, hFlowSum, vFlowSum;
   double       sFlowScale, bFlowScale, hFlowScale, vFlowScale;
   double       normScale, phiScale;

   hFlowSum = vFlowSum = sFlowSum = bFlowSum = 0;

   pixelCount = houghCache->dExtent * houghCache->dExtent;

   for(i = 0; i < pixelCount; i++) {
      hFlowSum += abs(hFlowCache[i].mag);
      vFlowSum += abs(vFlowCache[i].mag);
      sFlowSum += abs(sFlowCache[i].mag);
      bFlowSum += abs(bFlowCache[i].mag);
   }

   hFlowScale = (double)65536/hFlowSum;
   vFlowScale = (double)65536/vFlowSum;
   sFlowScale = (double)65536/sFlowSum;
   bFlowScale = (double)65536/bFlowSum;

   for(phi = 0; phi < 128; phi++) {
      for(d = 0; d < LOCAL_SIZE; d++) {
         idx = d * 128 + phi;
         if(phi < 16 || phi >= 112)
            normScale = vFlowScale;
         else if(phi < 48)
            normScale = bFlowScale;
         else if(phi < 80)
            normScale = hFlowScale;
         else if(phi < 112)
            normScale = sFlowScale;
         else
            normScale = 1;

         phiScale = ((phi < 32 || phi >= 96) ? abs(uCos128[phi]) : uSin128[phi])/1024.0;

         houghCache->mag[idx] = (int)(houghCache->mag[idx] * normScale * phiScale + 0.5);
      }
   }
}

/**
 *
 *
 */
static void
MarkHoughMaxima(struct HoughCache *houghCache)
{
   int x, y;
   int idx0, idx1;
   int width, height;

   width = houghCache->phiExtent;
   height = houghCache->dExtent;

   /* Find local maxima for each angle */
   for(x = 0; x < width; x++) {
      for(y = 0; y < height - 1; y++) {

         idx0 = y * width + x;
         idx1 = idx0 + width;

         if(houghCache->mag[idx0] == 0)
            houghCache->isMax[idx0] = 0;
         else if(houghCache->mag[idx0] > houghCache->mag[idx1])
            houghCache->isMax[idx1] = 0;
         else if(houghCache->mag[idx1] > houghCache->mag[idx0])
            houghCache->isMax[idx0] = 0;
      }
   }
}

/**
 *
 *
 */
static void
AddToAngleSort(struct HoughLineSort *stack, struct HoughLine line)
{
   int i, startHere;
   int dDiff, phiDiff;
   DmtxBoolean willGrow;

   /* Special case: first addition */
   if(stack->count == 0) {
      stack->lines[stack->count++] = line;
      return;
   }

   willGrow = (stack->count < stack->maxCount) ? DmtxTrue : DmtxFalse;
   startHere = stack->count - 1; /* Sort normally starts at weakest element */

   /* If stack already has entry for this angle+offset (or close) then either:
    *   a) Overwrite the old one without shifting (if stronger), or
    *   b) Reject the new one completely (if weaker)
    */
   for(i = 0; i < stack->count; i++) {
      phiDiff = abs(line.phi - stack->lines[i].phi);

      /* If phiDiff spans the 0/180 deg boundary then offsets flip */
      dDiff = (phiDiff > 119) ? abs(64 - line.d - stack->lines[i].d) :
            abs(line.d - stack->lines[i].d);

      if(dDiff < 3 && (phiDiff < 8 || phiDiff > 119)) {
         if(line.mag > stack->lines[i].mag) {
            startHere = i - 1;
            willGrow = DmtxFalse;
            stack->lines[i] = line;
            break;
         }
         else {
            return;
         }
      }
   }

   /* Shift weak entries downward */
   for(i = startHere; i >= 0; i--) {
      if(line.mag > stack->lines[i].mag) {
         if(i + 1 < stack->maxCount)
            stack->lines[i+1] = stack->lines[i];
         stack->lines[i] = line;
      }
   }

   /* Count changes only if shift occurs */
   if(willGrow == DmtxTrue)
      stack->count++;
}

/**
 *
 *
 */
static struct HoughLineSort
FindBestAngles(struct HoughCache *houghCache)
{
   int phiExtent, dExtent;
   int phi, d, idx;
   int magBest;
   struct HoughLine lineBest;
   struct HoughLineSort lineSort;

   phiExtent = houghCache->phiExtent;
   dExtent = houghCache->dExtent;

   memset(&lineSort, 0x00, sizeof(struct HoughLineSort));
   lineSort.maxCount = LINE_SORT_MAX;

   /* Add strongest line at each angle to sort */
   for(phi = 0; phi < phiExtent; phi++) {

      magBest = houghCache->mag[phi]; /* i.e., [0 * phiExtent + line.phi] */
      lineBest.phi = phi;
      lineBest.d = 0;
      lineBest.mag = magBest;

      for(d = 1; d < dExtent; d++) {
         idx = d * phiExtent + phi;
         if(houghCache->isMax[idx] && houghCache->mag[idx] > magBest) {
            magBest = houghCache->mag[idx];
            lineBest.phi = phi;
            lineBest.d = d;
            lineBest.mag = magBest;
         }
      }

      AddToAngleSort(&lineSort, lineBest);
   }

   return lineSort;
}

/**
 * period is the width of a black/white block pair
 * period of 2.25 would pass periodScaled = 9, scale = 4 (9/4 == 2.25)
 * scale = 5, periodScaled = 10: X X X X X X X   <-- 1 chunk each
 * scale = 5, periodScaled = 20: XX  XX  XX  XX  <-- 2 chunks each
 */
static void
SetTimingPattern(int periodScaled, int scale, int center, char pattern[], int patternSize)
{
   int i, j;
   int chunks, localChunks;
   int centerAdjust;
   char *ptr, *ptrEnd;

   memset(pattern, 0x00, sizeof(char) * patternSize);

   chunks = periodScaled / (2 * scale);

   centerAdjust = center - chunks/2; /* i.e., center - start */
   while(centerAdjust > 0)
      centerAdjust -= periodScaled;

   ptr = pattern;
   ptrEnd = pattern + 64;

   for(i = 0; ptr < ptrEnd; i += periodScaled) {
      ptr = pattern + i/scale + centerAdjust;

      localChunks = ((i+periodScaled)/scale - i/scale)/2;

      for(j = 0; j < localChunks && ptr < ptrEnd; j++, ptr++)
         if(ptr >= pattern)
            *ptr = 1;
      for(j = 0; j < localChunks && ptr < ptrEnd; j++, ptr++)
         if(ptr >= pattern)
            *ptr = -1;
   }
}

/**
 *
 */
static struct Timing
FindGridTiming(struct HoughCache *hough, struct HoughLineSort *sort)
{
   int i;
   struct Timing t, tBest;

   /* Find best timing for strongest lines and retain best overall */
   for(i = 0; i < sort->count; i++) {
      t = FindGridTimingAtLine(hough, sort->lines[i]);
      if(i == 0 || t.mag > tBest.mag)
         tBest = t;
   }

   return tBest;
}

/**
 *
 */
static struct Timing
FindGridTimingAtLine(struct HoughCache *houghCache, struct HoughLine line)
{
   int phiExtent, dExtent;
   int scale, periodScaled, d, shift, idx;
   char pattern[LOCAL_SIZE];
   struct Timing timing, timingBest;

   phiExtent = houghCache->phiExtent;
   dExtent = houghCache->dExtent;

   memset(&timingBest, 0x00, sizeof(timingBest));
   timingBest.mag = -1;

   scale = 7;
   shift = line.d; /* logic needs confirmation */
   for(periodScaled = 2 * scale; periodScaled < (LOCAL_SIZE * scale)/4; periodScaled++) {

      timing.angle = line.phi;
      timing.periodScaled = periodScaled;
      timing.scale = scale;
      timing.shift = shift;

      SetTimingPattern(periodScaled, scale, shift, pattern, LOCAL_SIZE);

      /* Accumulate timing fit for each offset */
      timing.mag = 0;
      for(d = 0; d < dExtent; d++) {
         idx = d * phiExtent + line.phi;
         timing.mag += (pattern[d] * houghCache->mag[idx]);
      }
      timing.mag = abs(timing.mag); /* XXX oversimplifying for now -- careful for later */

      /* XXX tune this scaling later -- emphasizes small periods */
      timing.mag = (timing.mag * LOCAL_SIZE * scale)/(periodScaled);

      if(timing.mag > timingBest.mag)
         timingBest = timing;
   }

   return timingBest;
}

/**
 *
 */
static void
BlitFlowCache(SDL_Surface *screen, struct Flow *flowCache, int maxFlowMag, int screenX, int screenY)
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

/**
 *
 *
 */
static void
BlitHoughCache(SDL_Surface *screen, struct HoughCache *houghCache, int screenX, int screenY)
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
         if(houghCache->isMax[row * width + col] == 0)
            continue;

         if(houghCache->mag[row * width + col] > maxVal)
            maxVal = houghCache->mag[row * width + col];
      }
   }

   for(row = 0; row < height; row++) {
      for(col = 0; col < width; col++) {

         cache = houghCache->mag[row * width + col];

         if(houghCache->isMax[row * width + col] > 2) {
            rgb[0] = 255;
            rgb[1] = rgb[2] = 0;
         }
         else if(houghCache->isMax[row * width + col] == 1) {
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
BlitActiveRegion(SDL_Surface *screen, SDL_Surface *active, int zoom, int screenX, int screenY)
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
      src = zoomSurface(active, 2.0, 2.0, 1 /* smoothing on */);
      SDL_BlitSurface(src, NULL, screen, &clipRect);
      SDL_FreeSurface(src);
   }
}

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

   if(Ray2Intersect(&tTmp, rBtm, ray) == DmtxPass && tTmp >= 0.0 && tTmp < extent)
      dmtxPointAlongRay2(&(p[tCount++]), &rBtm, tTmp);

   if(Ray2Intersect(&tTmp, rTop, ray) == DmtxPass && tTmp >= 0.0 && tTmp < extent)
      dmtxPointAlongRay2(&(p[tCount++]), &rTop, tTmp);

   if(Ray2Intersect(&tTmp, rLft, ray) == DmtxPass && tTmp >= 0.0 && tTmp < extent)
      dmtxPointAlongRay2(&(p[tCount++]), &rLft, tTmp);

   if(Ray2Intersect(&tTmp, rRgt, ray) == DmtxPass && tTmp >= 0.0 && tTmp < extent)
      dmtxPointAlongRay2(&(p[tCount++]), &rRgt, tTmp);

   if(tCount != 2)
      return DmtxFail;

   *p0 = p[0];
   *p1 = p[1];

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

   lineColor(screen, x00, y00, x10, y10, 0x0000ffff);
   lineColor(screen, x10, y10, x11, y11, 0x0000ffff);
   lineColor(screen, x11, y11, x01, y01, 0x0000ffff);
   lineColor(screen, x01, y01, x00, y00, 0x0000ffff);
}

static void
DrawLine(SDL_Surface *screen, int baseExtent, int screenX, int screenY, int phi, int d, int displayScale)
{
   double phiRad;
   double dScaled;
   DmtxVector2 bb0, bb1;
   DmtxVector2 p0, p1;
   DmtxRay2 rStart, rLine;
   DmtxPixelLoc d0, d1;

   bb0.X = bb0.Y = 0.0;
   bb1.X = bb1.Y = baseExtent - 1;

   rStart.p.X = rStart.p.Y = 0.0;

   phiRad = phi * M_PI/128.0;

   rStart.v.X = cos(phiRad);
   rStart.v.Y = sin(phiRad);

   rLine.v.X = -rStart.v.Y;
   rLine.v.Y = rStart.v.X;

   dScaled = AdjustOffset(d, phi, LOCAL_SIZE) * displayScale;

   dmtxPointAlongRay2(&(rLine.p), &rStart, dScaled);

   p0.X = p0.Y = p1.X = p1.Y = 0.0;

   if(IntersectBox(rLine, bb0, bb1, &p0, &p1) == DmtxFalse)
      return;

   d0.X = (int)(p0.X + 0.5) + screenX;
   d1.X = (int)(p1.X + 0.5) + screenX;

   d0.Y = screenY + (baseExtent - (int)(p0.Y + 0.5) - 1);
   d1.Y = screenY + (baseExtent - (int)(p1.Y + 0.5) - 1);

   lineColor(screen, d0.X, d0.Y, d1.X, d1.Y, 0xff0000ff);
}

static void
DrawPhiBox(SDL_Surface *screen, int extent, int screenX, int screenY, int phi, int d)
{
   Sint16 x1, y1, x2, y2;

   x1 = screenX + phi - 2;
   y1 = screenY + extent - d - 3;
   x2 = x1 + 4;
   y2 = y1 + 4;

   rectangleColor(screen, x1, y1, x2, y2, 0xff0000ff);
}

static void
DrawTimingLines(SDL_Surface *screen, struct Timing timing, int scale, int screenX, int screenY)
{
   int i;
   double period;

   period = (double)timing.periodScaled/timing.scale;

   for(i = -64 * scale; i <= 64 * scale; i++) {
      DrawLine(screen, 64 * scale, screenX, screenY, timing.angle,
            (int)((timing.shift + period * i) * scale + 0.5), scale);
   }
}

static void
DrawTimingDots(SDL_Surface *screen, struct Timing timing, int screenX, int screenY)
{
   int dScaled, shiftScaled;

   shiftScaled = timing.shift * timing.scale;

   for(dScaled = 0; dScaled < 64 * timing.scale; dScaled++) {
      if(abs(dScaled - shiftScaled) % timing.periodScaled == 0) {
         PlotPixel(screen, screenX + timing.angle, screenY + 63 - dScaled/timing.scale);
      }
   }
}
