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

#ifndef GfSum
#define GfSum(a,b) (a ^ b)
#endif

/**
 *
 * @param XXX
 * @return XXX
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

   dataLength = dmtxGetSymbolAttribute(DmtxSymAttribDataWordLength, sizeIdx);
   errorWordCount = dmtxGetSymbolAttribute(DmtxSymAttribErrorWordLength, sizeIdx);
   totalLength = dataLength + errorWordCount;

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
 * XXX
 *
 * @param
 * @return XXX
 */
static int
DecodeCheckErrors(DmtxMessage *message, int sizeIdx, int fix)
{
   int errorWordLength;
   void *rs;
   int fixederr;

   errorWordLength = dmtxGetSymbolAttribute(DmtxSymAttribErrorWordLength, sizeIdx);

   rs = init_rs_char(8, 0x12d, 1, 1, errorWordLength, 255 - message->codeSize);
   fixederr = decode_rs_char(rs, message->code, NULL, 0);

   if(fixederr == 0) {
      return DMTX_SUCCESS;
   }
   else if(fixederr > 0) {
/*    if(fixederr)
         fprintf(stdout, "libdmtx: RS fixed %d errors\n", fixederr); */

      return (fix == 1) ? DMTX_SUCCESS : DMTX_FAILURE;
   }
   else {
      return DMTX_FAILURE;
   }
}

/**
 * a times b
 * @param XXX
 * @return XXX
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
 *
 * @param XXX
 * @return XXX
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
