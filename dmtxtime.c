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

/* $Id: dmtxregion.c 148 2008-06-05 05:06:56Z mblaughton $ */

/**
 *
 *
 */
#if defined(HAVE_SYS_TIME_H) && defined(HAVE_GETTIMEOFDAY)
#include <sys/time.h>
extern DmtxTime
dmtxTimeNow(void)
{
   int err;
   struct timeval tv;
   DmtxTime tNow;

   err = gettimeofday(&tv, NULL);
   if(err != 0)
      ; /* XXX handle error better here */

   tNow.sec = tv.tv_sec;
   tNow.usec = tv.tv_usec;

   return tNow;
}
#elif defined (_MSC_VER)
extern DmtxTime
dmtxTimeNow(void)
{
   ; /* MICROSOFT VC++ VERSION GOES HERE */
}
#else
#include <time.h>
extern DmtxTime
dmtxTimeNow(void)
{
   time_t s;
   DmtxTime tNow;

   s = time(NULL);
   if(errno != 0)
      ; /* XXX handle error better here */

   tNow.sec = s;
   tNow.usec = 0;

   return tNow;
}
#endif

/**
 *
 *
 */
extern int
dmtxTimeExceeded(DmtxTime tBeg, long duration)
{
   DmtxTime tNow;
   DmtxTime tEnd;

   tNow = dmtxTimeNow();

   tEnd.sec = tBeg.sec + (duration/1000);
   tEnd.usec = tBeg.usec + (duration%1000)*1000;

   if(tEnd.usec >= 1000000) {
      tEnd.sec++;
      tEnd.usec -= 1000000;
   }

   assert(tEnd.sec < 1000000);

   if(tNow.sec > tEnd.sec || (tNow.sec == tEnd.sec && tNow.usec > tEnd.usec))
      return 0;

   return 1;
}
