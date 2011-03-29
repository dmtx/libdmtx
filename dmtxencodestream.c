/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2011 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in parent directory for full terms of
 * use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxencodestream.c
 */

/**
 *
 *
 */
static DmtxEncodeStream
StreamInit(DmtxByteList *input, DmtxByteList *output)
{
   DmtxEncodeStream stream;

   stream.input = input;
   stream.output = output;

   stream.currentScheme = DmtxSchemeAscii;
   stream.inputNext = 0;
   stream.outputChainValueCount = 0;
   stream.outputChainWordCount = 0;
   stream.reason = DmtxUndefined;
   stream.sizeIdx = DmtxUndefined;
   stream.status = DmtxStatusEncoding;

   return stream;
}

/**
 *
 *
 */
static void
StreamCopy(DmtxEncodeStream *dst, DmtxEncodeStream *src)
{
   int length;

   dst->currentScheme = src->currentScheme;
   dst->inputNext = src->inputNext;
   dst->outputChainValueCount = src->outputChainValueCount;
   dst->outputChainWordCount = src->outputChainWordCount;
   dst->reason = src->reason;
   dst->sizeIdx = src->sizeIdx;
   dst->status = src->status;
   dst->input = src->input;

   length = min(dst->input->capacity, dst->output->capacity);
   memmove(dst->output, src->output, length);
}

/**
 *
 *
 */
static void
StreamMarkComplete(DmtxEncodeStream *stream, int sizeIdx)
{
   if(stream->status == DmtxStatusEncoding)
   {
      stream->sizeIdx = sizeIdx;
      stream->status = DmtxStatusComplete;
      assert(stream->reason == DmtxUndefined);
   }
}

/**
 *
 *
 */
static void
StreamMarkInvalid(DmtxEncodeStream *stream, int reason)
{
   stream->status = DmtxStatusInvalid;
   stream->reason = reason;
}

/**
 *
 *
 */
static void
StreamMarkFatal(DmtxEncodeStream *stream, int reason)
{
   stream->status = DmtxStatusFatal;
   stream->reason = reason;
}

/**
 * push on newest/last append
 * used for encoding each output cw
 */
static void
StreamOutputChainAppend(DmtxEncodeStream *stream, DmtxByte value)
{
   DmtxPassFail passFail;

   dmtxByteListPush(stream->output, value, &passFail);

   if(passFail == DmtxPass)
      stream->outputChainWordCount++;
   else
      StreamMarkFatal(stream, 1 /*DmtxOutOfBounds*/);
}

/**
 * pop off newest/last
 * used for edifact
 */
static DmtxByte
StreamOutputChainRemoveLast(DmtxEncodeStream *stream)
{
   DmtxByte value;
   DmtxPassFail passFail;

   if(stream->outputChainWordCount > 0)
   {
      value = dmtxByteListPop(stream->output, &passFail);
      stream->outputChainWordCount--;
   }
   else
   {
      value = 0;
      StreamMarkFatal(stream, 1 /*DmtxEmptyList*/);
   }

   return value;
}

/**
 * overwrite arbitrary element
 * used for binary length changes
 */
static void
StreamOutputSet(DmtxEncodeStream *stream, int index, DmtxByte value)
{
   if(index < 0 || index >= stream->output->length)
      StreamMarkFatal(stream, 1 /* out of bounds */);
   else
      stream->output->b[index] = value;
}

/**
 *
 *
 */
static DmtxBoolean
StreamInputHasNext(DmtxEncodeStream *stream)
{
   return (stream->inputNext < stream->input->length) ? DmtxTrue : DmtxFalse;
}

/**
 * peek at first/oldest
 * used for ascii double digit
 */
static DmtxByte
StreamInputPeekNext(DmtxEncodeStream *stream)
{
   DmtxByte value = 0;

   if(StreamInputHasNext(stream))
      value = stream->input->b[stream->inputNext];
   else
      StreamMarkFatal(stream, 1 /*DmtxOutOfBounds*/);

   return value;
}

/**
 * used as each input cw is processed
 *
 * /param value Value to populate, can be null (for blind dequeues)
 * /param stream
 */
static DmtxByte
StreamInputAdvanceNext(DmtxEncodeStream *stream)
{
   DmtxByte value;

   value = StreamInputPeekNext(stream);

   if(stream->status == DmtxStatusEncoding)
      stream->inputNext++; /* XXX is this what we really mean here? */

   return value;
}

/**
 * used as each input cw is processed
 *
 * /param value Value to populate, can be null (for blind dequeues)
 * /param stream
 */
static void
StreamInputAdvancePrev(DmtxEncodeStream *stream)
{
   if(stream->inputNext > 0)
      stream->inputNext--;
   else
      StreamMarkFatal(stream, 1 /*DmtxOutOfBounds*/);
}
