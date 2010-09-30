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
 * x Move main() scanning logic into "dmtxScanImage()" function and use callbacks
 * o Clean up gAppState usage in main()
 * o Add "Dmtx" prefix to remaining core types (and review names)
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "../../dmtx.h"
#include "multi_test.h"

AppState gAppState;

int
main(int argc, char *argv[])
{
   UserOptions        opt;
   SDL_Event          event;
   SDL_Rect           imageLoc;
   Uint32             bgColorB;
   DmtxDecode        *dec;
   SDL_Rect           clipRect;
   DmtxCallbacks      callbacks;

   opt = GetDefaultOptions();

   if(HandleArgs(&opt, &argc, &argv) == DmtxFail) {
      exit(1);
/*    ShowUsage(1); */
   }

   gAppState = InitAppState();

   /* Load image */
   gAppState.picture = IMG_Load(opt.imagePath);
   if(gAppState.picture == NULL) {
      fprintf(stderr, "Unable to load image \"%s\": %s\n", opt.imagePath, SDL_GetError());
      exit(1);
   }
   gAppState.imageWidth = gAppState.picture->w;
   gAppState.imageHeight = gAppState.picture->h;

   atexit(SDL_Quit);

   /* Initialize SDL library */
   if(SDL_Init(SDL_INIT_VIDEO) < 0) {
      fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
      exit(1);
   }

   gAppState.screen = SetWindowSize(gAppState.windowWidth, gAppState.windowHeight);
   NudgeImage(CTRL_COL1_X, gAppState.picture->w, &(gAppState.imageLocX));
   NudgeImage(gAppState.windowHeight, gAppState.picture->h, &(gAppState.imageLocY));

   bgColorB = SDL_MapRGBA(gAppState.screen->format, 100, 100, 100, 255);

   /* Create surface to hold image pixels to be scanned */
   gAppState.local = SDL_CreateRGBSurface(SDL_SWSURFACE, LOCAL_SIZE, LOCAL_SIZE, 32, 0, 0, 0, 0);
   gAppState.imgActive = dmtxImageCreate(gAppState.local->pixels, gAppState.local->w, gAppState.local->h, DmtxPack32bppXRGB);
   assert(gAppState.imgActive != NULL);

   /* Create another surface for scaling purposes */
   gAppState.localTmp = SDL_CreateRGBSurface(SDL_SWSURFACE, LOCAL_SIZE, LOCAL_SIZE, 32, 0, 0, 0, 0);

   switch(gAppState.picture->format->BytesPerPixel) {
      case 1:
         gAppState.imgFull = dmtxImageCreate(gAppState.picture->pixels, gAppState.picture->w, gAppState.picture->h, DmtxPack8bppK);
         break;
      case 3:
         gAppState.imgFull = dmtxImageCreate(gAppState.picture->pixels, gAppState.picture->w, gAppState.picture->h, DmtxPack24bppRGB);
         dmtxImageSetProp(gAppState.imgFull, DmtxPropRowPadBytes,
               gAppState.picture->pitch - (gAppState.picture->w * gAppState.picture->format->BytesPerPixel));
         break;
      case 4:
         gAppState.imgFull = dmtxImageCreate(gAppState.picture->pixels, gAppState.picture->w, gAppState.picture->h, DmtxPack32bppXRGB);
         dmtxImageSetProp(gAppState.imgFull, DmtxPropRowPadBytes,
               gAppState.picture->pitch - (gAppState.picture->w * gAppState.picture->format->BytesPerPixel));
         break;
      default:
         exit(1);
   }
   assert(gAppState.imgFull != NULL);

   dec = dmtxDecodeCreate(gAppState.imgFull, 1);
   assert(dec != NULL);

   for(;;) {
      SDL_Delay(10);

      while(SDL_PollEvent(&event))
         HandleEvent(&event, &gAppState, gAppState.picture, &gAppState.screen);

      if(gAppState.quit == DmtxTrue)
         break;

      imageLoc.x = gAppState.imageLocX;
      imageLoc.y = gAppState.imageLocY;

      captureLocalPortion(gAppState.local, gAppState.localTmp, gAppState.picture, gAppState.screen, &gAppState, imageLoc);

      /* Start with blank canvas */
      SDL_FillRect(gAppState.screen, NULL, bgColorB);

      /* Draw image to main canvas area */
      clipRect.w = CTRL_COL1_X - 1;
      clipRect.h = gAppState.windowHeight;
      clipRect.x = 0;
      clipRect.y = 0;
      SDL_SetClipRect(gAppState.screen, &clipRect);
      SDL_BlitSurface(gAppState.picture, NULL, gAppState.screen, &imageLoc);
      SDL_SetClipRect(gAppState.screen, NULL);

      DrawActiveBorder(gAppState.screen, gAppState.activeExtent);

      callbacks.edgeCacheCallback = EdgeCacheCallback;
      callbacks.houghCacheCallback = HoughCacheCallback;
      callbacks.vanishPointCallback = VanishPointCallback;
      callbacks.timingCallback = TimingCallback;
      callbacks.gridCallback = GridCallback;
      callbacks.perimeterCallback = PerimeterCallback;

      ShowActiveRegion(gAppState.screen, gAppState.local);

      SDL_LockSurface(gAppState.local);
      dmtxScanImage(dec, gAppState.imgActive, &callbacks);
      SDL_UnlockSurface(gAppState.local);

      /* Dump FFT results */
      if(gAppState.printValues == DmtxTrue)
         gAppState.printValues = DmtxFalse;

      SDL_Flip(gAppState.screen);
   }

   SDL_FreeSurface(gAppState.localTmp);
   SDL_FreeSurface(gAppState.local);

   dmtxDecodeDestroy(&dec);
   dmtxImageDestroy(&gAppState.imgFull);
   dmtxImageDestroy(&gAppState.imgActive);

   exit(0);
}

void AddFullTransforms(AlignmentGrid *grid)
{
   DmtxMatrix3 mTranslate, mScale, mTmp;

   if(gAppState.activeExtent == 64) {
      dmtxMatrix3Translate(mTranslate, gAppState.imageLocX - 288,
            518 - (227 + gAppState.imageLocY + gAppState.imageHeight));
      dmtxMatrix3Multiply(grid->raw2fitFull, mTranslate, grid->raw2fitActive); /* not tested */
   }
   else {
      dmtxMatrix3Scale(mScale, 2.0, 2.0);
      dmtxMatrix3Multiply(mTmp, mScale, grid->raw2fitActive);
      dmtxMatrix3Copy(grid->raw2fitActive, mTmp);

      dmtxMatrix3Translate(mTranslate, gAppState.imageLocX - 304,
            518 - (243 + gAppState.imageLocY + gAppState.imageHeight));
      dmtxMatrix3Multiply(grid->raw2fitFull, mTranslate, grid->raw2fitActive); /* not tested */
   }

   if(gAppState.activeExtent == 64) {
      dmtxMatrix3Translate(mTranslate, 288 - gAppState.imageLocX,
            227 + gAppState.imageLocY + gAppState.imageHeight - 518);
   }
   else {
      dmtxMatrix3Scale(mScale, 0.5, 0.5);
      dmtxMatrix3Multiply(mTmp, grid->fit2rawActive, mScale);
      dmtxMatrix3Copy(grid->fit2rawActive, mTmp);

      dmtxMatrix3Translate(mTranslate, 304 - gAppState.imageLocX,
            243 + gAppState.imageLocY + gAppState.imageHeight - 518);
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
      clipRect.x = (screen->w - state->activeExtent)/2 - imageLoc.x;
      clipRect.y = (screen->h - state->activeExtent)/2 - imageLoc.y;
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
UserOptions
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
DmtxPassFail
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
AppState
InitAppState(void)
{
   AppState state;

   state.adjust = DmtxTrue;
   state.windowWidth = 640;
   state.windowHeight = 518;
   state.imageWidth = 0;
   state.imageHeight = 0;
   state.activeExtent = 64;
   state.imgActive = NULL;
   state.imgFull = NULL;
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
   state.picture = NULL;
   state.screen = NULL;
   state.local = NULL;
   state.localTmp = NULL;

   return state;
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

/**
 *
 *
 */
/*
void
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
