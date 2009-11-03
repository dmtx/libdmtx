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

struct Edge {
   int count;
};

struct Hough {
   unsigned int mag;
};

/* Later replace these with integers for fixed point math */
static double unitSin32[] = {
   0.000000,  0.098017,  0.195090,  0.290285, 0.382683,  0.471397,  0.555570,  0.634393,
   0.707107,  0.773010,  0.831470,  0.881921, 0.923880,  0.956940,  0.980785,  0.995185,
   1.000000,  0.995185,  0.980785,  0.956940, 0.923880,  0.881921,  0.831470,  0.773010,
   0.707107,  0.634393,  0.555570,  0.471397, 0.382683,  0.290285,  0.195090,  0.098017 };

static double unitCos32[] = {
   1.000000,  0.995185,  0.980785,  0.956940,  0.923880,  0.881921,  0.831470,  0.773010,
   0.707107,  0.634393,  0.555570,  0.471397,  0.382683,  0.290285,  0.195090,  0.098017,
   0.000000, -0.098017, -0.195090, -0.290285, -0.382683, -0.471397, -0.555570, -0.634393,
  -0.707107, -0.773010, -0.831470, -0.881921, -0.923880, -0.956940, -0.980785, -0.995185 };

static double unitSin128[] = {
   0.000000,  0.024541,  0.049068,  0.073565,  0.098017,  0.122411,  0.146730,  0.170962,
   0.195090,  0.219101,  0.242980,  0.266713,  0.290285,  0.313682,  0.336890,  0.359895,
   0.382683,  0.405241,  0.427555,  0.449611,  0.471397,  0.492898,  0.514103,  0.534998,
   0.555570,  0.575808,  0.595699,  0.615232,  0.634393,  0.653173,  0.671559,  0.689541,
   0.707107,  0.724247,  0.740951,  0.757209,  0.773010,  0.788346,  0.803208,  0.817585,
   0.831470,  0.844854,  0.857729,  0.870087,  0.881921,  0.893224,  0.903989,  0.914210,
   0.923880,  0.932993,  0.941544,  0.949528,  0.956940,  0.963776,  0.970031,  0.975702,
   0.980785,  0.985278,  0.989177,  0.992480,  0.995185,  0.997290,  0.998795,  0.999699,
   1.000000,  0.999699,  0.998795,  0.997290,  0.995185,  0.992480,  0.989177,  0.985278,
   0.980785,  0.975702,  0.970031,  0.963776,  0.956940,  0.949528,  0.941544,  0.932993,
   0.923880,  0.914210,  0.903989,  0.893224,  0.881921,  0.870087,  0.857729,  0.844854,
   0.831470,  0.817585,  0.803208,  0.788346,  0.773010,  0.757209,  0.740951,  0.724247,
   0.707107,  0.689541,  0.671559,  0.653173,  0.634393,  0.615232,  0.595699,  0.575808,
   0.555570,  0.534998,  0.514103,  0.492898,  0.471397,  0.449611,  0.427555,  0.405241,
   0.382683,  0.359895,  0.336890,  0.313682,  0.290285,  0.266713,  0.242980,  0.219101,
   0.195090,  0.170962,  0.146730,  0.122411,  0.098017,  0.073565,  0.049068,  0.024541 };

static double unitCos128[] = {
   1.000000,  0.999699,  0.998795,  0.997290,  0.995185,  0.992480,  0.989177,  0.985278,
   0.980785,  0.975702,  0.970031,  0.963776,  0.956940,  0.949528,  0.941544,  0.932993,
   0.923880,  0.914210,  0.903989,  0.893224,  0.881921,  0.870087,  0.857729,  0.844854,
   0.831470,  0.817585,  0.803208,  0.788346,  0.773010,  0.757209,  0.740951,  0.724247,
   0.707107,  0.689541,  0.671559,  0.653173,  0.634393,  0.615232,  0.595699,  0.575808,
   0.555570,  0.534998,  0.514103,  0.492898,  0.471397,  0.449611,  0.427555,  0.405241,
   0.382683,  0.359895,  0.336890,  0.313682,  0.290285,  0.266713,  0.242980,  0.219101,
   0.195090,  0.170962,  0.146730,  0.122411,  0.098017,  0.073565,  0.049068,  0.024541,
   0.000000, -0.024541, -0.049068, -0.073565, -0.098017, -0.122411, -0.146730, -0.170962,
  -0.195090, -0.219101, -0.242980, -0.266713, -0.290285, -0.313682, -0.336890, -0.359895,
  -0.382683, -0.405241, -0.427555, -0.449611, -0.471397, -0.492898, -0.514103, -0.534998,
  -0.555570, -0.575808, -0.595699, -0.615232, -0.634393, -0.653173, -0.671559, -0.689541,
  -0.707107, -0.724247, -0.740951, -0.757209, -0.773010, -0.788346, -0.803208, -0.817585,
  -0.831470, -0.844854, -0.857729, -0.870087, -0.881921, -0.893224, -0.903989, -0.914210,
  -0.923880, -0.932993, -0.941544, -0.949528, -0.956940, -0.963776, -0.970031, -0.975702,
  -0.980785, -0.985278, -0.989177, -0.992480, -0.995185, -0.997290, -0.998795, -0.999699 };

/* XXX change this to -256 < x <= 256 but still call it unitSin128[] */
static int unitSin127[] = {
     0,    3,    6,    9,   12,   16,   19,   22,   25,   28,   31,   34,   37,   40,   43,   46,
    49,   51,   54,   57,   60,   63,   65,   68,   71,   73,   76,   78,   81,   83,   85,   88,
    90,   92,   94,   96,   98,  100,  102,  104,  106,  107,  109,  111,  112,  113,  115,  116,
   117,  118,  120,  121,  122,  122,  123,  124,  125,  125,  126,  126,  126,  127,  127,  127,
   127,  127,  127,  127,  126,  126,  126,  125,  125,  124,  123,  122,  122,  121,  120,  118,
   117,  116,  115,  113,  112,  111,  109,  107,  106,  104,  102,  100,   98,   96,   94,   92,
    90,   88,   85,   83,   81,   78,   76,   73,   71,   68,   65,   63,   60,   57,   54,   51,
    49,   46,   43,   40,   37,   34,   31,   28,   25,   22,   19,   16,   12,    9,    6,    3 };

static int unitCos127[] = {
  +127, +127, +127, +127, +126, +126, +126, +125, +125, +124, +123, +122, +122, +121, +120, +118,
  +117, +116, +115, +113, +112, +111, +109, +107, +106, +104, +102, +100,  +98,  +96,  +94,  +92,
   +90,  +88,  +85,  +83,  +81,  +78,  +76,  +73,  +71,  +68,  +65,  +63,  +60,  +57,  +54,  +51,
   +49,  +46,  +43,  +40,  +37,  +34,  +31,  +28,  +25,  +22,  +19,  +16,  +12,   +9,   +6,   +3,
    +0,   -3,   -6,   -9,  -12,  -16,  -19,  -22,  -25,  -28,  -31,  -34,  -37,  -40,  -43,  -46,
   -49,  -51,  -54,  -57,  -60,  -63,  -65,  -68,  -71,  -73,  -76,  -78,  -81,  -83,  -85,  -88,
   -90,  -92,  -94,  -96,  -98, -100, -102, -104, -106, -107, -109, -111, -112, -113, -115, -116,
  -117, -118, -120, -121, -122, -122, -123, -124, -125, -125, -126, -126, -126, -127, -127, -127 };

static struct UserOptions GetDefaultOptions(void);
static DmtxPassFail HandleArgs(struct UserOptions *opt, int *argcp, char **argvp[]);
static struct AppState InitAppState(void);
static SDL_Surface *SetWindowSize(int windowWidth, int windowHeight);
static DmtxPassFail HandleEvent(SDL_Event *event, struct AppState *state,
      SDL_Surface *picture, SDL_Surface **screen);
static DmtxPassFail NudgeImage(int windowExtent, int pictureExtent, Sint16 *imageLoc);
/*static void WriteDiagnosticImage(DmtxDecode *dec, char *imagePath);*/
static DmtxBoolean IsEdge(struct Flow flowTop, struct Flow flowMid, struct Flow flowBtm, double *pcntTop);
static void PopulateFlowCache(struct Flow *sFlowCache, struct Flow *bFlowCache,
      DmtxImage *img, int width, int height);
static void PopulateEdgeCache(struct Edge *sEdgeCache, struct Edge *bEdgeCache, struct Flow *sFlowCache, struct Flow *bFlowCache, int width, int height);
static void PopulateHoughCache(struct Hough *pHoughCache, struct Hough *nHoughCache, struct Edge *sEdgeCache, struct Edge *bEdgeCache, int width, int height, int diag);
static void PopulateTightCache(struct Hough *tight, struct Hough *pHoughCache, struct Hough *nHoughCache, int width, int height);
static void WriteFlowCacheImage(struct Flow *flow, int width, int height, char *imagePath);
static void WriteEdgeCacheImage(struct Edge *edge, int width, int height, char *imagePath);
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
   struct Flow       *sFlowCache, *bFlowCache;
   struct Edge       *sEdgeCache, *bEdgeCache;
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

   /* XXX Edge caches will actually have one fewer locations in height and width */
   sEdgeCache = (struct Edge *)calloc(width * height, sizeof(struct Edge));
   assert(sEdgeCache != NULL);

   bEdgeCache = (struct Edge *)calloc(width * height, sizeof(struct Edge));
   assert(bEdgeCache != NULL);

   diag = (int)(sqrt(width * width + height * height) + 0.5);

   pHoughCache = (struct Hough *)calloc(128 * 2 * diag, sizeof(struct Hough));
   assert(pHoughCache != NULL);

   nHoughCache = (struct Hough *)calloc(128 * 2 * diag, sizeof(struct Hough));
   assert(nHoughCache != NULL);

   tight = (struct Hough *)calloc(128 * 2 * diag, sizeof(struct Hough));
   assert(tight != NULL);

   SDL_LockSurface(picture);
   PopulateFlowCache(sFlowCache, bFlowCache, dec->image, width, height);
   SDL_UnlockSurface(picture);

   PopulateEdgeCache(sEdgeCache, bEdgeCache, sFlowCache, bFlowCache, width, height);

   PopulateHoughCache(pHoughCache, nHoughCache, sEdgeCache, bEdgeCache, width, height, diag);

   PopulateTightCache(tight, pHoughCache, nHoughCache, 128, 2 * diag);

   WriteFlowCacheImage(sFlowCache, width, height, "sFlowCache.pnm");
   WriteFlowCacheImage(bFlowCache, width, height, "bFlowCache.pnm");
   WriteEdgeCacheImage(sEdgeCache, width, height, "sEdgeCache.pnm");
   WriteEdgeCacheImage(bEdgeCache, width, height, "bEdgeCache.pnm");
   WriteHoughCacheImage(pHoughCache, 128, 2 * diag, "pHoughCache.pnm");
   WriteHoughCacheImage(nHoughCache, 128, 2 * diag, "nHoughCache.pnm");
   WriteHoughCacheImage(tight, 128, 2 * diag, "tHoughCache.pnm");

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
   free(bEdgeCache);
   free(sEdgeCache);
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

/**
 *
 *
 */
static void
PopulateFlowCache(struct Flow *sFlowCache, struct Flow  *bFlowCache,
      DmtxImage *img, int width, int height)
{
   int bytesPerPixel, rowSizeBytes, colorPlane;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
/* int vMag, hMag; */
   int sMag, bMag;
   int colorLoLf, colorLoMd, colorLoRt;
   int colorMdRt, colorHiRt, colorHiMd;
   int colorHiLf, colorMdLf, colorMdMd;
   int offset, offsetLo, offsetMd, offsetHi;
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

         /**
          *     slash positive = ...
          *     slash negative = ...
          * backslash positive = ...
          * backslash negative = ...
          */

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
   fprintf(stdout, "PopulateFlowCache time: %ldms\n", (1000000 *
         (tb.sec - ta.sec) + (tb.usec - ta.usec))/1000);
}

/**
 *
 *
 */
static DmtxBoolean
IsEdge(struct Flow flowTop, struct Flow flowMid, struct Flow flowBtm, double *pcntTop)
{
   int diffTop, diffBtm, diffMag;

   if(flowTop.mag == 0 && flowBtm.mag == 0) {
      return DmtxFalse;
   }
   /* All non-zero flows must have same sign */
   else if(flowTop.mag == 0) {
      if(flowMid.mag * flowBtm.mag < 0)
         return DmtxFalse;
   }
   /* All non-zero flows must have same sign */
   else if(flowBtm.mag == 0) {
      if(flowMid.mag * flowTop.mag < 0)
         return DmtxFalse;
   }
   /* All non-zero flows must have same sign */
   else {
      if(flowMid.mag * flowTop.mag < 0 || flowMid.mag * flowBtm.mag < 0)
         return DmtxFalse;
   }

   diffTop = flowTop.mag - flowMid.mag;
   diffBtm = flowMid.mag - flowBtm.mag;

   /* 2nd derivative has same sign -- no zero crossing */
   if(diffTop * diffBtm > 0)
      return DmtxFalse;

   diffMag = abs(diffTop) + abs(diffBtm);
   if(diffMag == 0)
      return DmtxFalse;

   *pcntTop = abs(diffBtm)/(double)diffMag;

   return DmtxTrue;
}

/**
 *
 *
 */
static void
PopulateEdgeCache(struct Edge *sEdgeCache, struct Edge *bEdgeCache, struct Flow *sFlowCache,
      struct Flow *bFlowCache, int width, int height)
{
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
   int offset, offsets[8];
   double pcntTop;
   struct Flow flowTop, flowMid, flowBtm;
   DmtxTime ta, tb;

   ta = dmtxTimeNow();

   offsets[0] = -width - 1;
   offsets[1] = -width;
   offsets[2] = -width + 1;
   offsets[3] = 1;
   offsets[4] = width + 1;
   offsets[5] = width;
   offsets[6] = width - 1;
   offsets[7] = -1;

   xBeg = 1;
   xEnd = width - 2;
   yBeg = 1;
   yEnd = height - 2;

   for(y = yBeg; y <= yEnd; y++) {
      for(x = xBeg; x <= xEnd; x++) {

         offset = y * width + x;

         /**
          * "Slash" edge:
          *
          *  i---h        i---h
          *  |top|        |top|         +---+
          *  j---d---c    j---d---c     | d |
          *      |mid|        |mid|     +---+---+
          *      a---b---g    a---b---g     | b |
          *          |btm|        |btm|     +---+
          *          e---f        e---f
          *
          *      Pixel        First        Second
          *      Value      Derivative   Derivative
          *     top = j      top = j       top = d
          *     mid = a      mid = a       btm = b
          *     btm = e      btm = e
          */
         flowMid = sFlowCache[offset];
         if(abs(flowMid.mag) > 10) {
            flowTop = sFlowCache[offset + offsets[6]];
            flowBtm = sFlowCache[offset + offsets[2]];
            if(IsEdge(flowTop, flowMid, flowBtm, &pcntTop) == DmtxTrue) {
               if(pcntTop > 0.67)
                  sEdgeCache[offset + offsets[5]].count += flowMid.mag;
               else if(pcntTop < 0.33)
                  sEdgeCache[offset + offsets[3]].count += flowMid.mag;
               else
                  sEdgeCache[offset + offsets[4]].count += flowMid.mag;
            }
         }

         /**
          * "Backslash" edge:
          *
          *          j---i        j---i
          *          |top|        |top|     +---+
          *      d---c---h    d---c---h     | c |
          *      |mid|        |mid|     +---+---+
          *  e---a---b    e---a---b     | a |
          *  |btm|        |btm|         +---+
          *  f---g        f---g
          *
          *      Pixel        First        Second
          *      Value      Derivative   Derivative
          *     top = c      top = c       top = c
          *     mid = a      mid = a       btm = a
          *     btm = f      btm = f
          */
         flowMid = bFlowCache[offset];
         if(abs(flowMid.mag) > 10) {
            flowTop = bFlowCache[offset + offsets[4]];
            flowBtm = bFlowCache[offset + offsets[0]];
            if(IsEdge(flowTop, flowMid, flowBtm, &pcntTop) == DmtxTrue) {
               if(pcntTop > 0.67)
                  bEdgeCache[offset + offsets[4]].count += flowMid.mag;
               else if(pcntTop < 0.33)
                  bEdgeCache[offset].count += flowMid.mag;
               else
                  bEdgeCache[offset + offsets[5]].count += flowMid.mag;
            }
         }
      }
   }

   tb = dmtxTimeNow();
   fprintf(stdout, "PopulateEdgeCache time: %ldms\n", (1000000 *
         (tb.sec - ta.sec) + (tb.usec - ta.usec))/1000);
}

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
PopulateHoughCache(struct Hough *pHoughCache, struct Hough *nHoughCache, struct Edge *sEdgeCache, struct Edge *bEdgeCache, int width, int height, int diag)
{
   int idx, phi, d;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
/* double dFloat; */
   DmtxTime ta, tb;

   xBeg = 2;
   xEnd = width - 3;
   yBeg = 2;
   yEnd = height - 3;

   ta = dmtxTimeNow();

   for(y = yBeg; y <= yEnd; y++) {
      for(x = xBeg; x <= xEnd; x++) {

         idx = y * width + x;

         /* 0-90 deg. Hough for 90-180 deg. "Backslash" edges */
         if(bEdgeCache[idx].count != 0) {
            for(phi = 0; phi < 128; phi++) {
/*          for(phi = 0; phi < 64; phi++) { */
               /* shift works, divide doesn't ... watch endianess */
               d = diag + ((x * unitCos127[phi] + y * unitSin127[phi]) >> 8);
/*             dFloat = (x * unitCos128[phi] + y * unitSin128[phi]);
               d = diag + ((dFloat < 0) ? (int)dFloat - 1 : (int)dFloat)/2; */
               assert(abs(d) < diag * 2);
               /* for now accumulate abs() */
               if(bEdgeCache[idx].count > 0) {
/*                pHoughCache[d * 128 + phi].mag += 1; */
                  pHoughCache[d * 128 + phi].mag += bEdgeCache[idx].count;
               }
               else {
/*                nHoughCache[d * 128 + phi].mag += 1; */
                  nHoughCache[d * 128 + phi].mag -= bEdgeCache[idx].count;
               }
            }
         }

         /* 90-180 deg. Hough for 0-90 deg. "Slash" edges */
         if(sEdgeCache[idx].count != 0) {
            for(phi = 0; phi < 128; phi++) {
/*          for(phi = 64; phi < 128; phi++) { */
               d = diag + ((x * unitCos127[phi] + y * unitSin127[phi]) >> 8);
/*             dFloat = (x * unitCos128[phi] + y * unitSin128[phi]);
               d = diag + ((dFloat < 0) ? (int)dFloat - 1 : (int)dFloat)/2; */
               assert(abs(d) < diag * 2);
               /* for now accumulate abs() */
               if(sEdgeCache[idx].count > 0) {
/*                pHoughCache[d * 128 + phi].mag += 1; */
                  pHoughCache[d * 128 + phi].mag += sEdgeCache[idx].count;
               }
               else {
/*                nHoughCache[d * 128 + phi].mag += 1; */
                  nHoughCache[d * 128 + phi].mag -= sEdgeCache[idx].count;
               }
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
WriteEdgeCacheImage(struct Edge *edgeCache, int width, int height, char *imagePath)
{
   int row, col;
   int maxVal;
   int rgb[3];
   int color;
   struct Edge edge;
   FILE *fp;

   fp = fopen(imagePath, "wb");
   if(fp == NULL)
      exit(1);

   fprintf(fp, "P6\n%d %d\n255\n", width, height);

   maxVal = 0;
   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {
         if(abs(edgeCache[row * width + col].count) > maxVal)
            maxVal = abs(edgeCache[row * width + col].count);
      }
   }

   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {
         edge = edgeCache[row * width + col];

         color = (int)((abs(edge.count) * 254.0)/maxVal + 0.5);

         rgb[0] = rgb[1] = rgb[2] = 0;

         if(edge.count > 0)
            rgb[0] = color;
         else if(edge.count < 0)
            rgb[1] = color;

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
