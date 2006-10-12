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

/* $Id: dmtxencode.c,v 1.5 2006-10-12 20:49:40 mblaughton Exp $ */

/**
 *
 * @param XXX
 * @return XXX
 */
extern DmtxEncode *
dmtxEncodeCreate(void)
{
   DmtxEncode *encode;

   encode = (DmtxEncode *)malloc(sizeof(DmtxEncode));
   if(encode == NULL)
      exit(3);

   memset(encode, 0x00, sizeof(DmtxEncode));

   encode->scheme = DMTX_ENCODING_AUTO;
   encode->moduleSize = 5;
   encode->marginSize = 10;

   // This can be cleaned up later
   encode->matrix.gradient.isDefined = DMTX_TRUE;

   // Initialize background color to white
   encode->matrix.gradient.ray.p.R = 255.0;
   encode->matrix.gradient.ray.p.G = 255.0;
   encode->matrix.gradient.ray.p.B = 255.0;

   // Initialize foreground color to black
   encode->matrix.gradient.tMin = 0.0;
   encode->matrix.gradient.tMax = dmtxColor3Mag(&(encode->matrix.gradient.ray.p));
   encode->matrix.gradient.tMid = (encode->matrix.gradient.tMin + encode->matrix.gradient.tMax)/2.0;

   dmtxColor3Scale(&(encode->matrix.gradient.ray.c),
         &(encode->matrix.gradient.ray.p), -1.0/encode->matrix.gradient.tMax);

   dmtxMatrix3Identity(encode->xfrm);

   return encode;
}

/**
 *
 * @param XXX
 * @return XXX
 */
extern void
dmtxEncodeDestroy(DmtxEncode **encode)
{
   if(*encode == NULL)
      return;

   dmtxImageDeInit(&((*encode)->image));

   free(*encode);
   *encode = NULL;
}

/**
 *
 * @param XXX
 * @return XXX
 */
extern int
dmtxEncodeData(DmtxEncode *encode, unsigned char *inputString)
{
   // Analyze data stream for encoding type
   // Insert additional codewords to switch between more efficient encoding schemes
   EncodeText(&(encode->matrix), inputString);

   // Add pad characters to fill out required number of codewords
   // If matrix size is not specified then choose the smallest size that accomodates data
   AddPadChars(&(encode->matrix));

   // Generate error correction codewords
   GenReedSolEcc(&(encode->matrix));

   // Allocate storage for pattern
   PatternInit(&(encode->matrix));

   // Module placement in matrix
   ModulePlacementEcc200(&(encode->matrix));

   // Allocate memory for the image to be generated
   encode->image.width = (2 * encode->marginSize) + (encode->matrix.dataCols + 2) * encode->moduleSize;
   encode->image.height = (2 * encode->marginSize) + (encode->matrix.dataRows + 2) * encode->moduleSize;

   encode->image.pxl = (DmtxPixel *)malloc(encode->image.width * encode->image.height * sizeof(DmtxPixel));
   if(encode->image.pxl == NULL) {
      perror("malloc error");
      exit(2);
   }

   // Insert aligment pattern modules (if any)
   // Insert finder pattern modules
   PrintPattern(encode);

   // Clean up
   dmtxMatrixRegionDeInit(&(encode->matrix));

   return DMTX_SUCCESS;
}

/**
 *
 * @param XXX
 * @return XXX
 */
static void
EncodeText(DmtxMatrixRegion *matrixRegion, unsigned char *inputText)
{
   unsigned char *rangeStart, *ptr;
   unsigned char *codewordPtr;
   unsigned char buf[1024];
   int unpaddedDataSize;

   /* initial implementation: latching to new encodation scheme becomes
    * justified when latching overhead is offset by benefit as compared to
    * staying with basic ASCII.  Question: If switching from one non-ASCII
    * scheme to another non-ASCII scheme do I have to switch back to ASCII
    * and then to the new one?  Or instead can I switch directly to the new
    * scheme.
    *
    * In any event, depending on the answer to that question, it might make
    * sense in the future to weigh latching benefit as compared to CURRENT
    * encodation scheme.
    */

   // XXX first read into a big buffer
   memset(buf, 0x00, 1024);

   fprintf(stdout, "\ninput:   \"%s\"", inputText);

   rangeStart = ptr = inputText;
   codewordPtr = buf;

   do {
      if(isdigit(*ptr) && isdigit(*rangeStart)) {
         ptr++;
      }
      else {
         // If there are more than 2 characters of digits then write them (even number)
         if(ptr - rangeStart > 1) {
            rangeStart = ptr = WriteAscii2Digit(&codewordPtr, rangeStart, ptr - 1);
         }
         // And then write the remaining characters as straight ASCII
         else {
            rangeStart = ptr = WriteAsciiChar(&codewordPtr, rangeStart);
         }
      }
   } while(*rangeStart != '\0');

   unpaddedDataSize = codewordPtr - buf;

   // Loop determines matrix size and data/error counts
   for(matrixRegion->sizeIdx = 0; matrixRegion->sizeIdx < 24; matrixRegion->sizeIdx++) {
      if(dataWordLength[matrixRegion->sizeIdx] >= unpaddedDataSize)
         break;
   }

   matrixRegion->padSize = dataWordLength[matrixRegion->sizeIdx] - unpaddedDataSize;
   matrixRegion->codeSize = dataWordLength[matrixRegion->sizeIdx] + errorWordLength[matrixRegion->sizeIdx];

   matrixRegion->dataRows = matrixRegion->dataCols = dataRegionSize[matrixRegion->sizeIdx];
// fprintf(stdout, "\n\nsize:    %dx%d w/ %d error codewords\n", rows, cols, errorWordLength(matrixRegion->sizeIdx));

   matrixRegion->code = (unsigned char *)malloc(sizeof(unsigned char) * matrixRegion->codeSize);
   if(matrixRegion->code == NULL) {
      perror("dmtxEncodeData");
      exit(7);
   }
   memset(matrixRegion->code, 0x00, sizeof(unsigned char) * matrixRegion->codeSize);
   memcpy(matrixRegion->code, buf, unpaddedDataSize);
}

/**
 *
 * @param XXX
 * @return XXX
 */
static unsigned char *
WriteAscii2Digit(unsigned char **dest, unsigned char *src, unsigned char *srcEnd)
{
   while(src < srcEnd) {
      *((*dest)++) = 10 * (*src - '0') + (*(src + 1) - '0') + 130;
      fprintf(stdout, "\ndigits:  \"%c%c\" ==> %d", *src, *(src + 1), *((*dest)-1)); fflush(stdout);
      src += 2;
   }

   return src;
}

/**
 *
 * @param XXX
 * @return XXX
 */
static unsigned char *
WriteAsciiChar(unsigned char **dest, unsigned char *src)
{
   fprintf(stdout, "\nascii:   \"%c\" ==> %d", *src, *src + 1); fflush(stdout);
   *((*dest)++) = *(src++) + 1;

   return src;
}

/**
 *
 * @param XXX
 * @return XXX
 */
static void
AddPadChars(DmtxMatrixRegion *matrix)
{
   int i;
   int dataSize;
   int padBegin;

   // XXX we can do it cleaner than this // XXX this is probably broken too (compare to older version for fix)
   dataSize = dataWordLength[matrix->sizeIdx];
   padBegin = dataSize - matrix->padSize;
   for(i = padBegin; i < dataSize; i++) {
      matrix->code[i] = (i == padBegin) ? 129 : Randomize253State(129, i + 1);
   }
}

/**
 *
 * @param XXX
 * @return XXX
 */
static unsigned char
Randomize253State(unsigned char codewordValue, int codewordPosition)
{
   int pseudoRandom;
   int tmp;

   pseudoRandom = ((149 * codewordPosition) % 253) + 1;
   tmp = codewordValue + pseudoRandom;

   return (tmp <= 254) ? tmp : tmp - 254;
}

/**
 *
 * @param XXX
 * @return XXX
 */
/* XXX uncomment this function later -- will need for encoding Base256
static unsigned char
Randomize255State(unsigned char codewordValue, int codewordPosition)
{
   int pseudoRandom;
   int tmp;

   pseudoRandom = ((149 * codewordPosition) % 255) + 1;
   tmp = codewordValue + pseudoRandom;

   return (tmp <= 255) ? tmp : tmp - 256;
}
*/

/**
 *
 * @param XXX
 * @return XXX
 */
static void
PatternInit(DmtxMatrixRegion *matrixRegion)
{
   int patternSize;

   if(matrixRegion == NULL)
      exit(1); // XXX find better error handling here

   patternSize = sizeof(unsigned char) * matrixRegion->dataRows * matrixRegion->dataCols;
   matrixRegion->array = (unsigned char *)malloc(patternSize);
   if(matrixRegion->array == NULL)
      exit(2); // XXX find better error handling here

   memset(matrixRegion->array, 0x00, patternSize);
}

/**
 *
 * @param XXX
 * @return XXX
 */
static void
PrintPattern(DmtxEncode *encode)
{
   int row, col, dataRow, dataCol;
   float sxy, txy;
   DmtxMatrix3 m1, m2, m3;
   DmtxVector2 vIn, vOut;
   DmtxPixel black, white;

   // Print ASCII rendition of barcode pattern
   for(row = 0; row < encode->matrix.dataRows + 2; row++) {
      for(col = 0; col < encode->matrix.dataCols + 2; col++) {

         if(col == 0 || row == encode->matrix.dataRows + 1) {
            fprintf(stdout, "XX");
         }
         else if (row == 0) {
            fprintf(stdout, "%s", (col & 0x01) ? "  " : "XX");
         }
         else if (col == encode->matrix.dataCols + 1) {
            fprintf(stdout, "%s", (row & 0x01) ? "XX" : "  ");
         }
         else {
            dataRow = row - 1;
            dataCol = col - 1;

            fprintf(stdout, "%s", (encode->matrix.array[dataRow *
                  encode->matrix.dataCols + dataCol] & DMTX_MODULE_ON) ?
                  "XX" : "  ");
         }
      }
      fprintf(stdout, "\n");
   }
   fprintf(stdout, "\n");

   txy = -1.0 * (encode->marginSize + encode->moduleSize);
   sxy = 1.0/encode->moduleSize;

   dmtxMatrix3Translate(m1, txy, txy);
   dmtxMatrix3Scale(m2, sxy, -sxy);
   dmtxMatrix3Translate(m3, -0.5, encode->matrix.dataRows - 0.5);
   dmtxMatrix3Multiply(encode->xfrm, m1, m2);
   dmtxMatrix3MultiplyBy(encode->xfrm, m3);

// dmtxMatrix3Rotate(m4, 0.1);
// dmtxMatrix3MultiplyBy(encode->xfrm, m4);

   memset(&black, 0x00, sizeof(DmtxPixel));
   memset(&white, 0xff, sizeof(DmtxPixel));

   // Print raster version of barcode pattern
   // IMPORTANT: DmtxImage is stored with its origin at bottom-right
   // (unlike common image file formats) to preserve "right-handed" 2D space
   for(row = 0; row < encode->image.height; row++) {
      for(col = 0; col < encode->image.width; col++) {

         vIn.X = col;
         vIn.Y = row;
         dmtxMatrix3VMultiply(&vOut, &vIn, encode->xfrm);

         dataCol = (int)(vOut.X + ((vOut.X > 0) ? 0.5 : -0.5));
         dataRow = (int)(vOut.Y + ((vOut.Y > 0) ? 0.5 : -0.5));

         // Margin
         if(dataCol < -1 || dataCol > encode->matrix.dataCols ||
            dataRow < -1 || dataRow > encode->matrix.dataRows) {
            encode->image.pxl[row * encode->image.width + col] = white;
         }
         // Finder bars
         else if(dataCol == -1 || dataRow == encode->matrix.dataRows) {
            encode->image.pxl[row * encode->image.width + col] = black;
         }
         // Top calibration bar
         else if(dataRow == -1) {
            encode->image.pxl[row * encode->image.width + col] = (dataCol & 0x01) ? black : white;
         }
         // Right calibration bar
         else if(dataCol == encode->matrix.dataCols) {
            encode->image.pxl[row * encode->image.width + col] = (dataRow & 0x01) ? white : black;
         }
         else if(encode->matrix.array[dataRow * encode->matrix.dataCols + dataCol] & DMTX_MODULE_ON) {
            encode->image.pxl[row * encode->image.width + col] = black;
         }
         else {
            encode->image.pxl[row * encode->image.width + col] = white;
         }
      }
   }
}
