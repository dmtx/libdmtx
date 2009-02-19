/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2008, 2009 Mike Laughton

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

#include "dmtxwrite.h"

char *programName;

/**
 * @brief  Main function for the dmtxwrite Data Matrix scanning utility.
 * @param  argc count of arguments passed from command line
 * @param  argv list of argument passed strings from command line
 * @return Numeric exit code
 */
int
main(int argc, char *argv[])
{
   int err;
   char *format;
   UserOptions opt;
   DmtxEncode *enc;
   unsigned char codeBuffer[DMTXWRITE_BUFFER_SIZE];
   int codeBufferSize = sizeof codeBuffer;

   opt = GetDefaultOptions();

   /* Override defaults with requested options */
   err = HandleArgs(&opt, &argc, &argv);
   if(err != DmtxPass)
      ShowUsage(EX_USAGE);

   /* Create and initialize libdmtx encoding struct */
   enc = dmtxEncodeCreate();
   if(enc == NULL)
      FatalError(EX_SOFTWARE, "create error");

   /* Set output image properties */
   dmtxEncodeSetProp(enc, DmtxPropPixelPacking, DmtxPack24bppRGB);
   dmtxEncodeSetProp(enc, DmtxPropImageFlip, DmtxFlipNone);
   dmtxEncodeSetProp(enc, DmtxPropRowPadBytes, 0);

   /* Set encoding options */
   dmtxEncodeSetProp(enc, DmtxPropMarginSize, opt.marginSize);
   dmtxEncodeSetProp(enc, DmtxPropModuleSize, opt.moduleSize);
   dmtxEncodeSetProp(enc, DmtxPropScheme, opt.scheme);
   dmtxEncodeSetProp(enc, DmtxPropSizeRequest, opt.sizeIdx);

   /* Read input data into buffer */
   ReadInputData(&codeBufferSize, codeBuffer, &opt);

   /* Create barcode image */
   if(opt.mosaic == DmtxTrue)
      err = dmtxEncodeDataMosaic(enc, codeBufferSize, codeBuffer);
   else
      err = dmtxEncodeDataMatrix(enc, codeBufferSize, codeBuffer);

   if(err == DmtxFail)
      FatalError(EX_SOFTWARE,
            _("Unable to encode message (possibly too large for requested size)"));

   /* Write image file, but only if preview and codewords are not used */
   if(opt.preview == DmtxTrue || opt.codewords == DmtxTrue) {
      if(opt.preview == DmtxTrue)
         WriteAsciiPreview(enc);
      if(opt.codewords == DmtxTrue)
         WriteCodewordList(enc);
   }
   else {
      format = GetImageFormat(&opt);
      if(format == NULL)
         format = "png";

      if(StrNCmpI(format, "svg", 3) == DmtxTrue)
         WriteSvgFile(&opt, enc, format);
      else
         WriteImageFile(&opt, enc, format);
   }

   /* Clean up */
   dmtxEncodeDestroy(&enc);

   exit(0);
}

/**
 *
 *
 */
static UserOptions
GetDefaultOptions(void)
{
   UserOptions opt;
   int white[3] = { 255, 255, 255 };
   int black[3] = {   0,   0,   0 };

   memset(&opt, 0x00, sizeof(UserOptions));

   opt.inputPath = NULL;    /* default stdin */
   opt.outputPath = NULL;   /* default stdout */
   opt.format = NULL;
   opt.codewords = DmtxFalse;
   opt.marginSize = 10;
   opt.moduleSize = 5;
   opt.scheme = DmtxSchemeEncodeAscii;
   opt.preview = DmtxFalse;
   opt.rotate = 0;
   opt.sizeIdx = DmtxSymbolSquareAuto;
   memcpy(opt.color, black, sizeof(int) * 3);
   memcpy(opt.bgColor, white, sizeof(int) * 3);
   opt.mosaic = DmtxFalse;
   opt.dpi = 0; /* default to native resolution of requested image format */
   opt.verbose = DmtxFalse;

   return opt;
}

/**
 * @brief  Set and validate user-requested options from command line arguments.
 * @param  opt runtime options from defaults or command line
 * @param  argcp pointer to argument count
 * @param  argvp pointer to argument list
 * @return DmtxPass | DmtxFail
 */
static DmtxPassFail
HandleArgs(UserOptions *opt, int *argcp, char **argvp[])
{
   int err;
   int i;
   int optchr;
   int longIndex;
   char *ptr;

   struct option longOptions[] = {
         {"codewords",        no_argument,       NULL, 'c'},
         {"module",           required_argument, NULL, 'd'},
         {"margin",           required_argument, NULL, 'm'},
         {"encoding",         required_argument, NULL, 'e'},
         {"format",           required_argument, NULL, 'f'},
         {"list-formats",     no_argument,       NULL, 'l'},
         {"output",           required_argument, NULL, 'o'},
         {"preview",          no_argument,       NULL, 'p'},
         {"rotate",           required_argument, NULL, 'r'},
         {"symbol-size",      required_argument, NULL, 's'},
         {"color",            required_argument, NULL, 'C'},
         {"bg-color",         required_argument, NULL, 'B'},
         {"mosaic",           no_argument,       NULL, 'M'},
         {"resolution",       required_argument, NULL, 'R'},
         {"verbose",          no_argument,       NULL, 'v'},
         {"version",          no_argument,       NULL, 'V'},
         {"help",             no_argument,       NULL,  0 },
         {0, 0, 0, 0}
   };

   programName = Basename((*argvp)[0]);

   for(;;) {
      optchr = getopt_long(*argcp, *argvp, "cd:m:e:f:lo:pr:s:C:B:MR:vV", longOptions, &longIndex);
      if(optchr == -1)
         break;

      switch(optchr) {
         case 0: /* --help */
            ShowUsage(EX_OK);
            break;
         case 'c':
            opt->codewords = DmtxTrue;
            break;
         case 'd':
            err = StringToInt(&opt->moduleSize, optarg, &ptr);
            if(err != DmtxPass || opt->moduleSize <= 0 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid module size specified \"%s\""), optarg);
            break;
         case 'm':
            err = StringToInt(&opt->marginSize, optarg, &ptr);
            if(err != DmtxPass || opt->marginSize <= 0 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid margin size specified \"%s\""), optarg);
            break;
         case 'e':
            if(strlen(optarg) != 1) {
               fprintf(stdout, "Invalid encodation scheme \"%s\"\n", optarg);
               return DmtxFail;
            }
            switch(*optarg) {
               case 'b':
                  opt->scheme = DmtxSchemeEncodeAutoBest;
                  break;
               case 'f':
                  opt->scheme = DmtxSchemeEncodeAutoFast;
                  fprintf(stdout, "\"Fast optimized\" not implemented\n");
                  return DmtxFail;
               case 'a':
                  opt->scheme = DmtxSchemeEncodeAscii;
                  break;
               case 'c':
                  opt->scheme = DmtxSchemeEncodeC40;
                  break;
               case 't':
                  opt->scheme = DmtxSchemeEncodeText;
                  break;
               case 'x':
                  opt->scheme = DmtxSchemeEncodeX12;
                  break;
               case 'e':
                  opt->scheme = DmtxSchemeEncodeEdifact;
                  break;
               case '8':
                  opt->scheme = DmtxSchemeEncodeBase256;
                  break;
               default:
                  fprintf(stdout, "Invalid encodation scheme \"%s\"\n", optarg);
                  return DmtxFail;
            }
            break;
         case 'f':
            opt->format = optarg;
            break;
         case 'l':
            ListImageFormats();
            exit(EX_OK);
            break;
         case 'o':
            if(strncmp(optarg, "-", 2) == 0)
               opt->outputPath = NULL;
            else
               opt->outputPath = optarg;
            break;
         case 'p':
            opt->preview = DmtxTrue;
            break;
         case 'r':
            err = StringToInt(&(opt->rotate), optarg, &ptr);
            if(err != DmtxPass || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid rotation angle specified \"%s\""), optarg);
            break;
         case 's':
            /* Determine correct barcode size and/or shape */
            if(*optarg == 's') {
               opt->sizeIdx = DmtxSymbolSquareAuto;
            }
            else if(*optarg == 'r') {
               opt->sizeIdx = DmtxSymbolRectAuto;
            }
            else {
               for(i = 0; i < DmtxSymbolSquareCount + DmtxSymbolRectCount; i++) {
                  if(strncmp(optarg, symbolSizes[i], 8) == 0) {
                     opt->sizeIdx = i;
                     break;
                  }
               }
               if(i == DmtxSymbolSquareCount + DmtxSymbolRectCount)
                  return DmtxFail;
            }
            break;
         case 'C':
            opt->color[0] = 0;
            opt->color[1] = 0;
            opt->color[2] = 0;
            fprintf(stdout, "Option \"%c\" not implemented\n", optchr);
            break;
         case 'B':
            opt->bgColor[0] = 255;
            opt->bgColor[1] = 255;
            opt->bgColor[2] = 255;
            fprintf(stdout, "Option \"%c\" not implemented\n", optchr);
            break;
         case 'M':
            opt->mosaic = DmtxTrue;
            break;
         case 'R':
            err = StringToInt(&(opt->dpi), optarg, &ptr);
            if(err != DmtxPass || opt->dpi <= 0 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid dpi specified \"%s\""), optarg);
            break;
         case 'v':
            opt->verbose = DmtxTrue;
            break;
         case 'V':
            fprintf(stdout, "%s version %s\n", programName, DmtxVersion);
            fprintf(stdout, "libdmtx version %s\n", dmtxVersion());
            exit(0);
            break;
         default:
            return DmtxFail;
            break;
      }
   }

   opt->inputPath = (*argvp)[optind];

   /* XXX here test for incompatibility between options. For example you
      cannot specify dpi if PNM output is requested */

   return DmtxPass;
}

/**
 *
 *
 */
static void
ReadInputData(int *codeBufferSize, unsigned char *codeBuffer, UserOptions *opt)
{
   int fd;

   /* Open file or stdin for reading */
   fd = (opt->inputPath == NULL) ? 0 : open(opt->inputPath, O_RDONLY);
   if(fd == -1)
      FatalError(EX_IOERR, _("Error while opening file \"%s\""), opt->inputPath);

   /* Read input contents into buffer */
   *codeBufferSize = read(fd, codeBuffer, DMTXWRITE_BUFFER_SIZE);
   if(*codeBufferSize == DMTXWRITE_BUFFER_SIZE)
      FatalError(EX_DATAERR, _("Message to be encoded is too large"));

   /* Close file only if not stdin */
   if(fd != 0 && close(fd) != 0)
      FatalError(EX_IOERR, _("Error while closing file"));
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
      fprintf(stderr, _("Usage: %s [OPTION]... [FILE]\n"), programName);
      fprintf(stderr, _("Try `%s --help' for more information.\n"), programName);
   }
   else {
      fprintf(stdout, _("Usage: %s [OPTION]... [FILE]\n"), programName);
      fprintf(stdout, _("\
Encode FILE or standard input and write Data Matrix image\n\
\n\
Example: %s message.txt -o message.png\n\
Example: echo -n 123456 | %s -o message.png\n\
\n\
OPTIONS:\n"), programName, programName);
      fprintf(stdout, _("\
  -c, --codewords             print codeword listing\n\
  -d, --module=NUM            module size (in pixels)\n\
  -m, --margin=NUM            margin size (in pixels)\n\
  -e, --encoding=[abcet8x]    primary encodation scheme\n\
            a = ASCII [default]   b = Best optimized [beta]\n\
            c = C40               e = EDIFACT\n\
            t = Text              8 = Base 256\n\
            x = X12\n"));
      fprintf(stdout, _("\
  -f, --format=FORMAT         PNG [default], TIF, GIF, PDF, etc...\n\
  -l, --list-formats          list supported image formats\n\
  -o, --output=FILE           output filename\n\
  -p, --preview               print ASCII art preview\n\
  -r, --rotate=DEGREES        rotation angle (degrees)\n"));
      fprintf(stdout, _("\
  -s, --symbol-size=[sr|RxC]  symbol size (default \"s\")\n\
        Automatic size options:\n\
            s = Auto square         r = Auto rectangle\n\
        Manual size options for square symbols:\n\
            10x10   12x12   14x14   16x16   18x18   20x20\n\
            22x22   24x24   26x26   32x32   36x36   40x40\n\
            44x44   48x48   52x52   64x64   72x72   80x80\n\
            88x88   96x96 104x104 120x120 132x132 144x144\n\
        Manual size options for rectangle symbols:\n\
             8x18    8x32   12x26   12x36   16x36   16x48\n"));
      fprintf(stdout, _("\
  -C, --color=COLOR           barcode color (not implemented)\n\
  -B, --bg-color=COLOR        background color (not implemented)\n\
  -M, --mosaic                create Data Mosaic (non-standard)\n\
  -R, --resolution=NUM        set image print resolution (dpi)\n\
  -v, --verbose               use verbose messages\n\
  -V, --version               print version information\n\
      --help                  display this help and exit\n"));
      fprintf(stdout, _("\nReport bugs to <mike@dragonflylogic.com>.\n"));
   }

   exit(status);
}

/**
 *
 *
 */
static void
CleanupMagick(MagickWand **wand, int magickError)
{
   char *excMessage;
   ExceptionType excSeverity;

   if(magickError == DmtxTrue) {
      excMessage = MagickGetException(*wand, &excSeverity);
      fprintf(stderr, "%s %s %lu %s\n", GetMagickModule(), excMessage);
      MagickRelinquishMemory(excMessage);
   }

   if(*wand != NULL) {
      DestroyMagickWand(*wand);
      *wand = NULL;
   }
}

/**
 * @brief  List supported input image formats on stdout
 * @return void
 */
static void
ListImageFormats(void)
{
   int i, index;
   int row, rowCount;
   int col, colCount;
   unsigned long totalCount;
   char **list;

   list = MagickQueryFormats("*", &totalCount);

   if(list == NULL)
      return;

   fprintf(stdout, "\n");

   colCount = 7;
   rowCount = totalCount/colCount;
   if(totalCount % colCount)
      rowCount++;

   for(i = 0; i < colCount * rowCount; i++) {
      col = i%colCount;
      row = i/colCount;
      index = col*rowCount + row;
      fprintf(stdout, "%10s", (index < totalCount) ? list[col*rowCount+row] : " ");
      fprintf(stdout, "%s", (col+1 < colCount) ? " " : "\n");
   }
   fprintf(stdout, "\n");

   MagickRelinquishMemory(list);
}

/**
 *
 *
 *
 */
static char *
GetImageFormat(UserOptions *opt)
{
   char *ptr = NULL;

   /* Derive format from filename extension */
   if(opt->outputPath != NULL) {
      ptr = strrchr(opt->outputPath, '.');
      if(ptr != NULL)
         ptr++;
   }

   /* Found filename extension but format was also provided */
   if(ptr != NULL && strlen(ptr) > 0 && opt->format != NULL)
      fprintf(stderr, "WARNING: --format (-f) argument ignored; Format taken from filename\n");

   /* If still undefined then use format argument */
   if(ptr == NULL || strnlen(ptr, 1) == 0)
      ptr = opt->format;

   return ptr;
}

/**
 *
 *
 */
static DmtxPassFail
WriteImageFile(UserOptions *opt, DmtxEncode *enc, char *format)
{
   MagickBooleanType success;
   MagickWand *wand;
   char *outputPath;

   MagickWandGenesis();

   wand = NewMagickWand();
   if(wand == NULL)
      FatalError(EX_OSERR, "Undefined error");

   success = MagickConstituteImage(wand, enc->image->width, enc->image->height,
         "RGB", CharPixel, enc->image->pxl);
   if(success == MagickFalse) {
      CleanupMagick(&wand, DmtxTrue);
      FatalError(EX_OSERR, "Undefined error");
   }

   success = MagickSetImageFormat(wand, format);
   if(success == MagickFalse) {
      CleanupMagick(&wand, DmtxFalse);
      FatalError(EX_OSERR, "Illegal format \"%s\"", format);
   }

   outputPath = (opt->outputPath == NULL) ? "-" : opt->outputPath;

   success = MagickWriteImage(wand, outputPath);
   if(success == MagickFalse) {
      CleanupMagick(&wand, DmtxTrue);
      FatalError(EX_OSERR, "Undefined error");
   }

   CleanupMagick(&wand, DmtxFalse);

   MagickWandTerminus();

   return DmtxPass;
}

/**
 *
 *
 */
static DmtxPassFail
WriteSvgFile(UserOptions *opt, DmtxEncode *enc, char *format)
{
   int col, row, rowInv;
   int symbolCols, symbolRows;
   int width, height, module;
   int defineOnly = DmtxFalse;
   char *idString = NULL;
   FILE *fp;

   if(StrNCmpI(format, "svg:", 4) == DmtxTrue) {
      defineOnly = DmtxTrue;
      idString = &format[4];
   }

   if(idString == NULL || strlen(idString) == 0)
      idString = "dmtx_0001";

   if(opt->outputPath == NULL) {
      fp = stdout;
   }
   else {
      fp = fopen(opt->outputPath, "wb");
      if(fp == NULL)
         FatalError(EX_CANTCREAT, "Unable to create output file \"%s\"", opt->outputPath);
   }

   width = 2 * enc->marginSize + (enc->region.symbolCols * enc->moduleSize);
   height = 2 * enc->marginSize + (enc->region.symbolRows * enc->moduleSize);

   symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, enc->region.sizeIdx);
   symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, enc->region.sizeIdx);

   /* Print SVG Header */
   if(defineOnly == DmtxFalse) {
      fprintf(fp, "\
<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n\
<!-- Created with dmtxwrite (http://www.libdmtx.org/) -->\n\
<svg\n\
   xmlns:svg=\"http://www.w3.org/2000/svg\"\n\
   xmlns=\"http://www.w3.org/2000/svg\"\n\
   xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n\
   version=\"1.0\"\n\
   width=\"%d\"\n\
   height=\"%d\"\n\
   id=\"svg2\">\n\
  <defs>\n", width, height);
   }

   fprintf(fp, "  <g id=\"%s\">\n", idString);
   fprintf(fp, "    <desc>Layout:%dx%d Symbol:%dx%d Data Matrix</desc>\n",
         width, height, symbolCols, symbolRows);

   /* Write Data Matrix ON modules */
   for(row = 0; row < enc->region.symbolRows; row++) {
      rowInv = enc->region.symbolRows - row - 1;
      for(col = 0; col < enc->region.symbolCols; col++) {
         module = dmtxSymbolModuleStatus(enc->message, enc->region.sizeIdx, row, col);
         if(module & DmtxModuleOn)
            fprintf(fp, "    <rect width=\"%d\" height=\"%d\" x=\"%d\" y=\"%d\" />\n",
                  opt->moduleSize, opt->moduleSize,
                  col * opt->moduleSize + opt->marginSize,
                  rowInv * opt->moduleSize + opt->marginSize);
      }
   }

   fprintf(fp, "  </g>\n");

   /* Close SVG document */
   if(defineOnly == DmtxFalse) {
      fprintf(fp, "\
  </defs>\n\
\n\
  <use xlink:href=\"#%s\" x='0' y='0' style=\"fill:#000000;fill-opacity:1;stroke:none\" />\n\
\n\
</svg>\n", idString);
   }

   return DmtxPass;
}

/**
 *
 *
 */
static DmtxPassFail
WriteAsciiPreview(DmtxEncode *enc)
{
   int symbolRow, symbolCol;

   fputc('\n', stdout);

   /* ASCII prints from top to bottom */
   for(symbolRow = enc->region.symbolRows - 1; symbolRow >= 0; symbolRow--) {

      fputs("    ", stdout);
      for(symbolCol = 0; symbolCol < enc->region.symbolCols; symbolCol++) {
         fputs((dmtxSymbolModuleStatus(enc->message, enc->region.sizeIdx,
               symbolRow, symbolCol) & DmtxModuleOnRGB) ? "XX" : "  ", stdout);
      }
      fputs("\n", stdout);
   }

   fputc('\n', stdout);

   return DmtxPass;
}

/**
 *
 *
 */
static DmtxPassFail
WriteCodewordList(DmtxEncode *enc)
{
   int i;
   int dataWordLength;
   int remainingDataWords;

   dataWordLength = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, enc->region.sizeIdx);

   for(i = 0; i < enc->message->codeSize; i++) {
      remainingDataWords = dataWordLength - i;
      if(remainingDataWords > enc->message->padCount)
         fprintf(stdout, "%c:%03d\n", 'd', enc->message->code[i]);
      else if(remainingDataWords > 0)
         fprintf(stdout, "%c:%03d\n", 'p', enc->message->code[i]);
      else
         fprintf(stdout, "%c:%03d\n", 'e', enc->message->code[i]);
   }

   return DmtxPass;
}

/**
 *
 *
 */
static DmtxBoolean
StrNCmpI(const char *s1, const char *s2, size_t n)
{
   size_t i;

   if(s1 == NULL || s2 == NULL || n == 0)
      return DmtxFalse;

   for(i = 0; i < n; i++) {
      if(tolower(s1[i]) != tolower(s2[i]))
         return DmtxFalse;
      if(s1[i] == '\0' || s2[i] == '\0')
         break;
   }

   return DmtxTrue;
}
