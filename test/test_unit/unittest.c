/*
libdmtx - Data Matrix Encoding/Decoding Library
Copyright (C) 2007 Mike Laughton

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
#include <dmtx.h>
#include "../../util/common/dmtxutil.h"

char *programName;

static void timeAddTest(void);
static void timePrint(DmtxTime t);

int
main(int argc, char *argv[])
{
   programName = argv[0];

   timeAddTest();

   exit(0);
}

/**
 *
 *
 */
static void
timePrint(DmtxTime t)
{
   fprintf(stdout, "t.sec: %llu\n", t.sec);
   fprintf(stdout, "t.usec: %lu\n", t.usec);
}

/**
 *
 *
 */
static void
timeAddTest(void)
{
   DmtxTime t0, t1;

   t0 = dmtxTimeNow();
   t0.usec = 999000;

   t1 = dmtxTimeAdd(t0, 0);
   if(memcmp(&t0, &t1, sizeof(DmtxTime)) != 0)
      FatalError(1, "timeAddTest\n");

   t1 = dmtxTimeAdd(t0, 1);
   if(memcmp(&t0, &t1, sizeof(DmtxTime)) == 0)
      FatalError(2, "timeAddTest\n");

   t1 = dmtxTimeAdd(t0, 1);
   if(t1.sec != t0.sec + 1 || t1.usec != 0) {
      timePrint(t0);
      timePrint(t1);
      FatalError(3, "timeAddTest\n");
   }

   t1 = dmtxTimeAdd(t0, 1001);
   if(t1.sec != t0.sec + 2 || t1.usec != 0) {
      timePrint(t0);
      timePrint(t1);
      FatalError(4, "timeAddTest\n");
   }

   t1 = dmtxTimeAdd(t0, 2002);
   if(t1.sec != t0.sec + 3 || t1.usec != 1000) {
      timePrint(t0);
      timePrint(t1);
      FatalError(5, "timeAddTest\n");
   }
}
