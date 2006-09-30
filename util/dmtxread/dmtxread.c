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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>
#include <png.h>
#include <tiffio.h>
#include <dmtx.h>
#include "dmtxread.h"

#define DMTXREAD_SUCCESS     0
#define DMTXREAD_ERROR       1

#define MIN(x,y) ((x < y) ? x : y)

static int HandleArgs(ScanOptions *options, int *argcp, char **argvp[], int *fileIndex);
static long StringToLong(char *numberString);
static void ShowUsage(int status);
static void FatalError(int errorCode, char *fmt, ...);
static ImageFormat GetImageFormat(char *imagePath);
static int LoadImage(DmtxImage *image, char *imagePath, int imageIndex);
static int LoadPngImage(DmtxImage *image, char *imagePath);
static int LoadTiffImage(DmtxImage *image, char *imagePath, int imageIndex);
static int ScanImage(ScanOptions *options, DmtxDecode *decode, char *prefix);

char *programName;

/**
 * Main function for the dmtxread Data Matrix scanning utility.
 *
 * @param  argc count of arguments passed from command line
 * @param  argv list of argument passed strings from command line
 * @return      numeric success / failure exit code
 */
int
main(int argc, char *argv[])
{
   int err;
   int fileIndex;
   DmtxDecode *decode;
   ScanOptions options;
   char *imagePath;
   char prefix[16];
   int imageCount;
   int imageIndex;

   err = HandleArgs(&options, &argc, &argv, &fileIndex);
   if(err)
      ShowUsage(err);

   decode = dmtxDecodeStructCreate();
   decode->option = DmtxSingleScanOnly;

   while(fileIndex < argc) {

      imagePath = argv[fileIndex];

      imageIndex = 0;
      do {
         dmtxImageInit(&(decode->image));
         imageCount = LoadImage(&(decode->image), imagePath, imageIndex++);

         if(imageCount == 0)
            break;

         memset(prefix, 0x00, 16);
         if(options.pageNumber)
            snprintf(prefix, 15, "%d:", imageIndex);
         else
            *prefix = '\0';

         ScanImage(&options, decode, prefix);

         dmtxImageDeInit(&(decode->image));
      } while(imageIndex < imageCount);

      fileIndex++;
   }

   dmtxDecodeStructDestroy(&decode);

   exit(0);
}

/**
 * Sets and validates user-requested options from command line arguments.
 *
 * @param options    runtime options from defaults or command line
 * @param  argcp     pointer to argument count
 * @param  argvp     pointer to argument list
 * @param  fileIndex pointer to index of first non-option arg (if successful)
 * @return           DMTXREAD_SUCCESS | DMTXREAD_ERROR
 */
static int
HandleArgs(ScanOptions *options, int *argcp, char **argvp[], int *fileIndex)
{
   int opt;
   int longIndex;

   struct option longOptions[] = {
         {"count",       required_argument, NULL, 'c'},
         {"vertical",    required_argument, NULL, 'v'},
         {"horizontal",  required_argument, NULL, 'h'},
         {"page-number", no_argument,       NULL, 'n'},
         {"help",        no_argument,       NULL,  0 },
         {0, 0, 0, 0}
   };

   programName = (*argvp)[0];
   if(programName && strrchr(programName, '/'))
      programName = strrchr(programName, '/') + 1;

   memset(options, 0x00, sizeof(ScanOptions));

   options->maxCount = 0; // Unlimited
   options->hScanCount = 5;
   options->vScanCount = 5;
   options->pageNumber = 0;

   *fileIndex = 0;

   for(;;) {
      opt = getopt_long(*argcp, *argvp, "c:v:h:n", longOptions, &longIndex);
      if(opt == -1)
         break;

      switch(opt) {
         case 0: // --help
            ShowUsage(0);
            break;
         case 'c':
            options->maxCount = StringToLong(optarg);
            break;
         case 'v':
            options->vScanCount = StringToLong(optarg);
            break;
         case 'h':
            options->hScanCount = StringToLong(optarg);
            break;
         case 'n':
            options->pageNumber = 1;
            break;
         default:
            return DMTXREAD_ERROR;
            break;
      }
   }
   *fileIndex = optind;

   // File not specified
   if(*fileIndex == *argcp) {
      if(*argcp == 1)
         return DMTXREAD_ERROR;
      else
         FatalError(1, _("Must specify image file\n"));
   }

   return DMTXREAD_SUCCESS;
}

/**
 * Convert a string of characters to a long integer.  If string cannot be
 * converted then the function will abort the program.
 *
 * @param numberString pointer to string of numbers
 * @return             converted long
 */
static long
StringToLong(char *numberString)
{
   long numberLong;
   char *trailingChars;

   errno = 0;
   numberLong = strtol(numberString, &trailingChars, 10);

   while(isspace(*trailingChars))
      trailingChars++;

   if(errno != 0 || *trailingChars != '\0')
      FatalError(2, _("Invalid number \"%s\""), numberString);

   return numberLong;
}

/**
 * Display program usage and exit with received status.
 *
 * @param status error code returned to OS
 * @return       void
 */
static void
ShowUsage(int status)
{
   if(status != 0) {
      fprintf(stderr, _("Usage: %s [OPTION]... [FILE]...\n"), programName);
      fprintf(stderr, _("Try `%s --help' for more information.\n"), programName);
   }
   else {
      fprintf(stdout, _("Usage: %s [OPTION]... [FILE]...\n"), programName);
      fprintf(stdout, _("\
Scan FILE (a PNG image) for Data Matrix barcodes and print decoded results\n\
to STDOUT.  Note that %s may find multiple barcodes in one image.\n\
\n\
Example: %s -v 4 -h 4 IMAGE001.png IMAGE002.png\n\
\n\
OPTIONS:\n"), programName, programName);
      fprintf(stdout, _("\
  -c NUM, --count=NUM       stop after finding NUM barcodes\n\
  -v NUM, --vertical=NUM    use NUM vertical scan lines in scan pattern\n\
  -h NUM, --horizontal=NUM  use NUM horizontal scan lines in scan pattern\n\
  -n,     --page-number     prefix decoded messages with corresponding page number\n\
          --help            display this help and exit\n"));
      fprintf(stdout, _("\nReport bugs to <mike@dragonflylogic.com>.\n"));
   }

   exit(status);
}

/**
 * Display error message and exit with error status.
 *
 * @param errorCode error code returned to OS
 * @param fmt       error message format for printing
 * @return          void
 */
static void
FatalError(int errorCode, char *fmt, ...)
{
   va_list va;

   va_start(va, fmt);
   fprintf(stderr, "%s: ", programName);
   vfprintf(stderr, fmt, va);
   va_end(va);
   fprintf(stderr, "\n\n");
   fflush(stderr);

   exit(errorCode);
}

/**
 *
 */
static ImageFormat
GetImageFormat(char *imagePath)
{
   char *extension, *ptr;

   // XXX Right now this determines file type based on filename extension.
   // XXX Not ideal -- but only temporary.
   assert(imagePath != NULL);

   ptr = strrchr(imagePath, '/');
   extension = (ptr == NULL) ? imagePath : ptr + 1;

   ptr = strrchr(extension, '.');
   extension = (ptr == NULL) ? NULL : ptr + 1;

   if(extension == NULL)
      return ImageFormatUnknown;
   else if(strncmp(extension, "png", 3) == 0 || strncmp(extension, "PNG", 3) == 0)
      return ImageFormatPng;
   else if(strncmp(extension, "tif", 3) == 0 || strncmp(extension, "TIF", 3) == 0)
      return ImageFormatTiff;

   return ImageFormatUnknown;
}

/**
 *
 */
static int
LoadImage(DmtxImage *image, char *imagePath, int imageIndex)
{
   int imageCount;
   ImageFormat imageFormat;

   imageFormat = GetImageFormat(imagePath);

   switch(imageFormat) {
      case ImageFormatPng:
         imageCount = LoadPngImage(image, imagePath);
         break;
      case ImageFormatTiff:
         imageCount = LoadTiffImage(image, imagePath, imageIndex);
         break;
      default:
         imageCount = 0;
         break;
   }

   return imageCount;
}

/**
 * Load data from PNG file into DmtxImage format.
 *
 * @param image     pointer to DmtxImage structure to be populated
 * @param imagePath path/name of PNG image
 * @return          DMTX_SUCCESS | DMTX_FAILURE
 */
static int
LoadPngImage(DmtxImage *image, char *imagePath)
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

   fp = fopen(imagePath, "rb");
   if(fp == NULL) {
      perror(programName);
      return DMTX_FAILURE;
   }

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
//    perror("Error while during malloc for rowPointers"); // XXX shouldn't programName be passed instead?
      perror(programName);
      return DMTX_FAILURE;
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
   if(image->pxl == NULL) {
      perror(programName);
      return DMTX_FAILURE;
   }

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

/**
 * Load data from TIFF file into DmtxImage format.
 *
 * @param image    pointer to DmtxImage structure to be populated
 * @param filename path/name of PNG image
 * @return         DMTX_SUCCESS | DMTX_FAILURE
 */
static int
LoadTiffImage(DmtxImage *image, char *imagePath, int imageIndex)
{
   int dirIndex = 0;
   TIFF* tif;
   int row, col, offset;
   uint32 w, h;
   uint32* raster;
   size_t npixels;

   tif = TIFFOpen(imagePath, "r");
   if(tif == NULL) {
      perror(programName);
      return DMTX_FAILURE;
   }

   do {
      if(dirIndex == imageIndex) {
         TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
         TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
         npixels = w * h;

         raster = (uint32*) _TIFFmalloc(npixels * sizeof (uint32));
         if(raster == NULL) {
            perror(programName);
            exit(2);
         }

         if(TIFFReadRGBAImage(tif, w, h, raster, 0)) {
            // Use TIFF information to populate DmtxImage information
            image->width = w;
            image->height = h;

            image->pxl = (DmtxPixel *)malloc(image->width * image->height * sizeof(DmtxPixel));
            if(image->pxl == NULL) {
               perror(programName);
               return DMTX_FAILURE;
            }

            // This copy reverses row order top-to-bottom so image coordinate system
            // corresponds with normal "right-handed" 2D space
            for(row = 0; row < image->height; row++) {
               for(col = 0; col < image->width; col++) {
                  // XXX TIFF uses ABGR packed
                  offset = row * image->width + col;
                  image->pxl[offset].R = raster[offset] & 0x000000ff;
                  image->pxl[offset].G = (raster[offset] & 0x0000ff00) >> 8;
                  image->pxl[offset].B = (raster[offset] & 0x00ff0000) >> 16;
               }
            }
         }

         _TIFFfree(raster);
      }

      dirIndex++;
   } while(TIFFReadDirectory(tif));

   TIFFClose(tif);

   return dirIndex;
}

/**
 * Scan image for Data Matrix barcodes and print decoded data to STDOUT.
 *
 * @param options   runtime options from defaults or command line
 * @param decode    pointer to DmtxDecode struct
 * @return          DMTXREAD_SUCCESS | DMTXREAD_ERROR
 */
static int
ScanImage(ScanOptions *options, DmtxDecode *decode, char *prefix)
{
   int row, col;
   int hScanGap, vScanGap;
   unsigned char outputString[1024];
   DmtxMatrixRegion *matrixRegion;

   dmtxScanStartNew(decode);

   hScanGap = (int)((double)decode->image.width/options->hScanCount + 0.5);
   vScanGap = (int)((double)decode->image.height/options->vScanCount + 0.5);

   for(col = hScanGap; col < decode->image.width; col += hScanGap)
      dmtxScanLine(decode, DmtxDirUp, col);

   for(row = vScanGap; row < decode->image.height; row += vScanGap)
      dmtxScanLine(decode, DmtxDirRight, row);

   if(dmtxDecodeGetMatrixCount(decode) > 0) {
      matrixRegion = dmtxDecodeGetMatrix(decode, 0);
      memset(outputString, 0x00, 1024);
      strncpy((char *)outputString, (const char *)matrixRegion->output, MIN(matrixRegion->outputIdx, 1023));
      fprintf(stdout, "%s%s\n", prefix, outputString);
   }

   return DMTXREAD_SUCCESS;
}
