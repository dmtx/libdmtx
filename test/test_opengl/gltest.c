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

GLfloat       view_rotx = 0.0, view_roty = 0.0, view_rotz = 0.0;
GLfloat       angle = 0.0;

GLuint        barcodeTexture;
GLint         barcodeList;

DmtxImage     *captured;
DmtxImage     textureImage;
DmtxImage     passOneImage;
DmtxImage     passTwoImage;

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
int main(int argc, char **argv)
{
   int             i;
   int             count;
   int             done;
   int             vScanGap, hScanGap;
   DmtxDecode      *decode;
   SDL_Event       event;
   SDL_Surface     *screen;
   unsigned char   outputString[1024];
   DmtxMatrixRegion *region;

   decode = dmtxDecodeStructCreate();

   decode->option |= DmtxSingleScanOnly;

   dmtxSetPlotPointCallback(decode, &PlotPointCallback);
   dmtxSetXfrmPlotPointCallback(decode, &XfrmPlotPointCallback);
   dmtxSetPlotModuleCallback(decode, &PlotModuleCallback);
/* dmtxSetCrossScanCallback(decode, &CrossScanCallback);
   dmtxSetFollowScanCallback(decode, &FollowScanCallback); */
   dmtxSetBuildMatrixCallback2(decode, &BuildMatrixCallback2);
   dmtxSetBuildMatrixCallback3(decode, &BuildMatrixCallback3);
   dmtxSetBuildMatrixCallback4(decode, &BuildMatrixCallback4);
   dmtxSetFinalCallback(decode, &FinalCallback);

   memset(&textureImage, 0x00, sizeof(DmtxImage));
   memset(&passOneImage, 0x00, sizeof(DmtxImage));
   memset(&passTwoImage, 0x00, sizeof(DmtxImage));

   // Initialize display window
   screen = initDisplay();

   // Load input image to DmtxImage
   loadTextureImage(&textureImage); // XXX check error condition

   // XXX More ugly temporary stuff
   passOneImage.width = passOneImage.height = 320;
   passOneImage.pxl = (DmtxPixel *)malloc(passOneImage.width * passOneImage.height * sizeof(DmtxPixel));
   if(passOneImage.pxl == NULL)
      exit(5); // XXX Yeah, I know

   // XXX More ugly temporary stuff
   passTwoImage.width = passTwoImage.height = 320;
   passTwoImage.pxl = (DmtxPixel *)malloc(passTwoImage.width * passTwoImage.height * sizeof(DmtxPixel));
   if(passTwoImage.pxl == NULL)
      exit(6); // XXX Yeah, I know

   done = 0;
   while(!done) {

      // Generate input image through input events and OpenGL routines
      SDL_Delay(50);

      while(SDL_PollEvent(&event))
         done = HandleEvent(&event, screen);

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      DrawGeneratedImage(screen);

      // Capture screenshot of generated image
      captureImage(decode);
      captured = &(decode->image); // XXX ugliest hack -- this stuff is all wrong

      memset(passOneImage.pxl, 0x00, passOneImage.width * passOneImage.height * sizeof(DmtxPixel));
      memset(passTwoImage.pxl, 0x00, passTwoImage.width * passTwoImage.height * sizeof(DmtxPixel));

      // Scan for data matrix step ranges within captured image
      vScanGap = decode->image.height/2.0 + 0.5;
//    vScanGap = 32;
      hScanGap = decode->image.width/2.0 + 0.5;
//    hScanGap = 32;

      // Erase Data Matrix list from previous iteration and start new
      dmtxScanStartNew(decode);

      for(i = vScanGap; i < decode->image.height; i += vScanGap) {
         if(decode->matrixCount > 0)
            break;
         dmtxScanLine(decode, DmtxDirRight, i);
      }

//    for(i = hScanGap; i < decode->image.width; i += hScanGap) {
//       if(decode->matrixCount > 0)
//          break;
//       dmtxScanLine(decode, DmtxDirUp, i);
//    }

      count = dmtxDecodeGetMatrixCount(decode);
      if(count > 0) {
         region = dmtxDecodeGetMatrix(decode, 0);
         memset(outputString, 0x00, 1024);
         strncpy((char *)outputString, (const char *)region->output, MIN(region->outputIdx, 1023));
         fprintf(stdout, "%s\n", outputString);
      }

      DrawBorders(screen);
      DrawPane2(screen, &passOneImage);
//    DrawPane4(screen, &passTwoImage);

      SDL_GL_SwapBuffers();
   }

   dmtxImageDeInit(&textureImage);

   dmtxDecodeStructDestroy(&decode);

   exit(0);
}
