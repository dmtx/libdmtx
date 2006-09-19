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

/* $Id: dmtxdecode.c,v 1.4 2006-09-19 05:27:36 mblaughton Exp $ */

/**
 *
 * @param XXX
 * @return XXX
 */
extern DmtxDecode *
dmtxDecodeStructCreate(void)
{
   DmtxDecode *decode;

   decode = (DmtxDecode *)malloc(sizeof(DmtxDecode));
   if(decode == NULL) {
      exit(1); // XXX better error strategy required here
   }

   memset(decode, 0x00, sizeof(DmtxDecode));

   return decode;
}

/**
 *
 * @param XXX
 * @return XXX
 */
extern void
dmtxDecodeStructDestroy(DmtxDecode **decode)
{
   // Call this to free the scanned Data Matrix list from memory
   dmtxScanStartNew(*decode);

   if(*decode != NULL) {
      free(*decode);
   }

   *decode = NULL;
}

/**
 *
 * @param decode   Pointer to DmtxDecode information struct
 * @return         success (DMTX_SUCCESS|DMTX_FAILURE)
 */
extern int
dmtxDecodeGetMatrixCount(DmtxDecode *decode)
{
   return decode->matrixCount;
}

/**
 *
 * @param decode   Pointer to DmtxDecode information struct
 * @return         success (DMTX_SUCCESS|DMTX_FAILURE)
 */
extern DmtxMatrixRegion *
dmtxDecodeGetMatrix(DmtxDecode *decode, int index)
{
   if(index < 0 || index >= decode->matrixCount)
      return NULL;

   return &(decode->matrix[index]);
}

/**
 *
 * @param decode   Pointer to DmtxDecode information struct
 * @return         success (DMTX_SUCCESS|DMTX_FAILURE)
 */
extern void
dmtxScanStartNew(DmtxDecode *decode)
{
   int i;

   for(i = 0; i < decode->matrixCount; i++)
      dmtxMatrixRegionDeInit(&(decode->matrix[i]));

   decode->matrixCount = 0;
}

/**
 *
 * @param XXX
 * @return XXX
 */
extern void
dmtxSetStepScanCallback(DmtxDecode *decode, void (* func)(DmtxDecode *, DmtxScanRange *, DmtxJumpScan *))
{
   if(decode == NULL) {
      exit(1); // XXX better error handling strategy
   }

   decode->stepScanCallback = func;
}

/**
 *
 * @param XXX
 * @return XXX
 */
extern void
dmtxSetCrossScanCallback(DmtxDecode *decode, void (* func)(DmtxScanRange *, DmtxGradient *, DmtxEdgeScan *))
{
   if(decode == NULL) {
      exit(1); // XXX better error handling strategy
   }

   decode->crossScanCallback = func;
}

/**
 *
 * @param XXX
 * @return XXX
 */
extern void
dmtxSetFollowScanCallback(DmtxDecode *decode, void (* func)(DmtxEdgeFollower *))
{
   if(decode == NULL) {
      exit(1); // XXX better error handling strategy
   }

   decode->followScanCallback = func;
}

/**
 *
 * @param XXX
 * @return XXX
 */
extern void
dmtxSetFinderBarCallback(DmtxDecode *decode, void (* func)(DmtxRay2 *))
{
   if(decode == NULL) {
      exit(1); // XXX better error handling strategy
   }

   decode->finderBarCallback = func;
}

/**
 *
 * @param XXX
 * @return XXX
 */
extern void
dmtxSetBuildMatrixCallback2(DmtxDecode *decode, void (* func)(DmtxFinderBar *, DmtxMatrixRegion *))
{
   if(decode == NULL) {
      exit(1); // XXX better error handling strategy
   }

   decode->buildMatrixCallback2 = func;
}

/**
 *
 * @param XXX
 * @return XXX
 */
extern void
dmtxSetBuildMatrixCallback3(DmtxDecode *decode, void (* func)(DmtxMatrix3))
{
   if(decode == NULL) {
      exit(1); // XXX better error handling strategy
   }

   decode->buildMatrixCallback3 = func;
}

/**
 *
 * @param XXX
 * @return XXX
 */
extern void
dmtxSetBuildMatrixCallback4(DmtxDecode *decode, void (* func)(DmtxMatrix3))
{
   if(decode == NULL) {
      exit(1); // XXX better error handling strategy
   }

   decode->buildMatrixCallback4 = func;
}

/**
 *
 * @param XXX
 * @return XXX
 */
extern void
dmtxSetPlotPointCallback(DmtxDecode *decode, void (* func)(DmtxVector2, int, int, int))
{
   if(decode == NULL) {
      exit(1); // XXX better error handling strategy
   }

   decode->plotPointCallback = func;
}

/**
 *
 * @param XXX
 * @return XXX
 */
extern void
dmtxSetXfrmPlotPointCallback(DmtxDecode *decode, void (* func)(DmtxVector2, DmtxMatrix3, int, int))
{
   if(decode == NULL) {
      exit(1); // XXX better error handling strategy
   }

   decode->xfrmPlotPointCallback = func;
}

/**
 *
 * @param XXX
 * @return XXX
 */
extern void
dmtxSetFinalCallback(DmtxDecode *decode, void (* func)(DmtxMatrixRegion *))
{
   if(decode == NULL) {
      exit(1); // XXX better error handling strategy
   }

   decode->finalCallback = func;
}

/**
 *
 * @param XXX
 * @return XXX
 */
extern void
dmtxSetPlotModuleCallback(DmtxDecode *decode, void (* func)(DmtxDecode *, DmtxMatrixRegion *, int, int, DmtxVector3))
{
   if(decode == NULL) {
      exit(1); // XXX better error handling strategy
   }

   decode->plotModuleCallback = func;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
MatrixRegionRead(DmtxMatrixRegion *matrixRegion, DmtxDecode *decode)
{
   int i;
   int status = DMTX_FAILURE;
   int sizeIdx;
   int sizeEstimate;
   int adjust[] = { 0, -2, +2, -4, +4, -6, +6 };
   int adjustAttempts = 7;
   int rowColAttempts[24];

   sizeEstimate = MatrixRegionEstimateSize(matrixRegion, decode);

   memset(rowColAttempts, 0x00, sizeof(rowColAttempts));
   assert(sizeof(symbolSize) / sizeof(int) == 24);

   for(i = 0; i < adjustAttempts; i++) {
      matrixRegion->dataRows = matrixRegion->dataCols = sizeEstimate - 2 + adjust[i];

      if(matrixRegion->dataRows < 8 || matrixRegion->dataRows > 132)
         continue;

      status = PatternReadNonDataModules(matrixRegion, decode);
      if(status == DMTX_SUCCESS)
         break;
   }

   // If we try all adjustments and still can't get a good read then just give up
   if(status == DMTX_FAILURE)
      return status; // XXX bad read

   // Determine how many data words and error words will be stored in this size of grid
   // First loop determines matrix size and data/error counts
   for(sizeIdx = 0; sizeIdx < 9; sizeIdx++) { // XXX this will not work for structured append arrays
      if(symbolSize[sizeIdx] == matrixRegion->dataRows + 2) {
         matrixRegion->sizeIdx = sizeIdx;
         break;
      }
   }

   matrixRegion->arraySize = sizeof(unsigned char) * matrixRegion->dataRows * matrixRegion->dataCols;
   matrixRegion->array = (unsigned char *)malloc(matrixRegion->arraySize);
   memset(matrixRegion->array, 0x00, matrixRegion->arraySize);
   if(matrixRegion->array == NULL) {
      perror("Malloc failed");
      exit(2); // XXX find better error handling here
   }

   matrixRegion->codeSize = sizeof(unsigned char) * dataWordLength[sizeIdx] + errorWordLength[sizeIdx];
   matrixRegion->code = (unsigned char *)malloc(matrixRegion->codeSize);
   memset(matrixRegion->code, 0x00, matrixRegion->codeSize);
   if(matrixRegion->code == NULL) {
      perror("Malloc failed");
      exit(2); // XXX find better error handling here
   }

   // XXX not sure if this is the right place or even the right approach.
   // Trying to allocate memory for the decoded data stream and will
   // initially assume that decoded data will not be larger than 2x encoded data
   matrixRegion->outputSize = sizeof(unsigned char) * matrixRegion->codeSize * 10;
   matrixRegion->output = (unsigned char *)malloc(matrixRegion->outputSize);
   memset(matrixRegion->output, 0x00, matrixRegion->outputSize);
   if(matrixRegion->output == NULL) {
      perror("Malloc failed");
      exit(2); // XXX find better error handling here
   }

   return DMTX_SUCCESS; // XXX good read
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
PatternReadNonDataModules(DmtxMatrixRegion *matrixRegion, DmtxDecode *decode)
{
   int row, col;
   int errors = 0;
   float t, tRatio;
   DmtxVector2 p, p0;
   DmtxVector3 color, colorAdjust;

   for(row = matrixRegion->dataRows; row >= -1; row--) {
      for(col = matrixRegion->dataCols; col >= -1; col--) {

         if(row > -1 && row < matrixRegion->dataRows) {
            if(col > -1 && col < matrixRegion->dataCols) {
               continue;
            }
         }

         // the following "+ 2" hack is to allow for the Finder and Calibration bars
         p.X = (100.0/(matrixRegion->dataCols + 2)) * (col + 1.5);
         p.Y = (100.0/(matrixRegion->dataRows + 2)) * (row + 1.5);

         dmtxMatrix3VMultiply(&p0, &p, matrixRegion->fit2raw);

         dmtxColorFromImage2(&color, &(decode->image), p0);
         t = dmtxDistanceAlongRay3(&(matrixRegion->gradient.ray), &color);
         tRatio = (t - matrixRegion->gradient.tMin)/(matrixRegion->gradient.tMax - matrixRegion->gradient.tMin);
         if(tRatio > 0.5)
            dmtxPointAlongRay3(&colorAdjust, &(matrixRegion->gradient.ray), matrixRegion->gradient.tMax);
         else
            dmtxPointAlongRay3(&colorAdjust, &(matrixRegion->gradient.ray), matrixRegion->gradient.tMin);

         if(decode && decode->plotPointCallback)
            (*(decode->plotPointCallback))(p0, 5, 1, DMTX_DISPLAY_POINT);

         if(decode && decode->plotModuleCallback)
            (*(decode->plotModuleCallback))(decode, matrixRegion, row, col, colorAdjust);

         // Check that finder and calibration bar modules have correct colors
         if(col == -1 || row == -1) {
            if(tRatio < 0.5) {
//             fprintf(stdout, "Error: %g < 0.5\n", tRatio);
               if(decode && decode->plotPointCallback)
                  (*(decode->plotPointCallback))(p0, 1, 1, DMTX_DISPLAY_SQUARE);
               errors++;
            }
         }
         else if(row == matrixRegion->dataRows || col == matrixRegion->dataCols) {
            if((row + col) % 2) {
               if(tRatio < 0.5) {
//                fprintf(stdout, "Error: %g < 0.5\n", tRatio);
                  if(decode && decode->plotPointCallback)
                     (*(decode->plotPointCallback))(p0, 1, 1, DMTX_DISPLAY_SQUARE);
                  errors++;
               }
            }
            else {
               if(tRatio > 0.5) {
//                fprintf(stdout, "Error: %g > 0.5\n", tRatio);
                  if(decode && decode->plotPointCallback)
                     (*(decode->plotPointCallback))(p0, 1, 1, DMTX_DISPLAY_SQUARE);
                  errors++;
               }
            }
         }

         if(errors > 4)
            return DMTX_FAILURE; // Too many errors detected
      }
   }

   return DMTX_SUCCESS;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static void
PopulateArrayFromImage(DmtxMatrixRegion *matrixRegion, DmtxDecode *decode)
{
   int row, col;
   float t, tRatio;
   DmtxVector2 sampleLocFit, sampleLocRaw;
   DmtxVector3 sampleColor;

   memset(matrixRegion->array, 0x00, matrixRegion->arraySize);

   for(row = 0; row < matrixRegion->dataRows; row++) {
      for(col = 0; col < matrixRegion->dataCols; col++) {

         // XXX The following sample calc could/should be abstracted by a function call

         // the following "+ 2" hack is to allow for the Finder and Calibration bars
         sampleLocFit.X = (100.0/(matrixRegion->dataCols + 2)) * (col + 1.5);

         // this "swap" is because the array's origin is top-left whereas everything else is stored with origin at bottom-left
         sampleLocFit.Y = 100 - (100.0/(matrixRegion->dataRows + 2)) * (row + 1.5);

         dmtxMatrix3VMultiply(&sampleLocRaw, &sampleLocFit, matrixRegion->fit2raw);

         if(decode && decode->plotPointCallback)
            (*(decode->plotPointCallback))(sampleLocRaw, 1, 1, DMTX_DISPLAY_POINT);

         dmtxColorFromImage2(&sampleColor, &(decode->image), sampleLocRaw);
         t = dmtxDistanceAlongRay3(&(matrixRegion->gradient.ray), &sampleColor);
         tRatio = (t - matrixRegion->gradient.tMin)/(matrixRegion->gradient.tMax - matrixRegion->gradient.tMin);

         // Value has been assigned, but not visited
         matrixRegion->array[row*matrixRegion->dataCols+col] |= (tRatio < 0.5) ? DMTX_MODULE_ON : DMTX_MODULE_OFF;
         matrixRegion->array[row*matrixRegion->dataCols+col] |= DMTX_MODULE_ASSIGNED;

         // Draw "ideal" barcode
         if(decode && decode->plotModuleCallback)
            (*(decode->plotModuleCallback))(decode, matrixRegion, matrixRegion->dataRows - row - 1, col, sampleColor);
      }
   }
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
CheckErrors(DmtxMatrixRegion *matrixRegion)
{
   int i, j;
   unsigned char reg, a;

   for(i = 0; i < errorWordLength[matrixRegion->sizeIdx]; i++) {
      a = aLogVal[i+1];

      for(j = 0, reg = 0; j < matrixRegion->codeSize; j++)
         reg = GfSum(matrixRegion->code[j], GfProduct(a, reg));

      if(reg != 0)
         return DMTX_FAILURE;
   }

   return DMTX_SUCCESS;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static void
DataStreamDecode(DmtxMatrixRegion *matrixRegion)
{
   DmtxEncScheme encScheme;
   unsigned char *ptr, *dataEnd;

   ptr = matrixRegion->code;
   dataEnd = ptr + dataWordLength[matrixRegion->sizeIdx];

   while(ptr < dataEnd) {

      ptr = NextEncodationScheme(&encScheme, ptr);

      switch(encScheme) {
         case DmtxSchemeAsciiStd:
            ptr = DecodeSchemeAsciiStd(matrixRegion, ptr, dataEnd);
            break;

         case DmtxSchemeAsciiExt:
            ptr = DecodeSchemeAsciiExt(matrixRegion, ptr, dataEnd);
            break;

         case DmtxSchemeC40:
         case DmtxSchemeText:
            ptr = DecodeSchemeC40Text(matrixRegion, ptr, dataEnd, encScheme);
            break;

         case DmtxSchemeX12:
            ptr = DecodeSchemeX12(matrixRegion, ptr, dataEnd);
            break;

         case DmtxSchemeEdifact:
            ptr = DecodeSchemeEdifact(matrixRegion, ptr, dataEnd);
            break;

         case DmtxSchemeBase256:
            ptr = DecodeSchemeBase256(matrixRegion, ptr, dataEnd);
            break;
      }
   }
}

/**
 *
 * @param XXX
 * @return XXX
 */
static unsigned char *
NextEncodationScheme(DmtxEncScheme *encScheme, unsigned char *ptr)
{
   switch(*ptr) {
      case 230:
         *encScheme = DmtxSchemeC40;
         break;
      case 231:
         *encScheme = DmtxSchemeBase256;
         break;
      case 235:
         *encScheme = DmtxSchemeAsciiExt;
         break;
      case 238:
         *encScheme = DmtxSchemeX12;
         break;
      case 239:
         *encScheme = DmtxSchemeText;
         break;
      case 240:
         *encScheme = DmtxSchemeEdifact;
         break;
      default:
         *encScheme = DmtxSchemeAsciiStd;
         return ptr;
   }

   return ptr + 1;
}

/**
 *
 * @param XXX
 * @return XXX
 */
static unsigned char *
DecodeSchemeAsciiStd(DmtxMatrixRegion *matrixRegion, unsigned char *ptr, unsigned char *dataEnd)
{
   int digits;

   if(*ptr <= 128) {
      matrixRegion->output[matrixRegion->outputIdx++] = *ptr - 1;
   }
   else if(*ptr == 129) {
      return dataEnd;
   }
   else if(*ptr <= 229) {
      digits = *ptr - 130;
      matrixRegion->output[matrixRegion->outputIdx++] = digits/10 + '0';
      matrixRegion->output[matrixRegion->outputIdx++] = digits - (digits/10)*10 + '0';
   }

   return ptr + 1;
}

/**
 *
 * @param XXX
 * @return XXX
 */
static unsigned char *
DecodeSchemeAsciiExt(DmtxMatrixRegion *matrixRegion, unsigned char *ptr, unsigned char *dataEnd)
{
   matrixRegion->output[matrixRegion->outputIdx++] = *ptr + 128;

   return ptr + 1;
}

/**
 *
 * @param XXX
 * @return XXX
 */
static unsigned char *
DecodeSchemeC40Text(DmtxMatrixRegion *matrixRegion, unsigned char *ptr, unsigned char *dataEnd, DmtxEncScheme encScheme)
{
   int i;
   int packed;
   int shift = 0;
   unsigned char c40Values[3];

   assert(encScheme == DmtxSchemeC40 || encScheme == DmtxSchemeText);

   while(ptr < dataEnd) {

      // FIXME Also check that ptr+1 is safe to access
      packed = (*ptr << 8) | *(ptr+1);
      c40Values[0] = ((packed - 1)/1600);
      c40Values[1] = ((packed - 1)/40) % 40;
      c40Values[2] =  (packed - 1) % 40;
      ptr += 2;

      for(i = 0; i < 3; i++) {
         if(shift == 0) { // Basic set
            if(c40Values[i] <= 2) {
               shift = c40Values[i] + 1;
            }
            else if(c40Values[i] == 3) {
               matrixRegion->output[matrixRegion->outputIdx++] = ' '; // Space
            }
            else if(c40Values[i] <= 13) {
               matrixRegion->output[matrixRegion->outputIdx++] = c40Values[i] - 13 + '9'; // 0-9
            }
            else if(c40Values[i] <= 39) {
               if(encScheme == DmtxSchemeC40) {
                  matrixRegion->output[matrixRegion->outputIdx++] = c40Values[i] - 39 + 'Z'; // A-Z
               }
               else if(encScheme == DmtxSchemeText) {
                  matrixRegion->output[matrixRegion->outputIdx++] = c40Values[i] - 39 + 'z'; // a-z
               }
            }
         }
         else if(shift == 1) { // Shift 1 set
            matrixRegion->output[matrixRegion->outputIdx++] = c40Values[i]; // ASCII 0 - 31

            shift = 0;
         }
         else if(shift == 2) { // Shift 2 set
            if(c40Values[i] <= 14)
               matrixRegion->output[matrixRegion->outputIdx++] = c40Values[i] + 33; // ASCII 33 - 47
            else if(c40Values[i] <= 21)
               matrixRegion->output[matrixRegion->outputIdx++] = c40Values[i] + 43; // ASCII 58 - 64
            else if(c40Values[i] <= 26)
               matrixRegion->output[matrixRegion->outputIdx++] = c40Values[i] + 69; // ASCII 91 - 95
            else if(c40Values[i] == 27)
               fprintf(stdout, "FNC1 (?)"); // FNC1 (eh?)
            else if(c40Values[i] == 30)
               fprintf(stdout, "Upper Shift (?)"); // Upper Shift (eh?)

            shift = 0;
         }
         else if(shift == 3) { // Shift 3 set
            if(encScheme == DmtxSchemeC40) {
               matrixRegion->output[matrixRegion->outputIdx++] = c40Values[i] + 96;
            }
            else if(encScheme == DmtxSchemeText) {
               if(c40Values[i] == 0)
                  matrixRegion->output[matrixRegion->outputIdx++] = c40Values[i] + 96;
               else if(c40Values[i] <= 26)
                  matrixRegion->output[matrixRegion->outputIdx++] = c40Values[i] - 26 + 'Z'; // A-Z
               else
                  matrixRegion->output[matrixRegion->outputIdx++] = c40Values[i] - 31 + 127; // { | } ~ DEL
            }

            shift = 0;
         }
      }

      // Unlatch if codeword 254 follows 2 codewords in C40/Text encodation
      if(*ptr == 254)
         return ptr + 1;

      // Unlatch is implied if only one codeword remains
      if(dataEnd - ptr == 1)
         return ptr;
   }

   return ptr;
}

/**
 *
 * @param XXX
 * @return XXX
 */
static unsigned char *
DecodeSchemeX12(DmtxMatrixRegion *matrixRegion, unsigned char *ptr, unsigned char *dataEnd)
{
   int i;
   int packed;
   unsigned char x12Values[3];

   while(ptr < dataEnd) {

      // FIXME Also check that ptr+1 is safe to access
      packed = (*ptr << 8) | *(ptr+1);
      x12Values[0] = ((packed - 1)/1600);
      x12Values[1] = ((packed - 1)/40) % 40;
      x12Values[2] =  (packed - 1) % 40;
      ptr += 2;

      for(i = 0; i < 3; i++) {
         if(x12Values[i] == 0)
            matrixRegion->output[matrixRegion->outputIdx++] = 13;
         else if(x12Values[i] == 1)
            matrixRegion->output[matrixRegion->outputIdx++] = 42;
         else if(x12Values[i] == 2)
            matrixRegion->output[matrixRegion->outputIdx++] = 62;
         else if(x12Values[i] == 3)
            matrixRegion->output[matrixRegion->outputIdx++] = 32;
         else if(x12Values[i] <= 13)
            matrixRegion->output[matrixRegion->outputIdx++] = x12Values[i] + 44;
         else if(x12Values[i] <= 90)
            matrixRegion->output[matrixRegion->outputIdx++] = x12Values[i] + 51;
      }

      // Unlatch if codeword 254 follows 2 codewords in C40/Text encodation
      if(*ptr == 254)
         return ptr + 1;

      // Unlatch is implied if only one codeword remains
      if(dataEnd - ptr == 1)
         return ptr;
   }

   return ptr;
}

/**
 *
 * @param XXX
 * @return XXX
 */
static unsigned char *
DecodeSchemeEdifact(DmtxMatrixRegion *matrixRegion, unsigned char *ptr, unsigned char *dataEnd)
{
   int i;
   unsigned char unpacked[4];

   while(ptr < dataEnd) {

      // FIXME Also check that ptr+2 is safe to access
      unpacked[0] = (*ptr & 0xfc) >> 2;
      unpacked[1] = (*ptr & 0x03) << 4 | (*(ptr+1) & 0xf0) >> 4;
      unpacked[2] = (*(ptr+1) & 0x0f) << 2 | (*(ptr+2) & 0xc0) >> 6;
      unpacked[3] = *(ptr+2) & 0x3f;
      ptr += 3;

      for(i = 0; i < 4; i++) {
         // Test for unlatch condition
         if(unpacked[i] == 0x1f)
            return ptr;

         matrixRegion->output[matrixRegion->outputIdx++] = unpacked[i] ^ (((unpacked[i] & 0x20) ^ 0x20) << 1);
      }

      // Unlatch is implied if fewer than 3 codewords remain
      if(dataEnd - ptr < 3) {
         return ptr;
      }
   }

   return ptr;
}

/**
 *
 * @param XXX
 * @return XXX
 */
static unsigned char *
DecodeSchemeBase256(DmtxMatrixRegion *matrixRegion, unsigned char *ptr, unsigned char *dataEnd)
{
   // XXX unlatch test goes here

   return ptr + 1;
}
