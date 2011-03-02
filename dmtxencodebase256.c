/*
 * libdmtx - Data Matrix Encoding/Decoding Library
 *
 * Copyright (C) 2011 Mike Laughton
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact: mike@dragonflylogic.com
 */

/* $Id$ */

#include "dmtx.h"
#include "dmtxstatic.h"

/**
 *
 *
 */
static void
EncodeNextChunkBase256(DmtxEncodeStream *stream)
{
   DmtxByte value;

   if(StreamInputHasNext(stream))
   {
      value = StreamInputAdvanceNext(stream); CHKERR;
      EncodeValueBase256(stream, value); CHKERR;
   }
}

/**
 *
 *
 */
static void
EncodeValueBase256(DmtxEncodeStream *stream, DmtxByte value)
{
   CHKSCHEME(DmtxSchemeBase256);

   StreamOutputChainAppend(stream, Randomize255State2(value, stream->output.length + 1)); CHKERR;
   stream->outputChainValueCount++;

   UpdateBase256ChainHeader(stream, DmtxUndefined); CHKERR;
}

/**
 * check remaining symbol capacity and remaining codewords
 * if the chain can finish perfectly at the end of symbol data words there is a
 * special one-byte length header value that can be used (i think ... read the
 * spec again before commiting to anything)
 */
static void
CompleteIfDoneBase256(DmtxEncodeStream *stream, int requestedSizeIdx)
{
   int sizeIdx;
   int headerByteCount, outputLength, symbolRemaining;

   if(!StreamInputHasNext(stream))
   {
      headerByteCount = stream->outputChainWordCount - stream->outputChainValueCount;
      assert(headerByteCount == 1 || headerByteCount == 2);

      /* Check for special case where every last symbol word is used */
      if(headerByteCount == 2)
      {
         /* Find symbol size as if headerByteCount was only 1 */
         outputLength = stream->output.length - 1;
         sizeIdx = FindSymbolSize(outputLength, requestedSizeIdx); /* No CHKSIZE */
         if(sizeIdx != DmtxUndefined)
         {
            symbolRemaining = GetRemainingSymbolCapacity(outputLength, sizeIdx);

            if(symbolRemaining == 0)
            {
               /* Perfect fit -- complete encoding */
               UpdateBase256ChainHeader(stream, sizeIdx); CHKERR;
               StreamMarkComplete(stream, sizeIdx);
               return;
            }
         }
      }

      /* Normal case */
      sizeIdx = FindSymbolSize(stream->output.length, requestedSizeIdx); CHKSIZE;
      EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchImplicit);
      PadRemainingInAscii(stream, sizeIdx);
      StreamMarkComplete(stream, sizeIdx);
   }
}

/**
 *
 *
 */
static void
UpdateBase256ChainHeader(DmtxEncodeStream *stream, int perfectSizeIdx)
{
   int headerIndex;
   int outputLength;
   int headerByteCount;
   int symbolDataWords;
   DmtxBoolean perfectFit;
   DmtxByte headerValue0;
   DmtxByte headerValue1;

   headerIndex = stream->output.length - stream->outputChainWordCount;
   outputLength = stream->outputChainValueCount;
   headerByteCount = stream->outputChainWordCount - stream->outputChainValueCount;
   perfectFit = (perfectSizeIdx == DmtxUndefined) ? DmtxFalse : DmtxTrue;

   /*
    * If requested perfect fit verify symbol capacity against output length
    */

   if(perfectFit)
   {
      symbolDataWords = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, perfectSizeIdx);
      if(symbolDataWords != stream->output.length)
      {
         StreamMarkFatal(stream, 1);
         return;
      }
   }

   /*
    * Adjust header to hold correct number of bytes, not worrying about the
    * values held there until below. Note: Header bytes are not considered
    * scheme "values" so we can insert or remove them without updating the
    * outputChainValueCount.
    */

   if(headerByteCount == 0 && stream->outputChainWordCount == 0)
   {
      /* No output words written yet -- insert single header byte */
      StreamOutputChainAppend(stream, 0); CHKERR;
      headerByteCount++;
   }
   else if(!perfectFit && headerByteCount == 1 && outputLength > 249)
   {
      /* Beyond 249 bytes requires a second header byte */
      StreamOutputChainInsertFirst(stream); CHKERR; /* XXX just a stub right now */
      headerByteCount++;
   }
   else if(perfectFit && headerByteCount == 2)
   {
      /* Encoding to exact end of symbol only requires single byte */
      StreamOutputChainRemoveFirst(stream); CHKERR; /* XXX just a stub right now */
      headerByteCount--;
   }

   /*
    * Encode header byte(s) with current length
    */

   if(!perfectFit && headerByteCount == 1 && outputLength <= 249)
   {
      /* Normal condition for chain length < 250 bytes */
      headerValue0 = Randomize255State2(outputLength, headerIndex + 1);
      StreamOutputSet(stream, headerIndex, headerValue0); CHKERR;
   }
   else if(!perfectFit && headerByteCount == 2 && outputLength > 249)
   {
      /* Normal condition for chain length >= 250 bytes */
      headerValue0 = Randomize255State2(outputLength/250 + 249, headerIndex + 1);
      StreamOutputSet(stream, headerIndex, headerValue0); CHKERR;

      headerValue1 = Randomize255State2(outputLength%250, headerIndex + 2);
      StreamOutputSet(stream, headerIndex + 1, headerValue1); CHKERR;
   }
   else if(perfectFit && headerByteCount == 1)
   {
      /* Special condition when Base 256 stays in effect to end of symbol */
      headerValue0 = Randomize255State2(0, headerIndex + 1); /* XXX replace magic value 0? */
      StreamOutputSet(stream, headerIndex, headerValue0); CHKERR;
   }
   else
   {
      StreamMarkFatal(stream, 1); /* XXX error */
      return;
   }
}
