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

/**
 * @file dmtxtime.c
 * @brief Time handling
 */

/**
 *
 *
 */
#if defined(HAVE_SYS_TIME_H) && defined(HAVE_GETTIMEOFDAY)

#include <sys/time.h>

#define DMTX_TICK_USEC 1

/* GETTIMEOFDAY version */
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

#include <Windows.h>

#define DMTX_TICK_USEC 1

/* MICROSOFT VC++ version */
extern DmtxTime
dmtxTimeNow(void)
{
   FILETIME ft;
   unsigned __int64 tm;
   DmtxTime tNow;

   GetSystemTimeAsFileTime(&ft);

   tm = ft.dwHighDateTime;
   tm <<= 32;
   tm |= ft.dwLowDateTime;
   tm /= 10;

   tNow.sec = tm / 1000000UL;
   tNow.usec = tm % 1000000UL;

   return tNow;
}

#else

#include <time.h>

#define DMTX_TICK_USEC 1000000

/* Generic 1 second resolution version */
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
extern DmtxTime
dmtxTimeAdd(DmtxTime t, long duration)
{
   /* Round duration to next-higher atomic time unit */
   if(t.usec % DMTX_TICK_USEC)
      t.usec = t.usec/DMTX_TICK_USEC + DMTX_TICK_USEC;

   t.sec += (duration/1000);
   t.usec += (duration%1000)*1000;

   while(t.usec >= 1000000) {
      t.sec++;
      t.usec -= 1000000;
   }

   return t;
}

/**
 *
 *
 */
extern int
dmtxTimeExceeded(DmtxTime timeout)
{
   DmtxTime now;

   now = dmtxTimeNow();

   return (now.sec > timeout.sec || (now.sec == timeout.sec && now.usec > timeout.usec));
}
