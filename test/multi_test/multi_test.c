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
#include "kiss_fftr.h"

#define LOCAL_SIZE            64
#define MAXIMA_SORT_MAX_COUNT  8
#define ANGLE_SORT_MAX_COUNT   8
#define TIMING_SORT_MAX_COUNT  8

#define NFFT                  64 /* FFT input size */
#define HOUGH_D_EXTENT        64
#define HOUGH_PHI_EXTENT     128

/* Layout constants */
#define CTRL_COL1_X          380
#define CTRL_COL2_X          445
#define CTRL_COL3_X          510
#define CTRL_COL4_X          575
#define CTRL_ROW1_Y            0
#define CTRL_ROW2_Y           65
#define CTRL_ROW3_Y          130
#define CTRL_ROW4_Y          195
#define CTRL_ROW5_Y          260
#define CTRL_ROW6_Y          324

struct UserOptions {
   const char *imagePath;
};

struct AppState {
   int         adjust;
   int         windowWidth;
   int         windowHeight;
   int         activeExtent;
   DmtxBoolean displayVanish;
   DmtxBoolean displayTiming;
   DmtxBoolean printValues;
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

struct HoughCache {
   int offExtent;
   int phiExtent;
   char isMax[HOUGH_D_EXTENT * HOUGH_PHI_EXTENT];
   unsigned int mag[HOUGH_D_EXTENT * HOUGH_PHI_EXTENT];
};

struct HoughMaximaSort {
   int count;
   int mag[MAXIMA_SORT_MAX_COUNT];
};

struct VanishPointSum {
   int phi;
   int mag;
};

struct VanishPointSort {
   int count;
   struct VanishPointSum vanishSum[ANGLE_SORT_MAX_COUNT];
};

struct Timing {
   int phi;
   double shift;
   double period;
   double mag;
};

struct TimingSort {
   int count;
   struct Timing timing[TIMING_SORT_MAX_COUNT];
};

struct FitRegion {
   DmtxMatrix3 raw2fit;
   DmtxMatrix3 fit2raw;
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
static void PopulateFlowCache(struct Flow *sFlow, struct Flow *bFlow,
      struct Flow *hFlow, struct Flow *vFlow, DmtxImage *img);
static int GetCompactOffset(int x, int y, int phiIdx, int extent);
static double UncompactOffset(double d, int phiIdx, int extent);
static void PopulateHoughCache(struct HoughCache *hough, struct Flow *sFlow, struct Flow *bFlow, struct Flow *hFlow, struct Flow *vFlow);
static void NormalizeHoughCache(struct HoughCache *hough, struct Flow *sFlow, struct Flow *bFlow, struct Flow *hFlow, struct Flow *vFlow);
static void MarkHoughMaxima(struct HoughCache *hough);
static void AddToVanishPointSort(struct VanishPointSort *sort, struct VanishPointSum vanishSum);
static struct VanishPointSort FindVanishPoints(struct HoughCache *hough);
static void AddToMaximaSort(struct HoughMaximaSort *sort, int maximaMag);
static struct VanishPointSum GetAngleSumAtPhi(struct HoughCache *hough, int phi);
static void AddToTimingSort(struct TimingSort *sort, struct Timing timing);
static struct TimingSort FindGridTiming(struct HoughCache *hough, struct VanishPointSort *sort, struct AppState *state);
static DmtxRay2 HoughLineToRay2(int phi, double d);
static DmtxPixelLoc Vector2ToPixelLoc(DmtxVector2 v);
static DmtxPassFail NormalizeRegion(struct FitRegion *normal, struct TimingSort *sort, SDL_Surface *screen);

/* Process visualization functions */
static void BlitFlowCache(SDL_Surface *screen, struct Flow *flowCache, int maxFlowMag, int screenY, int screenX);
static void BlitHoughCache(SDL_Surface *screen, struct HoughCache *hough, int screenY, int screenX);
static void BlitActiveRegion(SDL_Surface *screen, SDL_Surface *active, int zoom, int screenY, int screenX);
static void PlotPixel(SDL_Surface *surface, int x, int y);
static int Ray2Intersect(double *t, DmtxRay2 p0, DmtxRay2 p1);
static int IntersectBox(DmtxRay2 ray, DmtxVector2 bb0, DmtxVector2 bb1, DmtxVector2 *p0, DmtxVector2 *p1);
static void DrawActiveBorder(SDL_Surface *screen, int activeExtent);
static void DrawLine(SDL_Surface *screen, int baseExtent, int screenX, int screenY, int phi, double d, int displayScale);
static void DrawTimingLines(SDL_Surface *screen, struct Timing timing, int displayScale, int screenY, int screenX);
static void DrawVanishingPoints(SDL_Surface *screen, struct VanishPointSort sort, int screenY, int screenX);
static void DrawTimingDots(SDL_Surface *screen, struct Timing timing, int screenY, int screenX);

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
   DmtxImage         *img;
   DmtxDecode        *dec;
   struct Flow        sFlow[LOCAL_SIZE * LOCAL_SIZE];
   struct Flow        bFlow[LOCAL_SIZE * LOCAL_SIZE];
   struct Flow        hFlow[LOCAL_SIZE * LOCAL_SIZE];
   struct Flow        vFlow[LOCAL_SIZE * LOCAL_SIZE];
   struct HoughCache  hough;
   struct VanishPointSort vanishSort;
   struct TimingSort      timingSort;
   SDL_Rect           clipRect;
   SDL_Surface       *local, *localTmp;
   struct FitRegion   normal;
   DmtxPassFail       err;

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

   atexit(SDL_Quit);

   /* Initialize SDL library */
   if(SDL_Init(SDL_INIT_VIDEO) < 0) {
      fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
      exit(1);
   }

   screen = SetWindowSize(state.windowWidth, state.windowHeight);
   NudgeImage(state.windowWidth, picture->w, &state.imageLocX);
   NudgeImage(state.windowHeight, picture->h, &state.imageLocY);

   bgColorB = SDL_MapRGBA(screen->format, 100, 100, 100, 255);
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

      /* Start with blank canvas */
      SDL_FillRect(screen, NULL, bgColorB);

      /* Draw image to main canvas area */
      clipRect.w = CTRL_COL1_X - 1;
      clipRect.h = state.windowHeight;
      clipRect.x = 0;
      clipRect.y = 0;
      SDL_SetClipRect(screen, &clipRect);
      SDL_BlitSurface(picture, NULL, screen, &imageLoc);
      SDL_SetClipRect(screen, NULL);

      DrawActiveBorder(screen, state.activeExtent);

      SDL_LockSurface(local);
      PopulateFlowCache(sFlow, bFlow, hFlow, vFlow, dec->image);
      SDL_UnlockSurface(local);

      /* Write flow cache images to feedback panes */
      pixelCount = LOCAL_SIZE * LOCAL_SIZE;
      maxFlowMag = 0;
      for(i = 0; i < pixelCount; i++) {
         if(abs(hFlow[i].mag) > maxFlowMag)
            maxFlowMag = abs(hFlow[i].mag);

         if(abs(vFlow[i].mag) > maxFlowMag)
            maxFlowMag = abs(vFlow[i].mag);

         if(abs(sFlow[i].mag) > maxFlowMag)
            maxFlowMag = abs(sFlow[i].mag);

         if(abs(bFlow[i].mag) > maxFlowMag)
            maxFlowMag = abs(bFlow[i].mag);
      }

      BlitActiveRegion(screen, local, 1, CTRL_ROW1_Y, CTRL_COL1_X);
      BlitActiveRegion(screen, local, 1, CTRL_ROW1_Y, CTRL_COL2_X);
      BlitActiveRegion(screen, local, 1, CTRL_ROW1_Y, CTRL_COL3_X);
      BlitActiveRegion(screen, local, 1, CTRL_ROW1_Y, CTRL_COL4_X);

      BlitFlowCache(screen, vFlow, maxFlowMag, CTRL_ROW2_Y, CTRL_COL1_X);
      BlitFlowCache(screen, bFlow, maxFlowMag, CTRL_ROW2_Y, CTRL_COL2_X);
      BlitFlowCache(screen, hFlow, maxFlowMag, CTRL_ROW2_Y, CTRL_COL3_X);
      BlitFlowCache(screen, sFlow, maxFlowMag, CTRL_ROW2_Y, CTRL_COL4_X);

      /* Find relative size of hough quadrants */
      PopulateHoughCache(&hough, sFlow, bFlow, hFlow, vFlow);
      NormalizeHoughCache(&hough, sFlow, bFlow, hFlow, vFlow);
      BlitHoughCache(screen, &hough, CTRL_ROW3_Y, CTRL_COL1_X);

      MarkHoughMaxima(&hough);
      BlitHoughCache(screen, &hough, CTRL_ROW4_Y, CTRL_COL1_X);

      /* Find vanishing points */
      vanishSort = FindVanishPoints(&hough);
      if(state.displayVanish == DmtxTrue)
         DrawVanishingPoints(screen, vanishSort, CTRL_ROW3_Y, CTRL_COL1_X);

      /* Draw timing lines */
      BlitActiveRegion(screen, local, 2, CTRL_ROW3_Y, CTRL_COL3_X);
      timingSort = FindGridTiming(&hough, &vanishSort, &state);
      if(state.displayTiming == DmtxTrue) {
         for(i = 0; i < 2; i++) {
            if(i >= timingSort.count)
               continue;
            DrawTimingDots(screen, timingSort.timing[i], CTRL_ROW3_Y, CTRL_COL1_X);
            DrawTimingLines(screen, timingSort.timing[i], 2, CTRL_ROW3_Y, CTRL_COL3_X);
         }
      }

      /* Normalize region */
      err = NormalizeRegion(&normal, &timingSort, screen);
/*    DrawNormalizedRegion(); */
/*    BlitActiveRegion(screen, local, 2, CTRL_ROW4_Y, CTRL_COL3_X); */

      SDL_Flip(screen);
   }

   SDL_FreeSurface(localTmp);
   SDL_FreeSurface(local);

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

   state.adjust = DmtxTrue;
   state.windowWidth = 640;
   state.windowHeight = 480;
   state.activeExtent = 64;
   state.displayVanish = DmtxFalse;
   state.displayTiming = DmtxTrue;
   state.printValues = DmtxFalse;
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
            case SDLK_a:
               state->adjust = (state->adjust == DmtxTrue) ? DmtxFalse : DmtxTrue;
               break;
            case SDLK_p:
               state->printValues = (state->printValues == DmtxTrue) ? DmtxFalse : DmtxTrue;
               break;
            case SDLK_l:
               fprintf(stdout, "Image Location: (%d, %d)\n", state->imageLocX,  state->imageLocY);
               break;
            case SDLK_t:
               state->displayTiming = (state->displayTiming == DmtxTrue) ? DmtxFalse : DmtxTrue;
               break;
            case SDLK_v:
               state->displayVanish = (state->displayVanish == DmtxTrue) ? DmtxFalse : DmtxTrue;
               break;
            case SDLK_UP:
               state->imageLocY--;
               nudgeRequired = DmtxTrue;
               break;
            case SDLK_DOWN:
               state->imageLocY++;
               nudgeRequired = DmtxTrue;
               break;
            case SDLK_RIGHT:
               state->imageLocX++;
               nudgeRequired = DmtxTrue;
               break;
            case SDLK_LEFT:
               state->imageLocX--;
               nudgeRequired = DmtxTrue;
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
               if(state->activeExtent > 32)
                  state->activeExtent /= 2;
               break;
            case SDL_BUTTON_WHEELUP:
               if(state->activeExtent < 64)
                  state->activeExtent *= 2;
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
 * 3x3 Sobel Kernel
 */
static void
PopulateFlowCache(struct Flow *sFlow, struct Flow *bFlow,
      struct Flow *hFlow, struct Flow *vFlow, DmtxImage *img)
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

   memset(sFlow, 0x00, sizeof(struct Flow) * width * height);
   memset(bFlow, 0x00, sizeof(struct Flow) * width * height);
   memset(hFlow, 0x00, sizeof(struct Flow) * width * height);
   memset(vFlow, 0x00, sizeof(struct Flow) * width * height);

   for(y = yBeg; y <= yEnd; y++) {

      /* Pixel data first pixel = top-left; everything else bottom-left */
      offsetMd = ((height - y - 1) * rowSizeBytes) + colorPlane;
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

         hFlow[idx].mag = hMag;
         vFlow[idx].mag = vMag;
         sFlow[idx].mag = sMag;
         bFlow[idx].mag = bMag;

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

static int
GetCompactOffset(int x, int y, int phiIdx, int extent)
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

static double
UncompactOffset(double compactedOffset, int phiIdx, int extent)
{
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

   assert(extent > 0);
   scale = (posMax - negMax) / extent;

   return (compactedOffset * scale) + negMax;

/* fixed point version:
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
*/
}

/**
 *
 *
 */
static void
PopulateHoughCache(struct HoughCache *hough, struct Flow *sFlow,
      struct Flow *bFlow, struct Flow *hFlow, struct Flow *vFlow)
{
   int idx, phi, d;
   int angleBase, imgExtent;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;

   hough->offExtent = HOUGH_D_EXTENT;
   hough->phiExtent = HOUGH_PHI_EXTENT;
   memset(hough->isMax, 0x01, sizeof(char) * HOUGH_D_EXTENT * HOUGH_PHI_EXTENT);
   memset(&(hough->mag), 0x00, sizeof(int) * HOUGH_D_EXTENT * HOUGH_PHI_EXTENT);

   imgExtent = hough->offExtent;
   angleBase = hough->phiExtent;

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
          * That should provide a huge speedup.
          */
         if(abs(vFlow[idx].mag) > 0) {
            for(phi = 0; phi < 16; phi++) {
               d = GetCompactOffset(x, y, phi, imgExtent);
               if(d == -1) continue;
               hough->mag[d * angleBase + phi] += abs(vFlow[idx].mag);
            }
            for(phi = 112; phi < angleBase; phi++) {
               d = GetCompactOffset(x, y, phi, imgExtent);
               if(d == -1) continue;
               hough->mag[d * angleBase + phi] += abs(vFlow[idx].mag);
            }
         }

         if(abs(bFlow[idx].mag) > 0) {
            for(phi = 16; phi < 48; phi++) {
               d = GetCompactOffset(x, y, phi, imgExtent);
               if(d == -1) continue;
               hough->mag[d * angleBase + phi] += abs(bFlow[idx].mag);
            }
         }

         if(abs(hFlow[idx].mag) > 0) {
            for(phi = 48; phi < 80; phi++) {
               d = GetCompactOffset(x, y, phi, imgExtent);
               if(d == -1) continue;
               hough->mag[d * angleBase + phi] += abs(hFlow[idx].mag);
            }
         }

         if(abs(sFlow[idx].mag) > 0) {
            for(phi = 80; phi < 112; phi++) {
               d = GetCompactOffset(x, y, phi, imgExtent);
               if(d == -1) continue;
               hough->mag[d * angleBase + phi] += abs(sFlow[idx].mag);
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
NormalizeHoughCache(struct HoughCache *hough,
      struct Flow *sFlow, struct Flow *bFlow,
      struct Flow *hFlow, struct Flow *vFlow)
{
   int          pixelCount;
   int          i, idx, phi, d;
   unsigned int sFlowSum, bFlowSum, hFlowSum, vFlowSum;
   double       sFlowScale, bFlowScale, hFlowScale, vFlowScale;
   double       normScale, phiScale;

   hFlowSum = vFlowSum = sFlowSum = bFlowSum = 0;

   pixelCount = hough->offExtent * hough->offExtent;

   for(i = 0; i < pixelCount; i++) {
      hFlowSum += abs(hFlow[i].mag);
      vFlowSum += abs(vFlow[i].mag);
      sFlowSum += abs(sFlow[i].mag);
      bFlowSum += abs(bFlow[i].mag);
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

         hough->mag[idx] = (int)(hough->mag[idx] * normScale * phiScale + 0.5);
      }
   }
}

/**
 *
 *
 */
static void
MarkHoughMaxima(struct HoughCache *hough)
{
   int phi, offset;
   int idx0, idx1;

   /* Find local maxima for each angle */
   for(phi = 0; phi < hough->phiExtent; phi++) {
      for(offset = 0; offset < hough->offExtent - 1; offset++) {

         idx0 = offset * hough->phiExtent + phi;
         idx1 = idx0 + hough->phiExtent;

         if(hough->mag[idx0] == 0)
            hough->isMax[idx0] = 0;
         else if(hough->mag[idx0] > hough->mag[idx1])
            hough->isMax[idx1] = 0;
         else if(hough->mag[idx1] > hough->mag[idx0])
            hough->isMax[idx0] = 0;
      }
   }
}

/**
 *
 *
 */
static void
AddToVanishPointSort(struct VanishPointSort *sort, struct VanishPointSum vanishSum)
{
   int i, startHere;
   int phiDiff;
   DmtxBoolean willGrow;

   /* Special case: first addition */
   if(sort->count == 0) {
      sort->vanishSum[sort->count++] = vanishSum;
      return;
   }

   willGrow = (sort->count < ANGLE_SORT_MAX_COUNT) ? DmtxTrue : DmtxFalse;
   startHere = sort->count - 1; /* Sort normally starts at weakest element */

   /* If sort already has entry for this angle (or close) then either:
    *   a) Overwrite the old one without shifting (if stronger), or
    *   b) Reject the new one completely (if weaker)
    */
   for(i = 0; i < sort->count; i++) {
      phiDiff = abs(vanishSum.phi - sort->vanishSum[i].phi);

      if(phiDiff < 8 || phiDiff > 119) {
         /* Similar angle is already represented with stronger magnitude */
         if(vanishSum.mag < sort->vanishSum[i].mag) {
            return;
         }
         /* Found similar-but-weaker angle that will be overwritten */
         else {
            sort->vanishSum[i] = vanishSum;
            willGrow = DmtxFalse; /* Non-growing re-sort required */
            startHere = i - 1;
            break;
         }
      }
   }

   /* Shift weak entries downward */
   for(i = startHere; i >= 0; i--) {
      if(vanishSum.mag > sort->vanishSum[i].mag) {
         if(i + 1 < ANGLE_SORT_MAX_COUNT)
            sort->vanishSum[i+1] = sort->vanishSum[i];
         sort->vanishSum[i] = vanishSum;
      }
   }

   /* Count changes only if shift occurs */
   if(willGrow == DmtxTrue)
      sort->count++;
}

/**
 *
 *
 */
static struct VanishPointSort
FindVanishPoints(struct HoughCache *hough)
{
   int phi;
   struct VanishPointSort sort;

   memset(&sort, 0x00, sizeof(struct VanishPointSort));

   /* Add strongest line at each angle to sort */
   for(phi = 0; phi < hough->phiExtent; phi++)
      AddToVanishPointSort(&sort, GetAngleSumAtPhi(hough, phi));

   return sort;
}

/**
 *
 *
 */
static void
AddToMaximaSort(struct HoughMaximaSort *sort, int maximaMag)
{
   int i;

   /* If new entry would be weakest (or only) one in list, then append */
   if(sort->count == 0 || maximaMag < sort->mag[sort->count - 1]) {
      if(sort->count + 1 < MAXIMA_SORT_MAX_COUNT)
         sort->mag[sort->count++] = maximaMag;
      return;
   }

   /* Otherwise shift the weaker entries downward */
   for(i = sort->count - 1; i >= 0; i--) {
      if(maximaMag > sort->mag[i]) {
         if(i + 1 < MAXIMA_SORT_MAX_COUNT)
            sort->mag[i+1] = sort->mag[i];
         sort->mag[i] = maximaMag;
      }
   }

   if(sort->count < MAXIMA_SORT_MAX_COUNT)
      sort->count++;
}

/**
 *
 *
 */
static struct VanishPointSum
GetAngleSumAtPhi(struct HoughCache *hough, int phi)
{
   int offset, i;
   struct VanishPointSum vanishSum;
   struct HoughMaximaSort sort;

   memset(&sort, 0x00, sizeof(struct HoughMaximaSort));

   for(offset = 0; offset < hough->offExtent; offset++) {
      i = offset * hough->phiExtent + phi;
      if(hough->isMax[i])
         AddToMaximaSort(&sort, hough->mag[i]);
   }

   vanishSum.phi = phi;
   vanishSum.mag = 0;
   for(i = 0; i < 8; i++)
      vanishSum.mag += sort.mag[i];

   return vanishSum;
}

/**
 *
 *
 */
static void
AddToTimingSort(struct TimingSort *sort, struct Timing timing)
{
   int i;

   if(timing.mag < 1.0) /* XXX or some minimum threshold */
      return;

   /* If new entry would be weakest (or only) one in list, then append */
   if(sort->count == 0 || timing.mag < sort->timing[sort->count - 1].mag) {
      if(sort->count + 1 < TIMING_SORT_MAX_COUNT)
         sort->timing[sort->count++] = timing;
      return;
   }

   /* Otherwise shift the weaker entries downward */
   for(i = sort->count - 1; i >= 0; i--) {
      if(timing.mag > sort->timing[i].mag) {
         if(i + 1 < TIMING_SORT_MAX_COUNT)
            sort->timing[i+1] = sort->timing[i];
         sort->timing[i] = timing;
      }
   }

   if(sort->count < TIMING_SORT_MAX_COUNT)
      sort->count++;
}

/**
 *
 *
 */
static struct TimingSort
FindGridTiming(struct HoughCache *hough, struct VanishPointSort *vanishSort, struct AppState *state)
{
   int x, y, fitMag, fitMax, fitOff, attempts, iter;
   int i, vSortIdx, phi;
   kiss_fftr_cfg   cfg = NULL;
   kiss_fft_scalar rin[NFFT];
   kiss_fft_cpx    sout[NFFT/2+1];
   kiss_fft_scalar mag[NFFT/2+1];
   int maxIdx;
   struct Timing timing;
   struct TimingSort timingSort;

   memset(&timingSort, 0x00, sizeof(struct TimingSort));

   for(vSortIdx = 0; vSortIdx < vanishSort->count; vSortIdx++) {

      phi = vanishSort->vanishSum[vSortIdx].phi;

      /* Load FFT input array */
      for(i = 0; i < NFFT; i++) {
         rin[i] = (i < 64) ? hough->mag[i * hough->phiExtent + phi] : 0;
      }

      /* Execute FFT */
      memset(sout, 0x00, sizeof(kiss_fft_cpx) * (NFFT/2 + 1));
      cfg = kiss_fftr_alloc(NFFT, 0, 0, 0);
      kiss_fftr(cfg, rin, sout);
      free(cfg);

      /* Select best result */
      maxIdx = NFFT/9-1;
      for(i = 0; i < NFFT/9-1; i++)
         mag[i] = 0.0;
      for(i = NFFT/9-1; i < NFFT/2+1; i++) {
         mag[i] = sout[i].r * sout[i].r + sout[i].i * sout[i].i;
         if(mag[i] > mag[maxIdx])
            maxIdx = i;
      }

      timing.phi = phi;
      timing.period = NFFT / (double)maxIdx;
      timing.mag = mag[maxIdx];

      /* Find best offset */
      fitOff = fitMax = 0;
      attempts = (int)timing.period + 1;
      for(x = 0; x < attempts; x++) {
         fitMag = 0;
         for(iter = 0; ; iter++) {
            y = x + (int)(iter * timing.period);
            if(y >= 64)
               break;
            fitMag += hough->mag[y * hough->phiExtent + timing.phi];
         }
         if(x == 0 || fitMag > fitMax) {
            fitMax = fitMag;
            fitOff = x;
         }
      }
      timing.shift = fitOff;

      AddToTimingSort(&timingSort, timing);
   }

   return timingSort;
}

/**
 *
 *
 */
static DmtxRay2
HoughLineToRay2(int phi, double d)
{
   double phiRad;
   double dScaled;
   DmtxRay2 rStart, rLine;

   memset(&rStart, 0x00, sizeof(DmtxRay2));
   memset(&rLine, 0x00, sizeof(DmtxRay2));

   rStart.p.X = rStart.p.Y = 0.0;

   phiRad = phi * M_PI/128.0;

   rStart.v.X = cos(phiRad);
   rStart.v.Y = sin(phiRad);

   rLine.v.X = -rStart.v.Y;
   rLine.v.Y = rStart.v.X;

   dScaled = UncompactOffset(d, phi, LOCAL_SIZE);

   dmtxPointAlongRay2(&(rLine.p), &rStart, dScaled);

   return rLine;
}

/**
 *
 *
 */
static DmtxPixelLoc
Vector2ToPixelLoc(DmtxVector2 v)
{
   DmtxPixelLoc p;

   p.X = (int)(v.X + 0.5);
   p.Y = (int)(v.Y + 0.5);

   return p;
}

/**
 *
 *
 */
static DmtxPassFail
NormalizeRegion(struct FitRegion *normal, struct TimingSort *sort, SDL_Surface *screen)
{
   int i;
   DmtxRay2 hLines[4];
   DmtxVector2 p[4];
   DmtxPixelLoc l[4];
   DmtxPassFail err;

   /* Convert extreme Hough points to lines in image space */
   hLines[0] = HoughLineToRay2(sort->timing[0].phi, 0.0);
   hLines[1] = HoughLineToRay2(sort->timing[1].phi, 0.0);
   hLines[2] = HoughLineToRay2(sort->timing[0].phi, 63.0);
   hLines[3] = HoughLineToRay2(sort->timing[1].phi, 63.0);

   err = dmtxRay2Intersect(&p[0], &hLines[0], &hLines[1]);
   if(err == DmtxFail)
      return DmtxFail;

   err = dmtxRay2Intersect(&p[1], &hLines[1], &hLines[2]);
   if(err == DmtxFail)
      return DmtxFail;

   err = dmtxRay2Intersect(&p[2], &hLines[2], &hLines[3]);
   if(err == DmtxFail)
      return DmtxFail;

   err = dmtxRay2Intersect(&p[3], &hLines[3], &hLines[0]);
   if(err == DmtxFail)
      return DmtxFail;

   for(i = 0; i < 4; i++) {
      l[i] = Vector2ToPixelLoc(p[i]);
      l[i].X += 287;
      l[i].Y = 271 - l[i].Y;
   }

   lineColor(screen, l[0].X, l[0].Y, l[1].X, l[1].Y, 0xff0000ff);
   lineColor(screen, l[1].X, l[1].Y, l[2].X, l[2].Y, 0xff0000ff);
   lineColor(screen, l[2].X, l[2].Y, l[3].X, l[3].Y, 0xff0000ff);
   lineColor(screen, l[3].X, l[3].Y, l[0].X, l[0].Y, 0xff0000ff);

   dmtxMatrix3Identity(normal->raw2fit);
   dmtxMatrix3Identity(normal->fit2raw);

   return DmtxPass;
}

/**
 *
 */
static void
BlitFlowCache(SDL_Surface *screen, struct Flow *flowCache, int maxFlowMag, int screenY, int screenX)
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
BlitHoughCache(SDL_Surface *screen, struct HoughCache *hough, int screenY, int screenX)
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
         if(hough->isMax[row * width + col] == 0)
            continue;

         if(hough->mag[row * width + col] > maxVal)
            maxVal = hough->mag[row * width + col];
      }
   }

   for(row = 0; row < height; row++) {
      for(col = 0; col < width; col++) {

         cache = hough->mag[row * width + col];

         if(hough->isMax[row * width + col] > 2) {
            rgb[0] = 255;
            rgb[1] = rgb[2] = 0;
         }
         else if(hough->isMax[row * width + col] == 1) {
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
BlitActiveRegion(SDL_Surface *screen, SDL_Surface *active, int zoom, int screenY, int screenX)
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
      /* Smoothing option distorts symbol proportions -- DO NOT USE */
      src = zoomSurface(active, 2.0, 2.0, 0 /* smoothing */);
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
DrawLine(SDL_Surface *screen, int baseExtent, int screenX, int screenY,
      int phi, double d, int displayScale)
{
   int scaledExtent;
   DmtxVector2 bb0, bb1;
   DmtxVector2 p0, p1;
   DmtxRay2 rLine;
   DmtxPixelLoc d0, d1;

   scaledExtent = baseExtent * displayScale;
   bb0.X = bb0.Y = 0.0;
   bb1.X = bb1.Y = scaledExtent - 1;

   rLine = HoughLineToRay2(phi, d);
   dmtxVector2ScaleBy(&rLine.p, (double)displayScale);

   p0.X = p0.Y = p1.X = p1.Y = 0.0;

   if(IntersectBox(rLine, bb0, bb1, &p0, &p1) == DmtxFalse)
      return;

   d0.X = (int)(p0.X + 0.5) + screenX;
   d1.X = (int)(p1.X + 0.5) + screenX;

   d0.Y = screenY + (scaledExtent - (int)(p0.Y + 0.5) - 1);
   d1.Y = screenY + (scaledExtent - (int)(p1.Y + 0.5) - 1);

   lineColor(screen, d0.X, d0.Y, d1.X, d1.Y, 0xff0000ff);
}

/**
 *
 *
 */
static void
DrawTimingLines(SDL_Surface *screen, struct Timing timing, int displayScale,
      int screenY, int screenX)
{
   int i;

   for(i = -64; i <= 64; i++) {
      DrawLine(screen, 64, screenX, screenY, timing.phi,
            timing.shift + (timing.period * i), displayScale);
   }
}

/**
 *
 *
 */
static void
DrawVanishingPoints(SDL_Surface *screen, struct VanishPointSort sort, int screenY, int screenX)
{
   int sortIdx;
   DmtxPixelLoc d0, d1;
   Uint32 rgba;

   for(sortIdx = 0; sortIdx < sort.count; sortIdx++) {
      d0.X = d1.X = screenX + sort.vanishSum[sortIdx].phi;
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

static void
DrawTimingDots(SDL_Surface *screen, struct Timing timing, int screenY, int screenX)
{
   int i, d;

   for(i = 0; i < 64; i++) {
      d = (int)(i * timing.period + timing.shift);
      if(d >= 64)
         break;

      PlotPixel(screen, screenX + timing.phi, screenY + 63 - d);
   }
}
