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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <png.h>
#include <dmtx.h>
#include "dmtxwrite.h"
#include "../common/dmtxutil.h"

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
   UserOptions opt;
   DmtxEncode enc;
   unsigned char codeBuffer[DMTXWRITE_BUFFER_SIZE];
   int codeBufferSize = sizeof codeBuffer;

   InitUserOptions(&opt);

   /* Create and initialize libdmtx structures */
   enc = dmtxEncodeStructInit();

   /* Process user options */
   err = HandleArgs(&opt, &argc, &argv, &enc);
   if(err != DMTX_SUCCESS)
      ShowUsage(err);

   /* Read input data into buffer */
   ReadData(&opt, &codeBufferSize, codeBuffer);

   /* Create barcode image */
   if(opt.mosaic)
      err = dmtxEncodeDataMosaic(&enc, codeBufferSize, codeBuffer, opt.sizeIdx);
   else
      err = dmtxEncodeDataMatrix(&enc, codeBufferSize, codeBuffer, opt.sizeIdx);

   if(err == DMTX_FAILURE)
      FatalError(1, _("Unable to encode message (possibly too large for requested size)"));

   /* Write barcode image to requested format */
   switch(opt.format) {
      case 'n':
         break; /* do nothing */
      case 'p':
         WriteImagePng(&opt, &enc);
         break;
      case 'm':
         WriteImagePnm(&opt, &enc);
         break;
   }

   /* Write barcode preview if requested */
   switch(opt.preview) {
      case 'n':
         break; /* do nothing */
      case 'a':
         WriteAsciiBarcode(&enc);
         break;
      case 'c':
         WriteCodewords(&enc);
         break;
   }

   /* Clean up */
   dmtxEncodeStructDeInit(&enc);

   exit(0);
}

/**
 *
 *
 */
static void
InitUserOptions(UserOptions *opt)
{
   memset(opt, 0x00, sizeof(UserOptions));

/* opt->color = ""; */
/* opt->bgColor = ""; */
   opt->format = 'p';
   opt->inputPath = NULL;    /* default stdin */
   opt->outputPath = NULL;   /* default stdout */
   opt->preview = 'n';
   opt->rotate = 0;
   opt->sizeIdx = DMTX_SYMBOL_SQUARE_AUTO;
   opt->verbose = 0;
   opt->mosaic = 0;
   opt->dpi = 0; /* default to native resolution of requested image format */
}

/**
 * Sets and validates user-requested options from command line arguments.
 *
 * @param options runtime options from defaults or command line
 * @param argcp   pointer to argument count
 * @param argvp   pointer to argument list
 * @return        DMTX_SUCCESS | DMTX_FAILURE
 */
static int
HandleArgs(UserOptions *opt, int *argcp, char **argvp[], DmtxEncode *enc)
{
   int err;
   int i;
   int optchr;
   int longIndex;
   char *ptr;

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
         {"resolution",  required_argument, NULL, 'R'},
         {"version",     no_argument,       NULL, 'V'},
         {"help",        no_argument,       NULL,  0 },
         {0, 0, 0, 0}
   };

   programName = Basename((*argvp)[0]);

   /* Set default values before considering arguments */
   enc->moduleSize = 5;
   enc->marginSize = 10;
   enc->scheme = DmtxSchemeEncodeAscii;

   for(;;) {
      optchr = getopt_long(*argcp, *argvp, "c:b:d:m:e:f:o:p:r:s:vMR:V", longOptions, &longIndex);
      if(optchr == -1)
         break;

      switch(optchr) {
         case 0: /* --help */
            ShowUsage(0);
            break;
         case 'c':
            opt->color[0] = 0;
            opt->color[1] = 0;
            opt->color[2] = 0;
            fprintf(stdout, "Option \"%c\" not implemented yet\n", optchr);
            break;
         case 'b':
            opt->bgColor[0] = 255;
            opt->bgColor[1] = 255;
            opt->bgColor[2] = 255;
            fprintf(stdout, "Option \"%c\" not implemented yet\n", optchr);
            break;
         case 'd':
            err = StringToInt(&(enc->moduleSize), optarg, &ptr);
            if(err != DMTX_SUCCESS || enc->moduleSize <= 0 || *ptr != '\0')
               FatalError(1, _("Invalid module size specified \"%s\""), optarg);
            break;
         case 'm':
            err = StringToInt(&(enc->marginSize), optarg, &ptr);
            if(err != DMTX_SUCCESS || enc->marginSize <= 0 || *ptr != '\0')
               FatalError(1, _("Invalid margin size specified \"%s\""), optarg);
            break;
         case 'e':
            if(strlen(optarg) != 1) {
               fprintf(stdout, "Invalid encodation scheme \"%s\"\n", optarg);
               return DMTX_FAILURE;
            }
            switch(*optarg) {
               case 'b':
                  enc->scheme = DmtxSchemeEncodeAutoBest;
                  break;
               case 'f':
                  enc->scheme = DmtxSchemeEncodeAutoFast;
                  fprintf(stdout, "\"Fast optimized\" not implemented yet\n");
                  return DMTX_FAILURE;
                  break;
               case 'a':
                  enc->scheme = DmtxSchemeEncodeAscii;
                  break;
               case 'c':
                  enc->scheme = DmtxSchemeEncodeC40;
                  break;
               case 't':
                  enc->scheme = DmtxSchemeEncodeText;
                  break;
               case 'x':
                  enc->scheme = DmtxSchemeEncodeX12;
                  break;
               case 'e':
                  enc->scheme = DmtxSchemeEncodeEdifact;
                  break;
               case '8':
                  enc->scheme = DmtxSchemeEncodeBase256;
                  break;
               default:
                  fprintf(stdout, "Invalid encodation scheme \"%s\"\n", optarg);
                  return DMTX_FAILURE;
                  break;
            }
            break;
         case 'f':
            opt->format = *optarg;

            if(opt->format != 'n' && opt->format != 'p' &&
                  opt->format != 'm') {
               fprintf(stdout, "Invalid output format \"%c\"\n", opt->format);
               return DMTX_FAILURE;
            }

            break;
         case 'o':
            opt->outputPath = optarg;
            break;
         case 'p':
            opt->preview = *optarg;

            if(opt->preview != 'n' && opt->preview != 'a' &&
                  opt->preview != 'c') {
               fprintf(stdout, "Invalid preview format \"%c\"\n", opt->preview);
               return DMTX_FAILURE;
            }

            break;
         case 'r':
            err = StringToInt(&(opt->rotate), optarg, &ptr);
            if(err != DMTX_SUCCESS || *ptr != '\0')
               FatalError(1, _("Invalid rotation angle specified \"%s\""), optarg);
            break;
         case 's':
            /* Determine correct barcode size and/or shape */
            if(*optarg == 's') {
               opt->sizeIdx = DMTX_SYMBOL_SQUARE_AUTO;
            }
            else if(*optarg == 'r') {
               opt->sizeIdx = DMTX_SYMBOL_RECT_AUTO;
            }
            else {
               for(i = 0; i < DMTX_SYMBOL_SQUARE_COUNT + DMTX_SYMBOL_RECT_COUNT; i++) {
                  if(strncmp(optarg, symbolSizes[i], 8) == 0) {
                     opt->sizeIdx = i;
                     break;
                  }
               }
               if(i == DMTX_SYMBOL_SQUARE_COUNT + DMTX_SYMBOL_RECT_COUNT)
                  return DMTX_FAILURE;
            }

            break;
         case 'v':
            opt->verbose = 1;
            break;
         case 'M':
            opt->mosaic = 1;
            break;
         case 'R':
            err = StringToInt(&(opt->dpi), optarg, &ptr);
            if(err != DMTX_SUCCESS || opt->dpi <= 0 || *ptr != '\0')
               FatalError(1, _("Invalid dpi specified \"%s\""), optarg);
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

   opt->inputPath = (*argvp)[optind];

   /* XXX here test for incompatibility between options. For example you
      cannot specify dpi if PNM output is requested */

   return DMTX_SUCCESS;
}

/**
 *
 */
static void
ReadData(UserOptions *opt, int *codeBufferSize, unsigned char *codeBuffer)
{
   int fd;

   /* Open file or stdin for reading */
   fd = (opt->inputPath == NULL) ? 0 : open(opt->inputPath, O_RDONLY);
   if(fd == -1)
      FatalError(1, _("Error while opening file \"%s\""), opt->inputPath);

   /* Read input contents into buffer */
   *codeBufferSize = read(fd, codeBuffer, DMTXWRITE_BUFFER_SIZE);
   if(*codeBufferSize == DMTXWRITE_BUFFER_SIZE)
      FatalError(1, _("Message to be encoded is too large"));

   /* Close file only if not stdin */
   if(fd != 0 && close(fd) != 0)
      FatalError(1, _("Error while closing file"));
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
      fprintf(stderr, _("Usage: %s [OPTION]... [FILE]\n"), programName);
      fprintf(stderr, _("Try `%s --help' for more information.\n"), programName);
   }
   else {
      fprintf(stdout, _("Usage: %s [OPTION]... [FILE]\n"), programName);
      fprintf(stdout, _("\
Encode FILE or STDIN and write Data Matrix barcode to desired format\n\
\n\
Example: %s message.txt -o message.png\n\
Example: echo -n 123456 | %s -o message.png\n\
\n\
OPTIONS:\n"), programName, programName);
      fprintf(stdout, _("\
  -c, --color=COLOR          barcode color (not implemented)\n\
  -b, --bg-color=COLOR       background color (not implemented)\n\
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
  -o, --output=FILE          output filename, or \"-\" for STDOUT\n\
  -p, --preview=[ac]         print preview of barcode data to STDOUT\n\
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
  -M, --mosaic               create Data Mosaic barcode (non-standard)\n\
  -R, --resolution=NUM       set image print resolution (dpi)\n\
  -v, --verbose              use verbose messages\n\
  -V, --version              print version information\n\
      --help                 display this help and exit\n"));
      fprintf(stdout, _("\nReport bugs to <mike@dragonflylogic.com>.\n"));
   }

   exit(status);
}

/**
 *
 */
static void
WriteImagePng(UserOptions *opt, DmtxEncode *enc)
{
   FILE *fp;
   int row;
   int width, height;
   png_structp pngPtr;
   png_infop infoPtr;
   png_bytepp rowPointers;
   png_uint_32 pixelsPerMeter;

   /* Open file or stdin for writing */
   fp = (opt->outputPath == NULL) ? stdout : fopen(opt->outputPath, "wb");
   if(fp == NULL) {
      perror(programName);
      exit(3);
   }

   /* Create and initialize the png_struct with the desired error handler functions */
   pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if(pngPtr == NULL) {
      if(fp != stdout) {
         fclose(fp);
      }
      perror(programName);
   }

   /* Create and initialize image information struct */
   infoPtr = png_create_info_struct(pngPtr);
   if(infoPtr == NULL) {
      if(fp != stdout) {
         fclose(fp);
      }
      png_destroy_write_struct(&pngPtr,  png_infopp_NULL);
      perror(programName);
   }

   /* Set error handling */
   if(setjmp(png_jmpbuf(pngPtr))) {
      if(fp != stdout) {
         fclose(fp);
      }
      png_destroy_write_struct(&pngPtr, &infoPtr);
      perror(programName);
   }

   width = dmtxImageGetProp(enc->image, DmtxPropWidth);
   height = dmtxImageGetProp(enc->image, DmtxPropHeight);

   /* Set up output control using standard streams */
   png_init_io(pngPtr, fp);

   png_set_IHDR(pngPtr, infoPtr, width, height,
         8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
         PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

   if(opt->dpi > 0) {
      pixelsPerMeter = (png_uint_32)(39.3700787 * opt->dpi + 0.5);
      png_set_pHYs(pngPtr, infoPtr, pixelsPerMeter, pixelsPerMeter, PNG_RESOLUTION_METER);
   }

   rowPointers = (png_bytepp)png_malloc(pngPtr, sizeof(png_bytep) * height);
   if(rowPointers == NULL) {
      perror(programName);
   }

   /* This copy reverses row order top-to-bottom so image coordinate system
      corresponds with normal "right-handed" 2D space */
   for(row = 0; row < height; row++) {
      rowPointers[row] = (png_bytep)png_malloc(pngPtr, png_get_rowbytes(pngPtr, infoPtr));

      assert(png_get_rowbytes(pngPtr, infoPtr) == width * sizeof(DmtxRgb));

      /* Flip rows top-to-bottom to account for PNM "top-left" origin */
      memcpy(rowPointers[row], enc->image->pxl + (row * width), width * sizeof(DmtxRgb));
   }

   png_set_rows(pngPtr, infoPtr, rowPointers);

   png_write_png(pngPtr, infoPtr, PNG_TRANSFORM_PACKING, NULL);

   /* Clean up after the write, and free any memory allocated */
   png_destroy_write_struct(&pngPtr, &infoPtr);

   for(row = 0; row < height; row++) {
      png_free(pngPtr, rowPointers[row]);
   }
   png_free(pngPtr, rowPointers);
   rowPointers = NULL;

   if(fp != stdout) {
      fclose(fp);
   }
}

/**
 *
 */
static void
WriteImagePnm(UserOptions *opt, DmtxEncode *enc)
{
   int row, col;
   int width, height;
   FILE *fp;
   DmtxRgb rgb;

   fp = (opt->outputPath == NULL) ? stdout : fopen(opt->outputPath, "wb");
   if(fp == NULL) {
      perror(programName);
      exit(3);
   }

   width = dmtxImageGetProp(enc->image, DmtxPropWidth);
   height = dmtxImageGetProp(enc->image, DmtxPropHeight);

   /* Flip rows top-to-bottom to account for PNM "top-left" origin */
   fprintf(fp, "P6 %d %d 255 ", width, height);
   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {
         dmtxImageGetRgb(enc->image, col, row, rgb);
         fwrite(rgb, sizeof(char), 3, fp);
      }
   }

   if(fp != stdout) {
      fclose(fp);
   }
}

/**
 *
 */
static void
WriteAsciiBarcode(DmtxEncode *enc)
{
   int symbolRow, symbolCol;
   int moduleOnAll;

   moduleOnAll = DMTX_MODULE_ON_RED | DMTX_MODULE_ON_GREEN | DMTX_MODULE_ON_BLUE;

   fputc('\n', stdout);

   /* ASCII prints from top to bottom */
   for(symbolRow = enc->region.symbolRows - 1; symbolRow >= 0; symbolRow--) {

      fputs("    ", stdout);
      for(symbolCol = 0; symbolCol < enc->region.symbolCols; symbolCol++) {
         fputs((dmtxSymbolModuleStatus(enc->message, enc->region.sizeIdx, symbolRow, symbolCol) &
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
WriteCodewords(DmtxEncode *enc)
{
   int i, dataWordLength;

   dataWordLength = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, enc->region.sizeIdx);

   for(i = 0; i < enc->message->codeSize; i++) {
      fprintf(stdout, "%c:%03d\n", (i < dataWordLength) ?
            'd' : 'e', enc->message->code[i]);
   }
}
