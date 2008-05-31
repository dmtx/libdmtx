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

/* $Id: dmtxquery.c 124 2008-04-13 01:38:03Z mblaughton $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <dmtx.h>
#include "dmtxquery.h"
#include "../common/dmtxutil.h"

static void SetOptionDefaults(UserOptions *options);
static int HandleArgs(UserOptions *options, int *argcp, char **argvp[]);
static void ShowUsage(int status);

char *programName;

/**
 * Main function for the dmtxquery Data Matrix scanning utility.
 *
 * @param argc count of arguments passed from command line
 * @param argv list of argument passed strings from command line
 * @return     numeric success / failure exit code
 */
int
main(int argc, char *argv[])
{
   int err;
   UserOptions options;

   SetOptionDefaults(&options);

   err = HandleArgs(&options, &argc, &argv);
   if(err)
      ShowUsage(err);

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
}

/**
 * Sets and validates user-requested options from command line arguments.
 *
 * @param options    runtime options from defaults or command line
 * @param argcp      pointer to argument count
 * @param argvp      pointer to argument list
 * @return           DMTXUTIL_SUCCESS | DMTXUTIL_ERROR
 */
static int
HandleArgs(UserOptions *options, int *argcp, char **argvp[])
{
   int err;
   int opt;
   int longIndex;
   char *ptr;

   struct option longOptions[] = {
         {"codewords",        no_argument,       NULL, 'c'},
         {"gap",              required_argument, NULL, 'g'},
         {"newline",          no_argument,       NULL, 'n'},
         {"x-range-min",      required_argument, NULL, 'x'},
         {"x-range-max",      required_argument, NULL, 'X'},
         {"y-range-min",      required_argument, NULL, 'y'},
         {"y-range-max",      required_argument, NULL, 'Y'},
         {"verbose",          no_argument,       NULL, 'v'},
         {"corners",          no_argument,       NULL, 'C'},
         {"diagnose",         no_argument,       NULL, 'D'},
         {"error-correction", required_argument, NULL, 'E'},
         {"mosaic",           no_argument,       NULL, 'M'},
         {"page-number",      no_argument,       NULL, 'P'},
         {"version",          no_argument,       NULL, 'V'},
         {"help",             no_argument,       NULL,  0 },
         {0, 0, 0, 0}
   };

   programName = (*argvp)[0];
   if(programName && strrchr(programName, '/'))
      programName = strrchr(programName, '/') + 1;

   if(*argcp == 1) /* Program called without arguments */
      return DMTXUTIL_ERROR;

   for(;;) {
      opt = getopt_long(*argcp, *argvp, "cg:nx:X:y:Y:vCDE:MPV", longOptions, &longIndex);
      if(opt == -1)
         break;

      switch(opt) {
         case 0: /* --help */
            ShowUsage(0);
            break;
         case 'c':
            options->placeHolder = 1;
            break;
         default:
            return DMTXUTIL_ERROR;
            break;
      }
   }

   return DMTXUTIL_SUCCESS;
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
Query XML scan result file for individual or group attributes.\n\
\n\
OPTIONS:\n"), programName, programName);
      fprintf(stdout, _("\
  -c, --codewords            print codewords extracted from barcode pattern\n\
  -v, --verbose              use verbose messages\n\
  -V, --version              print program version information\n\
      --help                 display this help and exit\n"));
      fprintf(stdout, _("\nReport bugs to <mike@dragonflylogic.com>.\n"));
   }

   exit(status);
}
