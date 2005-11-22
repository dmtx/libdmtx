/*
libdmtx - Data Matrix Encoding/Decoding Library
Copyright (C) 2006 Mike Laughton

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

Contact: mike@dragonflylogic.com
*/

extern int
dmtxImageInit(DmtxImage *image)
{
   image->width = image->height = 0;
   image->pxl = NULL;

   return 0;
}

extern int
dmtxImageDeInit(DmtxImage *image)
{
   if(image == NULL)
      return 0; // Error

   if(image->pxl != NULL) {
      free(image->pxl);
      image->pxl = NULL;
   }

   return 0;
}

extern int
dmtxImageGetWidth(DmtxImage *image)
{
   if(image == NULL)
      ; // Error

   return image->width;
}

extern int
dmtxImageGetHeight(DmtxImage *image)
{
   if(image == NULL)
      ; // Error

   return image->height;
}

extern int
dmtxImageGetOffset(DmtxImage *image, DmtxDirection dir, int lineNbr, int offset)
{
   return (dir & DmtxDirHorizontal) ? image->width * lineNbr + offset : image->width * offset + lineNbr;
}

/**
 * Load image data from PNG file into DmtxImage format.
 *
 * @param image    pointer to DmtxImage structure to be populated
 * @param filename path/name of PNG image
 * @return         DMTX_SUCCESS | DMTX_FAILURE
 */
extern int
dmtxImageLoadPng(DmtxImage *image, char *filename)
{
   png_byte        pngHeader[8];
   FILE            *fp;
   int             isPng;
   int             bitDepth, colorType, interlaceType, compressionType, filterMethod;
   int             row;
   png_uint_32     width, height;
   png_structp     pngPtr;
   png_infop       infoPtr;
   png_infop       endInfo;
   png_bytepp      rowPointers;

   fp = fopen(filename, "rb");
   if(fp == NULL)
      return DMTX_FAILURE;

   fread(pngHeader, 1, sizeof(pngHeader), fp);
   isPng = !png_sig_cmp(pngHeader, 0, sizeof(pngHeader));
   if(!isPng)
      return DMTX_FAILURE;

   pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

   if(pngPtr == NULL)
      return DMTX_FAILURE;

   infoPtr = png_create_info_struct(pngPtr);
   if(infoPtr == NULL) {
      png_destroy_read_struct(&pngPtr, (png_infopp)NULL, (png_infopp)NULL);
      return DMTX_FAILURE;
   }

   endInfo = png_create_info_struct(pngPtr);
   if(endInfo == NULL) {
      png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)NULL);
      return DMTX_FAILURE;
   }

   if(setjmp(png_jmpbuf(pngPtr))) {
      png_destroy_read_struct(&pngPtr, &infoPtr, &endInfo);
      fclose(fp);
      return DMTX_FAILURE;
   }

   png_init_io(pngPtr, fp);
   png_set_sig_bytes(pngPtr, sizeof(pngHeader));

   png_read_info(pngPtr, infoPtr);
   png_get_IHDR(pngPtr, infoPtr, &width, &height, &bitDepth, &colorType,
         &interlaceType, &compressionType, &filterMethod);

   png_set_strip_16(pngPtr);
   png_set_strip_alpha(pngPtr);
   png_set_packswap(pngPtr);

   if(colorType == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(pngPtr);

   if (colorType == PNG_COLOR_TYPE_GRAY || PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(pngPtr);

   png_read_update_info(pngPtr, infoPtr);

   png_get_IHDR(pngPtr, infoPtr, &width, &height, &bitDepth, &colorType,
         &interlaceType, &compressionType, &filterMethod);

   rowPointers = (png_bytepp)png_malloc(pngPtr, sizeof(png_bytep) * height);
   if(rowPointers == NULL) {
      perror("Error while during malloc for rowPointers");
      exit(7);
   }

   for(row = 0; row < height; row++) {
      rowPointers[row] = (png_bytep)png_malloc(pngPtr,
            png_get_rowbytes(pngPtr, infoPtr));
   }

   png_read_image(pngPtr, rowPointers);
   png_read_end(pngPtr, infoPtr);

   png_destroy_read_struct(&pngPtr, &infoPtr, &endInfo);

   // Use PNG information to populate DmtxImage information
   image->width = width;
   image->height = height;

   image->pxl = (DmtxPixel *)malloc(image->width * image->height *
         sizeof(DmtxPixel));
   if(image->pxl == NULL)
      return DMTX_FAILURE;

   // This copy reverses row order top-to-bottom so image coordinate system
   // corresponds with normal "right-handed" 2D space
   for(row = 0; row < image->height; row++) {
      memcpy(image->pxl + (row * image->width), rowPointers[image->height - row - 1], image->width * sizeof(DmtxPixel));
   }

   for(row = 0; row < height; row++) {
      png_free(pngPtr, rowPointers[row]);
   }
   png_free(pngPtr, rowPointers);

   fclose(fp);

   return DMTX_SUCCESS;
}
