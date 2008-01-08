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

Contact: mike@dragonflylogic.com
*/

/* $Id: dmtxwrite.c,v 1.5 2006/10/15 19:37:58 mblaughton Exp $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <png.h>
#include <dmtx.h>
#include "dmtxwrite.h"

#define DMTXWRITE_SUCCESS     0
#define DMTXWRITE_ERROR       1

#define DMTXWRITE_BUFFER_SIZE 4096

#define MIN(x,y) ((x < y) ? x : y)

static void InitUserOptions(UserOptions *options);
static int HandleArgs(UserOptions *options, int *argcp, char **argvp[], DmtxEncode *encode);
static void ReadData(UserOptions *options, int *codeBuffer, unsigned char *codeBufferSize);
static long StringToLong(char *numberString);
static void ShowUsage(int status);
static void FatalError(int errorCode, char *fmt, ...);
static void WriteImagePng(DmtxEncode *encode, char *path);
static void WriteImagePnm(DmtxEncode *encode, char *path);
static void WriteAsciiBarcode(DmtxEncode *encode);
static void WriteCodewords(DmtxEncode *encode);

char *programName;

/**
 * Main function for the dmtxwrite Data Matrix scanning utility.
 *
 * @param  argc count of arguments passed from command line
 * @param  argv list of argument passed strings from command line
 * @return      numeric success / failure exit code
 */
int
main(int argc, char *argv[])
{
   int err;
   UserOptions options;
   DmtxEncode *encode;
   unsigned char codeBuffer[DMTXWRITE_BUFFER_SIZE];
   int codeBufferSize = sizeof codeBuffer;

   InitUserOptions(&options);

   // Create and initialize libdmtx structures
   encode = dmtxEncodeStructCreate();

   // Process user options
   err = HandleArgs(&options, &argc, &argv, encode);
   if(err)
      ShowUsage(err);

   // Read input data into buffer
   ReadData(&options, &codeBufferSize, codeBuffer);

   // Create barcode image
   if(options.mosaic)
      dmtxEncodeDataMosaic(encode, codeBufferSize, codeBuffer, options.sizeIdx);
   else
      dmtxEncodeDataMatrix(encode, codeBufferSize, codeBuffer, options.sizeIdx);

   // Write barcode image to requested format
   switch(options.format) {
      case 'n':
         break; // do nothing
      case 'p':
         WriteImagePng(encode, options.outputPath);
         break;
      case 'm':
         WriteImagePnm(encode, options.outputPath);
         break;
   }

   // Write barcode preview if requested
   switch(options.preview) {
      case 'n':
         break; // do nothing
      case 'a':
         WriteAsciiBarcode(encode);
         break;
      case 'c':
         WriteCodewords(encode);
         break;
   }

   // Clean up
   dmtxEncodeStructDestroy(&encode);

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

// options->color = "";
// options->bgColor = "";
   options->format = 'p';
   options->inputPath = NULL;
   options->outputPath = "dmtxwrite.png"; // XXX needs to have different extensions for other formats
   options->preview = 'n';
   options->rotate = 0;
   options->sizeIdx = DMTX_SYMBOL_SQUARE_AUTO;
   options->verbose = 0;
   options->mosaic = 0;
}

/**
 * Sets and validates user-requested options from command line arguments.
 *
 * @param options runtime options from defaults or command line
 * @param argcp   pointer to argument count
 * @param argvp   pointer to argument list
 * @return        DMTXWRITE_SUCCESS | DMTXWRITE_ERROR
 */
static int
HandleArgs(UserOptions *options, int *argcp, char **argvp[], DmtxEncode *encode)
{
   int i;
   int opt;
   int longIndex;

   static char *symbolSizes[] = {
         "10x10", "12x12",   "14x14",   "16x16",   "18x18",   "20x20",
         "22x22", "24x24",   "26x26",   "32x32",   "36x36",   "40x40",
         "44x44", "48x48",   "52x52",   "64x64",   "72x72",   "80x80",
         "88x88", "96x96", "104x104", "120x120", "132x132", "144x144",
          "8x18",  "8x32",   "12x26",   "12x36",   "16x36",   "16x48"
   };

   struct option longOptions[] = {
         {"color",       required_argument, NULL, 'c'},
         {"bg-color",    required_argument, NULL, 'b'},
         {"module",      required_argument, NULL, 'd'},
         {"margin",      required_argument, NULL, 'm'},
         {"encoding",    required_argument, NULL, 'e'},
         {"format",      required_argument, NULL, 'f'},
         {"output",      required_argument, NULL, 'o'},
         {"preview",     required_argument, NULL, 'p'},
         {"rotate",      required_argument, NULL, 'r'},
         {"symbol-size", required_argument, NULL, 's'},
         {"verbose",     no_argument,       NULL, 'v'},
         {"mosaic",      no_argument,       NULL, 'M'},
         {"help",        no_argument,       NULL,  0 },
         {0, 0, 0, 0}
   };

   programName = (*argvp)[0];
   if(programName && strrchr(programName, '/'))
      programName = strrchr(programName, '/') + 1;

   // Set default values before considering arguments
   encode->moduleSize = 5;
   encode->marginSize = 10;
   encode->scheme = DmtxSchemeEncodeAscii;

   for(;;) {
      opt = getopt_long(*argcp, *argvp, "c:b:d:m:e:f:o:p:r:s:vM", longOptions, &longIndex);
      if(opt == -1)
         break;

      switch(opt) {
         case 0: // --help
            ShowUsage(0);
            break;
         case 'c':
            options->color.R = 0;
            options->color.G = 0;
            options->color.B = 0;
            fprintf(stdout, "Option \"%c\" not implemented yet\n", opt);
            break;
         case 'b':
            options->bgColor.R = 255;
            options->bgColor.G = 255;
            options->bgColor.B = 255;
            fprintf(stdout, "Option \"%c\" not implemented yet\n", opt);
            break;
         case 'd':
            encode->moduleSize = StringToLong(optarg);
            break;
         case 'm':
            encode->marginSize = StringToLong(optarg);
            break;
         case 'e':
            if(strlen(optarg) != 1) {
               fprintf(stdout, "Invalid encodation scheme \"%s\"\n", optarg);
               return DMTXWRITE_ERROR;
            }
            switch(*optarg) {
               case 'b':
                  encode->scheme = DmtxSchemeEncodeAutoBest;
                  break;
               case 'f':
                  encode->scheme = DmtxSchemeEncodeAutoFast;
                  fprintf(stdout, "\"Fast optimized\" not implemented yet\n", opt);
                  return DMTXWRITE_ERROR;
                  break;
               case 'a':
                  encode->scheme = DmtxSchemeEncodeAscii;
                  break;
               case 'c':
                  encode->scheme = DmtxSchemeEncodeC40;
                  break;
               case 't':
                  encode->scheme = DmtxSchemeEncodeText;
                  break;
               case 'x':
                  encode->scheme = DmtxSchemeEncodeX12;
                  break;
               case 'e':
                  encode->scheme = DmtxSchemeEncodeEdifact;
                  break;
               case '8':
                  encode->scheme = DmtxSchemeEncodeBase256;
                  break;
               default:
                  fprintf(stdout, "Invalid encodation scheme \"%s\"\n", optarg);
                  return DMTXWRITE_ERROR;
                  break;
            }
            break;
         case 'f':
            options->format = *optarg;

            if(options->format != 'n' && options->format != 'p' &&
                  options->format != 'm') {
               fprintf(stdout, "Invalid output format \"%c\"\n", options->format);
               return DMTXWRITE_ERROR;
            }

            break;
         case 'o':
            options->outputPath = optarg;
            break;
         case 'p':
            options->preview = *optarg;

            if(options->preview != 'n' && options->preview != 'a' &&
                  options->preview != 'c') {
               fprintf(stdout, "Invalid preview format \"%c\"\n", options->preview);
               return DMTXWRITE_ERROR;
            }

            break;
         case 'r':
            options->rotate = StringToLong(optarg);
            break;
         case 's':
            // Determine correct barcode size and/or shape
            if(*optarg == 's') {
               options->sizeIdx = DMTX_SYMBOL_SQUARE_AUTO;
            }
            else if(*optarg == 'r') {
               options->sizeIdx = DMTX_SYMBOL_RECT_AUTO;
            }
            else {
               for(i = 0; i < DMTX_SYMBOL_SQUARE_COUNT + DMTX_SYMBOL_RECT_COUNT; i++) {
                  if(strncmp(optarg, symbolSizes[i], 8) == 0) {
                     options->sizeIdx = i;
                     break;
                  }
               }
               if(i == DMTX_SYMBOL_SQUARE_COUNT + DMTX_SYMBOL_RECT_COUNT)
                  return DMTXWRITE_ERROR;
            }

            break;
         case 'v':
            options->verbose = 1;
            break;
         case 'M':
            options->mosaic = 1;
            break;
         default:
            return DMTXWRITE_ERROR;
            break;
      }
   }

   options->inputPath = (*argvp)[optind];

   return DMTXWRITE_SUCCESS;
}

/**
 *
 */
static void
ReadData(UserOptions *options, int *codeBufferSize, unsigned char *codeBuffer)
{
   int fd;

   /* Open file or stdin for reading */
   if(options->inputPath == NULL) {
      fd = 0;
   }
   else {
      fd = open(options->inputPath, O_RDONLY);
      if(fd == -1) {
         FatalError(1, _("Error while opening file \"%s\""), options->inputPath);
      }
   }

   /* Read input contents into buffer */
   *codeBufferSize = read(fd, codeBuffer, DMTXWRITE_BUFFER_SIZE);
   if(*codeBufferSize == DMTXWRITE_BUFFER_SIZE)
      FatalError(1, _("Message to be encoded is too large"));

   /* Close file only if not stdin */
   if(fd != 0 && close(fd) != 0)
      FatalError(1, _("Error while closing file"));
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
      fprintf(stderr, _("Usage: %s [OPTION]... DATA\n"), programName);
      fprintf(stderr, _("Try `%s --help' for more information.\n"), programName);
   }
   else {
      fprintf(stdout, _("Usage: %s [OPTION]... DATA\n"), programName);
      fprintf(stdout, _("\
Encode DATA and write Data Matrix barcode to desired format\n\
\n\
Example: %s -o message.png message.txt\n\
Example: cat message.txt | %s -o message.png\n\
\n\
OPTIONS:\n"), programName, programName);
      fprintf(stdout, _("\
  -c, --color=COLOR          barcode color\n\
  -b, --bg-color=COLOR       background color\n\
  -d, --module=NUM           module size (in pixels)\n\
  -m, --margin=NUM           margin size (in pixels)\n\
  -e, --encoding=[bfactxe8]  encodation scheme\n\
        b = Best optimized   best possible optimization (beta)\n\
        f = Fast optimized   basic optimization (not implemented)\n\
        a = ASCII  [default] ASCII standard & extended\n\
        c = C40              digits and uppercase\n\
        t = Text             digits and lowercase\n\
        x = X12              ANSI X12 EDI\n\
        e = EDIFACT          ASCII values 32-94\n\
        8 = Base 256         all byte values 0-255\n\
  -f, --format=[npm]         image output format\n\
        n = None             do not create barcode image\n\
        p = PNG    [default] PNG image\n\
        m = PNM              PNM image\n\
  -o, --output=FILE          path of file containing barcode image output\n\
  -p, --preview=[nac]        print a preview of the barcode data to STDOUT\n\
        n = None   [default] do not print barcode preview\n\
        a = ASCII            ASCII-art representation\n\
        c = Codewords        list data and error codewords\n\
  -r, --rotate=DEGREES       rotation angle (degrees)\n\
  -s, --symbol-size=SIZE     symbol size in Rows x Cols\n\
        Automatic SIZE options:\n\
            s = Auto square  [default]\n\
            r = Auto rectangle\n\
        Manually specified SIZE options for square symbols:\n\
            10x10,   12x12,   14x14,   16x16,   18x18,   20x20,\n\
            22x22,   24x24,   26x26,   32x32,   36x36,   40x40,\n\
            44x44,   48x48,   52x52,   64x64,   72x72,   80x80,\n\
            88x88,   96x96, 104x104, 120x120, 132x132, 144x144\n\
        Manually specified SIZE options for rectangular symbols:\n\
             8x18,    8x32,   12x26,   12x36,   16x36,   16x48\n\
  -v, --verbose              use verbose messages\n\
  -M, --mosaic               create non-standard Data Mosaic barcode\n\
  -R, --resolution=NUM       set physical resolution of image (dpi)\n\
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
static void
WriteImagePng(DmtxEncode *encode, char *path)
{
   FILE *fp;
   int row;
   png_structp pngPtr;
   png_infop infoPtr;
   png_bytepp rowPointers;

   fp = fopen(path, "wb");
   if(fp == NULL) {
      perror(programName);
      exit(3);
   }

   /* Create and initialize the png_struct with the desired error handler functions */
   pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if(pngPtr == NULL) {
      fclose(fp);
      perror(programName);
   }

   /* Create and initialize image information struct */
   infoPtr = png_create_info_struct(pngPtr);
   if(infoPtr == NULL) {
      fclose(fp);
      png_destroy_write_struct(&pngPtr,  png_infopp_NULL);
      perror(programName);
   }

   /* Set error handling */
   if(setjmp(png_jmpbuf(pngPtr))) {
      fclose(fp);
      png_destroy_write_struct(&pngPtr, &infoPtr);
      perror(programName);
   }

   /* Set up output control using standard streams */
   png_init_io(pngPtr, fp);

   png_set_IHDR(pngPtr, infoPtr, encode->image.width, encode->image.height,
         8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
         PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

   rowPointers = (png_bytepp)png_malloc(pngPtr, sizeof(png_bytep) * encode->image.height);
   if(rowPointers == NULL) {
      perror(programName);
   }

   // This copy reverses row order top-to-bottom so image coordinate system
   // corresponds with normal "right-handed" 2D space
   for(row = 0; row < encode->image.height; row++) {
      rowPointers[row] = (png_bytep)png_malloc(pngPtr, png_get_rowbytes(pngPtr, infoPtr));

      assert(png_get_rowbytes(pngPtr, infoPtr) == encode->image.width * sizeof(DmtxPixel));

      // Flip rows top-to-bottom to account for PNM "top-left" origin
      memcpy(rowPointers[row],
            encode->image.pxl + ((encode->image.height - row - 1) * encode->image.width),
            encode->image.width * sizeof(DmtxPixel));
   }

   png_set_rows(pngPtr, infoPtr, rowPointers);

   png_write_png(pngPtr, infoPtr, PNG_TRANSFORM_PACKING, NULL);

   /* Clean up after the write, and free any memory allocated */
   png_destroy_write_struct(&pngPtr, &infoPtr);

   for(row = 0; row < encode->image.height; row++) {
      png_free(pngPtr, rowPointers[row]);
   }
   png_free(pngPtr, rowPointers);
   rowPointers = NULL;

   fclose(fp);
}

/**
 *
 */
static void
WriteImagePnm(DmtxEncode *encode, char *path)
{
   int row, col;
   FILE *output;

   // Flip rows top-to-bottom to account for PNM "top-left" origin
   output = fopen(path, "wb");
   if(output == NULL) {
      perror(programName);
      exit(3);
   }

   fprintf(output, "P6 %d %d 255 ", encode->image.width, encode->image.height);
   for(row = 0; row < encode->image.height; row++) {
      for(col = 0; col < encode->image.width; col++) {
         fputc(encode->image.pxl[(encode->image.height - row - 1) * encode->image.width + col].R, output);
         fputc(encode->image.pxl[(encode->image.height - row - 1) * encode->image.width + col].G, output);
         fputc(encode->image.pxl[(encode->image.height - row - 1) * encode->image.width + col].B, output);
      }
   }
   fclose(output);
}

/**
 *
 */
static void
WriteAsciiBarcode(DmtxEncode *encode)
{
   int symbolRow, symbolCol;
   int moduleOnAll;

   moduleOnAll = DMTX_MODULE_ON_RED | DMTX_MODULE_ON_GREEN | DMTX_MODULE_ON_BLUE;

   fputc('\n', stdout);

   // ASCII prints from top to bottom
   for(symbolRow = encode->matrix.symbolRows - 1; symbolRow >= 0; symbolRow--) {

      fputs("    ", stdout);
      for(symbolCol = 0; symbolCol < encode->matrix.symbolCols; symbolCol++) {
         fputs((dmtxSymbolModuleStatus(&(encode->matrix), symbolRow, symbolCol) &
               moduleOnAll) ? "XX" : "  ", stdout);
      }
      fputs("\n", stdout);
   }

   fputc('\n', stdout);
}

/**
 *
 */
static void
WriteCodewords(DmtxEncode *encode)
{
   int i, dataWordLength;

   dataWordLength = dmtxGetSymbolAttribute(DmtxSymAttribDataWordLength, encode->matrix.sizeIdx);

   for(i = 0; i < encode->matrix.codeSize; i++) {
      fprintf(stdout, "%c:%03d\n", (i < dataWordLength) ?
            'd' : 'e', encode->matrix.code[i]);
   }
}
