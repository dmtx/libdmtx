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

#ifdef CALLBACKS
#define CALLBACK_DECODE_FUNC1(f,d,p1)             if(d && d->f) (*(d->f))(p1)
#define CALLBACK_DECODE_FUNC2(f,d,p1,p2)          if(d && d->f) (*(d->f))(p1,p2)
#define CALLBACK_DECODE_FUNC3(f,d,p1,p2,p3)       if(d && d->f) (*(d->f))(p1,p2,p3)
#define CALLBACK_DECODE_FUNC4(f,d,p1,p2,p3,p4)    if(d && d->f) (*(d->f))(p1,p2,p3,p4)
#define CALLBACK_DECODE_FUNC5(f,d,p1,p2,p3,p4,p5) if(d && d->f) (*(d->f))(p1,p2,p3,p4,p5)
#else
#define CALLBACK_DECODE_FUNC1(f,d,p1)
#define CALLBACK_DECODE_FUNC2(f,d,p1,p2)
#define CALLBACK_DECODE_FUNC3(f,d,p1,p2,p3)
#define CALLBACK_DECODE_FUNC4(f,d,p1,p2,p3,p4)
#define CALLBACK_DECODE_FUNC5(f,d,p1,p2,p3,p4,p5)
#endif

#define DMTX_END_OF_RANGE      5
#define DMTX_EDGE_FOUND        6

#define DMTX_TURN_CW           1
#define DMTX_TURN_CCW          2

#define DMTX_ALMOST_ZERO       0.000001
#define DMTX_ALMOST_INFINITY  -1

#define DMTX_FALSE             0
#define DMTX_TRUE              1

#define DMTX_EDGE_STEP_TOO_WEAK       1
#define DMTX_EDGE_STEP_PERPENDICULAR  2
#define DMTX_EDGE_STEP_NOT_QUITE      3
#define DMTX_EDGE_STEP_TOO_FAR        4
#define DMTX_EDGE_STEP_EXACT          5

#ifndef min
#define min(X,Y) ((X < Y) ? X : Y)
#endif

#ifndef max
#define max(X,Y) ((X > Y) ? X : Y)
#endif

typedef enum {
   DmtxEdgeTop    = 0x01 << 0,
   DmtxEdgeBottom = 0x01 << 1,
   DmtxEdgeLeft   = 0x01 << 2,
   DmtxEdgeRight  = 0x01 << 3
} DmtxEdgeLoc;

typedef enum {
   DmtxMaskBit1 = 0x01 << 7,
   DmtxMaskBit2 = 0x01 << 6,
   DmtxMaskBit3 = 0x01 << 5,
   DmtxMaskBit4 = 0x01 << 4,
   DmtxMaskBit5 = 0x01 << 3,
   DmtxMaskBit6 = 0x01 << 2,
   DmtxMaskBit7 = 0x01 << 1,
   DmtxMaskBit8 = 0x01 << 0
} DmtxBitMask;

#define DMTX_ALL_COMPASS_DIRS 0x0f

typedef struct DmtxEdgeSubPixel_struct {
   int             isEdge;
   int             xInt;
   int             yInt;
   double          xFrac;
   double          yFrac;
   DmtxCompassEdge compass;
} DmtxEdgeSubPixel;

/* dmtxregion.c */
static void ClampIntRange(int *value, int min, int max);
static DmtxCompassEdge GetCompassEdge(DmtxImage *image, int x, int y, int edgeScanDirs);
static DmtxEdgeSubPixel FindZeroCrossing(DmtxImage *image, int x, int y, DmtxCompassEdge compassStart);
static DmtxRay2 FollowEdge(DmtxImage *image, int x, int y, DmtxEdgeSubPixel edgeStart, int forward, DmtxDecode *decode);
static double RightAngleTrueness(DmtxVector2 c0, DmtxVector2 c1, DmtxVector2 c2, double angle);
static int MatrixRegionUpdateXfrms(DmtxRegion *region, DmtxImage *image);
static int MatrixRegionAlignFirstEdge(DmtxDecode *decode, DmtxEdgeSubPixel *edgeStart, DmtxRay2 ray0, DmtxRay2 ray1);
static void SetCornerLoc(DmtxRegion *region, DmtxCornerLoc cornerLoc, DmtxVector2 point);
static int MatrixRegionAlignSecondEdge(DmtxDecode *decode);
static int MatrixRegionAlignRightEdge(DmtxDecode *decode);
static int MatrixRegionAlignTopEdge( DmtxDecode *decode);
static int MatrixRegionAlignCalibEdge(DmtxDecode *decode, DmtxEdgeLoc edgeLoc, DmtxMatrix3 preFit2Raw, DmtxMatrix3 postRaw2Fit);
static int MatrixRegionAlignEdge(DmtxDecode *decode, DmtxMatrix3 postRaw2Fit, DmtxMatrix3 preFit2Raw, DmtxVector2 *p0, DmtxVector2 *p1, DmtxVector2 *pCorner, int *weakCount);
static int StepAlongEdge(DmtxImage *image, DmtxRegion *region, DmtxVector2 *pProgress, DmtxVector2 *pExact, DmtxVector2 forward, DmtxVector2 lateral, DmtxDecode *decode);
static int AllocateStorage(DmtxDecode *decode);
static DmtxColor3 ReadModuleColor(DmtxDecode *decode, int symbolRow, int symbolCol, int sizeIdx, DmtxMatrix3 fit2raw);
static int MatrixRegionFindSize(DmtxDecode *decode);
static int PopulateArrayFromMatrix(DmtxDecode *decode);
static void TallyModuleJumps(DmtxDecode *decode, int tally[][24], int xOrigin, int yOrigin, int mapWidth, int mapHeight, int direction);
static int PopulateArrayFromMosaic(DmtxDecode *decode);

/* dmtxdecode.c */
static int DecodeMatrixRegion(DmtxRegion *region);
static void DecodeDataStream(DmtxRegion *region);
static unsigned char *NextEncodationScheme(DmtxSchemeDecode *encScheme, unsigned char *ptr);
static unsigned char *DecodeSchemeAsciiStd(DmtxRegion *region, unsigned char *ptr, unsigned char *dataEnd);
static unsigned char *DecodeSchemeAsciiExt(DmtxRegion *region, unsigned char *ptr, unsigned char *dataEnd);
static unsigned char *DecodeSchemeC40Text(DmtxRegion *region, unsigned char *ptr, unsigned char *dataEnd, DmtxSchemeDecode encScheme);
static unsigned char *DecodeSchemeX12(DmtxRegion *region, unsigned char *ptr, unsigned char *dataEnd);
static unsigned char *DecodeSchemeEdifact(DmtxRegion *region, unsigned char *ptr, unsigned char *dataEnd);
static unsigned char *DecodeSchemeBase256(DmtxRegion *region, unsigned char *ptr, unsigned char *dataEnd);
/* static unsigned char UnRandomize253State(unsigned char codewordValue, int codewordPosition); XXX need later */
static unsigned char UnRandomize255State(unsigned char value, int idx);

/* dmtxencode.c */
static void AddPadChars(unsigned char *buf, int *bufSize, int paddedSize);
static unsigned char Randomize253State(unsigned char codewordValue, int codewordPosition);
static unsigned char Randomize255State(unsigned char codewordValue, int codewordPosition);
static void PatternInit(DmtxRegion *region);
static void PrintPattern(DmtxEncode *encode);
static void InitChannel(DmtxChannel *channel, unsigned char *codewords, int length);
static int EncodeSingleScheme(unsigned char *buf, unsigned char *codewords, int length, DmtxSchemeEncode scheme);
static int EncodeAutoBest(unsigned char *buf, unsigned char *codewords, int length);
/*static int EncodeAutoFast(unsigned char *buf, unsigned char *codewords, int length); */
static DmtxChannel FindBestChannel(DmtxChannelGroup group, DmtxSchemeEncode targetScheme);
static void EncodeNextWord(DmtxChannel *channel, DmtxSchemeEncode targetScheme);
static void EncodeAsciiCodeword(DmtxChannel *channel);
static void EncodeTripletCodeword(DmtxChannel *channel);
static void EncodeEdifactCodeword(DmtxChannel *channel);
static void EncodeBase256Codeword(DmtxChannel *channel);
static void ChangeEncScheme(DmtxChannel *channel, DmtxSchemeEncode targetScheme, int unlatchType);
static void PushInputWord(DmtxChannel *channel, unsigned char codeword);
static void PushTriplet(DmtxChannel *channel, DmtxTriplet *triplet);
static void IncrementProgress(DmtxChannel *channel, int encodedUnits);
static void ProcessEndOfSymbolTriplet(DmtxChannel *channel, DmtxTriplet *triplet, int tripletCount, int inputCount);
static void TestForEndOfSymbolEdifact(DmtxChannel *channel);
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
      unsigned char *codeword, DmtxBitMask mask, int moduleOnColor);

/* dmtxreedsol.c */
static void GenReedSolEcc(DmtxRegion *region);
static int DecodeCheckErrors(DmtxRegion *region);
static int GfProduct(int a, int b);
static int GfDoublify(int a, int b);

/* dmtxscangrid.c */
static DmtxScanGrid InitScanGrid(DmtxImage *image, DmtxPixelLoc p0, DmtxPixelLoc p1, int minGapSize);
static void IncrementPixelProgress(DmtxScanGrid *cross);
static void SetDerivedFields(DmtxScanGrid *cross);
static DmtxPixelLoc GetGridCoordinates(DmtxScanGrid *grid);

/* dmtxsymbol.c */
static int FindCorrectBarcodeSize(int symbolShape, int dataWords);

/* Galois Field log values */
static int logVal[] =
   { -255, 255,   1, 240,   2, 225, 241,  53,   3,  38, 226, 133, 242,  43,  54, 210,
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
   {    1,   2,   4,   8,  16,  32,  64, 128,  45,  90, 180,  69, 138,  57, 114, 228,
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
