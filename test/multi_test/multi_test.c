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

static int xPattern[] = { -1,  0,  1,  1,  1,  0, -1, -1 };
static int yPattern[] = { -1, -1, -1,  0,  1,  1,  1,  0 };

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
static void PopulateFlowCache(struct Flow *flowCache, DmtxImage *img, int width, int height);
static void PopulateEdgeCache(struct Edge *edgeCache, struct Flow *flowCache, int width, int height);
static DmtxPassFail PoisonFlowUpstream(struct Edge *edgeCache, int width, int x, int y);
static void PopulateHoughCache(struct Hough *houghCache, struct Flow *flowCache, int width, int height);
static void WriteFlowCacheImage(struct Flow *flow, int width, int height, char *imagePath);
static void WriteEdgeCacheImage(struct Edge *edge, int width, int height, char *imagePath);
static void WriteHoughCacheImage(struct Hough *hough, int width, int height, char *imagePath);

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
/* DmtxRegion        *reg; */
/* DmtxMessage       *msg; */
   struct Flow       *flowCache;
   struct Edge       *edgeCache;
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

   flowCache = (struct Flow *)calloc(width * height, sizeof(struct Flow));
   assert(flowCache != NULL);
   edgeCache = (struct Edge *)calloc(width * height, sizeof(struct Edge));
   assert(edgeCache != NULL);
   houghCache = (struct Hough *)calloc(width * height, sizeof(struct Hough));
   assert(houghCache != NULL);

   SDL_LockSurface(picture);
   PopulateFlowCache(flowCache, dec->image, width, height);
   SDL_UnlockSurface(picture);

   PopulateEdgeCache(edgeCache, flowCache, width, height);
   PopulateHoughCache(houghCache, flowCache, width, height);

   WriteFlowCacheImage(flowCache, width, height, "flowCache.pnm");
   WriteEdgeCacheImage(edgeCache, width, height, "edgeCache.pnm");
   WriteHoughCacheImage(houghCache, width, height, "houghCache.pnm");

exit(1);
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
         scanY = picture->h - (state.pointerY - state.imageLocY) - 1;
/*
         SDL_LockSurface(picture);
         reg = dmtxRegionScanPixel(dec, scanX, scanY);
         SDL_UnlockSurface(picture);

         if(reg != NULL) {
            WriteDiagnosticImage(dec, "debug.pnm");

            msg = dmtxDecodeMatrixRegion(dec, reg, DmtxUndefined);
            if(msg != NULL) {
               fwrite(msg->output, sizeof(char), msg->outputIdx, stdout);
               fputc('\n', stdout);
               dmtxMessageDestroy(&msg);
            }

            dmtxRegionDestroy(&reg);
         }
*/
      }

      SDL_Flip(screen);
   }

   free(houghCache);
   free(edgeCache);
   free(flowCache);

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
PopulateFlowCache(struct Flow *flowCache, DmtxImage *img, int width, int height)
{
   int bytesPerPixel, rowSizeBytes, colorPlane;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
   int vMag, hMag, compassMax;
   int colorLoLf, colorLoMd, colorLoRt;
   int colorMdRt, colorHiRt, colorHiMd;
   int colorHiLf, colorMdLf, colorMdMd;
   int offset, offsetLo, offsetMd, offsetHi;
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

      offsetMd = (y * rowSizeBytes) + bytesPerPixel + colorPlane;
      offsetHi = offsetMd + rowSizeBytes;
      offsetLo = offsetMd - rowSizeBytes;

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

         /* Identify strongest compass flow */
         if(abs(vMag) > abs(hMag)) {
            flowCache[y * width + x].dir = (vMag > 0) ? DmtxDirUp : DmtxDirDown;
            compassMax = abs(vMag);
         }
         else {
            flowCache[y * width + x].dir = (hMag > 0) ? DmtxDirRight : DmtxDirLeft;
            compassMax = abs(hMag);
         }

         if(compassMax < 20) {
            flowCache[y * width + x].dir = DmtxDirNone;
            flowCache[y * width + x].mag = 0;
         }
         else {
            flowCache[y * width + x].mag = (compassMax > 255) ? 255 : compassMax;
         }

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

#define ArriveMask      0xf0
#define ArriveConnected 0x80
#define ArriveDirection 0x70
#define DepartMask      0x0f
#define DepartConnected 0x08
#define DepartDirection 0x07
#define Unvisited       0x00
#define Poisoned        0x11

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
 *   Unconnected values    Connected values
 *   ------------------    ------------------
 *   0x00  Unvisited       0x88  [unassigned]
 *   0x11  Poisoned        0x99  [unassigned]
 *   0x22  [unassigned]    0xaa  [unassigned]
 *   0x33  [unassigned]    0xbb  [unassigned]
 *   0x44  [unassigned]    0xcc  [unassigned]
 *   0x55  [unassigned]    0xdd  [unassigned]
 *   0x66  [unassigned]    0xee  [unassigned]
 *   0x77  [unassigned]    0xff  [unassinged]
 *
 */
static void
PopulateEdgeCache(struct Edge *edgeCache, struct Flow *flowCache, int width, int height)
{
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
   int offset, offsets[8];
   int shiftAB, shiftBC, shiftNeighbor, compare;
   int offsetFwLf, offsetFwMd, offsetFwRt, neighborOffset;
   int shiftFwLf, shiftFwMd, shiftFwRt;
   DmtxTime ta, tb;

   ta = dmtxTimeNow();

   offsets[0] = width - 1;
   offsets[1] = width;
   offsets[2] = width + 1;
   offsets[3] = 1;
   offsets[4] = -width + 1;
   offsets[5] = -width;
   offsets[6] = -width - 1;
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
         if(flowCache[offset].dir == DmtxDirNone)
            continue;

         /* Is departure connection already set? */
         assert(!(edgeCache[offset].dir & DepartConnected));

         /* Start with basic pointFlow direction */
         switch(flowCache[offset].dir) {
            case DmtxDirUp:
               shiftFwMd = 5;
               break;
            case DmtxDirLeft:
               shiftFwMd = 7;
               break;
            case DmtxDirDown:
               shiftFwMd = 1;
               break;
            case DmtxDirRight:
               shiftFwMd = 3;
               break;
            default:
               exit(10);
         }
         shiftFwLf = (shiftFwMd + 1) & 0x07;
         shiftFwRt = (shiftFwMd + 7) & 0x07;

         /* Capture edge strength of 3 forward neighbors */
         offsetFwLf = offset + offsets[shiftFwLf];
         offsetFwMd = offset + offsets[shiftFwMd];
         offsetFwRt = offset + offsets[shiftFwRt];

         shiftAB = (flowCache[offsetFwLf].mag >= flowCache[offsetFwMd].mag) ? shiftFwLf : shiftFwMd;
         shiftBC = (flowCache[offsetFwRt].mag >= flowCache[offsetFwMd].mag) ? shiftFwRt : shiftFwMd;

         /* Best candidate is strongest of 3 forward */
         if(flowCache[offset + offsets[shiftAB]].mag > flowCache[offset + offsets[shiftBC]].mag)
            shiftNeighbor = shiftAB;
         else
            shiftNeighbor = shiftBC;

         neighborOffset = offset + offsets[shiftNeighbor];

         if(flowCache[neighborOffset].mag < 20)
            continue;

         /* This flow has been intentially poisoned */
         if(edgeCache[neighborOffset].dir == Poisoned) {
            edgeCache[offset].dir &= ArriveMask;
            edgeCache[offset].dir |= (DepartConnected | shiftNeighbor);
            continue;
         }

         /* If someone already flows into that neighbor */
         if(edgeCache[neighborOffset].dir & ArriveConnected) {
            compare = ((edgeCache[neighborOffset].dir & ArriveDirection) >> 4);
            /* verify that neighbor + offsets[compare] is inbounds */
            if(flowCache[offset].mag > flowCache[neighborOffset + offsets[compare]].mag) {
               /* Trail is dead: Mark for later */
               edgeCache[neighborOffset + offsets[compare]].dir = Poisoned;
            }
            else {
               edgeCache[offset].dir = Poisoned;
               continue;
            }
         }

         /* Mark departure for this location */
         assert((edgeCache[offset].dir & 0x0f) == 0x00);
         edgeCache[offset].dir |= (DepartConnected | shiftNeighbor);
         edgeCache[neighborOffset].dir &= DepartMask;
         edgeCache[neighborOffset].dir |= (ArriveConnected | (((shiftNeighbor + 4) % 8) << 4));
      }
   }

   for(y = yBeg; y <= yEnd; y++) {
      for(x = xBeg; x <= xEnd; x++) {
         offset = y * width + x;
         if(edgeCache[offset] == Poisoned)
            PoisonFlowUpstream(edgeCache, width, x, y);
      }
   }

   tb = dmtxTimeNow();
   fprintf(stdout, "PopulateEdgeCache time: %ldms\n", (1000000 *
         (tb.sec - ta.sec) + (tb.usec - ta.usec))/1000);
}

/**
 *
 *
 */
static DmtxPassFail
PoisonFlowUpstream(struct Edge *edgeCache, int width, int x, int y)
{
   int offset, offsets[8], upstream;

   offsets[0] = width - 1;
   offsets[1] = width;
   offsets[2] = width + 1;
   offsets[3] = 1;
   offsets[4] = -width + 1;
   offsets[5] = -width;
   offsets[6] = -width - 1;
   offsets[7] = -1;

   offset = y * width + x;

   for(i = 0; i < 8; i++) {

      neighbor = edgeCache[offset + offsets[i]].dir;

      /* If neighbor doesn't flow downstream then nevermind */
      if(!(neighbor.dir & DepartConnected))
         continue;

      /* If neighbor doesn't point to me then nevermind */
      if((neighbor.dir & DepartDirection) != (i + 4) % 8)
         continue;

      /* Otherwise poison him and everyone that came before him */
XXX left off here
      upstream = i;
      x += xPattern[upstream];
      y += yPattern[upstream];
      do {
         x += xPattern[upstream];
         y += yPattern[upstream];
         offset = y * width + x;

         upstream = (edgeCache[offset].dir & ArriveDirection) >> 4;
         edgeCache[offset].dir = Poisoned;
      } while(edgeCache[offset].dir & ArriveConnected) {
   }

   return DmtxPass;
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

   for(row = 0; row < height; row++) {
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
   unsigned char cache;
   FILE *fp;

   fp = fopen(imagePath, "wb");
   if(fp == NULL)
      exit(1);

   fprintf(fp, "P6\n%d %d\n255\n", width, height);

   for(row = 0; row < height; row++) {
      for(col = 0; col < width; col++) {
         cache = edgeCache[row * width + col].dir;
         rgb[0] = rgb[1] = rgb[2] = 0;
         if(cache == Poisoned) {
            rgb[2] = 255;
         }
         else if((cache & ArriveConnected) && (cache & DepartConnected)) {
            rgb[0] = 255;
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
