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

enum SchemeState {
   Ascii,
   AsciiDigit1,
   AsciiDigit2,
   C40Digit1,
   C40Digit2,
   C40Digit3,
   TextDigit1,
   TextDigit2,
   TextDigit3,
   X12Digit1,
   X12Digit2,
   X12Digit3,
   EdifactDigit1,
   EdifactDigit2,
   EdifactDigit3,
   EdifactDigit4,
   Base256,
   SchemeStateCount
};

/**
 *
 *
 */
static int
EncodeOptimizeBest(DmtxByteList *input, DmtxByteList *outputBest, int sizeIdxRequest)
{
/*
   DmtxEncodeStream stream;

   stream = StreamInit(input, output);

   // Continue encoding until complete
   while(stream.status == DmtxStatusEncoding)
      EncodeNextChunk(&stream, stream.currentScheme, sizeIdxRequest);

   // Verify encoding completed successfully and all inputs were consumed
   if(stream.status != DmtxStatusComplete || StreamInputHasNext(&stream))
      return DmtxUndefined;

   return stream.sizeIdx;
*/
   int i, state, statePrevious;
   DmtxEncodeStream stream[SchemeStateCount];
   DmtxEncodeStream streamNext[SchemeStateCount];
   DmtxByte outputStorage[SchemeStateCount][4096];
   DmtxByteList output[SchemeStateCount];

   for(i = 0; i < SchemeStateCount; i++)
   {
      output[i] = dmtxByteListBuild(outputStorage[i], sizeof(outputStorage[i]));
      stream[i] = StreamInit(input, &(output[i]));
   }

   for(;;)
   {
      /* Find most efficient way to reach each state for the next input value */
      for(state = 0; state < SchemeStateCount; state++)
      {
         statePrevious = GetPreviousSchemeState(state);

         if(statePrevious != DmtxUndefined)
         {
            StreamCopy(&(streamNext[state]), &(stream[statePrevious]));
            streamNext[state].inputNext++;
         }
         else
         {
            StreamAdvanceFromBest(&(streamNext[state]), stream);
         }
      }

      /* Update "current" streams with results */
      for(state = 0; state < SchemeStateCount; state++)
         StreamCopy(&(stream[state]), &(streamNext[state]));

      /* Break condition -- Quit when all streams are either finished or invalid */
      if(1)
         break;
   }

   return DmtxUndefined;
}

/**
 *
 *
 */
static int
GetPreviousSchemeState(int stateCurrent)
{
   enum SchemeState statePrevious;

   switch(stateCurrent)
   {
      case AsciiDigit2:
         statePrevious = AsciiDigit1;
         break;
      case C40Digit2:
         statePrevious = C40Digit1;
         break;
      case C40Digit3:
         statePrevious = C40Digit2;
         break;
      case TextDigit2:
         statePrevious = TextDigit1;
         break;
      case TextDigit3:
         statePrevious = TextDigit2;
         break;
      case X12Digit2:
         statePrevious = X12Digit1;
         break;
      case X12Digit3:
         statePrevious = X12Digit2;
         break;
      case EdifactDigit2:
         statePrevious = EdifactDigit1;
         break;
      case EdifactDigit3:
         statePrevious = EdifactDigit2;
         break;
      case EdifactDigit4:
         statePrevious = EdifactDigit3;
         break;
      default:
         statePrevious = DmtxUndefined;
         break;
   }

   return statePrevious;
}

/**
 *
 *
 */
static void
StreamAdvanceFromBest(DmtxEncodeStream *streamNext, DmtxEncodeStream *stream)
{
/*
   for(i = 0; i < 18; i++)
   {
      tmpEncodeStream[i] = stream[i];
      stopped = encodeNextWord(tmpEncodeStream[i], targetScheme, sizeIdxRequest);

      if(result is shortest)
      {
         best = i;
      }
   }

   return stream[best];
*/
}
