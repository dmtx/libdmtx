/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2011 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in parent directory for full terms of
 * use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxencodescheme.c
 */

/**
 * this file deals with encoding logic (scheme rules)
 *
 * In the context of this file:
 *
 * A "word" refers to a full codeword byte to be appended to the encoded output.
 *
 * A "value" refers to any scheme value being appended to the output stream,
 * regardless of how many bytes are used to represent it. Examples:
 *
 *   ASCII:                   1 value  in  1 word
 *   ASCII (digits):          2 values in  1 word
 *   C40/Text/X12:            3 values in  2 words
 *   C40/Text/X12 (unlatch):  1 values in  1 word
 *   EDIFACT:                 4 values in  3 words
 *   Base 256:                1 value  in  1 word
 *
 *   - Shifts count as values, so outputChainValueCount will reflect these.
 *
 *   - Latches and unlatches are also counted as values, but always in the
 *     scheme being exited.
 *
 *   - Base256 header bytes are not included as values.
 *
 * A "chunk" refers to the minimum grouping of values in a schema that must be
 * encoded together.
 *
 *   ASCII:                   1 value  (1 word)  in 1 chunk
 *   ASCII (digits):          2 values (1 word)  in 1 chunk (optional)
 *   C40/Text/X12:            3 values (2 words) in 1 chunk
 *   C40/Text/X12 (unlatch):  1 value  (1 word)  in 1 chunk
 *   EDIFACT:                 1 value  (1 word*) in 1 chunk
 *   Base 256:                1 value  (1 word)  in 1 chunk
 *
 *   * EDIFACT writes 6 bits at a time, but progress is tracked to the next byte
 *     boundary. If unlatch value finishes mid-byte, the remaining bits before
 *     the next boundary are all set to zero.
 *
 * XXX maybe reorder the functions list in the file and break them up:
 *
 * Each scheme implements 3 equivalent functions:
 *   - EncodeNextChunk[Scheme]
 *   - EncodeValue[Scheme]
 *   - CompleteIfDone[Scheme]
 *
 * XXX what about renaming EncodeValue[Scheme] to AppendValue[Scheme]? That
 * shows that the stream is being affected
 *
 * The master function EncodeNextChunk() (no Scheme in the name) knows which
 * scheme-specific implementations to call based on the stream's current
 * encodation scheme.
 *
 * It's important that EncodeNextChunk[Scheme] not call CompleteIfDone[Scheme]
 * directly because some parts of the logic might want to encode a stream
 * without allowing the padding and other extra logic that can occur when an
 * end-of-symbol condition is triggered.
 */

/* XXX is there a way to handle multiple values of s? */
#define CHKSCHEME(s) { if(stream->currentScheme != (s)) { StreamMarkFatal(stream, 1); return; } }

/* CHKERR should follow any call that might alter stream status */
#define CHKERR { if(stream->status != DmtxStatusEncoding) { return; } }

/* CHKSIZE should follows typical calls to FindSymbolSize()  */
#define CHKSIZE { if(sizeIdx == DmtxUndefined) { StreamMarkInvalid(stream, 1); return; } }

/**
 *
 *
 */
static int
EncodeSingleScheme(DmtxByteList *input, DmtxByteList *output, int sizeIdxRequest, DmtxScheme scheme)
{
   DmtxEncodeStream stream;

   stream = StreamInit(input, output);

   /* Latch to target scheme (if necessary) */
   EncodeChangeScheme(&stream, scheme, DmtxUnlatchImplicit);
   if(stream.status != DmtxStatusEncoding)
      return DmtxUndefined;

   /* Continue encoding until complete */
   while(stream.status == DmtxStatusEncoding)
      EncodeNextChunk(&stream, stream.currentScheme, DmtxEncodeNormal, sizeIdxRequest);

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
   DmtxBoolean compactDigits;

   /* Change to target scheme if necessary */
   if(stream->currentScheme != scheme)
   {
      EncodeChangeScheme(stream, scheme, DmtxUnlatchExplicit); CHKERR;
      CHKSCHEME(scheme);
   }

   switch(stream->currentScheme)
   {
      case DmtxSchemeAscii:
         compactDigits = (option == DmtxEncodeExpand) ? DmtxFalse : DmtxTrue;
         EncodeNextChunkAscii(stream, compactDigits); CHKERR;
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
         StreamMarkFatal(stream, 1 /* unknown */);
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
            EncodeUnlatchCTX(stream); CHKERR;
         }
         break;
      case DmtxSchemeEdifact:
         if(unlatchType == DmtxUnlatchExplicit)
         {
            EncodeValueEdifact(stream, DmtxValueEdifactUnlatch); CHKERR;
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
         EncodeValueAscii(stream, DmtxValueC40Latch); CHKERR;
         break;
      case DmtxSchemeText:
         EncodeValueAscii(stream, DmtxValueTextLatch); CHKERR;
         break;
      case DmtxSchemeX12:
         EncodeValueAscii(stream, DmtxValueX12Latch); CHKERR;
         break;
      case DmtxSchemeEdifact:
         EncodeValueAscii(stream, DmtxValueEdifactLatch); CHKERR;
         break;
      case DmtxSchemeBase256:
         EncodeValueAscii(stream, DmtxValueBase256Latch); CHKERR;
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
