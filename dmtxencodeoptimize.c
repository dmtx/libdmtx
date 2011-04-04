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
   AsciiExpanded,
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
   enum SchemeState i, cameFrom;
   DmtxScheme targetScheme;
   DmtxEncodeOption encodeOption;
   DmtxEncodeStream streamBest[SchemeStateCount];
   DmtxEncodeStream streamTemp[SchemeStateCount];
   DmtxByte outputBestStorage[SchemeStateCount][4096];
   DmtxByte outputTempStorage[SchemeStateCount][4096];
   DmtxByteList outputBest[SchemeStateCount];
   DmtxByteList outputTemp[SchemeStateCount];
   DmtxEncodeStream *winner;
   DmtxPassFail passFail;

   /* Initialize streams with their own output storage */
   for(i = 0; i < SchemeStateCount; i++)
   {
      outputBest[i] = dmtxByteListBuild(outputBestStorage[i], sizeof(outputBestStorage[i]));
      outputTemp[i] = dmtxByteListBuild(outputTempStorage[i], sizeof(outputTempStorage[i]));
      streamBest[i] = StreamInit(input, &(outputBest[i]));
      streamTemp[i] = StreamInit(input, &(outputTemp[i]));
   }

   /* Encode first chunk to each state or invalidate if not possible */
   for(i = 0; i < SchemeStateCount; i++)
   {
      cameFrom = GetPreviousSchemeState(i);

      if(cameFrom == DmtxUndefined)
      {
         targetScheme = GetSchemeFromState(i);
         encodeOption = (i == AsciiExpanded) ? DmtxEncodeExpand : DmtxEncodeNormal;
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

      /* Break condition -- Quit if no streams are still encoding */
      if(AllStreamsDone(streamBest))
         break;

      /* Find most efficient way to reach each state for the next input value */
      for(i = 0; i < SchemeStateCount; i++)
      {
         if(streamBest[i].status == DmtxStatusComplete)
            continue;

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
         if(streamBest[i].status == DmtxStatusComplete)
            continue;

         StreamCopy(&(streamBest[i]), &(streamTemp[i]));
      }
   }

   /* Choose the winner */
   winner = NULL;
   for(i = 0; i < SchemeStateCount; i++)
   {
      if(streamBest[i].status == DmtxStatusComplete)
      {
         if(winner == NULL || streamBest[i].output->length < winner->output->length)
            winner = &(streamBest[i]);
      }
   }

   if(winner == NULL)
      return DmtxUndefined;

   dmtxByteListCopy(output, winner->output, &passFail);

   return (passFail == DmtxPass) ? winner->sizeIdx : DmtxUndefined;
}

/**
 *
 *
 */
static int
StreamAdvanceFromBest(DmtxEncodeStream *streamNext, DmtxEncodeStream *stream, int targetState, int sizeIdxRequest)
{
   int i;
   int bestOrigin = DmtxUndefined;
   DmtxScheme targetScheme;
   DmtxEncodeOption encodeOption;
   DmtxEncodeStream streamTemp;
   DmtxByte outputTempStorage[4096];
   DmtxByteList outputTemp = dmtxByteListBuild(outputTempStorage, sizeof(outputTempStorage));

   streamTemp.output = &outputTemp;
   targetScheme = GetSchemeFromState(targetState);
   encodeOption = (targetState == AsciiExpanded) ? DmtxEncodeExpand : DmtxEncodeNormal;

   for(i = 0; i < SchemeStateCount; i++)
   {
      if(CantUnlatch(i) || stream[i].status != DmtxStatusEncoding)
         continue;

      StreamCopy(&streamTemp, &(stream[i]));

      EncodeNextChunk(&streamTemp, targetScheme, encodeOption, sizeIdxRequest);

      if(streamTemp.status != DmtxStatusInvalid &&
            (bestOrigin == DmtxUndefined || streamTemp.output->length < streamNext->output->length))
      {
         bestOrigin = i;
         StreamCopy(streamNext, &streamTemp);
      }
   }

   if(bestOrigin == DmtxUndefined)
      StreamMarkInvalid(streamNext, 1 /* all streams are invalid */);

   return bestOrigin;
}

/*
   DmtxScheme targetScheme;
   DmtxEncodeOption encodeOption;

   StreamCopy(streamNext, &(stream[AsciiExpanded]));

   targetScheme = GetSchemeFromState(targetState);
   encodeOption = (targetState == AsciiExpanded) ? DmtxEncodeExpand : DmtxEncodeNormal;

   EncodeNextChunk(streamNext, targetScheme, encodeOption, sizeIdxRequest);
*/

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
      case AsciiExpanded:
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

/**
 *
 *
 */
static DmtxBoolean
CantUnlatch(int state)
{
   DmtxBoolean cantUnlatch;

   if(state == AsciiDigit1 ||
         state == C40Digit1 || state == C40Digit2 ||
         state == TextDigit1 || state == TextDigit2 ||
         state == X12Digit1 || state == X12Digit2)
   {
      cantUnlatch = DmtxTrue;
   }
   else
   {
      cantUnlatch = DmtxFalse;
   }

   return cantUnlatch;
}

static DmtxBoolean
AllStreamsDone(DmtxEncodeStream *streams)
{
   enum SchemeState i;

   for(i = 0; i < SchemeStateCount; i++)
   {
      if(streams[i].status == DmtxStatusEncoding)
         return DmtxFalse;
   }

   return DmtxTrue;
}
