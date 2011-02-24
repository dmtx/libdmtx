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

/* $Id: dmtxencodescheme.c 1178 2011-01-27 17:59:43Z mblaughton $ */

/**
 * this file deals with encoding logic (scheme rules)
 */

#include "dmtx.h"
#include "dmtxstatic.h"

#define RETURN_IF_ERROR(s) if((s)->status != DmtxStatusEncoding) { return; }

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
      EncodeChangeScheme(stream, targetScheme, DmtxUnlatchExplicit);

   /* Poor man's polymorphism */
   switch(targetScheme)
   {
      case DmtxSchemeAscii:
         EncodeNextWordAscii(stream);
         CompleteIfDoneAscii(stream);
         break;
      case DmtxSchemeC40:
      case DmtxSchemeText:
      case DmtxSchemeX12:
         EncodeNextWordTriplet(stream, targetScheme);
         CompleteIfDoneTriplet(stream);
         break;
      case DmtxSchemeEdifact:
         EncodeNextWordEdifact(stream, requestedSizeIdx);
         CompleteIfDoneEdifact(stream, requestedSizeIdx);
         break;
      case DmtxSchemeBase256:
         EncodeNextWordBase256(stream);
         CompleteIfDoneBase256(stream);
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

   RETURN_IF_ERROR(stream);
   assert(stream->currentScheme == DmtxSchemeAscii);

   if(StreamInputHasNext(stream))
   {
      v0 = StreamInputAdvanceNext(stream);

      if(StreamInputHasNext(stream))
      {
         v1 = StreamInputPeekNext(stream);
         v1set = DmtxTrue;
      }
      else
      {
         v1 = 0;
         v1set = DmtxFalse;
      }

      /* Two adjacent digit chars */
      if(ISDIGIT(v0) && v1set && ISDIGIT(v1))
      {
         StreamInputAdvanceNext(stream); /* Make the peek progress official */
         StreamOutputChainAppend(stream, 10 * (v0 - '0') + (v1 - '0') + 130);
      }
      else
      {
         if(v0 < 128)
         {
            /* Regular ASCII char */
            StreamOutputChainAppend(stream, v0 + 1);
         }
         else
         {
            /* Extended ASCII char */
            StreamOutputChainAppend(stream, DmtxCharAsciiUpperShift);
            StreamOutputChainAppend(stream, v0 - 127);
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
   RETURN_IF_ERROR(stream);

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
   RETURN_IF_ERROR(stream);
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
   RETURN_IF_ERROR(stream);
}

/**
 *
 *
 */
static void
EncodeNextWordEdifact(DmtxEncodeStream *stream, int requestedSizeIdx)
{
   DmtxByte inputValue, edifactValue, previousOutput;

   RETURN_IF_ERROR(stream);
   assert(stream->currentScheme == DmtxSchemeEdifact);

   if(StreamInputHasNext(stream))
   {
      inputValue = StreamInputAdvanceNext(stream);

      if(inputValue < 32 || inputValue > 94)
      {
         StreamMarkInvalid(stream, DmtxChannelUnsupportedChar);
         return;
      }

      edifactValue = (inputValue & 0x3f) << 2;

      switch(stream->outputChainLength % 4)
      {
         case 0:
            StreamOutputChainAppend(stream, edifactValue);
            break;
         case 1:
            previousOutput = StreamOutputChainRemoveLast(stream);
            StreamOutputChainAppend(stream, previousOutput | (edifactValue >> 6));
            StreamOutputChainAppend(stream, edifactValue << 2);
            break;
         case 2:
            previousOutput = StreamOutputChainRemoveLast(stream);
            StreamOutputChainAppend(stream, previousOutput | (edifactValue >> 4));
            StreamOutputChainAppend(stream, edifactValue << 4);
            break;
         case 3:
            previousOutput = StreamOutputChainRemoveLast(stream);
            StreamOutputChainAppend(stream, previousOutput | (edifactValue >> 2));
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
   DmtxByteList ascii; /* instantiate to hold up to 2 bytes */

   RETURN_IF_ERROR(stream);

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

   /* Test if eligible to end in ASCII using one of the special cases */
/*
   passFail = EncodeRemainingInAscii(ascii, 2, stream); XXX won't work until this gets written
   if(passFail == DmtxFail || ascii.length > symbolRemaining)
      return; // Too big: keep going
*/

   if(ascii.length == 0)
   {
      /* Explicit unlatch required unless on clean byte boundary and full symbol */
      if(cleanBoundary == DmtxFalse || symbolRemaining > 0)
         EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit);

      StreamMarkComplete(stream);
   }
   else if(ascii.length < 3 && cleanBoundary == DmtxTrue)
   {
      EncodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchImplicit);

      for(i = 0; i < ascii.length; i++)
         StreamOutputChainAppend(stream, ascii.b[i]);

      stream->inputNext = stream->input.length;

      /* add padding too? */

      StreamMarkComplete(stream);
   }
}

/**
 *
 *
 */
static void
EncodeNextWordBase256(DmtxEncodeStream *stream)
{
   RETURN_IF_ERROR(stream);
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
   RETURN_IF_ERROR(stream);
}

/**
 *
 *
 */
static void
EncodeChangeScheme(DmtxEncodeStream *stream, DmtxScheme targetScheme, int unlatchType)
{
   assert(stream->currentScheme != targetScheme);

   /* Every latch must go through ASCII */
   switch(stream->currentScheme)
   {
      case DmtxSchemeC40:
      case DmtxSchemeText:
      case DmtxSchemeX12:
         if(unlatchType == DmtxUnlatchExplicit)
            StreamOutputChainAppend(stream, DmtxCharTripletUnlatch);
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
         StreamOutputChainAppend(stream, DmtxCharC40Latch);
         break;
      case DmtxSchemeText:
         StreamOutputChainAppend(stream, DmtxCharTextLatch);
         break;
      case DmtxSchemeX12:
         StreamOutputChainAppend(stream, DmtxCharX12Latch);
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
