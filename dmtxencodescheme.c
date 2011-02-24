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
 */

#include "dmtx.h"
#include "dmtxstatic.h"

/* CHKERR should follow any call that might change stream to bad status */
#define CHKERR { if(stream->status != DmtxStatusEncoding) { return; } }

/**
 *
 *
 */
static DmtxPassFail
EncodeSingleScheme2(DmtxEncodeStream *stream, DmtxScheme targetScheme, int requestedSizeIdx)
{
   assert(stream->currentScheme == DmtxSchemeAscii);

   while(stream->status == DmtxStatusEncoding)
      EncodeNextWord2(stream, targetScheme, requestedSizeIdx);

   if(stream->status != DmtxStatusComplete || StreamInputHasNext(stream))
      return DmtxFail; /* throw out an error too? */

   return DmtxPass;
}

/**
 * This function distributes work to the equivalent scheme-specific
 * implementation. This effectively creates a polymorphic function.
 *
 * Each of these functions will encode the next symbol input word, and in some
 * cases this requires additional input words to be encoded as well.
 */
static void
EncodeNextWord2(DmtxEncodeStream *stream, DmtxScheme targetScheme, int requestedSizeIdx)
{
   /* Change to target scheme if necessary */
   if(stream->currentScheme != targetScheme)
   {
      EncodeChangeScheme(stream, targetScheme, DmtxUnlatchExplicit); CHKERR;
   }

   /* Explicit polymorphism */
   switch(targetScheme)
   {
      case DmtxSchemeAscii:
         EncodeNextWordAscii(stream); CHKERR;
         CompleteIfDoneAscii(stream); CHKERR;
         break;
      case DmtxSchemeC40:
      case DmtxSchemeText:
      case DmtxSchemeX12:
         EncodeNextWordTriplet(stream, targetScheme); CHKERR;
         CompleteIfDoneTriplet(stream); CHKERR;
         break;
      case DmtxSchemeEdifact:
         EncodeNextWordEdifact(stream); CHKERR;
         CompleteIfDoneEdifact(stream, requestedSizeIdx); CHKERR;
         break;
      case DmtxSchemeBase256:
         EncodeNextWordBase256(stream); CHKERR;
         CompleteIfDoneBase256(stream); CHKERR;
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
EncodeNextWordAscii(DmtxEncodeStream *stream)
{
   DmtxBoolean v1set;
   DmtxByte v0, v1;

   assert(stream->currentScheme == DmtxSchemeAscii);

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
         StreamOutputChainAppend(stream, 10 * (v0 - '0') + (v1 - '0') + 130); CHKERR;
      }
      else
      {
         if(v0 < 128)
         {
            /* Regular ASCII char */
            StreamOutputChainAppend(stream, v0 + 1); CHKERR;
         }
         else
         {
            /* Extended ASCII char */
            StreamOutputChainAppend(stream, DmtxCharAsciiUpperShift); CHKERR;
            StreamOutputChainAppend(stream, v0 - 127); CHKERR;
         }
      }
   }
}

/**
 *
 *
 */
static void
CompleteIfDoneAscii(DmtxEncodeStream *stream)
{
   /* padding ? */

   if(!StreamInputHasNext(stream))
      StreamMarkComplete(stream);
}

/**
 *
 *
 */
static void
EncodeNextWordTriplet(DmtxEncodeStream *stream, DmtxScheme targetScheme)
{
   assert(stream->currentScheme == targetScheme);
   assert(targetScheme == DmtxSchemeC40 || targetScheme == DmtxSchemeText || targetScheme == DmtxSchemeX12);

   /* stuff goes here */
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
CompleteIfDoneTriplet(DmtxEncodeStream *stream)
{
}

/**
 *
 *
 */
static void
EncodeNextWordEdifact(DmtxEncodeStream *stream)
{
   DmtxByte inputValue, edifactValue, previousOutput;

   assert(stream->currentScheme == DmtxSchemeEdifact);

   if(StreamInputHasNext(stream))
   {
      inputValue = StreamInputAdvanceNext(stream); CHKERR;

      if(inputValue < 32 || inputValue > 94)
      {
         StreamMarkInvalid(stream, DmtxChannelUnsupportedChar);
         return;
      }

      edifactValue = (inputValue & 0x3f) << 2;

      switch(stream->outputChainLength % 4)
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
   DmtxByte outputAsciiStorage[2];
   DmtxByteList outputAscii;

   /* Check if sitting on a clean byte boundary */
   cleanBoundary = (stream->outputChainLength % 3 == 0) ? DmtxTrue : DmtxFalse;

   /* Find smallest symbol able to hold current encoded length */
   sizeIdx = FindSymbolSize(stream->output.length, requestedSizeIdx);

   /* Stop encoding: Unable to find matching symbol */
   if(sizeIdx == DmtxUndefined)
   {
      StreamMarkInvalid(stream, 1 /*DmtxInsufficientCapacity*/);
      return;
   }

   /* Find symbol's remaining capacity */
   symbolRemaining = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx) -
         stream->output.length;

   if(!StreamInputHasNext(stream))
   {
      /* Explicit unlatch required unless on clean boundary and full symbol */
      if(cleanBoundary == DmtxFalse || symbolRemaining > 0)
      {
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit); CHKERR;
         /* padding necessary? */
      }

      StreamMarkComplete(stream);
   }
   else
   {
      /* Test if eligible to end in ASCII using one of the special cases */
      assert(sizeof(outputAsciiStorage) >= 2);
      outputAscii = EncodeRemainingInAscii(stream, outputAsciiStorage, 2, &passFail);
      if(passFail == DmtxFail || outputAscii.length > symbolRemaining)
         return; /* Doesn't fit */

      if(cleanBoundary && (outputAscii.length == 1 || outputAscii.length == 2))
      {
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchImplicit); CHKERR;

         for(i = 0; i < outputAscii.length; i++)
         {
            StreamOutputChainAppend(stream, outputAscii.b[i]); CHKERR;
         }

         /* Register input progress since we encoded outside normal stream */
         stream->inputNext = stream->input.length;

         StreamMarkComplete(stream);
      }
   }
}

/**
 *
 *
 */
static void
EncodeNextWordBase256(DmtxEncodeStream *stream)
{
   assert(stream->currentScheme == DmtxSchemeBase256);

   /* stuff goes here */
}

/**
 *
 *
 */
static void
CompleteIfDoneBase256(DmtxEncodeStream *stream)
{
}

/**
 *
 *
 */
static void
EncodeChangeScheme(DmtxEncodeStream *stream, DmtxScheme targetScheme, int unlatchType)
{
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
            StreamOutputChainAppend(stream, DmtxCharTripletUnlatch); CHKERR;
         }
         break;
      case DmtxSchemeEdifact:
         if(unlatchType == DmtxUnlatchExplicit)
            ;
         /* something goes here */
         break;
      case DmtxSchemeBase256:
         /* something goes here */
         break;
      default:
         /* Nothing to do for ASCII */
         assert(stream->currentScheme == DmtxSchemeAscii);
         break;
   }

   /* Anything other than ASCII (the default) requires a latch */
   switch(targetScheme)
   {
      case DmtxSchemeC40:
         StreamOutputChainAppend(stream, DmtxCharC40Latch); CHKERR;
         break;
      case DmtxSchemeText:
         StreamOutputChainAppend(stream, DmtxCharTextLatch); CHKERR;
         break;
      case DmtxSchemeX12:
         StreamOutputChainAppend(stream, DmtxCharX12Latch); CHKERR;
         break;
      case DmtxSchemeEdifact:
         /* something goes here */
         break;
      case DmtxSchemeBase256:
         /* something goes here */
         break;
      default:
         /* Nothing to do for ASCII */
         assert(stream->currentScheme == DmtxSchemeAscii);
         break;
   }

   /* Reset current chain length to zero */
   stream->outputChainLength = 0;
   stream->currentScheme = targetScheme;
}

/**
 *
 *
 */
static DmtxByteList
EncodeRemainingInAscii(DmtxEncodeStream *stream, DmtxByte *storage, int capacity, DmtxPassFail *passFail)
{
   DmtxEncodeStream streamAscii;

   /* Create temporary copy of stream that writes to storage */
   streamAscii = *stream;
   streamAscii.currentScheme = DmtxSchemeAscii;
   streamAscii.outputChainLength = 0;
   streamAscii.reason = DmtxUndefined;
   streamAscii.status = DmtxStatusEncoding;
   streamAscii.output = dmtxByteListBuild(storage, capacity);

   while(streamAscii.status == DmtxStatusEncoding)
      EncodeNextWordAscii(&streamAscii);

   *passFail = (streamAscii.status == DmtxStatusComplete) ? DmtxPass : DmtxFail;

   return streamAscii.output;
}
