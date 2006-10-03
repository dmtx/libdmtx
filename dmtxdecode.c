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

/* $Id: dmtxdecode.c,v 1.8 2006-10-03 05:58:30 mblaughton Exp $ */

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
 * XXX
 *
 * @param
 * @return XXX
 */
static int
DecodeRegion(DmtxMatrixRegion *matrixRegion)
{
   int success;

   ModulePlacementEcc200(matrixRegion);

   success = DecodeCheckErrors(matrixRegion);
   if(!success) {
      fprintf(stderr, "Rejected due to ECC validation\n");
      return DMTX_FAILURE;
   }

   DecodeDataStream(matrixRegion);

   return DMTX_SUCCESS;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
DecodeCheckErrors(DmtxMatrixRegion *matrixRegion)
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
DecodeDataStream(DmtxMatrixRegion *matrixRegion)
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
   int d0, d1;
   int i, iBeg, iEnd;
   unsigned char *code;

   code = matrixRegion->code;
   i = iBeg = ptr - code;

   d0 = UnRandomize255State(code, i++);
   if(d0 == 0) {
      iEnd = dataEnd - code; // XXX not verifed
   }
   else if(d0 <= 249) {
      iEnd = iBeg + d0 + 1; // XXX verifed
   }
   else {
      d1 = UnRandomize255State(code, i++);
      iEnd = iBeg + (d0 - 249) * 250 + d1; // XXX not verified
   }

   if(iEnd > dataEnd - code) {
      // XXX throw an error instead
      iEnd = dataEnd - code;
   }

   while(i < iEnd) {
      matrixRegion->output[matrixRegion->outputIdx++] = UnRandomize255State(code, i++);
   }

   return code + i;
}

/**
 *
 * @param XXX
 * @return XXX
 */
/*
static unsigned char
UnRandomize253State(unsigned char codewordValue, int codewordPosition)
{
   int pseudoRandom;
   int tmp;

   pseudoRandom = ((149 * codewordPosition) % 253) + 1;
   tmp = codewordValue - pseudoRandom;

   return (tmp >= 1) ? tmp : tmp + 254;
}
*/

/**
 *
 * @param XXX
 * @return XXX
 */
static unsigned char
UnRandomize255State(unsigned char *value, int idx)
{
   int pseudoRandom;
   int tmp;
   pseudoRandom = ((149 * (idx+1)) % 255) + 1;
   tmp = value[idx] - pseudoRandom;

   return (tmp >= 0) ? tmp : tmp + 256;
}
