/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2008, 2009 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in parent directory for full terms of
 * use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 */

/**
 * $Id: file 1153 2011-01-13 08:34:06Z mblaughton $
 * \file dmtxencode.c
 * \brief Encode messages
 */

#undef ISDIGIT
#define ISDIGIT(n) (n > 47 && n < 58)

/**
 * @brief  Initialize encode struct with default values
 * @return Initialized DmtxEncode struct
 */
extern DmtxEncode *
dmtxEncodeCreate(void)
{
   DmtxEncode *enc;

   enc = (DmtxEncode *)calloc(1, sizeof(DmtxEncode));
   if(enc == NULL)
      return NULL;

   enc->scheme = DmtxSchemeAscii;
   enc->sizeIdxRequest = DmtxSymbolSquareAuto;
   enc->marginSize = 10;
   enc->moduleSize = 5;
   enc->pixelPacking = DmtxPack24bppRGB;
   enc->imageFlip = DmtxFlipNone;
   enc->rowPadBytes = 0;

   /* Initialize background color to white */
/* enc.region.gradient.ray.p.R = 255.0;
   enc.region.gradient.ray.p.G = 255.0;
   enc.region.gradient.ray.p.B = 255.0; */

   /* Initialize foreground color to black */
/* enc.region.gradient.tMin = 0.0;
   enc.region.gradient.tMax = xyz; */

   dmtxMatrix3Identity(enc->xfrm);

   return enc;
}

/**
 * @brief  Deinitialize encode struct
 * @param  enc
 * @return void
 */
extern DmtxPassFail
dmtxEncodeDestroy(DmtxEncode **enc)
{
   if(enc == NULL || *enc == NULL)
      return DmtxFail;

   /* Free pixel array allocated in dmtxEncodeDataMatrix() */
   if((*enc)->image != NULL && (*enc)->image->pxl != NULL) {
      free((*enc)->image->pxl);
      (*enc)->image->pxl = NULL;
   }

   dmtxImageDestroy(&((*enc)->image));
   dmtxMessageDestroy(&((*enc)->message));

   free(*enc);

   *enc = NULL;

   return DmtxPass;
}

/**
 * @brief  Set encoding behavior property
 * @param  enc
 * @param  prop
 * @param  value
 * @return DmtxPass | DmtxFail
 */
extern DmtxPassFail
dmtxEncodeSetProp(DmtxEncode *enc, int prop, int value)
{
   switch(prop) {

      /* Encoding details */
      case DmtxPropScheme:
         enc->scheme = value;
         break;
      case DmtxPropSizeRequest:
         if(value == DmtxSymbolShapeAuto)
            return DmtxFail;
         enc->sizeIdxRequest = value;
         break;

      /* Presentation details */
      case DmtxPropMarginSize:
         enc->marginSize = value;
         break;
      case DmtxPropModuleSize:
         enc->moduleSize = value;
         break;

      /* Image properties */
      case DmtxPropPixelPacking:
         enc->pixelPacking = value;
         break;
      case DmtxPropImageFlip:
         enc->imageFlip = value;
         break;
      case DmtxPropRowPadBytes:
         enc->rowPadBytes = value;
      default:
         break;
   }

   return DmtxPass;
}

/**
 * @brief  Get encoding behavior property
 * @param  enc
 * @param  prop
 * @return value
 */
extern int
dmtxEncodeGetProp(DmtxEncode *enc, int prop)
{
   switch(prop) {
      case DmtxPropMarginSize:
         return enc->marginSize;
      case DmtxPropModuleSize:
         return enc->moduleSize;
      case DmtxPropScheme:
         return enc->scheme;
      default:
         break;
   }

   return DmtxUndefined;
}

/**
 * @brief  Convert message into Data Matrix image
 * @param  enc
 * @param  inputSize
 * @param  inputString
 * @param  sizeIdxRequest
 * @return DmtxPass | DmtxFail
 */
#ifndef CUSTOM_ENCODEDATAMATRIX
extern DmtxPassFail
dmtxEncodeDataMatrix(DmtxEncode *enc, int inputSize, unsigned char *inputString)
{
   int dataWordCount;
   int sizeIdx;
   int padCount;
   int width, height, bitsPerPixel;
   unsigned char buf[4096];
   unsigned char *pxl;

   /* Encode input string into data codewords */
   sizeIdx = enc->sizeIdxRequest;
   dataWordCount = EncodeDataCodewords(enc, buf, inputString, inputSize, &sizeIdx);
   if(dataWordCount <= 0)
      return(DmtxFail);

   /* EncodeDataCodewords() should have updated any auto sizeIdx to a real one */
   assert(sizeIdx != DmtxSymbolSquareAuto && sizeIdx != DmtxSymbolRectAuto);

   /* Add pad characters to match a standard symbol size (whether smallest or requested) */
   padCount = AddPadChars(buf, &dataWordCount,
         dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx));

   /* XXX we can remove a lot of this redundant data */
   enc->region.sizeIdx = sizeIdx;
   enc->region.symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, sizeIdx);
   enc->region.symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, sizeIdx);
   enc->region.mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, sizeIdx);
   enc->region.mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, sizeIdx);

   /* Allocate memory for message and array */
   enc->message = dmtxMessageCreate(sizeIdx, DmtxFormatMatrix);
   enc->message->padCount = padCount;
   memcpy(enc->message->code, buf, dataWordCount);

   /* Generate error correction codewords */
   RsEncode(enc->message, enc->region.sizeIdx);

   /* Module placement in region */
   ModulePlacementEcc200(enc->message->array, enc->message->code,
         enc->region.sizeIdx, DmtxModuleOnRGB);

   width = 2 * enc->marginSize + (enc->region.symbolCols * enc->moduleSize);
   height = 2 * enc->marginSize + (enc->region.symbolRows * enc->moduleSize);
   bitsPerPixel = GetBitsPerPixel(enc->pixelPacking);
   if(bitsPerPixel == DmtxUndefined)
      return DmtxFail;
   assert(bitsPerPixel % 8 == 0);

   /* Allocate memory for the image to be generated */
   pxl = (unsigned char *)malloc(width * height * (bitsPerPixel/8) + enc->rowPadBytes);
   if(pxl == NULL) {
      perror("pixel malloc error");
      return DmtxFail;
   }

   enc->image = dmtxImageCreate(pxl, width, height, enc->pixelPacking);
   if(enc->image == NULL) {
      perror("image malloc error");
      return DmtxFail;
   }

   dmtxImageSetProp(enc->image, DmtxPropImageFlip, enc->imageFlip);
   dmtxImageSetProp(enc->image, DmtxPropRowPadBytes, enc->rowPadBytes);

   /* Insert finder and aligment pattern modules */
   PrintPattern(enc);

   return DmtxPass;
}
#endif

/**
 * @brief  Convert message into Data Mosaic image
 * @param  enc
 * @param  inputSize
 * @param  inputString
 * @param  sizeIdxRequest
 * @return DmtxPass | DmtxFail
 */
#ifndef CUSTOM_ENCODEDATAMOSAIC
extern DmtxPassFail
dmtxEncodeDataMosaic(DmtxEncode *enc, int inputSize, unsigned char *inputString)
{
   int dataWordCount;
   int tmpInputSize;
   unsigned char *inputStart;
   int splitInputSize[3];
   int sizeIdx, sizeIdxRequest;
   int splitSizeIdxAttempt, splitSizeIdxFirst, splitSizeIdxLast;
   unsigned char buf[3][4096];
   DmtxEncode encGreen, encBlue;
   int row, col, mappingRows, mappingCols;

   /* 1) count how many codewords it would take to encode the whole thing
    * 2) take ceiling N of codeword count divided by 3
    * 3) using minimum symbol size that can accomodate N codewords:
    * 4) create several barcodes over iterations of increasing numbers of
    *    input codewords until you go one too far
    * 5) if codewords remain after filling R, G, and B barcodes then go back
    *    to 3 and try with next larger size
    * 6) take the 3 different images you created and write out a new barcode
    */

   /* Encode full input string to establish baseline data codeword count */
   sizeIdx = sizeIdxRequest = enc->sizeIdxRequest;
   /* XXX buf can be changed here to use all 3 buffers' length */
   dataWordCount = EncodeDataCodewords(enc, buf[0], inputString, inputSize, &sizeIdx);
   if(dataWordCount <= 0)
      return DmtxFail;

   /* Use 1/3 (ceiling) of inputSize establish input size target */
   tmpInputSize = (inputSize + 2) / 3;
   splitInputSize[0] = tmpInputSize;
   splitInputSize[1] = tmpInputSize;
   splitInputSize[2] = inputSize - (splitInputSize[0] + splitInputSize[1]);
   /* XXX clean up above lines later for corner cases */

   /* Use 1/3 (floor) of dataWordCount establish first symbol size attempt */
   splitSizeIdxFirst = FindSymbolSize(tmpInputSize, sizeIdxRequest);
   if(splitSizeIdxFirst == DmtxUndefined)
      return DmtxFail;

   /* Set the last possible symbol size for this symbol shape or specific size request */
   if(sizeIdxRequest == DmtxSymbolSquareAuto)
      splitSizeIdxLast = DmtxSymbolSquareCount - 1;
   else if(sizeIdxRequest == DmtxSymbolRectAuto)
      splitSizeIdxLast = DmtxSymbolSquareCount + DmtxSymbolRectCount - 1;
   else
      splitSizeIdxLast = splitSizeIdxFirst;

   /* XXX would be nice if we could choose a size and then fill up each
      layer as we go, but this can cause problems with all data fits on
      first 2 layers.  Revisit this later after things look a bit cleaner. */

   /* Try increasing symbol sizes until 3 of them can hold all input values */
   for(splitSizeIdxAttempt = splitSizeIdxFirst; splitSizeIdxAttempt <= splitSizeIdxLast; splitSizeIdxAttempt++) {

      assert(splitSizeIdxAttempt >= 0);

      /* RED LAYER */
      sizeIdx = splitSizeIdxAttempt;
      inputStart = inputString;
      EncodeDataCodewords(enc, buf[0], inputStart, splitInputSize[0], &sizeIdx);
      if(sizeIdx != splitSizeIdxAttempt)
         continue;

      /* GREEN LAYER */
      sizeIdx = splitSizeIdxAttempt;
      inputStart += splitInputSize[0];
      EncodeDataCodewords(enc, buf[1], inputStart, splitInputSize[1], &sizeIdx);
      if(sizeIdx != splitSizeIdxAttempt)
         continue;

      /* BLUE LAYER */
      sizeIdx = splitSizeIdxAttempt;
      inputStart += splitInputSize[1];
      EncodeDataCodewords(enc, buf[2], inputStart, splitInputSize[2], &sizeIdx);
      if(sizeIdx != splitSizeIdxAttempt)
         continue;

      break;
   }

   dmtxEncodeSetProp(enc, DmtxPropSizeRequest, splitSizeIdxAttempt);

   /* Now we have the correct lengths for splitInputSize, and they all fit into the desired size */
   encGreen = *enc;
   encBlue = *enc;

   /* First encode red to the main encode struct (image portion will be overwritten) */
   inputStart = inputString;
   dmtxEncodeDataMatrix(enc, splitInputSize[0], inputStart);

   inputStart += splitInputSize[0];
   dmtxEncodeDataMatrix(&encGreen, splitInputSize[1], inputStart);

   inputStart += splitInputSize[1];
   dmtxEncodeDataMatrix(&encBlue, splitInputSize[2], inputStart);

   mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, splitSizeIdxAttempt);
   mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, splitSizeIdxAttempt);

   memset(enc->message->array, 0x00, sizeof(unsigned char) * enc->region.mappingRows * enc->region.mappingCols);
   ModulePlacementEcc200(enc->message->array, enc->message->code, enc->region.sizeIdx, DmtxModuleOnRed);

   /* Data Mosaic will traverse this array multiple times -- reset
      DmtxModuleAssigned and DMX_MODULE_VISITED bits before starting */
   for(row = 0; row < mappingRows; row++) {
      for(col = 0; col < mappingCols; col++) {
         enc->message->array[row*mappingCols+col] &= (0xff ^ (DmtxModuleAssigned | DmtxModuleVisited));
      }
   }

   ModulePlacementEcc200(enc->message->array, encGreen.message->code, enc->region.sizeIdx, DmtxModuleOnGreen);

   /* Data Mosaic will traverse this array multiple times -- reset
      DmtxModuleAssigned and DMX_MODULE_VISITED bits before starting */
   for(row = 0; row < mappingRows; row++) {
      for(col = 0; col < mappingCols; col++) {
         enc->message->array[row*mappingCols+col] &= (0xff ^ (DmtxModuleAssigned | DmtxModuleVisited));
      }
   }

   ModulePlacementEcc200(enc->message->array, encBlue.message->code, enc->region.sizeIdx, DmtxModuleOnBlue);

/* dmtxEncodeStructDeInit(&encGreen);
   dmtxEncodeStructDeInit(&encBlue); */

   PrintPattern(enc);

   return DmtxPass;
}
#endif

/**
 * @brief  Convert input into message using specific encodation scheme
 * @param  buf
 * @param  inputString
 * @param  inputSize
 * @param  scheme
 * @param  sizeIdx
 * @return Count of encoded data words
 */
static int
EncodeDataCodewords(DmtxEncode *enc, unsigned char *buf, unsigned char *inputString,
      int inputSize, int *sizeIdx)
{
   int dataWordCount;
   DmtxEncodeStream stream;

   /*
    * This function needs to take both dataWordCount and sizeIdx into account
    * because symbol size is tied to an encodation. That is, a data stream
    * might be different from one symbol size to another
    */

   /* Encode input string into data codewords */
   switch(enc->scheme) {
      case DmtxSchemeAutoBest:
         dataWordCount = EncodeAutoBest(enc, buf, inputString, inputSize);
         break;
      case DmtxSchemeAutoFast:
         dataWordCount = 0;
         /* dataWordCount = EncodeAutoFast(enc, buf, inputString, inputSize); */
         break;
      default:
/*
         dataWordCount = EncodeSingleScheme(enc, buf, inputString, inputSize, enc->scheme);
*/
         stream = StreamInit(inputString, inputSize, buf, 4096);
         EncodeSingleScheme2(&stream, enc->scheme, DmtxSymbolSquareAuto);
         dataWordCount = stream.output.length;
         *sizeIdx = stream.sizeIdx;
/*       dmtxByteListPrint(&(stream.output), "xxx:"); */
         break;
   }


   /* XXX must fix ... will need to handle sizeIdx requests here because it is
      needed by Encode...() for triplet termination */

   /* parameter sizeIdx is requested value, returned sizeIdx is decision */
/* *sizeIdx = FindSymbolSize(dataWordCount, *sizeIdx); */
   if(*sizeIdx == DmtxUndefined)
      return 0;

   return dataWordCount;
}

/**
 * @brief  Add necessary padding codewords to message
 * @param  buf
 * @param  bufSize
 * @param  paddedSize
 * @return void
 */
static int
AddPadChars(unsigned char *buf,  int *bufSize, int paddedSize)
{
   int padCount = 0;

   /* First pad character is not randomized */
   if(*bufSize < paddedSize) {
      padCount++;
      buf[(*bufSize)++] = DmtxValueAsciiPad;
   }

   /* All remaining pad characters are randomized based on character position */
   while(*bufSize < paddedSize) {
      padCount++;
      buf[*bufSize] = Randomize253State(DmtxValueAsciiPad, *bufSize + 1);
      (*bufSize)++;
   }

   return padCount;
}

/**
 * @brief  Randomize 253 state
 * @param  codewordValue
 * @param  codewordPosition
 * @return Randomized value
 */
static unsigned char
Randomize253State(unsigned char codewordValue, int codewordPosition)
{
   int pseudoRandom;
   int tmp;

   pseudoRandom = ((149 * codewordPosition) % 253) + 1;
   tmp = codewordValue + pseudoRandom;
   if(tmp > 254)
      tmp -= 254;

   assert(tmp >= 0 && tmp < 256);

   return (unsigned char)tmp;
}

/**
 * @brief  Randomize 255 state
 * @param  codewordValue
 * @param  codewordPosition
 * @return Randomized value
 */
static unsigned char
Randomize255State(unsigned char codewordValue, int codewordPosition)
{
   int pseudoRandom;
   int tmp;

   pseudoRandom = ((149 * codewordPosition) % 255) + 1;
   tmp = codewordValue + pseudoRandom;

   return (tmp <= 255) ? tmp : tmp - 256;
}

/**
 * @brief  Write encoded message to image
 * @param  enc
 * @return void
 */
static void
PrintPattern(DmtxEncode *enc)
{
   int i, j;
   int symbolRow, symbolCol;
   int pixelRow, pixelCol;
   int moduleStatus;
   size_t rowSize, height;
   int rgb[3];
   double sxy, txy;
   DmtxMatrix3 m1, m2;
   DmtxVector2 vIn, vOut;

   txy = enc->marginSize;
   sxy = 1.0/enc->moduleSize;

   dmtxMatrix3Translate(m1, -txy, -txy);
   dmtxMatrix3Scale(m2, sxy, -sxy);
   dmtxMatrix3Multiply(enc->xfrm, m1, m2);

   dmtxMatrix3Translate(m1, txy, txy);
   dmtxMatrix3Scale(m2, enc->moduleSize, enc->moduleSize);
   dmtxMatrix3Multiply(enc->rxfrm, m2, m1);

   rowSize = dmtxImageGetProp(enc->image, DmtxPropRowSizeBytes);
   height = dmtxImageGetProp(enc->image, DmtxPropHeight);

   memset(enc->image->pxl, 0xff, rowSize * height);

   for(symbolRow = 0; symbolRow < enc->region.symbolRows; symbolRow++) {
      for(symbolCol = 0; symbolCol < enc->region.symbolCols; symbolCol++) {

         vIn.X = symbolCol;
         vIn.Y = symbolRow;

         dmtxMatrix3VMultiply(&vOut, &vIn, enc->rxfrm);

         pixelCol = (int)(vOut.X);
         pixelRow = (int)(vOut.Y);

         moduleStatus = dmtxSymbolModuleStatus(enc->message,
               enc->region.sizeIdx, symbolRow, symbolCol);

         for(i = pixelRow; i < pixelRow + enc->moduleSize; i++) {
            for(j = pixelCol; j < pixelCol + enc->moduleSize; j++) {
               rgb[0] = ((moduleStatus & DmtxModuleOnRed) != 0x00) ? 0 : 255;
               rgb[1] = ((moduleStatus & DmtxModuleOnGreen) != 0x00) ? 0 : 255;
               rgb[2] = ((moduleStatus & DmtxModuleOnBlue) != 0x00) ? 0 : 255;
/*             dmtxImageSetRgb(enc->image, j, i, rgb); */
               dmtxImageSetPixelValue(enc->image, j, i, 0, rgb[0]);
               dmtxImageSetPixelValue(enc->image, j, i, 1, rgb[1]);
               dmtxImageSetPixelValue(enc->image, j, i, 2, rgb[2]);
            }
         }

      }
   }
}

/**
 * @brief  Initialize encoding channel
 * @param  channel
 * @param  codewords
 * @param  length
 * @return void
 */
static void
InitChannel(DmtxChannel *channel, unsigned char *codewords, int length)
{
   memset(channel, 0x00, sizeof(DmtxChannel));
   channel->encScheme = DmtxSchemeAscii;
   channel->invalid = DmtxChannelValid;
   channel->inputPtr = codewords;
   channel->inputStop = codewords + length;
}

/**
 * @brief  Encode message using single encodation scheme
 * @param  buf
 * @param  codewords
 * @param  length
 * @param  scheme
 * @return Encoded length
 */
static int
EncodeSingleScheme(DmtxEncode *enc, unsigned char *buf, unsigned char *codewords, int length, DmtxScheme scheme)
{
   int size;
   DmtxPassFail err;
   DmtxChannel channel;

   InitChannel(&channel, codewords, length);

   while(channel.inputPtr < channel.inputStop) {
      err = EncodeNextWord(enc, &channel, scheme);
      if(err == DmtxFail)
         return 0;

      /* DumpChannel(&channel); */

      if(channel.invalid != 0) {
         fprintf(stderr, "Character \"%c\" not supported by requested encodation scheme\n\n", *channel.inputPtr);
         return 0;
      }
   }
   /* DumpChannel(&channel); */

   size = channel.encodedLength/12;
   memcpy(buf, channel.encodedWords, size);

   return size;
}

/**
 * @brief  Encode message using best possible encodation (combine schemes)
 * @param  buf
 * @param  codewords
 * @param  length
 * @return Encoded length of winning channel
 */
static int
EncodeAutoBest(DmtxEncode *enc, unsigned char *buf, unsigned char *codewords, int length)
{
   int targetScheme;
   int winnerSize;
   DmtxChannelGroup optimal, best;
   DmtxChannel *channel, *winner;

   /* Intialize optimizing channels and encode first codeword from default ASCII */
   for(targetScheme = DmtxSchemeAscii; targetScheme <= DmtxSchemeBase256; targetScheme++) {
      channel = &(optimal.channel[targetScheme]);
      InitChannel(channel, codewords, length);
      EncodeNextWord(enc, channel, targetScheme);
      /* EncodeNextWord() may fail here which is okay because messages may
         begin with characters not supported in all encodation schemes */
   }

   /* fprintf(stdout,"\nWinners:"); */
   /* DumpChannelGroup(&optimal, DmtxSchemeAscii); */

   /* For each remaining word in the input stream, test the efficiency of
      getting to this encodation scheme for each input character by
      switching here from each of the other channels (which are always
      optimally encoded) */
   while(optimal.channel[0].inputPtr < optimal.channel[0].inputStop) { /* XXX only tracking first channel */

      /* fprintf(stdout,"\n** codeword **\n"); */
      for(targetScheme = DmtxSchemeAscii; targetScheme <= DmtxSchemeBase256; targetScheme++) {
         best.channel[targetScheme] = FindBestChannel(enc, optimal, targetScheme);
      }
      optimal = best;

      /* fprintf(stdout, "\nWinners:"); */
      /* DumpChannelGroup(&optimal, 0); */
   }

   /* Choose a winner now that all channels are finished */
   winner = &(optimal.channel[DmtxSchemeAscii]);
   for(targetScheme = DmtxSchemeAscii + 1; targetScheme <= DmtxSchemeBase256; targetScheme++) {
      if(optimal.channel[targetScheme].invalid != 0)
         continue;

      if(optimal.channel[targetScheme].encodedLength < winner->encodedLength)
         winner = &(optimal.channel[targetScheme]);
   }

   /* XXX get rid of buf concept and try to do something with channel -> matrix copy instead */
   winnerSize = winner->encodedLength/12;
   memcpy(buf, winner->encodedWords, winnerSize);

   return winnerSize;
}

/**
 * @brief  Determine current best channel in encoding process
 * @param  group
 * @param  targetScheme
 * @return Winning channel
 */
static DmtxChannel
FindBestChannel(DmtxEncode *enc, DmtxChannelGroup group, DmtxScheme targetScheme)
{
   DmtxPassFail err;
   DmtxScheme encFrom;
   DmtxChannel *channel, *winner;

   winner = NULL;
   for(encFrom = DmtxSchemeAscii; encFrom <= DmtxSchemeBase256; encFrom++) {

      channel = &(group.channel[encFrom]);

      /* If from channel doesn't hold valid data because it couldn't
         represent the previous value then skip it */
      if(channel->invalid != 0)
         continue;

      /* If channel has already processed all of its input values then it
         cannot be used as a starting point */
      if(channel->inputPtr == channel->inputStop)
         continue;

      err = EncodeNextWord(enc, channel, targetScheme);
      if(err == DmtxFail)
         ; /* XXX fix this */

      /* If channel scheme can't represent next word then stop for this channel */
      if((channel->invalid & DmtxChannelUnsupportedChar) != 0) {
         winner = channel;
         break;
      }

      /* If channel scheme was unable to unlatch here then skip */
      if((channel->invalid & DmtxChannelCannotUnlatch) != 0)
         continue;

      if(winner == NULL || channel->currentLength < winner->currentLength)
         winner = channel;
   }

   /* DumpChannelGroup(&group, targetScheme); */

   return *winner;
}

/**
 * @brief  Encode next codeword using requested encodation scheme
 * @param  channel
 * @param  targetScheme
 * @return void
 */
static DmtxPassFail
EncodeNextWord(DmtxEncode *enc, DmtxChannel *channel, DmtxScheme targetScheme)
{
   DmtxPassFail err;

   /* Change to new encodation scheme if necessary */
   if(channel->encScheme != targetScheme) {
      ChangeEncScheme(channel, targetScheme, DmtxUnlatchExplicit);
      if(channel->invalid != 0)
         return DmtxFail;
   }

   assert(channel->encScheme == targetScheme);

   /* Encode next input value */
   switch(channel->encScheme) {
      case DmtxSchemeAscii:
         err = EncodeAsciiCodeword(channel);
         break;
      case DmtxSchemeC40:
         err = EncodeTripletCodeword(enc, channel);
         break;
      case DmtxSchemeText:
         err = EncodeTripletCodeword(enc, channel);
         break;
      case DmtxSchemeX12:
         err = EncodeTripletCodeword(enc, channel);
         break;
      case DmtxSchemeEdifact:
         err = EncodeEdifactCodeword(enc, channel);
         break;
      case DmtxSchemeBase256:
         err = EncodeBase256Codeword(channel);
         break;
      default:
         err = DmtxFail;
         break;
   }

   if(err == DmtxFail)
      return DmtxFail;

   return DmtxPass;
}

/**
 * @brief  Encode value using ASCII encodation (standard or extended)
 * @param  channel
 * @return void
 */
static DmtxPassFail
EncodeAsciiCodeword(DmtxChannel *channel)
{
   unsigned char inputValue, prevValue, prevPrevValue;
   int prevIndex;

   assert(channel->encScheme == DmtxSchemeAscii);

   inputValue = *(channel->inputPtr);

   /* XXX this is problematic ... We should not be looking backward in the
      channel to determine what state we're in. Add the necessary logic to
      fix the current bug (prevprev != 253) but when rewriting encoder later
      make sure double digit ascii as treated as a forward-encoded condition.
      i.e., encode ahead of where we currently stand, and not comparable to other
      channels because currently sitting between byte boundaries (like the
      triplet-based schemes). Much simpler. */

   /* XXX another thought on the future rewrite: if adopting a forward-encoding
      approach on double digits then the following input situation:

         digit digit c40a c40b c40c

      would create:

         ASCII_double C40_triplet1ab C40_triplet2bc

      although it might be more efficient in some cases to do

         digit C40_triplet1(digit a) C40_triplet2(a b)

      (I can't think of a situation like this, but I can't rule it out either)
      Unfortunately the forward encoding approach would never allow ascii to unlatch
      between the ASCII_double input words.

      One approach that would definitely avoid this is to treat ASCII_dd as a
      separate channel when using "--best".  However, when encoding to single-
      scheme ascii you would always use the ASCII_dd approach.

      This function, EncodeAsciiCodeword(), should have another parameter to flag
      whether or not to compress double digits. When encoding in single scheme
      ascii, then compress the double digits. If using --best then use both options
      as separate channels. */

   /* 2nd digit char in a row - overwrite first digit word with combined value */
   if(ISDIGIT(inputValue) && channel->currentLength >= channel->firstCodeWord + 12) {
      prevIndex = (channel->currentLength - 12)/12;
      prevValue = channel->encodedWords[prevIndex] - 1;

      prevPrevValue = (prevIndex > channel->firstCodeWord/12) ?
            channel->encodedWords[prevIndex-1] : 0;

      if(prevPrevValue != 235 && ISDIGIT(prevValue)) {
         channel->encodedWords[prevIndex] = 10 * (prevValue - '0') + (inputValue - '0') + 130;
         channel->inputPtr++;
         return DmtxPass;
      }
   }

   /* Extended ASCII char */
   if(inputValue >= 128) {
      PushInputWord(channel, DmtxValueAsciiUpperShift);
      IncrementProgress(channel, 12);
      inputValue -= 128;
   }

   PushInputWord(channel, inputValue + 1);
   IncrementProgress(channel, 12);
   channel->inputPtr++;

   return DmtxPass;
}

/**
 * @brief  Encode value using C40, Text, or X12 encodation
 * @param  channel
 * @return void
 */
static DmtxPassFail
EncodeTripletCodeword(DmtxEncode *enc, DmtxChannel *channel)
{
   int i;
   int inputCount;
   int tripletCount;
   int count;
   int outputWords[4];       /* biggest: upper shift to non-basic set */
   unsigned char buffer[6];  /* biggest: 2 words followed by 4-word upper shift */
   DmtxPassFail err;
   DmtxTriplet triplet;
   unsigned char inputWord;
   unsigned char *ptr;

   assert(channel->encScheme == DmtxSchemeC40 ||
         channel->encScheme == DmtxSchemeText ||
         channel->encScheme == DmtxSchemeX12);

   assert(channel->currentLength <= channel->encodedLength);

   /* If there are no pre-encoded codewords then generate some */
   if(channel->currentLength == channel->encodedLength) {

      assert(channel->currentLength % 12 == 0);

      /* Ideally we would only encode one codeword triplet here (the
         minimum that you can do at a time) but we can't leave the channel
         with the last encoded word as a shift.  The following loop
         prevents this condition by encoding until we have a clean break or
         until we reach the end of the input data. */

      ptr = channel->inputPtr;

      tripletCount = 0;
      for(;;) {

         /* Fill array with at least 3 values (the minimum necessary to
            encode a triplet), but possibly up to 6 values due to presence
            of upper shifts.  Note that buffer may already contain values
            from a previous iteration of the outer loop, and this step
            "tops off" the buffer to make sure there are at least 3 values. */

         while(tripletCount < 3 && ptr < channel->inputStop) {
            inputWord = *(ptr++);
            count = GetC40TextX12Words(outputWords, inputWord, channel->encScheme);

            if(count == 0) {
               channel->invalid = DmtxChannelUnsupportedChar;
               return DmtxFail;
            }

            for(i = 0; i < count; i++) {
               buffer[tripletCount++] = outputWords[i];
            }
         }

         /* Take the next 3 values from buffer to encode */
         triplet.value[0] = buffer[0];
         triplet.value[1] = buffer[1];
         triplet.value[2] = buffer[2];

         if(tripletCount >= 3) {
            PushTriplet(channel, &triplet);
            buffer[0] = buffer[3];
            buffer[1] = buffer[4];
            buffer[2] = buffer[5];
            tripletCount -= 3;
         }

         /* If we reach the end of input and have not encountered a clean
            break opportunity then complete the symbol here */

         if(ptr == channel->inputStop) {
            /* tripletCount represents the number of values in triplet waiting to be pushed
               inputCount represents the number of values after inputPtr waiting to be pushed */
            while(channel->currentLength < channel->encodedLength) {
               IncrementProgress(channel, 8);
               channel->inputPtr++;
            }

            /* If final triplet value was shift then IncrementProgress will
               overextend us .. hack it back a little.  Note that this means
               this barcode is invalid unless one of the specific end-of-symbol
               conditions explicitly allows it. */
            if(channel->currentLength == channel->encodedLength + 8) {
               channel->currentLength = channel->encodedLength;
               channel->inputPtr--;
            }

            assert(channel->inputStop >= channel->inputPtr);
            assert(channel->inputStop - channel->inputPtr <= INT_MAX);
            inputCount = (int)(channel->inputStop - channel->inputPtr);

            err = ProcessEndOfSymbolTriplet(enc, channel, &triplet, tripletCount, inputCount);
            if(err == DmtxFail)
               return DmtxFail;
            break;
         }

         /* If there are no triplet values remaining in the buffer then
            break.  This guarantees that we will always stop encoding on a
            clean "unshifted" break */

         if(tripletCount == 0)
            break;
      }
   }

   /* Pre-encoded codeword is available for consumption */
   if(channel->currentLength < channel->encodedLength) {
      IncrementProgress(channel, 8);
      channel->inputPtr++;
   }

   return DmtxPass;
}

/**
 * @brief  Encode value using EDIFACT encodation
 * @param  channel
 * @return void
 */
static DmtxPassFail
EncodeEdifactCodeword(DmtxEncode *enc, DmtxChannel *channel)
{
   unsigned char inputValue;

   assert(channel->encScheme == DmtxSchemeEdifact);

   inputValue = *(channel->inputPtr);

   if(inputValue < 32 || inputValue > 94) {
      channel->invalid = DmtxChannelUnsupportedChar;
      return DmtxFail;
   }

   PushInputWord(channel, inputValue & 0x3f);
   IncrementProgress(channel, 9);
   channel->inputPtr++;

   /* XXX rename this to CheckforEndOfSymbolEdifact() */
   TestForEndOfSymbolEdifact(enc, channel);

   return DmtxPass;
}

/**
 * @brief  Encode value using Base 256 encodation
 * @param  channel
 * @return void
 */
static DmtxPassFail
EncodeBase256Codeword(DmtxChannel *channel)
{
   int i;
   int newDataLength;
   int headerByteCount;
   unsigned char valueTmp;
   unsigned char *firstBytePtr;
   unsigned char headerByte[2];

   assert(channel->encScheme == DmtxSchemeBase256);

   firstBytePtr = &(channel->encodedWords[channel->firstCodeWord/12]);
   headerByte[0] = UnRandomize255State(*firstBytePtr, channel->firstCodeWord/12 + 1);

   /* newSchemeLength contains size byte(s) too */
   if(headerByte[0] <= 249) {
      newDataLength = headerByte[0];
   }
   else {
      newDataLength = 250 * (headerByte[0] - 249);
      newDataLength += UnRandomize255State(*(firstBytePtr+1), channel->firstCodeWord/12 + 2);
   }

   newDataLength++;

   if(newDataLength <= 249) {
      headerByteCount = 1;
      headerByte[0] = newDataLength;
      headerByte[1] = 0; /* unused */
   }
   else {
      headerByteCount = 2;
      headerByte[0] = newDataLength/250 + 249;
      headerByte[1] = newDataLength%250;
   }

   /* newDataLength does not include header bytes */
   assert(newDataLength > 0 && newDataLength <= 1555);

   /* One time shift of codewords when passing the 250 byte size threshold */
   if(newDataLength == 250) {
      for(i = channel->currentLength/12 - 1; i > channel->firstCodeWord/12; i--) {
         valueTmp = UnRandomize255State(channel->encodedWords[i], i+1);
         channel->encodedWords[i+1] = Randomize255State(valueTmp, i+2);
      }
      IncrementProgress(channel, 12);
      channel->encodedLength += 12; /* ugly */
   }

   /* Update scheme length in Base 256 header */
   for(i = 0; i < headerByteCount; i++)
      *(firstBytePtr+i) = Randomize255State(headerByte[i], channel->firstCodeWord/12 + i + 1);

   PushInputWord(channel, Randomize255State(*(channel->inputPtr), channel->currentLength/12 + 1));
   IncrementProgress(channel, 12);
   channel->inputPtr++;

   /* XXX will need to introduce an EndOfSymbolBase256() that recognizes
      opportunity to encode headerLength of 0 if remaining Base 256 message
      exactly matches symbol capacity */

   return DmtxPass;
}

/**
 * @brief  Change from one encodation scheme to another
 * @param  channel
 * @param  targetScheme
 * @param  unlatchType
 * @return void
 */
static void
ChangeEncScheme(DmtxChannel *channel, DmtxScheme targetScheme, int unlatchType)
{
   int advance;

   assert(channel->encScheme != targetScheme);

   /* Unlatch to ASCII (base encodation scheme) */
   switch(channel->encScheme) {
      case DmtxSchemeAscii:
         /* Nothing to do */
         assert(channel->currentLength % 12 == 0);
         break;

      case DmtxSchemeC40:
      case DmtxSchemeText:
      case DmtxSchemeX12:

         /* Can't unlatch unless currently at a byte boundary */
         if((channel->currentLength % 12) != 0) {
            channel->invalid = DmtxChannelCannotUnlatch;
            return;
         }

         /* Can't unlatch if last word in previous triplet is a shift */
         if(channel->currentLength != channel->encodedLength) {
            channel->invalid = DmtxChannelCannotUnlatch;
            return;
         }

         /* Unlatch to ASCII and increment progress */
         if(unlatchType == DmtxUnlatchExplicit) {
            PushInputWord(channel, DmtxValueCTXUnlatch);
            IncrementProgress(channel, 12);
         }
         break;

      case DmtxSchemeEdifact:

         /* must overwrite next 6 bits (after current) with 011111 (31) and
            then fill remaining bits until next byte bounday with zeros
            then set encodedLength, encodedTwothirdsbits, currentLength,
            currentTwothirdsbits.  PushInputWord guarantees that remaining
            bits are padded to 0, so just push the unlatch code and then
            increment current and encoded length */

         assert(channel->currentLength % 3 == 0);
         if(unlatchType == DmtxUnlatchExplicit) {
            PushInputWord(channel, DmtxValueEdifactUnlatch);
            IncrementProgress(channel, 9);
         }

         /* Advance progress to next byte boundary */
         advance = (channel->currentLength % 4) * 3;
         channel->currentLength += advance;
         channel->encodedLength += advance;
         /* assert(remaining bits are zero); */
         break;

      case DmtxSchemeBase256:

         /* since Base 256 stores the length value at the beginning of the
            string instead of using an unlatch character, "unlatching" Base
            256 involves going to the beginning of this stretch of Base 256
            codewords and update the placeholder with the current length.
            Note that the Base 256 length value can either be 1 or 2 bytes,
            depending on the length of the current stretch of Base 256
            chars.  However, this value will already have the correct
            number of codewords allocated since this is checked every time
            a new Base 256 codeword is pushed to the channel. */
         break;

      default:
         break;
   }
   channel->encScheme = DmtxSchemeAscii;

   /* Latch to new encodation scheme */
   switch(targetScheme) {
      case DmtxSchemeAscii:
         /* Nothing to do */
         break;
      case DmtxSchemeC40:
         PushInputWord(channel, DmtxValueC40Latch);
         IncrementProgress(channel, 12);
         break;
      case DmtxSchemeText:
         PushInputWord(channel, DmtxValueTextLatch);
         IncrementProgress(channel, 12);
         break;
      case DmtxSchemeX12:
         PushInputWord(channel, DmtxValueX12Latch);
         IncrementProgress(channel, 12);
         break;
      case DmtxSchemeEdifact:
         PushInputWord(channel, DmtxValueEdifactLatch);
         IncrementProgress(channel, 12);
         break;
      case DmtxSchemeBase256:
         PushInputWord(channel, DmtxValueBase256Latch);
         IncrementProgress(channel, 12);

         /* Write temporary field length (0 indicates remainder of symbol) */
         PushInputWord(channel, Randomize255State(0, 2));
         IncrementProgress(channel, 12);
         break;
      default:
         break;
   }
   channel->encScheme = targetScheme;
   channel->firstCodeWord = channel->currentLength - 12;

   assert(channel->firstCodeWord % 12 == 0);
}

/**
 * @brief  Push codeword onto channel and increment length
 * @param  channel
 * @param  codeword
 * @return void
 */
static void
PushInputWord(DmtxChannel *channel, unsigned char codeword)
{
   int i;
   int startByte, pos;
   DmtxQuadruplet quad;

   /* XXX should this assertion actually be a legit runtime test? */
   assert(channel->encodedLength/12 <= 3*1558); /* increased for Mosaic */

   /* XXX this is currently pretty ugly, but can wait until the
      rewrite. What is required is to go through and decide on a
      consistent approach (probably that all encodation schemes use
      currentLength except for triplet-based schemes which use
      currentLength and encodedLength).  All encodation schemes should
      maintain both currentLength and encodedLength though.  Perhaps
      another approach would be to maintain currentLength and "extraLength" */

   switch(channel->encScheme) {
      case DmtxSchemeAscii:
         channel->encodedWords[channel->currentLength/12] = codeword;
         channel->encodedLength += 12;
         break;

      case DmtxSchemeC40:
      case DmtxSchemeText:
      case DmtxSchemeX12:
         channel->encodedWords[channel->encodedLength/12] = codeword;
         channel->encodedLength += 12;
         break;

      case DmtxSchemeEdifact:
         /* EDIFACT is the only encodation scheme where we don't encode up to the
            next byte boundary.  This is because EDIFACT can be unlatched at any
            point, including mid-byte, so we can't guarantee what the next
            codewords will be.  All other encodation schemes only unlatch on byte
            boundaries, allowing us to encode to the next boundary knowing that
            we have predicted the only codewords that could be used while in this
            scheme. */

         /* write codeword value to next 6 bits (might span codeword bytes) and
            then pad any remaining bits until next byte boundary with zero bits. */
         pos = channel->currentLength % 4;
         startByte = ((channel->currentLength + 9) / 12) - pos;

         quad = GetQuadrupletValues(channel->encodedWords[startByte],
                                    channel->encodedWords[startByte+1],
                                    channel->encodedWords[startByte+2]);
         quad.value[pos] = codeword;
         for(i = pos + 1; i < 4; i++)
            quad.value[i] = 0;

         /* Only write the necessary codewords */
         switch(pos) {
            case 3:
            case 2:
               channel->encodedWords[startByte+2] = ((quad.value[2] & 0x03) << 6) | quad.value[3];
            case 1:
               channel->encodedWords[startByte+1] = ((quad.value[1] & 0x0f) << 4) | (quad.value[2] >> 2);
            case 0:
               channel->encodedWords[startByte] = (quad.value[0] << 2) | (quad.value[1] >> 4);
         }

         channel->encodedLength += 9;
         break;

      case DmtxSchemeBase256:
         channel->encodedWords[channel->currentLength/12] = codeword;
         channel->encodedLength += 12;
         break;

      default:
         break;
   }
}

/**
 * @brief  Push triplet codeword onto channel
 * @param  channel
 * @param  triplet
 * @return void
 */
static void
PushTriplet(DmtxChannel *channel, DmtxTriplet *triplet)
{
   int tripletValue;

   tripletValue = (1600 * triplet->value[0]) + (40 * triplet->value[1]) + triplet->value[2] + 1;
   PushInputWord(channel, tripletValue / 256);
   PushInputWord(channel, tripletValue % 256);
}

/**
 * @brief  Increment encoding progress tracking variables
 * @param  channel
 * @param  encodedUnits
 * @return void
 */
static void
IncrementProgress(DmtxChannel *channel, int encodedUnits)
{
   int startByte, pos;
   DmtxTriplet triplet;

   /* XXX this function became a misnomer when we started incrementing by
    * an amount other than what was specified with the C40/Text exception.
    * Maybe a new name/convention is in order.
    */

   /* In C40 and Text encodation schemes while we normally use 5 1/3 bits
    * to encode a regular character, we also must account for the extra
    * 5 1/3 bits (for a total of 10 2/3 bits that gets used for a shifted
    * character.
    */

   if(channel->encScheme == DmtxSchemeC40 ||
         channel->encScheme == DmtxSchemeText) {

      pos = (channel->currentLength % 6) / 2;
      startByte = (channel->currentLength / 12) - (pos >> 1);
      triplet = GetTripletValues(channel->encodedWords[startByte], channel->encodedWords[startByte+1]);

      /* Note that we will alway increment progress according to a whole
         input codeword, so the value at "pos" is guaranteed to not be in
         a shifted state. */
      if(triplet.value[pos] <= 2)
         channel->currentLength += 8;
   }

   channel->currentLength += encodedUnits;
}

/**
 * @brief  Special end-of-symbol encoding for triplet-based schemes
 * @param  channel
 * @param  triplet
 * @param  tripletCount
 * @param  inputCount
 * @return void
 */
static DmtxPassFail
ProcessEndOfSymbolTriplet(DmtxEncode *enc, DmtxChannel *channel,
      DmtxTriplet *triplet, int tripletCount, int inputCount)
{
   int sizeIdx;
   int currentByte;
   int remainingCodewords;
   int inputAdjust;
   DmtxPassFail err;

   /* In this function we process some special cases from the Data Matrix
    * standard, and as such we circumvent the normal functions for
    * accomplishing certain tasks.  This breaks our internal counts, but this
    * function always marks the end of processing so it will not affect
    * anything downstream.  This approach allows the normal encoding functions
    * to be built with very strict checks and assertions.
    *
    * EXIT CONDITIONS:
    *
    *   triplet  symbol  action
    *   -------  ------  -------------------
    *         1       0  need bigger symbol
    *         1       1  special case (d)
    *         1       2  special case (c)
    *         1       3  unlatch ascii pad
    *         1       4  unlatch ascii pad pad
    *         2       0  need bigger symbol
    *         2       1  need bigger symbol
    *         2       2  special case (b)
    *         2       3  unlatch ascii ascii
    *         2       4  unlatch ascii ascii pad
    *         3       0  need bigger symbol
    *         3       1  need bigger symbol
    *         3       2  special case (a)
    *         3       3  c40 c40 unlatch
    *         3       4  c40 c40 unlatch pad
    */

   /* We should always reach this point on a byte boundary */
   assert(channel->currentLength % 12 == 0);

   /* XXX Capture how many extra input values will be counted ... for later adjustment */
   inputAdjust = tripletCount - inputCount;

   /* Find minimum symbol size big enough to accomodate remaining codewords */
   currentByte = channel->currentLength/12;

   sizeIdx = FindSymbolSize(currentByte + ((inputCount == 3) ? 2 : inputCount),
         enc->sizeIdxRequest);

   if(sizeIdx == DmtxUndefined)
      return DmtxFail;

   /* XXX test for sizeIdx == DmtxUndefined here */
   remainingCodewords = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx) - currentByte;

   /* XXX the big problem with all of these special cases is what if one of
      these last words requires multiple bytes in ASCII (like upper shift?).
      We probably need to add a test against this and then just force an
      unlatch if we see this coming. */

   /* Special case (d): Unlatch is implied (switch manually) */
   if(inputCount == 1 && remainingCodewords == 1) {
      ChangeEncScheme(channel, DmtxSchemeAscii, DmtxUnlatchImplicit);
      err = EncodeNextWord(enc, channel, DmtxSchemeAscii);
      if(err == DmtxFail)
         return DmtxFail;
      assert(channel->invalid == 0);
      assert(channel->inputPtr == channel->inputStop);
   }
   else if(remainingCodewords == 2) {
      /* Special case (a): Unlatch is implied */
      if(tripletCount == 3) {
         PushTriplet(channel, triplet);
         IncrementProgress(channel, 24);
         channel->encScheme = DmtxSchemeAscii;
         channel->inputPtr += 3;
         channel->inputPtr -= inputAdjust;
      }
      /* Special case (b): Unlatch is implied */
      else if(tripletCount == 2) {
/*       assert(2nd C40 is not a shift character); */
         triplet->value[2] = 0;
         PushTriplet(channel, triplet);
         IncrementProgress(channel, 24);
         channel->encScheme = DmtxSchemeAscii;
         channel->inputPtr += 2;
         channel->inputPtr -= inputAdjust;
      }
      /* Special case (c) */
      else if(tripletCount == 1) {
         ChangeEncScheme(channel, DmtxSchemeAscii, DmtxUnlatchExplicit);
         err = EncodeNextWord(enc, channel, DmtxSchemeAscii);
         if(err == DmtxFail)
            return DmtxFail;
         assert(channel->invalid == 0);
         /* XXX I can still think of a case that looks ugly here.  What if
            the final 2 C40 codewords are a Shift word and a non-Shift
            word.  This special case will unlatch after the shift ... which
            is probably legal but I'm not loving it.  Give it more thought. */
      }
   }
   else {
/*    assert(remainingCodewords == 0 || remainingCodewords >= 3); */

      currentByte = channel->currentLength/12;
      remainingCodewords = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx) - currentByte;

      if(remainingCodewords > 0) {
         ChangeEncScheme(channel, DmtxSchemeAscii, DmtxUnlatchExplicit);

         while(channel->inputPtr < channel->inputStop) {
            err = EncodeNextWord(enc, channel, DmtxSchemeAscii);
            if(err == DmtxFail)
               return DmtxFail;
            assert(channel->invalid == 0);
         }
      }
   }

   assert(channel->inputPtr == channel->inputStop);

   return DmtxPass;
}

/**
 * @brief  Determine if end-of-symbol condition is met for EDIFACT-based schemes
 * @param  channel
 * @return void
 */
static DmtxPassFail
TestForEndOfSymbolEdifact(DmtxEncode *enc, DmtxChannel *channel)
{
   int edifactValues;
   int currentByte;
   int sizeIdx;
   int symbolCodewords;
   int asciiCodewords;
   int i;
   DmtxPassFail err;

   /* This function tests if the remaining input values can be completed using
    * one of the valid end-of-symbol cases, and finishes encodation if possible.
    *
    * This function must exit in ASCII encodation.  EDIFACT must always be
    * unlatched, although implicit Unlatch is possible.
    *
    * End   Symbol  ASCII  EDIFACT  End        Codeword
    * Case  Words   Words  Values   Condition  Sequence
    * ----  ------  -----  -------  ---------  -------------------------------
    * (a)        1      0           Special    PAD
    * (b)        1      1           Special    ASCII (could be 2 digits)
    * (c)        1   >= 2           Continue   Need larger symbol
    * (d)        2      0           Special    PAD PAD
    * (e)        2      1           Special    ASCII PAD
    * (f)        2      2           Special    ASCII ASCII
    * (g)        2   >= 3           Continue   Need larger symbol
    * (h)      N/A    N/A        0  Normal     UNLATCH
    * (i)      N/A    N/A     >= 1  Continue   Not end of symbol
    *
    * Note: All "Special" cases (a,b,d,e,f) require clean byte boundary to start
    */

   /* Count remaining input values assuming EDIFACT encodation */
   assert(channel->inputStop >= channel->inputPtr);
   assert(channel->inputStop - channel->inputPtr <= INT_MAX);
   edifactValues = (int)(channel->inputStop - channel->inputPtr);

   /* Can't end symbol right now if there are 5+ values remaining
      (noting that '9999' can still terminate in case (f)) */
   if(edifactValues > 4) /* subset of (i) -- performance only */
      return DmtxPass;

   /* Find minimum symbol size big enough to accomodate remaining codewords */
   /* XXX broken -- what if someone asks for DmtxSymbolRectAuto or specific sizeIdx? */

   currentByte = channel->currentLength/12;
   sizeIdx = FindSymbolSize(currentByte, DmtxSymbolSquareAuto);
   /* XXX test for sizeIdx == DmtxUndefined here */
   symbolCodewords = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx) - currentByte;

   /* Test for special case condition */
   if(channel->currentLength % 12 == 0 &&
         (symbolCodewords == 1 || symbolCodewords == 2)) {

      /* Count number of codewords left to write (assuming ASCII) */
      /* XXX temporary hack ... later create function that knows about shifts and digits */
      asciiCodewords = edifactValues;

      if(asciiCodewords <= symbolCodewords) { /* (a,b,d,e,f) */
         ChangeEncScheme(channel, DmtxSchemeAscii, DmtxUnlatchImplicit);

         /* XXX this loop should produce exactly asciiWords codewords ... assert somehow? */
         for(i = 0; i < edifactValues; i++) {
            err = EncodeNextWord(enc, channel, DmtxSchemeAscii);
            if(err == DmtxFail)
               return DmtxFail;
            assert(channel->invalid == 0);
         }
      }
      /* else (c,g) -- do nothing */
   }
   else if(edifactValues == 0) { /* (h) */
      ChangeEncScheme(channel, DmtxSchemeAscii, DmtxUnlatchExplicit);
   }
   /* else (i) -- do nothing */

   return DmtxPass;
}

/**
 * @brief  Convert 3 input values into 2 codewords for triplet-based schemes
 * @param  outputWords
 * @param  inputWord
 * @param  encScheme
 * @return Codeword count
 */
static int
GetC40TextX12Words(int *outputWords, int inputWord, DmtxScheme encScheme)
{
   int count;

   assert(encScheme == DmtxSchemeC40 ||
         encScheme == DmtxSchemeText ||
         encScheme == DmtxSchemeX12);

   count = 0;

   /* Handle extended ASCII with Upper Shift character */
   if(inputWord > 127) {
      if(encScheme == DmtxSchemeX12) {
         return 0;
      }
      else {
         outputWords[count++] = DmtxValueCTXShift2;
         outputWords[count++] = 30;
         inputWord -= 128;
      }
   }

   /* Handle all other characters according to encodation scheme */
   if(encScheme == DmtxSchemeX12) {
      if(inputWord == 13)
         outputWords[count++] = 0;
      else if(inputWord == 42)
         outputWords[count++] = 1;
      else if(inputWord == 62)
         outputWords[count++] = 2;
      else if(inputWord == 32)
         outputWords[count++] = 3;
      else if(inputWord >= 48 && inputWord <= 57)
         outputWords[count++] = inputWord - 44;
      else if(inputWord >= 65 && inputWord <= 90)
         outputWords[count++] = inputWord - 51;
   }
   else { /* encScheme is C40 or Text */
      if(inputWord <= 31) {
         outputWords[count++] = DmtxValueCTXShift1;
         outputWords[count++] = inputWord;
      }
      else if(inputWord == 32) {
         outputWords[count++] = 3;
      }
      else if(inputWord <= 47) {
         outputWords[count++] = DmtxValueCTXShift2;
         outputWords[count++] = inputWord - 33;
      }
      else if(inputWord <= 57) {
         outputWords[count++] = inputWord - 44;
      }
      else if(inputWord <= 64) {
         outputWords[count++] = DmtxValueCTXShift2;
         outputWords[count++] = inputWord - 43;
      }
      else if(inputWord <= 90 && encScheme == DmtxSchemeC40) {
         outputWords[count++] = inputWord - 51;
      }
      else if(inputWord <= 90 && encScheme == DmtxSchemeText) {
         outputWords[count++] = DmtxValueCTXShift3;
         outputWords[count++] = inputWord - 64;
      }
      else if(inputWord <= 95) {
         outputWords[count++] = DmtxValueCTXShift2;
         outputWords[count++] = inputWord - 69;
      }
      else if(inputWord == 96 && encScheme == DmtxSchemeText) {
         outputWords[count++] = DmtxValueCTXShift3;
         outputWords[count++] = 0;
      }
      else if(inputWord <= 122 && encScheme == DmtxSchemeText) {
         outputWords[count++] = inputWord - 83;
      }
      else if(inputWord <= 127) {
         outputWords[count++] = DmtxValueCTXShift3;
         outputWords[count++] = inputWord - 96;
      }
   }

   return count;
}

/**
 * @brief  Convert 2 codewords into 3 values for triplet-based schemes
 * @param  cw1
 * @param  cw2
 * @return C40/Text/X12 data
 */
static DmtxTriplet
GetTripletValues(unsigned char cw1, unsigned char cw2)
{
   int compact;
   DmtxTriplet triplet;

   /* XXX this really belongs with the decode functions */

   compact = (cw1 << 8) | cw2;
   triplet.value[0] = ((compact - 1)/1600);
   triplet.value[1] = ((compact - 1)/40) % 40;
   triplet.value[2] =  (compact - 1) % 40;

   return triplet;
}

/**
 * @brief  Convert 3 codewords into 4 values for quadrulplet-based schemes
 * @param  cw1
 * @param  cw2
 * @param  cw3
 * @return Quadruplet data
 */
static DmtxQuadruplet
GetQuadrupletValues(unsigned char cw1, unsigned char cw2, unsigned char cw3)
{
   DmtxQuadruplet quad;

   /* XXX this really belongs with the decode functions */

   quad.value[0] = cw1 >> 2;
   quad.value[1] = ((cw1 & 0x03) << 4) | ((cw2 & 0xf0) >> 4);
   quad.value[2] = ((cw2 & 0x0f) << 2) | ((cw3 & 0xc0) >> 6);
   quad.value[3] = cw3 & 0x3f;

   return quad;
}

/**
 * @brief  Write channel contents to standard output
 * @param  channel
 * @return void
 */
/**
static void
DumpChannel(DmtxChannel *channel)
{
   int j;

   for(j = 0; j < channel->currentLength / 12; j++)
      fprintf(stdout, "%3d ", channel->encodedWords[j]);

   if(channel->currentLength % 12)
      fprintf(stdout, "%3d-", channel->encodedWords[j]);

   if(channel->invalid & DmtxChannelCannotUnlatch)
      fprintf(stdout, "(can't unlatch right now)");
   else if(channel->invalid & DmtxChannelUnsupportedChar)
      fprintf(stdout, "(unsupported character)");

   fprintf(stdout, "\n");
}
*/

/**
 * @brief  Write all channels' contents to standard output
 * @param  group
 * @param  encTarget
 * @return void
 */
/**
static void
DumpChannelGroup(DmtxChannelGroup *group, int encTarget)
{
   int encScheme, i;
   char *encNames[] = { "ASCII", "C40", "Text", "X12", "EDIFACT", "Base256" };
   DmtxChannel *channel;

   fprintf(stdout, "\n");
   for(encScheme = DmtxSchemeAscii; encScheme <= DmtxSchemeBase256; encScheme++) {
      channel = &(group->channel[encScheme]);
      fprintf(stdout, "%s from %s: ", encNames[encTarget], encNames[encScheme]);
      for(i = 8 + strlen(encNames[encTarget]) + strlen(encNames[encScheme]); i < 24; i++)
         fprintf(stdout, " ");
      DumpChannel(channel);
      fflush(stdout);
   }
}
*/
