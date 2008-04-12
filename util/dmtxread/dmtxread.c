/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (c) 2008 Mike Laughton

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

/* $Id$ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <assert.h>
#include <png.h>
#include <tiffio.h>
#include <dmtx.h>
#include "dmtxread.h"

#define DMTXREAD_SUCCESS     0
#define DMTXREAD_ERROR       1

static void SetOptionDefaults(UserOptions *options);
static int HandleArgs(UserOptions *options, int *fileIndex, int *argcp, char **argvp[]);
static int SetRangeLimit(int *target, char *optionString, int minMax, int limit);
static int SetScanRegion(DmtxPixelLoc *p0, DmtxPixelLoc *p1, UserOptions *options, DmtxImage *image);
static int StringToInt(int *numberInt, char *numberString, char **terminate);
static void ShowUsage(int status);
static void FatalError(int errorCode, char *fmt, ...);
static ImageFormat GetImageFormat(char *imagePath);
static DmtxImage *LoadImage(char *imagePath, int pageIndex);
static DmtxImage *LoadImagePng(char *imagePath);
static DmtxImage *LoadImageTiff(char *imagePath, int pageIndex);
static int PrintDecodedOutput(UserOptions *options, DmtxImage *image,
      DmtxRegion *region, DmtxMessage *message, int pageIndex);
static void WriteImagePnm(UserOptions *options, DmtxDecode *decode,
      DmtxMessage *message, int sizeIdx, char *imagePath);

char *programName;

/**
 * Main function for the dmtxread Data Matrix scanning utility.
 *
 * @param argc count of arguments passed from command line
 * @param argv list of argument passed strings from command line
 * @return     numeric success / failure exit code
 */
int
main(int argc, char *argv[])
{
   int err;
   int status;
   int fileIndex;
   int pageIndex;
   UserOptions options;
   DmtxImage *image;
   DmtxDecode decode;
   DmtxRegion region;
   DmtxMessage *message;
   DmtxPixelLoc p0, p1;

   SetOptionDefaults(&options);

   err = HandleArgs(&options, &fileIndex, &argc, &argv);
   if(err)
      ShowUsage(err);

   /* Loop once for each page of each image listed in parameters */
   for(pageIndex = 0; fileIndex < argc;) {

      /* Load image page from file (many formats are single-page only) */
      image = LoadImage(argv[fileIndex], pageIndex++);

      /* If requested page did not load then move to the next image */
      if(image == NULL) {
         fileIndex++;
         pageIndex = 0;
         continue;
      }

      assert(image->pageCount > 0 && pageIndex <= image->pageCount);

      /* Set image region to be scanned XXX this is a poorly named function */
      err = SetScanRegion(&p0, &p1, &options, image); /* XXX parameters are not intuitive */
      if(err)
         continue;

      /* Initialize decode struct for newly loaded image */
      decode = dmtxDecodeStructInit(image, p0, p1, options.scanGap);

      /* Loop once for each detected barcode region */
      for(;;) {

         /* Find next barcode region within image, but do not decode yet */
         region = dmtxDecodeFindNextRegion(&decode);
         if(region.found == DMTX_REGION_EOF)
            break;

         if(options.diagnose)
            WriteImagePnm(&options, &decode, message, region.sizeIdx, "debug.pnm");

         /* Decode region based on requested scan mode */
/*       message = (options.mosaic) ? dmtxDecodeMosaic(&region) : dmtxDecodeMatrix(&region); */
         message = dmtxDecodeMatrixRegion(&decode, &region);
         if(message == NULL)
            continue;

         PrintDecodedOutput(&options, image, &region, message, pageIndex);

         dmtxMessageFree(&message);
         break; /* XXX for now, break after first barcode is found in image */
      }

      dmtxDecodeStructDeInit(&decode);
      dmtxImageFree(&image);
   }

   exit(0);
}

/**
 *
 *
 */
static void
SetOptionDefaults(UserOptions *options)
{
   memset(options, 0x00, sizeof(UserOptions));

   /* Set default options */
   options->codewords = 0;
   options->scanGap = 2;
   options->newline = 0;
   options->xRangeMin = NULL;
   options->xRangeMax = NULL;
   options->yRangeMin = NULL;
   options->yRangeMax = NULL;
   options->verbose = 0;
   options->coordinates = 0;
   options->diagnose = 0;
   options->pageNumber = 0;
}

/**
 * Sets and validates user-requested options from command line arguments.
 *
 * @param options    runtime options from defaults or command line
 * @param argcp      pointer to argument count
 * @param argvp      pointer to argument list
 * @param fileIndex  pointer to index of first non-option arg (if successful)
 * @return           DMTXREAD_SUCCESS | DMTXREAD_ERROR
 */
static int
HandleArgs(UserOptions *options, int *fileIndex, int *argcp, char **argvp[])
{
   int err;
   int opt;
   int longIndex;
   char *ptr;

   struct option longOptions[] = {
         {"codewords",   no_argument,       NULL, 'c'},
         {"gap",         required_argument, NULL, 'g'},
         {"newline",     no_argument,       NULL, 'n'},
         {"x-range-min", required_argument, NULL, 'x'},
         {"x-range-max", required_argument, NULL, 'X'},
         {"y-range-min", required_argument, NULL, 'y'},
         {"y-range-max", required_argument, NULL, 'Y'},
         {"verbose",     no_argument,       NULL, 'v'},
         {"coordinates", no_argument,       NULL, 'C'},
         {"diagnose",    no_argument,       NULL, 'D'},
         {"mosaic",      no_argument,       NULL, 'M'},
         {"page-number", no_argument,       NULL, 'P'},
         {"version",     no_argument,       NULL, 'V'},
         {"help",        no_argument,       NULL,  0 },
         {0, 0, 0, 0}
   };

   programName = (*argvp)[0];
   if(programName && strrchr(programName, '/'))
      programName = strrchr(programName, '/') + 1;

   *fileIndex = 0;

   for(;;) {
      opt = getopt_long(*argcp, *argvp, "cg:nx:X:y:Y:vCDMPV", longOptions, &longIndex);
      if(opt == -1)
         break;

      switch(opt) {
         case 0: /* --help */
            ShowUsage(0);
            break;
         case 'c':
            options->codewords = 1;
            break;
         case 'g':
            err = StringToInt(&(options->scanGap), optarg, &ptr);
            if(err != DMTXREAD_SUCCESS || options->scanGap <= 0 || *ptr != '\0')
               FatalError(1, _("Invalid gap specified \"%s\""), optarg);
            break;
         case 'n':
            options->newline = 1;
            break;
         case 'x':
            options->xRangeMin = optarg;
            break;
         case 'X':
            options->xRangeMax = optarg;
            break;
         case 'y':
            options->yRangeMin = optarg;
            break;
         case 'Y':
            options->yRangeMax = optarg;
            break;
         case 'v':
            options->verbose = 1;
            break;
         case 'C':
            options->coordinates = 1;
            break;
         case 'D':
            options->diagnose = 1;
            break;
         case 'M':
            options->mosaic = 1;
            break;
         case 'P':
            options->pageNumber = 1;
            break;
         case 'V':
            fprintf(stdout, "%s version %s\n", programName, DMTX_VERSION);
            fprintf(stdout, "libdmtx version %s\n", dmtxVersion());
            exit(0);
            break;
         default:
            return DMTXREAD_ERROR;
            break;
      }
   }
   *fileIndex = optind;

   /* File not specified */
   if(*fileIndex == *argcp) {

      if(*argcp == 1) /* program called without arguments */
         return DMTXREAD_ERROR;
      else
         FatalError(1, _("Must specify image file"));

   }

   return DMTXREAD_SUCCESS;
}

/**
 *
 *
 */
static int
SetRangeLimit(int *target, char *optionString, int minMax, int limit)
{
   int err;
   int value;
   char *terminate;

   if(optionString == NULL) {
      *target = minMax * limit;
   }
   if(optionString) {
      err = StringToInt(&value, optionString, &terminate);
      if(err != DMTXREAD_SUCCESS)
         return DMTXREAD_ERROR;
      *target = (*terminate == '%') ? (int)(0.01 * value * limit + 0.5) : value;
   }

   return DMTXREAD_SUCCESS;
}

/**
 *
 *
 */
static int
SetScanRegion(DmtxPixelLoc *pMin, DmtxPixelLoc *pMax, UserOptions *options, DmtxImage *image)
{
   assert(options && image && image->width != 0 && image->height != 0);

   if(SetRangeLimit(&(pMin->X), options->xRangeMin, 0, image->width - 1) == DMTXREAD_ERROR ||
         SetRangeLimit(&(pMin->Y), options->yRangeMin, 0, image->height - 1) == DMTXREAD_ERROR ||
         SetRangeLimit(&(pMax->X), options->xRangeMax, 1, image->width - 1) == DMTXREAD_ERROR ||
         SetRangeLimit(&(pMax->Y), options->yRangeMax, 1, image->height - 1) == DMTXREAD_ERROR) {
      fprintf(stderr, _("Badly formed range parameter\n\n"));
      return DMTXREAD_ERROR;
   }

   if(pMin->X >= pMax->X || pMin->Y >= pMax->Y)
      FatalError(2, _("Specified range has non-positive area"));

   if(pMin->X < 0 || pMax->X > image->width - 1 ||
         pMin->Y < 0 || pMax->Y > image->height - 1) {
      fprintf(stderr, _("Specified range extends beyond image boundaries\n\n"));
      return DMTXREAD_ERROR;
   }

   return DMTXREAD_SUCCESS;
}

/**
 * Convert a string of characters to an integer.  If string cannot be
 * converted then the function will abort the program.
 *
 * @param numberString pointer to string of numbers
 * @return             converted long
 */
static int
StringToInt(int *numberInt, char *numberString, char **terminate)
{
   long numberLong;

   errno = 0;
   numberLong = strtol(numberString, terminate, 10);

   while(isspace((int)**terminate))
      (*terminate)++;

   if(errno != 0 || (**terminate != '\0' && **terminate != '%')) {
      *numberInt = -1;
      return DMTXREAD_ERROR;
   }

   *numberInt = (int)numberLong;
   return DMTXREAD_SUCCESS;
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
/*to STDOUT.  Note that %s may find multiple barcodes in one image.\n\*/
      fprintf(stdout, _("\
Scan image FILE for Data Matrix barcodes and print decoded results to\n\
STDOUT.  Note: %s currently stops scanning after it decodes the\n\
first barcode in an image.\n\
\n\
Example: (scans top third of images using minimum gap size of 10 pixels)\n\
\n\
   %s -Y33%% -g10 IMAGE001.png IMAGE002.png\n\
\n\
OPTIONS:\n"), programName, programName);
      fprintf(stdout, _("\
  -c, --codewords            print codewords extracted from barcode pattern\n\
  -g, --gap=NUM              use scan grid with gap of NUM pixels between lines\n\
  -n, --newline              print newline character at the end of decoded data\n\
  -d, --distortion=K1,K2     radial distortion coefficients (not implemented)\n\
  -x, --x-range-min=N[%%]     do not scan pixels to the left of N (or N%%)\n\
  -X, --x-range-max=N[%%]     do not scan pixels to the right of N (or N%%)\n\
  -y, --y-range-min=N[%%]     do not scan pixels above N (or N%%)\n\
  -Y, --y-range-max=N[%%]     do not scan pixels below N (or N%%)\n\
  -C, --coordinates          prefix decoded message with corner locations\n\
  -D, --diagnose=[op]        create copy of original image with diagnostic data\n\
      o = Overlay            overlay image with module colors\n\
      p = Path               capture path taken by scanning logic\n\
  -M, --mosaic               interpret detected regions as Data Mosaic barcodes\n\
  -P, --page-number          prefix decoded message with fax/tiff page number\n\
  -v, --verbose              use verbose messages\n\
  -V, --version              print program version information\n\
      --help                 display this help and exit\n"));
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

   /* XXX Right now this determines file type based on filename extension.
    * XXX Not ideal -- but only temporary. */
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
static DmtxImage *
LoadImage(char *imagePath, int pageIndex)
{
   DmtxImage *image;

   switch(GetImageFormat(imagePath)) {
      case ImageFormatPng:
         image = (pageIndex == 0) ? LoadImagePng(imagePath) : NULL;
         break;
      case ImageFormatTiff:
         image = LoadImageTiff(imagePath, pageIndex);
         break;
      default:
         image = NULL;
         FatalError(1, _("Unrecognized file type \"%s\""), imagePath);
         break;
   }

   return image;
}

/**
 * Load data from PNG file into DmtxImage format.
 *
 * @param image     pointer to DmtxImage structure to be populated
 * @param imagePath path/name of PNG image
 * @return          DMTX_SUCCESS | DMTX_FAILURE
 */
static DmtxImage *
LoadImagePng(char *imagePath)
{
   DmtxImage       *image;
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
   png_color_16p   image_background;

   /* XXX should be setting set_jmpbuf */

   fp = fopen(imagePath, "rb");
   if(fp == NULL) {
      perror(programName);
      return NULL;
   }

   fread(pngHeader, 1, sizeof(pngHeader), fp);
   isPng = !png_sig_cmp(pngHeader, 0, sizeof(pngHeader));
   if(!isPng)
      return NULL;

   pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

   if(pngPtr == NULL) {
      png_destroy_read_struct(&pngPtr, (png_infopp)NULL, (png_infopp)NULL);
      return NULL;
   }

   infoPtr = png_create_info_struct(pngPtr);
   if(infoPtr == NULL) {
      png_destroy_read_struct(&pngPtr, (png_infopp)NULL, (png_infopp)NULL);
      return NULL;
   }

   endInfo = png_create_info_struct(pngPtr);
   if(endInfo == NULL) {
      png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)NULL);
      return NULL;
   }

   if(setjmp(png_jmpbuf(pngPtr))) {
      png_destroy_read_struct(&pngPtr, &infoPtr, &endInfo);
      fclose(fp);
      return NULL;
   }

   png_init_io(pngPtr, fp);
   png_set_sig_bytes(pngPtr, sizeof(pngHeader));

   png_read_info(pngPtr, infoPtr);
   png_get_IHDR(pngPtr, infoPtr, &width, &height, &bitDepth, &colorType,
         &interlaceType, &compressionType, &filterMethod);

   if(colorType == PNG_COLOR_TYPE_PALETTE && bitDepth <= 8)
      png_set_expand(pngPtr);

   if(colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
      png_set_expand(pngPtr);

   if(png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS))
      png_set_expand(pngPtr);

   if(bitDepth < 8)
      png_set_packing(pngPtr);

   png_set_strip_16(pngPtr);
   png_set_strip_alpha(pngPtr);
   png_set_packswap(pngPtr);

   if(colorType == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(pngPtr);

   if(colorType == PNG_COLOR_TYPE_GRAY || PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(pngPtr);

   if(png_get_bKGD(pngPtr, infoPtr, &image_background))
      png_set_background(pngPtr, image_background, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);

   png_read_update_info(pngPtr, infoPtr);

   png_get_IHDR(pngPtr, infoPtr, &width, &height, &bitDepth, &colorType,
         &interlaceType, &compressionType, &filterMethod);

   rowPointers = (png_bytepp)png_malloc(pngPtr, sizeof(png_bytep) * height);
   if(rowPointers == NULL) {
      perror(programName);
      /* free first? */
      return NULL;
   }

   for(row = 0; row < height; row++) {
      rowPointers[row] = (png_bytep)png_malloc(pngPtr,
            png_get_rowbytes(pngPtr, infoPtr));
      assert(rowPointers[row] != NULL);
   }

   png_read_image(pngPtr, rowPointers);
   png_read_end(pngPtr, infoPtr);

   png_destroy_read_struct(&pngPtr, &infoPtr, &endInfo);

   image = dmtxImageMalloc(width, height);
   if(image == NULL) {
      perror(programName);
      /* free first? */
      return NULL;
   }

   /* This copy reverses row order top-to-bottom so image coordinate system
      corresponds with normal "right-handed" 2D space */
   for(row = 0; row < image->height; row++) {
      memcpy(image->pxl + (row * image->width),
            rowPointers[image->height - row - 1],
            image->width * sizeof(DmtxPixel));
   }

   for(row = 0; row < height; row++) {
      png_free(pngPtr, rowPointers[row]);
   }
   png_free(pngPtr, rowPointers);
   rowPointers = NULL;

   fclose(fp);

   return image;
}

/**
 * Load data from TIFF file into DmtxImage format.
 *
 * @param image    pointer to DmtxImage structure to be populated
 * @param filename path/name of PNG image
 * @return         number of pages contained in image file
 */
static DmtxImage *
LoadImageTiff(char *imagePath, int pageIndex)
{
   DmtxImage *image;
   int dirIndex = 0;
   TIFF* tif;
   int row, col, offset;
   uint32 w, h;
   uint32* raster;
   size_t npixels;

   image = NULL;

   tif = TIFFOpen(imagePath, "r");
   if(tif == NULL) {
      perror(programName);
      return NULL;
   }

   do {
      if(dirIndex == pageIndex) {
         TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
         TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
         npixels = w * h;

         raster = (uint32*) _TIFFmalloc(npixels * sizeof (uint32));
         if(raster == NULL) {
            perror(programName);
            /* free before returning */
            return NULL;
         }

         if(TIFFReadRGBAImage(tif, w, h, raster, 0)) {

            image = dmtxImageMalloc(w, h);
            if(image == NULL) {
               perror(programName);
               /* free first? */
               return NULL;
            }

            /* This copy reverses row order top-to-bottom so image coordinate system
               corresponds with normal "right-handed" 2D space */
            for(row = 0; row < image->height; row++) {
               for(col = 0; col < image->width; col++) {
                  /* XXX TIFF uses ABGR packed */
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

   if(image != NULL)
      image->pageCount = dirIndex;

   return image;
}

/**
 * XXX
 *
 * @param options   runtime options from defaults or command line
 * @param decode    pointer to DmtxDecode struct
 * @return          DMTXREAD_SUCCESS | DMTXREAD_ERROR
 */
static int
PrintDecodedOutput(UserOptions *options, DmtxImage *image,
      DmtxRegion *region, DmtxMessage *message, int pageIndex)
{
   int i;
   int dataWordLength;
   int rotateInt;
   double rotate;

   dataWordLength = dmtxGetSymbolAttribute(DmtxSymAttribDataWordLength, region->sizeIdx);
   if(options->verbose) {

      rotate = (2 * M_PI) + (atan2(region->fit2raw[0][1], region->fit2raw[1][1]) -
            atan2(region->fit2raw[1][0], region->fit2raw[0][0])) / 2.0;

      rotateInt = (int)(rotate * 180/M_PI + 0.5);
      if(rotateInt >= 360)
         rotateInt -= 360;

      fprintf(stdout, "--------------------------------------------------\n");
      fprintf(stdout, "       Matrix Size: %d x %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, region->sizeIdx),
            dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, region->sizeIdx));
      fprintf(stdout, "    Data Codewords: %d (capacity %d)\n",
            message->outputIdx, dataWordLength);
      fprintf(stdout, "   Error Codewords: %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribErrorWordLength, region->sizeIdx));
      fprintf(stdout, "      Data Regions: %d x %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, region->sizeIdx),
            dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, region->sizeIdx));
      fprintf(stdout, "Interleaved Blocks: %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, region->sizeIdx));
      fprintf(stdout, "    Rotation Angle: %d\n", rotateInt);
      fprintf(stdout, "          Corner 0: (%0.1f, %0.1f)\n",
            region->corners.c00.X, region->corners.c00.Y);
      fprintf(stdout, "          Corner 1: (%0.1f, %0.1f)\n",
            region->corners.c10.X, region->corners.c10.Y);
      fprintf(stdout, "          Corner 2: (%0.1f, %0.1f)\n",
            region->corners.c11.X, region->corners.c11.Y);
      fprintf(stdout, "          Corner 3: (%0.1f, %0.1f)\n",
            region->corners.c01.X, region->corners.c01.Y);
      fprintf(stdout, "--------------------------------------------------\n");
   }

   if(options->pageNumber)
      fprintf(stdout, "%d:", pageIndex + 1);

   if(options->coordinates) {
      fprintf(stdout, "%d,%d:", (int)(region->corners.c00.X + 0.5),
            image->height - (int)(region->corners.c00.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c10.X + 0.5),
            image->height - (int)(region->corners.c10.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c11.X + 0.5),
            image->height - (int)(region->corners.c11.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c01.X + 0.5),
            image->height - (int)(region->corners.c01.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c00.X + 0.5),
            image->height - (int)(region->corners.c00.Y + 0.5));
   }

   if(options->codewords) {
      for(i = 0; i < message->codeSize; i++) {
         fprintf(stdout, "%c:%03d\n", (i < dataWordLength) ?
               'd' : 'e', message->code[i]);
      }
   }
   else {
      fwrite(message->output, sizeof(char), message->outputIdx, stdout);
      if(options->newline)
         fputc('\n', stdout);
   }

   return DMTXREAD_SUCCESS;
}

/**
 *
 */
static void
WriteImagePnm(UserOptions *options, DmtxDecode *decode,
      DmtxMessage *message, int sizeIdx, char *imagePath)
{
   int row, col;
   int moduleStatus;
   int symbolRows, symbolCols;
   FILE *fp;

   symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, sizeIdx);
   symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, sizeIdx);

   fp = fopen(imagePath, "wb");
   if(fp == NULL) {
      perror(programName);
      exit(3);
   }

   /* Flip rows top-to-bottom to account for PNM "top-left" origin */
   fprintf(fp, "P6 %d %d 255 ", symbolCols, symbolRows);
   for(row = symbolRows - 1; row  >= 0; row--) {
      for(col = 0; col < symbolCols; col++) {
         moduleStatus = dmtxSymbolModuleStatus(message, sizeIdx, row, col);

         fputc((moduleStatus & DMTX_MODULE_ON_RED) ? 0 : 255, fp);
         fputc((moduleStatus & DMTX_MODULE_ON_GREEN) ? 0 : 255, fp);
         fputc((moduleStatus & DMTX_MODULE_ON_BLUE) ? 0 : 255, fp);
      }
   }

   fclose(fp);
}
