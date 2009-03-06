/*
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright (C) 2009 Mike Laughton
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact: mblaughton@users.sourceforge.net
 */

/* $Id: multi_test.c 561 2008-12-28 16:28:58Z mblaughton $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

static struct UserOptions GetDefaultOptions(void);
static DmtxPassFail HandleArgs(struct UserOptions *opt, int *argcp, char **argvp[]);
static struct AppState InitAppState(void);
static SDL_Surface *SetWindowSize(int windowWidth, int windowHeight);
static DmtxPassFail HandleEvent(SDL_Event *event, struct AppState *state,
      SDL_Surface *picture, SDL_Surface **screen);
static DmtxPassFail NudgeImage(int windowExtent, int pictureExtent, Sint16 *imageLoc);
static void WriteDiagnosticImage(DmtxDecode *dec, char *imagePath);

int main(int argc, char *argv[])
{
   struct UserOptions opt;
   struct AppState state;
   SDL_Surface *screen;
   SDL_Surface *picture;
   SDL_Event    event;
   SDL_Rect     imageLoc;
   DmtxImage   *img;
   DmtxDecode  *dec;
   DmtxRegion  *reg;
   DmtxMessage *msg;

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

   img = dmtxImageCreate(picture->pixels, picture->w, picture->h, DmtxPack32bppXRGB);
   assert(img != NULL);

   dec = dmtxDecodeCreate(img, 1);
   assert(dec != NULL);

/* here we can populate a fake cache to test the new approach */

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

         SDL_LockSurface(picture);
         reg = dmtxRegionScanPixel(dec, state.pointerX, state.pointerY);
         SDL_UnlockSurface(picture);

         if(reg != NULL) {
/*          WriteDiagnosticImage(dec, reg, "debug.pnm"); */

            msg = dmtxDecodeMatrixRegion(dec, reg, DmtxUndefined);
            if(msg != NULL) {
               fwrite(msg->output, sizeof(char), msg->outputIdx, stdout);
               fputc('\n', stdout);
               dmtxMessageDestroy(&msg);
            }

            dmtxRegionDestroy(&reg);
         }
      }

      SDL_Flip(screen);
   }

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
 *   +   +
 * ---> --->
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

/**
 *
 *
 */
/*
PopulateCache()
{
   DmtxTime ta, tb;
   int x, xBeg, xEnd;
   int y, yBeg, yEnd;
   static const int coefficient[] = {  0,  1,  2,  1,  0, -1, -2, -1 };
   static const int dmtxPatternX[] = { -1,  0,  1,  1,  1,  0, -1, -1 };
   static const int dmtxPatternY[] = { -1, -1, -1,  0,  1,  1,  1,  0 };
   int err;
   int bytesPerPixel, rowSizeBytes;
   int patternIdx, coefficientIdx;
   int compass, compassMax;
   int mag[4] = { 0 };
   int xAdjust, yAdjust;
   int color, colorPattern[8];
   DmtxPointFlow flow;
   int offset;

   xBeg = 1;
   xEnd = width - 2;
   yBeg = 1;
   rolEng = height - 2;

   rowSizeBytes = dmtxImageGetProp(img, DmtxPropRowSizeBytes);
   bytesPerPixel = dmtxImageGetProp(img, DmtxPropBytesPerPixel);

   ta = dmtxTimeNow();
   for(y = yBeg; y <= yMax; y++) {
      for(x = xBeg; x <= xEnd; x++) {

         offset = y * rowSizeBytes + x * bytesPerPixel + colorPlane;
         colorPattern[0] = img->pxl[(offset - rowSizeBytes - bytesPerPixel)];
         colorPattern[1] = img->pxl[(offset - rowSizeBytes                )];
         colorPattern[2] = img->pxl[(offset - rowSizeBytes + bytesPerPixel)];
         colorPattern[3] = img->pxl[(offset                + bytesPerPixel)];
         colorPattern[4] = img->pxl[(offset + rowSizeBytes + bytesPerPixel)];
         colorPattern[5] = img->pxl[(offset + rowSizeBytes                )];
         colorPattern[6] = img->pxl[(offset + rowSizeBytes - bytesPerPixel)];
         colorPattern[7] = img->pxl[(offset                - bytesPerPixel)];

         // Calculate this pixel's horizontal and vertical flow
         compassMax = 0;
         for(compass = 0; compass < 2; compass++) {

            // Add portion from each position in the convolution matrix pattern
            for(patternIdx = 0; patternIdx < 8; patternIdx++) {

               coefficientIdx = (patternIdx - compass + 8) % 8; <-- fix match for 2 directions
               if(coefficient[coefficientIdx] == 0)
                  continue;

               color = colorPattern[patternIdx];

               switch(coefficient[coefficientIdx]) {
                  case 2:
                     mag[compass] += (color << 1);
                     break;
                  case 1:
                     mag[compass] += color;
                     break;
                  case -2:
                     mag[compass] -= (color << 1);
                     break;
                  case -1:
                     mag[compass] -= color;
                     break;
               }
            }

            // Identify strongest compass flow
            if(compass != 0 && abs(mag[compass]) > abs(mag[compassMax]))
               compassMax = compass;
         }
      }
   }
   tb = dmtxTimeNow();
   fprintf(stdout, "before %ld.%06ld\n", ta.sec, ta.usec);
   fprintf(stdout, "after  %ld.%06ld\n", tb.sec, tb.usec);
   fprintf(stdout, "delta  %ld\n", (1000000 * (tb.sec - ta.sec) + (tb.usec - ta.usec))/10);
}
*/
