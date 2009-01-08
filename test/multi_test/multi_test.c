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
   int mouseButton;
   int pointerX;
   int pointerY;
};

static struct UserOptions GetDefaultOptions(void);
static DmtxPassFail HandleArgs(struct UserOptions *opt, int *argcp, char **argvp[]);
static struct AppState InitAppState(void);
static SDL_Surface *SetWindowSize(int windowWidth, int windowHeight);
static int HandleEvent(SDL_Event *event, struct AppState *appState, SDL_Surface **screen);
static int NudgeImage(int windowExtent, int pictureExtent, int imageLoc);

int main(int argc, char *argv[])
{
   int done;
   struct UserOptions opt;
   struct AppState appState;
   SDL_Surface *screen;
   SDL_Surface *picture;
   SDL_Event event;
   SDL_Rect imageLoc;
   DmtxImage *img;
   DmtxDecode *dec;
   DmtxRegion reg;
   DmtxMessage *msg;

   opt = GetDefaultOptions();

   if(HandleArgs(&opt, &argc, &argv) == DmtxFail) {
      exit(1);
/*    ShowUsage(1); */
   }

   appState = InitAppState();

   /* Load image */
   picture = IMG_Load(opt.imagePath);
   if(picture == NULL) {
      fprintf(stderr, "Couldn't load %s: %s\n", opt.imagePath, SDL_GetError());
      return 0;
   }

   img = dmtxImageCreate(picture->pixels, picture->w, picture->h, 24,
         DmtxPackRGB, DmtxFlipY);

   atexit(SDL_Quit);

   /* Initialize SDL library */
   if(SDL_Init(SDL_INIT_VIDEO) < 0) {
      fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
      exit(1);
   }

   screen = SetWindowSize(appState.windowWidth, appState.windowHeight);

   for(done = 0; !done;) {
      SDL_Delay(10);

      while(SDL_PollEvent(&event))
         done = HandleEvent(&event, &appState, &screen);

      appState.imageLocX = NudgeImage(appState.windowWidth, picture->w, appState.imageLocX);
      appState.imageLocY = NudgeImage(appState.windowHeight, picture->h, appState.imageLocY);

      imageLoc.x = appState.imageLocX;
      imageLoc.y = appState.imageLocY;

      SDL_FillRect(screen, NULL, 0xff000050);
      SDL_BlitSurface(picture, NULL, screen, &imageLoc);

      dec = dmtxDecodeStructCreate(img);
/*
      for(;;) {
         SDL_LockSurface(picture);
         reg = dmtxDecodeFindNextRegion(dec, NULL);
         SDL_UnlockSurface(picture);

         if(reg.found != DMTX_REGION_FOUND)
            break;

         msg = dmtxDecodeMatrixRegion(img, &reg, -1);
         if(msg == NULL)
            continue;

         fwrite(msg->output, sizeof(char), msg->outputIdx, stdout);
         fputc('\n', stdout);

         dmtxMessageDestroy(&msg);
      }
*/
      dmtxDecodeStructDestroy(&dec);

      SDL_Flip(screen);
   }

   dmtxImageDestroy(&img);

   return 0;
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
   struct AppState appState;

   appState.windowWidth = 640;
   appState.windowHeight = 480;
   appState.imageLocX = 0;
   appState.imageLocY = 0;
   appState.mouseButton = SDL_RELEASED;
   appState.pointerX = 0;
   appState.pointerY = 0;

   return appState;
}

/**
 *
 *
 */
static SDL_Surface *
SetWindowSize(int windowWidth, int windowHeight)
{
   SDL_Surface *screen;

   screen = SDL_SetVideoMode(windowWidth, windowHeight, 24,
         SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE);

   if(screen == NULL) {
      fprintf(stderr, "Couldn't set %dx%dx24 video mode: %s\n", windowWidth,
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
HandleEvent(SDL_Event *event, struct AppState *appState, SDL_Surface **screen)
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
            appState->mouseButton = event->button.state;
         break;

      case SDL_MOUSEMOTION:
         appState->pointerX = event->motion.x;
         appState->pointerY = event->motion.y;

         if(appState->mouseButton == SDL_PRESSED) {
            appState->imageLocX += event->motion.xrel;
            appState->imageLocY += event->motion.yrel;
         }
         break;

      case SDL_VIDEORESIZE:
         appState->windowWidth = event->resize.w;
         appState->windowHeight = event->resize.h;
         *screen = SetWindowSize(appState->windowWidth, appState->windowHeight);
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
