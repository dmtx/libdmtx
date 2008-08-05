/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (c) 2008 Mike Laughton

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Contact: mike@dragonflylogic.com
*/

/* $Id$ */

/**
 * @file dmtxreedsol.c
 * @brief Reed Solomon error correction
 */

#ifndef GfSum
#define GfSum(a,b) (a ^ b)
#endif

/**
 * @brief  XXX
 * @param  message
 * @param  sizeIdx
 * @return void
 */
static void
GenReedSolEcc(DmtxMessage *message, int sizeIdx)
{
   int i, j, val;
   int step, block;
   int blockSize;
   int dataLength;
   int errorWordCount;
   int totalLength;
   unsigned char g[69], b[68], *bPtr;
   unsigned char *codewords = message->code;

   errorWordCount = dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords, sizeIdx);
   step = dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, sizeIdx);
   blockSize = errorWordCount / step;

   memset(g, 0x01, sizeof(g));

   /* Generate ECC polynomial */
   for(i = 1; i <= blockSize; i++) {
      for(j = i - 1; j >= 0; j--) {
         g[j] = GfDoublify(g[j], i);     /* g[j] *= 2**i */
         if(j > 0)
            g[j] = GfSum(g[j], g[j-1]);  /* g[j] += g[j-1] */
      }
   }

   /* Populate error codeword array */
   for(block = 0; block < step; block++) {

      /* XXX should this lookup automatically handle this special case? */
      dataLength = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx);
      if(sizeIdx == DmtxSize144x144 && block > 7)
         dataLength -= 2;

      totalLength = dataLength + errorWordCount;

      memset(b, 0x00, sizeof(b));
      for(i = block; i < dataLength; i += step) {
         val = GfSum(b[blockSize-1], codewords[i]);
         for(j = blockSize - 1; j > 0; j--) {
            b[j] = GfSum(b[j-1], GfProduct(g[j], val));
         }
         b[0] = GfProduct(g[0], val);
      }

      bPtr = b + blockSize - 1;
      for(i = dataLength + block; i < totalLength; i += step) {
         codewords[i] = *(bPtr--);
      }

      assert(b - bPtr == 1);
   }
}

/**
 * @brief  XXX
 * @param  code
 * @param  sizeIdx
 * @param  fix
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
DecodeCheckErrors(unsigned char *code, int sizeIdx, int fix)
{
   int i, j;
   int interleavedBlocks;
   int blockErrorWords;
   int blockDataWords;
   int blockTotalWords;
   int blockMaxCorrectable;
   struct rs *rs;
   int fixedErr, fixedErrSum;
   unsigned char data[255];

   interleavedBlocks = dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, sizeIdx);
   blockErrorWords = dmtxGetSymbolAttribute(DmtxSymAttribBlockErrorWords, sizeIdx);
   blockDataWords = dmtxGetSymbolAttribute(DmtxSymAttribBlockDataWords, sizeIdx);
   blockMaxCorrectable = dmtxGetSymbolAttribute(DmtxSymAttribBlockMaxCorrectable, sizeIdx);
   blockTotalWords = blockErrorWords + blockDataWords;

   /* XXX not implemented yet */
   if(sizeIdx == DmtxSize144x144)
      return DMTX_FAILURE;

   /* XXX something like this will be necessary to decode 144x144 ... but no time now
   if(sizeIdx == DmtxSize144x144 && i > 7)
      blockTotalWords--;
   */

   rs = init_rs_char(blockErrorWords, 255 - blockTotalWords);
   if(rs == NULL)
      return DMTX_FAILURE;

   fixedErrSum = 0;
   for(i = 0; i < interleavedBlocks; i++) {

      for(j = 0; j < blockTotalWords; j++)
         data[j] = code[j*interleavedBlocks+i];

      fixedErr = decode_rs_char(rs, data, NULL, 0, fix);

      if(fixedErr < 0 || fixedErr > blockMaxCorrectable) {
         free_rs_char(rs);
         return DMTX_FAILURE;
      }

      fixedErrSum += fixedErr;

      for(j = 0; j < blockTotalWords; j++)
         code[j*interleavedBlocks+i] = data[j];
   }

   free_rs_char(rs);

   if(fix >= 0 && fixedErrSum > fix)
      return DMTX_FAILURE;

   return DMTX_SUCCESS;
}

/**
 * @brief  Galois Field Arithmetic: a times b
 * @param  a
 * @param  b
 * @return Galois Field Product
 */
static int
GfProduct(int a, int b)
{
   if(a == 0 || b == 0)
      return 0;
   else
      return aLogVal[(logVal[a] + logVal[b]) % 255];
}

/**
 * @brief  XXX
 * @param  a
 * @param  b
 * @return Result
 */
static int
GfDoublify(int a, int b)
{
   if(a == 0) /* XXX this is right, right? */
      return 0;
   else if(b == 0)
      return a; /* XXX this is right, right? */
   else
      return aLogVal[(logVal[a] + b) % 255];
}
