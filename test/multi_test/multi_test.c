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
 * o Move main() scanning logic into "dmtxScanImage()" function and use callbacks to display progress
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "../../dmtx.h"
#include "multi_test.h"

int
main(int argc, char *argv[])
{
   UserOptions        opt;
   AppState           state;
   SDL_Surface       *screen;
   SDL_Surface       *picture;
   SDL_Event          event;
   SDL_Rect           imageLoc;
   Uint32             bgColorB;
   int                i, j;
   int                maxIntensity;
   DmtxImage         *imgActive, *imgFull;
   DmtxDecode        *dec;
   DmtxEdgeCache      edgeCache;
   HoughCache         hough;
   VanishPointSort    vPoints;
   DmtxTimingSort     timings;
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

      captureLocalPortion(local, localTmp, picture, screen, &state, imageLoc);

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
      dmtxBuildEdgeCache(&edgeCache, imgActive);
      SDL_UnlockSurface(local);

      BlitActiveRegion(screen, local, 1, CTRL_ROW1_Y, CTRL_COL1_X);
      BlitActiveRegion(screen, local, 1, CTRL_ROW1_Y, CTRL_COL2_X);
      BlitActiveRegion(screen, local, 1, CTRL_ROW1_Y, CTRL_COL3_X);
      BlitActiveRegion(screen, local, 1, CTRL_ROW1_Y, CTRL_COL4_X);

      /* Write flow cache images to feedback panes */
      maxIntensity = FindMaxEdgeIntensity(&edgeCache);
      BlitFlowCache(screen, edgeCache.vDir, maxIntensity, CTRL_ROW2_Y, CTRL_COL1_X);
      BlitFlowCache(screen, edgeCache.bDir, maxIntensity, CTRL_ROW2_Y, CTRL_COL2_X);
      BlitFlowCache(screen, edgeCache.hDir, maxIntensity, CTRL_ROW2_Y, CTRL_COL3_X);
      BlitFlowCache(screen, edgeCache.sDir, maxIntensity, CTRL_ROW2_Y, CTRL_COL4_X);

      /* Find relative size of hough quadrants */
      dmtxPopulateHoughCache(&hough, &edgeCache);
      dmtxNormalizeHoughCache(&hough, &edgeCache);
      BlitHoughCache(screen, &hough, CTRL_ROW3_Y, CTRL_COL1_X + 1);

      dmtxMarkHoughMaxima(&hough);
      BlitHoughCache(screen, &hough, CTRL_ROW4_Y - 1, CTRL_COL1_X + 1);

      /* Find vanishing points */
      vPoints = FindVanishPoints(&hough);
      if(state.displayVanish == DmtxTrue)
         DrawVanishingPoints(screen, vPoints, CTRL_ROW3_Y, CTRL_COL1_X);

      /* Find and rank best timings for vanishing points */
      timings = dmtxFindGridTiming(&hough, &vPoints, &state);

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
            /* auto tweak will go here */
            err = FindRegionWithinGrid(&region, imgFull, &grid, dec, screen, &state);
            regionFound = (err == DmtxPass) ? DmtxTrue : DmtxFalse;

            if(regionFound == DmtxTrue) {
               region.sizeIdx = GetSizeIdx(region.width, region.height);
               /* XXX should we introduce dmtxSymbolSizeValid() library function? */
               if(region.sizeIdx >= DmtxSymbol10x10 && region.sizeIdx <= DmtxSymbol16x48)
                  DecodeSymbol(&region, dec);
            }

            SDL_UnlockSurface(picture);

            regionFound = DmtxTrue; /* break out of outer loop */
            break; /* break out of inner loop */
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
