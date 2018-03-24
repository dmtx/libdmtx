/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2011 Mike Laughton. All rights reserved.
 * Copyright 2012-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxencodescheme.c
 * \brief Logic for encoding in single scheme
 */

/**
 * In this file:
 *
 * A "word" refers to a full codeword byte to be appended to the encoded output.
 *
 * A "value" refers to any scheme value being appended to the output stream,
 * regardless of how many bytes are used to represent it. Examples:
 *
 *   ASCII:                   1 value  in  1 codeword
 *   ASCII (digits):          2 values in  1 codeword
 *   C40/Text/X12:            3 values in  2 codewords
 *   C40/Text/X12 (unlatch):  1 values in  1 codeword
 *   EDIFACT:                 4 values in  3 codewords
 *   Base 256:                1 value  in  1 codeword
 *
 *   * Shifts count as values, so outputChainValueCount will reflect these.
 *
 *   * Latches and unlatches are also counted as values, but always in the
 *     scheme being exited.
 *
 *   * Base256 header bytes are not included as values.
 *
 * A "chunk" refers to the minimum grouping of values in a schema that must be
 * encoded together.
 *
 *   ASCII:                   1 value  (1 codeword)  in 1 chunk
 *   ASCII (digits):          2 values (1 codeword)  in 1 chunk (optional)
 *   C40/Text/X12:            3 values (2 codewords) in 1 chunk
 *   C40/Text/X12 (unlatch):  1 value  (1 codeword)  in 1 chunk
 *   EDIFACT:                 1 value  (1 codeword*) in 1 chunk
 *   Base 256:                1 value  (1 codeword)  in 1 chunk
 *
 *   * EDIFACT writes 6 bits at a time, but progress is tracked to the next byte
 *     boundary. If unlatch value finishes mid-byte, the remaining bits before
 *     the next boundary are set to zero.
 *
 * Each scheme implements 3 equivalent functions:
 *   * EncodeNextChunk[Scheme]
 *   * AppendValue[Scheme]
 *   * CompleteIfDone[Scheme]
 *
 * The function EncodeNextChunk() (no Scheme in the name) knows which scheme-
 * specific implementations to call based on the stream's current encodation
 * scheme.
 *
 * It's important that EncodeNextChunk[Scheme] not call CompleteIfDone[Scheme]
 * directly because some parts of the logic might want to encode a stream
 * without allowing the padding and other extra logic that can occur when an
 * end-of-symbol condition is triggered.
 */

/* Verify stream is using expected scheme */
#define CHKSCHEME(s) { \
   if(stream->currentScheme != (s)) { StreamMarkFatal(stream, DmtxErrorUnexpectedScheme); return; } \
}

/* CHKERR should follow any call that might alter stream status */
#define CHKERR { \
   if(stream->status != DmtxStatusEncoding) { return; } \
}

/* CHKSIZE should follows typical calls to FindSymbolSize()  */
#define CHKSIZE { \
   if(sizeIdx == DmtxUndefined) { StreamMarkInvalid(stream, DmtxErrorUnknown); return; } \
}

/**
 *
 *
 */
static int
EncodeSingleScheme(DmtxByteList *input, DmtxByteList *output, int sizeIdxRequest, DmtxScheme scheme, int fnc1)
{
   DmtxEncodeStream stream;

   stream = StreamInit(input, output);
   stream.fnc1 = fnc1;

   /* 1st FNC1 special case, encode before scheme switch */
   if (fnc1 != DmtxUndefined && (int)(input->b[0]) == fnc1)
   {
      StreamInputAdvanceNext(&stream);
      AppendValueAscii(&stream, DmtxValueFNC1);
   }

   /* Continue encoding until complete */
   while(stream.status == DmtxStatusEncoding)
      EncodeNextChunk(&stream, scheme, DmtxEncodeNormal, sizeIdxRequest);

   /* Verify encoding completed and all inputs were consumed */
   if(stream.status != DmtxStatusComplete || StreamInputHasNext(&stream))
      return DmtxUndefined;

   return stream.sizeIdx;
}

/**
 * This function distributes work to the equivalent scheme-specific
 * implementation.
 *
 * Each of these functions will encode the next symbol input word, and in some
 * cases this requires additional input words to be encoded as well.
 */
static void
EncodeNextChunk(DmtxEncodeStream *stream, int scheme, int option, int sizeIdxRequest)
{
   /* Special case: Prevent X12 from entering state with no way to unlatch */
   if(stream->currentScheme != DmtxSchemeX12 && scheme == DmtxSchemeX12)
   {
      if(PartialX12ChunkRemains(stream))
         scheme = DmtxSchemeAscii;
   }

   /* Change to target scheme if necessary */
   if(stream->currentScheme != scheme)
   {
      EncodeChangeScheme(stream, scheme, DmtxUnlatchExplicit); CHKERR;
      CHKSCHEME(scheme);
   }

   /* Special case: Edifact may be done before writing first word */
   if(scheme == DmtxSchemeEdifact)
      CompleteIfDoneEdifact(stream, sizeIdxRequest); CHKERR;

   switch(stream->currentScheme)
   {
      case DmtxSchemeAscii:
         EncodeNextChunkAscii(stream, option); CHKERR;
         CompleteIfDoneAscii(stream, sizeIdxRequest); CHKERR;
         break;
      case DmtxSchemeC40:
      case DmtxSchemeText:
      case DmtxSchemeX12:
         EncodeNextChunkCTX(stream, sizeIdxRequest); CHKERR;
         CompleteIfDoneCTX(stream, sizeIdxRequest); CHKERR;
         break;
      case DmtxSchemeEdifact:
         EncodeNextChunkEdifact(stream); CHKERR;
         CompleteIfDoneEdifact(stream, sizeIdxRequest); CHKERR;
         break;
      case DmtxSchemeBase256:
         EncodeNextChunkBase256(stream); CHKERR;
         CompleteIfDoneBase256(stream, sizeIdxRequest); CHKERR;
         break;
      default:
         StreamMarkFatal(stream, DmtxErrorUnknown);
         break;
   }
}

/**
 *
 *
 */
static void
EncodeChangeScheme(DmtxEncodeStream *stream, DmtxScheme targetScheme, int unlatchType)
{
   /* Nothing to do */
   if(stream->currentScheme == targetScheme)
      return;

   /* Every latch must go through ASCII */
   switch(stream->currentScheme)
   {
      case DmtxSchemeC40:
      case DmtxSchemeText:
      case DmtxSchemeX12:
         if(unlatchType == DmtxUnlatchExplicit)
         {
            AppendUnlatchCTX(stream); CHKERR;
         }
         break;
      case DmtxSchemeEdifact:
         if(unlatchType == DmtxUnlatchExplicit)
         {
            AppendValueEdifact(stream, DmtxValueEdifactUnlatch); CHKERR;
         }
         break;
      default:
         /* Nothing to do for ASCII or Base 256 */
         assert(stream->currentScheme == DmtxSchemeAscii ||
               stream->currentScheme == DmtxSchemeBase256);
         break;
   }
   stream->currentScheme = DmtxSchemeAscii;

   /* Anything other than ASCII (the default) requires a latch */
   switch(targetScheme)
   {
      case DmtxSchemeC40:
         AppendValueAscii(stream, DmtxValueC40Latch); CHKERR;
         break;
      case DmtxSchemeText:
         AppendValueAscii(stream, DmtxValueTextLatch); CHKERR;
         break;
      case DmtxSchemeX12:
         AppendValueAscii(stream, DmtxValueX12Latch); CHKERR;
         break;
      case DmtxSchemeEdifact:
         AppendValueAscii(stream, DmtxValueEdifactLatch); CHKERR;
         break;
      case DmtxSchemeBase256:
         AppendValueAscii(stream, DmtxValueBase256Latch); CHKERR;
         break;
      default:
         /* Nothing to do for ASCII */
         CHKSCHEME(DmtxSchemeAscii);
         break;
   }
   stream->currentScheme = targetScheme;

   /* Reset new chain length to zero */
   stream->outputChainWordCount = 0;
   stream->outputChainValueCount = 0;

   /* Insert header byte if just latched to Base256 */
   if(targetScheme == DmtxSchemeBase256)
   {
      UpdateBase256ChainHeader(stream, DmtxUndefined); CHKERR;
   }
}

/**
 *
 *
 */
static int
GetRemainingSymbolCapacity(int outputLength, int sizeIdx)
{
   int capacity;
   int remaining;

   if(sizeIdx == DmtxUndefined)
   {
      remaining = DmtxUndefined;
   }
   else
   {
      capacity = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx);
      remaining = capacity - outputLength;
   }

   return remaining;
}
