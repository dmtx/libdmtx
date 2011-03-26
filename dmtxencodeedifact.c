/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2011 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in parent directory for full terms of
 * use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxencodeedifact.c
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
      value = StreamInputAdvanceNext(stream); CHKERR;
      EncodeValueEdifact(stream, value); CHKERR;
   }
}

/**
 *
 *
 */
static void
EncodeValueEdifact(DmtxEncodeStream *stream, DmtxByte value)
{
   DmtxByte edifactValue, previousOutput;

   CHKSCHEME(DmtxSchemeEdifact);

   if(value < 31 || value > 94)
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
   sizeIdx = FindSymbolSize(stream->output.length, requestedSizeIdx); CHKSIZE;
   symbolRemaining = GetRemainingSymbolCapacity(stream->output.length, sizeIdx); CHKERR;

   if(!StreamInputHasNext(stream))
   {
      /* Explicit unlatch required unless on clean boundary and full symbol */
      if(cleanBoundary == DmtxFalse || symbolRemaining > 0)
      {
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit); CHKERR;
         sizeIdx = FindSymbolSize(stream->output.length, requestedSizeIdx); CHKSIZE;
         PadRemainingInAscii(stream, sizeIdx); CHKERR;
      }

      StreamMarkComplete(stream, sizeIdx);
   }
   else
   {
      /*
       * Allow encoder to write up to 3 additional codewords to a temporary
       * stream. If it finishes in 1 or 2 it is a known end-of-symbol condition.
       */
      outputTmp = EncodeTmpRemainingInAscii(stream, outputTmpStorage,
            sizeof(outputTmpStorage), &passFail);

      if(passFail == DmtxFail || outputTmp.length > symbolRemaining)
         return; /* Doesn't fit -- continue encoding */

      if(cleanBoundary && (outputTmp.length == 1 || outputTmp.length == 2))
      {
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchImplicit); CHKERR;

         for(i = 0; i < outputTmp.length; i++)
         {
            EncodeValueAscii(stream, outputTmp.b[i]); CHKERR;
         }

         /* Register progress since encoding happened outside normal path */
         stream->inputNext = stream->input.length;

         /* Pad remaining if necessary */
         PadRemainingInAscii(stream, sizeIdx); CHKERR;

         StreamMarkComplete(stream, sizeIdx);
      }
   }
}
