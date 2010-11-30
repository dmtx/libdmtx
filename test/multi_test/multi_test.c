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

/* $Id$ */

/**
 * This "multi_test" program is for experimental algorithms. Please
 * consider this code to be untested, unoptimized, and even unstable.
 * If something works here it will make its way into libdmtx "proper"
 * only after being properly written, tuned, and tested.
 */

/**
 * TODO
 * o Should we use a setter for dmtx callback functions?
 * o Rename new code to use iRow, iCol, sRow, sCol, aRow, aCol, zRow, and zCol
 *   for index like values and x,y for coordinates (and iIdx, aIdx, etc...)
 * o Create functions to convert between index schemes (accel -> sobel, etc...)
 * o Add dynamic compensation adjustment for 45 deg Sobels based on sum of
 *   buckets in leading and trailing phi, compared to next orthagonal ones
 * o Add sqrt(2.0) to zero crossing calculation
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "../../dmtx.h"
#include "multi_test.h"

AppState gState;

int
main(int argc, char *argv[])
{
   UserOptions        opt;
   SDL_Event          event;
   SDL_Rect           imageLoc;
   Uint32             bgColorB;
   DmtxDecode        *dec;
   DmtxDecode2       *dec2;
   SDL_Rect           clipRect;

   if(HandleArgs(&opt, &argc, &argv) == DmtxFail) {
      exit(1);
   }

   gState = InitAppState();

   /* Load image */
   gState.picture = IMG_Load(opt.imagePath);
   if(gState.picture == NULL) {
      fprintf(stderr, "Unable to load image \"%s\": %s\n", opt.imagePath, SDL_GetError());
      exit(1);
   }

   atexit(SDL_Quit);

   /* Initialize SDL library */
   if(SDL_Init(SDL_INIT_VIDEO) < 0) {
      fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
      exit(2);
   }

   gState.screen = SetWindowSize(gState.windowWidth, gState.windowHeight);

   NudgeImage(CTRL_COL1_X, gState.picture->w, &(gState.imageLocX));
   NudgeImage(gState.windowHeight, gState.picture->h, &(gState.imageLocY));

   bgColorB = SDL_MapRGBA(gState.screen->format, 100, 100, 100, 255);

   /* Create surface to hold image pixels to be scanned */
   gState.local = SDL_CreateRGBSurface(SDL_SWSURFACE, LOCAL_SIZE, LOCAL_SIZE, 32, 0, 0, 0, 0);
   gState.imgActive = dmtxImageCreate(gState.local->pixels, gState.local->w, gState.local->h, DmtxPack32bppXRGB);
   assert(gState.imgActive != NULL);

   /* Create another surface for scaling purposes */
   gState.localTmp = SDL_CreateRGBSurface(SDL_SWSURFACE, LOCAL_SIZE, LOCAL_SIZE, 32, 0, 0, 0, 0);

   gState.imgFull = CreateDmtxImage(gState.picture);
   assert(gState.imgFull != NULL);

   gState.dmtxImage = CreateDmtxImage(gState.picture);
   assert(gState.dmtxImage != NULL);

   dec = dmtxDecodeCreate(gState.imgFull, 1);
   assert(dec != NULL);

   dec2 = dmtxDecode2Create();
   assert(dec2 != NULL);

/* dmtxDecode2SetScale(dec2); */

   /* Set up callback functions */
   dec2->fn.dmtxValueGridCallback = ValueGridCallback;
   dec2->fn.zeroCrossingCallback = ZeroCrossingCallback;
   dec2->fn.dmtxHoughLocalCallback = HoughLocalCallback;
   dec2->fn.vanishPointCallback = VanishPointCallback;
   dec2->fn.timingCallback = TimingCallback;
   dec2->fn.gridCallback = GridCallback;
   dec2->fn.perimeterCallback = PerimeterCallback;

   for(;;) {
      SDL_Delay(10);

      while(SDL_PollEvent(&event))
         HandleEvent(&event, &gState, gState.picture, &gState.screen);

      if(gState.quit == DmtxTrue)
         break;

      imageLoc.x = gState.imageLocX;
      imageLoc.y = gState.imageLocY;

      captureLocalPortion(gState.local, gState.localTmp, gState.picture, gState.screen, &gState, imageLoc);

      /* Start with blank canvas */
      SDL_FillRect(gState.screen, NULL, bgColorB);

      /* Draw image to main canvas area */
      clipRect.w = CTRL_COL1_X - 1;
      clipRect.h = gState.windowHeight;
      clipRect.x = 0;
      clipRect.y = 0;
      SDL_SetClipRect(gState.screen, &clipRect);
      SDL_BlitSurface(gState.picture, NULL, gState.screen, &imageLoc);
      SDL_SetClipRect(gState.screen, NULL);

      DrawActiveBorder(gState.screen, gState.activeExtent);

      ShowActiveRegion(gState.screen, gState.local);

      SDL_LockSurface(gState.local);
      dmtxDecode2SetImage(dec2, gState.dmtxImage);
      dmtxRegion2FindNext(dec2);
      SDL_UnlockSurface(gState.local);

      /* Dump FFT results */
      if(gState.printValues == DmtxTrue)
         gState.printValues = DmtxFalse;

      SDL_Flip(gState.screen);
   }

   SDL_FreeSurface(gState.localTmp);
   SDL_FreeSurface(gState.local);

   dmtxDecode2Destroy(&dec2);
   dmtxDecodeDestroy(&dec);
   dmtxImageDestroy(&gState.dmtxImage);
   dmtxImageDestroy(&gState.imgFull);
   dmtxImageDestroy(&gState.imgActive);

   exit(0);
}

/**
 *
 *
 */
DmtxPassFail
HandleArgs(UserOptions *opt, int *argcp, char **argvp[])
{
   memset(opt, 0x00, sizeof(UserOptions));

   if(*argcp < 2) {
      fprintf(stdout, "Argument required\n");
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
AppState
InitAppState(void)
{
   AppState state;

   memset(&state, 0x00, sizeof(AppState));

   state.picture = NULL;
   state.windowWidth = 640;
   state.windowHeight = 518;
   state.activeExtent = 64;
   state.imgActive = NULL;
   state.imgFull = NULL;
   state.autoNudge = DmtxFalse;
   state.displayEdge = 0;
   state.displayVanish = DmtxFalse;
   state.displayTiming = DmtxTrue;
   state.printValues = DmtxFalse;
   state.imageLocX = 0;
   state.imageLocY = 0;
   state.imageOffsetX = 0;
   state.imageOffsetY = 0;
   state.localOffsetX = 0;
   state.localOffsetY = 0;
   state.leftButton = SDL_RELEASED;
   state.rightButton = SDL_RELEASED;
   state.pointerX = 0;
   state.pointerY = 0;
   state.quit = DmtxFalse;
   state.screen = NULL;
   state.local = NULL;
   state.localTmp = NULL;

   return state;
}

/**
 *
 *
 */
DmtxImage *
CreateDmtxImage(SDL_Surface *sdlImage)
{
   DmtxPackOrder packOrder;
   DmtxImage *dmtxImage = NULL;

   switch(sdlImage->format->BytesPerPixel) {
      case 1:
         packOrder = DmtxPack8bppK;
         break;
      case 3:
         packOrder = DmtxPack24bppRGB;
         break;
      case 4:
         packOrder = DmtxPack32bppXRGB;
         break;
      default:
         return NULL;
   }

   dmtxImage = dmtxImageCreate(sdlImage->pixels, sdlImage->w, sdlImage->h, packOrder);
   if(dmtxImage == NULL)
      return NULL;

   if(sdlImage->format->BytesPerPixel > 1) {
      dmtxImageSetProp(dmtxImage, DmtxPropRowPadBytes, sdlImage->pitch -
            (sdlImage->w * sdlImage->format->BytesPerPixel));
   }

   return dmtxImage;
}

/**
 *
 *
 */
void AddFullTransforms(AlignmentGrid *grid)
{
   DmtxMatrix3 mTranslate, mScale, mTmp;

   if(gState.activeExtent == 64) {
      dmtxMatrix3Translate(mTranslate, gState.imageLocX - 288, 518 - (227 +
            gState.imageLocY + gState.picture->h));
      dmtxMatrix3Multiply(grid->raw2fitFull, mTranslate, grid->raw2fitActive); /* not tested */
   }
   else {
      dmtxMatrix3Scale(mScale, 2.0, 2.0);
      dmtxMatrix3Multiply(mTmp, mScale, grid->raw2fitActive);
      dmtxMatrix3Copy(grid->raw2fitActive, mTmp);

      dmtxMatrix3Translate(mTranslate, gState.imageLocX - 304, 518 - (243 +
            gState.imageLocY + gState.picture->h));
      dmtxMatrix3Multiply(grid->raw2fitFull, mTranslate, grid->raw2fitActive); /* not tested */
   }

   if(gState.activeExtent == 64) {
      dmtxMatrix3Translate(mTranslate, 288 - gState.imageLocX, 227 +
            gState.imageLocY + gState.picture->h - 518);
   }
   else {
      dmtxMatrix3Scale(mScale, 0.5, 0.5);
      dmtxMatrix3Multiply(mTmp, grid->fit2rawActive, mScale);
      dmtxMatrix3Copy(grid->fit2rawActive, mTmp);

      dmtxMatrix3Translate(mTranslate, 304 - gState.imageLocX, 243 +
            gState.imageLocY + gState.picture->h - 518);
   }
   dmtxMatrix3Multiply(grid->fit2rawFull, grid->fit2rawActive, mTranslate);
}

void
captureLocalPortion(SDL_Surface *local, SDL_Surface *localTmp,
      SDL_Surface *picture, SDL_Surface *screen, AppState *state, SDL_Rect imageLoc)
{
   int      i, j;
   Uint32   bgColorK;
   SDL_Rect clipRect;

   bgColorK = SDL_MapRGBA(screen->format, 0, 0, 0, 255);

   /* Capture portion of image that falls within highlighted region */
   /* Use blitsurface if 1:1, otherwise scale */
   SDL_FillRect(local, NULL, bgColorK);
   if(state->activeExtent == 64) {
      clipRect.x = state->localOffsetX;
      clipRect.y = picture->h - (state->localOffsetY + 64) - 1;
      clipRect.w = LOCAL_SIZE;
      clipRect.h = LOCAL_SIZE;
      SDL_BlitSurface(picture, &clipRect, local, NULL);
   }
   else if(state->activeExtent == 32) {
      Uint8 localBpp;
      Uint8 *writePixel, *readTL, *readTR, *readBL, *readBR;

      /* first blit, then expand */
      clipRect.x = (screen->w - state->activeExtent)/2 - imageLoc.x;
      clipRect.y = (screen->h - state->activeExtent)/2 - imageLoc.y;
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
}

/**
 *
 *
 */
SDL_Surface *
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
DmtxPassFail
HandleEvent(SDL_Event *event, AppState *state, SDL_Surface *picture, SDL_Surface **screen)
{
   int nudgeRequired = DmtxFalse;

   switch(event->type) {
      case SDL_KEYDOWN:
         switch(event->key.keysym.sym) {
            case SDLK_ESCAPE:
               state->quit = DmtxTrue;
               break;
            case SDLK_0:
               state->displayEdge = 0;
               break;
            case SDLK_1:
               state->displayEdge = 1;
               break;
            case SDLK_2:
               state->displayEdge = 2;
               break;
            case SDLK_3:
               state->displayEdge = 3;
               break;
            case SDLK_4:
               state->displayEdge = 4;
               break;
            case SDLK_5:
               state->displayEdge = 5;
               break;
            case SDLK_6:
               state->displayEdge = 6;
               break;
            case SDLK_l:
               fprintf(stdout, "Image Location: (%d, %d)\n", state->imageLocX,  state->imageLocY);
               break;
            case SDLK_n:
               state->autoNudge = (state->autoNudge == DmtxTrue) ? DmtxFalse : DmtxTrue;
               fprintf(stdout, "autoNudge %s\n", (state->autoNudge == DmtxTrue) ? "ON" : "OFF");
               break;
            case SDLK_p:
               state->printValues = (state->printValues == DmtxTrue) ? DmtxFalse : DmtxTrue;
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

   /* Offset from bottom left corner of screen to bottom left corner of image */
   state->imageOffsetX = state->imageLocX;
   state->imageOffsetY = (state->screen->h - state->picture->h) - state->imageLocY;

   /* Location of active area relative to bottom left corner of picture */
   if(gState.activeExtent == 64)
   {
      state->localOffsetX = 158 - state->imageOffsetX;
      state->localOffsetY = 227 - state->imageOffsetY;
   }
   else
   {
      state->localOffsetX = 174 - state->imageOffsetX;
      state->localOffsetY = 243 - state->imageOffsetY;
   }

   return DmtxPass;
}

/**
 *
 *
 */
DmtxPassFail
NudgeImage(int windowExtent, int pictureExtent, Sint16 *imageLoc)
{
   int minReveal = 16;

   if(*imageLoc < minReveal - pictureExtent)
      *imageLoc = minReveal - pictureExtent;
   else if(*imageLoc > windowExtent - minReveal)
      *imageLoc = windowExtent - minReveal;

   return DmtxPass;
}
