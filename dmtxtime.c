/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2008, 2009 Mike Laughton. All rights reserved.
 * Copyright 2012-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxtime.c
 * \brief Time handling
 */

#define DMTX_USEC_PER_SEC 1000000

#if defined(HAVE_SYS_TIME_H) && defined(HAVE_GETTIMEOFDAY)

#include <sys/time.h>
#include <time.h>
#define DMTX_TIME_PREC_USEC 1

/**
 * \brief  GETTIMEOFDAY version
 * \return Time now
 */
extern DmtxTime
dmtxTimeNow(void)
{
   DmtxPassFail err;
   struct timeval tv;
   DmtxTime tNow;

   err = gettimeofday(&tv, NULL);
   if(err != 0)
      ; /* XXX handle error better here */

   tNow.sec = tv.tv_sec;
   tNow.usec = tv.tv_usec;

   return tNow;
}

#elif defined(_MSC_VER)

#include <Windows.h>
#define DMTX_TIME_PREC_USEC 1

/**
 * \brief  MICROSOFT VC++ version
 * \return Time now
 */
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
#define DMTX_TIME_PREC_USEC 1000000

/**
 * \brief  Generic 1 second resolution version
 * \return Time now
 */
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
 * \brief  Add milliseconds to time t
 * \param  t
 * \param  msec
 * \return Adjusted time
 */
extern DmtxTime
dmtxTimeAdd(DmtxTime t, long msec)
{
   int usec;

   usec = msec * 1000;

   /* Ensure that time difference will register on local system */
   if(usec > 0 && usec < DMTX_TIME_PREC_USEC)
      usec = DMTX_TIME_PREC_USEC;

   /* Add time */
   t.sec += usec/DMTX_USEC_PER_SEC;
   t.usec += usec%DMTX_USEC_PER_SEC;

   /* Roll extra usecs into secs */
   while(t.usec >= DMTX_USEC_PER_SEC) {
      t.sec++;
      t.usec -= DMTX_USEC_PER_SEC;
   }

   return t;
}

/**
 * \brief  Determine whether the received timeout has been exceeded
 * \param  timeout
 * \return 1 (true) | 0 (false)
 */
extern int
dmtxTimeExceeded(DmtxTime timeout)
{
   DmtxTime now;

   now = dmtxTimeNow();

   return (now.sec > timeout.sec || (now.sec == timeout.sec && now.usec > timeout.usec));
}

#undef DMTX_TIME_PREC_USEC
#undef DMTX_USEC_PER_SEC
