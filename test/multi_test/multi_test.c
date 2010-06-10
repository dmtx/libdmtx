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
 * x Find grid at +-90 degress
 * x Display normalized view in realtime
 * x Switch back to fitted 0-1 unit region regardless of grid size (maybe?)
 * x Use consistent naming to hold grid lineCount vs. rowCount
 * / Start region growing function
 * o Consider storing FFT results in timing struct so multiple periods can be tried
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

#define max(N,M) ((N > M) ? N : M)
#define min(N,M) ((N < M) ? N : M)

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
#define CTRL_ROW5_Y          259
#define CTRL_ROW6_Y          324
#define CTRL_ROW7_Y          388

typedef struct UserOptions_struct {
   const char *imagePath;
} UserOptions;

typedef struct AppState_struct {
   int         adjust;
   int         windowWidth;
   int         windowHeight;
   int         imageWidth;
   int         imageHeight;
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
} AppState;

typedef struct Flow_struct {
   int mag;
} Flow;

typedef struct HoughCache_struct {
   int offExtent;
   int phiExtent;
   char isMax[HOUGH_D_EXTENT * HOUGH_PHI_EXTENT];
   unsigned int mag[HOUGH_D_EXTENT * HOUGH_PHI_EXTENT];
} HoughCache;

typedef struct HoughMaximaSort_struct {
   int count;
   int mag[MAXIMA_SORT_MAX_COUNT];
} HoughMaximaSort;

typedef struct VanishPointSum_struct {
   int phi;
   int mag;
} VanishPointSum;

typedef struct VanishPointSort_struct {
   int count;
   VanishPointSum vanishSum[ANGLE_SORT_MAX_COUNT];
} VanishPointSort;

typedef struct Timing_struct {
   int phi;
   double shift;
   double period;
   double mag;
} Timing;

typedef struct TimingSort_struct {
   int count;
   Timing timing[TIMING_SORT_MAX_COUNT];
} TimingSort;

typedef struct AlignmentGrid_struct {
   int rowCount;
   int colCount;
   DmtxMatrix3 raw2fitActive;
   DmtxMatrix3 raw2fitFull;
   DmtxMatrix3 fit2rawActive;
   DmtxMatrix3 fit2rawFull;
} AlignmentGrid;

typedef struct GridRegion_struct {
   AlignmentGrid grid;
   int x;
   int y;
   int width;
   int height;
   int sizeIdx;
} GridRegion;

typedef struct RegionLines_struct {
   int gridCount;
   Timing timing;
   double dA, dB;
   DmtxRay2 line[2];
} RegionLines;

/* All values in GridRegionGrowth should be negative because list
 * is combined with the positive values of DmtxSymbolSize enum */
typedef enum {
   GridRegionGrowthUp       = -5,
   GridRegionGrowthLeft     = -4,
   GridRegionGrowthDown     = -3,
   GridRegionGrowthRight    = -2,
   GridRegionGrowthError    = -1
} GridRegionGrowth;

typedef enum {
   DmtxBarNone     = 0x00,
   DmtxBarTiming   = 0x01 << 0,
   DmtxBarFinder   = 0x01 << 1,
   DmtxBarInterior = 0x01 << 2,
   DmtxBarExterior = 0x01 << 3
} DmtxBarType;

typedef struct PerimeterState_struct {
   int expandDirs;
   int boundary[4];
   int finderBarCount;
   int timingBarCount;
   int finderBarDirs;
   int timingBarDirs;
} PerimeterState;

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
static UserOptions GetDefaultOptions(void);
static DmtxPassFail HandleArgs(UserOptions *opt, int *argcp, char **argvp[]);
static AppState InitAppState(void);
static SDL_Surface *SetWindowSize(int windowWidth, int windowHeight);
static DmtxPassFail HandleEvent(SDL_Event *event, AppState *state,
      SDL_Surface *picture, SDL_Surface **screen);
static DmtxPassFail NudgeImage(int windowExtent, int pictureExtent, Sint16 *imageLoc);
/*static void WriteDiagnosticImage(DmtxDecode *dec, char *imagePath);*/

/* Image processing functions */
static void PopulateFlowCache(Flow *sFlow, Flow *bFlow, Flow *hFlow, Flow *vFlow, DmtxImage *img);
static int GetCompactOffset(int x, int y, int phiIdx, int extent);
static double UncompactOffset(double d, int phiIdx, int extent);
static void PopulateHoughCache(HoughCache *hough, Flow *sFlow, Flow *bFlow, Flow *hFlow, Flow *vFlow);
static void NormalizeHoughCache(HoughCache *hough, Flow *sFlow, Flow *bFlow, Flow *hFlow, Flow *vFlow);
static void MarkHoughMaxima(HoughCache *hough);
static void AddToVanishPointSort(VanishPointSort *sort, VanishPointSum vanishSum);
static VanishPointSort FindVanishPoints(HoughCache *hough);
static void AddToMaximaSort(HoughMaximaSort *sort, int maximaMag);
static VanishPointSum GetAngleSumAtPhi(HoughCache *hough, int phi);
static void AddToTimingSort(TimingSort *sort, Timing timing);
static TimingSort FindGridTiming(HoughCache *hough, VanishPointSort *sort, AppState *state);
static DmtxRay2 HoughLineToRay2(int phi, double d);
static DmtxPassFail BuildGridFromTimings(AlignmentGrid *grid, Timing vp0, Timing vp1, AppState *state);
static PerimeterState RegionGetPerimeterStats(DmtxImage *img, AlignmentGrid *grid, const GridRegion *region);
static int TallyJumps(int colors[], int extent, int contrast);
static int PerimeterEdgeTest(DmtxImage *img, AlignmentGrid *grid, const GridRegion *region, DmtxDirection dir);
static int GetSizeIdx(int a, int b);
static int RegionGetNextExpansion(const GridRegion *region, const PerimeterState *ps);
static DmtxPassFail RegionExpand(GridRegion *region, GridRegionGrowth growDir);
static DmtxPassFail FindRegionWithinGrid(GridRegion *region, DmtxImage *img, AlignmentGrid *grid, DmtxDecode *dec, SDL_Surface *screen, AppState *state);
static DmtxPassFail RegionUpdateCorners(DmtxMatrix3 fit2raw, DmtxMatrix3 raw2fit, DmtxVector2 p00, DmtxVector2 p10, DmtxVector2 p11, DmtxVector2 p01);

/* Process visualization functions */
static void BlitFlowCache(SDL_Surface *screen, Flow *flowCache, int maxFlowMag, int screenY, int screenX);
static void BlitHoughCache(SDL_Surface *screen, HoughCache *hough, int screenY, int screenX);
static void BlitActiveRegion(SDL_Surface *screen, SDL_Surface *active, int zoom, int screenY, int screenX);
static void PlotPixel(SDL_Surface *surface, int x, int y);
static int Ray2Intersect(double *t, DmtxRay2 p0, DmtxRay2 p1);
static int IntersectBox(DmtxRay2 ray, DmtxVector2 bb0, DmtxVector2 bb1, DmtxVector2 *p0, DmtxVector2 *p1);
static DmtxPassFail DecodeSymbol(GridRegion *region, PerimeterState *ps, DmtxDecode *dec);
static void DrawActiveBorder(SDL_Surface *screen, int activeExtent);
static void DrawLine(SDL_Surface *screen, int baseExtent, int screenX, int screenY, int phi, double d, int displayScale);
static void DrawTimingLines(SDL_Surface *screen, Timing timing, int displayScale, int screenY, int screenX);
static void DrawVanishingPoints(SDL_Surface *screen, VanishPointSort sort, int screenY, int screenX);
static void DrawTimingDots(SDL_Surface *screen, Timing timing, int screenY, int screenX);
static void DrawNormalizedRegion(SDL_Surface *screen, DmtxImage *img, AlignmentGrid *grid, AppState *state, int screenY, int screenX);
static int ReadModuleColor(DmtxImage *img, AlignmentGrid *region, int symbolRow, int symbolCol, int colorPlane);
static Sint16 Clamp(Sint16 x, Sint16 xMin, Sint16 extent);
static void DrawSymbolPreview(SDL_Surface *screen, DmtxImage *img, AlignmentGrid *grid, AppState *state, int screenY, int screenX);
static void DrawPerimeterPatterns(SDL_Surface *screen, GridRegion *region, PerimeterState *ps, AppState *state);
static void DrawPerimeterSide(SDL_Surface *screen, PerimeterState *ps, int x00, int y00, int x11, int y11, int dispModExtent, DmtxDirection dir);

int
main(int argc, char *argv[])
{
   UserOptions        opt;
   AppState           state;
   SDL_Surface       *screen;
   SDL_Surface       *picture;
   SDL_Event          event;
   SDL_Rect           imageLoc;
   Uint32             bgColorB, bgColorK;
   int                i, j;
   int                pixelCount, maxFlowMag;
   DmtxImage         *imgActive, *imgFull;
   DmtxDecode        *dec;
   Flow               sFlow[LOCAL_SIZE * LOCAL_SIZE];
   Flow               bFlow[LOCAL_SIZE * LOCAL_SIZE];
   Flow               hFlow[LOCAL_SIZE * LOCAL_SIZE];
   Flow               vFlow[LOCAL_SIZE * LOCAL_SIZE];
   HoughCache         hough;
   VanishPointSort    vPoints;
   TimingSort         timings;
   SDL_Rect           clipRect;
   SDL_Surface       *local, *localTmp;
   AlignmentGrid      grid;
   GridRegion         region;
   DmtxPassFail       err;
   DmtxBoolean        regionFound;
   int                phiDiff;
   double             periodRatio;

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
   state.imageWidth = picture->w;
   state.imageHeight = picture->h;

   atexit(SDL_Quit);

   /* Initialize SDL library */
   if(SDL_Init(SDL_INIT_VIDEO) < 0) {
      fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
      exit(1);
   }

   screen = SetWindowSize(state.windowWidth, state.windowHeight);
   NudgeImage(CTRL_COL1_X, picture->w, &state.imageLocX);
   NudgeImage(state.windowHeight, picture->h, &state.imageLocY);

   bgColorB = SDL_MapRGBA(screen->format, 100, 100, 100, 255);
   bgColorK = SDL_MapRGBA(screen->format, 0, 0, 0, 255);

   /* Create surface to hold image pixels to be scanned */
   local = SDL_CreateRGBSurface(SDL_SWSURFACE, LOCAL_SIZE, LOCAL_SIZE, 32, 0, 0, 0, 0);
   imgActive = dmtxImageCreate(local->pixels, local->w, local->h, DmtxPack32bppXRGB);
   assert(imgActive != NULL);

   /* Create another surface for scaling purposes */
   localTmp = SDL_CreateRGBSurface(SDL_SWSURFACE, LOCAL_SIZE, LOCAL_SIZE, 32, 0, 0, 0, 0);

   switch(picture->format->BytesPerPixel) {
      case 1:
         imgFull = dmtxImageCreate(picture->pixels, picture->w, picture->h, DmtxPack8bppK);
         break;
      case 3:
         imgFull = dmtxImageCreate(picture->pixels, picture->w, picture->h, DmtxPack24bppRGB);
         dmtxImageSetProp(imgFull, DmtxPropRowPadBytes,
               picture->pitch - (picture->w * picture->format->BytesPerPixel));
         break;
      case 4:
         imgFull = dmtxImageCreate(picture->pixels, picture->w, picture->h, DmtxPack32bppXRGB);
         dmtxImageSetProp(imgFull, DmtxPropRowPadBytes,
               picture->pitch - (picture->w * picture->format->BytesPerPixel));
         break;
      default:
         exit(1);
   }
   assert(imgFull != NULL);

   dec = dmtxDecodeCreate(imgFull, 1);
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
      PopulateFlowCache(sFlow, bFlow, hFlow, vFlow, imgActive);
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
      BlitHoughCache(screen, &hough, CTRL_ROW3_Y, CTRL_COL1_X + 1);

      MarkHoughMaxima(&hough);
      BlitHoughCache(screen, &hough, CTRL_ROW4_Y - 1, CTRL_COL1_X + 1);

      /* Find vanishing points */
      vPoints = FindVanishPoints(&hough);
      if(state.displayVanish == DmtxTrue)
         DrawVanishingPoints(screen, vPoints, CTRL_ROW3_Y, CTRL_COL1_X);

      /* Find and rank best timings for vanishing points */
      timings = FindGridTiming(&hough, &vPoints, &state);

      /* Test timing combinations for potential barcode regions */
      regionFound = DmtxFalse;
      for(i = 0; regionFound == DmtxFalse && i < timings.count; i++) {
         for(j = i+1; j < timings.count; j++) {
            phiDiff = abs(timings.timing[i].phi - timings.timing[j].phi);

            /* Reject combinations that deviate from right angle (phi == 64) */
            if(abs(64 - phiDiff) > 28) /* within +- ~40 deg */
               continue;

            /* Reject/alter combinations with large period ratio */
            periodRatio = timings.timing[i].period / timings.timing[j].period;
/*
            if(periodRatio < 0.5 || periodRatio > 2.0) {
               fprintf(stdout, "opp_1 ");
               fflush(stdout);
            }
*/
            /* XXX Additional criteria go here */

            /* Normalize region based on this angle combination */
            err = BuildGridFromTimings(&grid, timings.timing[i], timings.timing[j], &state);
            if(err == DmtxFail)
               continue; /* Keep trying */

            /* Draw timed and untimed region lines */
            BlitActiveRegion(screen, local, 2, CTRL_ROW3_Y, CTRL_COL3_X);
            if(state.displayTiming == DmtxTrue) {
               DrawTimingDots(screen, timings.timing[i], CTRL_ROW3_Y, CTRL_COL1_X);
               DrawTimingDots(screen, timings.timing[j], CTRL_ROW3_Y, CTRL_COL1_X);
               DrawTimingLines(screen, timings.timing[i], 2, CTRL_ROW3_Y, CTRL_COL3_X);
               DrawTimingLines(screen, timings.timing[j], 2, CTRL_ROW3_Y, CTRL_COL3_X);
            }

            /* Test for timing patterns */
            SDL_LockSurface(picture);
            DrawNormalizedRegion(screen, imgFull, &grid, &state, CTRL_ROW5_Y, CTRL_COL1_X + 1);
            DrawSymbolPreview(screen, imgFull, &grid, &state, CTRL_ROW5_Y, CTRL_COL3_X);
            err = FindRegionWithinGrid(&region, imgFull, &grid, dec, screen, &state);
            SDL_UnlockSurface(picture);

            if(err == DmtxPass) {
               regionFound = DmtxTrue;
               break;
            }
            else {
               regionFound = DmtxTrue;
               break;
            }
         }
      }

      if(state.printValues == DmtxTrue) {
         /* Dump FFT results here */
         state.printValues = DmtxFalse;
      }

      SDL_Flip(screen);
   }

   SDL_FreeSurface(localTmp);
   SDL_FreeSurface(local);

   dmtxDecodeDestroy(&dec);
   dmtxImageDestroy(&imgFull);
   dmtxImageDestroy(&imgActive);

   exit(0);
}

/**
 *
 *
 */
static UserOptions
GetDefaultOptions(void)
{
   UserOptions opt;

   memset(&opt, 0x00, sizeof(UserOptions));
   opt.imagePath = NULL;

   return opt;
}

/**
 *
 *
 */
static DmtxPassFail
HandleArgs(UserOptions *opt, int *argcp, char **argvp[])
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
static AppState
InitAppState(void)
{
   AppState state;

   state.adjust = DmtxTrue;
   state.windowWidth = 640;
   state.windowHeight = 518;
   state.imageWidth = 0;
   state.imageHeight = 0;
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
         SDL_HWSURFACE | SDL_DOUBLEBUF); /* | SDL_RESIZABLE); */

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
HandleEvent(SDL_Event *event, AppState *state, SDL_Surface *picture, SDL_Surface **screen)
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
      NudgeImage(CTRL_COL1_X, picture->w, &(state->imageLocX));
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
PopulateFlowCache(Flow *sFlow, Flow *bFlow,
      Flow *hFlow, Flow *vFlow, DmtxImage *img)
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
   colorPlane = 1; /* XXX need to make some decisions here */

   xBeg = 1;
   xEnd = width - 2;
   yBeg = 1;
   yEnd = height - 2;

   ta = dmtxTimeNow();

   memset(sFlow, 0x00, sizeof(Flow) * width * height);
   memset(bFlow, 0x00, sizeof(Flow) * width * height);
   memset(hFlow, 0x00, sizeof(Flow) * width * height);
   memset(vFlow, 0x00, sizeof(Flow) * width * height);

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

   assert(offset >= 0);
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
PopulateHoughCache(HoughCache *hough, Flow *sFlow,
      Flow *bFlow, Flow *hFlow, Flow *vFlow)
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
NormalizeHoughCache(HoughCache *hough,
      Flow *sFlow, Flow *bFlow,
      Flow *hFlow, Flow *vFlow)
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
MarkHoughMaxima(HoughCache *hough)
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
AddToVanishPointSort(VanishPointSort *sort, VanishPointSum vanishSum)
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
static VanishPointSort
FindVanishPoints(HoughCache *hough)
{
   int phi;
   VanishPointSort sort;

   memset(&sort, 0x00, sizeof(VanishPointSort));

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
AddToMaximaSort(HoughMaximaSort *sort, int maximaMag)
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
static VanishPointSum
GetAngleSumAtPhi(HoughCache *hough, int phi)
{
   int offset, i;
   VanishPointSum vanishSum;
   HoughMaximaSort sort;

   memset(&sort, 0x00, sizeof(HoughMaximaSort));

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
AddToTimingSort(TimingSort *sort, Timing timing)
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
static TimingSort
FindGridTiming(HoughCache *hough, VanishPointSort *vPoints, AppState *state)
{
   int x, y, fitMag, fitMax, fitOff, attempts, iter;
   int i, vSortIdx, phi;
   kiss_fftr_cfg   cfg = NULL;
   kiss_fft_scalar rin[NFFT];
   kiss_fft_cpx    sout[NFFT/2+1];
   kiss_fft_scalar mag[NFFT/2+1];
   int maxIdx;
   Timing timing;
   TimingSort timings;

   memset(&timings, 0x00, sizeof(TimingSort));

   for(vSortIdx = 0; vSortIdx < vPoints->count; vSortIdx++) {

      phi = vPoints->vanishSum[vSortIdx].phi;

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

      AddToTimingSort(&timings, timing);
   }

   return timings;
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
 *
 */
static DmtxPassFail
BuildGridFromTimings(AlignmentGrid *grid, Timing vp0, Timing vp1, AppState *state)
{
   RegionLines rl0, rl1, *flat, *steep;
   DmtxVector2 p00, p10, p11, p01;
   DmtxPassFail err;
   DmtxMatrix3 fit2raw, raw2fit;
   DmtxMatrix3 mScale, mTranslate, mTmp;

   /* (1) -- later compare all possible combinations for strongest pair */
   rl0.timing = vp0;
   rl0.gridCount = (int)((64.0 - vp0.shift)/vp0.period);
   rl0.dA = vp0.shift;
   rl0.dB = vp0.shift + vp0.period * rl0.gridCount; /* doesn't work but whatever */

   rl1.timing = vp1;
   rl1.gridCount = (int)((64.0 - vp1.shift)/vp1.period);
   rl1.dA = vp1.shift;
   rl1.dB = vp1.shift + vp1.period * rl1.gridCount; /* doesn't work but whatever */

   /* flat[0] is the bottom flat line */
   /* flat[1] is the top flat line */
   /* steep[0] is the left steep line */
   /* steep[1] is the right line */

   /* Line with angle closest to horizontal is flatest */
   if(abs(64 - rl0.timing.phi) < abs(64 - rl1.timing.phi)) {
      flat = &rl0;
      steep = &rl1;
   }
   else {
      flat = &rl1;
      steep = &rl0;
   }

   flat->line[0] = HoughLineToRay2(flat->timing.phi, flat->dA);
   flat->line[1] = HoughLineToRay2(flat->timing.phi, flat->dB);

   if(steep->timing.phi < 64) {
      steep->line[0] = HoughLineToRay2(steep->timing.phi, steep->dA);
      steep->line[1] = HoughLineToRay2(steep->timing.phi, steep->dB);
   }
   else {
      steep->line[0] = HoughLineToRay2(steep->timing.phi, steep->dB);
      steep->line[1] = HoughLineToRay2(steep->timing.phi, steep->dA);
   }

   err = dmtxRay2Intersect(&p00, &(flat->line[0]), &(steep->line[0]));
   if(err == DmtxFail)
      return DmtxFail;

   err = dmtxRay2Intersect(&p10, &(flat->line[0]), &(steep->line[1]));
   if(err == DmtxFail)
      return DmtxFail;

   err = dmtxRay2Intersect(&p11, &(flat->line[1]), &(steep->line[1]));
   if(err == DmtxFail)
      return DmtxFail;

   err = dmtxRay2Intersect(&p01, &(flat->line[1]), &(steep->line[0]));
   if(err == DmtxFail)
      return DmtxFail;

   err = RegionUpdateCorners(fit2raw, raw2fit, p00, p10, p11, p01);
   if(err == DmtxFail)
      return DmtxFail;

   grid->rowCount = flat->gridCount;
   grid->colCount = steep->gridCount;

   /* raw2fit: Final transformation fits single origin module */
   dmtxMatrix3Identity(mScale);
   dmtxMatrix3Multiply(grid->raw2fitActive, raw2fit, mScale);

   if(state->activeExtent == 64) {
      dmtxMatrix3Translate(mTranslate, state->imageLocX - 288,
            518 - (227 + state->imageLocY + state->imageHeight));
      dmtxMatrix3Multiply(grid->raw2fitFull, mTranslate, grid->raw2fitActive); /* not tested */
   }
   else {
      dmtxMatrix3Scale(mScale, 2.0, 2.0);
      dmtxMatrix3Multiply(mTmp, mScale, grid->raw2fitActive);
      dmtxMatrix3Copy(grid->raw2fitActive, mTmp);

      dmtxMatrix3Translate(mTranslate, state->imageLocX - 304,
            518 - (243 + state->imageLocY + state->imageHeight));
      dmtxMatrix3Multiply(grid->raw2fitFull, mTranslate, grid->raw2fitActive); /* not tested */
   }

   /* fit2raw: Abstract away display nuances of multi_test application */
   dmtxMatrix3Identity(mScale);
   dmtxMatrix3Multiply(grid->fit2rawActive, mScale, fit2raw);

   if(state->activeExtent == 64) {
      dmtxMatrix3Translate(mTranslate, 288 - state->imageLocX,
            227 + state->imageLocY + state->imageHeight - 518);
   }
   else {
      dmtxMatrix3Scale(mScale, 0.5, 0.5);
      dmtxMatrix3Multiply(mTmp, grid->fit2rawActive, mScale);
      dmtxMatrix3Copy(grid->fit2rawActive, mTmp);

      dmtxMatrix3Translate(mTranslate, 304 - state->imageLocX,
            243 + state->imageLocY + state->imageHeight - 518);
   }
   dmtxMatrix3Multiply(grid->fit2rawFull, grid->fit2rawActive, mTranslate);

   return DmtxPass;
}

/**
 * Populates edge array with bar types (even if returning false)
 * Returns true if boundaries meet end conditions
 */
static PerimeterState
RegionGetPerimeterStats(DmtxImage *img, AlignmentGrid *grid, const GridRegion *region)
{
   int i;
   PerimeterState ps;

   memset(&ps, 0x00, sizeof(PerimeterState));

   ps.expandDirs = DmtxDirNone;
   ps.finderBarCount = 0;
   ps.timingBarCount = 0;
   ps.finderBarDirs = DmtxDirNone;
   ps.timingBarDirs = DmtxDirNone;

   /* Test each edge of perimeter (e.g., DmtxBarTiming | DmtxBarExterior) */
   ps.boundary[0] = PerimeterEdgeTest(img, grid, region, DmtxDirUp);
   ps.boundary[1] = PerimeterEdgeTest(img, grid, region, DmtxDirLeft);
   ps.boundary[2] = PerimeterEdgeTest(img, grid, region, DmtxDirDown);
   ps.boundary[3] = PerimeterEdgeTest(img, grid, region, DmtxDirRight);

   /* Ensure libdmtx still defines directions in the required manner */
   assert(DmtxDirUp   == 0x01 << 0); assert(DmtxDirLeft  == 0x01 << 1);
   assert(DmtxDirDown == 0x01 << 2); assert(DmtxDirRight == 0x01 << 3);

   /* Count finder and timing bars, or if no bar then mark as expandable */
   for(i = 0; i < 4; i++) {
      if(ps.boundary[i] & DmtxBarFinder) {
         ps.finderBarCount++;
         ps.finderBarDirs |= (0x01 << i);
      }
      else if(ps.boundary[i] & DmtxBarTiming) {
         ps.timingBarCount++;
         ps.timingBarDirs |= (0x01 << i);
      }
      else {
         ps.expandDirs |= (0x01 << i); /* DmtxDirUp | DmtxDirLeft | etc... */
      }

      /* Edge can't be both a finder bar AND a timing bar */
      assert(!((ps.boundary[i] & DmtxBarFinder) && (ps.boundary[i] & DmtxBarTiming)));
   }

   return ps;
}

/**
 *
 *
 */
static int
TallyJumps(int colors[], int extent, int contrast)
{
   int i;
   int jumpMag;
   int jumpCount;
   int jumpThresh;
   int thisJumpDir, lastJumpDir;

   jumpThresh = (contrast * 4)/10;

   jumpCount = 0;
   lastJumpDir = 0; /* 0 == none / +1 == up / -1 == down */

/* this is actually an oversimplification ...
 * should take into account that evens need to be same color */
   for(i = 1; i < extent; i++) {
      jumpMag = colors[i] - colors[i-1];
      if(abs(jumpMag) >= jumpThresh) {
         thisJumpDir = (jumpMag > 0) ? +1 : -1;
         if(thisJumpDir != lastJumpDir) {
            lastJumpDir = thisJumpDir;
            jumpCount++;
         }
      }
   }

   return jumpCount;
}

/**
 *
 *
 */
static int
PerimeterEdgeTest(DmtxImage *img, AlignmentGrid *grid,
      const GridRegion *region, DmtxDirection dir)
{
   int extent;
   int i, row, col;
   int inner[26], outer[26];
   int bar;
   int colBeg, rowBeg;
   int colOut, rowOut;
   int innerAllAvg, innerEvnAvg, innerOddAvg;
   int outerAllAvg, outerEvnAvg, outerOddAvg;
   int contrast, contFinderExt, contFinderInt, contTimingAny;
   int jumpsInner, jumpsOuter;
   int allowedJumpErrors;

   assert(region->width <= 26 && region->height <= 26);
   assert(dir == DmtxDirUp || dir == DmtxDirLeft ||
         dir == DmtxDirDown || dir == DmtxDirRight);

   switch(dir) {
      case DmtxDirUp:
         colBeg = region->x;
         rowBeg = region->y + region->height - 1;
         colOut =  0;
         rowOut = +1;
         break;
      case DmtxDirLeft:
         colBeg = region->x;
         rowBeg = region->y;
         colOut = -1;
         rowOut =  0;
         break;
      case DmtxDirDown:
         colBeg = region->x;
         rowBeg = region->y;
         colOut =  0;
         rowOut = -1;
         break;
      case DmtxDirRight:
         colBeg = region->x + region->width - 1;
         rowBeg = region->y;
         colOut = +1;
         rowOut =  0;
         break;
      default:
         /* error */
         colBeg = rowBeg = colOut = rowOut = 0;
         break;
   }

   /* Note opposite meaning (DmtxDirUp means "top", extent is horizontal) */
   extent = (dir & DmtxDirVertical) ? region->width : region->height;

   /* Sample and hold colors at each module along both inner and outer edges */
   col = colBeg;
   row = rowBeg;
   for(i = 0; i < extent; i++) {
      inner[i] = ReadModuleColor(img, grid, row, col, 0);
      outer[i] = ReadModuleColor(img, grid, row + rowOut, col + colOut, 0);

      /* Note opposite meaning (DmtxDirUp means "top", extent is horizontal) */
      if(dir & DmtxDirVertical)
         col++;
      else
         row++;
   }

   /* Calculate averages along inner and outer edges */
   innerAllAvg = outerAllAvg = 0;
   innerEvnAvg = outerEvnAvg = 0;
   innerOddAvg = outerOddAvg = 0;
   for(i = 0; i < extent; i++) {
      innerAllAvg += inner[i];
      outerAllAvg += outer[i];

      if(i & 0x01) {
         innerOddAvg += inner[i];
         outerOddAvg += outer[i];
      }
      else {
         innerEvnAvg += inner[i];
         outerEvnAvg += outer[i];
      }
   }

   innerEvnAvg *= 2;
   outerEvnAvg *= 2;
   innerOddAvg *= 2;
   outerOddAvg *= 2;

   /* Determine contrast */
   contFinderExt = abs(outerAllAvg - innerAllAvg);
   contFinderInt = abs(outerEvnAvg - outerOddAvg);
   contTimingAny = abs(innerEvnAvg - innerOddAvg);

/*
 * 3) Internal Finder Bar test (useful? -- skip initially)
 *      contTmpA = abs(innerAllAvg - outerEvnAvg)
 *      contTmpB = abs(innerAllAvg - outerOddAvg)
 *      If max(contTmpA, contTmpB) is similar to contFinderInt then timing bar likely
 *
 * 4) Timing bar test (useful? -- skip initially)
 *      contTmpC = abs(innerEvnAvg - outerAllAvg)
 *      contTmpD = abs(innerOddAvg - outerAllAvg)
 *      If max(contTmpC, contTmpD) is similar to contTimingAny then timing bar likely
 */

   /* Choose best contrast */
   contrast = max(contTimingAny, max(contFinderExt, contFinderInt));
   contrast /= extent;

   /* Tally jumps */
   jumpsInner = TallyJumps(inner, extent, contrast);
   jumpsOuter = TallyJumps(outer, extent, contrast);

/* TallyFinderJumps()
   TallyTimingJumps() */

   allowedJumpErrors = max(1, extent/8);

   /* Test boundary conditions */
   if(jumpsOuter < allowedJumpErrors && jumpsInner < allowedJumpErrors) {
      bar = (DmtxBarFinder | DmtxBarExterior);
   }
   else if(jumpsOuter + 1 - extent < allowedJumpErrors && jumpsInner < allowedJumpErrors) {
      bar = (DmtxBarFinder | DmtxBarInterior);
   }
   else if(jumpsInner + 1 - extent < allowedJumpErrors && jumpsOuter < allowedJumpErrors) {
/*    if(innerEvnAvg is same color as outerAllAvg) */
         bar = (DmtxBarTiming | DmtxBarExterior);
/*    else if(innerOddAvg is same color as outerAllAvg)
         bar = (DmtxBarTiming | DmtxBarInterior);
      else
         bar = DmtxBarNone; */
   }
   else {
      bar = DmtxBarNone;
   }

   return bar;
}

/**
 *
 *
 */
static int
GetSizeIdx(int a, int b)
{
   int i, rows, cols;
   const int totalSymbolSizes = 30; /* better way to determine this? */

   for(i = 0; i < totalSymbolSizes; i++) {
      rows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, i);
      cols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, i);

      if((rows == a && cols == b) || (rows == b && cols == a))
         return i;
   }

   return DmtxUndefined;
}

/**
 * This function determines the best direction for expansion. It uses the
 * boundary status parameter to decide which directions are possible, and
 * maintains a square shape while growing for as long as possible to
 * maximize align-while-growing feature.
 */
static int
RegionGetNextExpansion(const GridRegion *region, const PerimeterState *ps)
{
   int sizeIdx;
   DmtxBoolean growVertical, growHorizontal;

   /* If region appears complete (i.e., holds 2 adjacent finder bars and
    * 2 timing bars) then test for valid size (DmtxSymbolSize enum) */
   if(ps->finderBarCount == 2 && ps->timingBarCount == 2 &&
         ps->finderBarDirs != (DmtxDirLeft | DmtxDirRight) &&
         ps->finderBarDirs != (DmtxDirUp | DmtxDirDown)) {

      sizeIdx = GetSizeIdx(region->width, region->height);

      assert(ps->timingBarDirs != (DmtxDirLeft | DmtxDirRight));
      assert(ps->timingBarDirs != (DmtxDirUp | DmtxDirDown));

      if(sizeIdx != DmtxUndefined)
         return sizeIdx; /* Success */
   }

   /* If all of the sides are valid but region still isn't an official
    * shape, then we need to force growth in a direction. be careful to not
    * destroy rectangle possibilities. may need to poll possible directions
    * to find best option. */
   /* TODO add this logic */

   growVertical = (ps->expandDirs & DmtxDirVertical) ? DmtxTrue : DmtxFalse;
   growHorizontal = (ps->expandDirs & DmtxDirHorizontal) ? DmtxTrue : DmtxFalse;

   /* Can't expand beyond largest single region size */
   if(region->width == 26)
      growHorizontal = DmtxFalse;
   if(region->height == 26)
      growVertical = DmtxFalse;
   assert(region->width <= 26 && region->height <= 26);

   /* If no expansion opportunities remain, end now */
   if(!(growHorizontal || growVertical))
      return GridRegionGrowthError;

   /* Can only grow in one direction; maintain square shape if possible */
   if(growVertical && growHorizontal) {
      if(region->height > region->width)
         growVertical = DmtxFalse;
      else
         growHorizontal = DmtxFalse;
   }
   assert(growVertical + growHorizontal == DmtxTrue + DmtxFalse);

   /* Decide on final direction */
   if(growVertical == DmtxTrue) {
      if(ps->expandDirs & DmtxDirUp)
         return GridRegionGrowthUp;
      else
         return GridRegionGrowthDown;
   }
   else {
      if(ps->expandDirs & DmtxDirRight)
         return GridRegionGrowthRight;
      else
         return GridRegionGrowthLeft;
   }

   return GridRegionGrowthError;
}

/**
 *
 *
 */
static DmtxPassFail
RegionExpand(GridRegion *region, GridRegionGrowth growDir)
{
/* int oldRowCount, oldColCount;
   DmtxMatrix3 mScale, mTrans, mTmp; */

   switch(growDir) {
      case GridRegionGrowthDown:
         region->y--;
         /* Fall through */
      case GridRegionGrowthUp:
         region->height++;
         break;

      case GridRegionGrowthLeft:
         region->x--;
         /* Fall through */
      case GridRegionGrowthRight:
         region->width++;
         break;

      default:
         return DmtxFail;
   }

   /* Adjust fit2raw */
/* dmtxMatrix3Scale(mScale, (double)region->rowCount/oldRowCount,
         (double)region->colCount/oldColCount);

   dmtxMatrix3Translate(mTrans,
         (growDir == GridRegionGrowthLeft) ? -1.0/region->colCount : 0.0,
         (growDir == GridRegionGrowthDown) ? -1.0/region->rowCount : 0.0);

   dmtxMatrix3Multiply(mTmp, region->fit2rawFull, mScale);
   dmtxMatrix3Multiply(region->fit2rawFull, mTmp, mTrans);

   dmtxMatrix3Multiply(mTmp, region->fit2rawActive, mScale);
   dmtxMatrix3Multiply(region->fit2rawActive, mTmp, mTrans); */

   /* Adjust raw2fit */
/* dmtxMatrix3Scale(mScale, (double)oldRowCount/region->rowCount,
         (double)oldColCount/region->colCount);

   dmtxMatrix3Translate(mTrans,
         (growDir == GridRegionGrowthLeft) ? 1.0/region->colCount : 0.0,
         (growDir == GridRegionGrowthDown) ? 1.0/region->rowCount : 0.0);

   dmtxMatrix3Multiply(mTmp, region->raw2fitFull, mTrans);
   dmtxMatrix3Multiply(region->raw2fitFull, mTmp, mScale);

   dmtxMatrix3Multiply(mTmp, region->raw2fitActive, mTrans);
   dmtxMatrix3Multiply(region->raw2fitActive, mTmp, mScale); */

   return DmtxPass;
}

/**
 * Next step: start with small 2-layer box and step each edge outward until
 * outer edge is approximately solid and inner edge is either approximately
 * solid-but-opposite or 50% of both colors.
 */
static DmtxPassFail
FindRegionWithinGrid(GridRegion *region, DmtxImage *img, AlignmentGrid *grid, DmtxDecode *dec,
      SDL_Surface *screen, AppState *state)
{
   int expand;
   PerimeterState ps;
   DmtxPassFail err;

   memset(region, 0x00, sizeof(GridRegion));

   region->grid = *grid; /* Capture local copy of grid for tweaking */
   region->x = region->grid.colCount / 2;
   region->y = region->grid.rowCount / 2;
   region->width = 2;
   region->height = 2;
   region->sizeIdx = DmtxUndefined;

   /* Grow region outward until success/failure condition is met */
   for(;;) {

      /* Generate bar-detection statistics for current perimeter */
      ps = RegionGetPerimeterStats(img, grid, region);
      DrawPerimeterPatterns(screen, region, &ps, state);

      /* Determine best expansion direction, or symbol size if complete */
      expand = RegionGetNextExpansion(region, &ps);
      if(expand == GridRegionGrowthError)
         break; /* Failure (further expansion not possible) */

      /* Expansion complete: Perimeter patterns form valid symbol size */
      if(expand >= DmtxSymbol10x10 && expand <= DmtxSymbol16x48) {
         region->sizeIdx = expand;
         err = DecodeSymbol(region, &ps, dec);
         return err; /* Success (unless something unexpected happens) */
      }
      assert(DmtxSymbol10x10 == 0 && DmtxSymbol16x48 > 0);

      /* Attempt to grow in determined direction */
      err = RegionExpand(region, expand);
      if(err == DmtxFail)
         break; /* Failure (error while attempting expansion) */

      /* Update region to reflect growth */
      /* err = BuildGridFromTimings(grid, vp0, vp1, NULL); */

   }

   return DmtxFail;
}

/**
 *
 *
 */
static DmtxPassFail
RegionUpdateCorners(DmtxMatrix3 fit2raw, DmtxMatrix3 raw2fit, DmtxVector2 p00,
      DmtxVector2 p10, DmtxVector2 p11, DmtxVector2 p01)
{
/* double xMax, yMax; */
   double tx, ty, phi, shx, scx, scy, skx, sky;
   double dimOT, dimOR, dimTX, dimRX; /* , ratio; */
   DmtxVector2 vOT, vOR, vTX, vRX, vTmp;
   DmtxMatrix3 m, mtxy, mphi, mshx, mscx, mscy, mscxy, msky, mskx;

/* xMax = (double)(dmtxDecodeGetProp(dec, DmtxPropWidth) - 1);
   yMax = (double)(dmtxDecodeGetProp(dec, DmtxPropHeight) - 1);

   if(p00.X < 0.0 || p00.Y < 0.0 || p00.X > xMax || p00.Y > yMax ||
         p01.X < 0.0 || p01.Y < 0.0 || p01.X > xMax || p01.Y > yMax ||
         p10.X < 0.0 || p10.Y < 0.0 || p10.X > xMax || p10.Y > yMax)
      return DmtxFail; */
   dimOT = dmtxVector2Mag(dmtxVector2Sub(&vOT, &p01, &p00)); /* XXX could use MagSquared() */
   dimOR = dmtxVector2Mag(dmtxVector2Sub(&vOR, &p10, &p00));
   dimTX = dmtxVector2Mag(dmtxVector2Sub(&vTX, &p11, &p01));
   dimRX = dmtxVector2Mag(dmtxVector2Sub(&vRX, &p11, &p10));

   /* Verify that sides are reasonably long */
/* if(dimOT <= 8.0 || dimOR <= 8.0 || dimTX <= 8.0 || dimRX <= 8.0)
      return DmtxFail; */

   /* Verify that the 4 corners define a reasonably fat quadrilateral */
/* ratio = dimOT / dimRX;
   if(ratio <= 0.5 || ratio >= 2.0)
      return DmtxFail; */

/* ratio = dimOR / dimTX;
   if(ratio <= 0.5 || ratio >= 2.0)
      return DmtxFail; */

   /* Verify this is not a bowtie shape */
/*
   if(dmtxVector2Cross(&vOR, &vRX) <= 0.0 ||
         dmtxVector2Cross(&vOT, &vTX) >= 0.0)
      return DmtxFail; */

/* if(RightAngleTrueness(p00, p10, p11, M_PI_2) <= dec->squareDevn)
      return DmtxFail;
   if(RightAngleTrueness(p10, p11, p01, M_PI_2) <= dec->squareDevn)
      return DmtxFail; */

   /* Calculate values needed for transformations */
   tx = -1 * p00.X;
   ty = -1 * p00.Y;
   dmtxMatrix3Translate(mtxy, tx, ty);

   phi = atan2(vOT.X, vOT.Y);
   dmtxMatrix3Rotate(mphi, phi);
   dmtxMatrix3Multiply(m, mtxy, mphi);

   dmtxMatrix3VMultiply(&vTmp, &p10, m);
   shx = -vTmp.Y / vTmp.X;
   dmtxMatrix3Shear(mshx, 0.0, shx);
   dmtxMatrix3MultiplyBy(m, mshx);

   scx = 1.0/vTmp.X;
   dmtxMatrix3Scale(mscx, scx, 1.0);
   dmtxMatrix3MultiplyBy(m, mscx);

   dmtxMatrix3VMultiply(&vTmp, &p11, m);
   scy = 1.0/vTmp.Y;
   dmtxMatrix3Scale(mscy, 1.0, scy);
   dmtxMatrix3MultiplyBy(m, mscy);

   dmtxMatrix3VMultiply(&vTmp, &p11, m);
   skx = vTmp.X;
   dmtxMatrix3LineSkewSide(mskx, 1.0, skx, 1.0);
   dmtxMatrix3MultiplyBy(m, mskx);

   dmtxMatrix3VMultiply(&vTmp, &p01, m);
   sky = vTmp.Y;
   dmtxMatrix3LineSkewTop(msky, sky, 1.0, 1.0);
   dmtxMatrix3Multiply(raw2fit, m, msky);

   /* Create inverse matrix by reverse (avoid straight matrix inversion) */
   dmtxMatrix3LineSkewTopInv(msky, sky, 1.0, 1.0);
   dmtxMatrix3LineSkewSideInv(mskx, 1.0, skx, 1.0);
   dmtxMatrix3Multiply(m, msky, mskx);

   dmtxMatrix3Scale(mscxy, 1.0/scx, 1.0/scy);
   dmtxMatrix3MultiplyBy(m, mscxy);

   dmtxMatrix3Shear(mshx, 0.0, -shx);
   dmtxMatrix3MultiplyBy(m, mshx);

   dmtxMatrix3Rotate(mphi, -phi);
   dmtxMatrix3MultiplyBy(m, mphi);

   dmtxMatrix3Translate(mtxy, -tx, -ty);
   dmtxMatrix3Multiply(fit2raw, m, mtxy);

   return DmtxPass;
}

/**
 *
 */
static void
BlitFlowCache(SDL_Surface *screen, Flow *flowCache, int maxFlowMag, int screenY, int screenX)
{
   int row, col;
   unsigned char rgb[3];
   int width, height;
   int offset;
   Flow flow;
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
BlitHoughCache(SDL_Surface *screen, HoughCache *hough, int screenY, int screenX)
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

/**
 *
 *
 */
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

/**
 *
 *
 */
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

/**
 *
 *
 */
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

/**
 *
 *
 */
static DmtxPassFail
DecodeSymbol(GridRegion *region, PerimeterState *ps, DmtxDecode *dec)
{
   DmtxMatrix3 m, mRot;
   DmtxVector2 p00, p10, p11, p01;
   DmtxPassFail err;
   DmtxRegion reg;
   DmtxMessage *msg;

   /* Determine origin */
   switch(ps->finderBarDirs) {
      case (DmtxDirUp | DmtxDirLeft):
         dmtxMatrix3Rotate(mRot, M_PI/2.0);
         break;
      case (DmtxDirLeft | DmtxDirDown):
         dmtxMatrix3Rotate(mRot, 0.0);
         break;
      case (DmtxDirDown | DmtxDirRight):
         dmtxMatrix3Rotate(mRot, -M_PI/2.0);
         break;
      case (DmtxDirRight | DmtxDirUp):
         dmtxMatrix3Rotate(mRot, M_PI);
         break;
      default:
         return DmtxFail;
   }

   dmtxMatrix3Multiply(m, mRot, region->grid.fit2rawFull);

   p00.X = p01.X = region->x * (1.0/region->grid.colCount);
   p10.X = p11.X = (region->x + region->width) * (1.0/region->grid.colCount);
   p00.Y = p10.Y = region->y * (1.0/region->grid.rowCount);
   p01.Y = p11.Y = (region->y + region->height) * (1.0/region->grid.rowCount);

   dmtxMatrix3VMultiplyBy(&p00, m);
   dmtxMatrix3VMultiplyBy(&p10, m);
   dmtxMatrix3VMultiplyBy(&p11, m);
   dmtxMatrix3VMultiplyBy(&p01, m);

   /* Update DmtxRegion with detected corners */
   err = dmtxRegionUpdateCorners(dec, &reg, p00, p10, p11, p01);
   if(err == DmtxFail)
      return err;

   reg.flowBegin.plane = 0; /* or 1, or 2 (0 = red, 1 = green, etc...) */
   reg.offColor = 120; /* XXX needs to be determined programatically */
   reg.onColor = 10; /* XXX needs to be determined programatically */
   reg.sizeIdx = region->sizeIdx;
   reg.symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, region->sizeIdx);
   reg.symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, region->sizeIdx);
   reg.mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, region->sizeIdx);
   reg.mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, region->sizeIdx);

   msg = dmtxDecodeMatrixRegion(dec, &reg, DmtxUndefined);
   if(msg == NULL)
      return DmtxFail;

   fwrite(msg->output, sizeof(char), msg->outputIdx, stdout);
   fputc('\n', stdout);

   return DmtxPass;
}

/**
 *
 *
 */
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

/**
 *
 *
 */
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
DrawTimingLines(SDL_Surface *screen, Timing timing, int displayScale,
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
DrawVanishingPoints(SDL_Surface *screen, VanishPointSort sort, int screenY, int screenX)
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

/**
 *
 *
 */
static void
DrawTimingDots(SDL_Surface *screen, Timing timing, int screenY, int screenX)
{
   int i, d;

   for(i = 0; i < 64; i++) {
      d = (int)(i * timing.period + timing.shift);
      if(d >= 64)
         break;

      PlotPixel(screen, screenX + timing.phi, screenY + 63 - d);
   }
}

/**
 *
 *
 */
static void
DrawNormalizedRegion(SDL_Surface *screen, DmtxImage *img,
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
}

/**
 *
 *
 */
static int
ReadModuleColor(DmtxImage *img, AlignmentGrid *region, int symbolRow,
      int symbolCol, int colorPlane)
{
   int err;
   int i;
   int color, colorTmp;
   double sampleX[] = { 0.5, 0.4, 0.5, 0.6, 0.5 };
   double sampleY[] = { 0.5, 0.5, 0.4, 0.5, 0.6 };
   DmtxVector2 p;

   color = 0;
   for(i = 0; i < 5; i++) {

      p.X = (1.0/region->colCount) * (symbolCol + sampleX[i]);
      p.Y = (1.0/region->rowCount) * (symbolRow + sampleY[i]);

      dmtxMatrix3VMultiplyBy(&p, region->fit2rawFull);

      /* Should use dmtxDecodeGetPixelValue() later to properly handle pixel skipping */
      err = dmtxImageGetPixelValue(img, p.X, p.Y, colorPlane, &colorTmp);
      if(err == DmtxFail)
         return 0;

      color += colorTmp;
   }

   return color/5;
}

/**
 *
 *
 */
static Sint16
Clamp(Sint16 x, Sint16 xMin, Sint16 extent)
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
static void
DrawSymbolPreview(SDL_Surface *screen, DmtxImage *img, AlignmentGrid *grid,
      AppState *state, int screenY, int screenX)
{
   DmtxVector2 pTmp, pCtr;
   DmtxVector2 gridTest;
   int shiftX, shiftY;
   int rColor, gColor, bColor, color;
   Sint16 x1, y1, x2, y2;
   int extent = 128;
   int modulesToDisplay = 16;
   int dispModExtent = extent/modulesToDisplay;
   int row, col;
   int rowBeg, colBeg;
   int rowEnd, colEnd;

   pTmp.X = pTmp.Y = state->activeExtent/2.0;
   dmtxMatrix3VMultiply(&pCtr, &pTmp, grid->raw2fitActive);

   gridTest.X = pCtr.X * grid->colCount * dispModExtent;
   gridTest.X += (gridTest.X >= 0.0) ? 0.5 : -0.5;
   shiftX = 64 - (int)gridTest.X;
   colBeg = (shiftX < 0) ? 0 : -shiftX/8 - 1;
   colEnd = max(colBeg + 17, grid->colCount);

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

         rColor = ReadModuleColor(img, grid, row, col, 0);
         gColor = ReadModuleColor(img, grid, row, col, 1);
         bColor = ReadModuleColor(img, grid, row, col, 2);
         color = (rColor << 24) | (gColor << 16) | (bColor << 8) | 0xff;

         x1 = col * dispModExtent + shiftX;
         x2 = x1 + dispModExtent - 1;

         x1 = Clamp(x1 + screenX, screenX, 128);
         x2 = Clamp(x2 + screenX, screenX, 128);

         boxColor(screen, x1, y1, x2, y2, color);
      }
   }
}

/**
 *
 *
 */
static void
DrawPerimeterPatterns(SDL_Surface *screen, GridRegion *region, PerimeterState *ps, AppState *state)
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

   DrawPerimeterSide(screen, ps, x00, y00, x11, y11, dispModExtent, DmtxDirUp);
   DrawPerimeterSide(screen, ps, x00, y00, x11, y11, dispModExtent, DmtxDirLeft);
   DrawPerimeterSide(screen, ps, x00, y00, x11, y11, dispModExtent, DmtxDirDown);
   DrawPerimeterSide(screen, ps, x00, y00, x11, y11, dispModExtent, DmtxDirRight);
}

/**
 *
 *
 */
static void
DrawPerimeterSide(SDL_Surface *screen, PerimeterState *ps, int x00, int y00,
      int x11, int y11, int dispModExtent, DmtxDirection dir)
{
   Sint16 xBeg, yBeg;
   Sint16 xEnd, yEnd;
   int extent = 128;
   const int screenX = CTRL_COL3_X;
   const int screenY = CTRL_ROW5_Y;
   Uint32 color;
   int boundary;

   switch(dir) {
      case DmtxDirUp:
         xBeg = x00;
         xEnd = x11;
         yBeg = yEnd = y11 - dispModExtent/2;
         boundary = ps->boundary[0];
         break;
      case DmtxDirLeft:
         yBeg = y00;
         yEnd = y11;
         xBeg = xEnd = x00 + dispModExtent/2;
         boundary = ps->boundary[1];
         break;
      case DmtxDirDown:
         xBeg = x00;
         xEnd = x11;
         yBeg = yEnd = y00 + dispModExtent/2;
         boundary = ps->boundary[2];
         break;
      case DmtxDirRight:
         yBeg = y00;
         yEnd = y11;
         xBeg = xEnd = x11 - dispModExtent/2;
         boundary = ps->boundary[3];
         break;
      default:
         xBeg = x00;
         yBeg = y00;
         xEnd = x11;
         yEnd = y11;
         boundary = 0;
         break;
   }

   yBeg = (extent - 1 - yBeg);
   yEnd = (extent - 1 - yEnd);

   xBeg = Clamp(xBeg + screenX, screenX, 128);
   yBeg = Clamp(yBeg + screenY, screenY, 128);

   xEnd = Clamp(xEnd + screenX, screenX, 128);
   yEnd = Clamp(yEnd + screenY, screenY, 128);

   if(boundary & DmtxBarFinder)
      color = 0x0000ffff;
   else if(boundary & DmtxBarTiming)
      color = 0xff0000ff;
   else
      color = 0x00ff00ff;

   lineColor(screen, xBeg, yBeg, xEnd, yEnd, color);
}
