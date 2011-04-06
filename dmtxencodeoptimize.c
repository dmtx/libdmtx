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

/* temporary */
/*
static void DumpStreams(DmtxEncodeStream *streamBest)
{
   enum SchemeState state;
   char prefix[32];

   fprintf(stdout, "----------------------------------------\n");
   for(state = 0; state < SchemeStateCount; state++)
   {
      switch(streamBest[state].status) {
         case DmtxStatusEncoding:
            snprintf(prefix, sizeof(prefix), "%2d (%s): ", state, " encode ");
            break;
         case DmtxStatusComplete:
            snprintf(prefix, sizeof(prefix), "%2d (%s): ", state, "complete");
            break;
         case DmtxStatusInvalid:
            snprintf(prefix, sizeof(prefix), "%2d (%s): ", state, "invalid ");
            break;
         case DmtxStatusFatal:
            snprintf(prefix, sizeof(prefix), "%2d (%s): ", state, " fatal  ");
            break;
      }
      dmtxByteListPrint(streamBest[state].output, prefix);
   }
}
*/

/**
 *
 *
 */
static int
EncodeOptimizeBest(DmtxByteList *input, DmtxByteList *output, int sizeIdxRequest)
{
   enum SchemeState state, cameFrom;
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

   /* Initialize all streams with their own output storage */
   for(state = 0; state < SchemeStateCount; state++)
   {
      outputBest[state] = dmtxByteListBuild(outputBestStorage[state],
            sizeof(outputBestStorage[state]));

      outputTemp[state] = dmtxByteListBuild(outputTempStorage[state],
            sizeof(outputTempStorage[state]));

      streamBest[state] = StreamInit(input, &(outputBest[state]));
      streamTemp[state] = StreamInit(input, &(outputTemp[state]));
   }

   /* Encode first chunk in each stream or invalidate if not possible */
   for(state = 0; state < SchemeStateCount; state++)
   {
      /* XXX rename GetPreviousState() to something more meaningful */
      if(GetPreviousState(state) != DmtxUndefined)
      {
         /* This state can only come from a precursor; Can't start here */
         StreamMarkInvalid(&(streamBest[state]), 1 /* can't start intermediate */);
      }
      else
      {
         /* Not an intermediate state -- Start encoding */
         targetScheme = GetScheme(state);

         if(state == AsciiExpanded)
            encodeOption = DmtxEncodeExpanded;
         else if(state == AsciiDigit1)
            encodeOption = DmtxEncodeCompact;
         else
            encodeOption = DmtxEncodeNormal;

         EncodeNextChunk(&(streamBest[state]), targetScheme, encodeOption, sizeIdxRequest);
      }
   }

/* DumpStreams(streamBest); */

   /* Continue encoding until all streams are complete */
   for(;;)
   {
      /* XXX check streams for fatal condition here ? */

      /* Break condition -- Quit if no streams are still encoding */
      if(AllStreamsDone(streamBest))
         break;

      /* Find most efficient way to reach each state for the next input value */
      for(state = 0; state < SchemeStateCount; state++)
      {
         if(streamBest[state].status == DmtxStatusComplete)
            continue;

         cameFrom = GetPreviousState(state);

         if(cameFrom != DmtxUndefined)
            StreamCopy(&(streamTemp[state]), &(streamBest[cameFrom]));
         else
            StreamAdvanceFromBest(&(streamTemp[state]), streamBest, state, sizeIdxRequest);
      }

      /* Update "current" streams with results */
      for(state = 0; state < SchemeStateCount; state++)
      {
         if(streamBest[state].status == DmtxStatusComplete)
            continue;

         StreamCopy(&(streamBest[state]), &(streamTemp[state]));
      }
/* DumpStreams(streamBest); */
   }

   /* Choose the winner */
   winner = NULL;
   for(state = 0; state < SchemeStateCount; state++)
   {
      if(streamBest[state].status == DmtxStatusComplete)
      {
         if(winner == NULL || streamBest[state].output->length < winner->output->length)
            winner = &(streamBest[state]);
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
StreamAdvanceFromBest(DmtxEncodeStream *streamNext, DmtxEncodeStream *stream,
      int targetState, int sizeIdxRequest)
{
   int i;
   int bestOrigin = DmtxUndefined;
   DmtxScheme targetScheme;
   DmtxEncodeOption encodeOption;
   DmtxEncodeStream streamTemp;
   DmtxByte outputTempStorage[4096];
   DmtxByteList outputTemp = dmtxByteListBuild(outputTempStorage, sizeof(outputTempStorage));

   streamTemp.output = &outputTemp;
   targetScheme = GetScheme(targetState);

   /* XXX this logic is repeated below */
   if(targetState == AsciiExpanded)
      encodeOption = DmtxEncodeExpanded;
   else if(targetState == AsciiDigit1)
      encodeOption = DmtxEncodeCompact;
   else
      encodeOption = DmtxEncodeNormal;

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

/**
 *
 *
 */
static int
GetScheme(int state)
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
GetPreviousState(int state)
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
