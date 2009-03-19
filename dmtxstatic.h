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

#define DmtxAlmostZero                 0.000001
#define DmtxAlmostInfinity            -1

#define DmtxC40TextBasicSet            0
#define DmtxC40TextShift1              1
#define DmtxC40TextShift2              2
#define DmtxC40TextShift3              3

#define DMTX_CHAR_ASCII_PAD            129
#define DMTX_CHAR_ASCII_UPPER_SHIFT    235
#define DMTX_CHAR_C40_LATCH            230
#define DMTX_CHAR_TEXT_LATCH           239
#define DMTX_CHAR_X12_LATCH            238
#define DMTX_CHAR_TRIPLET_UNLATCH      254
#define DMTX_CHAR_TRIPLET_SHIFT_1        0
#define DMTX_CHAR_TRIPLET_SHIFT_2        1
#define DMTX_CHAR_TRIPLET_SHIFT_3        2
#define DMTX_CHAR_EDIFACT_LATCH        240
#define DMTX_CHAR_EDIFACT_UNLATCH       31
#define DMTX_CHAR_BASE256_LATCH        231
#define DMTX_CHAR_FNC1                 232
#define DMTX_CHAR_STRUCTURED_APPEND    233
#define DMTX_CHAR_ECI                  241
#define DMTX_CHAR_05MACRO              236
#define DMTX_CHAR_06MACRO              237

#define DMTX_CHANNEL_VALID            0x00
#define DMTX_CHANNEL_UNSUPPORTED_CHAR 0x01 << 0
#define DMTX_CHANNEL_CANNOT_UNLATCH   0x01 << 1

#define DMTX_UNLATCH_EXPLICIT            0
#define DMTX_UNLATCH_IMPLICIT            1

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
   DmtxEdgeTop    = 0x01 << 0,
   DmtxEdgeBottom = 0x01 << 1,
   DmtxEdgeLeft   = 0x01 << 2,
   DmtxEdgeRight  = 0x01 << 3
} DmtxEdge;

typedef enum {
   DmtxMaskBit1 = 0x01 << 7,
   DmtxMaskBit2 = 0x01 << 6,
   DmtxMaskBit3 = 0x01 << 5,
   DmtxMaskBit4 = 0x01 << 4,
   DmtxMaskBit5 = 0x01 << 3,
   DmtxMaskBit6 = 0x01 << 2,
   DmtxMaskBit7 = 0x01 << 1,
   DmtxMaskBit8 = 0x01 << 0
} DmtxMaskBit;

/**
 * @struct DmtxFollow
 * @brief DmtxFollow
 */
typedef struct DmtxFollow_struct {
   unsigned char *ptr;
   unsigned char neighbor;
   int step;
   DmtxPixelLoc loc;
} DmtxFollow;

/**
 * @struct DmtxBresLine
 * @brief DmtxBresLine
 */
typedef struct DmtxBresLine_struct {
   int xStep;
   int yStep;
   int xDelta;
   int yDelta;
   int steep;
   int xOut;
   int yOut;
   int travel;
   int outward;
   int error;
   DmtxPixelLoc loc;
   DmtxPixelLoc loc0;
   DmtxPixelLoc loc1;
} DmtxBresLine;

typedef struct C40TextState_struct {
   int shift;
   DmtxBoolean upperShift;
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
static unsigned char *NextEncodationScheme(DmtxSchemeDecode *encScheme, unsigned char *ptr);
static void PushOutputWord(DmtxMessage *msg, int value);
static void PushOutputC40TextWord(DmtxMessage *msg, C40TextState *state, int value);
static unsigned char *DecodeSchemeAsciiStd(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd);
static unsigned char *DecodeSchemeAsciiExt(DmtxMessage *msg, unsigned char *ptr);
static unsigned char *DecodeSchemeC40Text(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd, DmtxSchemeDecode encScheme);
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
static int EncodeSingleScheme(DmtxEncode *enc, unsigned char *buf, unsigned char *codewords, int length, DmtxSchemeEncode scheme);
static int EncodeAutoBest(DmtxEncode *enc, unsigned char *buf, unsigned char *codewords, int length);
/*static int EncodeAutoFast(unsigned char *buf, unsigned char *codewords, int length); */
static DmtxChannel FindBestChannel(DmtxEncode *enc, DmtxChannelGroup group, DmtxSchemeEncode targetScheme);
static DmtxPassFail EncodeNextWord(DmtxEncode *enc, DmtxChannel *channel, DmtxSchemeEncode targetScheme);
static DmtxPassFail EncodeAsciiCodeword(DmtxChannel *channel);
static DmtxPassFail EncodeTripletCodeword(DmtxEncode *enc, DmtxChannel *channel);
static DmtxPassFail EncodeEdifactCodeword(DmtxEncode *enc, DmtxChannel *channel);
static DmtxPassFail EncodeBase256Codeword(DmtxChannel *channel);
static void ChangeEncScheme(DmtxChannel *channel, DmtxSchemeEncode targetScheme, int unlatchType);
static void PushInputWord(DmtxChannel *channel, unsigned char codeword);
static void PushTriplet(DmtxChannel *channel, DmtxTriplet *triplet);
static void IncrementProgress(DmtxChannel *channel, int encodedUnits);
static DmtxPassFail ProcessEndOfSymbolTriplet(DmtxEncode *enc, DmtxChannel *channel, DmtxTriplet *triplet, int tripletCount, int inputCount);
static DmtxPassFail TestForEndOfSymbolEdifact(DmtxEncode *enc, DmtxChannel *channel);
static int GetC40TextX12Words(int *outputWords, int inputWord, DmtxSchemeEncode encScheme);
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
static void GenReedSolEcc(DmtxMessage *msg, int sizeidx);
static int DecodeCheckErrors(unsigned char *code, int sizeIdx, int fix);
static int GfProduct(int a, int b);
static int GfDoublify(int a, int b);

/* dmtxscangrid.c */
static DmtxScanGrid InitScanGrid(DmtxDecode *dec);
static int PopGridLocation(DmtxScanGrid *grid, /*@out@*/ DmtxPixelLoc *locPtr);
static int GetGridCoordinates(DmtxScanGrid *grid, /*@out@*/ DmtxPixelLoc *locPtr);
static void SetDerivedFields(DmtxScanGrid *grid);

/* dmtxsymbol.c */
static int FindCorrectSymbolSize(int dataWords, int symbolShape);

/* dmtximage.c */
static int GetBitsPerPixel(int pack);

static const int dmtxNeighborNone = 8;
static const int dmtxPatternX[] = { -1,  0,  1,  1,  1,  0, -1, -1 };
static const int dmtxPatternY[] = { -1, -1, -1,  0,  1,  1,  1,  0 };
static const DmtxPointFlow dmtxBlankEdge = { 0, 0, 0, DmtxUndefined, { -1, -1 } };

/*@ +charint @*/

/* Galois Field log values */
static int logVal[] =
   {-255, 255,   1, 240,   2, 225, 241,  53,   3,  38, 226, 133, 242,  43,  54, 210,
       4, 195,  39, 114, 227, 106, 134,  28, 243, 140,  44,  23,  55, 118, 211, 234,
       5, 219, 196,  96,  40, 222, 115, 103, 228,  78, 107, 125, 135,   8,  29, 162,
     244, 186, 141, 180,  45,  99,  24,  49,  56,  13, 119, 153, 212, 199, 235,  91,
       6,  76, 220, 217, 197,  11,  97, 184,  41,  36, 223, 253, 116, 138, 104, 193,
     229,  86,  79, 171, 108, 165, 126, 145, 136,  34,   9,  74,  30,  32, 163,  84,
     245, 173, 187, 204, 142,  81, 181, 190,  46,  88, 100, 159,  25, 231,  50, 207,
      57, 147,  14,  67, 120, 128, 154, 248, 213, 167, 200,  63, 236, 110,  92, 176,
       7, 161,  77, 124, 221, 102, 218,  95, 198,  90,  12, 152,  98,  48, 185, 179,
      42, 209,  37, 132, 224,  52, 254, 239, 117, 233, 139,  22, 105,  27, 194, 113,
     230, 206,  87, 158,  80, 189, 172, 203, 109, 175, 166,  62, 127, 247, 146,  66,
     137, 192,  35, 252,  10, 183,  75, 216,  31,  83,  33,  73, 164, 144,  85, 170,
     246,  65, 174,  61, 188, 202, 205, 157, 143, 169,  82,  72, 182, 215, 191, 251,
      47, 178,  89, 151, 101,  94, 160, 123,  26, 112, 232,  21,  51, 238, 208, 131,
      58,  69, 148,  18,  15,  16,  68,  17, 121, 149, 129,  19, 155,  59, 249,  70,
     214, 250, 168,  71, 201, 156,  64,  60, 237, 130, 111,  20,  93, 122, 177, 150 };

/* log(0) = -inf */
static unsigned char indexOf[] =
   { 255,   0,   1, 240,   2, 225, 241,  53,   3,  38, 226, 133, 242,  43,  54, 210,
       4, 195,  39, 114, 227, 106, 134,  28, 243, 140,  44,  23,  55, 118, 211, 234,
       5, 219, 196,  96,  40, 222, 115, 103, 228,  78, 107, 125, 135,   8,  29, 162,
     244, 186, 141, 180,  45,  99,  24,  49,  56,  13, 119, 153, 212, 199, 235,  91,
       6,  76, 220, 217, 197,  11,  97, 184,  41,  36, 223, 253, 116, 138, 104, 193,
     229,  86,  79, 171, 108, 165, 126, 145, 136,  34,   9,  74,  30,  32, 163,  84,
     245, 173, 187, 204, 142,  81, 181, 190,  46,  88, 100, 159,  25, 231,  50, 207,
      57, 147,  14,  67, 120, 128, 154, 248, 213, 167, 200,  63, 236, 110,  92, 176,
       7, 161,  77, 124, 221, 102, 218,  95, 198,  90,  12, 152,  98,  48, 185, 179,
      42, 209,  37, 132, 224,  52, 254, 239, 117, 233, 139,  22, 105,  27, 194, 113,
     230, 206,  87, 158,  80, 189, 172, 203, 109, 175, 166,  62, 127, 247, 146,  66,
     137, 192,  35, 252,  10, 183,  75, 216,  31,  83,  33,  73, 164, 144,  85, 170,
     246,  65, 174,  61, 188, 202, 205, 157, 143, 169,  82,  72, 182, 215, 191, 251,
      47, 178,  89, 151, 101,  94, 160, 123,  26, 112, 232,  21,  51, 238, 208, 131,
      58,  69, 148,  18,  15,  16,  68,  17, 121, 149, 129,  19, 155,  59, 249,  70,
     214, 250, 168,  71, 201, 156,  64,  60, 237, 130, 111,  20,  93, 122, 177, 150 };

/* Galois Field antilog values */
static int aLogVal[] =
   {   1,   2,   4,   8,  16,  32,  64, 128,  45,  90, 180,  69, 138,  57, 114, 228,
     229, 231, 227, 235, 251, 219, 155,  27,  54, 108, 216, 157,  23,  46,  92, 184,
      93, 186,  89, 178,  73, 146,   9,  18,  36,  72, 144,  13,  26,  52, 104, 208,
     141,  55, 110, 220, 149,   7,  14,  28,  56, 112, 224, 237, 247, 195, 171, 123,
     246, 193, 175, 115, 230, 225, 239, 243, 203, 187,  91, 182,  65, 130,  41,  82,
     164, 101, 202, 185,  95, 190,  81, 162, 105, 210, 137,  63, 126, 252, 213, 135,
      35,  70, 140,  53, 106, 212, 133,  39,  78, 156,  21,  42,  84, 168, 125, 250,
     217, 159,  19,  38,  76, 152,  29,  58, 116, 232, 253, 215, 131,  43,  86, 172,
     117, 234, 249, 223, 147,  11,  22,  44,  88, 176,  77, 154,  25,  50, 100, 200,
     189,  87, 174, 113, 226, 233, 255, 211, 139,  59, 118, 236, 245, 199, 163, 107,
     214, 129,  47,  94, 188,  85, 170, 121, 242, 201, 191,  83, 166,  97, 194, 169,
     127, 254, 209, 143,  51, 102, 204, 181,  71, 142,  49,  98, 196, 165, 103, 206,
     177,  79, 158,  17,  34,  68, 136,  61, 122, 244, 197, 167,  99, 198, 161, 111,
     222, 145,  15,  30,  60, 120, 240, 205, 183,  67, 134,  33,  66, 132,  37,  74,
     148,   5,  10,  20,  40,  80, 160, 109, 218, 153,  31,  62, 124, 248, 221, 151,
       3,   6,  12,  24,  48,  96, 192, 173, 119, 238, 241, 207, 179,  75, 150,   1 };

/* alpha**-inf = 0 */
static unsigned char alphaTo[] =
   {   1,   2,   4,   8,  16,  32,  64, 128,  45,  90, 180,  69, 138,  57, 114, 228,
     229, 231, 227, 235, 251, 219, 155,  27,  54, 108, 216, 157,  23,  46,  92, 184,
      93, 186,  89, 178,  73, 146,   9,  18,  36,  72, 144,  13,  26,  52, 104, 208,
     141,  55, 110, 220, 149,   7,  14,  28,  56, 112, 224, 237, 247, 195, 171, 123,
     246, 193, 175, 115, 230, 225, 239, 243, 203, 187,  91, 182,  65, 130,  41,  82,
     164, 101, 202, 185,  95, 190,  81, 162, 105, 210, 137,  63, 126, 252, 213, 135,
      35,  70, 140,  53, 106, 212, 133,  39,  78, 156,  21,  42,  84, 168, 125, 250,
     217, 159,  19,  38,  76, 152,  29,  58, 116, 232, 253, 215, 131,  43,  86, 172,
     117, 234, 249, 223, 147,  11,  22,  44,  88, 176,  77, 154,  25,  50, 100, 200,
     189,  87, 174, 113, 226, 233, 255, 211, 139,  59, 118, 236, 245, 199, 163, 107,
     214, 129,  47,  94, 188,  85, 170, 121, 242, 201, 191,  83, 166,  97, 194, 169,
     127, 254, 209, 143,  51, 102, 204, 181,  71, 142,  49,  98, 196, 165, 103, 206,
     177,  79, 158,  17,  34,  68, 136,  61, 122, 244, 197, 167,  99, 198, 161, 111,
     222, 145,  15,  30,  60, 120, 240, 205, 183,  67, 134,  33,  66, 132,  37,  74,
     148,   5,  10,  20,  40,  80, 160, 109, 218, 153,  31,  62, 124, 248, 221, 151,
       3,   6,  12,  24,  48,  96, 192, 173, 119, 238, 241, 207, 179,  75, 150,   0 };
/**
static int rHvX[] =
    { 256,  256,  255,  255,  254,  252,  250,  248,  246,  243,  241,  237,  234,  230,  226,
      222,  217,  212,  207,  202,  196,  190,  184,  178,  171,  165,  158,  150,  143,  136,
      128,  120,  112,  104,   96,   88,   79,   71,   62,   53,   44,   36,   27,   18,    9,
        0,   -9,  -18,  -27,  -36,  -44,  -53,  -62,  -71,  -79,  -88,  -96, -104, -112, -120,
     -128, -136, -143, -150, -158, -165, -171, -178, -184, -190, -196, -202, -207, -212, -217,
     -222, -226, -230, -234, -237, -241, -243, -246, -248, -250, -252, -254, -255, -255, -256 };

static int rHvY[] =
    {   0,    9,   18,   27,   36,   44,   53,   62,   71,   79,   88,   96,  104,  112,  120,
      128,  136,  143,  150,  158,  165,  171,  178,  184,  190,  196,  202,  207,  212,  217,
      222,  226,  230,  234,  237,  241,  243,  246,  248,  250,  252,  254,  255,  255,  256,
      256,  256,  255,  255,  254,  252,  250,  248,  246,  243,  241,  237,  234,  230,  226,
      222,  217,  212,  207,  202,  196,  190,  184,  178,  171,  165,  158,  150,  143,  136,
      128,  120,  112,  104,   96,   88,   79,   71,   62,   53,   44,   36,   27,   18,    9 };
*/

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
