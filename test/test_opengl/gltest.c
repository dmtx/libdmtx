/*
libdmtx - Data Matrix Encoding/Decoding Library
Copyright (C) 2007 Mike Laughton

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL.h>
#include "dmtx.h"
#include "image.h"
#include "display.h"
#include "callback.h"

#define MIN(x,y) ((x < y) ? x : y)
#define MAX(x,y) ((x > y) ? x : y)

GLfloat view_rotx = 0.0, view_roty = 0.0, view_rotz = 0.0;
GLfloat angle = 0.0;

GLuint barcodeTexture;
GLint barcodeList;

DmtxImage *textureImage = NULL;
DmtxImage *captured = NULL;
DmtxImage *passOneImage = NULL;
DmtxImage *passTwoImage = NULL;

char *gFilename[] = { "test_image18.png"
                    , "test_image16.png"
                    , "test_image17.png"
                    , "test_image01.png"
                    , "test_image05.png"
                    , "test_image06.png"
                    , "test_image07.png"
                    , "test_image12.png"
                    , "test_image13.png"
                    , "test_image08.png"
                    , "test_image09.png"
                    , "test_image10.png"
                    , "test_image04.png"
                    , "test_image11.png"
                    , "test_image02.png"
                    , "test_image03.png"
                    , "test_image14.png"
                    , "test_image15.png" };
int gFileIdx = 0;
int gFileCount = 18;

/**
 *
 *
 */
int main(int argc, char *argv[])
{
   int             i;
   int             count;
   int             done;
   SDL_Event       event;
   SDL_Surface     *screen;
   unsigned char   outputString[1024];
   DmtxPixelLoc    p0, p1;
   DmtxDecode      decode;
   DmtxRegion      region;
   DmtxMessage     *message;
   DmtxTime        timeout;

   // Initialize display window
   screen = initDisplay();

   // Load input image to DmtxImage
   loadTextureImage(&textureImage);

   captured = dmtxImageMalloc(320, 320);
   if(captured == NULL)
      exit(4);

   passOneImage = dmtxImageMalloc(320, 320);
   if(passOneImage == NULL)
      exit(5);

   passTwoImage = dmtxImageMalloc(320, 320);
   if(passTwoImage == NULL)
      exit(6);

   p0.X = p0.Y = 0;
   p1.X = captured->width - 1;
   p1.Y = captured->height - 1;

   done = 0;
   while(!done) {

      SDL_Delay(50);

      while(SDL_PollEvent(&event))
         done = HandleEvent(&event, screen);

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      DrawGeneratedImage(screen);

      /* Capture screenshot of generated image */
      captureImage(captured);

      memset(passOneImage->pxl, 0x00, passOneImage->width * passOneImage->height * sizeof(DmtxRgb));
      memset(passTwoImage->pxl, 0x00, passTwoImage->width * passTwoImage->height * sizeof(DmtxRgb));

      /* Start fresh scan */
      decode = dmtxDecodeStructInit(captured);

      for(;;) {

         timeout = dmtxTimeAdd(dmtxTimeNow(), 100);

         region = dmtxDecodeFindNextRegion(&decode, &timeout);
         if(region.found != DMTX_REGION_FOUND)
            break;

         message = dmtxDecodeMatrixRegion(&decode, &region, 1);
         if(message == NULL)
            continue;

         fwrite(message->output, sizeof(unsigned char), message->outputIdx, stdout);
         fputc('\n', stdout);

         dmtxMessageFree(&message);
         break;
      }

      DrawBorders(screen);
//    DrawPane2(screen, passOneImage);
//    DrawPane4(screen, passTwoImage);

      SDL_GL_SwapBuffers();
   }

   dmtxImageFree(&textureImage);
   dmtxDecodeStructDeInit(&decode);

   exit(0);
}
