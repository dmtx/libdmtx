/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2011 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in parent directory for full terms of
 * use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxencodeascii.c
 */

/**
 *
 * Add comment explaining the 3 modes this function can use, and why it's done this way
 * Simple single scheme encoding uses "Normal"
 * The optimizer needs to track "Expanded" and "Compact" streams separately, so they
 * are called explicitly.
 *
 *   Normal:   Automatically collapses 2 consecutive digits into one codeword
 *   Expanded: Uses a whole codeword to represent a digit (never collapses)
 *   Compact:  Collapses 2 digits into a single codeword or marks the stream
 *             invalid if either values are not digits
 * /param stream
 * /param option [Expanded|Compact|Normal]
 */
static void
EncodeNextChunkAscii(DmtxEncodeStream *stream, int option)
{
   DmtxByte v0, v1;
   DmtxBoolean compactDigits;

   if(StreamInputHasNext(stream))
   {
      v0 = StreamInputAdvanceNext(stream); CHKERR;

      if((option == DmtxEncodeCompact || option == DmtxEncodeNormal) &&
            StreamInputHasNext(stream))
      {
         v1 = StreamInputPeekNext(stream); CHKERR;
         compactDigits = (ISDIGIT(v0) && ISDIGIT(v1)) ? DmtxTrue : DmtxFalse;
      }
      else /* option == DmtxEncodeExpanded */
      {
         v1 = 0;
         compactDigits = DmtxFalse;
      }

      if(compactDigits == DmtxTrue)
      {
         /* Two adjacent digit chars: Make peek progress official and encode */
         StreamInputAdvanceNext(stream); CHKERR;
         EncodeValueAscii(stream, 10 * (v0-'0') + (v1-'0') + 130); CHKERR;
      }
      else if(option == DmtxEncodeCompact)
      {
         /* Can't compact non-digits */
         StreamMarkInvalid(stream, 1 /* can't compact */);
      }
      else
      {
         /* Encode single ASCII char */
         if(v0 < 128)
         {
            /* Regular ASCII */
            EncodeValueAscii(stream, v0 + 1); CHKERR;
         }
         else
         {
            /* Extended ASCII */
            EncodeValueAscii(stream, DmtxValueAsciiUpperShift); CHKERR;
            EncodeValueAscii(stream, v0 - 127); CHKERR;
         }
      }
   }
}

/**
 * this code is separated from EncodeNextChunkAscii() because it needs to be
 * called directly elsewhere
 *
 */
static void
EncodeValueAscii(DmtxEncodeStream *stream, DmtxByte value)
{
   CHKSCHEME(DmtxSchemeAscii);

   StreamOutputChainAppend(stream, value); CHKERR;
   stream->outputChainValueCount++;
}

/**
 *
 *
 */
static void
CompleteIfDoneAscii(DmtxEncodeStream *stream, int sizeIdxRequest)
{
   int sizeIdx;

   if(!StreamInputHasNext(stream))
   {
      sizeIdx = FindSymbolSize(stream->output->length, sizeIdxRequest); CHKSIZE;
      PadRemainingInAscii(stream, sizeIdx); CHKERR;
      StreamMarkComplete(stream, sizeIdx);
   }
}

/**
 * Can we just receive a length to pad here? I don't like receiving
 * sizeIdxRequest (or sizeIdx) this late in the game
 *
 */
static void
PadRemainingInAscii(DmtxEncodeStream *stream, int sizeIdx)
{
   int symbolRemaining;
   DmtxByte padValue;

   CHKSCHEME(DmtxSchemeAscii);
   CHKSIZE;

   symbolRemaining = GetRemainingSymbolCapacity(stream->output->length, sizeIdx);

   /* First pad character is not randomized */
   if(symbolRemaining > 0)
   {
      padValue = DmtxValueAsciiPad;
      StreamOutputChainAppend(stream, padValue); CHKERR;
      symbolRemaining--;
   }

   /* All remaining pad characters are randomized based on character position */
   while(symbolRemaining > 0)
   {
      padValue = Randomize253State(DmtxValueAsciiPad, stream->output->length + 1);
      StreamOutputChainAppend(stream, padValue); CHKERR;
      symbolRemaining--;
   }
}

/**
 *
 * consider receiving instantiated DmtxByteList instead of the output components
 */
static DmtxByteList
EncodeTmpRemainingInAscii(DmtxEncodeStream *stream, DmtxByte *storage,
      int capacity, DmtxPassFail *passFail)
{
   DmtxEncodeStream streamAscii;
   DmtxByteList output = dmtxByteListBuild(storage, capacity);

   /* Create temporary copy of stream that writes to storage */

   streamAscii = *stream;
   streamAscii.currentScheme = DmtxSchemeAscii;
   streamAscii.outputChainValueCount = 0;
   streamAscii.outputChainWordCount = 0;
   streamAscii.reason = DmtxUndefined;
   streamAscii.sizeIdx = DmtxUndefined;
   streamAscii.status = DmtxStatusEncoding;
   streamAscii.output = &output;

   while(dmtxByteListHasCapacity(streamAscii.output))
   {
      if(StreamInputHasNext(&streamAscii))
         EncodeNextChunkAscii(&streamAscii, DmtxEncodeNormal); /* No CHKERR */
      else
         break;
   }

   /*
    * We stopped encoding before attempting to write beyond output boundary so
    * any stream errors are truly unexpected. The passFail status indicates
    * whether output.length can be trusted by the calling function.
    */

   *passFail = (streamAscii.status == DmtxStatusEncoding) ? DmtxPass : DmtxFail;

   return output;
}

/**
 * \brief  Randomize 253 state
 * \param  codewordValue
 * \param  codewordPosition
 * \return Randomized value
 */
static DmtxByte
Randomize253State(DmtxByte cwValue, int cwPosition)
{
   int pseudoRandom, tmp;

   pseudoRandom = ((149 * cwPosition) % 253) + 1;
   tmp = cwValue + pseudoRandom;
   if(tmp > 254)
      tmp -= 254;

   assert(tmp >= 0 && tmp < 256);

   return (DmtxByte)tmp;
}
