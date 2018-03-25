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
 * \file dmtxencodeedifact.c
 * \brief Edifact encoding rules
 */

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
      /* Check for FNC1 character, which needs to be sent in ASCII */
      value = StreamInputPeekNext(stream); CHKERR;
      if(stream->fnc1 != DmtxUndefined && (int)value == stream->fnc1) {
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit); CHKERR;

         StreamInputAdvanceNext(stream); CHKERR;
         AppendValueAscii(stream, DmtxValueFNC1); CHKERR;
         return;
      }

      value = StreamInputAdvanceNext(stream); CHKERR;
      AppendValueEdifact(stream, value); CHKERR;
   }
}

/**
 *
 *
 */
static void
AppendValueEdifact(DmtxEncodeStream *stream, DmtxByte value)
{
   DmtxByte edifactValue, previousOutput;

   CHKSCHEME(DmtxSchemeEdifact);

   /*
    *  TODO: KECA -> korean, circles
    *  TODO: UNOX -> ISO-2022-JP
    *  TODO: and so on
    */
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
 * Complete EDIFACT encoding if it matches a known end-of-symbol condition.
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
CompleteIfDoneEdifact(DmtxEncodeStream *stream, int sizeIdxRequest)
{
   int i;
   int sizeIdx;
   int symbolRemaining;
   DmtxBoolean cleanBoundary;
   DmtxPassFail passFail;
   DmtxByte outputTmpStorage[3];
   DmtxByteList outputTmp;

   if(stream->status == DmtxStatusComplete)
      return;

   /*
    * If we just completed a triplet (cleanBoundary), 1 or 2 symbol codewords
    * remain, and our remaining inputs (if any) represented in ASCII would fit
    * in the remaining space, encode them in ASCII with an implicit unlatch.
    */

   cleanBoundary = (stream->outputChainValueCount % 4 == 0) ? DmtxTrue : DmtxFalse;

   if(cleanBoundary == DmtxTrue)
   {
      /* Encode up to 3 codewords to a temporary stream */
      outputTmp = EncodeTmpRemainingInAscii(stream, outputTmpStorage,
            sizeof(outputTmpStorage), &passFail);

      if(passFail == DmtxFail)
      {
         StreamMarkFatal(stream, DmtxErrorUnknown);
         return;
      }

      if(outputTmp.length < 3)
      {
         /* Find minimum symbol size for projected length */
         sizeIdx = FindSymbolSize(stream->output->length + outputTmp.length, sizeIdxRequest); CHKSIZE;

         /* Find remaining capacity over current length */
         symbolRemaining = GetRemainingSymbolCapacity(stream->output->length, sizeIdx); CHKERR;

         if(symbolRemaining < 3 && outputTmp.length <= symbolRemaining)
         {
            EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchImplicit); CHKERR;

            for(i = 0; i < outputTmp.length; i++)
            {
               AppendValueAscii(stream, outputTmp.b[i]); CHKERR;
            }

            /* Register progress since encoding happened outside normal path */
            stream->inputNext = stream->input->length;

            /* Pad remaining if necessary */
            PadRemainingInAscii(stream, sizeIdx); CHKERR;
            StreamMarkComplete(stream, sizeIdx);
            return;
         }
      }
   }

   if(!StreamInputHasNext(stream))
   {
      sizeIdx = FindSymbolSize(stream->output->length, sizeIdxRequest); CHKSIZE;
      symbolRemaining = GetRemainingSymbolCapacity(stream->output->length, sizeIdx); CHKERR;

      /* Explicit unlatch required unless on clean boundary and full symbol */
      if(cleanBoundary == DmtxFalse || symbolRemaining > 0)
      {
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit); CHKERR;
         sizeIdx = FindSymbolSize(stream->output->length, sizeIdxRequest); CHKSIZE;
         PadRemainingInAscii(stream, sizeIdx); CHKERR;
      }

      StreamMarkComplete(stream, sizeIdx);
   }
}
