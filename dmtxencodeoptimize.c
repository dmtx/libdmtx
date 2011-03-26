/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2011 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in parent directory for full terms of
 * use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxencodeoptimize.c
 */

/**
 *
 *
 */
static void
EncodeOptimizeBest(DmtxEncodeStream *stream, int requestedSizeIdx)
{
   CHKSCHEME(DmtxSchemeAscii);

   while(stream->status == DmtxStatusEncoding)
   {
      /* Use current scheme as target in single scheme mode */
      EncodeNextChunk(stream, stream->currentScheme, requestedSizeIdx);
   }

   if(StreamInputHasNext(stream))
      StreamMarkFatal(stream, 1 /* Found unexplained leftovers */);
/*
   DmtxEncodeStream streams[18];
   initialize streams

   // Encode input words until none are left
   for(;;)
   {
      // find the most efficient way to reach the 18 possible states of input+1
      // starting from the 18 states at current input progress
      for(targetScheme : 1 .. 18)
      {
         sourceScheme = getSourceScheme(targetScheme);

         if(sourceScheme != SEARCH_REQUIRED)
         {
            // starting from the 18 states at current input progress
            stream.advanceCopy(stream, sourceScheme);
         }
         else
         {
            stream.encodeBest(stream);
         }
      }
   }
*/
}

/**
 *
 *
 */
/*
encodeBest(stream, requestedSizeIdx)
{
   for(i = 0; i < 18; i++)
   {
      tmpEncodeStream[i] = stream[i];
      stopped = encodeNextWord(tmpEncodeStream[i], targetScheme, requestedSizeIdx);

      if(result is shortest)
      {
         best = i;
      }
   }

   return stream[best];
}
*/

/**
 *
 *
 */
/*
getSourceScheme(targetScheme)
{
   sourceScheme;

   switch(targetScheme)
   {
      case AsciiDigit2:
         sourceScheme = AsciiDigit1;
         break;
      case C40Digit2:
         sourceScheme = C40Digit1:
         break;
      case C40Digit3:
         sourceScheme = C40Digit2:
         break;
      case TextDigit2:
         sourceScheme = TextDigit1:
         break;
      case TextDigit3:
         sourceScheme = TextDigit2:
         break;
      case X12Digit2:
         sourceScheme = X12Digit1:
         break;
      case X12Digit3:
         sourceScheme = X12Digit2:
         break;
      case EdifactDigit2:
         sourceScheme = EdifactDigit1:
         break;
      case EdifactDigit3:
         sourceScheme = EdifactDigit2:
         break;
      case EdifactDigit4:
         sourceScheme = EdifactDigit3:
         break;
      default:
         sourceScheme = SEARCH_REQUIRED;
         break;
   }

   return sourceScheme;
}
*/
