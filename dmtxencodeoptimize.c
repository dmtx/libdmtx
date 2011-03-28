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
   int i;
   DmtxEncodeStream stream[SchemeStateCount];
   DmtxByte outputStorage[SchemeStateCount][4096];
   DmtxByteList output[SchemeStateCount];

   for(i = 0; i < SchemeStateCount; i++)
   {
      output[i] = dmtxByteListBuild(outputStorage[i], sizeof(outputStorage[i]));
      stream[i] = StreamInit(input, &(output[i]));
   }

   /* Continue encoding until complete */
   for(;;)
   {
      /*
       * find the most efficient way to reach the 18 possible states of input+1
       * starting from the 18 states at current input progress
       */
/*
      for(state = 0; state < SchemeStateCount; state++)
      {
         statePrevious = GetPreviousSchemeState(state);

         if(statePrevious == DmtxUndefined)
         {
            stream.encodeBest(stream);
         }
         else
         {
            stream[state] = stream[statePrevious];
            stream[state].inputNext++; (decide how best to handle)
            stream[state].outputChainValueCount (decide how best to handle)
            stream[state].outputChainWordCount (decide how best to handle)
         }
      }
*/
   }

   return DmtxUndefined;
}

/**
 *
 *
 */
/*
encodeBest(stream, sizeIdxRequest)
{
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
}
*/

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
