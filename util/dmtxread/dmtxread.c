/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (c) 2008 Mike Laughton
Copyright (c) 2008 Ryan Raasch
Copyright (c) 2008 Olivier Guilyardi

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
#include <sysexits.h>
#include <magick/api.h>
#include <dmtx.h>
#include "dmtxread.h"
#include "../common/dmtxutil.h"

char *programName;

/**
 * @brief  Main function for the dmtxread Data Matrix scanning utility.
 * @param  argc count of arguments passed from command line
 * @param  argv list of argument passed strings from command line
 * @return Numeric exit code
 */
int
main(int argc, char *argv[])
{
   int err;
   int fileIndex;
   int pageIndex;
   int imgWidth, imgHeight;
   int fileScanCount, globalScanCount;
   int nFiles;
   UserOptions opt;
   DmtxTime  msec, *timeout;
   DmtxImage *img;
   DmtxDecode dec;
   DmtxRegion reg;
   DmtxMessage *message;
   ImageReader reader;

   SetOptionDefaults(&opt);
   InitializeMagick(*argv);

   err = HandleArgs(&opt, &fileIndex, &argc, &argv);
   if(err != DMTX_SUCCESS)
      ShowUsage(EX_USAGE);

   timeout = (opt.timeoutMS == -1) ? NULL : &msec;

   reader.image = NULL;
   nFiles = argc - fileIndex;
   globalScanCount = 0;

   /* Loop once for each page of each image listed in parameters */
   for(pageIndex = 0; fileIndex < argc || (nFiles == 0 && fileIndex == argc);) {

      /* Reset timeout for each new image */
      if(timeout != NULL)
         msec = dmtxTimeAdd(dmtxTimeNow(), opt.timeoutMS);

      /* Open image file/stream */
      if(!reader.image) {
         if(argc == fileIndex)
            OpenImage(&reader, "-", opt.resolution);
         else
            OpenImage(&reader, argv[fileIndex], opt.resolution);

         if(!reader.image) {
            fileIndex++;
            continue;
         }
      }

      /* Read next image page (many formats are single-page only) */
      img = ReadImagePage(&reader, pageIndex++);

      /* If requested page did not load then move to the next image */
      if(img == NULL) {
         CloseImage(&reader);
         fileIndex++;
         pageIndex = 0;
         continue;
      }

      imgWidth = dmtxImageGetProp(img, DmtxPropWidth);
      imgHeight = dmtxImageGetProp(img, DmtxPropHeight);

      assert(img->pageCount > 0 && pageIndex <= img->pageCount);

      /* Initialize decode struct for newly loaded image */
      dec = dmtxDecodeStructInit(img);

      err = dmtxDecodeSetProp(&dec, DmtxPropScanGap, opt.scanGap);
      assert(err == DMTX_SUCCESS);

      if(opt.edgeMin != -1) {
         err = dmtxDecodeSetProp(&dec, DmtxPropEdgeMin, opt.edgeMin);
         assert(err == DMTX_SUCCESS);
      }

      if(opt.edgeMax != -1) {
         err = dmtxDecodeSetProp(&dec, DmtxPropEdgeMax, opt.edgeMax);
         assert(err == DMTX_SUCCESS);
      }

      if(opt.squareDevn != -1) {
         err = dmtxDecodeSetProp(&dec, DmtxPropSquareDevn, opt.squareDevn);
         assert(err == DMTX_SUCCESS);
      }

      err = dmtxDecodeSetProp(&dec, DmtxPropSymbolSize, opt.sizeIdxExpected);
      assert(err == DMTX_SUCCESS);

      err = dmtxDecodeSetProp(&dec, DmtxPropEdgeThresh, opt.edgeThresh);
      assert(err == DMTX_SUCCESS);

      if(opt.xMin) {
         err = dmtxDecodeSetProp(&dec, DmtxPropXmin, ScaleNumberString(opt.xMin, imgWidth));
         assert(err == DMTX_SUCCESS);
      }

      if(opt.xMax) {
         err = dmtxDecodeSetProp(&dec, DmtxPropXmax, ScaleNumberString(opt.xMax, imgWidth));
         assert(err == DMTX_SUCCESS);
      }

      if(opt.yMin) {
         err = dmtxDecodeSetProp(&dec, DmtxPropYmin, ScaleNumberString(opt.yMin, imgHeight));
         assert(err == DMTX_SUCCESS);
      }

      if(opt.yMax) {
         err = dmtxDecodeSetProp(&dec, DmtxPropYmax, ScaleNumberString(opt.yMax, imgHeight));
         assert(err == DMTX_SUCCESS);
      }

      err = dmtxDecodeSetProp(&dec, DmtxPropShrinkMin, opt.shrinkMin);
      assert(err == DMTX_SUCCESS);

      err = dmtxDecodeSetProp(&dec, DmtxPropShrinkMax, opt.shrinkMax);
      assert(err == DMTX_SUCCESS);

      /* Loop once for each detected barcode region */
      for(fileScanCount = 0;;) {

         /* Find next barcode region within image, but do not decode yet */
         reg = dmtxDecodeFindNextRegion(&dec, timeout);

         /* Finished file or ran out of time before finding another region */
         if(reg.found != DMTX_REGION_FOUND)
            break;

         /* Decode region based on requested barcode mode */
         if(opt.mosaic)
            message = dmtxDecodeMosaicRegion(img, &reg, opt.correctionsMax);
         else
            message = dmtxDecodeMatrixRegion(img, &reg, opt.correctionsMax);

         if(message == NULL)
            continue;

         PrintDecodedOutput(&opt, img, &reg, message, pageIndex);
         fileScanCount++;

         dmtxMessageFree(&message);

         if(opt.stopAfter != -1 && fileScanCount >= opt.stopAfter)
            break;
      }
      globalScanCount += fileScanCount;

      if(opt.diagnose)
         WriteDiagnosticImage(&dec, &reg, "debug.pnm");

      dmtxDecodeStructDeInit(&dec);
      dmtxImageFree(&img);
   }

   DestroyMagick();

   exit((globalScanCount > 0) ? EX_OK : 1);
}

/**
 *
 *
 */
static void
SetOptionDefaults(UserOptions *opt)
{
   UserOptions option;

   memset(&option, 0x00, sizeof(UserOptions));

   /* Set default options */
   option.codewords = 0;
   option.edgeMin = -1;
   option.edgeMax = -1;
   option.squareDevn = -1;
   option.scanGap = 2;
   option.timeoutMS = -1;
   option.newline = 0;
   option.resolution = NULL;
   option.sizeIdxExpected = DmtxSymbolShapeAuto;
   option.edgeThresh = 5;
   option.xMin = NULL;
   option.xMax = NULL;
   option.yMin = NULL;
   option.yMax = NULL;
   option.correctionsMax = -1;
   option.diagnose = 0;
   option.mosaic = 0;
   option.stopAfter = -1;
   option.pageNumber = 0;
   option.corners = 0;
   option.shrinkMin = 1;
   option.shrinkMax = 1;
   option.verbose = 0;

   *opt = option;
}

/**
 * @brief  Set and validate user-requested options from command line arguments.
 * @param  opt runtime options from defaults or command line
 * @param  argcp pointer to argument count
 * @param  argvp pointer to argument list
 * @param  fileIndex pointer to index of first non-option arg (if successful)
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
HandleArgs(UserOptions *opt, int *fileIndex, int *argcp, char **argvp[])
{
   int i;
   int err;
   int optchr;
   int longIndex;
   char *ptr;

   struct option longOptions[] = {
         {"codewords",        no_argument,       NULL, 'c'},
         {"minimum-edge",     required_argument, NULL, 'e'},
         {"maximum-edge",     required_argument, NULL, 'E'},
         {"gap",              required_argument, NULL, 'g'},
         {"list-formats",     no_argument,       NULL, 'l'},
         {"milliseconds",     required_argument, NULL, 'm'},
         {"newline",          no_argument,       NULL, 'n'},
         {"square-deviation", required_argument, NULL, 'q'},
         {"resolution",       required_argument, NULL, 'r'},
         {"symbol-size",      required_argument, NULL, 's'},
         {"threshold",        required_argument, NULL, 't'},
         {"x-range-min",      required_argument, NULL, 'x'},
         {"x-range-max",      required_argument, NULL, 'X'},
         {"y-range-min",      required_argument, NULL, 'y'},
         {"y-range-max",      required_argument, NULL, 'Y'},
         {"max-corrections",  required_argument, NULL, 'C'},
         {"diagnose",         no_argument,       NULL, 'D'},
         {"mosaic",           no_argument,       NULL, 'M'},
         {"stop-after",       required_argument, NULL, 'N'},
         {"page-number",      no_argument,       NULL, 'P'},
         {"corners",          no_argument,       NULL, 'R'},
         {"shrink",           required_argument, NULL, 'S'},
         {"verbose",          no_argument,       NULL, 'v'},
         {"version",          no_argument,       NULL, 'V'},
         {"help",             no_argument,       NULL,  0 },
         {0, 0, 0, 0}
   };

   programName = Basename((*argvp)[0]);

   *fileIndex = 0;

   for(;;) {
      optchr = getopt_long(*argcp, *argvp, "ce:E:g:lm:nq:r:s:t:x:X:y:Y:vC:DMN:PRS:V", longOptions, &longIndex);
      if(optchr == -1)
         break;

      switch(optchr) {
         case 0: /* --help */
            ShowUsage(EX_OK);
            break;
         case 'l': /* --help */
            ListImageFormats();
            exit(EX_OK);
            break;
         case 'c':
            opt->codewords = 1;
            break;
         case 'e':
            err = StringToInt(&(opt->edgeMin), optarg, &ptr);
            if(err != DMTX_SUCCESS || opt->edgeMin <= 0 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid edge length specified \"%s\""), optarg);
            break;
         case 'E':
            err = StringToInt(&(opt->edgeMax), optarg, &ptr);
            if(err != DMTX_SUCCESS || opt->edgeMax <= 0 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid edge length specified \"%s\""), optarg);
            break;
         case 'g':
            err = StringToInt(&(opt->scanGap), optarg, &ptr);
            if(err != DMTX_SUCCESS || opt->scanGap <= 0 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid gap specified \"%s\""), optarg);
            break;
         case 'm':
            err = StringToInt(&(opt->timeoutMS), optarg, &ptr);
            if(err != DMTX_SUCCESS || opt->timeoutMS < 0 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid timeout (in milliseconds) specified \"%s\""), optarg);
            break;
         case 'n':
            opt->newline = 1;
            break;
         case 'q':
            err = StringToInt(&(opt->squareDevn), optarg, &ptr);
            if(err != DMTX_SUCCESS || *ptr != '\0' ||
                  opt->squareDevn < 0 || opt->squareDevn > 90)
               FatalError(EX_USAGE, _("Invalid squareness deviation specified \"%s\""), optarg);
            break;
         case 'r':
            opt->resolution = optarg;
            break;
         case 's':
            /* Determine correct barcode size and/or shape */
            if(*optarg == 'a') {
               opt->sizeIdxExpected = DmtxSymbolShapeAuto;
            }
            else if(*optarg == 's') {
               opt->sizeIdxExpected = DmtxSymbolSquareAuto;
            }
            else if(*optarg == 'r') {
               opt->sizeIdxExpected = DmtxSymbolRectAuto;
            }
            else {
               for(i = 0; i < DMTX_SYMBOL_SQUARE_COUNT + DMTX_SYMBOL_RECT_COUNT; i++) {
                  if(strncmp(optarg, symbolSizes[i], 8) == 0) {
                     opt->sizeIdxExpected = i;
                     break;
                  }
               }
               if(i == DMTX_SYMBOL_SQUARE_COUNT + DMTX_SYMBOL_RECT_COUNT)
                  return DMTX_FAILURE;
            }
            break;
         case 't':
            err = StringToInt(&(opt->edgeThresh), optarg, &ptr);
            if(err != DMTX_SUCCESS || *ptr != '\0' ||
                  opt->edgeThresh < 1 || opt->edgeThresh > 100)
               FatalError(EX_USAGE, _("Invalid edge threshold specified \"%s\""), optarg);
            break;
         case 'x':
            opt->xMin = optarg;
            break;
         case 'X':
            opt->xMax = optarg;
            break;
         case 'y':
            opt->yMin = optarg;
            break;
         case 'Y':
            opt->yMax = optarg;
            break;
         case 'v':
            opt->verbose = 1;
            break;
         case 'C':
            err = StringToInt(&(opt->correctionsMax), optarg, &ptr);
            if(err != DMTX_SUCCESS || opt->correctionsMax < 0 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid max corrections specified \"%s\""), optarg);
            break;
         case 'D':
            opt->diagnose = 1;
            break;
         case 'M':
            opt->mosaic = 1;
            break;
         case 'N':
            err = StringToInt(&(opt->stopAfter), optarg, &ptr);
            if(err != DMTX_SUCCESS || opt->stopAfter < 1 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid count specified \"%s\""), optarg);
            break;
         case 'P':
            opt->pageNumber = 1;
            break;
         case 'R':
            opt->corners = 1;
            break;
         case 'S':
            err = StringToInt(&(opt->shrinkMax), optarg, &ptr);
            if(err != DMTX_SUCCESS || opt->shrinkMax < 1 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid shrink factor specified \"%s\""), optarg);
            /* XXX later also popular shrinkMin based on N-N range */
            break;
         case 'V':
            fprintf(stdout, "%s version %s\n", programName, DMTX_VERSION);
            fprintf(stdout, "libdmtx version %s\n", dmtxVersion());
            exit(EX_OK);
            break;
         default:
            return DMTX_FAILURE;
            break;
      }
   }
   *fileIndex = optind;

   return DMTX_SUCCESS;
}

/**
 * @brief  Display program usage and exit with received status.
 * @param  status error code returned to OS
 * @return void
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
Scan image FILE for Data Matrix barcodes and print decoded results to\n\
STDOUT.  Note that %s may find multiple barcodes in one image.\n\
\n\
Example: Scan top third of images using gap no larger than 10 pixels\n\
\n\
   %s -Y33%% -g10 IMAGE001.png IMAGE002.jpg\n\
\n\
OPTIONS:\n"), programName, programName);
      fprintf(stdout, _("\
  -c, --codewords             print codewords extracted from barcode pattern\n\
  -e, --minimum-edge=N        pixel length of smallest expected edge in image\n\
  -E, --maximum-edge=N        pixel length of largest expected edge in image\n\
  -g, --gap=NUM               use scan grid with gap of NUM pixels between lines\n\
  -l, --list-formats          list supported input image formats\n\
  -m, --milliseconds=N        stop scan after N milliseconds (per image)\n\
  -n, --newline               print newline character at the end of decoded data\n\
  -q, --square-deviation=N    allow non-squareness of corners in degrees (0-90)\n"));
      fprintf(stdout, _("\
  -r, --resolution=N          resolution for vector images (PDF, SVG, etc...)\n\
  -s, --symbol-size=[asr|RxC] only consider barcodes of specific size or shape\n\
        a = All sizes         [default]\n\
        s = Only squares\n\
        r = Only rectangles\n\
      RxC = Exactly this many rows and columns (10x10, 8x18, etc...)\n\
  -t, --threshold=N           ignore weak edges below threshold N (1-100)\n"));
      fprintf(stdout, _("\
  -x, --x-range-min=N[%%]      do not scan pixels to the left of N (or N%%)\n\
  -X, --x-range-max=N[%%]      do not scan pixels to the right of N (or N%%)\n\
  -y, --y-range-min=N[%%]      do not scan pixels above N (or N%%)\n\
  -Y, --y-range-max=N[%%]      do not scan pixels below N (or N%%)\n\
  -C, --corrections-max=N     correct at most N errors (0 = correction disabled)\n\
  -D, --diagnose              make copy of image with additional diagnostic data\n\
  -M, --mosaic                interpret detected regions as Data Mosaic barcodes\n"));
      fprintf(stdout, _("\
  -N, --stop-after=N          stop scanning after Nth barcode is returned\n\
  -P, --page-number           prefix decoded message with fax/tiff page number\n\
  -R, --corners               prefix decoded message with corner locations\n\
  -S, --shrink=N              internally shrink image by a factor of N\n\
  -v, --verbose               use verbose messages\n\
  -V, --version               print program version information\n\
      --help                  display this help and exit\n"));
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
      FatalError(EX_USAGE, _("Integer value required"));

   scaledValue = (*terminate == '%') ? (int)(0.01 * numValue * extent + 0.5) : numValue;

   if(scaledValue < 0)
      scaledValue = 0;

   if(scaledValue >= extent)
      scaledValue = extent - 1;

   return scaledValue;
}

/**
 * @brief  List supported input image formats on stdout
 * @return void
 */
static void
ListImageFormats(void)
{
   int i;
   MagickInfo **formats;
   ExceptionInfo exception;

   GetExceptionInfo(&exception);
   formats=GetMagickInfoArray(&exception);
   CatchException(&exception);

   if(formats) {
      fprintf(stdout, "   Format  Description\n");
      fprintf(stdout, "---------  ------------------------------------------------------\n");
      for(i=0; formats[i] != 0; i++) {
         if(formats[i]->stealth || !formats[i]->decoder)
            continue;

         fprintf(stdout, "%9s", formats[i]->name ? formats[i]->name : "");
         if(formats[i]->description != (char *)NULL)
            fprintf(stdout, "  %s\n",formats[i]->description);
      }
      free(formats);
   }

   DestroyExceptionInfo(&exception);
}

/**
 * @brief  Open an image input file
 * @param  reader Pointer to ImageReader struct
 * @param  imagePath Image path or "-" for stdin
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
OpenImage(ImageReader *reader, char *imagePath, char *resolution)
{
   reader->image = NULL;
   GetExceptionInfo(&reader->exception);
   reader->info = CloneImageInfo((ImageInfo *) NULL);

   strcpy((reader->info)->filename, imagePath);
   if(resolution) {
      reader->info->density = strdup(resolution);
      reader->info->units = PixelsPerInchResolution;
   }
   reader->image = ReadImage(reader->info, &reader->exception);

   switch(reader->exception.severity) {
      case UndefinedException:
         break;
      case OptionError:
      case MissingDelegateError:
         fprintf(stderr,
               "%s: unsupported file format. Use -l to for a list of available formats.\n",
               imagePath);
         break;
      default:
         CatchException(&reader->exception);
         break;
   }

   if(!reader->image)
      CloseImage(reader);

   return (reader->image) ? DMTX_SUCCESS : DMTX_FAILURE;
}

/**
 * @brief  Read an image page
 * @param  reader pointer to ImageReader struct
 * @param  index page index
 * @return pointer to allocated DmtxImage or NULL
 */
static DmtxImage *
ReadImagePage(ImageReader *reader, int index)
{
   unsigned int dispatchResult;
   int pageCount;
   DmtxImage *image;

   image = NULL;
   if(reader->image) {
      pageCount = GetImageListLength(reader->image);
      if(index < pageCount) {
         reader->image = GetImageFromList(reader->image, index);

         image = dmtxImageMalloc(reader->image->columns, reader->image->rows);

         if(image) {
            image->pageCount = pageCount;
            dispatchResult = DispatchImage(reader->image, 0, 0, reader->image->columns,
                                           reader->image->rows, "RGB", CharPixel,
                                           image->pxl, &reader->exception);
            if(dispatchResult == MagickFail) {
               dmtxImageFree(&image);
               CatchException(&reader->exception);
            }
         }
         else {
            perror(programName);
         }
      }
   }

   return image;
}

/**
 * @brief  Close an image
 * @param  reader  pointer to ImageReader struct
 * @return void
 */
static void
CloseImage(ImageReader * reader)
{
   if(reader->image) {
      DestroyImage(reader->image);
      reader->image = NULL;
   }

   DestroyImageInfo(reader->info);
   DestroyExceptionInfo(&reader->exception);
}

/**
 * @brief  XXX
 * @param  opt runtime options from defaults or command line
 * @param  dec pointer to DmtxDecode struct
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
PrintDecodedOutput(UserOptions *opt, DmtxImage *image,
      DmtxRegion *reg, DmtxMessage *message, int pageIndex)
{
   int i;
   int height;
   int dataWordLength;
   int rotateInt;
   double rotate;
   DmtxVector2 p00, p10, p11, p01;

   height = dmtxImageGetProp(image, DmtxPropScaledHeight);

   p00.X = p00.Y = p10.Y = p01.X = 0.0;
   p10.X = p01.Y = p11.X = p11.Y = 1.0;
   dmtxMatrix3VMultiplyBy(&p00, reg->fit2raw);
   dmtxMatrix3VMultiplyBy(&p10, reg->fit2raw);
   dmtxMatrix3VMultiplyBy(&p11, reg->fit2raw);
   dmtxMatrix3VMultiplyBy(&p01, reg->fit2raw);

   dataWordLength = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, reg->sizeIdx);
   if(opt->verbose) {

      rotate = (2 * M_PI) + (atan2(reg->fit2raw[0][1], reg->fit2raw[1][1]) -
            atan2(reg->fit2raw[1][0], reg->fit2raw[0][0])) / 2.0;

      rotateInt = (int)(rotate * 180/M_PI + 0.5);
      if(rotateInt >= 360)
         rotateInt -= 360;

      fprintf(stdout, "--------------------------------------------------\n");
      fprintf(stdout, "       Matrix Size: %d x %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, reg->sizeIdx),
            dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, reg->sizeIdx));
      fprintf(stdout, "    Data Codewords: %d (capacity %d)\n",
            message->outputIdx, dataWordLength);
      fprintf(stdout, "   Error Codewords: %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords, reg->sizeIdx));
      fprintf(stdout, "      Data Regions: %d x %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, reg->sizeIdx),
            dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, reg->sizeIdx));
      fprintf(stdout, "Interleaved Blocks: %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, reg->sizeIdx));
      fprintf(stdout, "    Rotation Angle: %d\n", rotateInt);
      fprintf(stdout, "          Corner 0: (%0.1f, %0.1f)\n", p00.X, height - 1 - p00.Y);
      fprintf(stdout, "          Corner 1: (%0.1f, %0.1f)\n", p10.X, height - 1 - p10.Y);
      fprintf(stdout, "          Corner 2: (%0.1f, %0.1f)\n", p11.X, height - 1 - p11.Y);
      fprintf(stdout, "          Corner 3: (%0.1f, %0.1f)\n", p01.X, height - 1 - p01.Y);
      fprintf(stdout, "--------------------------------------------------\n");
   }

   if(opt->pageNumber)
      fprintf(stdout, "%d:", pageIndex + 1);

   if(opt->corners) {
      fprintf(stdout, "%d,%d:", (int)(p00.X + 0.5), height - 1 - (int)(p00.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(p10.X + 0.5), height - 1 - (int)(p10.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(p11.X + 0.5), height - 1 - (int)(p11.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(p01.X + 0.5), height - 1 - (int)(p01.Y + 0.5));
   }

   if(opt->codewords) {
      for(i = 0; i < message->codeSize; i++) {
         fprintf(stdout, "%c:%03d\n", (i < dataWordLength) ?
               'd' : 'e', message->code[i]);
      }
   }
   else {
      fwrite(message->output, sizeof(char), message->outputIdx, stdout);
      if(opt->newline)
         fputc('\n', stdout);
   }
   fflush(stdout);

   return DMTX_SUCCESS;
}

/**
 *
 *
 */
static void
WriteDiagnosticImage(DmtxDecode *dec, DmtxRegion *reg, char *imagePath)
{
   int row, col;
   int width, height;
   int offset;
   double shade;
   FILE *fp;
   DmtxRgb rgb;

   fp = fopen(imagePath, "wb");
   if(fp == NULL) {
      perror(programName);
      FatalError(EX_CANTCREAT, _("Unable to write image \"%s\""), imagePath);
   }

   width = dmtxImageGetProp(dec->image, DmtxPropScaledWidth);
   height = dmtxImageGetProp(dec->image, DmtxPropScaledHeight);

   /* Test each pixel of input image to see if it lies in region */
   fprintf(fp, "P6\n%d %d\n255\n", width, height);
   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {

         offset = dmtxImageGetOffset(dec->image, col, row);
         if(offset == DMTX_BAD_OFFSET) {
            rgb[0] = 0;
            rgb[1] = 0;
            rgb[2] = 128;
         }
         else {
            dmtxImageGetRgb(dec->image, col, row, rgb);

            if(dec->image->cache[offset] & 0x40) {
               rgb[0] = 255;
               rgb[1] = 0;
               rgb[2] = 0;
            }
            else {
               shade = (dec->image->cache[offset] & 0x80) ? 0.0 : 0.7;
               rgb[0] += (shade * (255 - rgb[0]));
               rgb[1] += (shade * (255 - rgb[1]));
               rgb[2] += (shade * (255 - rgb[2]));
            }
         }
         fwrite(rgb, sizeof(char), 3, fp);
      }
   }

   fclose(fp);
}
