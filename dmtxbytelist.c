/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2010 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in parent directory for full terms of
 * use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 */

/**
 * \file file.c
 */

#include <stdio.h>
#include <string.h>
#include "dmtx.h"
#include "dmtxstatic.h"

/**
 *
 *
 */
extern DmtxByteList
dmtxByteListBuild(DmtxByte *storage, int capacity)
{
   DmtxByteList list;

   list.b = storage;
   list.capacity = capacity;
   list.length = 0;

   return list;
}

/**
 *
 *
 */
extern void
dmtxByteListInit(DmtxByteList *list, int length, DmtxByte value, DmtxPassFail *passFail)
{
   if(length > list->capacity)
   {
      *passFail = DmtxFail;
   }
   else
   {
      list->length = length;
      memset(list->b, value, sizeof(DmtxByte) * list->capacity);
      *passFail = DmtxPass;
   }
}

/**
 *
 *
 */
extern DmtxBoolean
dmtxByteListHasCapacity(DmtxByteList *list)
{
   return (list->length < list->capacity) ? DmtxTrue : DmtxFalse;
}

/**
 *
 *
 */
extern void
dmtxByteListCopy(DmtxByteList *dest, const DmtxByteList *src, DmtxPassFail *passFail)
{
   int length;

   if(dest->capacity < src->length)
   {
      *passFail = DmtxFail; /* dest must be large enough to hold src data */
   }
   else
   {
      /* Copy as many bytes as dest can hold or src can provide (smaller of two) */
      length = (dest->capacity < src->capacity) ? dest->capacity : src->capacity;

      dest->length = src->length;
      memcpy(dest->b, src->b, sizeof(unsigned char) * length);
      *passFail = DmtxPass;
   }
}

/**
 *
 *
 */
extern void
dmtxByteListPush(DmtxByteList *list, DmtxByte value, DmtxPassFail *passFail)
{
   if(list->length >= list->capacity)
   {
      *passFail = DmtxFail;
   }
   else
   {
      list->b[list->length++] = value;
      *passFail = DmtxPass;
   }
}

/**
 *
 *
 */
extern DmtxByte
dmtxByteListPop(DmtxByteList *list, DmtxPassFail *passFail)
{
   *passFail = (list->length > 0) ? DmtxPass : DmtxFail;

   return list->b[--(list->length)];
}

/**
 *
 *
 */
extern void
dmtxByteListPrint(DmtxByteList *list, char *prefix)
{
   int i;

   if(prefix != NULL)
      fprintf(stdout, "%s", prefix);

   for(i = 0; i < list->length; i++)
      fprintf(stdout, " %d", list->b[i]);

   fputc('\n', stdout);
}
