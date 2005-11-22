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

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
ModulePlacementEcc200(DmtxMatrixRegion *matrixRegion)
{
   unsigned char *codewords;
   int row, col, chr;

   codewords = matrixRegion->code;

   // Start in the nominal location for the 8th bit of the first character
   chr = 0;
   row = 4;
   col = 0;

   do {
      // Repeatedly first check for one of the special corner cases, then ...
      if((row == matrixRegion->dataRows) && (col == 0))
         PatternShapeSpecial1(matrixRegion, &(codewords[chr++]));
      else if((row == matrixRegion->dataRows-2) && (col == 0) && (matrixRegion->dataCols%4))
         PatternShapeSpecial2(matrixRegion, &(codewords[chr++]));
      else if((row == matrixRegion->dataRows-2) && (col == 0) && (matrixRegion->dataCols%8 == 4))
         PatternShapeSpecial3(matrixRegion, &(codewords[chr++]));
      else if((row == matrixRegion->dataRows+4) && (col == 2) && (!(matrixRegion->dataCols%8)))
         PatternShapeSpecial4(matrixRegion, &(codewords[chr++]));

      // Sweep upward diagonally, inserting successive characters, ...
      do {
         if((row < matrixRegion->dataRows) && (col >= 0) &&
               !(matrixRegion->array[row*matrixRegion->dataCols+col] & DMTX_MODULE_VISITED))
            PatternShapeStandard(matrixRegion, row, col, &(codewords[chr++]));
         row -= 2;
         col += 2;
      } while ((row >= 0) && (col < matrixRegion->dataCols));
      row += 1;
      col += 3;

      // And then sweep downward diagonally, inserting successive characters, ...
      do {
         if((row >= 0) && (col < matrixRegion->dataCols) &&
               !(matrixRegion->array[row*matrixRegion->dataCols+col] & DMTX_MODULE_VISITED))
            PatternShapeStandard(matrixRegion, row, col, &(codewords[chr++]));
         row += 2;
         col -= 2;
      } while ((row < matrixRegion->dataRows) && (col >= 0));
      row += 3;
      col += 1;
      // ... until the entire array is scanned
   } while ((row < matrixRegion->dataRows) || (col < matrixRegion->dataCols));

   // XXX compare that chr == matrixRegion->dataSize here

   return chr; // XXX number of codewords read off
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static void
PatternShapeStandard(DmtxMatrixRegion *matrixRegion, int row, int col, unsigned char *codeword)
{
   PlaceModule(matrixRegion, row-2, col-2, codeword, DmtxMaskBit1);
   PlaceModule(matrixRegion, row-2, col-1, codeword, DmtxMaskBit2);
   PlaceModule(matrixRegion, row-1, col-2, codeword, DmtxMaskBit3);
   PlaceModule(matrixRegion, row-1, col-1, codeword, DmtxMaskBit4);
   PlaceModule(matrixRegion, row-1, col,   codeword, DmtxMaskBit5);
   PlaceModule(matrixRegion, row,   col-2, codeword, DmtxMaskBit6);
   PlaceModule(matrixRegion, row,   col-1, codeword, DmtxMaskBit7);
   PlaceModule(matrixRegion, row,   col,   codeword, DmtxMaskBit8);
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static void
PatternShapeSpecial1(DmtxMatrixRegion *matrixRegion, unsigned char *codeword)
{
   PlaceModule(matrixRegion, matrixRegion->dataRows-1, 0, codeword, DmtxMaskBit1);
   PlaceModule(matrixRegion, matrixRegion->dataRows-1, 1, codeword, DmtxMaskBit2);
   PlaceModule(matrixRegion, matrixRegion->dataRows-1, 2, codeword, DmtxMaskBit3);
   PlaceModule(matrixRegion, 0, matrixRegion->dataCols-2, codeword, DmtxMaskBit4);
   PlaceModule(matrixRegion, 0, matrixRegion->dataCols-1, codeword, DmtxMaskBit5);
   PlaceModule(matrixRegion, 1, matrixRegion->dataCols-1, codeword, DmtxMaskBit6);
   PlaceModule(matrixRegion, 2, matrixRegion->dataCols-1, codeword, DmtxMaskBit7);
   PlaceModule(matrixRegion, 3, matrixRegion->dataCols-1, codeword, DmtxMaskBit8);
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static void
PatternShapeSpecial2(DmtxMatrixRegion *matrixRegion, unsigned char *codeword)
{
   PlaceModule(matrixRegion, matrixRegion->dataRows-3, 0, codeword, DmtxMaskBit1);
   PlaceModule(matrixRegion, matrixRegion->dataRows-2, 0, codeword, DmtxMaskBit2);
   PlaceModule(matrixRegion, matrixRegion->dataRows-1, 0, codeword, DmtxMaskBit3);
   PlaceModule(matrixRegion, 0, matrixRegion->dataCols-4, codeword, DmtxMaskBit4);
   PlaceModule(matrixRegion, 0, matrixRegion->dataCols-3, codeword, DmtxMaskBit5);
   PlaceModule(matrixRegion, 0, matrixRegion->dataCols-2, codeword, DmtxMaskBit6);
   PlaceModule(matrixRegion, 0, matrixRegion->dataCols-1, codeword, DmtxMaskBit7);
   PlaceModule(matrixRegion, 1, matrixRegion->dataCols-1, codeword, DmtxMaskBit8);
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static void
PatternShapeSpecial3(DmtxMatrixRegion *matrixRegion, unsigned char *codeword)
{
   PlaceModule(matrixRegion, matrixRegion->dataRows-3, 0, codeword, DmtxMaskBit1);
   PlaceModule(matrixRegion, matrixRegion->dataRows-2, 0, codeword, DmtxMaskBit2);
   PlaceModule(matrixRegion, matrixRegion->dataRows-1, 0, codeword, DmtxMaskBit3);
   PlaceModule(matrixRegion, 0, matrixRegion->dataCols-2, codeword, DmtxMaskBit4);
   PlaceModule(matrixRegion, 0, matrixRegion->dataCols-1, codeword, DmtxMaskBit5);
   PlaceModule(matrixRegion, 1, matrixRegion->dataCols-1, codeword, DmtxMaskBit6);
   PlaceModule(matrixRegion, 2, matrixRegion->dataCols-1, codeword, DmtxMaskBit7);
   PlaceModule(matrixRegion, 3, matrixRegion->dataCols-1, codeword, DmtxMaskBit8);
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static void
PatternShapeSpecial4(DmtxMatrixRegion *matrixRegion, unsigned char *codeword)
{
   PlaceModule(matrixRegion, matrixRegion->dataRows-1, 0, codeword, DmtxMaskBit1);
   PlaceModule(matrixRegion, matrixRegion->dataRows-1, matrixRegion->dataCols-1, codeword, DmtxMaskBit2);
   PlaceModule(matrixRegion, 0, matrixRegion->dataCols-3, codeword, DmtxMaskBit3);
   PlaceModule(matrixRegion, 0, matrixRegion->dataCols-2, codeword, DmtxMaskBit4);
   PlaceModule(matrixRegion, 0, matrixRegion->dataCols-1, codeword, DmtxMaskBit5);
   PlaceModule(matrixRegion, 1, matrixRegion->dataCols-3, codeword, DmtxMaskBit6);
   PlaceModule(matrixRegion, 1, matrixRegion->dataCols-2, codeword, DmtxMaskBit7);
   PlaceModule(matrixRegion, 1, matrixRegion->dataCols-1, codeword, DmtxMaskBit8);
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static void
PlaceModule(DmtxMatrixRegion *matrixRegion, int row, int col, unsigned char *codeword, DmtxBitMask mask)
{
   if(row < 0) {
      row += matrixRegion->dataRows;
      col += 4 - ((matrixRegion->dataRows+4)%8);
   }
   if(col < 0) {
      col += matrixRegion->dataCols;
      row += 4 - ((matrixRegion->dataCols+4)%8);
   }

   // If module has already been assigned then we are decoding the pattern into codewords
   if(matrixRegion->array[row*matrixRegion->dataCols+col] & DMTX_MODULE_ASSIGNED) {
      if(!(matrixRegion->array[row*matrixRegion->dataCols+col] & DMTX_MODULE_ON))
         *codeword |= mask;
      else
         *codeword &= (0xff ^ mask);
   }
   // Otherwise we are encoding the codewords into a pattern
   else {
      if(*codeword & mask)
         matrixRegion->array[row*matrixRegion->dataCols+col] |= DMTX_MODULE_ON;

      matrixRegion->array[row*matrixRegion->dataCols+col] |= DMTX_MODULE_ASSIGNED;
   }

   matrixRegion->array[row*matrixRegion->dataCols+col] |= DMTX_MODULE_VISITED;
}
