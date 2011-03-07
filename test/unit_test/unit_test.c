/*
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2007, 2008, 2009 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in parent directory for full terms of
 * use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
*/

/**
 * \file unit_test.c
 */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../dmtx.h"
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
#ifdef _MSC_VER
   fprintf(stdout, "t.sec: %llu\n", t.sec);
#else
   fprintf(stdout, "t.sec: %lu\n", t.sec);
#endif

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

/**
 *
 *
 */
/**
static void
TestRGB(void)
{
   unsigned char *pxl;
   FILE *fp;

   pxl = (unsigned char *)malloc(320 * 240 * 3);
   assert(pxl != NULL);

   fp = fopen("fruit_matrix.rgb", "rb");
   assert(fp != NULL);

   fread(ptr, 3, 320 * 240, fp);
   fclose(fp);

   dmtxImageCreate(ptr, 320, 240, DmtxPack24bppRGB);
}
*/
