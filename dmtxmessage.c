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
 * @file dmtxmessage.c
 * @brief Data message handling
 */

/**
 * @brief  Allocate memory for message
 * @param  sizeIdx
 * @param  symbolFormat DMTX_FORMAT_MATRIX | DMTX_FORMAT_MOSAIC
 * @return Address of allocated memory
 */
extern DmtxMessage *
dmtxMessageMalloc(int sizeIdx, int symbolFormat)
{
   DmtxMessage *message;
   int mappingRows, mappingCols;

   assert(symbolFormat == DMTX_FORMAT_MATRIX || symbolFormat == DMTX_FORMAT_MOSAIC);

   mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, sizeIdx);
   mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, sizeIdx);

   message = (DmtxMessage *)malloc(sizeof(DmtxMessage));
   if(message == NULL)
      return NULL;
   memset(message, 0x00, sizeof(DmtxMessage));

   message->arraySize = sizeof(unsigned char) * mappingRows * mappingCols;

   message->array = (unsigned char *)calloc(1, message->arraySize);
   if(message->array == NULL) {
      perror("Calloc failed");
      dmtxMessageFree(&message);
      return NULL;
   }

   message->codeSize = sizeof(unsigned char) *
         dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx) +
         dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords, sizeIdx);

   if(symbolFormat == DMTX_FORMAT_MOSAIC)
      message->codeSize *= 3;

   message->code = (unsigned char *)calloc(message->codeSize, sizeof(unsigned char));
   if(message->code == NULL) {
      perror("Calloc failed");
      dmtxMessageFree(&message);
      return NULL;
   }

   /* XXX not sure if this is the right place or even the right approach.
      Trying to allocate memory for the decoded data stream and will
      initially assume that decoded data will not be larger than 2x encoded data */
   message->outputSize = sizeof(unsigned char) * message->codeSize * 10;
   message->output = (unsigned char *)calloc(message->outputSize, sizeof(unsigned char));
   if(message->output == NULL) {
      perror("Calloc failed");
      dmtxMessageFree(&message);
      return NULL;
   }

   return message;
}

/**
 * @brief  Free memory previously allocated for message
 * @param  message
 * @return void
 */
extern void
dmtxMessageFree(DmtxMessage **message)
{
   if(*message == NULL)
      return;

   if((*message)->array != NULL)
      free((*message)->array);

   if((*message)->code != NULL)
      free((*message)->code);

   if((*message)->output != NULL)
      free((*message)->output);

   free(*message);

   *message = NULL;
}
