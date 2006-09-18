/*
libdmtx - Data Matrix Encoding/Decoding Library
Copyright (C) 2006 Mike Laughton

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

/* $Id: dmtxreedsol.c,v 1.2 2006-09-18 17:55:46 mblaughton Exp $ */

/**
 *
 * @param XXX
 * @return XXX
 */
static void
GenReedSolEcc(DmtxMatrixRegion *matrixRegion)
{
   int i, j, val;
   int dataLength = dataWordLength[matrixRegion->sizeIdx];
   int errorWordCount = errorWordLength[matrixRegion->sizeIdx];
   int totalLength = dataLength + errorWordCount;
   unsigned char g[69], b[68];
   unsigned char *codewords = matrixRegion->code;

   SetEccPoly(g, errorWordCount);
   memset(b, 0x00, sizeof(b));

   for(i = 0; i < totalLength; i++) {
      if(i < dataLength) {
         val = GfSum(b[errorWordCount-1], codewords[i]);
         for(j = errorWordCount - 1; j > 0; j--) {
            b[j] = GfSum(b[j-1], GfProduct(g[j], val));
         }
         b[0] = GfProduct(g[0], val);
      }
      else {
         codewords[i] = b[totalLength-i-1];
      }
   }

   fprintf(stdout, "\nadd ecc:");
   for(i = 0; i < matrixRegion->codeSize; i++) {
      fprintf(stdout, " %d", codewords[i]);
   }
   fprintf(stdout, "\n");
}

/**
 *
 * @param XXX
 * @return XXX
 */
static void
SetEccPoly(unsigned char *g, int errorWordCount)
{
   int i, j;

   memset(g, 0x01, sizeof(unsigned char) * (errorWordCount + 1));

   for(i = 1; i <= errorWordCount; i++) {
      for(j = i - 1; j >= 0; j--) {
         g[j] = GfDoublify(g[j], i);     // g[j] *= 2**i
         if(j > 0)
            g[j] = GfSum(g[j], g[j-1]);  // g[j] += g[j-1]
      }
   }
}
