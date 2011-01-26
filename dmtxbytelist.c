/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2010 Mike Laughton

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
 *
 *
 */
extern DmtxByteList
dmtxByteListBuild(DmtxByte *storage, size_t capacity)
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
dmtxByteListInit(DmtxByteList *list, size_t length, DmtxByte value)
{
   assert(length <= list->capacity);

   list->length = length;
   memset(list->b, value, sizeof(DmtxByte) * list->capacity);
}

/**
 *
 *
 */
extern DmtxPassFail
dmtxByteListCopy(DmtxByteList *dest, DmtxByteList *src)
{
   size_t length;

   /* dest must be large enough to hold src data */
   if(dest->capacity < src->length)
      return DmtxFail;

   /* Copy as many bytes as dest can hold or src can provide (smaller of two) */
   length = (dest->capacity < src->capacity) ? dest->capacity : src->capacity;

   dest->length = src->length;
   memcpy(dest->b, src->b, sizeof(unsigned char) * length);

   return DmtxTrue;
}

/**
 *
 *
 */
extern size_t
dmtxByteListPush(DmtxByteList *list, DmtxByte value)
{
   assert(list->length < list->capacity);

   list->b[list->length++] = value;

   return list->length;
}
