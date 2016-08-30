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
 * \file dmtxmessage.c
 * \brief Data message handling
 */

/**
 * \brief  Allocate memory for message
 * \param  sizeIdx
 * \param  symbolFormat DmtxFormatMatrix | DmtxFormatMosaic
 * \return Address of allocated memory
 */
extern DmtxMessage *
dmtxMessageCreate(int sizeIdx, int symbolFormat)
{
   DmtxMessage *message;
   int mappingRows, mappingCols;

   assert(symbolFormat == DmtxFormatMatrix || symbolFormat == DmtxFormatMosaic);

   mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, sizeIdx);
   mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, sizeIdx);

   message = (DmtxMessage *)calloc(1, sizeof(DmtxMessage));
   if(message == NULL)
      return NULL;

   message->arraySize = sizeof(unsigned char) * mappingRows * mappingCols;

   message->array = (unsigned char *)calloc(1, message->arraySize);
   if(message->array == NULL) {
      perror("Calloc failed");
      dmtxMessageDestroy(&message);
      return NULL;
   }

   message->codeSize = sizeof(unsigned char) *
         dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx) +
         dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords, sizeIdx);

   if(symbolFormat == DmtxFormatMosaic)
      message->codeSize *= 3;

   message->code = (unsigned char *)calloc(message->codeSize, sizeof(unsigned char));
   if(message->code == NULL) {
      perror("Calloc failed");
      dmtxMessageDestroy(&message);
      return NULL;
   }

   /* XXX not sure if this is the right place or even the right approach.
      Trying to allocate memory for the decoded data stream and will
      initially assume that decoded data will not be larger than 2x encoded data */
   message->outputSize = sizeof(unsigned char) * message->codeSize * 10;
   message->output = (unsigned char *)calloc(message->outputSize, sizeof(unsigned char));
   if(message->output == NULL) {
      perror("Calloc failed");
      dmtxMessageDestroy(&message);
      return NULL;
   }

   return message;
}

/**
 * \brief  Free memory previously allocated for message
 * \param  message
 * \return void
 */
extern DmtxPassFail
dmtxMessageDestroy(DmtxMessage **msg)
{
   if(msg == NULL || *msg == NULL)
      return DmtxFail;

   if((*msg)->array != NULL)
      free((*msg)->array);

   if((*msg)->code != NULL)
      free((*msg)->code);

   if((*msg)->output != NULL)
      free((*msg)->output);

   free(*msg);

   *msg = NULL;

   return DmtxPass;
}
