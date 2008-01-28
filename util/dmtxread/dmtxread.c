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
#include <stdarg.h>
#include <assert.h>
#include <png.h>
#include <tiffio.h>
#include <dmtx.h>
#include "dmtxread.h"

#define DMTXREAD_SUCCESS     0
#define DMTXREAD_ERROR       1

#define MIN(x,y) ((x < y) ? x : y)

static void InitUserOptions(UserOptions *options);
static int HandleArgs(UserOptions *options, int *argcp, char **argvp[], int *fileIndex, DmtxDecode *decode);
static long StringToLong(char *numberString);
static void ShowUsage(int status);
static void FatalError(int errorCode, char *fmt, ...);
static ImageFormat GetImageFormat(char *imagePath);
static int LoadImage(DmtxImage *image, char *imagePath, int imageIndex);
static int LoadImagePng(DmtxImage *image, char *imagePath);
static int LoadImageTiff(DmtxImage *image, char *imagePath, int imageIndex);
static int PrintDecodedOutput(UserOptions *options, DmtxDecode *decode, int imageIndex);
static void WriteImagePnm(UserOptions *options, DmtxDecode *decode, char *imagePath);

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
   DmtxDecode decode;
   DmtxMatrixRegion region;
   UserOptions options;
   char *imagePath;
   int imageCount;
   int imageIndex;
   int count;
   DmtxPixelLoc p0, p1;

   InitUserOptions(&options);

   memset(&decode, 0x00, sizeof(DmtxDecode));
   memset(&region, 0x00, sizeof(DmtxMatrixRegion));

   // XXX remove decode from HandleArgs() parameter list
   err = HandleArgs(&options, &argc, &argv, &fileIndex, &decode);
   if(err)
      ShowUsage(err);

   while(fileIndex < argc) {

      imagePath = argv[fileIndex];

      // Loop through all "pages" present in current image
      imageIndex = 0;
      do {
         dmtxImageInit(&decode.image);

         imageCount = LoadImage(&decode.image, imagePath, imageIndex);

         if(imageCount == 0)
            break;

         p0.X = p0.Y = 0;
         p1.X = decode.image.width - 1;
         p1.Y = decode.image.height - 1;

         decode = dmtxDecodeInit(&decode.image, p0, p1, options.scanGap);

/* XXX and this is how it should be (very soon) ...

         region = dmtxFindNextRegion(&decode);

         if(region.status == DMTX_NO_MORE)
            break;
         else if(region.status == DMTX_INVALID)
            continue; (can this even happen?)

         if(options.mosaic)
            mosaic = dmtxDecodeMosaic(&region);
         else
            matrix = dmtxDecodeMatrix(&region);

         // use it here

         dmtxDeInitMatrix(&matrix);
         dmtxDeInitMosaic(&matrix);
*/

         count = dmtxFindNextRegion(&decode);
         if(count == 0)
            continue;

         // XXX later change this to a opt-in debug option
         WriteImagePnm(&options, &decode, imagePath);

         PrintDecodedOutput(&options, &decode, imageIndex);

         dmtxImageDeInit(&decode.image);
      } while(++imageIndex < imageCount);

      fileIndex++;
   }

   exit(0);
}

/**
 *
 *
 */
static void
InitUserOptions(UserOptions *options)
{
   memset(options, 0x00, sizeof(UserOptions));

   // Set default options
   options->codewords = 0;
   options->scanGap = 2;
   options->newline = 0;
   options->region = NULL;
   options->verbose = 0;
   options->coordinates = 0;
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
HandleArgs(UserOptions *options, int *argcp, char **argvp[], int *fileIndex, DmtxDecode *decode)
{
   int opt;
   int longIndex;

   struct option longOptions[] = {
         {"codewords",   no_argument,       NULL, 'c'},
         {"gap",         required_argument, NULL, 'g'},
         {"newline",     no_argument,       NULL, 'n'},
         {"region",      required_argument, NULL, 'r'},
         {"verbose",     no_argument,       NULL, 'v'},
         {"coordinates", no_argument,       NULL, 'C'},
         {"mosaic",      no_argument,       NULL, 'M'},
         {"page-number", no_argument,       NULL, 'P'},
         {"help",        no_argument,       NULL,  0 },
         {0, 0, 0, 0}
   };

   programName = (*argvp)[0];
   if(programName && strrchr(programName, '/'))
      programName = strrchr(programName, '/') + 1;

   *fileIndex = 0;

   for(;;) {
      opt = getopt_long(*argcp, *argvp, "cg:nr:vCMP", longOptions, &longIndex);
      if(opt == -1)
         break;

      switch(opt) {
         case 0: // --help
            ShowUsage(0);
            break;
         case 'c':
            options->codewords = 1;
            break;
         case 'g':
            options->scanGap = StringToLong(optarg);
            break;
         case 'n':
            options->newline = 1;
            break;
         case 'r':
            options->region = optarg;
            break;
         case 'v':
            options->verbose = 1;
            break;
         case 'C':
            options->coordinates = 1;
            break;
         case 'M':
            decode->mosaic = 1;
            break;
         case 'P':
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

      if(*argcp == 1) // program called without arguments
         return DMTXREAD_ERROR;
      else
         FatalError(1, _("Must specify image file"));

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
Scan FILE (PNG or TIFF image) for Data Matrix barcodes and print decoded results\n\
to STDOUT.  Note that %s may find multiple barcodes in one image.\n\
\n\
Example: %s -r10,10:200,200 -g10 IMAGE001.png IMAGE002.png\n\
\n\
OPTIONS:\n"), programName, programName);
      fprintf(stdout, _("\
  -c, --codewords        print codewords extracted from barcode pattern\n\
  -g, --gap=NUM          use scan grid with gap of NUM pixels between scanlines\n\
  -n, --newline          insert newline character at the end of decoded data\n\
  -d, --distortion=K1,K2 radial distortion coefficients (not implemented yet)\n\
  -r, --region=REGION    only scan a subset of image; specify REGION by listing\n\
        x1,y1:x2,y2      opposite corners of a rectangle region (not implemented)\n\
  -v, --verbose          use verbose messages\n\
  -C, --coordinates      prefix decoded message with barcode corner locations\n\
  -M, --mosaic           interpret detected regions as Data Mosaic barcodes\n\
  -P, --page-number      prefix decoded message with corresponding page number\n\
      --help             display this help and exit\n"));
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
         imageCount = LoadImagePng(image, imagePath);
         break;
      case ImageFormatTiff:
         imageCount = LoadImageTiff(image, imagePath, imageIndex);
         break;
      default:
         FatalError(1, _("Unrecognized file type"));
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
LoadImagePng(DmtxImage *image, char *imagePath)
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
   png_color_16p   image_background;

   // TODO: set_jmpbuf

   fp = fopen(imagePath, "rb");
   if(fp == NULL) {
      perror(programName);
      return 0;
   }

   fread(pngHeader, 1, sizeof(pngHeader), fp);
   isPng = !png_sig_cmp(pngHeader, 0, sizeof(pngHeader));
   if(!isPng)
      return 0;

   pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

   if(pngPtr == NULL) {
      png_destroy_read_struct(&pngPtr, (png_infopp)NULL, (png_infopp)NULL);
      return 0;
   }

   infoPtr = png_create_info_struct(pngPtr);
   if(infoPtr == NULL) {
      png_destroy_read_struct(&pngPtr, (png_infopp)NULL, (png_infopp)NULL);
      return 0;
   }

   endInfo = png_create_info_struct(pngPtr);
   if(endInfo == NULL) {
      png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)NULL);
      return 0;
   }

   if(setjmp(png_jmpbuf(pngPtr))) {
      png_destroy_read_struct(&pngPtr, &infoPtr, &endInfo);
      fclose(fp);
      return 0;
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
      // free first?
      return 0;
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
      // free first?
      return 0;
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
   rowPointers = NULL;

   fclose(fp);

   return 1;
}

/**
 * Load data from TIFF file into DmtxImage format.
 *
 * @param image    pointer to DmtxImage structure to be populated
 * @param filename path/name of PNG image
 * @return         number of images loaded
 */
static int
LoadImageTiff(DmtxImage *image, char *imagePath, int imageIndex)
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
      return 0;
   }

   do {
      if(dirIndex == imageIndex) {
         TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
         TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
         npixels = w * h;

         raster = (uint32*) _TIFFmalloc(npixels * sizeof (uint32));
         if(raster == NULL) {
            perror(programName);
            // free before returning
            return 0;
         }

         if(TIFFReadRGBAImage(tif, w, h, raster, 0)) {
            // Use TIFF information to populate DmtxImage information
            image->width = w;
            image->height = h;

            image->pxl = (DmtxPixel *)malloc(image->width * image->height * sizeof(DmtxPixel));
            if(image->pxl == NULL) {
               perror(programName);
               // free before returning
               return 0;
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
 * XXX
 *
 * @param options   runtime options from defaults or command line
 * @param decode    pointer to DmtxDecode struct
 * @return          DMTXREAD_SUCCESS | DMTXREAD_ERROR
 */
static int
PrintDecodedOutput(UserOptions *options, DmtxDecode *decode, int imageIndex)
{
   int i;
   int dataWordLength;
   DmtxMatrixRegion *region;

// region = dmtxDecodeGetMatrix(decode, 0);
// if(region == NULL)
//    return DMTXREAD_ERROR;

   region = &(decode->matrix);

// if(region->status != DMTX_STATUS_VALID)
//    return DMTXREAD_ERROR;

   dataWordLength = dmtxGetSymbolAttribute(DmtxSymAttribDataWordLength, region->sizeIdx);
   if(options->verbose) {
      fprintf(stdout, "--------------------------------------------------\n");
      fprintf(stdout, "       Matrix Size: %d x %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, region->sizeIdx),
            dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, region->sizeIdx));
      fprintf(stdout, "    Data Codewords: %d (capacity %d)\n",
            region->outputIdx, dataWordLength);
      fprintf(stdout, "   Error Codewords: %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribErrorWordLength, region->sizeIdx));
      fprintf(stdout, "      Data Regions: %d x %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, region->sizeIdx),
            dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, region->sizeIdx));
      fprintf(stdout, "Interleaved Blocks: %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, region->sizeIdx));
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
      fprintf(stdout, "%d:", imageIndex + 1);

   if(options->coordinates) {
      fprintf(stdout, "%d,%d:", (int)(region->corners.c00.X + 0.5),
            decode->image.height - (int)(region->corners.c00.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c10.X + 0.5),
            decode->image.height - (int)(region->corners.c10.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c11.X + 0.5),
            decode->image.height - (int)(region->corners.c11.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c01.X + 0.5),
            decode->image.height - (int)(region->corners.c01.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c00.X + 0.5),
            decode->image.height - (int)(region->corners.c00.Y + 0.5));
   }

   if(options->codewords) {
      for(i = 0; i < region->codeSize; i++) {
         fprintf(stdout, "%c:%03d\n", (i < dataWordLength) ?
               'd' : 'e', region->code[i]);
      }
   }
   else {
      fwrite(region->output, sizeof(char), region->outputIdx, stdout);
      if(options->newline)
         fputc('\n', stdout);
   }

   return DMTXREAD_SUCCESS;
}

/**
 *
 */
static void
WriteImagePnm(UserOptions *options, DmtxDecode *decode, char *imagePath)
{
   int row, col;
   int moduleStatus;
   FILE *fp;
   DmtxMatrixRegion *matrix;

   matrix = &(decode->matrix);

   /* XXX later change this to append to the input filename */
   imagePath = "debug.pnm";

   /* Flip rows top-to-bottom to account for PNM "top-left" origin */
   fp = fopen(imagePath, "wb");
   if(fp == NULL) {
      perror(programName);
      exit(3);
   }

   fprintf(fp, "P6 %d %d 255 ", matrix->symbolCols, matrix->symbolRows);
   for(row = matrix->symbolRows - 1; row  >= 0; row--) {
      for(col = 0; col < matrix->symbolCols; col++) {
         moduleStatus = dmtxSymbolModuleStatus(matrix, row, col);

         fputc((moduleStatus & DMTX_MODULE_ON_RED) ? 0 : 255, fp);
         fputc((moduleStatus & DMTX_MODULE_ON_GREEN) ? 0 : 255, fp);
         fputc((moduleStatus & DMTX_MODULE_ON_BLUE) ? 0 : 255, fp);
      }
   }

   fclose(fp);
}
