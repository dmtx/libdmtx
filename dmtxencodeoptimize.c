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
   int i, state, cameFrom;
   DmtxEncodeStream stream[SchemeStateCount];
   DmtxEncodeStream streamTmp[SchemeStateCount];
   DmtxByte outputStorage[SchemeStateCount][4096];
   DmtxByte outputTmpStorage[SchemeStateCount][4096];
   DmtxByteList output[SchemeStateCount];
   DmtxByteList outputTmp[SchemeStateCount];

   /* Initialize streams with their own output storage */
   for(i = 0; i < SchemeStateCount; i++)
   {
      output[i] = dmtxByteListBuild(outputStorage[i], sizeof(outputStorage[i]));
      outputTmp[i] = dmtxByteListBuild(outputTmpStorage[i], sizeof(outputTmpStorage[i]));
      stream[i] = StreamInit(input, &(output[i]));
      streamTmp[i] = StreamInit(input, &(outputTmp[i]));
   }

   /* Encode first chunk in each stream */
   for(i = 0; i < SchemeStateCount; i++)
      EncodeNextChunk(&(stream[i]), GetSchemeFromState(i), sizeIdxRequest);

   /* Continue encoding until all streams are complete */
   for(;;)
   {
      /* Break condition -- Quit when all streams are either finished or invalid */
      if(1)
         break;

      /* Find most efficient way to reach each state for the next input value */
      for(state = 0; state < SchemeStateCount; state++)
      {
         /* what about first iteration? */
         cameFrom = GetPreviousSchemeState(state);

         if(cameFrom == DmtxUndefined)
         {
            StreamAdvanceFromBest(&(streamTmp[state]), stream, sizeIdxRequest);
         }
         else
         {
            /* what about sizeidxrequest ... what if finished? */
            StreamCopy(&(streamTmp[state]), &(stream[cameFrom]));
            streamTmp[state].inputNext++;
         }
      }

      /* Update "current" streams with results */
      for(state = 0; state < SchemeStateCount; state++)
         StreamCopy(&(stream[state]), &(streamTmp[state]));
   }

   return DmtxUndefined;
}

/**
 *
 *
 */
static void
StreamAdvanceFromBest(DmtxEncodeStream *streamNext, DmtxEncodeStream *stream, int sizeIdxRequest)
{
   StreamCopy(streamNext, &(stream[Ascii]));

   EncodeNextChunk(streamNext, DmtxSchemeAscii, sizeIdxRequest);
/*
   for(i = 0; i < 18; i++)
   {
      tmpEncodeStream[i] = stream[i];
      stopped = encodeNextChunk(tmpEncodeStream[i], targetScheme, sizeIdxRequest);

      if(result is shortest)
      {
         best = i;
      }
   }
*/
}

/**
 *
 *
 */
static int
GetSchemeFromState(int state)
{
   DmtxScheme scheme;

   switch(state)
   {
      case Ascii:
      case AsciiDigit1:
      case AsciiDigit2:
         scheme = DmtxSchemeAscii;
         break;
      case C40Digit1:
      case C40Digit2:
      case C40Digit3:
         scheme = DmtxSchemeC40;
         break;
      case TextDigit1:
      case TextDigit2:
      case TextDigit3:
         scheme = DmtxSchemeText;
         break;
      case X12Digit1:
      case X12Digit2:
      case X12Digit3:
         scheme = DmtxSchemeX12;
         break;
      case EdifactDigit1:
      case EdifactDigit2:
      case EdifactDigit3:
      case EdifactDigit4:
         scheme = DmtxSchemeEdifact;
         break;
      case Base256:
         scheme = DmtxSchemeBase256;
         break;
      default:
         scheme = DmtxUndefined;
         break;
   }

   return scheme;
}

/**
 *
 *
 */
static int
GetPreviousSchemeState(int state)
{
   enum SchemeState cameFrom;

   switch(state)
   {
      case AsciiDigit2:
         cameFrom = AsciiDigit1;
         break;
      case C40Digit2:
         cameFrom = C40Digit1;
         break;
      case C40Digit3:
         cameFrom = C40Digit2;
         break;
      case TextDigit2:
         cameFrom = TextDigit1;
         break;
      case TextDigit3:
         cameFrom = TextDigit2;
         break;
      case X12Digit2:
         cameFrom = X12Digit1;
         break;
      case X12Digit3:
         cameFrom = X12Digit2;
         break;
      case EdifactDigit2:
         cameFrom = EdifactDigit1;
         break;
      case EdifactDigit3:
         cameFrom = EdifactDigit2;
         break;
      case EdifactDigit4:
         cameFrom = EdifactDigit3;
         break;
      default:
         cameFrom = DmtxUndefined;
         break;
   }

   return cameFrom;
}
