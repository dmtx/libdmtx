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
#include <dmtx.h>
#include "dmtxread.h"

#define DMTXREAD_SUCCESS     0
#define DMTXREAD_ERROR       1

#define MIN(x,y) ((x < y) ? x : y)

static int HandleArgs(ScanOptions *options, int *argcp, char **argvp[], int *fileIndex);
static long StringToLong(char *numberString);
static void ShowUsage(int status);
static void FatalError(int errorCode, char *fmt, ...);
static int ScanImage(ScanOptions *options, DmtxDecode *info, char *imageFile);

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
   DmtxDecode *info;
   ScanOptions options;

   err = HandleArgs(&options, &argc, &argv, &fileIndex);
   if(err)
      ShowUsage(err);

   info = dmtxDecodeStructCreate();

   info->option = DmtxSingleScanOnly;

   while(fileIndex < argc) {
      ScanImage(&options, info, argv[fileIndex]);

      fileIndex++;
   }

   dmtxDecodeStructDestroy(&info);

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
         {"count",      required_argument, NULL, 'c'},
         {"vertical",   required_argument, NULL, 'v'},
         {"horizontal", required_argument, NULL, 'h'},
         {"verbose",    no_argument,       NULL, 'V'},
         {"help",       no_argument,       NULL,  0 },
         {0, 0, 0, 0}
   };

   programName = (*argvp)[0];
   if(programName && strrchr(programName, '/'))
      programName = strrchr(programName, '/') + 1;

   memset(options, 0x00, sizeof(ScanOptions));

   options->maxCount = 0; // Unlimited
   options->hScanCount = 5;
   options->vScanCount = 5;
   options->verbose = 0;

   *fileIndex = 0;

   for(;;) {
      opt = getopt_long(*argcp, *argvp, "c:v:h:V", longOptions, &longIndex);
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
         case 'V':
            options->verbose = 1;
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
  -V,     --verbose         use verbose messages\n\
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
 * Scan image for Data Matrix barcodes and print decoded data to STDOUT.
 *
 * @param options   runtime options from defaults or command line
 * @param info      pointer to DmtxDecode struct
 * @param imageFile path/name of PNG image
 * @return          DMTXREAD_SUCCESS | DMTXREAD_ERROR
 */
static int
ScanImage(ScanOptions *options, DmtxDecode *info, char *imageFile)
{
   int success;
   int row, col;
   int hScanGap, vScanGap;
   unsigned char outputString[1024];
   DmtxMatrixRegion *matrixRegion;

   if(options->verbose)
      fprintf(stdout, _("Scanning \"%s\"\n"), imageFile);

   dmtxImageInit(&(info->image));
   dmtxScanStartNew(info);

   success = dmtxImageLoadPng(&(info->image), imageFile);
   if(success != DMTX_SUCCESS)
      FatalError(3, _("Unable to load image \"%s\""), imageFile);

   hScanGap = (int)((double)info->image.width/options->hScanCount + 0.5);
   vScanGap = (int)((double)info->image.height/options->vScanCount + 0.5);

   for(col = hScanGap; col < info->image.width; col += hScanGap)
      dmtxScanLine(info, DmtxDirUp, col);

   for(row = vScanGap; row < info->image.height; row += vScanGap)
      dmtxScanLine(info, DmtxDirRight, row);

   if(dmtxDecodeGetMatrixCount(info) > 0) {
      matrixRegion = dmtxDecodeGetMatrix(info, 0);
      memset(outputString, 0x00, 1024);
      strncpy((char *)outputString, (const char *)matrixRegion->output, MIN(matrixRegion->outputIdx, 1023));
      fprintf(stdout, "%s\n", outputString);
   }

   dmtxImageDeInit(&(info->image));

   return DMTXREAD_SUCCESS;
}
