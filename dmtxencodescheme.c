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

/**
 * this file deals with encoding logic (scheme rules)
 *
 * In the context of this file:
 *
 * A "word" refers to a full codeword byte that is appended to the encoded
 * output.
 *
 * A "value" refers to any scheme value being appended to the output stream,
 * regardless of how many bytes are used to represent it. Examples:
 *
 *   ASCII:                   1 value  in  1 word
 *   ASCII (digits):          2 values in  1 word
 *   C40/Text/X12:            3 values in  2 words
 *   C40/Text/X12 (unlatch):  1 values in  1 word
 *   EDIFACT:                 4 values in  3 words
 *   Base 256:                1 value  in  1 word
 *
 *   Shifts count as values, so outputChainValueCount will reflect these
 *
 *   Latches are not counted as values. Technically, unlatches are counted, but
 *   the chain lengths are immediately reset after unlatching so it never truly
 *   affects anything.
 *
 *   Not decided yet whether Base256 length byte(s) are included as values ...
 *   if we say 'no' then outputChainWordCount will always cleanly match the
 *   length byte(s) at the beginning of the Base256 chain
 *
 * A "chunk" refers to the minimum grouping of values in a schema that must be
 * encoded together.
 *
 *   ASCII:                   1 value  and 1 word  in 1 chunk
 *   ASCII (digits):          2 values and 1 word  in 1 chunk (optional)
 *   C40/Text/X12:            3 values and 2 words in 1 chunk
 *   C40/Text/X12 (unlatch):  1 value  and 1 word  in 1 chunk
 *   EDIFACT:                 1 value  and 1 word  in 1 chunk
 *   Base 256:                1 value  and 1 word  in 1 chunk
 *
 * Each scheme implements 3 equivalent functions:
 *   - EncodeValue[Scheme]
 *   - EncodeNextChunk[Scheme]
 *   - CompleteIfDone[Scheme]
 *
 * The master function EncodeNextChunk() (no Scheme in the name) knows which
 * scheme-specific implementations to call based on the stream's current
 * encodation scheme.
 *
 * It's important that EncodeNextChunk[Scheme] not call CompleteIfDone[Scheme]
 * directly because some parts of the logic might want to encode a stream
 * without allowing the padding and other extra logic that can occur when an
 * end-of-symbol condition is triggered.
 */

#include "dmtx.h"
#include "dmtxstatic.h"

/* CHKERR should follow any call that might alter stream status */
#define CHKERR { if(stream->status != DmtxStatusEncoding) { return; } }

/**
 *
 *
 */
static DmtxPassFail
EncodeSingleScheme2(DmtxEncodeStream *stream, DmtxScheme targetScheme, int requestedSizeIdx)
{
   /* XXX should be setting error instead of assert? */
   assert(stream->currentScheme == DmtxSchemeAscii);

   while(stream->status == DmtxStatusEncoding)
      EncodeNextChunk(stream, targetScheme, requestedSizeIdx);

   if(stream->status != DmtxStatusComplete || StreamInputHasNext(stream))
      return DmtxFail; /* throw out an error too? */

   return DmtxPass;
}

/**
 * This function distributes work to the equivalent scheme-specific
 * implementation.
 *
 * Each of these functions will encode the next symbol input word, and in some
 * cases this requires additional input words to be encoded as well.
 */
static void
EncodeNextChunk(DmtxEncodeStream *stream, DmtxScheme targetScheme, int requestedSizeIdx)
{
   /* Change to target scheme if necessary */
   if(stream->currentScheme != targetScheme)
   {
      EncodeChangeScheme(stream, targetScheme, DmtxUnlatchExplicit); CHKERR;
      assert(stream->currentScheme == targetScheme);
   }

   /* Explicit polymorphism */
   switch(stream->currentScheme)
   {
      case DmtxSchemeAscii:
         EncodeNextChunkAscii(stream); CHKERR;
         CompleteIfDoneAscii(stream, requestedSizeIdx); CHKERR;
         break;
      case DmtxSchemeC40:
      case DmtxSchemeText:
      case DmtxSchemeX12:
         EncodeNextChunkC40TextX12(stream); CHKERR;
         CompleteIfDoneC40TextX12(stream); CHKERR;
         break;
      case DmtxSchemeEdifact:
         EncodeNextChunkEdifact(stream); CHKERR;
         CompleteIfDoneEdifact(stream, requestedSizeIdx); CHKERR;
         break;
      case DmtxSchemeBase256:
         EncodeNextChunkBase256(stream); CHKERR;
         CompleteIfDoneBase256(stream, requestedSizeIdx); CHKERR;
         break;
      default:
         StreamMarkFatal(stream, 1 /* unknown */);
         break;
   }
}

/**
 *
 *
 */
static void
EncodeChangeScheme(DmtxEncodeStream *stream, DmtxScheme targetScheme, int unlatchType)
{
   /* Nothing to do */
   if(stream->currentScheme == targetScheme)
      return;

   /* Every latch must go through ASCII */
   switch(stream->currentScheme)
   {
      case DmtxSchemeC40:
      case DmtxSchemeText:
      case DmtxSchemeX12:
         if(unlatchType == DmtxUnlatchExplicit)
         {
            EncodeUnlatchC40TextX12(stream); CHKERR;
         }
         break;
      case DmtxSchemeEdifact:
         if(unlatchType == DmtxUnlatchExplicit)
         {
            EncodeValueEdifact(stream, DmtxValueEdifactUnlatch); CHKERR;
         }
         break;
      default:
         /* Nothing to do for ASCII or Base 256 */
         assert(stream->currentScheme == DmtxSchemeAscii ||
               stream->currentScheme == DmtxSchemeBase256);
         break;
   }
   stream->currentScheme = DmtxSchemeAscii;

   /* Anything other than ASCII (the default) requires a latch */
   switch(targetScheme)
   {
      case DmtxSchemeC40:
         EncodeValueAscii(stream, DmtxValueC40Latch); CHKERR;
         break;
      case DmtxSchemeText:
         EncodeValueAscii(stream, DmtxValueTextLatch); CHKERR;
         break;
      case DmtxSchemeX12:
         EncodeValueAscii(stream, DmtxValueX12Latch); CHKERR;
         break;
      case DmtxSchemeEdifact:
         EncodeValueAscii(stream, DmtxValueEdifactLatch); CHKERR;
         break;
      case DmtxSchemeBase256:
         EncodeValueAscii(stream, DmtxValueBase256Latch); CHKERR;
/*
         // Write temporary field length (0 indicates remainder of symbol)
         // need to switch schemes first?
         EncodeValueBase256(stream, 0); CHKERR;
*/
         break;
      default:
         /* Nothing to do for ASCII */
         assert(targetScheme == DmtxSchemeAscii);
         break;
   }
   stream->currentScheme = targetScheme;

   /* Reset new chain length to zero */
   stream->outputChainValueCount = 0;
   stream->outputChainWordCount = 0;
}

/**
 *
 *
 */
static void
EncodeValueAscii(DmtxEncodeStream *stream, DmtxByte value)
{
   /* XXX should be setting error instead of assert? */
   assert(stream->currentScheme == DmtxSchemeAscii);

   StreamOutputChainAppend(stream, value); CHKERR;
   stream->outputChainValueCount++;
}

/**
 *
 *
 */
static void
EncodeNextChunkAscii(DmtxEncodeStream *stream)
{
   DmtxBoolean v1set;
   DmtxByte v0, v1;

   if(StreamInputHasNext(stream))
   {
      v0 = StreamInputAdvanceNext(stream); CHKERR;

      if(StreamInputHasNext(stream))
      {
         v1 = StreamInputPeekNext(stream); CHKERR;
         v1set = DmtxTrue;
      }
      else
      {
         v1 = 0;
         v1set = DmtxFalse;
      }

      if(ISDIGIT(v0) && v1set && ISDIGIT(v1))
      {
         /* Two adjacent digit chars */
         StreamInputAdvanceNext(stream); CHKERR; /* Make the peek progress official */
         EncodeValueAscii(stream, 10 * (v0 - '0') + (v1 - '0') + 130); CHKERR;
      }
      else
      {
         if(v0 < 128)
         {
            /* Regular ASCII char */
            EncodeValueAscii(stream, v0 + 1); CHKERR;
         }
         else
         {
            /* Extended ASCII char */
            EncodeValueAscii(stream, DmtxValueAsciiUpperShift); CHKERR;
            EncodeValueAscii(stream, v0 - 127); CHKERR;
         }
      }
   }
}

/**
 *
 *
 */
static void
CompleteIfDoneAscii(DmtxEncodeStream *stream, int requestedSizeIdx)
{
   int sizeIdx;

   if(!StreamInputHasNext(stream))
   {
      sizeIdx = FindSymbolSize(stream->output.length, requestedSizeIdx);
      if(sizeIdx == DmtxUndefined)
      {
         StreamMarkInvalid(stream, 1 /* insufficient capacity */);
         return;
      }
      PadRemainingInAscii(stream, sizeIdx); CHKERR;
      StreamMarkComplete(stream, sizeIdx);
   }
}

/**
 *
 *
 */
static void
EncodeUnlatchC40TextX12(DmtxEncodeStream *stream)
{
   /* XXX should be setting error instead of assert? */
   assert(stream->currentScheme == DmtxSchemeC40 ||
         stream->currentScheme == DmtxSchemeText ||
         stream->currentScheme == DmtxSchemeX12);

   /* XXX Verify we are on byte boundary too */
/*
   if(stream->outputChainValueCount % 3 != 0)
   {
      streamMarkInvalid(stream, 1 not on byte boundary);
      return;
   }
*/
   StreamOutputChainAppend(stream, DmtxValueC40TextX12Unlatch); CHKERR;

   stream->outputChainValueCount++;
}

/**
 *
 *
 */
static void
EncodeValuesC40TextX12(DmtxEncodeStream *stream, DmtxByteList values)
{
   DmtxByte cw0, cw1;

   /* XXX should be setting error instead of assert? */
   assert(stream->currentScheme == DmtxSchemeC40 ||
         stream->currentScheme == DmtxSchemeText ||
         stream->currentScheme == DmtxSchemeX12);

   /* combine (v0,v1,v2) into (cw0,cw1) */
   cw0 = cw1 = 0; /* temporary */

   /* Append 2 codewords */
   StreamOutputChainAppend(stream, cw0); CHKERR;
   StreamOutputChainAppend(stream, cw1); CHKERR;

   /* Update count for 3 encoded values */
   stream->outputChainValueCount += 3;
}

/**
 *
 *
 */
static void
EncodeNextChunkC40TextX12(DmtxEncodeStream *stream)
{
/*
   DmtxByte inputNext;
   DmtxByte valueStorage[4];
   DmtxByteList values = dmtxByteListBuild(valueStorage, sizeof(valueStorage));

   while(streamInputHasNext(stream))
   {
      inputNext = StreamInputAdvanceNext(stream)); CHKERR;
      valuesNext = GetC40TextX12Values(valueNext)

      dmtxByteListPush(values, valuesNext);
      if(values.length >= 3)
      {
         values = EncodeValuesC40TextX12(stream, values); CHKERR;
      }
      xxx maybe EncodeValuesC40TextX12() returns the unused values, if any

      xxx need to account for end of symbol here too, right?

      if(values.length == 0)
         break;
   }
*/

   /* AppendChunkC40TextX12(stream, v0, v1, v2) */
}

/**
 * Complete C40/Text/X12 encoding if matching a known end-of-symbol condition.
 *
 *   Term  Trip   Symbol  Codeword
 *   Cond  Size   Remain  Sequence
 *   ----  -----  ------  -------------------
 *    (d)      1       1  Special case
 *    (c)      1       2  Special case
 *             1       3  UNLATCH ASCII PAD
 *             1       4  UNLATCH ASCII PAD PAD
 *    (b)      2       2  Special case
 *             2       3  UNLATCH ASCII ASCII
 *             2       4  UNLATCH ASCII ASCII PAD
 *    (a)      3       2  Special case
 *             3       3  C40 C40 UNLATCH
 *             3       4  C40 C40 UNLATCH PAD
 *
 *             1       0  Need bigger symbol
 *             2       0  Need bigger symbol
 *             2       1  Need bigger symbol
 *             3       0  Need bigger symbol
 *             3       1  Need bigger symbol
 */
static void
CompleteIfDoneC40TextX12(DmtxEncodeStream *stream)
{
}

/**
 *
 *
 */
static void
EncodeValueEdifact(DmtxEncodeStream *stream, DmtxByte value)
{
   DmtxByte edifactValue, previousOutput;

   /* XXX should be setting error instead of assert? */
   assert(stream->currentScheme == DmtxSchemeEdifact);

   if(value < 32 || value > 94)
   {
      StreamMarkInvalid(stream, DmtxChannelUnsupportedChar);
      return;
   }

   edifactValue = (value & 0x3f) << 2;

   switch(stream->outputChainValueCount % 4)
   {
      case 0:
         StreamOutputChainAppend(stream, edifactValue); CHKERR;
         break;
      case 1:
         previousOutput = StreamOutputChainRemoveLast(stream); CHKERR;
         StreamOutputChainAppend(stream, previousOutput | (edifactValue >> 6)); CHKERR;
         StreamOutputChainAppend(stream, edifactValue << 2); CHKERR;
         break;
      case 2:
         previousOutput = StreamOutputChainRemoveLast(stream); CHKERR;
         StreamOutputChainAppend(stream, previousOutput | (edifactValue >> 4)); CHKERR;
         StreamOutputChainAppend(stream, edifactValue << 4); CHKERR;
         break;
      case 3:
         previousOutput = StreamOutputChainRemoveLast(stream); CHKERR;
         StreamOutputChainAppend(stream, previousOutput | (edifactValue >> 2)); CHKERR;
         break;
   }

   stream->outputChainValueCount++;
}

/**
 *
 *
 */
static void
EncodeNextChunkEdifact(DmtxEncodeStream *stream)
{
   DmtxByte value;

   if(StreamInputHasNext(stream))
   {
      value = StreamInputAdvanceNext(stream); CHKERR;
      EncodeValueEdifact(stream, value); CHKERR;
   }
}

/**
 * Complete EDIFACT encoding if matching a known end-of-symbol condition.
 *
 *   Term  Clean  Symbol  ASCII   Codeword
 *   Cond  Bound  Remain  Remain  Sequence
 *   ----  -----  ------  ------  -----------
 *    (a)      Y       0       0  [none]
 *    (b)      Y       1       0  PAD
 *    (c)      Y       1       1  ASCII
 *    (d)      Y       2       0  PAD PAD
 *    (e)      Y       2       1  ASCII PAD
 *    (f)      Y       2       2  ASCII ASCII
 *             -       -       0  UNLATCH
 *
 * If not matching any of the above, continue without doing anything.
 */
static void
CompleteIfDoneEdifact(DmtxEncodeStream *stream, int requestedSizeIdx)
{
   int i;
   int sizeIdx;
   int symbolRemaining;
   DmtxBoolean cleanBoundary;
   DmtxPassFail passFail;
   DmtxByte outputTmpStorage[3];
   DmtxByteList outputTmp;

   /* Check if sitting on a clean byte boundary */
   cleanBoundary = (stream->outputChainValueCount % 4 == 0) ? DmtxTrue : DmtxFalse;

   /* Find symbol's remaining capacity based on current length */
   sizeIdx = FindSymbolSize(stream->output.length, requestedSizeIdx);
   if(sizeIdx == DmtxUndefined) /* XXX this is repeated elsewhere */
   {
      StreamMarkInvalid(stream, 1 /* Insufficient capacity */);
      return;
   }
   symbolRemaining = GetRemainingSymbolCapacity(stream->output.length, sizeIdx); CHKERR;

   if(!StreamInputHasNext(stream))
   {
      /* Explicit unlatch required unless on clean boundary and full symbol */
      if(cleanBoundary == DmtxFalse || symbolRemaining > 0)
      {
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit); CHKERR;
         PadRemainingInAscii(stream, sizeIdx); CHKERR;
      }

      StreamMarkComplete(stream, sizeIdx);
   }
   else
   {
      /**
       * Allow encoder to write up to 3 additional codewords to a temporary
       * stream. If it finishes in 1 or 2, is a known end-of-symbol condition.
       */
      outputTmp = EncodeTmpRemainingInAscii(stream, outputTmpStorage,
            sizeof(outputTmpStorage), &passFail);

      if(passFail == DmtxFail || outputTmp.length > symbolRemaining)
         return; /* Doesn't fit, continue encoding */

      if(cleanBoundary && (outputTmp.length == 1 || outputTmp.length == 2))
      {
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchImplicit); CHKERR;

         for(i = 0; i < outputTmp.length; i++)
         {
            EncodeValueAscii(stream, outputTmp.b[i]); CHKERR;
         }

         /* Register input progress since we encoded outside normal stream */
         stream->inputNext = stream->input.length;

         PadRemainingInAscii(stream, sizeIdx); CHKERR;
         StreamMarkComplete(stream, sizeIdx);
      }
   }
}

/**
 *
 *
 */
static void
EncodeValueBase256(DmtxEncodeStream *stream, DmtxByte value)
{
   int headerByteCount;

   /* XXX should be setting error instead of assert? */
   assert(stream->currentScheme == DmtxSchemeBase256);

   /* Append new codeword to end of chain */
   StreamOutputChainAppend(stream, Randomize255State2(value, stream->output.length)); CHKERR;

   /* If we hit the threshold the length header requires a second byte */
   if(stream->outputChainWordCount == 250)
   {
      StreamOutputChainInsertFirst(stream); CHKERR;
      /* Chain value count doesn't require update because the length header is
       * not considered a chain value */
   }

   /* Update header byte(s) with new length */
   headerByteCount = stream->outputChainWordCount - stream->outputChainValueCount;
   if(headerByteCount == 1)
   {
      /* XXX actually need to use Randomize255State() instead */
      StreamOutputChainSet(stream, 0, stream->outputChainWordCount); CHKERR;
   }
   else
   {
      /* XXX actually need to use Randomize255State() instead */
      StreamOutputChainSet(stream, 0, stream->outputChainWordCount/250 + 249); CHKERR;
      StreamOutputChainSet(stream, 1, stream->outputChainWordCount%250); CHKERR;
   }

   stream->outputChainValueCount++;
}

/**
 *
 *
 */
static void
EncodeNextChunkBase256(DmtxEncodeStream *stream)
{
   int headerByteCount;
   DmtxByte value;

   if(StreamInputHasNext(stream))
   {
      headerByteCount = stream->outputChainWordCount - stream->outputChainValueCount;
      value = StreamInputAdvanceNext(stream); CHKERR;
      EncodeValueBase256(stream, value); CHKERR;
   }
}

/**
 *
 *
 */
static void
CompleteIfDoneBase256(DmtxEncodeStream *stream, int requestedSizeIdx)
{
   int sizeIdx;
/*
   check remaining symbol capacity and remaining codewords
   if the chain can finish perfectly at the end of symbol data words there is a
   special one-byte length header value that can be used (i think ... read the
   spec again before commiting to anything)
*/
   if(!StreamInputHasNext(stream))
   {
/*
      headerLength = stream->outputChainWordCount - stream->outputChainValueCount;
      if(headerLength == 2)
      {
         // test as if encoded up to exact symbol capacity
         outputLength = stream->output.length - 1;
         sizeIdx = FindSymbolSize(outputLength, requestedSizeIdx);
         if(sizeIdx != DmtxUndefined)
         {
            symbolRemaining = GetRemainingSymbolCapacity(outputLength, sizeIdx);

            if(symbolRemaining == 0)
            {
               StreamOutputRemoveFirst(stream);
               StreamOutputSet(stream, 0, Randomize255(xyz)); // encode to end
               StreamMarkComplete(stream, sizeIdx);
               return;
            }
         }
      }
*/
      sizeIdx = FindSymbolSize(stream->output.length, requestedSizeIdx);
      if(sizeIdx == DmtxUndefined)
      {
         StreamMarkInvalid(stream, 1 /* Insufficient capacity */);
         return;
      }
/*
      capacity = dmtxSymbolGetProp(DmtxDataWords, sizeIdx);

      ChangeEncodingScheme(DmtxSchemeAscii, DmtxUnlatchImplicit);
      PadRemainingInAscii(stream, sizeIdx);
*/
      StreamMarkComplete(stream, sizeIdx);
   }
}

/**
 * \brief  Randomize 253 state
 * \param  codewordValue
 * \param  codewordPosition
 * \return Randomized value
 */
static DmtxByte
Randomize253State2(DmtxByte cwValue, int cwPosition)
{
   int pseudoRandom, tmp;

   pseudoRandom = ((149 * cwPosition) % 253) + 1;
   tmp = cwValue + pseudoRandom;
   if(tmp > 254)
      tmp -= 254;

   assert(tmp >= 0 && tmp < 256);

   return (DmtxByte)tmp;
}

/**
 * \brief  Randomize 255 state
 * \param  cwValue
 * \param  cwPosition
 * \return Randomized value
 */
static DmtxByte
Randomize255State2(DmtxByte cwValue, int cwPosition)
{
   int pseudoRandom, tmp;

   pseudoRandom = ((149 * cwPosition) % 255) + 1;
   tmp = cwValue + pseudoRandom;

   return (tmp <= 255) ? tmp : tmp - 256;
}

/**
 *
 *
 */
static int
GetRemainingSymbolCapacity(int outputLength, int sizeIdx)
{
   int dataCapacity;

   assert(sizeIdx != DmtxUndefined);

   dataCapacity = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx);

   return dataCapacity - outputLength;
}

/**
 * Can we just receive a length to pad here? I don't like receiving
 * requestedSizeIdx (or sizeIdx) this late in the game
 *
 */
static void
PadRemainingInAscii(DmtxEncodeStream *stream, int sizeIdx)
{
   int symbolRemaining;

   /* XXX replace this with a proper error later */
   assert(stream->currentScheme == DmtxSchemeAscii);

   assert(sizeIdx != DmtxUndefined);

   symbolRemaining = GetRemainingSymbolCapacity(stream->output.length, sizeIdx);

   /* First pad character is not randomized */
   if(symbolRemaining > 0)
   {
      StreamOutputChainAppend(stream, DmtxValueAsciiPad); CHKERR;
      symbolRemaining--;
   }

   /* All remaining pad characters are randomized based on character position */
   while(symbolRemaining > 0)
   {
      StreamOutputChainAppend(stream, Randomize253State2(DmtxValueAsciiPad,
            stream->output.length)); CHKERR;
      symbolRemaining--;
   }
}

/**
 *
 *
 */
static DmtxByteList
EncodeTmpRemainingInAscii(DmtxEncodeStream *stream, DmtxByte *storage, int capacity, DmtxPassFail *passFail)
{
   DmtxEncodeStream streamAscii;

   /* Create temporary copy of stream that writes to storage */
   streamAscii = *stream;
   streamAscii.currentScheme = DmtxSchemeAscii;
   streamAscii.outputChainValueCount = 0;
   streamAscii.outputChainWordCount = 0;
   streamAscii.reason = DmtxUndefined;
   streamAscii.sizeIdx = DmtxUndefined;
   streamAscii.status = DmtxStatusEncoding;
   streamAscii.output = dmtxByteListBuild(storage, capacity);

   while(dmtxByteListHasCapacity(&(streamAscii.output)))
   {
      /* Do not call CHKERR here because we don't want to return */
      if(StreamInputHasNext(&streamAscii))
         EncodeNextChunkAscii(&streamAscii);
      else
         break;
   }

   /**
    * We stopped encoding before attempting to write beyond output boundary so
    * any stream errors are truly unexpected. The passFail status indicates
    * whether output.length can be trusted by the calling function.
    */

   *passFail = (streamAscii.status == DmtxStatusEncoding) ? DmtxPass : DmtxFail;

   return streamAscii.output;
}
