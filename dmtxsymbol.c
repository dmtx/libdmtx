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
 * @file dmtxsymbol.c
 * @brief Data Matrix symbol attributes
 */

/**
 * @brief  XXX
 * @param  attribute
 * @param  sizeIdx
 * @return Attribute value
 */
extern int
dmtxGetSymbolAttribute(int attribute, int sizeIdx)
{
   static const int symbolRows[] = { 10, 12, 14, 16, 18, 20,  22,  24,  26,
                                                 32, 36, 40,  44,  48,  52,
                                                 64, 72, 80,  88,  96, 104,
                                                        120, 132, 144,
                                                  8,  8, 12,  12,  16,  16 };

   static const int symbolCols[] = { 10, 12, 14, 16, 18, 20,  22,  24,  26,
                                                 32, 36, 40,  44,  48,  52,
                                                 64, 72, 80,  88,  96, 104,
                                                        120, 132, 144,
                                                 18, 32, 26,  36,  36,  48 };

   static const int dataRegionRows[] = { 8, 10, 12, 14, 16, 18, 20, 22, 24,
                                                    14, 16, 18, 20, 22, 24,
                                                    14, 16, 18, 20, 22, 24,
                                                            18, 20, 22,
                                                     6,  6, 10, 10, 14, 14 };

   static const int dataRegionCols[] = { 8, 10, 12, 14, 16, 18, 20, 22, 24,
                                                    14, 16, 18, 20, 22, 24,
                                                    14, 16, 18, 20, 22, 24,
                                                            18, 20, 22,
                                                    16, 14, 24, 16, 16, 22 };

   static const int horizDataRegions[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                                    2, 2, 2, 2, 2, 2,
                                                    4, 4, 4, 4, 4, 4,
                                                          6, 6, 6,
                                                    1, 2, 1, 2, 2, 2 };

   static const int interleavedBlocks[] = { 1, 1, 1, 1, 1, 1, 1,  1, 1,
                                                     1, 1, 1, 1,  1, 2,
                                                     2, 4, 4, 4,  4, 6,
                                                           6, 8, 10,
                                                     1, 1, 1, 1,  1, 1 };

   static const int blockDataWords[] = { 3, 5, 8, 12,  18,  22,  30,  36,  44,
                                                  62,  86, 114, 144, 174, 102,
                                                 140,  92, 114, 144, 174, 136,
                                                           175, 163, 156,
                                                   5,  10,  16,  22,  32,  49 };

   static const int blockErrorWords[] = { 5, 7, 10, 12, 14, 18, 20, 24, 28,
                                                    36, 42, 48, 56, 68, 42,
                                                    56, 36, 48, 56, 68, 56,
                                                            68, 62, 62,
                                                     7, 11, 14, 18, 24, 28 };

   static const int blockMaxCorrectable[] = { 2, 3, 5,  6,  7,  9,  10,  12,  14,
                                                       18, 21, 24,  28,  34,  21,
                                                       28, 18, 24,  28,  34,  28,
                                                               34,  31,  31,
                                                   3,  5,  7,   9,  12,  14 };

   /* XXX maybe this should be a proper check instead of an assertion */
   assert(sizeIdx >= 0 && sizeIdx < DMTX_SYMBOL_SQUARE_COUNT + DMTX_SYMBOL_RECT_COUNT);

   switch(attribute) {
      case DmtxSymAttribSymbolRows:
         return symbolRows[sizeIdx];
         break;
      case DmtxSymAttribSymbolCols:
         return symbolCols[sizeIdx];
         break;
      case DmtxSymAttribDataRegionRows:
         return dataRegionRows[sizeIdx];
         break;
      case DmtxSymAttribDataRegionCols:
         return dataRegionCols[sizeIdx];
         break;
      case DmtxSymAttribHorizDataRegions:
         return horizDataRegions[sizeIdx];
         break;
      case DmtxSymAttribVertDataRegions:
         return (sizeIdx < DMTX_SYMBOL_SQUARE_COUNT) ? horizDataRegions[sizeIdx] : 1;
         break;
      case DmtxSymAttribMappingMatrixRows:
         return dataRegionRows[sizeIdx] * dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, sizeIdx);
         break;
      case DmtxSymAttribMappingMatrixCols:
         return dataRegionCols[sizeIdx] * horizDataRegions[sizeIdx];
         break;
      case DmtxSymAttribInterleavedBlocks:
         return interleavedBlocks[sizeIdx];
         break;
      case DmtxSymAttribBlockDataWords:
         return blockDataWords[sizeIdx];
         break;
      case DmtxSymAttribBlockErrorWords:
         return blockErrorWords[sizeIdx];
         break;
      case DmtxSymAttribBlockMaxCorrectable:
         return blockMaxCorrectable[sizeIdx];
         break;
      case DmtxSymAttribSymbolDataWords:
         return blockDataWords[sizeIdx] * interleavedBlocks[sizeIdx];
         break;
      case DmtxSymAttribSymbolErrorWords:
         return blockErrorWords[sizeIdx] * interleavedBlocks[sizeIdx];
         break;
      case DmtxSymAttribSymbolMaxCorrectable:
         return blockMaxCorrectable[sizeIdx] * interleavedBlocks[sizeIdx];
         break;
      default:
         exit(1); /* error condition */
         break;
   }

   return -1;
}

/**
 * @brief  XXX
 * @param  dataWords
 * @param  sizeIdxRequest
 * @return Barcode size index (or -1 if none)
 */
static int
FindCorrectBarcodeSize(int dataWords, int sizeIdxRequest)
{
   int sizeIdx;
   int idxBeg, idxEnd;

   if(dataWords <= 0)
      return -1;

   if(sizeIdxRequest == DMTX_SYMBOL_SQUARE_AUTO || sizeIdxRequest == DMTX_SYMBOL_RECT_AUTO) {

      idxBeg = (sizeIdxRequest == DMTX_SYMBOL_SQUARE_AUTO) ? 0 : DMTX_SYMBOL_SQUARE_COUNT;
      idxEnd = (sizeIdxRequest == DMTX_SYMBOL_SQUARE_AUTO) ? DMTX_SYMBOL_SQUARE_COUNT :
            DMTX_SYMBOL_SQUARE_COUNT + DMTX_SYMBOL_RECT_COUNT;

      for(sizeIdx = idxBeg; sizeIdx < idxEnd; sizeIdx++) {
         if(dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx) >= dataWords)
            break;
      }

      if(sizeIdx == idxEnd)
         return -1;
   }
   else {
      sizeIdx = sizeIdxRequest;
   }

   if(dataWords > dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx))
      return -1;

   return sizeIdx;
}
