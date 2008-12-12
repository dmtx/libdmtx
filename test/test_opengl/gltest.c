/*
libdmtx - Data Matrix Encoding/Decoding Library
Copyright (c) 2007 Mike Laughton

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
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <assert.h>
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

DmtxImage *gImage = NULL;
unsigned char *captured = NULL;
unsigned char *textureImage = NULL;
unsigned char *passOneImage = NULL;
unsigned char *passTwoImage = NULL;

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
   int             width, height;
   SDL_Event       event;
   SDL_Surface     *screen;
   unsigned char   outputString[1024];
   DmtxPixelLoc    p;
   DmtxDecode      *dec;
   DmtxRegion      reg;
   DmtxMessage     *msg;
   DmtxTime        timeout;

   /* Initialize display window */
   screen = initDisplay();

   /* Load input image to DmtxImage */
   textureImage = loadTextureImage(&width, &height);

/* captured = dmtxImageMalloc(320, 320); */
   captured = (unsigned char *)malloc(width * height * 3);
   assert(captured != NULL);

/* imgTmp = dmtxImageMalloc(320, 320);
   assert(imgTmp != NULL); */

/* passOneImage = dmtxImageMalloc(320, 320); */
   passOneImage = (unsigned char *)malloc(width * height * 3);
   assert(passOneImage != NULL);

/* passTwoImage = dmtxImageMalloc(320, 320); */
   passTwoImage = (unsigned char *)malloc(width * height * 3);
   assert(passTwoImage != NULL);

   done = 0;
   while(!done) {

      SDL_Delay(50);

      while(SDL_PollEvent(&event))
         done = HandleEvent(&event, screen);

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      DrawGeneratedImage(screen);

      memset(passOneImage, 0x00, width * height * 3);
      memset(passTwoImage, 0x00, width * height * 3);

      /* Capture screenshot of generated image */
      glReadPixels(2, 324, width, height, GL_RGB, GL_UNSIGNED_BYTE, captured);
      gImage = dmtxImageCreate(captured, width, height, 24, DmtxPackRGB, DmtxFlipNone);
      assert(gImage != NULL);

      /* Start fresh scan */
      dec = dmtxDecodeStructCreate(gImage);
      assert(dec != NULL);

      for(;;) {

         timeout = dmtxTimeAdd(dmtxTimeNow(), 500);
/*
         reg = dmtxDecodeFindNextRegion(dec, &timeout);
         if(reg.found != DMTX_REGION_FOUND)
            break;
*/
/*       p.X = 55; */
         p.X = 130;
         p.Y = 190;
         reg = dmtxRegionScanPixel(dec, p);
         if(reg.found != DMTX_REGION_FOUND)
            break;

         msg = dmtxDecodeMatrixRegion(gImage, &reg, 1);
         if(msg == NULL)
            break;
/*          continue; */

         fwrite(msg->output, sizeof(unsigned char), msg->outputIdx, stdout);
         fputc('\n', stdout);

         dmtxMessageDestroy(&msg);

         break;
      }

      dmtxDecodeStructDestroy(&dec);
      dmtxImageDestroy(&gImage);

      DrawBorders(screen);
//    DrawPane2(screen, passOneImage);
//    DrawPane4(screen, passTwoImage);

      SDL_GL_SwapBuffers();
   }

   free(passTwoImage);
   free(passOneImage);
   free(captured);
   free(textureImage);

   exit(0);
}
