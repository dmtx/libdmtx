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
#include <assert.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "../../dmtx.h"

#define ArriveMask      0xf0
#define ArriveConnected 0x80
#define ArriveDirection 0x70
#define DepartMask      0x0f
#define DepartConnected 0x08
#define DepartDirection 0x07
#define Unvisited       0x00
#define Poisoned        0x11

static int xPattern[]     = { -1,  0,  1,  1,  1,  0, -1, -1 };
/*static int yPattern[]     = { -1, -1, -1,  0,  1,  1,  1,  0 };*/
static int yPatternFlip[] = {  1,  1,  1,  0, -1, -1, -1,  0 };

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
   unsigned char dir;
   Uint16        mag;
};

struct Edge {
   unsigned char dir;
   int count;
   int up, down, left, right;
   int mag;
};

struct Hough {
   unsigned int mag;
};

static struct UserOptions GetDefaultOptions(void);
static DmtxPassFail HandleArgs(struct UserOptions *opt, int *argcp, char **argvp[]);
static struct AppState InitAppState(void);
static SDL_Surface *SetWindowSize(int windowWidth, int windowHeight);
static DmtxPassFail HandleEvent(SDL_Event *event, struct AppState *state,
      SDL_Surface *picture, SDL_Surface **screen);
static DmtxPassFail NudgeImage(int windowExtent, int pictureExtent, Sint16 *imageLoc);
/*static void WriteDiagnosticImage(DmtxDecode *dec, char *imagePath);*/
static void PopulateFlowCache(struct Flow *vFlowCache, struct Flow *hFlowCache,
      DmtxImage *img, int width, int height);
static void PopulateEdgeCache(struct Edge *edgeCache, struct Flow *vFlowCache, struct Flow *hFlowCache, int width, int height);
static void ConnectEdges(struct Edge *edgeCache, int width, int height);
static void PopulateHoughCache(struct Hough *houghCache, struct Flow *flowCache, int width, int height);
static void WriteFlowCacheImage(struct Flow *flow, int width, int height, char *imagePath);
static void WriteEdgeCacheImage(struct Edge *edge, int width, int height, char *imagePath);
static void WriteHoughCacheImage(struct Hough *hough, int width, int height, char *imagePath);
static void PlotPixel(SDL_Surface *surface, int x, int y);

int main(int argc, char *argv[])
{
   struct UserOptions opt;
   struct AppState    state;
   int                scanX, scanY;
   int                width, height;
   SDL_Surface       *screen;
   SDL_Surface       *picture;
   SDL_Event          event;
   SDL_Rect           imageLoc;
   DmtxImage         *img;
   DmtxDecode        *dec;
   struct Flow       *vFlowCache, *hFlowCache;
   struct Edge       *edgeCache, edge;
   struct Hough      *houghCache;

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

   vFlowCache = (struct Flow *)calloc(width * height, sizeof(struct Flow));
   assert(vFlowCache != NULL);
   hFlowCache = (struct Flow *)calloc(width * height, sizeof(struct Flow));
   assert(hFlowCache != NULL);

   /* XXX Edge caches will actually have one fewer locations in height and width */
   edgeCache = (struct Edge *)calloc(width * height, sizeof(struct Edge));
   assert(edgeCache != NULL);

   houghCache = (struct Hough *)calloc(width * height, sizeof(struct Hough));
   assert(houghCache != NULL);

   SDL_LockSurface(picture);
   PopulateFlowCache(vFlowCache, hFlowCache, dec->image, width, height);
   SDL_UnlockSurface(picture);

   PopulateEdgeCache(edgeCache, vFlowCache, hFlowCache, width, height);
   ConnectEdges(edgeCache, width, height);

   WriteFlowCacheImage(vFlowCache, width, height, "vFlowCache.pnm");
   WriteFlowCacheImage(hFlowCache, width, height, "hFlowCache.pnm");
   WriteEdgeCacheImage(edgeCache, width, height, "edgeCache.pnm");
   WriteHoughCacheImage(houghCache, width, height, "houghCache.pnm");

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

         /* Trace the path a few steps downstream */
         SDL_LockSurface(screen);
         for(;;) {
            edge = edgeCache[scanY * width + scanX];

            if(!(edge.dir & ArriveConnected))
               break;

            PlotPixel(screen, state.imageLocX + scanX, state.imageLocY + scanY);

            scanX += xPattern[(edge.dir & ArriveDirection)>>4];
            scanY += yPatternFlip[(edge.dir & ArriveDirection)>>4];
         }
         SDL_UnlockSurface(screen);
      }

      SDL_Flip(screen);
   }

   free(houghCache);
   free(edgeCache);
   free(hFlowCache);
   free(vFlowCache);

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
PopulateFlowCache(struct Flow *vFlowCache, struct Flow  *hFlowCache,
      DmtxImage *img, int width, int height)
{
   int bytesPerPixel, rowSizeBytes, colorPlane;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
   int vMag, hMag; /*, compassMax;*/
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

         /* Calculate vertical edge flow */
         vMag  =  colorLoRt;
         vMag += (colorMdRt << 1);
         vMag +=  colorHiRt;
         vMag -=  colorLoLf;
         vMag -= (colorMdLf << 1);
         vMag -=  colorHiLf;

         /* Calculate horizontal edge flow */
         hMag  =  colorHiLf;
         hMag += (colorHiMd << 1);
         hMag +=  colorHiRt;
         hMag -=  colorLoLf;
         hMag -= (colorLoMd << 1);
         hMag -=  colorLoRt;

         /**
          *   vertical pos = up =  left side dark
          *   vertical neg = dn = right side dark
          * horizontal pos = lf =   low side dark
          * horizontal neg = rt =  high side dark
          */

         /* XXX consider adding DmtxDirNone, or possible just handle with sign? */
         vFlowCache[idx].dir = (vMag > 0) ? DmtxDirUp : DmtxDirDown;
         vMag = abs(vMag);
         vFlowCache[idx].mag = (vMag > 255) ? 255 : vMag;

         hFlowCache[idx].dir = (hMag > 0) ? DmtxDirLeft : DmtxDirRight;
         hMag = abs(hMag);
         hFlowCache[idx].mag = (hMag > 255) ? 255 : hMag;

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
static void
PopulateEdgeCache(struct Edge *edgeCache, struct Flow *vFlowCache,
      struct Flow *hFlowCache, int width, int height)
{
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
   int offset, offsets[8];
   int shiftFwLf, shiftFwMd, shiftFwRt;
   int offsetFwLf, offsetFwRt;
   int strongDir, strongMag;
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

         /* XXX later play with option of offset++ as part of for loop */
         offset = y * width + x;

         /* Does edge meet min strength threshold */
         if(vFlowCache[offset].mag != 0) {
            switch(vFlowCache[offset].dir) {
               case DmtxDirUp:
                  shiftFwMd = 5;
                  break;
               case DmtxDirDown:
                  shiftFwMd = 1;
                  break;
               default:
                  exit(10);
            }
            shiftFwLf = (shiftFwMd + 1) & 0x07;
            shiftFwRt = (shiftFwMd + 7) & 0x07;

            /* Find forward diagonal neighbors */
            offsetFwLf = offset + offsets[shiftFwLf];
            offsetFwRt = offset + offsets[shiftFwRt];

            if(vFlowCache[offset].dir == DmtxDirUp) {
               if(vFlowCache[offsetFwLf].mag > vFlowCache[offsetFwRt].mag) {
                  /* up left */
                  edgeCache[offset-1].count++;
                  edgeCache[offset-1].up += vFlowCache[offsetFwLf].mag;
                  edgeCache[offset-1].left += vFlowCache[offsetFwLf].mag;
               }
               else {
                  /* up right */
                  edgeCache[offset].count++;
                  edgeCache[offset].up += vFlowCache[offsetFwRt].mag;
                  edgeCache[offset].right += vFlowCache[offsetFwRt].mag;
               }
            }
            else if(vFlowCache[offset].dir == DmtxDirDown) {
               if(vFlowCache[offsetFwLf].mag > vFlowCache[offsetFwRt].mag) {
                  /* down right */
                  edgeCache[offset-width].count++;
                  edgeCache[offset-width].down += vFlowCache[offsetFwLf].mag;
                  edgeCache[offset-width].right += vFlowCache[offsetFwLf].mag;
               }
               else {
                  /* down left */
                  edgeCache[offset-width-1].count++;
                  edgeCache[offset-width-1].down += vFlowCache[offsetFwRt].mag;
                  edgeCache[offset-width-1].left += vFlowCache[offsetFwRt].mag;
               }
            }
            else {
               exit(11);
            }
         }

         if(hFlowCache[offset].mag != 0) {
            switch(hFlowCache[offset].dir) {
               case DmtxDirLeft:
                  shiftFwMd = 7;
                  break;
               case DmtxDirRight:
                  shiftFwMd = 3;
                  break;
               default:
                  exit(10);
            }
            shiftFwLf = (shiftFwMd + 1) & 0x07;
            shiftFwRt = (shiftFwMd + 7) & 0x07;

            /* Find forward diagonal neighbors */
            offsetFwLf = offset + offsets[shiftFwLf];
            offsetFwRt = offset + offsets[shiftFwRt];

            if(hFlowCache[offset].dir == DmtxDirLeft) {
               if(hFlowCache[offsetFwLf].mag > hFlowCache[offsetFwRt].mag) {
                  /* down left */
                  edgeCache[offset-width-1].count++;
                  edgeCache[offset-width-1].down += hFlowCache[offsetFwLf].mag;
                  edgeCache[offset-width-1].left += hFlowCache[offsetFwLf].mag;
               }
               else {
                  /* up left */
                  edgeCache[offset-1].count++;
                  edgeCache[offset-1].up += hFlowCache[offsetFwRt].mag;
                  edgeCache[offset-1].left += hFlowCache[offsetFwRt].mag;
               }
            }
            else if(hFlowCache[offset].dir == DmtxDirRight) {
               if(hFlowCache[offsetFwLf].mag > hFlowCache[offsetFwRt].mag) {
                  /* up right */
                  edgeCache[offset].count++;
                  edgeCache[offset].up += hFlowCache[offsetFwLf].mag;
                  edgeCache[offset].right += hFlowCache[offsetFwLf].mag;
               }
               else {
                  /* down right */
                  edgeCache[offset-width].count++;
                  edgeCache[offset-width].down += hFlowCache[offsetFwRt].mag;
                  edgeCache[offset-width].right += hFlowCache[offsetFwRt].mag;
               }
            }
            else {
               exit(11);
            }
         }
      }
   }

   for(y = yBeg; y <= yEnd; y++) {
      for(x = xBeg; x <= xEnd; x++) {

         /* XXX later play with option of offset++ as part of for loop */
         offset = y * width + x;

         strongDir = 7; /* left */
         strongMag = edgeCache[offset].left;
         if(edgeCache[offset].up > strongMag) {
            strongDir = 5; /* up */
            strongMag = edgeCache[offset].up;
         }
         if(edgeCache[offset].right > strongMag) {
            strongDir = 3; /* right */
            strongMag = edgeCache[offset].right;
         }
         if(edgeCache[offset].down > strongMag) {
            strongDir = 1; /* down */
            strongMag = edgeCache[offset].down;
         }

         edgeCache[offset].dir = strongDir;
         edgeCache[offset].mag = strongMag;
      }
   }

   tb = dmtxTimeNow();
   fprintf(stdout, "PopulateEdgeCache time: %ldms\n", (1000000 *
         (tb.sec - ta.sec) + (tb.usec - ta.usec))/1000);
}

/**
 * AaaaDddd
 * --------
 *   A = arrive connected (0x80)
 * aaa = arrive direction (0x70)
 *   D = depart connected (0x08)
 * ddd = depart direction (0x07)
 *
 * Since it is not possible for a flow to both arrive from and
 * depart to the same neighbor, locations on a valid edge will
 * never hold a cache value that satisfies the condition:
 *
 *   ((N & ArriveDirection) >> 4) == (N & DepartDirection)
 *
 * Therefore, the resulting list of impossible values can be
 * assigned predefined meanings for use as restricted constants:
 *
 * Directional values    Unconnected values    Connected values
 * ------------------    ------------------    ------------------
 *                       0x00  Unvisited       0x88  [unassigned]
 *                       0x11  [unassigned]    0x99  [unassigned]
 *                       0x22  [unassigned]    0xaa  [unassigned]
 *                       0x33  [unassigned]    0xbb  [unassigned]
 *                       0x44  [unassigned]    0xcc  [unassigned]
 *                       0x55  [unassigned]    0xdd  [unassigned]
 *                       0x66  [unassigned]    0xee  [unassigned]
 *                       0x77  [unassigned]    0xff  [unassinged]
 *
 */
static void
ConnectEdges(struct Edge *edgeCache, int width, int height)
{
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
   int i, maxIdx;
   int dir, badDir1, badDir2, badDir3;
   int offset, offsets[8];
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

         /* XXX later play with option of offset++ as part of for loop */
         offset = y * width + x;

         dir = (edgeCache[offset].dir & DepartDirection);
         badDir1 = (dir + 3)%8;
         badDir2 = (dir + 4)%8;
         badDir3 = (dir + 5)%8;

         /* Test all non-backwards locations for strongest neighbor */
         maxIdx = DmtxUndefined;
         for(i = 0; i < 8; i++) {
            if(i == badDir1 || i == badDir2 || i == badDir3)
               continue;
            else if(maxIdx == DmtxUndefined ||
                  edgeCache[offset + offsets[i]].mag > edgeCache[offset + offsets[maxIdx]].mag)
               maxIdx = i;
         }
         assert((edgeCache[offset].dir & ArriveDirection) == 0x00);
         edgeCache[offset].dir |= (ArriveConnected | (maxIdx << 4));
      }
   }

   tb = dmtxTimeNow();
   fprintf(stdout, "ConnectEdges time: %ldms\n", (1000000 *
         (tb.sec - ta.sec) + (tb.usec - ta.usec))/1000);
}


/**
 *
 *
 */
static void
PopulateHoughCache(struct Hough *houghCache, struct Flow *flowCache, int width, int height)
{
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
   DmtxTime ta, tb;

   xBeg = 2;
   xEnd = width - 3;
   yBeg = 2;
   yEnd = height - 3;

   ta = dmtxTimeNow();

   for(y = yBeg; y <= yEnd; y++) {
      for(x = xBeg; x <= xEnd; x++) {
         ;
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
WriteFlowCacheImage(struct Flow *flowCache, int width, int height, char *imagePath)
{
   int row, col;
   int rgb[3];
   struct Flow flow;
   FILE *fp;

   fp = fopen(imagePath, "wb");
   if(fp == NULL)
      exit(1);

   fprintf(fp, "P6\n%d %d\n255\n", width, height);

   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {
         flow = flowCache[row * width + col];
         switch(flow.dir) {
            case DmtxDirUp:
               rgb[0] = flow.mag;
               rgb[1] = 0;
               rgb[2] = 0;
               break;
            case DmtxDirDown:
               rgb[0] = 0;
               rgb[1] = flow.mag;
               rgb[2] = 0;
               break;
            case DmtxDirLeft:
               rgb[0] = 0;
               rgb[1] = 0;
               rgb[2] = flow.mag;
               break;
            case DmtxDirRight:
               rgb[0] = flow.mag;
               rgb[1] = flow.mag;
               rgb[2] = 0;
               break;
            default:
               rgb[0] = 0;
               rgb[1] = 0;
               rgb[2] = 0;
               break;
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
   int rgb[3];
   struct Edge edge;
   FILE *fp;

   fp = fopen(imagePath, "wb");
   if(fp == NULL)
      exit(1);

   fprintf(fp, "P6\n%d %d\n255\n", width, height);

   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {
         edge = edgeCache[row * width + col];

         rgb[0] = rgb[1] = rgb[2] = 0;
         if(edge.dir & ArriveConnected) {
            switch((edge.dir & ArriveDirection) >> 4) {
               case 0:
               case 1:
                  rgb[0] = 1;
                  break;
               case 2:
               case 3:
                  rgb[1] = 1;
                  break;
               case 4:
               case 5:
                  rgb[2] = 1;
                  break;
               case 6:
               case 7:
                  rgb[0] = 1;
                  rgb[1] = 1;
                  break;
            }
         }

         rgb[0] *= (edge.mag/2);
         rgb[1] *= (edge.mag/2);
         rgb[2] *= (edge.mag/2);

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
   int rgb[3];
   unsigned int cache;
   FILE *fp;

   fp = fopen(imagePath, "wb");
   if(fp == NULL)
      exit(1);

   fprintf(fp, "P6\n%d %d\n255\n", width, height);

   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {
         cache = houghCache[row * width + col].mag;
         switch(cache) {
            case DmtxDirVertical:
               rgb[0] = 255;
               rgb[1] = 0;
               rgb[2] = 0;
               break;
            case DmtxDirHorizontal:
               rgb[0] = 0;
               rgb[1] = 255;
               rgb[2] = 0;
               break;
            default:
               rgb[0] = 0;
               rgb[1] = 0;
               rgb[2] = 0;
               break;
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
PlotPixel(SDL_Surface *surface, int x, int y)
{
   char *ptr;
   Uint32 col;

   ptr = (char *)surface->pixels;
   col = SDL_MapRGB(surface->format, 255, 0, 0);

   memcpy(ptr + surface->pitch * y + surface->format->BytesPerPixel * x,
         &col, surface->format->BytesPerPixel);
}
