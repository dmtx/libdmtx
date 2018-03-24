/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2011 Mike Laughton. All rights reserved.
 * Copyright 2012-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxencodec40textx12.c
 * \brief C40/Text/X12 encoding rules
 */

#undef CHKERR
#define CHKERR { if(stream->status != DmtxStatusEncoding) { return; } }

#undef CHKSIZE
#define CHKSIZE { if(sizeIdx == DmtxUndefined) { StreamMarkInvalid(stream, DmtxErrorUnknown); return; } }

#undef CHKPASS
#define CHKPASS { if(passFail == DmtxFail) { StreamMarkFatal(stream, DmtxErrorUnknown); return; } }

#undef RETURN_IF_FAIL
#define RETURN_IF_FAIL { if(*passFail == DmtxFail) return; }

/**
 *
 *
 */
static void
EncodeNextChunkCTX(DmtxEncodeStream *stream, int sizeIdxRequest)
{
   int i;
   DmtxPassFail passFail;
   DmtxByte inputValue;
   DmtxByte valueListStorage[6];
   DmtxByteList valueList = dmtxByteListBuild(valueListStorage, sizeof(valueListStorage));

   while(StreamInputHasNext(stream))
   {
      if(stream->currentScheme == DmtxSchemeX12)
      {
          /* Check for FNC1 character */
          inputValue = StreamInputPeekNext(stream); CHKERR;
          if(stream->fnc1 != DmtxUndefined && (int)inputValue == stream->fnc1) {
             /* X12 does not allow partial blocks, resend last 1 or 2 as ASCII */
             EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit); CHKERR;
             for(i = 0; i < valueList.length % 3; i++)
                StreamInputAdvancePrev(stream); CHKERR;

             while(i) {
                inputValue = StreamInputAdvanceNext(stream); CHKERR;
                AppendValueAscii(stream, inputValue + 1); CHKERR;
                i--;
             }

             StreamInputAdvanceNext(stream); CHKERR;
             AppendValueAscii(stream, DmtxValueFNC1); CHKERR;
             return;
          }
      }
      inputValue = StreamInputAdvanceNext(stream); CHKERR;

      /* Expand next input value into up to 4 CTX values and add to valueList */
      PushCTXValues(&valueList, inputValue, stream->currentScheme, &passFail, stream->fnc1);
      if(passFail == DmtxFail)
      {
         /* XXX Perhaps PushCTXValues should return this error code */
         StreamMarkInvalid(stream, DmtxErrorUnsupportedCharacter);
         return;
      }

      /* If there at least 3 CTX values available encode them to output */
      while(valueList.length >= 3)
      {
         AppendValuesCTX(stream, &valueList); CHKERR;
         ShiftValueListBy3(&valueList, &passFail); CHKPASS;
      }

      /* Finished on byte boundary -- done with current chunk */
      if(valueList.length == 0)
         break;
   }

   /*
    * Special case: If all input values have been consumed and 1 or 2 unwritten
    * C40/Text/X12 values remain, finish encoding the symbol according to the
    * established end-of-symbol conditions.
    */
   if(!StreamInputHasNext(stream) && valueList.length > 0)
   {
      if(stream->currentScheme == DmtxSchemeX12)
      {
         CompletePartialX12(stream, &valueList, sizeIdxRequest); CHKERR;
      }
      else
      {
         CompletePartialC40Text(stream, &valueList, sizeIdxRequest); CHKERR;
      }
   }
}

/**
 *
 *
 */
static void
AppendValuesCTX(DmtxEncodeStream *stream, DmtxByteList *valueList)
{
   int pairValue;
   DmtxByte cw0, cw1;

   if(!IsCTX(stream->currentScheme))
   {
      StreamMarkFatal(stream, DmtxErrorUnexpectedScheme);
      return;
   }

   if(valueList->length < 3)
   {
      StreamMarkFatal(stream, DmtxErrorIncompleteValueList);
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
AppendUnlatchCTX(DmtxEncodeStream *stream)
{
   if(!IsCTX(stream->currentScheme))
   {
      StreamMarkFatal(stream, DmtxErrorUnexpectedScheme);
      return;
   }

   /* Verify we are on byte boundary */
   if(stream->outputChainValueCount % 3 != 0)
   {
      StreamMarkInvalid(stream, DmtxErrorNotOnByteBoundary);
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

   if(stream->status == DmtxStatusComplete)
      return;

   if(!StreamInputHasNext(stream))
   {
      sizeIdx = FindSymbolSize(stream->output->length, sizeIdxRequest); CHKSIZE;
      symbolRemaining = GetRemainingSymbolCapacity(stream->output->length, sizeIdx);

      if(symbolRemaining > 0)
      {
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit); CHKERR;
         PadRemainingInAscii(stream, sizeIdx);
      }

      StreamMarkComplete(stream, sizeIdx);
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
CompletePartialC40Text(DmtxEncodeStream *stream, DmtxByteList *valueList, int sizeIdxRequest)
{
   int i;
   int sizeIdx1, sizeIdx2;
   int symbolRemaining1, symbolRemaining2;
   DmtxPassFail passFail;
   DmtxByte inputValue;
   DmtxByte outputTmpStorage[4];
   DmtxByteList outputTmp = dmtxByteListBuild(outputTmpStorage, sizeof(outputTmpStorage));

   if(stream->currentScheme != DmtxSchemeC40 && stream->currentScheme != DmtxSchemeText)
   {
      StreamMarkFatal(stream, DmtxErrorUnexpectedScheme);
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
      AppendValuesCTX(stream, valueList); CHKERR;
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
      PushCTXValues(&outputTmp, inputValue, stream->currentScheme, &passFail, stream->fnc1);
      if(valueList->length == 2 && outputTmp.length == 1)
         StreamInputAdvancePrev(stream); CHKERR;

      /* Re-use outputTmp to hold ASCII representation of 1-2 input values */
      /* XXX Refactor how the DmtxByteList is passed back here */
      outputTmp = EncodeTmpRemainingInAscii(stream, outputTmpStorage,
            sizeof(outputTmpStorage), &passFail);

      if(passFail == DmtxFail)
      {
         StreamMarkFatal(stream, DmtxErrorUnknown);
         return;
      }

      if(outputTmp.length == 1 && symbolRemaining1 == 1)
      {
         /* End of symbol condition (d) */
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchImplicit); CHKERR;
         AppendValueAscii(stream, outputTmp.b[0]); CHKERR;

         /* Register progress since encoding happened outside normal path */
         stream->inputNext = stream->input->length;
         StreamMarkComplete(stream, sizeIdx1);
      }
      else
      {
         /* Finish in ASCII (c) */
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit); CHKERR;
         for(i = 0; i < outputTmp.length; i++)
            AppendValueAscii(stream, outputTmp.b[i]); CHKERR;

         sizeIdx1 = FindSymbolSize(stream->output->length, sizeIdxRequest);
         PadRemainingInAscii(stream, sizeIdx1);

         /* Register progress since encoding happened outside normal path */
         stream->inputNext = stream->input->length;
         StreamMarkComplete(stream, sizeIdx1);
      }
   }
}

/**
 * Partial chunks are not valid in X12. Encode using ASCII instead, using
 * an implied unlatch if there is exactly one ascii codeword and one symbol
 * codeword remaining. Otherwise use explicit unlatch.
 */
static void
CompletePartialX12(DmtxEncodeStream *stream, DmtxByteList *valueList, int sizeIdxRequest)
{
   int i;
   int sizeIdx;
   int symbolRemaining;
   DmtxPassFail passFail;
   DmtxByte outputTmpStorage[2];
   DmtxByteList outputTmp;

   if(stream->currentScheme != DmtxSchemeX12)
   {
      StreamMarkFatal(stream, DmtxErrorUnexpectedScheme);
      return;
   }

   /* Should have exactly one or two input values left */
   assert(valueList->length == 1 || valueList->length == 2);

   /* Roll back input progress */
   for(i = 0; i < valueList->length; i++)
   {
      StreamInputAdvancePrev(stream); CHKERR;
   }

   /* Encode up to 2 codewords to a temporary stream */
   outputTmp = EncodeTmpRemainingInAscii(stream, outputTmpStorage,
         sizeof(outputTmpStorage), &passFail);

   sizeIdx = FindSymbolSize(stream->output->length + 1, sizeIdxRequest);
   symbolRemaining = GetRemainingSymbolCapacity(stream->output->length, sizeIdx);

   if(outputTmp.length == 1 && symbolRemaining == 1)
   {
      /* End of symbol condition (XXX) */
      EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchImplicit); CHKERR;
      AppendValueAscii(stream, outputTmp.b[0]); CHKERR;

      /* Register progress since encoding happened outside normal path */
      stream->inputNext = stream->input->length;
      StreamMarkComplete(stream, sizeIdx);
   }
   else
   {
      /* Finish in ASCII (XXX) */
      EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit); CHKERR;
      for(i = 0; i < outputTmp.length; i++)
         AppendValueAscii(stream, outputTmp.b[i]); CHKERR;

      sizeIdx = FindSymbolSize(stream->output->length, sizeIdxRequest);
      PadRemainingInAscii(stream, sizeIdx);

      /* Register progress since encoding happened outside normal path */
      stream->inputNext = stream->input->length;
      StreamMarkComplete(stream, sizeIdx);
   }
}

/**
 * Return DmtxTrue 1 or 2 X12 values remain, otherwise DmtxFalse
 */
static DmtxBoolean
PartialX12ChunkRemains(DmtxEncodeStream *stream)
{
   DmtxEncodeStream streamTmp;
   DmtxByte inputValue;
   DmtxByte valueListStorage[6];
   DmtxByteList valueList = dmtxByteListBuild(valueListStorage, sizeof(valueListStorage));
   DmtxPassFail passFail;

   /* Create temporary copy of stream to track test input progress */
   streamTmp = *stream;
   streamTmp.currentScheme = DmtxSchemeX12;
   streamTmp.outputChainValueCount = 0;
   streamTmp.outputChainWordCount = 0;
   streamTmp.reason = NULL;
   streamTmp.sizeIdx = DmtxUndefined;
   streamTmp.status = DmtxStatusEncoding;
   streamTmp.output = NULL;

   while(StreamInputHasNext(&streamTmp))
   {
      inputValue = StreamInputAdvanceNext(&streamTmp);
      if(stream->status != DmtxStatusEncoding)
      {
         StreamMarkInvalid(stream, DmtxErrorUnknown);
         return DmtxFalse;
      }

      /* Expand next input value into up to 4 CTX values and add to valueList */
      PushCTXValues(&valueList, inputValue, streamTmp.currentScheme, &passFail, stream->fnc1);
      if(passFail == DmtxFail)
      {
         StreamMarkInvalid(stream, DmtxErrorUnknown);
         return DmtxFalse;
      }

      /* Not a final partial chunk */
      if(valueList.length >= 3)
         return DmtxFalse;
   }

   return (valueList.length == 0) ? DmtxFalse : DmtxTrue;
}

/**
 *
 *
 */
static void
PushCTXValues(DmtxByteList *valueList, DmtxByte inputValue, int targetScheme,
      DmtxPassFail *passFail, int fnc1)
{
   assert(valueList->length <= 2);

   /* Handle extended ASCII with Upper Shift character */
   if(inputValue > 127 && (fnc1 == DmtxUndefined || (int)inputValue != fnc1))
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

      /* Check for FNC1 character */
      if(fnc1 != DmtxUndefined && (int)inputValue == fnc1)
      {
         dmtxByteListPush(valueList, DmtxValueCTXShift2, passFail); RETURN_IF_FAIL;
         dmtxByteListPush(valueList, 27, passFail); RETURN_IF_FAIL; /* C40 version of FNC1 */
      }
      else if(inputValue <= 31)
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
