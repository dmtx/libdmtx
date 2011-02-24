/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2008, 2009 Mike Laughton

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
 * @file dmtxstatic.h
 * @brief Static header
 */

#ifndef __DMTXSTATIC_H__
#define __DMTXSTATIC_H__

#define DmtxAlmostZero          0.000001
#define DmtxAlmostInfinity            -1

#define DmtxCharC40Latch             230
#define DmtxCharTextLatch            239
#define DmtxCharX12Latch             238
#define DmtxCharEdifactLatch         240
#define DmtxCharBase256Latch         231

#define DmtxCharTripletUnlatch       254
#define DmtxCharEdifactUnlatch        31

#define DmtxCharAsciiPad             129
#define DmtxCharAsciiUpperShift      235
#define DmtxCharTripletShift1          0
#define DmtxCharTripletShift2          1
#define DmtxCharTripletShift3          2
#define DmtxCharFNC1                 232
#define DmtxCharStructuredAppend     233
#define DmtxChar05Macro              236
#define DmtxChar06Macro              237
#define DmtxCharECI                  241

#define DmtxC40TextBasicSet            0
#define DmtxC40TextShift1              1
#define DmtxC40TextShift2              2
#define DmtxC40TextShift3              3

#define DmtxUnlatchExplicit            0
#define DmtxUnlatchImplicit            1

#define DmtxChannelValid            0x00
#define DmtxChannelUnsupportedChar  0x01 << 0
#define DmtxChannelCannotUnlatch    0x01 << 1

#undef min
#define min(X,Y) (((X) < (Y)) ? (X) : (Y))

#undef max
#define max(X,Y) (((X) > (Y)) ? (X) : (Y))

typedef enum {
   DmtxRangeGood,
   DmtxRangeBad,
   DmtxRangeEnd
} DmtxRange;

typedef enum {
   DmtxEdgeTop               = 0x01 << 0,
   DmtxEdgeBottom            = 0x01 << 1,
   DmtxEdgeLeft              = 0x01 << 2,
   DmtxEdgeRight             = 0x01 << 3
} DmtxEdge;

typedef enum {
   DmtxMaskBit8              = 0x01 << 0,
   DmtxMaskBit7              = 0x01 << 1,
   DmtxMaskBit6              = 0x01 << 2,
   DmtxMaskBit5              = 0x01 << 3,
   DmtxMaskBit4              = 0x01 << 4,
   DmtxMaskBit3              = 0x01 << 5,
   DmtxMaskBit2              = 0x01 << 6,
   DmtxMaskBit1              = 0x01 << 7
} DmtxMaskBit;

/**
 * @struct DmtxFollow
 * @brief DmtxFollow
 */
typedef struct DmtxFollow_struct {
   unsigned char  *ptr;
   unsigned char   neighbor;
   int             step;
   DmtxPixelLoc    loc;
} DmtxFollow;

/**
 * @struct DmtxBresLine
 * @brief DmtxBresLine
 */
typedef struct DmtxBresLine_struct {
   int             xStep;
   int             yStep;
   int             xDelta;
   int             yDelta;
   int             steep;
   int             xOut;
   int             yOut;
   int             travel;
   int             outward;
   int             error;
   DmtxPixelLoc    loc;
   DmtxPixelLoc    loc0;
   DmtxPixelLoc    loc1;
} DmtxBresLine;

typedef struct C40TextState_struct {
   int             shift;
   DmtxBoolean     upperShift;
} C40TextState;

/* dmtxregion.c */
static double RightAngleTrueness(DmtxVector2 c0, DmtxVector2 c1, DmtxVector2 c2, double angle);
static DmtxPointFlow MatrixRegionSeekEdge(DmtxDecode *dec, DmtxPixelLoc loc0);
static DmtxPassFail MatrixRegionOrientation(DmtxDecode *dec, DmtxRegion *reg, DmtxPointFlow flowBegin);
static long DistanceSquared(DmtxPixelLoc a, DmtxPixelLoc b);
static int ReadModuleColor(DmtxDecode *dec, DmtxRegion *reg, int symbolRow, int symbolCol, int sizeIdx, int colorPlane);

static DmtxPassFail MatrixRegionFindSize(DmtxDecode *dec, DmtxRegion *reg);
static int CountJumpTally(DmtxDecode *dec, DmtxRegion *reg, int xStart, int yStart, DmtxDirection dir);
static DmtxPointFlow GetPointFlow(DmtxDecode *dec, int colorPlane, DmtxPixelLoc loc, int arrive);
static DmtxPointFlow FindStrongestNeighbor(DmtxDecode *dec, DmtxPointFlow center, int sign);
static DmtxFollow FollowSeek(DmtxDecode *dec, DmtxRegion *reg, int seek);
static DmtxFollow FollowSeekLoc(DmtxDecode *dec, DmtxPixelLoc loc);
static DmtxFollow FollowStep(DmtxDecode *dec, DmtxRegion *reg, DmtxFollow followBeg, int sign);
static DmtxFollow FollowStep2(DmtxDecode *dec, DmtxFollow followBeg, int sign);
static DmtxPassFail TrailBlazeContinuous(DmtxDecode *dec, DmtxRegion *reg, DmtxPointFlow flowBegin, int maxDiagonal);
static int TrailBlazeGapped(DmtxDecode *dec, DmtxRegion *reg, DmtxBresLine line, int streamDir);
static int TrailClear(DmtxDecode *dec, DmtxRegion *reg, int clearMask);
static DmtxBestLine FindBestSolidLine(DmtxDecode *dec, DmtxRegion *reg, int step0, int step1, int streamDir, int houghAvoid);
static DmtxBestLine FindBestSolidLine2(DmtxDecode *dec, DmtxPixelLoc loc0, int tripSteps, int sign, int houghAvoid);
static DmtxPassFail FindTravelLimits(DmtxDecode *dec, DmtxRegion *reg, DmtxBestLine *line);
static DmtxPassFail MatrixRegionAlignCalibEdge(DmtxDecode *dec, DmtxRegion *reg, int whichEdge);
static DmtxBresLine BresLineInit(DmtxPixelLoc loc0, DmtxPixelLoc loc1, DmtxPixelLoc locInside);
static DmtxPassFail BresLineGetStep(DmtxBresLine line, DmtxPixelLoc target, int *travel, int *outward);
static DmtxPassFail BresLineStep(DmtxBresLine *line, int travel, int outward);
/*static void WriteDiagnosticImage(DmtxDecode *dec, DmtxRegion *reg, char *imagePath);*/

/* dmtxdecode.c */
static void DecodeDataStream(DmtxMessage *msg, int sizeIdx, unsigned char *outputStart);
static int GetEncodationScheme(unsigned char *ptr);
static void PushOutputWord(DmtxMessage *msg, int value);
static void PushOutputC40TextWord(DmtxMessage *msg, C40TextState *state, int value);
static void PushOutputMacroHeader(DmtxMessage *msg, int macroType);
static void PushOutputMacroTrailer(DmtxMessage *msg);
static unsigned char *DecodeSchemeAscii(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd);
static unsigned char *DecodeSchemeC40Text(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd, DmtxScheme encScheme);
static unsigned char *DecodeSchemeX12(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd);
static unsigned char *DecodeSchemeEdifact(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd);
static unsigned char *DecodeSchemeBase256(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd);
/* static unsigned char UnRandomize253State(unsigned char codewordValue, int codewordPosition); */
static unsigned char UnRandomize255State(unsigned char value, int idx);
static void TallyModuleJumps(DmtxDecode *dec, DmtxRegion *reg, int tally[][24], int xOrigin, int yOrigin, int mapWidth, int mapHeight, DmtxDirection dir);
static DmtxPassFail PopulateArrayFromMatrix(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);

/* dmtxencode.c */
static int AddPadChars(unsigned char *buf, int *bufSize, int paddedSize);
static unsigned char Randomize253State(unsigned char codewordValue, int codewordPosition);
static unsigned char Randomize255State(unsigned char codewordValue, int codewordPosition);
static void PrintPattern(DmtxEncode *encode);
static void InitChannel(DmtxChannel *channel, unsigned char *codewords, int length);
static int EncodeDataCodewords(DmtxEncode *enc, unsigned char *buf, unsigned char *inputString, int inputSize, int *sizeIdx);
static int EncodeSingleScheme(DmtxEncode *enc, unsigned char *buf, unsigned char *codewords, int length, DmtxScheme scheme);
static int EncodeAutoBest(DmtxEncode *enc, unsigned char *buf, unsigned char *codewords, int length);
/*static int EncodeAutoFast(unsigned char *buf, unsigned char *codewords, int length); */
static DmtxChannel FindBestChannel(DmtxEncode *enc, DmtxChannelGroup group, DmtxScheme targetScheme);
static DmtxPassFail EncodeNextWord(DmtxEncode *enc, DmtxChannel *channel, DmtxScheme targetScheme);
static DmtxPassFail EncodeAsciiCodeword(DmtxChannel *channel);
static DmtxPassFail EncodeTripletCodeword(DmtxEncode *enc, DmtxChannel *channel);
static DmtxPassFail EncodeEdifactCodeword(DmtxEncode *enc, DmtxChannel *channel);
static DmtxPassFail EncodeBase256Codeword(DmtxChannel *channel);
static void ChangeEncScheme(DmtxChannel *channel, DmtxScheme targetScheme, int unlatchType);
static void PushInputWord(DmtxChannel *channel, unsigned char codeword);
static void PushTriplet(DmtxChannel *channel, DmtxTriplet *triplet);
static void IncrementProgress(DmtxChannel *channel, int encodedUnits);
static DmtxPassFail ProcessEndOfSymbolTriplet(DmtxEncode *enc, DmtxChannel *channel, DmtxTriplet *triplet, int tripletCount, int inputCount);
static DmtxPassFail TestForEndOfSymbolEdifact(DmtxEncode *enc, DmtxChannel *channel);
static int GetC40TextX12Words(int *outputWords, int inputWord, DmtxScheme encScheme);
static DmtxTriplet GetTripletValues(unsigned char cw1, unsigned char cw2);
static DmtxQuadruplet GetQuadrupletValues(unsigned char cw1, unsigned char cw2, unsigned char cw3);
/*static void DumpChannel(DmtxChannel *channel);*/
/*static void DumpChannelGroup(DmtxChannelGroup *group, int encTarget);*/

/* dmtxplacemod.c */
static int ModulePlacementEcc200(unsigned char *modules, unsigned char *codewords, int sizeIdx, int moduleOnColor);
static void PatternShapeStandard(unsigned char *modules, int mappingRows, int mappingCols, int row, int col, unsigned char *codeword, int moduleOnColor);
static void PatternShapeSpecial1(unsigned char *modules, int mappingRows, int mappingCols, unsigned char *codeword, int moduleOnColor);
static void PatternShapeSpecial2(unsigned char *modules, int mappingRows, int mappingCols, unsigned char *codeword, int moduleOnColor);
static void PatternShapeSpecial3(unsigned char *modules, int mappingRows, int mappingCols, unsigned char *codeword, int moduleOnColor);
static void PatternShapeSpecial4(unsigned char *modules, int mappingRows, int mappingCols, unsigned char *codeword, int moduleOnColor);
static void PlaceModule(unsigned char *modules, int mappingRows, int mappingCols, int row, int col,
      unsigned char *codeword, int mask, int moduleOnColor);

/* dmtxreedsol.c */
static DmtxPassFail RsEncode(DmtxMessage *message, int sizeIdx);
static DmtxPassFail RsDecode(unsigned char *code, int sizeIdx, int fix);
static DmtxPassFail RsGenPoly(DmtxByteList *gen, int errorWordCount);
static DmtxBoolean RsComputeSyndromes(DmtxByteList *syn, const DmtxByteList *rec, int blockErrorWords);
static DmtxBoolean RsFindErrorLocatorPoly(DmtxByteList *elp, const DmtxByteList *syn, int errorWordCount, int maxCorrectable);
static DmtxBoolean RsFindErrorLocations(DmtxByteList *loc, const DmtxByteList *elp);
static void RsRepairErrors(DmtxByteList *rec, const DmtxByteList *loc, const DmtxByteList *elp, const DmtxByteList *syn);

/* dmtxscangrid.c */
static DmtxScanGrid InitScanGrid(DmtxDecode *dec);
static int PopGridLocation(DmtxScanGrid *grid, /*@out@*/ DmtxPixelLoc *locPtr);
static int GetGridCoordinates(DmtxScanGrid *grid, /*@out@*/ DmtxPixelLoc *locPtr);
static void SetDerivedFields(DmtxScanGrid *grid);

/* dmtxsymbol.c */
static int FindSymbolSize(int dataWords, int sizeIdxRequest);

/* dmtximage.c */
static int GetBitsPerPixel(int pack);

/* dmtxencodestream.c */
static DmtxEncodeStream StreamInit(DmtxByte *input, int inputLength, DmtxByte *output, int outputLength);
static void StreamMarkComplete(DmtxEncodeStream *stream);
static void StreamMarkInvalid(DmtxEncodeStream *stream, int reason);
static void StreamMarkFatal(DmtxEncodeStream *stream, int reason);
static void StreamOutputChainAppend(DmtxEncodeStream *stream, DmtxByte value);
static DmtxByte StreamOutputChainRemoveLast(DmtxEncodeStream *stream);
static void StreamOutputChainPrepend(DmtxEncodeStream *stream, DmtxByte value);
static DmtxByte StreamOutputChainRemoveFirst(DmtxEncodeStream *stream);
static void StreamOutputChainSet(DmtxEncodeStream stream, int i, DmtxByte value);
static DmtxBoolean StreamInputHasNext(DmtxEncodeStream *stream);
static DmtxByte StreamInputPeekNext(DmtxEncodeStream *stream);
static DmtxByte StreamInputAdvanceNext(DmtxEncodeStream *stream);

/* dmtxencodescheme.c */
static DmtxPassFail EncodeSingleScheme2(DmtxEncodeStream *stream, DmtxScheme targetScheme, int requestedSizeIdx);
static void EncodeChangeScheme(DmtxEncodeStream *stream, DmtxScheme targetScheme, int unlatchType);
static void EncodeNextWord2(DmtxEncodeStream *stream, DmtxScheme targetScheme, int requestedSizeIdx);
static void EncodeNextWordAscii(DmtxEncodeStream *stream);
static void CompleteIfDoneAscii(DmtxEncodeStream *stream);
static void EncodeNextWordTriplet(DmtxEncodeStream *stream, DmtxScheme targetScheme);
static void CompleteIfDoneTriplet(DmtxEncodeStream *stream);
static void EncodeNextWordEdifact(DmtxEncodeStream *stream, int requestedSizeIdx);
static void CompleteIfDoneEdifact(DmtxEncodeStream *stream, int requestedSizeIdx);
static void EncodeNextWordBase256(DmtxEncodeStream *stream);
static void CompleteIfDoneBase256(DmtxEncodeStream *stream);

static const int dmtxNeighborNone = 8;
static const int dmtxPatternX[] = { -1,  0,  1,  1,  1,  0, -1, -1 };
static const int dmtxPatternY[] = { -1, -1, -1,  0,  1,  1,  1,  0 };
static const DmtxPointFlow dmtxBlankEdge = { 0, 0, 0, DmtxUndefined, { -1, -1 } };

/*@ +charint @*/

static int rHvX[] =
    {  256,  256,  256,  256,  255,  255,  255,  254,  254,  253,  252,  251,  250,  249,  248,
       247,  246,  245,  243,  242,  241,  239,  237,  236,  234,  232,  230,  228,  226,  224,
       222,  219,  217,  215,  212,  210,  207,  204,  202,  199,  196,  193,  190,  187,  184,
       181,  178,  175,  171,  168,  165,  161,  158,  154,  150,  147,  143,  139,  136,  132,
       128,  124,  120,  116,  112,  108,  104,  100,   96,   92,   88,   83,   79,   75,   71,
        66,   62,   58,   53,   49,   44,   40,   36,   31,   27,   22,   18,   13,    9,    4,
         0,   -4,   -9,  -13,  -18,  -22,  -27,  -31,  -36,  -40,  -44,  -49,  -53,  -58,  -62,
       -66,  -71,  -75,  -79,  -83,  -88,  -92,  -96, -100, -104, -108, -112, -116, -120, -124,
      -128, -132, -136, -139, -143, -147, -150, -154, -158, -161, -165, -168, -171, -175, -178,
      -181, -184, -187, -190, -193, -196, -199, -202, -204, -207, -210, -212, -215, -217, -219,
      -222, -224, -226, -228, -230, -232, -234, -236, -237, -239, -241, -242, -243, -245, -246,
      -247, -248, -249, -250, -251, -252, -253, -254, -254, -255, -255, -255, -256, -256, -256 };

static int rHvY[] =
    {    0,    4,    9,   13,   18,   22,   27,   31,   36,   40,   44,   49,   53,   58,   62,
        66,   71,   75,   79,   83,   88,   92,   96,  100,  104,  108,  112,  116,  120,  124,
       128,  132,  136,  139,  143,  147,  150,  154,  158,  161,  165,  168,  171,  175,  178,
       181,  184,  187,  190,  193,  196,  199,  202,  204,  207,  210,  212,  215,  217,  219,
       222,  224,  226,  228,  230,  232,  234,  236,  237,  239,  241,  242,  243,  245,  246,
       247,  248,  249,  250,  251,  252,  253,  254,  254,  255,  255,  255,  256,  256,  256,
       256,  256,  256,  256,  255,  255,  255,  254,  254,  253,  252,  251,  250,  249,  248,
       247,  246,  245,  243,  242,  241,  239,  237,  236,  234,  232,  230,  228,  226,  224,
       222,  219,  217,  215,  212,  210,  207,  204,  202,  199,  196,  193,  190,  187,  184,
       181,  178,  175,  171,  168,  165,  161,  158,  154,  150,  147,  143,  139,  136,  132,
       128,  124,  120,  116,  112,  108,  104,  100,   96,   92,   88,   83,   79,   75,   71,
        66,   62,   58,   53,   49,   44,   40,   36,   31,   27,   22,   18,   13,    9,    4 };

/*@ -charint @*/

#endif
