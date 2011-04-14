/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2011 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in parent directory for full terms of
 * use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxencodec40textx12.c
 */

#undef CHKERR
#define CHKERR { if(stream->status != DmtxStatusEncoding) { return; } }

#undef CHKSIZE
#define CHKSIZE { if(sizeIdx == DmtxUndefined) { StreamMarkInvalid(stream, 1); return; } }

#undef CHKPASS
#define CHKPASS { if(passFail == DmtxFail) { StreamMarkFatal(stream, 1); return; } }

#undef RETURN_IF_FAIL
#define RETURN_IF_FAIL { if(*passFail == DmtxFail) return; }

/**
 *
 *
 */
static void
EncodeNextChunkCTX(DmtxEncodeStream *stream, int sizeIdxRequest)
{
   DmtxPassFail passFail;
   DmtxByte inputValue;
   DmtxByte valueListStorage[6];
   DmtxByteList valueList = dmtxByteListBuild(valueListStorage, sizeof(valueListStorage));

   while(StreamInputHasNext(stream))
   {
      inputValue = StreamInputAdvanceNext(stream); CHKERR;

      /* Expand next input value into up to 4 CTX values and add to valueList */
      PushCTXValues(&valueList, inputValue, stream->currentScheme, &passFail);
      if(passFail == DmtxFail)
      {
         StreamMarkInvalid(stream, 1);
         return;
      }

      /* If there at least 3 CTX values available encode them to output */
      while(valueList.length >= 3)
      {
         EncodeValuesCTX(stream, &valueList); CHKERR;
         ShiftValueListBy3(&valueList, &passFail); CHKPASS;
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
      CompleteIfDonePartialCTX(stream, &valueList, sizeIdxRequest); CHKERR;
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

   if(valueList->length < 3)
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
CompleteIfDoneCTX(DmtxEncodeStream *stream, int sizeIdxRequest)
{
   int sizeIdx;
   int symbolRemaining;

   if(!StreamInputHasNext(stream))
   {
      sizeIdx = FindSymbolSize(stream->output->length, sizeIdxRequest); CHKSIZE;
      symbolRemaining = GetRemainingSymbolCapacity(stream->output->length, sizeIdx);

      if(symbolRemaining == 0)
      {
         /* End of symbol condition (a) -- Perfect fit */
         StreamMarkComplete(stream, sizeIdx);
      }
      else
      {
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit); CHKERR;
         PadRemainingInAscii(stream, sizeIdx);
         StreamMarkComplete(stream, sizeIdx);
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
 *               -       -  UNLATCH (finish ASCII)
 */
static void
CompleteIfDonePartialCTX(DmtxEncodeStream *stream, DmtxByteList *valueList, int sizeIdxRequest)
{
   int i;
   int sizeIdx1, sizeIdx2;
   int symbolRemaining1, symbolRemaining2;
   DmtxPassFail passFail;
   DmtxByte inputValue;
   DmtxByte outputTmpStorage[4];
   DmtxByteList outputTmp = dmtxByteListBuild(outputTmpStorage, sizeof(outputTmpStorage));

   if(!IsCTX(stream->currentScheme))
   {
      StreamMarkFatal(stream, 1);
      return;
   }

   /* Should have exactly one or two input values left */
   assert(valueList->length == 1 || valueList->length == 2);

   sizeIdx1 = FindSymbolSize(stream->output->length + 1, sizeIdxRequest);
   sizeIdx2 = FindSymbolSize(stream->output->length + 2, sizeIdxRequest);

   symbolRemaining1 = GetRemainingSymbolCapacity(stream->output->length, sizeIdx1);
   symbolRemaining2 = GetRemainingSymbolCapacity(stream->output->length, sizeIdx2);

   if(valueList->length == 2 && symbolRemaining2 == 2)
   {
      /* End of symbol condition (b) -- Use Shift1 to pad final list value */
      dmtxByteListPush(valueList, DmtxValueCTXShift1, &passFail); CHKPASS;
      EncodeValuesCTX(stream, valueList); CHKERR;
      StreamMarkComplete(stream, sizeIdx2);
   }
   else
   {
      /*
       * Rollback progress of previously consumed input value(s) since ASCII
       * encoder will be used to finish the symbol. 2 rollbacks are needed if
       * valueList holds 2 data words (i.e., not shifts or upper shifts).
       */

      StreamInputAdvancePrev(stream); CHKERR;
      inputValue = StreamInputPeekNext(stream); CHKERR;

      /* Test-encode most recently consumed input value to C40/Text/X12 */
      PushCTXValues(&outputTmp, inputValue, stream->currentScheme, &passFail);
      if(valueList->length == 2 && outputTmp.length == 1)
      {
         StreamInputAdvancePrev(stream); CHKERR;
      }

      /* Re-use outputTmp to hold ASCII representation of 1-2 input values */
      /* XXX Refactor how the DmtxByteList is passed back here */
      outputTmp = EncodeTmpRemainingInAscii(stream, outputTmpStorage,
            sizeof(outputTmpStorage), &passFail);

      if(passFail == DmtxFail)
      {
         StreamMarkFatal(stream, 1 /* should never happen */);
         return;
      }

      if(outputTmp.length == 1 && symbolRemaining1 == 1)
      {
         /* End of symbol condition (d) */
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchImplicit); CHKERR;
         EncodeValueAscii(stream, outputTmp.b[0]); CHKERR;

         /* Register progress since encoding happened outside normal path */
         stream->inputNext = stream->input->length;
         StreamMarkComplete(stream, sizeIdx1);
      }
      else
      {
         /* Finish in ASCII (c) */
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit); CHKERR;
         for(i = 0; i < outputTmp.length; i++)
            EncodeValueAscii(stream, outputTmp.b[i]); CHKERR;

         sizeIdx1 = FindSymbolSize(stream->output->length, sizeIdxRequest);
         PadRemainingInAscii(stream, sizeIdx1);

         /* Register progress since encoding happened outside normal path */
         stream->inputNext = stream->input->length;
         StreamMarkComplete(stream, sizeIdx1);
      }
   }
}

/**
 *
 *
 */
static void
PushCTXValues(DmtxByteList *valueList, DmtxByte inputValue, int targetScheme,
      DmtxPassFail *passFail)
{
   assert(valueList->length <= 2);

   /* Handle extended ASCII with Upper Shift character */
   if(inputValue > 127)
   {
      if(targetScheme == DmtxSchemeX12)
      {
         *passFail = DmtxFail;
         return;
      }
      else
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift2, passFail); RETURN_IF_FAIL;
         dmtxByteListPush(valueList, 30, passFail); RETURN_IF_FAIL;
         inputValue -= 128;
      }
   }

   /* Handle all other characters according to encodation scheme */
   if(targetScheme == DmtxSchemeX12)
   {
      if(inputValue == 13)
      {
         dmtxByteListPush(valueList, 0, passFail); RETURN_IF_FAIL;
      }
      else if(inputValue == 42)
      {
         dmtxByteListPush(valueList, 1, passFail); RETURN_IF_FAIL;
      }
      else if(inputValue == 62)
      {
         dmtxByteListPush(valueList, 2, passFail); RETURN_IF_FAIL;
      }
      else if(inputValue == 32)
      {
         dmtxByteListPush(valueList, 3, passFail); RETURN_IF_FAIL;
      }
      else if(inputValue >= 48 && inputValue <= 57)
      {
         dmtxByteListPush(valueList, inputValue - 44, passFail); RETURN_IF_FAIL;
      }
      else if(inputValue >= 65 && inputValue <= 90)
      {
         dmtxByteListPush(valueList, inputValue - 51, passFail); RETURN_IF_FAIL;
      }
      else
      {
         *passFail = DmtxFail;
         return;
      }
   }
   else
   {
      /* targetScheme is C40 or Text */
      if(inputValue <= 31)
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift1, passFail); RETURN_IF_FAIL;
         dmtxByteListPush(valueList, inputValue, passFail); RETURN_IF_FAIL;
      }
      else if(inputValue == 32)
      {
         dmtxByteListPush(valueList, 3, passFail); RETURN_IF_FAIL;
      }
      else if(inputValue <= 47)
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift2, passFail); RETURN_IF_FAIL;
         dmtxByteListPush(valueList, inputValue - 33, passFail); RETURN_IF_FAIL;
      }
      else if(inputValue <= 57)
      {
         dmtxByteListPush(valueList, inputValue - 44, passFail); RETURN_IF_FAIL;
      }
      else if(inputValue <= 64)
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift2, passFail); RETURN_IF_FAIL;
         dmtxByteListPush(valueList, inputValue - 43, passFail); RETURN_IF_FAIL;
      }
      else if(inputValue <= 90 && targetScheme == DmtxSchemeC40)
      {
         dmtxByteListPush(valueList, inputValue - 51, passFail); RETURN_IF_FAIL;
      }
      else if(inputValue <= 90 && targetScheme == DmtxSchemeText)
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift3, passFail); RETURN_IF_FAIL;
         dmtxByteListPush(valueList, inputValue - 64, passFail); RETURN_IF_FAIL;
      }
      else if(inputValue <= 95)
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift2, passFail); RETURN_IF_FAIL;
         dmtxByteListPush(valueList, inputValue - 69, passFail); RETURN_IF_FAIL;
      }
      else if(inputValue == 96 && targetScheme == DmtxSchemeText)
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift3, passFail); RETURN_IF_FAIL;
         dmtxByteListPush(valueList, 0, passFail); RETURN_IF_FAIL;
      }
      else if(inputValue <= 122 && targetScheme == DmtxSchemeText)
      {
         dmtxByteListPush(valueList, inputValue - 83, passFail); RETURN_IF_FAIL;
      }
      else if(inputValue <= 127)
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift3, passFail); RETURN_IF_FAIL;
         dmtxByteListPush(valueList, inputValue - 96, passFail); RETURN_IF_FAIL;
      }
      else
      {
         *passFail = DmtxFail;
         return;
      }
   }

   *passFail = DmtxPass;
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

/**
 *
 *
 */
static void
ShiftValueListBy3(DmtxByteList *list, DmtxPassFail *passFail)
{
   int i;

   /* Shift values */
   for(i = 0; i < list->length - 3; i++)
      list->b[i] = list->b[i+3];

   /* Shorten list by 3 (or less) */
   for(i = 0; i < 3; i++)
   {
      dmtxByteListPop(list, passFail);
      if(*passFail == DmtxFail)
         return;

      if(list->length == 0)
         break;
   }

   *passFail = DmtxPass;
}
