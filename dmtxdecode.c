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
 *
 *
 */
extern DmtxDecode
dmtxDecodeInit(DmtxImage *image, DmtxPixelLoc p0, DmtxPixelLoc p1, int minGapSize)
{
   DmtxDecode decode;

   memset(&decode, 0x00, sizeof(DmtxDecode));

   decode.image = image;
   decode.grid = InitScanGrid(image, p0, p1, minGapSize);

   return decode;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static int
DecodeMatrixRegion(DmtxRegion *region)
{
   int success;

   ModulePlacementEcc200(region->array, region->code,
         region->sizeIdx, DMTX_MODULE_ON_RED | DMTX_MODULE_ON_GREEN | DMTX_MODULE_ON_BLUE);

   success = DecodeCheckErrors(region);
   if(!success) {
      fprintf(stderr, "Rejected due to ECC validation\n");
      return DMTX_FAILURE;
   }

   DecodeDataStream(region);

   return DMTX_SUCCESS;
}

/**
 * XXX
 *
 * @param
 * @return XXX
 */
static void
DecodeDataStream(DmtxRegion *region)
{
   DmtxSchemeDecode encScheme;
   unsigned char *ptr, *dataEnd;

   ptr = region->code;
   dataEnd = ptr + dmtxGetSymbolAttribute(DmtxSymAttribDataWordLength, region->sizeIdx);

   while(ptr < dataEnd) {

      ptr = NextEncodationScheme(&encScheme, ptr);

      switch(encScheme) {
         case DmtxSchemeDecodeAsciiStd:
            ptr = DecodeSchemeAsciiStd(region, ptr, dataEnd);
            break;

         case DmtxSchemeDecodeAsciiExt:
            ptr = DecodeSchemeAsciiExt(region, ptr, dataEnd);
            break;

         case DmtxSchemeDecodeC40:
         case DmtxSchemeDecodeText:
            ptr = DecodeSchemeC40Text(region, ptr, dataEnd, encScheme);
            break;

         case DmtxSchemeDecodeX12:
            ptr = DecodeSchemeX12(region, ptr, dataEnd);
            break;

         case DmtxSchemeDecodeEdifact:
            ptr = DecodeSchemeEdifact(region, ptr, dataEnd);
            break;

         case DmtxSchemeDecodeBase256:
            ptr = DecodeSchemeBase256(region, ptr, dataEnd);
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
NextEncodationScheme(DmtxSchemeDecode *encScheme, unsigned char *ptr)
{
   switch(*ptr) {
      case 230:
         *encScheme = DmtxSchemeDecodeC40;
         break;
      case 231:
         *encScheme = DmtxSchemeDecodeBase256;
         break;
      case 235:
         *encScheme = DmtxSchemeDecodeAsciiExt;
         break;
      case 238:
         *encScheme = DmtxSchemeDecodeX12;
         break;
      case 239:
         *encScheme = DmtxSchemeDecodeText;
         break;
      case 240:
         *encScheme = DmtxSchemeDecodeEdifact;
         break;
      default:
         *encScheme = DmtxSchemeDecodeAsciiStd;
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
DecodeSchemeAsciiStd(DmtxRegion *region, unsigned char *ptr, unsigned char *dataEnd)
{
   int digits;

   if(*ptr <= 128) {
      region->output[region->outputIdx++] = *ptr - 1;
   }
   else if(*ptr == 129) {
      return dataEnd;
   }
   else if(*ptr <= 229) {
      digits = *ptr - 130;
      region->output[region->outputIdx++] = digits/10 + '0';
      region->output[region->outputIdx++] = digits - (digits/10)*10 + '0';
   }

   return ptr + 1;
}

/**
 *
 * @param XXX
 * @return XXX
 */
static unsigned char *
DecodeSchemeAsciiExt(DmtxRegion *region, unsigned char *ptr, unsigned char *dataEnd)
{
   region->output[region->outputIdx++] = *ptr + 128;

   return ptr + 1;
}

/**
 *
 * @param XXX
 * @return XXX
 */
static unsigned char *
DecodeSchemeC40Text(DmtxRegion *region, unsigned char *ptr, unsigned char *dataEnd, DmtxSchemeDecode encScheme)
{
   int i;
   int packed;
   int shift = 0;
   unsigned char c40Values[3];

   assert(encScheme == DmtxSchemeDecodeC40 || encScheme == DmtxSchemeDecodeText);

   while(ptr < dataEnd) {

      /* FIXME Also check that ptr+1 is safe to access */
      packed = (*ptr << 8) | *(ptr+1);
      c40Values[0] = ((packed - 1)/1600);
      c40Values[1] = ((packed - 1)/40) % 40;
      c40Values[2] =  (packed - 1) % 40;
      ptr += 2;

      for(i = 0; i < 3; i++) {
         if(shift == 0) { /* Basic set */
            if(c40Values[i] <= 2) {
               shift = c40Values[i] + 1;
            }
            else if(c40Values[i] == 3) {
               region->output[region->outputIdx++] = ' '; /* Space */
            }
            else if(c40Values[i] <= 13) {
               region->output[region->outputIdx++] = c40Values[i] - 13 + '9'; /* 0-9 */
            }
            else if(c40Values[i] <= 39) {
               if(encScheme == DmtxSchemeDecodeC40) {
                  region->output[region->outputIdx++] = c40Values[i] - 39 + 'Z'; /* A-Z */
               }
               else if(encScheme == DmtxSchemeDecodeText) {
                  region->output[region->outputIdx++] = c40Values[i] - 39 + 'z'; /* a-z */
               }
            }
         }
         else if(shift == 1) { /* Shift 1 set */
            region->output[region->outputIdx++] = c40Values[i]; /* ASCII 0 - 31 */

            shift = 0;
         }
         else if(shift == 2) { /* Shift 2 set */
            if(c40Values[i] <= 14)
               region->output[region->outputIdx++] = c40Values[i] + 33; /* ASCII 33 - 47 */
            else if(c40Values[i] <= 21)
               region->output[region->outputIdx++] = c40Values[i] + 43; /* ASCII 58 - 64 */
            else if(c40Values[i] <= 26)
               region->output[region->outputIdx++] = c40Values[i] + 69; /* ASCII 91 - 95 */
            else if(c40Values[i] == 27)
               fprintf(stdout, "FNC1 (?)"); /* FNC1 (eh?) */
            else if(c40Values[i] == 30)
               fprintf(stdout, "Upper Shift (?)"); /* Upper Shift (eh?) */

            shift = 0;
         }
         else if(shift == 3) { /* Shift 3 set */
            if(encScheme == DmtxSchemeDecodeC40) {
               region->output[region->outputIdx++] = c40Values[i] + 96;
            }
            else if(encScheme == DmtxSchemeDecodeText) {
               if(c40Values[i] == 0)
                  region->output[region->outputIdx++] = c40Values[i] + 96;
               else if(c40Values[i] <= 26)
                  region->output[region->outputIdx++] = c40Values[i] - 26 + 'Z'; /* A-Z */
               else
                  region->output[region->outputIdx++] = c40Values[i] - 31 + 127; /* { | } ~ DEL */
            }

            shift = 0;
         }
      }

      /* Unlatch if codeword 254 follows 2 codewords in C40/Text encodation */
      if(*ptr == 254)
         return ptr + 1;

      /* Unlatch is implied if only one codeword remains */
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
DecodeSchemeX12(DmtxRegion *region, unsigned char *ptr, unsigned char *dataEnd)
{
   int i;
   int packed;
   unsigned char x12Values[3];

   while(ptr < dataEnd) {

      /* FIXME Also check that ptr+1 is safe to access */
      packed = (*ptr << 8) | *(ptr+1);
      x12Values[0] = ((packed - 1)/1600);
      x12Values[1] = ((packed - 1)/40) % 40;
      x12Values[2] =  (packed - 1) % 40;
      ptr += 2;

      for(i = 0; i < 3; i++) {
         if(x12Values[i] == 0)
            region->output[region->outputIdx++] = 13;
         else if(x12Values[i] == 1)
            region->output[region->outputIdx++] = 42;
         else if(x12Values[i] == 2)
            region->output[region->outputIdx++] = 62;
         else if(x12Values[i] == 3)
            region->output[region->outputIdx++] = 32;
         else if(x12Values[i] <= 13)
            region->output[region->outputIdx++] = x12Values[i] + 44;
         else if(x12Values[i] <= 90)
            region->output[region->outputIdx++] = x12Values[i] + 51;
      }

      /* Unlatch if codeword 254 follows 2 codewords in C40/Text encodation */
      if(*ptr == 254)
         return ptr + 1;

      /* Unlatch is implied if only one codeword remains */
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
DecodeSchemeEdifact(DmtxRegion *region, unsigned char *ptr, unsigned char *dataEnd)
{
   int i;
   unsigned char unpacked[4];

   while(ptr < dataEnd) {

      /* FIXME Also check that ptr+2 is safe to access -- shouldn't be a
         problem because I'm guessing you can guarantee there will always
         be at least 3 error codewords */
      unpacked[0] = (*ptr & 0xfc) >> 2;
      unpacked[1] = (*ptr & 0x03) << 4 | (*(ptr+1) & 0xf0) >> 4;
      unpacked[2] = (*(ptr+1) & 0x0f) << 2 | (*(ptr+2) & 0xc0) >> 6;
      unpacked[3] = *(ptr+2) & 0x3f;

      for(i = 0; i < 4; i++) {

         /* Advance input ptr (4th value comes from already-read 3rd byte) */
         if(i < 3)
            ptr++;

         /* Test for unlatch condition */
         if(unpacked[i] == 0x1f) {
            assert(region->output[region->outputIdx] == 0); /* XXX dirty why? */
            return ptr;
         }

         region->output[region->outputIdx++] = unpacked[i] ^ (((unpacked[i] & 0x20) ^ 0x20) << 1);
      }

      /* Unlatch is implied if fewer than 3 codewords remain */
      if(dataEnd - ptr < 3) {
         return ptr;
      }
   }

   return ptr;

/* XXX the following version should be safer, but requires testing before replacing the old version
   int bits = 0;
   int bitCount = 0;
   int value;

   while(ptr < dataEnd) {

      if(bitCount < 6) {
         bits = (bits << 8) | *(ptr++);
         bitCount += 8;
      }

      value = bits >> (bitCount - 6);
      bits -= (value << (bitCount - 6));
      bitCount -= 6;

      if(value == 0x1f) {
         assert(bits == 0); // should be padded with zero-value bits
         return ptr;
      }
      region->output[region->outputIdx++] = value ^ (((value & 0x20) ^ 0x20) << 1);

      // Unlatch implied if just completed triplet and 1 or 2 words are left
      if(bitCount == 0 && dataEnd - ptr - 1 > 0 && dataEnd - ptr - 1 < 3)
         return ptr;
   }

   assert(bits == 0); // should be padded with zero-value bits
   assert(bitCount == 0); // should be padded with zero-value bits
   return ptr;
*/
}

/**
 *
 * @param XXX
 * @return XXX
 */
static unsigned char *
DecodeSchemeBase256(DmtxRegion *region, unsigned char *ptr, unsigned char *dataEnd)
{
   int d0, d1;
   int i;
   unsigned char *ptrEnd;

   /* XXX i is the positional index used for unrandomizing */
   i = ptr - region->code + 1;

   d0 = UnRandomize255State(*(ptr++), i++);
   if(d0 == 0) {
      ptrEnd = dataEnd;
   }
   else if(d0 <= 249) {
      ptrEnd = ptr + d0; /* XXX not verifed */
   }
   else {
      d1 = UnRandomize255State(*(ptr++), i++);
      ptrEnd = ptr + (d0 - 249) * 250 + d1; /* XXX not verified */
   }

   if(ptrEnd > dataEnd) {
      exit(40); /* XXX needs cleaner error handling */
   }

   while(ptr < ptrEnd) {
      region->output[region->outputIdx++] = UnRandomize255State(*(ptr++), i++);
   }

   return ptr;
}

/**
 *
 * @param XXX
 * @return XXX
 */
/**
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
UnRandomize255State(unsigned char value, int idx)
{
   int pseudoRandom;
   int tmp;

   pseudoRandom = ((149 * idx) % 255) + 1;
   tmp = value - pseudoRandom;

   return (tmp >= 0) ? tmp : tmp + 256;
}
