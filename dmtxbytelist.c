/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2010 Mike Laughton. All rights reserved.
 * Copyright 2012-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * \file file.c
 */

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
extern void
dmtxByteListClear(DmtxByteList *list)
{
   memset(list->b, 0x00, sizeof(DmtxByte) * list->capacity);
   list->length = 0;
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
dmtxByteListCopy(DmtxByteList *dst, const DmtxByteList *src, DmtxPassFail *passFail)
{
   int length;

   if(dst->capacity < src->length)
   {
      *passFail = DmtxFail; /* dst must be large enough to hold src data */
   }
   else
   {
      /* Copy as many bytes as dst can hold or src can provide (smaller of two) */
      length = (dst->capacity < src->capacity) ? dst->capacity : src->capacity;

      dst->length = src->length;
      memcpy(dst->b, src->b, sizeof(unsigned char) * length);
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
