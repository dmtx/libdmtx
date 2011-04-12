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

   /* For each input value */
   for(inputNext = 0; inputNext < input->length; inputNext++)
   {
      /* For each encoding state */
      for(state = 0; state < SchemeStateCount; state++)
      {
         if(streamBest[state].status != DmtxStatusComplete)
         {
            /* XXX incomplete ... still need test to determine state to receive correct offsets */
            if(streamBest[state].status != DmtxStatusInvalid && streamBest[state].encodedInputCount > 0)
               StreamCopy(&(streamTemp[state]), &(streamBest[state]));
            else
               StreamAdvanceFromBest(&(streamTemp[state]), streamBest, state, sizeIdxRequest);

            streamTemp[state].encodedInputCount--;
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
StreamAdvanceFromBest(DmtxEncodeStream *streamNext, DmtxEncodeStream *streamList, int targetState, int sizeIdxRequest)
{
   enum SchemeState fromState;
   DmtxScheme targetScheme;
   DmtxEncodeOption encodeOption;
   DmtxEncodeStream streamTemp;
   DmtxByte outputTempStorage[4096];
   DmtxByteList outputTemp = dmtxByteListBuild(outputTempStorage, sizeof(outputTempStorage));

   assert(streamList[targetState].status == DmtxStatusInvalid ||
         streamList[targetState].encodedInputCount == 0);

   streamTemp.output = &outputTemp; /* Set directly instead of calling StreamInit() */
   targetScheme = GetScheme(targetState);

   if(targetState == AsciiFull)
      encodeOption = DmtxEncodeFull;
   else if(targetState == AsciiCompactOffset0)
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
