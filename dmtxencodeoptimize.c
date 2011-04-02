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
   AsciiExpand,
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
EncodeOptimizeBest(DmtxByteList *input, DmtxByteList *output, int sizeIdxRequest)
{
   int i, cameFrom;
   DmtxScheme targetScheme;
   DmtxEncodeOption encodeOption;
   DmtxEncodeStream streamBest[SchemeStateCount];
   DmtxEncodeStream streamTemp[SchemeStateCount];
   DmtxByte outputBestStorage[SchemeStateCount][4096];
   DmtxByte outputTempStorage[SchemeStateCount][4096];
   DmtxByteList outputBest[SchemeStateCount];
   DmtxByteList outputTemp[SchemeStateCount];

   /* Initialize streams with their own output storage */
   for(i = 0; i < SchemeStateCount; i++)
   {
      outputBest[i] = dmtxByteListBuild(outputBestStorage[i], sizeof(outputBestStorage[i]));
      outputTemp[i] = dmtxByteListBuild(outputTempStorage[i], sizeof(outputTempStorage[i]));
      streamBest[i] = StreamInit(input, &(outputBest[i]));
      streamTemp[i] = StreamInit(input, &(outputTemp[i]));
   }

   /* Encode first chunk in each stream or invalidate if not possible */
   for(i = 0; i < SchemeStateCount; i++)
   {
      cameFrom = GetPreviousSchemeState(i);

      if(cameFrom == DmtxUndefined)
      {
         targetScheme = GetSchemeFromState(i);
         encodeOption = (i == AsciiExpand) ? DmtxEncodeExpand : DmtxEncodeNormal;
         EncodeNextChunk(&(streamBest[i]), targetScheme, encodeOption, sizeIdxRequest);
      }
      else
      {
         StreamMarkInvalid(&(streamBest[i]), DmtxChannelUnsupportedChar);
      }
   }

   /* Continue encoding until all streams are complete */
   for(;;)
   {
      /* XXX check streams for fatal condition here ? */

      /* Break condition -- Quit when all streams are either finished or invalid */
      if(AllStreamsComplete(streamBest))
         break;

      /* Find most efficient way to reach each state for the next input value */
      for(i = 0; i < SchemeStateCount; i++)
      {
         cameFrom = GetPreviousSchemeState(i);

         if(cameFrom == DmtxUndefined || streamBest[i].status == DmtxStatusInvalid)
         {
            StreamAdvanceFromBest(&(streamTemp[i]), streamBest, i, sizeIdxRequest);
         }
         else
         {
            StreamCopy(&(streamTemp[i]), &(streamBest[cameFrom]));
         }
      }

      /* Update "current" streams with results */
      for(i = 0; i < SchemeStateCount; i++)
      {
/* XXX only copy over streamBest if streamBest is not complete */
         StreamCopy(&(streamBest[i]), &(streamTemp[i]));
      }
   }

   return DmtxUndefined;
}

/**
 *
 *
 */
static void
StreamAdvanceFromBest(DmtxEncodeStream *streamNext, DmtxEncodeStream *stream, int targetState, int sizeIdxRequest)
{
   DmtxScheme targetScheme;
   DmtxEncodeOption encodeOption;

   StreamCopy(streamNext, &(stream[AsciiExpand]));

   targetScheme = GetSchemeFromState(targetState);
   encodeOption = (targetState == AsciiExpand) ? DmtxEncodeExpand : DmtxEncodeNormal;

   EncodeNextChunk(streamNext, targetScheme, encodeOption, sizeIdxRequest);
/* XXX should produce invalid output if all streams are invalid */
/*
   for(i = 0; i < 18; i++)
   {
      tmpEncodeStream[i] = stream[i];
      stopped = EncodeNextChunk(tmpEncodeStream[i], scheme, subScheme, sizeIdxRequest);

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
      case AsciiExpand:
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

static DmtxBoolean
AllStreamsComplete(DmtxEncodeStream *streams)
{
   int i;

   for(i = 0; i < SchemeStateCount; i++)
   {
      if(streams[i].status == DmtxStatusEncoding)
         return DmtxFalse;
   }

   return DmtxTrue;
}
