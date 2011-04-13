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
   AsciiFull,
   AsciiCompactOffset0, /* 0 offset from first regular input value */
   AsciiCompactOffset1,
   C40Offset0,          /* 0 offset from first expanded C40 value */
   C40Offset1,
   C40Offset2,
   TextOffset0,         /* 0 offset from first expanded Text value */
   TextOffset1,
   TextOffset2,
   X12Offset0,          /* 0 offset from first expanded X12 value */
   X12Offset1,
   X12Offset2,
   EdifactOffset0,      /* 0 offset from first regular input value */
   EdifactOffset1,
   EdifactOffset2,
   EdifactOffset3,
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
      if(streamBest[state].status == DmtxStatusEncoding ||
            streamBest[state].status == DmtxStatusComplete)
         fprintf(stdout, "\"%c\" ", streamBest[state].input->b[streamBest[state].inputNext-1]);
      else
         fprintf(stdout, "    ");

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
   enum SchemeState state;
   int inputNext = 0;
   int sizeIdx;
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
      outputBest[state] = dmtxByteListBuild(outputBestStorage[state], sizeof(outputBestStorage[state]));
      outputTemp[state] = dmtxByteListBuild(outputTempStorage[state], sizeof(outputTempStorage[state]));
      streamBest[state] = StreamInit(input, &(outputBest[state]));
      streamTemp[state] = StreamInit(input, &(outputTemp[state]));
   }

/*
   for(inputNext = 0; inputNext < input->length; inputNext++)
   {
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, AsciiFull, sizeIdxRequest);

      asciiCompactStart = f(inputNext % 2);
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, (asciiCompactStart == AsciiCompactOffset1), sizeIdxRequest);
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, (asciiCompactStart == AsciiCompactOffset2), sizeIdxRequest);

      ctxStart = f(ctxValueCount % 3);
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, (ctxStart == C40Offset0), sizeIdxRequest);
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, (ctxStart == C40Offset1), sizeIdxRequest);
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, (ctxStart == C40Offset2), sizeIdxRequest);

      textStart = f(textValueCount % 3);
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, (textStart == TextOffset0), sizeIdxRequest);
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, (textStart == TextOffset1), sizeIdxRequest);
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, (textStart == TextOffset2), sizeIdxRequest);

      x12Start = f(x12ValueCount % 3);
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, (x12Start == X12Offset0), sizeIdxRequest);
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, (x12Start == X12Offset1), sizeIdxRequest);
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, (x12Start == X12Offset2), sizeIdxRequest);

      edifactStart = f(inputNext % 4);
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, (edifactStart == EdifactOffset0), sizeIdxRequest);
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, (edifactStart == EdifactOffset1), sizeIdxRequest);
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, (edifactStart == EdifactOffset2), sizeIdxRequest);
      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, (edifactStart == EdifactOffset3), sizeIdxRequest);

      StreamAdvanceFromBest(&(streamTemp[state]), streamBest, Base256, sizeIdxRequest);

      // Update "current" streams with results
      for(state = 0; state < SchemeStateCount; state++)
      {
         if(streamBest[state].status != DmtxStatusComplete)
            StreamCopy(&(streamBest[state]), &(streamTemp[state]));
      }
   }
*/
   /* For each input value */
   for(inputNext = 0; inputNext < input->length; inputNext++)
   {
      /* For each encoding state */
      for(state = 0; state < SchemeStateCount; state++)
      {
         if(streamBest[state].status != DmtxStatusComplete)
         {
            if(streamBest[state].status != DmtxStatusInvalid && inputNext < streamBest[state].inputNext)
            {
               StreamCopy(&(streamTemp[state]), &(streamBest[state]));
            }
            else
            {
               /* Better name: CanSwitchHere() or something */
               if(IsValidTargetState(state, inputNext))
               {
                  StreamAdvanceFromBest(&(streamTemp[state]), streamBest, state, sizeIdxRequest);
               }
               else
               {
                  StreamCopy(&(streamTemp[state]), &(streamBest[state]));
                  EncodeNextChunk(&(streamTemp[state]), GetScheme(state), DmtxEncodeNormal, sizeIdxRequest);
               }

               /*
                * Here's the problem -- we need to encode next chunk from same
                * state, but only if we're already in a valid state to begin with.
                * Maybe we should initialize intermediate states to Invalid? How
                * does EncodeNextChunk() handle attempts to encode invalid streams?
                */

            }
         }
      }

      /* Update "current" streams with results */
      for(state = 0; state < SchemeStateCount; state++)
      {
         if(streamBest[state].status != DmtxStatusComplete)
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

   /* Copy winner to output */
   if(winner == NULL)
   {
      sizeIdx = DmtxUndefined;
   }
   else
   {
      dmtxByteListCopy(output, winner->output, &passFail);
      sizeIdx = (passFail == DmtxPass) ? winner->sizeIdx : DmtxUndefined;
   }

   return sizeIdx;
}

/**
 * It's safe to compare output length because all targetState combinations
 * start on same input and encodes same number of inputs. Only difference
 * is the number of latches/unlatches that are also encoded
 */
static void
StreamAdvanceFromBest(DmtxEncodeStream *streamNext, DmtxEncodeStream *streamList,
     int targetState, int sizeIdxRequest)
{
   enum SchemeState fromState;
   DmtxScheme targetScheme;
   DmtxEncodeOption encodeOption;
   DmtxEncodeStream streamTemp;
   DmtxByte outputTempStorage[4096];
   DmtxByteList outputTemp = dmtxByteListBuild(outputTempStorage, sizeof(outputTempStorage));

   streamTemp.output = &outputTemp; /* Set directly instead of calling StreamInit() */
   targetScheme = GetScheme(targetState);

   if(targetState == AsciiFull)
      encodeOption = DmtxEncodeFull;
   else if(targetState == AsciiCompactOffset0 || targetState == AsciiCompactOffset1)
      encodeOption = DmtxEncodeCompact;
   else
      encodeOption = DmtxEncodeNormal;

   for(fromState = 0; fromState < SchemeStateCount; fromState++)
   {
      StreamCopy(&streamTemp, &(streamList[fromState]));
      EncodeNextChunk(&streamTemp, targetScheme, encodeOption, sizeIdxRequest);

      if(fromState == 0 || (streamTemp.status != DmtxStatusInvalid &&
            streamTemp.output->length < streamNext->output->length))
      {
         StreamCopy(streamNext, &streamTemp);
      }
   }
}

/**
 *
 *
 */
static DmtxBoolean
IsValidTargetState(int state, int inputNext)
{
   DmtxBoolean validTargetState;

   switch(state)
   {
      case AsciiCompactOffset0:
         validTargetState = (inputNext % 2 == 0) ? DmtxTrue : DmtxFalse;
         break;

      case AsciiCompactOffset1:
         validTargetState = (inputNext % 2 == 1) ? DmtxTrue : DmtxFalse;
         break;

      case C40Offset0:
      case TextOffset0:
      case X12Offset0:
         validTargetState = (inputNext % 3 == 0) ? DmtxTrue : DmtxFalse; /* wrong */
         break;

      case C40Offset1:
      case TextOffset1:
      case X12Offset1:
         validTargetState = (inputNext % 3 == 1) ? DmtxTrue : DmtxFalse; /* wrong */
         break;

      case C40Offset2:
      case TextOffset2:
      case X12Offset2:
         validTargetState = (inputNext % 3 == 2) ? DmtxTrue : DmtxFalse; /* wrong */
         break;

      case EdifactOffset0:
         validTargetState = (inputNext % 4 == 0) ? DmtxTrue : DmtxFalse;
         break;

      case EdifactOffset1:
         validTargetState = (inputNext % 4 == 1) ? DmtxTrue : DmtxFalse;
         break;

      case EdifactOffset2:
         validTargetState = (inputNext % 4 == 2) ? DmtxTrue : DmtxFalse;
         break;

      case EdifactOffset3:
         validTargetState = (inputNext % 4 == 3) ? DmtxTrue : DmtxFalse;
         break;

      default:
         validTargetState = DmtxTrue;
         break;
   }

   return validTargetState;
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
      case AsciiFull:
      case AsciiCompactOffset0:
      case AsciiCompactOffset1:
         scheme = DmtxSchemeAscii;
         break;
      case C40Offset0:
      case C40Offset1:
      case C40Offset2:
         scheme = DmtxSchemeC40;
         break;
      case TextOffset0:
      case TextOffset1:
      case TextOffset2:
         scheme = DmtxSchemeText;
         break;
      case X12Offset0:
      case X12Offset1:
      case X12Offset2:
         scheme = DmtxSchemeX12;
         break;
      case EdifactOffset0:
      case EdifactOffset1:
      case EdifactOffset2:
      case EdifactOffset3:
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
