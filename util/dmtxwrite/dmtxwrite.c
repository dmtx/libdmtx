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

/* $Id: dmtxwrite.c,v 1.5 2006-10-15 19:37:58 mblaughton Exp $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <dmtx.h>
#include "dmtxwrite.h"

#define DMTXWRITE_SUCCESS     0
#define DMTXWRITE_ERROR       1

#define MIN(x,y) ((x < y) ? x : y)

static void InitScanOptions(ScanOptions *options);
static int HandleArgs(ScanOptions *options, int *argcp, char **argvp[], DmtxEncode *encode);
static long StringToLong(char *numberString);
static void ShowUsage(int status);
static void FatalError(int errorCode, char *fmt, ...);
static void WriteImagePnm(DmtxEncode *encode, char *path);

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
   ScanOptions options;
   DmtxEncode *encode;

   encode = dmtxEncodeCreate();

   InitScanOptions(&options);

   err = HandleArgs(&options, &argc, &argv, encode);
   if(err)
      ShowUsage(err);

   dmtxEncodeData(encode, options.inputString);

   WriteImagePnm(encode, options.outputPath);

   dmtxEncodeDestroy(&encode);

   exit(0);
}

/**
 *
 *
 */
static void
InitScanOptions(ScanOptions *options)
{
   memset(options, 0x00, sizeof(ScanOptions));

   options->outputPath = "encode.pnm";
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
HandleArgs(ScanOptions *options, int *argcp, char **argvp[], DmtxEncode *encode)
{
   int opt;
   int longIndex;

   struct option longOptions[] = {
         {"output",   required_argument, NULL, 'o'},
         {"format",   required_argument, NULL, 'f'},
         {"encoding", required_argument, NULL, 'e'},
         {"size",     required_argument, NULL, 's'},
         {"tilde",    no_argument,       NULL, 't'},
         {"margin",   required_argument, NULL, 'm'},
         {"module",   required_argument, NULL, 'd'},
         {"rotate",   required_argument, NULL, 'r'},
         {"bgcolor",  required_argument, NULL, 'b'},
         {"color",    required_argument, NULL, 'c'},
         {"verbose",  no_argument,       NULL, 'V'},
         {"help",     no_argument,       NULL,  0 },
         {0, 0, 0, 0}
   };

   programName = (*argvp)[0];
   if(programName && strrchr(programName, '/'))
      programName = strrchr(programName, '/') + 1;

   for(;;) {
      opt = getopt_long(*argcp, *argvp, "o:f:e:s:tm:d:r:b:c:V", longOptions, &longIndex);
      if(opt == -1)
         break;

      switch(opt) {
         case 0: // --help
            ShowUsage(0);
            break;
         case 'o':
            options->outputPath = optarg;
            break;
         case 'f':
         case 'e':
         case 's':
         case 't':
            fprintf(stdout, "Option \"%c\" not implemented yet\n", opt);
            break;
         case 'm':
            encode->marginSize = StringToLong(optarg);
            break;
         case 'd':
            encode->moduleSize = StringToLong(optarg);
            break;
         case 'r':
            options->rotate = StringToLong(optarg);
            break;
         case 'b':
            options->bgColor.R = 255;
            options->bgColor.G = 255;
            options->bgColor.B = 255;
            fprintf(stdout, "Option \"%c\" not implemented yet\n", opt);
            break;
         case 'c':
            options->fgColor.R = 0;
            options->fgColor.G = 0;
            options->fgColor.B = 0;
            fprintf(stdout, "Option \"%c\" not implemented yet\n", opt);
            break;
         case 'V':
            options->verbose = 1;
            break;
         default:
            return DMTXWRITE_ERROR;
            break;
      }
   }

   // Message not provided
   if(optind == *argcp) {
      // XXX not sure if this is defined behavior -- optind only
      // XXX defined when non-option parameter is present?
      if(*argcp == 1)
         return DMTXWRITE_ERROR;
      else
         FatalError(1, _("Must provide message to be encoded\n"));
   }

   options->inputString = (unsigned char *)(*argvp)[optind];

   return DMTXWRITE_SUCCESS;
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
Example: %s -f PNG -o secret.png \"Top Secret Code\"\n\
\n\
OPTIONS:\n"), programName);
      fprintf(stdout, _("\
  -o, --output=FILE          barcode output filename\n\
  -f, --format=[pa]          barcode output format\n\
        p = PNG              [default]\n\
        a = ASCII\n\
  -e, --encoding=[uact8]     encodation scheme\n\
        u = Auto             [default]\n\
        a = ASCII\n\
        c = C40\n\
        t = Text\n\
        8 = Base 256\n\
  -s, --size=SIZE            symbol size in Rows x Cols\n\
        s = Auto square      [default]\n\
        r = Auto rectangle\n\
        Valid SIZE options for square symbols:\n\
        10x10,  12x12,   14x14,   16x16,   18x18,   20x20,\n\
        22x22,  24x24,   26x26,   32x32,   36x36,   40x40,\n\
        44x44,  48x48,   52x52,   64x64,   72x72,   80x80,\n\
        88x88,  96x96, 104x104, 120x120, 132x132, 144x144\n\
        Valid SIZE options for rectangular symbols:\n\
         8x18,   8x32,   12x26,   12x36,   16x36,   16x48\n\
  -t, --tilde                process tilde\n\
  -m, --margin=NUM           margin size (in pixels)\n\
  -d, --module=NUM           module size (in pixels)\n\
  -r, --rotate=NUM           rotation angle (degrees)\n\
  -b, --bgcolor=COLOR        background color\n\
  -c, --color=COLOR          foreground color\n\
  -V, --verbose              use verbose messages\n\
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
         fprintf(output, "%c", encode->image.pxl[(encode->image.height - row - 1) * encode->image.width + col].R);
         fprintf(output, "%c", encode->image.pxl[(encode->image.height - row - 1) * encode->image.width + col].G);
         fprintf(output, "%c", encode->image.pxl[(encode->image.height - row - 1) * encode->image.width + col].B);
      }
   }
   fclose(output);
}
