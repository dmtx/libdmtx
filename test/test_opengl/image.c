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
#include <png.h>
#include "dmtx.h"
#include "gltest.h"
#include "display.h"
#include "image.h"

/**
 *
 *
 */
void captureImage(DmtxImage *captured)
{
   glReadPixels(2, 324, 320, 320, GL_RGB, GL_UNSIGNED_BYTE, captured->pxl);
}

/**
 *
 *
 */
int loadTextureImage(DmtxImage **image)
{
   int error;
   char filepath[128];

   strcpy(filepath, "../images_opengl/");
   strcat(filepath, gFilename[gFileIdx]);
   fprintf(stdout, "Opening %s\n", filepath);

   dmtxImageFree(image);

   *image = loadPng(filepath);
   if(*image == NULL)
      exit(1);

   gFileIdx++;
   if(gFileIdx == gFileCount)
      gFileIdx = 0;

   /* Set up texture */
   glGenTextures(1, &barcodeTexture);
   glBindTexture(GL_TEXTURE_2D, barcodeTexture);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   /* Read barcode image */
   gluBuild2DMipmaps(GL_TEXTURE_2D, 3, (*image)->width, (*image)->height, GL_RGB, GL_UNSIGNED_BYTE, (*image)->pxl);

   /* Create the barcode list */
   barcodeList = glGenLists(1);
   glNewList(barcodeList, GL_COMPILE);
   DrawBarCode();
   glEndList();

   return 0;
}

/**
 *
 *
 */
DmtxImage *loadPng(char *filename)
{
   png_byte        pngHeader[8];
   FILE            *fp;
   int             headerTestSize = sizeof(pngHeader);
   int             isPng;
   int             bitDepth, color_type, interlace_type, compression_type, filter_method;
   int             row;
   png_uint_32     width, height;
   png_structp     png_ptr;
   png_infop       info_ptr;
   png_infop       end_info;
   png_bytepp      row_pointers;
   DmtxImage       *image;

   fp = fopen(filename, "rb");
   if(!fp)
      return NULL;

   fread(pngHeader, 1, headerTestSize, fp);
   isPng = !png_sig_cmp(pngHeader, 0, headerTestSize);
   if(!isPng)
      return NULL;

   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

   if(!png_ptr)
      return NULL;

   info_ptr = png_create_info_struct(png_ptr);
   if(!info_ptr) {
      png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
      return NULL;
   }

   end_info = png_create_info_struct(png_ptr);
   if(!end_info) {
      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
      return NULL;
   }

   if(setjmp(png_jmpbuf(png_ptr))) {
      png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
      fclose(fp);
      return NULL;
   }

   png_init_io(png_ptr, fp);
   png_set_sig_bytes(png_ptr, headerTestSize);

   png_read_info(png_ptr, info_ptr);
   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bitDepth, &color_type,
         &interlace_type, &compression_type, &filter_method);

   png_set_strip_16(png_ptr);
   png_set_strip_alpha(png_ptr);
   png_set_packswap(png_ptr);

   if(color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(png_ptr);

   if (color_type == PNG_COLOR_TYPE_GRAY || PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(png_ptr);

   png_read_update_info(png_ptr, info_ptr);

   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bitDepth, &color_type,
         &interlace_type, &compression_type, &filter_method);

   row_pointers = (png_bytepp)png_malloc(png_ptr, sizeof(png_bytep) * height);
   if(row_pointers == NULL) {
      fprintf(stdout, "Fatal error!\n"); fflush(stdout); // XXX finish later
      ; // FatalError(1, "Error while during malloc for row_pointers");
   }

   for(row = 0; row < height; row++) {
      row_pointers[row] = (png_bytep)png_malloc(png_ptr,
            png_get_rowbytes(png_ptr, info_ptr));
   }

   png_read_image(png_ptr, row_pointers);
   png_read_end(png_ptr, info_ptr);

   png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

   // Use PNG information to populate DmtxImage information
   image = dmtxImageMalloc(width, height);
   if(image == NULL)
      return NULL;

   for(row = 0; row < image->height; row++) {
      memcpy(image->pxl + (row * image->width), row_pointers[image->height - row - 1],
            image->width * sizeof(DmtxPixel));
   }

   for(row = 0; row < height; row++) {
      png_free(png_ptr, row_pointers[row]);
   }
   png_free(png_ptr, row_pointers);

   fclose(fp);

   return image;
}

/**
 *
 *
 */
void plotPoint(DmtxImage *image, float rowFloat, float colFloat, int targetColor)
{
   int i, row, col;
   float xFloat, yFloat;
   int offset[4];
   int color[4];

   row = (int)rowFloat;
   col = (int)colFloat;

   xFloat = colFloat - col;
   yFloat = rowFloat - row;

   offset[0] = row * image->width + col;
   offset[1] = row * image->width + (col + 1);
   offset[2] = (row + 1) * image->width + col;
   offset[3] = (row + 1) * image->width + (col + 1);

   color[0] = clampRGB(255.0 * ((1.0 - xFloat) * (1.0 - yFloat)));
   color[1] = clampRGB(255.0 * (xFloat * (1.0 - yFloat)));
   color[2] = clampRGB(255.0 * ((1.0 - xFloat) * yFloat));
   color[3] = clampRGB(255.0 * (xFloat * yFloat));

   for(i = 0; i < 4; i++) {
      if((i == 1 || i== 3) && col + 1 > 319)
         continue;
      else if((i == 2 || i== 3) && row + 1 > 319)
         continue;

      if(targetColor & (ColorWhite | ColorRed | ColorYellow))
         image->pxl[offset[i]].R = max(image->pxl[offset[i]].R, color[i]);

      if(targetColor & (ColorWhite | ColorGreen | ColorYellow))
         image->pxl[offset[i]].G = max(image->pxl[offset[i]].G, color[i]);

      if(targetColor & (ColorWhite | ColorBlue))
         image->pxl[offset[i]].B = max(image->pxl[offset[i]].B, color[i]);
   }
}

/**
 *
 *
 */
int clampRGB(float color)
{
   if(color < 0.0)
      return 0;
   else if(color > 255.0)
      return 255;
   else
      return (int)(color + 0.5);
}
