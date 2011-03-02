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
EncodeNextChunkCTX(DmtxEncodeStream *stream, int requestedSizeIdx)
{
   DmtxPassFail passFail;
   DmtxByte inputValue;
   DmtxByte valueListStorage[4];
   DmtxByteList valueList = dmtxByteListBuild(valueListStorage, sizeof(valueListStorage));

   while(StreamInputHasNext(stream))
   {
      inputValue = StreamInputAdvanceNext(stream); CHKERR;
      /* XXX remember to account for upper shift (4 values each) */
      passFail = PushCTXValues(&valueList, inputValue, stream->currentScheme);

      /* remember to account for upper shift (4 values each) ... does this loop structure still work? */
      while(valueList.length >= 3)
      {
         EncodeValuesCTX(stream, &valueList); CHKERR;
/*       DmtxByteListRemoveFirst(valueList, 3); */
      }

      /* Finished on byte boundary -- done with current chunk */
      if(valueList.length == 0)
         break;
   }

   /*
    * Special case: If all input values have been consumed and 1 or 2 unwritten
    * C40/Text/X12 values remain, finish encoding the symbol in ASCII according
    * to the published end-of-symbol conditions.
    */
   if(!StreamInputHasNext(stream) && valueList.length > 0)
   {
      CompleteIfDonePartialCTX(stream, &valueList, requestedSizeIdx); CHKERR;
   }
}

/**
 *
 *
 */
static void
EncodeValuesCTX(DmtxEncodeStream *stream, DmtxByteList *valueList)
{
   int pairValue;
   DmtxByte cw0, cw1;

   if(!IsCTX(stream->currentScheme))
   {
      StreamMarkFatal(stream, 1);
      return;
   }

   if(valueList->length != 3)
   {
      StreamMarkFatal(stream, 1);
      return;
   }

   /* Build codewords from computed value */
   pairValue = (1600 * valueList->b[0]) + (40 * valueList->b[1]) + valueList->b[2] + 1;
   cw0 = pairValue / 256;
   cw1 = pairValue % 256;

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
EncodeUnlatchCTX(DmtxEncodeStream *stream)
{
   if(!IsCTX(stream->currentScheme))
   {
      StreamMarkFatal(stream, 1);
      return;
   }

   /* Verify we are on byte boundary */
   if(stream->outputChainValueCount % 3 != 0)
   {
      StreamMarkInvalid(stream, 1 /* not on byte boundary */);
      return;
   }

   StreamOutputChainAppend(stream, DmtxValueCTXUnlatch); CHKERR;

   stream->outputChainValueCount++;
}

/**
 * Complete C40/Text/X12 encoding if it matches a known end-of-symbol condition.
 *
 *   Term  Trip  Symbol  Codeword
 *   Cond  Size  Remain  Sequence
 *   ----  ----  ------  -----------------------
 *    (a)     3       2  Special case
 *            -       -  UNLATCH [PAD]
 */
static void
CompleteIfDoneCTX(DmtxEncodeStream *stream, int requestedSizeIdx)
{
   int sizeIdx;
   int symbolRemaining;

   sizeIdx = FindSymbolSize(stream->output.length, requestedSizeIdx); CHKSIZE;
   symbolRemaining = GetRemainingSymbolCapacity(stream->output.length, sizeIdx);

   if(!StreamInputHasNext(stream))
   {
      if(symbolRemaining == 0)
      {
         /* End of symbol condition (a) -- Perfect fit */
         StreamMarkComplete(stream, sizeIdx);
      }
      else
      {
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit); CHKERR;
      }
   }
}

/**
 * The remaining values can exist in 3 possible cases:
 *
 *   a) 1 C40/Text/X12 remaining == 1 data
 *   b) 2 C40/Text/X12 remaining == 1 shift + 1 data
 *   c) 2 C40/Text/X12 remaining == 1 data +  1 data
 *
 * To distinguish between cases (b) and (c), encode the final input value to
 * C40/Text/X12 in a temporary location and check the resulting length. If
 * it expands to multiple values it represents (b); otherwise it is (c). This
 * accounts for both shift and upper shift conditions.
 *
 * Note that in cases (a) and (c) the final C40/Text/X12 value encoded in the
 * previous chunk may have been a shift value, but this will be ignored by
 * the decoder due to the implicit shift to ASCII. <-- what if symbol is much
 * larger though?
 *
 *   Term    Value  Symbol  Codeword
 *   Cond    Count  Remain  Sequence
 *   ----  -------  ------  ------------------------
 *    (b)    C40 2       2  C40+C40+0
 *    (d)  ASCII 1       1  ASCII (implicit unlatch)
 *    (c)  ASCII 1       2  UNLATCH ASCII
 *               -       -  UNLATCH (continue ASCII)
 */
static void
CompleteIfDonePartialCTX(DmtxEncodeStream *stream, DmtxByteList *valueList, int requestedSizeIdx)
{
   int sizeIdx;
   int symbolRemaining;

   if(!IsCTX(stream->currentScheme))
   {
      StreamMarkFatal(stream, 1);
      return;
   }

   /* Should have exactly one or two input values left */
   assert(valueList->length == 1 || valueList->length == 2);

   sizeIdx = FindSymbolSize(stream->output.length, requestedSizeIdx); CHKSIZE;
   symbolRemaining = GetRemainingSymbolCapacity(stream->output.length, sizeIdx);

   if(valueList->length == 2 && symbolRemaining == 2)
   {
      /* End of symbol condition (b) -- Use Shift1 to pad final list value */
      dmtxByteListPush(valueList, DmtxValueCTXShift1);
      EncodeValuesCTX(stream, valueList); CHKERR;
      StreamMarkComplete(stream, sizeIdx);
   }
   else
   {
      /*
       * Rollback progress of previously consumed input value(s) since ASCII
       * encoder will be used to finish the symbol. 2 rollbacks are needed if
       * valueList holds 2 data words (i.e., not shift or upper shifts).
       */
/*
      StreamInputAdvancePrev(stream); CHKERR;

      // temporary re-encode most recently consumed input value to C40/Text/X12
      passFail = PushCTXValues(&tmp, inputValue));
      if(valueList.length == 2 && tmp.length > 1)
      {
         StreamInputAdvancePrev(stream); CHKERR;
      }

      ascii = encodeTmpRemainingToAscii(stream);
      if(ascii.length == 1 && symbolRemaining == 1)
      {
         // End of symbol condition (d)
         changeScheme(stream, DmtxSchemeAscii, DmtxUnlatchImplicit); CHKERR;
         EncodeValueAscii(stream, ascii.b[0]); CHKERR;
         StreamMarkComplete(stream, sizeIdx);
      }
      else
      {
         // Continue in ASCII (c)
         changeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit); CHKERR;
      }
*/
   }
}

/**
 * @brief  Convert 3 input values into 2 codewords for triplet-based schemes
 * @param  outputWords
 * @param  inputWord
 * @param  encScheme
 * @return Codeword count
 */
static DmtxPassFail
PushCTXValues(DmtxByteList *valueList, int inputValue, int targetScheme)
{
   /* Handle extended ASCII with Upper Shift character */
   if(inputValue > 127)
   {
      if(targetScheme == DmtxSchemeX12)
      {
         return DmtxFail; /* XXX shouldn't this be an error? */
      }
      else
      {
/*       passFail = dmtxByteListPush(valueList, DmtxValueCTXShift2); CHKPASS(passFail); */
         dmtxByteListPush(valueList, DmtxValueCTXShift2); /* XXX can throw error? */
         dmtxByteListPush(valueList, 30);
         inputValue -= 128;
      }
   }

   /* Handle all other characters according to encodation scheme */
   if(targetScheme == DmtxSchemeX12)
   {
      if(inputValue == 13)
         dmtxByteListPush(valueList, 0);
      else if(inputValue == 42)
         dmtxByteListPush(valueList, 1);
      else if(inputValue == 62)
         dmtxByteListPush(valueList, 2);
      else if(inputValue == 32)
         dmtxByteListPush(valueList, 3);
      else if(inputValue >= 48 && inputValue <= 57)
         dmtxByteListPush(valueList, inputValue - 44);
      else if(inputValue >= 65 && inputValue <= 90)
         dmtxByteListPush(valueList, inputValue - 51);
   }
   else
   {
      /* targetScheme is C40 or Text */
      if(inputValue <= 31)
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift1);
         dmtxByteListPush(valueList, inputValue);
      }
      else if(inputValue == 32)
      {
         dmtxByteListPush(valueList, 3);
      }
      else if(inputValue <= 47)
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift2);
         dmtxByteListPush(valueList, inputValue - 33);
      }
      else if(inputValue <= 57)
      {
         dmtxByteListPush(valueList, inputValue - 44);
      }
      else if(inputValue <= 64)
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift2);
         dmtxByteListPush(valueList, inputValue - 43);
      }
      else if(inputValue <= 90 && targetScheme == DmtxSchemeC40)
      {
         dmtxByteListPush(valueList, inputValue - 51);
      }
      else if(inputValue <= 90 && targetScheme == DmtxSchemeText)
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift3);
         dmtxByteListPush(valueList, inputValue - 64);
      }
      else if(inputValue <= 95)
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift2);
         dmtxByteListPush(valueList, inputValue - 69);
      }
      else if(inputValue == 96 && targetScheme == DmtxSchemeText)
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift3);
         dmtxByteListPush(valueList, 0);
      }
      else if(inputValue <= 122 && targetScheme == DmtxSchemeText)
      {
         dmtxByteListPush(valueList, inputValue - 83);
      }
      else if(inputValue <= 127)
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift3);
         dmtxByteListPush(valueList, inputValue - 96);
      }
   }

   return DmtxPass;
}

/**
 *
 *
 */
static DmtxBoolean
IsCTX(int scheme)
{
   DmtxBoolean isCTX;

   if(scheme == DmtxSchemeC40 || scheme == DmtxSchemeText || scheme == DmtxSchemeX12)
      isCTX = DmtxTrue;
   else
      isCTX = DmtxFalse;

   return isCTX;
}
