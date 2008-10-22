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
   UserOptions opt;
   DmtxTime  msec, *timeout;
   DmtxImage *img;
   DmtxDecode decode;
   DmtxRegion region;
   DmtxMessage *message;
   ImageReader reader;
   int nFiles;

   SetOptionDefaults(&opt);
   InitializeMagick(*argv);

   err = HandleArgs(&opt, &fileIndex, &argc, &argv);
   if(err != DMTX_SUCCESS)
      ShowUsage(err);

   timeout = (opt.timeoutMS == -1) ? NULL : &msec;

   reader.image = NULL;
   nFiles = argc - fileIndex;

   /* Loop once for each page of each image listed in parameters */
   for(pageIndex = 0; fileIndex < argc || (nFiles == 0 && fileIndex == argc);) {

      /* Reset timeout for each new image */
      if(timeout != NULL)
         msec = dmtxTimeAdd(dmtxTimeNow(), opt.timeoutMS);

      /* Open image file/stream */
      if (!reader.image) {
         if (argc == fileIndex)
            OpenImage(&reader, "-", opt.resolution);
         else
            OpenImage(&reader, argv[fileIndex], opt.resolution);

         if (!reader.image) {
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
      decode = dmtxDecodeStructInit(img);

      err = dmtxDecodeSetProp(&decode, DmtxPropShrinkMin, opt.shrinkMin);
      assert(err == DMTX_SUCCESS);

      err = dmtxDecodeSetProp(&decode, DmtxPropShrinkMax, opt.shrinkMax);
      assert(err == DMTX_SUCCESS);

      err = dmtxDecodeSetProp(&decode, DmtxPropEdgeThresh, opt.edgeThresh);
      assert(err == DMTX_SUCCESS);

      err = dmtxDecodeSetProp(&decode, DmtxPropScanGap, opt.scanGap);
      assert(err == DMTX_SUCCESS);

      if(opt.squareDevn != -1) {
         err = dmtxDecodeSetProp(&decode, DmtxPropSquareDevn, opt.squareDevn);
         assert(err == DMTX_SUCCESS);
      }

      if(opt.xMin) {
         err = dmtxDecodeSetProp(&decode, DmtxPropXmin, ScaleNumberString(opt.xMin, imgWidth));
         assert(err == DMTX_SUCCESS);
      }

      if(opt.xMax) {
         err = dmtxDecodeSetProp(&decode, DmtxPropXmax, ScaleNumberString(opt.xMax, imgWidth));
         assert(err == DMTX_SUCCESS);
      }

      if(opt.yMin) {
         err = dmtxDecodeSetProp(&decode, DmtxPropYmin, ScaleNumberString(opt.yMin, imgHeight));
         assert(err == DMTX_SUCCESS);
      }

      if(opt.yMax) {
         err = dmtxDecodeSetProp(&decode, DmtxPropYmax, ScaleNumberString(opt.yMax, imgHeight));
         assert(err == DMTX_SUCCESS);
      }

      /* Loop once for each detected barcode region */
      for(;;) {

         /* Find next barcode region within image, but do not decode yet */
         region = dmtxDecodeFindNextRegion(&decode, timeout);

         /* Finished file or ran out of time before finding another region */
         if(region.found != DMTX_REGION_FOUND)
            break;

         if(opt.diagnose)
            WriteDiagnosticImage(&decode, &region, "debug.pnm");

         /* Decode region based on requested barcode mode */
         if(opt.mosaic)
            message = dmtxDecodeMosaicRegion(img, &region, opt.correctionsMax);
         else
            message = dmtxDecodeMatrixRegion(img, &region, opt.correctionsMax);

         if(message == NULL)
            continue;

         PrintDecodedOutput(&opt, img, &region, message, pageIndex);

         dmtxMessageFree(&message);
         break; /* XXX for now, break after first barcode is found in image */
      }

      dmtxDecodeStructDeInit(&decode);
      dmtxImageFree(&img);
   }

   DestroyMagick();

   exit(0);
}

/**
 *
 *
 */
static void
SetOptionDefaults(UserOptions *opt)
{
   memset(opt, 0x00, sizeof(UserOptions));

   /* Set default options */
   opt->codewords = 0;
   opt->squareDevn = -1;
   opt->scanGap = 2;
   opt->timeoutMS = -1;
   opt->newline = 0;
   opt->shrinkMin = 1;
   opt->shrinkMax = 1;
   opt->edgeThresh = 5;
   opt->xMin = NULL;
   opt->xMax = NULL;
   opt->yMin = NULL;
   opt->yMax = NULL;
   opt->correctionsMax = -1;
   opt->diagnose = 0;
   opt->mosaic = 0;
   opt->pageNumber = 0;
   opt->corners = 0;
   opt->verbose = 0;
   opt->resolution = NULL;
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
   int err;
   int optchr;
   int longIndex;
   char *ptr;

   struct option longOptions[] = {
         {"codewords",        no_argument,       NULL, 'c'},
         {"gap",              required_argument, NULL, 'g'},
         {"list-formats",     no_argument,       NULL, 'l'},
         {"milliseconds",     required_argument, NULL, 'm'},
         {"newline",          no_argument,       NULL, 'n'},
         {"square-deviation", required_argument, NULL, 'q'},
         {"resolution",       required_argument, NULL, 'r'},
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
      optchr = getopt_long(*argcp, *argvp, "cg:lm:nq:r:s:t:x:X:y:Y:vC:DMPRV", longOptions, &longIndex);
      if(optchr == -1)
         break;

      switch(optchr) {
         case 0: /* --help */
            ShowUsage(0);
            break;
         case 'l': /* --help */
            ListImageFormats();
            break;
         case 'c':
            opt->codewords = 1;
            break;
         case 'g':
            err = StringToInt(&(opt->scanGap), optarg, &ptr);
            if(err != DMTX_SUCCESS || opt->scanGap <= 0 || *ptr != '\0')
               FatalError(1, _("Invalid gap specified \"%s\""), optarg);
            break;
         case 'm':
            err = StringToInt(&(opt->timeoutMS), optarg, &ptr);
            if(err != DMTX_SUCCESS || opt->timeoutMS < 0 || *ptr != '\0')
               FatalError(1, _("Invalid timeout (in milliseconds) specified \"%s\""), optarg);
            break;
         case 'n':
            opt->newline = 1;
            break;
         case 'q':
            err = StringToInt(&(opt->squareDevn), optarg, &ptr);
            if(err != DMTX_SUCCESS || *ptr != '\0' ||
                  opt->squareDevn < 0 || opt->squareDevn > 90)
               FatalError(1, _("Invalid squareness deviation specified \"%s\""), optarg);
            break;
         case 'r':
            opt->resolution = optarg;
            break;
         case 's':
            err = StringToInt(&(opt->shrinkMax), optarg, &ptr);
            if(err != DMTX_SUCCESS || opt->shrinkMax < 1 || *ptr != '\0')
               FatalError(1, _("Invalid shrink factor specified \"%s\""), optarg);
            /* XXX later also popular shrinkMin based on N-N range */
            break;
         case 't':
            err = StringToInt(&(opt->edgeThresh), optarg, &ptr);
            if(err != DMTX_SUCCESS || *ptr != '\0' ||
                  opt->edgeThresh < 1 || opt->edgeThresh > 100)
               FatalError(1, _("Invalid edge threshold specified \"%s\""), optarg);
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
               FatalError(1, _("Invalid max corrections specified \"%s\""), optarg);
            break;
         case 'D':
            opt->diagnose = 1;
            break;
         case 'M':
            opt->mosaic = 1;
            break;
         case 'P':
            opt->pageNumber = 1;
            break;
         case 'R':
            opt->corners = 1;
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
/*to STDOUT.  Note that %s may find multiple barcodes in one image.\n\*/
      fprintf(stdout, _("\
Scan image FILE for Data Matrix barcodes and print decoded results to\n\
STDOUT.  Note: %s currently stops scanning after it decodes the\n\
first barcode in an image.\n\
\n\
Example: Scan top third of images using gap no larger than 10 pixels\n\
\n\
   %s -Y33%% -g10 IMAGE001.png IMAGE002.jpg\n\
\n\
OPTIONS:\n"), programName, programName);
      fprintf(stdout, _("\
  -c, --codewords            print codewords extracted from barcode pattern\n\
  -g, --gap=NUM              use scan grid with gap of NUM pixels between lines\n\
  -l, --list-formats         list supported input image formats\n\
  -m, --milliseconds=N       stop scan after N milliseconds (per image)\n\
  -n, --newline              print newline character at the end of decoded data\n\
  -q, --square-deviation=N   allowed non-squareness of corners in degrees (0-90)\n\
  -r, --resolution=N         decoding resolution for vectorial images (PDF, ..)\n\
  -s, --shrink=N             internally shrink image by a factor of N\n"));
      fprintf(stdout, _("\
  -t, --threshold=N          ignore weak edges below threshold N (1-100)\n\
  -x, --x-range-min=N[%%]     do not scan pixels to the left of N (or N%%)\n\
  -X, --x-range-max=N[%%]     do not scan pixels to the right of N (or N%%)\n\
  -y, --y-range-min=N[%%]     do not scan pixels above N (or N%%)\n\
  -Y, --y-range-max=N[%%]     do not scan pixels below N (or N%%)\n\
  -C, --corrections-max=N    correct at most N errors (0 = correction disabled)\n"));
      fprintf(stdout, _("\
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
 * @brief  List supported input image formats on stdout
 * @return void
 */
static void
ListImageFormats(void)
{
   MagickInfo **formats;
   int i;
   ExceptionInfo exception;

   GetExceptionInfo(&exception);
   formats=GetMagickInfoArray(&exception);
   CatchException(&exception);
   if (formats) {
      printf("   Format  Description\n");
      printf("--------------------------------------------------------"
             "-----------------------\n");
      for (i=0; formats[i] != 0; i++) {
         if (formats[i]->stealth || !formats[i]->decoder)
           continue;

         printf("%9s",formats[i]->name ? formats[i]->name : "");
         if (formats[i]->description != (char *) NULL)
            printf("  %s\n",formats[i]->description);
      }
      free(formats);
   }

   DestroyExceptionInfo(&exception);
   exit(0);
}

/**
 * @brief  Open an image input file
 * @param  reader     pointer to ImageReader struct
 * @param  imagePath  image path or "-" for stdin
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
OpenImage(ImageReader *reader, char *imagePath, char *resolution)
{
   reader->image = NULL;
   GetExceptionInfo(&reader->exception);
   reader->info = CloneImageInfo((ImageInfo *) NULL);

   strcpy((reader->info)->filename, imagePath);
   if (resolution) {
      reader->info->density = strdup(resolution);
      reader->info->units = PixelsPerInchResolution;
   }
   reader->image = ReadImage(reader->info, &reader->exception);

   switch (reader->exception.severity) {
      case UndefinedException:
         break;
      case OptionError:
      case MissingDelegateError:
         fprintf(stderr,
                 "%s: unsupported file format. Use -l to see the available ones.\n",
                 imagePath);
         break;
      default:
         CatchException(&reader->exception);
   }

   if (!reader->image)
      CloseImage(reader);

   return reader->image ? DMTX_SUCCESS : DMTX_FAILURE;
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
   DmtxImage    *image;
   unsigned int dispatchResult;
   int          pageCount;

   image = NULL;
   if (reader->image) {
      pageCount = GetImageListLength(reader->image);
      if (index < pageCount)
      {
         reader->image = GetImageFromList(reader->image, index);

         image = dmtxImageMalloc(reader->image->columns, reader->image->rows);

         if(image) {
            image->pageCount = pageCount;
            dispatchResult = DispatchImage(reader->image, 0, 0, reader->image->columns,
                                           reader->image->rows, "RGB", CharPixel,
                                           image->pxl, &reader->exception);
            if (dispatchResult == MagickFail) {
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
   if (reader->image) {
      DestroyImage(reader->image);
      reader->image = NULL;
   }

   DestroyImageInfo(reader->info);
   DestroyExceptionInfo(&reader->exception);
}

/**
 * @brief  XXX
 * @param  opt runtime options from defaults or command line
 * @param  decode pointer to DmtxDecode struct
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
PrintDecodedOutput(UserOptions *opt, DmtxImage *image,
      DmtxRegion *region, DmtxMessage *message, int pageIndex)
{
   int i;
   int height;
   int dataWordLength;
   int rotateInt;
   double rotate;

   dataWordLength = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, region->sizeIdx);
   if(opt->verbose) {

      height = dmtxImageGetProp(image, DmtxPropHeight);

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
/*
      sorry -- temporarily need to disable this
      fprintf(stdout, "          Corner 0: (%0.1f, %0.1f)\n",
            region->corners.c00.X, height - 1 - region->corners.c00.Y);
      fprintf(stdout, "          Corner 1: (%0.1f, %0.1f)\n",
            region->corners.c10.X, height - 1 - region->corners.c10.Y);
      fprintf(stdout, "          Corner 2: (%0.1f, %0.1f)\n",
            region->corners.c11.X, height - 1 - region->corners.c11.Y);
      fprintf(stdout, "          Corner 3: (%0.1f, %0.1f)\n",
            region->corners.c01.X, height - 1 - region->corners.c01.Y);
*/
      fprintf(stdout, "--------------------------------------------------\n");
   }

   if(opt->pageNumber)
      fprintf(stdout, "%d:", pageIndex + 1);

   if(opt->corners) {
/*
      sorry -- temporarily need to disable this
      fprintf(stdout, "%d,%d:", (int)(region->corners.c00.X + 0.5),
            height - 1 - (int)(region->corners.c00.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c10.X + 0.5),
            height - 1 - (int)(region->corners.c10.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c11.X + 0.5),
            height - 1 - (int)(region->corners.c11.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c01.X + 0.5),
            height - 1 - (int)(region->corners.c01.Y + 0.5));
      fprintf(stdout, "%d,%d:", (int)(region->corners.c00.X + 0.5),
            height - 1 - (int)(region->corners.c00.Y + 0.5));
*/
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
