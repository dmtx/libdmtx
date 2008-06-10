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
#include <dmtx.h>
#include "dmtxquery.h"
#include "../common/dmtxutil.h"

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
         {"version",          no_argument,       NULL, 'V'},
         {"help",             no_argument,       NULL,  0 },
         {0, 0, 0, 0}
   };

   programName = Basename((*argvp)[0]);

   if(*argcp == 1) /* Program called without arguments */
      return DMTXUTIL_ERROR;

   for(;;) {
      opt = getopt_long(*argcp, *argvp, "V", longOptions, &longIndex);
      if(opt == -1)
         break;

      switch(opt) {
         case 0: /* --help */
            ShowUsage(0);
            break;
         case 'V':
            fprintf(stdout, "%s version %s\n", programName, DMTX_VERSION);
            fprintf(stdout, "libdmtx version %s\n", dmtxVersion());
            exit(0);
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
      fprintf(stderr, _("Usage: %s PROPERTY [OPTION]... [FILE]...\n"), programName);
      fprintf(stderr, _("Try `%s --help' for more information.\n"), programName);
   }
   else {
      fprintf(stdout, _("Usage: %s PROPERTY [OPTION]... [FILE]...\n"), programName);
      fprintf(stdout, _("\
Extract information from the XML output from dmtxread for individual or\n\
grouped barcode scan results.\n\
\n\
Example: dmtxread barcode.png | %s barcode.count\n\
Example: %s barcode.2.rotation scanresults.xml\n\
\n\
PROPERTY:\n"), programName, programName);
      fprintf(stdout, _("\
   barcode.count             count of all barcodes found in image\n\
   barcode.N                 print all properties of Nth barcode\n\
   barcode.N.BPROP           print BPROP property of Nth barcode\n\
\n\
   message.count             count of all messages found in image\n\
   message.N                 print all properties of Nth message\n\
   message.N.MPROP           print MPROP property of Nth message\n\
   message.N.barcode.count   count of all barcodes in Nth message\n\
   message.N.barcode.M       Mth barcode of Nth message, print all\n\
   message.N.barcode.M.BPROP Mth barcode of Nth message, print BPROP\n\
\n\
   BPROP barcode properties:\n\
      message        message_number      message_position\n\
      matrix_size    data_codewords      error_codewords\n\
      rotation       data_regions_count  interleaved_blocks\n\
\n\
   MPROP message properties:\n\
      message        data_codeword       error_codeword\n\
\n\
OPTIONS:\n\
  -V, --version              print program version information\n\
      --help                 display this help and exit\n"));
      fprintf(stdout, _("\nReport bugs to <mike@dragonflylogic.com>.\n"));
   }

   exit(status);
}
