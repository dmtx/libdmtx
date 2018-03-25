/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2008, 2009 Mike Laughton. All rights reserved.
 * Copyright 2012-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 * Copyright 2016 Tim Zaman. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxdecodescheme.c
 */

/**
 * \brief  Translate encoded data stream into final output
 * \param  msg
 * \param  sizeIdx
 * \param  outputStart
 * \return void
 */
extern void
DecodeDataStream(DmtxMessage *msg, int sizeIdx, unsigned char *outputStart)
{
   //fprintf(stdout, "libdmtx::DecodeDataStream()\n");
   //int oned = sqrt(msg->arraySize);
   //for (int i=0; i<msg->arraySize; i++){
   //   fprintf(stdout, " %c.", msg->array[i]);
   //   if (i%oned==oned-1){
   //      fprintf(stdout, "\n");
   //   }
   //}

   DmtxBoolean macro = DmtxFalse;
   DmtxScheme encScheme;
   unsigned char *ptr, *dataEnd;

   msg->output = (outputStart == NULL) ? msg->output : outputStart;
   msg->outputIdx = 0;

   ptr = msg->code;
   dataEnd = ptr + dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx);

   /* Print macro header if first codeword triggers it */
   if(*ptr == DmtxValue05Macro || *ptr == DmtxValue06Macro) {
      PushOutputMacroHeader(msg, *ptr);
      macro = DmtxTrue;
   }

   while(ptr < dataEnd) {

      encScheme = GetEncodationScheme(*ptr);
      if(encScheme != DmtxSchemeAscii)
         ptr++;

      switch(encScheme) {
         case DmtxSchemeAscii:
            ptr = DecodeSchemeAscii(msg, ptr, dataEnd);
            break;
         case DmtxSchemeC40:
         case DmtxSchemeText:
            ptr = DecodeSchemeC40Text(msg, ptr, dataEnd, encScheme);
            break;
         case DmtxSchemeX12:
            ptr = DecodeSchemeX12(msg, ptr, dataEnd);
            break;
         case DmtxSchemeEdifact:
            ptr = DecodeSchemeEdifact(msg, ptr, dataEnd);
            break;
         case DmtxSchemeBase256:
            ptr = DecodeSchemeBase256(msg, ptr, dataEnd);
            break;
         default:
            /* error */
            break;
      }
   }

   /* Print macro trailer if required */
   if(macro == DmtxTrue)
      PushOutputMacroTrailer(msg);
}

/**
 * \brief  Determine next encodation scheme
 * \param  encScheme
 * \param  cw
 * \return Pointer to next undecoded codeword
 */
static int
GetEncodationScheme(unsigned char cw)
{
   DmtxScheme encScheme;

   switch(cw) {
      case DmtxValueC40Latch:
         encScheme = DmtxSchemeC40;
         break;
      case DmtxValueTextLatch:
         encScheme = DmtxSchemeText;
         break;
      case DmtxValueX12Latch:
         encScheme = DmtxSchemeX12;
         break;
      case DmtxValueEdifactLatch:
         encScheme = DmtxSchemeEdifact;
         break;
      case DmtxValueBase256Latch:
         encScheme = DmtxSchemeBase256;
         break;
      default:
         encScheme = DmtxSchemeAscii;
         break;
   }

   return encScheme;
}

/**
 *
 *
 */
static void
PushOutputWord(DmtxMessage *msg, int value)
{
   assert(value >= 0 && value < 256);

   msg->output[msg->outputIdx++] = (unsigned char)value;
}

/**
 *
 *
 */
static void
PushOutputC40TextWord(DmtxMessage *msg, C40TextState *state, int value)
{
   assert(value >= 0 && value < 256);

   msg->output[msg->outputIdx] = (unsigned char)value;

   if(state->upperShift == DmtxTrue) {
      assert(value < 128);
      msg->output[msg->outputIdx] += 128;
   }

   msg->outputIdx++;

   state->shift = DmtxC40TextBasicSet;
   state->upperShift = DmtxFalse;
}

/**
 *
 *
 */
static void
PushOutputMacroHeader(DmtxMessage *msg, int macroType)
{
   PushOutputWord(msg, '[');
   PushOutputWord(msg, ')');
   PushOutputWord(msg, '>');
   PushOutputWord(msg, 30); /* ASCII RS */
   PushOutputWord(msg, '0');

   assert(macroType == DmtxValue05Macro || macroType == DmtxValue06Macro);
   if(macroType == DmtxValue05Macro)
      PushOutputWord(msg, '5');
   else
      PushOutputWord(msg, '6');

   PushOutputWord(msg, 29); /* ASCII GS */
}

/**
 *
 *
 */
static void
PushOutputMacroTrailer(DmtxMessage *msg)
{
   PushOutputWord(msg, 30); /* ASCII RS */
   PushOutputWord(msg, 4);  /* ASCII EOT */
}

/**
 * \brief  Decode stream assuming standard ASCII encodation
 * \param  msg
 * \param  ptr
 * \param  dataEnd
 * \return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeAscii(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd)
{
   int upperShift;
   int codeword, digits;

   upperShift = DmtxFalse;

   while(ptr < dataEnd) {

      codeword = (int)(*ptr);

      if(GetEncodationScheme(*ptr) != DmtxSchemeAscii)
         return ptr;
      else
         ptr++;

      if(upperShift == DmtxTrue) {
         PushOutputWord(msg, codeword + 127);
         upperShift = DmtxFalse;
      }
      else if(codeword == DmtxValueAsciiUpperShift) {
         upperShift = DmtxTrue;
      }
      else if(codeword == DmtxValueAsciiPad) {
         assert(dataEnd >= ptr);
         assert(dataEnd - ptr <= INT_MAX);
         msg->padCount = (int)(dataEnd - ptr);
         return dataEnd;
      }
      else if(codeword == 0 || codeword >= 242) {
        return ptr;
      }
      else if(codeword <= 128) {
         PushOutputWord(msg, codeword - 1);
      }
      else if(codeword <= 229) {
         digits = codeword - 130;
         PushOutputWord(msg, digits/10 + '0');
         PushOutputWord(msg, digits - (digits/10)*10 + '0');
      }
      else if(codeword == DmtxValueFNC1) {
         if(msg->fnc1 != DmtxUndefined) {
             PushOutputWord(msg, msg->fnc1);
         }
      }
   }

   return ptr;
}

/**
 * \brief  Decode stream assuming C40 or Text encodation
 * \param  msg
 * \param  ptr
 * \param  dataEnd
 * \param  encScheme
 * \return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeC40Text(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd, DmtxScheme encScheme)
{
   int i;
   int packed;
   int c40Values[3];
   C40TextState state;

   state.shift = DmtxC40TextBasicSet;
   state.upperShift = DmtxFalse;

   assert(encScheme == DmtxSchemeC40 || encScheme == DmtxSchemeText);

   /* Unlatch is implied if only one codeword remains */
   if(dataEnd - ptr < 2)
      return ptr;

   while(ptr < dataEnd) {

      /* FIXME Also check that ptr+1 is safe to access */
      packed = (*ptr << 8) | *(ptr+1);
      c40Values[0] = ((packed - 1)/1600);
      c40Values[1] = ((packed - 1)/40) % 40;
      c40Values[2] =  (packed - 1) % 40;
      ptr += 2;

      for(i = 0; i < 3; i++) {
         if(state.shift == DmtxC40TextBasicSet) { /* Basic set */
            if(c40Values[i] <= 2) {
               state.shift = c40Values[i] + 1;
            }
            else if(c40Values[i] == 3) {
               PushOutputC40TextWord(msg, &state, ' ');
            }
            else if(c40Values[i] <= 13) {
               PushOutputC40TextWord(msg, &state, c40Values[i] - 13 + '9'); /* 0-9 */
            }
            else if(c40Values[i] <= 39) {
               if(encScheme == DmtxSchemeC40) {
                  PushOutputC40TextWord(msg, &state, c40Values[i] - 39 + 'Z'); /* A-Z */
               }
               else if(encScheme == DmtxSchemeText) {
                  PushOutputC40TextWord(msg, &state, c40Values[i] - 39 + 'z'); /* a-z */
               }
            }
         }
         else if(state.shift == DmtxC40TextShift1) { /* Shift 1 set */
            PushOutputC40TextWord(msg, &state, c40Values[i]); /* ASCII 0 - 31 */
         }
         else if(state.shift == DmtxC40TextShift2) { /* Shift 2 set */
            if(c40Values[i] <= 14) {
               PushOutputC40TextWord(msg, &state, c40Values[i] + 33); /* ASCII 33 - 47 */
            }
            else if(c40Values[i] <= 21) {
               PushOutputC40TextWord(msg, &state, c40Values[i] + 43); /* ASCII 58 - 64 */
            }
            else if(c40Values[i] <= 26) {
               PushOutputC40TextWord(msg, &state, c40Values[i] + 69); /* ASCII 91 - 95 */
            }
            else if(c40Values[i] == 27) {
               if(msg->fnc1 != DmtxUndefined) {
                   PushOutputC40TextWord(msg, &state, msg->fnc1);
               }
            }
            else if(c40Values[i] == 30) {
               state.upperShift = DmtxTrue;
               state.shift = DmtxC40TextBasicSet;
            }
         }
         else if(state.shift == DmtxC40TextShift3) { /* Shift 3 set */
            if(encScheme == DmtxSchemeC40) {
               PushOutputC40TextWord(msg, &state, c40Values[i] + 96);
            }
            else if(encScheme == DmtxSchemeText) {
               if(c40Values[i] == 0)
                  PushOutputC40TextWord(msg, &state, c40Values[i] + 96);
               else if(c40Values[i] <= 26)
                  PushOutputC40TextWord(msg, &state, c40Values[i] - 26 + 'Z'); /* A-Z */
               else
                  PushOutputC40TextWord(msg, &state, c40Values[i] - 31 + 127); /* { | } ~ DEL */
            }
         }
      }

      /* Unlatch if codeword 254 follows 2 codewords in C40/Text encodation */
      if(*ptr == DmtxValueCTXUnlatch)
         return ptr + 1;

      /* Unlatch is implied if only one codeword remains */
      if(dataEnd - ptr < 2)
         return ptr;
   }

   return ptr;
}

/**
 * \brief  Decode stream assuming X12 encodation
 * \param  msg
 * \param  ptr
 * \param  dataEnd
 * \return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeX12(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd)
{
   int i;
   int packed;
   int x12Values[3];

   /* Unlatch is implied if only one codeword remains */
   if(dataEnd - ptr < 2)
      return ptr;

   while(ptr < dataEnd) {

      /* FIXME Also check that ptr+1 is safe to access */
      packed = (*ptr << 8) | *(ptr+1);
      x12Values[0] = ((packed - 1)/1600);
      x12Values[1] = ((packed - 1)/40) % 40;
      x12Values[2] =  (packed - 1) % 40;
      ptr += 2;

      for(i = 0; i < 3; i++) {
         if(x12Values[i] == 0)
            PushOutputWord(msg, 13);
         else if(x12Values[i] == 1)
            PushOutputWord(msg, 42);
         else if(x12Values[i] == 2)
            PushOutputWord(msg, 62);
         else if(x12Values[i] == 3)
            PushOutputWord(msg, 32);
         else if(x12Values[i] <= 13)
            PushOutputWord(msg, x12Values[i] + 44);
         else if(x12Values[i] <= 90)
            PushOutputWord(msg, x12Values[i] + 51);
      }

      /* Unlatch if codeword 254 follows 2 codewords in C40/Text encodation */
      if(*ptr == DmtxValueCTXUnlatch)
         return ptr + 1;

      /* Unlatch is implied if only one codeword remains */
      if(dataEnd - ptr < 2)
         return ptr;
   }

   return ptr;
}

/**
 * \brief  Decode stream assuming EDIFACT encodation
 * \param  msg
 * \param  ptr
 * \param  dataEnd
 * \return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeEdifact(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd)
{
   int i;
   unsigned char unpacked[4];

   /* Unlatch is implied if fewer than 3 codewords remain */
   if(dataEnd - ptr < 3)
      return ptr;

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
         if(unpacked[i] == DmtxValueEdifactUnlatch) {
            assert(msg->output[msg->outputIdx] == 0); /* XXX dirty why? */
            return ptr;
         }

         PushOutputWord(msg, unpacked[i] ^ (((unpacked[i] & 0x20) ^ 0x20) << 1));
      }

      /* Unlatch is implied if fewer than 3 codewords remain */
      if(dataEnd - ptr < 3)
         return ptr;
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
      PushOutputWord(msg, value ^ (((value & 0x20) ^ 0x20) << 1));

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
 * \brief  Decode stream assuming Base 256 encodation
 * \param  msg
 * \param  ptr
 * \param  dataEnd
 * \return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeBase256(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd)
{
   int d0, d1;
   int idx;
   unsigned char *ptrEnd;

   /* Find positional index used for unrandomizing */
   assert(ptr + 1 >= msg->code);
   assert(ptr + 1 - msg->code <= INT_MAX);
   idx = (int)(ptr + 1 - msg->code);

   d0 = UnRandomize255State(*(ptr++), idx++);
   if(d0 == 0) {
      ptrEnd = dataEnd;
   }
   else if(d0 <= 249) {
      ptrEnd = ptr + d0;
   }
   else {
      d1 = UnRandomize255State(*(ptr++), idx++);
      ptrEnd = ptr + (d0 - 249) * 250 + d1;
   }

   if(ptrEnd > dataEnd)
      exit(40); /* XXX needs cleaner error handling */

   while(ptr < ptrEnd)
      PushOutputWord(msg, UnRandomize255State(*(ptr++), idx++));

   return ptr;
}
