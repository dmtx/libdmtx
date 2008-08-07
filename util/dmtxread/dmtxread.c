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
#include <dmtx.h>
#include "dmtxread.h"
#include "../common/dmtxutil.h"

#ifdef HAVE_LIBTIFF
#include <tiffio.h>
#endif

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
   int fileIndex;
   int pageIndex;
   int imgWidth, imgHeight;
   UserOptions options;
   DmtxTime  msec, *timeout;
   DmtxImage *img;
   DmtxDecode decode;
   DmtxRegion region;
   DmtxMessage *message;

   SetOptionDefaults(&options);

   err = HandleArgs(&options, &fileIndex, &argc, &argv);
   if(err != DMTX_SUCCESS)
      ShowUsage(err);

   timeout = (options.timeoutMS == -1) ? NULL : &msec;

   /* Loop once for each page of each image listed in parameters */
   for(pageIndex = 0; fileIndex < argc;) {

      /* Reset timeout for each new image */
      if(timeout != NULL)
         msec = dmtxTimeAdd(dmtxTimeNow(), options.timeoutMS);

      /* Load image page from file (many formats are single-page only) */
      img = LoadImage(argv[fileIndex], pageIndex++);

      /* If requested page did not load then move to the next image */
      if(img == NULL) {
         fileIndex++;
         pageIndex = 0;
         continue;
      }

      imgWidth = dmtxImageGetProp(img, DmtxPropWidth);
      imgHeight = dmtxImageGetProp(img, DmtxPropHeight);

      assert(img->pageCount > 0 && pageIndex <= img->pageCount);

      /* Initialize decode struct for newly loaded image */
      decode = dmtxDecodeStructInit(img);

      err = dmtxDecodeSetProp(&decode, DmtxPropShrinkMin, options.shrinkMin);
      assert(err == DMTX_SUCCESS);

      err = dmtxDecodeSetProp(&decode, DmtxPropShrinkMax, options.shrinkMax);
      assert(err == DMTX_SUCCESS);

      err = dmtxDecodeSetProp(&decode, DmtxPropEdgeThresh, options.edgeThresh);
      assert(err == DMTX_SUCCESS);

      err = dmtxDecodeSetProp(&decode, DmtxPropScanGap, options.scanGap);
      assert(err == DMTX_SUCCESS);

      if(options.squareDevn != -1) {
         err = dmtxDecodeSetProp(&decode, DmtxPropSquareDevn, options.squareDevn);
         assert(err == DMTX_SUCCESS);
      }

      if(options.xMin) {
         err = dmtxDecodeSetProp(&decode, DmtxPropXmin, ScaleNumberString(options.xMin, imgWidth));
         assert(err == DMTX_SUCCESS);
      }

      if(options.xMax) {
         err = dmtxDecodeSetProp(&decode, DmtxPropXmax, ScaleNumberString(options.xMax, imgWidth));
         assert(err == DMTX_SUCCESS);
      }

      if(options.yMin) {
         err = dmtxDecodeSetProp(&decode, DmtxPropYmin, ScaleNumberString(options.yMin, imgHeight));
         assert(err == DMTX_SUCCESS);
      }

      if(options.yMax) {
         err = dmtxDecodeSetProp(&decode, DmtxPropYmax, ScaleNumberString(options.yMax, imgHeight));
         assert(err == DMTX_SUCCESS);
      }

      /* Loop once for each detected barcode region */
      for(;;) {

         /* Find next barcode region within image, but do not decode yet */
         region = dmtxDecodeFindNextRegion(&decode, timeout);

         /* Finished file or ran out of time before finding another region */
         if(region.found != DMTX_REGION_FOUND)
            break;

         if(options.diagnose)
            WriteDiagnosticImage(&decode, &region, "debug.pnm");

         /* Decode region based on requested barcode mode */
         if(options.mosaic)
            message = dmtxDecodeMosaicRegion(&decode, &region, options.correctionsMax);
         else
            message = dmtxDecodeMatrixRegion(&decode, &region, options.correctionsMax);

         if(message == NULL)
            continue;

         PrintDecodedOutput(&options, img, &region, message, pageIndex);

         dmtxMessageFree(&message);
         break; /* XXX for now, break after first barcode is found in image */
      }

      dmtxDecodeStructDeInit(&decode);
      dmtxImageFree(&img);
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
   options->squareDevn = -1;
   options->scanGap = 2;
   options->timeoutMS = -1;
   options->newline = 0;
   options->shrinkMin = 1;
   options->shrinkMax = 1;
   options->edgeThresh = 10;
   options->xMin = NULL;
   options->xMax = NULL;
   options->yMin = NULL;
   options->yMax = NULL;
   options->correctionsMax = -1;
   options->diagnose = 0;
   options->mosaic = 0;
   options->pageNumber = 0;
   options->corners = 0;
   options->verbose = 0;
}

/**
 * Sets and validates user-requested options from command line arguments.
 *
 * @param options    runtime options from defaults or command line
 * @param argcp      pointer to argument count
 * @param argvp      pointer to argument list
 * @param fileIndex  pointer to index of first non-option arg (if successful)
 * @return           DMTX_SUCCESS | DMTX_FAILURE
 */
static int
HandleArgs(UserOptions *options, int *fileIndex, int *argcp, char **argvp[])
{
   int err;
   int opt;
   int longIndex;
   char *ptr;

   struct option longOptions[] = {
         {"codewords",        no_argument,       NULL, 'c'},
         {"gap",              required_argument, NULL, 'g'},
         {"milliseconds",     required_argument, NULL, 'm'},
         {"newline",          no_argument,       NULL, 'n'},
         {"square-deviation", required_argument, NULL, 'q'},
         {"shrink",           required_argument, NULL, 's'},
         {"threshold",        required_argument, NULL, 't'},
         {"x-range-min",      required_argument, NULL, 'x'},
         {"x-range-max",      required_argument, NULL, 'X'},
         {"y-range-min",      required_argument, NULL, 'y'},
         {"y-range-max",      required_argument, NULL, 'Y'},
         {"max-corrections",  required_argument, NULL, 'C'},
         {"diagnose",         no_argument,       NULL, 'D'},
         {"mosaic",           no_argument,       NULL, 'M'},
         {"page-number",      no_argument,       NULL, 'P'},
         {"corners",          no_argument,       NULL, 'R'},
         {"verbose",          no_argument,       NULL, 'v'},
         {"version",          no_argument,       NULL, 'V'},
         {"help",             no_argument,       NULL,  0 },
         {0, 0, 0, 0}
   };

   programName = Basename((*argvp)[0]);

   *fileIndex = 0;

   for(;;) {
      opt = getopt_long(*argcp, *argvp, "cg:m:nq:s:t:x:X:y:Y:vC:DMPRV", longOptions, &longIndex);
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
            if(err != DMTX_SUCCESS || options->scanGap <= 0 || *ptr != '\0')
               FatalError(1, _("Invalid gap specified \"%s\""), optarg);
            break;
         case 'm':
            err = StringToInt(&(options->timeoutMS), optarg, &ptr);
            if(err != DMTX_SUCCESS || options->timeoutMS < 0 || *ptr != '\0')
               FatalError(1, _("Invalid timeout (in milliseconds) specified \"%s\""), optarg);
            break;
         case 'n':
            options->newline = 1;
            break;
         case 'q':
            err = StringToInt(&(options->squareDevn), optarg, &ptr);
            if(err != DMTX_SUCCESS || *ptr != '\0' ||
                  options->squareDevn < 0 || options->squareDevn > 90)
               FatalError(1, _("Invalid squareness deviation specified \"%s\""), optarg);
            break;
         case 's':
            err = StringToInt(&(options->shrinkMax), optarg, &ptr);
            if(err != DMTX_SUCCESS || options->shrinkMax < 1 || *ptr != '\0')
               FatalError(1, _("Invalid shrink factor specified \"%s\""), optarg);
            /* XXX later also popular shrinkMin based on N-N range */
            break;
         case 't':
            err = StringToInt(&(options->edgeThresh), optarg, &ptr);
            if(err != DMTX_SUCCESS || *ptr != '\0' ||
                  options->edgeThresh < 1 || options->edgeThresh > 100)
               FatalError(1, _("Invalid edge threshold specified \"%s\""), optarg);
            break;
         case 'x':
            options->xMin = optarg;
            break;
         case 'X':
            options->xMax = optarg;
            break;
         case 'y':
            options->yMin = optarg;
            break;
         case 'Y':
            options->yMax = optarg;
            break;
         case 'v':
            options->verbose = 1;
            break;
         case 'C':
            err = StringToInt(&(options->correctionsMax), optarg, &ptr);
            if(err != DMTX_SUCCESS || options->correctionsMax < 0 || *ptr != '\0')
               FatalError(1, _("Invalid max corrections specified \"%s\""), optarg);
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
         case 'R':
            options->corners = 1;
            break;
         case 'V':
            fprintf(stdout, "%s version %s\n", programName, DMTX_VERSION);
            fprintf(stdout, "libdmtx version %s\n", dmtxVersion());
            exit(0);
            break;
         default:
            return DMTX_FAILURE;
            break;
      }
   }
   *fileIndex = optind;

   /* File not specified */
   if(*fileIndex == *argcp) {

      if(*argcp == 1) /* Program called without arguments */
         return DMTX_FAILURE;
      else
         FatalError(1, _("Must specify image file"));

   }

   return DMTX_SUCCESS;
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
Example: Scan top third of images using gap no larger than 10 pixels\n\
\n\
   %s -Y33%% -g10 IMAGE001.png IMAGE002.png\n\
\n\
OPTIONS:\n"), programName, programName);
      fprintf(stdout, _("\
  -c, --codewords            print codewords extracted from barcode pattern\n\
  -g, --gap=NUM              use scan grid with gap of NUM pixels between lines\n\
  -m, --milliseconds=N       stop scan after N milliseconds (per image)\n\
  -n, --newline              print newline character at the end of decoded data\n\
  -q, --square-deviation=N   allowed non-squareness of corners in degrees (0-90)\n\
  -s, --shrink=N             internally shrink image by a factor of N\n\
  -t, --threshold=N          ignore weak edges below threshold N (1-100)\n\
  -x, --x-range-min=N[%%]     do not scan pixels to the left of N (or N%%)\n\
  -X, --x-range-max=N[%%]     do not scan pixels to the right of N (or N%%)\n\
  -y, --y-range-min=N[%%]     do not scan pixels above N (or N%%)\n\
  -Y, --y-range-max=N[%%]     do not scan pixels below N (or N%%)\n\
  -C, --corrections-max=N    correct at most N errors (0 = correction disabled)\n\
  -D, --diagnose             make copy of image with additional diagnostic data\n\
  -M, --mosaic               interpret detected regions as Data Mosaic barcodes\n\
  -P, --page-number          prefix decoded message with fax/tiff page number\n\
  -R, --corners              prefix decoded message with corner locations\n\
  -v, --verbose              use verbose messages\n\
  -V, --version              print program version information\n\
      --help                 display this help and exit\n"));
      fprintf(stdout, _("\nReport bugs to <mike@dragonflylogic.com>.\n"));
   }

   exit(status);
}

/**
 *
 *
 */
static int
ScaleNumberString(char *s, int extent)
{
   int err;
   int numValue;
   int scaledValue;
   char *terminate;

   assert(s != NULL);

   err = StringToInt(&numValue, s, &terminate);
   if(err != DMTX_SUCCESS)
      FatalError(1, _("Integer value required"));

   scaledValue = (*terminate == '%') ? (int)(0.01 * numValue * extent + 0.5) : numValue;

   if(scaledValue < 0)
      scaledValue = 0;

   if(scaledValue >= extent)
      scaledValue = extent - 1;

   return scaledValue;
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
#ifdef HAVE_LIBTIFF
   else if(strncmp(extension, "tif", 3) == 0 || strncmp(extension, "TIF", 3) == 0)
      return ImageFormatTiff;
#endif

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
#ifdef HAVE_LIBTIFF
      case ImageFormatTiff:
         image = LoadImageTiff(imagePath, pageIndex);
         break;
#endif
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

   for(row = 0; row < height; row++)
      memcpy(image->pxl + (row * width), rowPointers[row], width * sizeof(DmtxRgb));

   for(row = 0; row < height; row++)
      png_free(pngPtr, rowPointers[row]);

   png_free(pngPtr, rowPointers);
   rowPointers = NULL;

   fclose(fp);

   return image;
}

#ifdef HAVE_LIBTIFF
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
   int row, col;
   int tiffOffset, dmtxOffset;
   uint32 width, height;
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
         TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
         TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
         npixels = width * height;

         raster = (uint32*) _TIFFmalloc(npixels * sizeof (uint32));
         if(raster == NULL) {
            perror(programName);
            /* free before returning */
            return NULL;
         }

         if(TIFFReadRGBAImage(tif, width, height, raster, 0)) {

            image = dmtxImageMalloc(width, height);
            if(image == NULL) {
               perror(programName);
               /* free first? */
               return NULL;
            }

            for(row = 0; row < height; row++) {
               for(col = 0; col < width; col++) {
                  dmtxOffset = dmtxImageGetOffset(image, col, row);
                  tiffOffset = row * image->width + col;

                  /* TIFF uses ABGR packed */
                  image->pxl[dmtxOffset][0] = raster[tiffOffset] & 0x000000ff;
                  image->pxl[dmtxOffset][1] = (raster[tiffOffset] & 0x0000ff00) >> 8;
                  image->pxl[dmtxOffset][2] = (raster[tiffOffset] & 0x00ff0000) >> 16;
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
#endif

/**
 * XXX
 *
 * @param options   runtime options from defaults or command line
 * @param decode    pointer to DmtxDecode struct
 * @return          DMTX_SUCCESS | DMTX_FAILURE
 */
static int
PrintDecodedOutput(UserOptions *options, DmtxImage *image,
      DmtxRegion *region, DmtxMessage *message, int pageIndex)
{
   int i;
   int height;
   int dataWordLength;
   int rotateInt;
   double rotate;

   dataWordLength = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, region->sizeIdx);
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
            dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords, region->sizeIdx));
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

   if(options->corners) {
      height = dmtxImageGetProp(image, DmtxPropHeight);
      fprintf(stdout, "%d,%d:", (int)(region->corners.c00.X + 0.5),
            height - (int)(region->corners.c00.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c10.X + 0.5),
            height - (int)(region->corners.c10.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c11.X + 0.5),
            height - (int)(region->corners.c11.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c01.X + 0.5),
            height - (int)(region->corners.c01.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c00.X + 0.5),
            height - (int)(region->corners.c00.Y + 0.5));
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

   return DMTX_SUCCESS;
}

/**
 *
 */
static void
WriteDiagnosticImage(DmtxDecode *dec, DmtxRegion *reg, char *imagePath)
{
   int row, col;
   int width, height;
   double shade;
   FILE *fp;
   DmtxRgb rgb;
   DmtxVector2 p;

   fp = fopen(imagePath, "wb");
   if(fp == NULL) {
      perror(programName);
      exit(3);
   }

   width = dmtxImageGetProp(dec->image, DmtxPropScaledWidth);
   height = dmtxImageGetProp(dec->image, DmtxPropScaledHeight);

   /* Test each pixel of input image to see if it lies in region */
   fprintf(fp, "P6\n%d %d\n255\n", width, height);
   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {

         dmtxImageGetRgb(dec->image, col, row, rgb);

         p.X = col;
         p.Y = row;
         dmtxMatrix3VMultiplyBy(&p, reg->raw2fit);

         if(p.X < 0.0 || p.X > 1.0 || p.Y < 0.0 || p.Y > 1.0)
            shade = 0.7;
         else if(p.X + p.Y < 1.0)
            shade = 0.0;
         else
            shade = 0.4;

         rgb[0] += (shade * (255 - rgb[0]));
         rgb[1] += (shade * (255 - rgb[1]));
         rgb[2] += (shade * (255 - rgb[2]));

         fwrite(rgb, sizeof(char), 3, fp);
      }
   }

   fclose(fp);
}
