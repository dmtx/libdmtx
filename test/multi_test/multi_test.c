/*
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright (c) 2009 Mike Laughton
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
#include "dmtx.h"

struct UserOptions {
   const char *imagePath;
};

struct AppState {
   int windowWidth;
   int windowHeight;
   int imageLocX;
   int imageLocY;
   int leftButton;
   int rightButton;
   int pointerX;
   int pointerY;
};

static struct UserOptions GetDefaultOptions(void);
static DmtxPassFail HandleArgs(struct UserOptions *opt, int *argcp, char **argvp[]);
static struct AppState InitAppState(void);
static SDL_Surface *SetWindowSize(int windowWidth, int windowHeight);
static int HandleEvent(SDL_Event *event, struct AppState *state, SDL_Surface **screen);
static int NudgeImage(int windowExtent, int pictureExtent, int imageLoc);
static void WriteDiagnosticImage(DmtxDecode *dec, DmtxRegion *reg, char *imagePath);

int main(int argc, char *argv[])
{
   int done;
   struct UserOptions opt;
   struct AppState state;
   SDL_Surface *screen;
   SDL_Surface *picture;
   SDL_Event event;
   SDL_Rect imageLoc;
   DmtxImage *img;
   DmtxDecode *dec;
   DmtxRegion reg;
/* DmtxMessage *msg; */

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

   /* fprintf(stdout, "pitch:%d\n", picture->pitch); assert? */
   img = dmtxImageCreate(picture->pixels, picture->w, picture->h, 32,
         DmtxPackRGB, DmtxFlipY);
   if(img == NULL) {
      fprintf(stderr, "Unable to create image\n");
      exit(1);
   }

   atexit(SDL_Quit);

   /* Initialize SDL library */
   if(SDL_Init(SDL_INIT_VIDEO) < 0) {
      fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
      exit(1);
   }

   screen = SetWindowSize(state.windowWidth, state.windowHeight);

   for(done = 0; !done;) {
      SDL_Delay(10);

      while(SDL_PollEvent(&event))
         done = HandleEvent(&event, &state, &screen);

      state.imageLocX = NudgeImage(state.windowWidth, picture->w, state.imageLocX);
      state.imageLocY = NudgeImage(state.windowHeight, picture->h, state.imageLocY);

      imageLoc.x = state.imageLocX;
      imageLoc.y = state.imageLocY;

      SDL_FillRect(screen, NULL, 0xff000050);
      SDL_BlitSurface(picture, NULL, screen, &imageLoc);

      dec = dmtxDecodeStructCreate(img);

      if(state.rightButton == SDL_PRESSED) {

         SDL_LockSurface(picture);
         reg = dmtxRegionScanPixel(dec, state.pointerX, state.pointerY);
         SDL_UnlockSurface(picture);

         WriteDiagnosticImage(dec, &reg, "debug.pnm");
/*
         if(reg.found != DMTX_REGION_FOUND)
            break;

         msg = dmtxDecodeMatrixRegion(img, &reg, -1);
         if(msg == NULL)
            continue;

         fwrite(msg->output, sizeof(char), msg->outputIdx, stdout);
         fputc('\n', stdout);

         dmtxMessageDestroy(&msg);
*/
      }

      dmtxDecodeStructDestroy(&dec);

      SDL_Flip(screen);
   }

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
static int
HandleEvent(SDL_Event *event, struct AppState *state, SDL_Surface **screen)
{
   switch(event->type) {
      case SDL_KEYDOWN:
         switch(event->key.keysym.sym) {
            case SDLK_ESCAPE:
               return 1;
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
         }
         break;

      case SDL_VIDEORESIZE:
         state->windowWidth = event->resize.w;
         state->windowHeight = event->resize.h;
         *screen = SetWindowSize(state->windowWidth, state->windowHeight);
         break;

      default:
         break;
   }

   return 0;
}

/**
 *   +   +
 * ---> --->
 */
static int
NudgeImage(int windowExtent, int pictureExtent, int imageLoc)
{
   int marginA, marginB;

   marginA = imageLoc;
   marginB = imageLoc + pictureExtent - windowExtent;

   /* One edge inside and one edge outside window */
   if(marginA * marginB > 0) {
      if((pictureExtent - windowExtent) * marginA < 0)
         return (imageLoc - marginB);
      else
         return (imageLoc - marginA);
   }
   /* Image falls within window */
   else if(marginA >= 0 && marginB <= 0) {
      return (marginA - marginB)/2;
   }

   return imageLoc;
}

/**
 *
 *
 **/
static void
WriteDiagnosticImage(DmtxDecode *dec, DmtxRegion *reg, char *imagePath)
{
   int row, col;
   int width, height;
   int offset;
   int rgb[3];
   double shade;
   FILE *fp;

   fp = fopen(imagePath, "wb");
   if(fp == NULL) {
      exit(1);
   }

   width = dmtxImageGetProp(dec->image, DmtxPropScaledWidth);
   height = dmtxImageGetProp(dec->image, DmtxPropScaledHeight);

   /* Test each pixel of input image to see if it lies in region */
   fprintf(fp, "P6\n%d %d\n255\n", width, height);
   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {

         offset = dmtxImageGetPixelOffset(dec->image, col, row);
         if(offset == DMTX_BAD_OFFSET) {
            rgb[0] = 0;
            rgb[1] = 0;
            rgb[2] = 128;
         }
         else {
            dmtxImageGetPixelValue(dec->image, col, row, 0, &rgb[0]);
            dmtxImageGetPixelValue(dec->image, col, row, 1, &rgb[1]);
            dmtxImageGetPixelValue(dec->image, col, row, 2, &rgb[2]);

            if(dec->image->cache[offset] & 0x40) {
               rgb[0] = 255;
               rgb[1] = 0;
               rgb[2] = 0;
            }
            else {
               shade = (dec->image->cache[offset] & 0x80) ? 0.0 : 0.7;
               rgb[0] += (shade * (255 - rgb[0]));
               rgb[1] += (shade * (255 - rgb[1]));
               rgb[2] += (shade * (255 - rgb[2]));
            }
         }
         fwrite(rgb, sizeof(char), 3, fp);
      }
   }

   fclose(fp);
}
