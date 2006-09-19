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

/* $Id: dmtxstatic.h,v 1.3 2006-09-19 05:28:53 mblaughton Exp $ */

#define DMTX_END_OF_RANGE      5
#define DMTX_EDGE_FOUND        6

#define DMTX_TURN_CW           1
#define DMTX_TURN_CCW          2

#define DMTX_ALMOST_ZERO       0.000001

#define DMTX_FALSE             0
#define DMTX_TRUE              1

#define DMTX_ENCODING_AUTO     1
#define DMTX_ENCODING_ASCII    2
#define DMTX_ENCODING_C40      3
#define DMTX_ENCODING_TEXT     4
#define DMTX_ENCODING_BASE256  5

#define DMTX_MODULE_OFF        0x00
#define DMTX_MODULE_ON         0x01
#define DMTX_MODULE_ASSIGNED   0x02
#define DMTX_MODULE_VISITED    0x04

#ifndef min
#define min(X,Y) ((X < Y) ? X : Y)
#endif

#ifndef max
#define max(X,Y) ((X > Y) ? X : Y)
#endif

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

typedef enum {
   DmtxSchemeAsciiStd,
   DmtxSchemeAsciiExt,
   DmtxSchemeC40,
   DmtxSchemeText,
   DmtxSchemeX12,
   DmtxSchemeEdifact,
   DmtxSchemeBase256
} DmtxEncScheme;

/* dmtxregion.c */
static void ScanRangeSet(DmtxScanRange *range, DmtxDirection dir, int lineNbr, int firstPos, int lastPos);
static DmtxDirection TurnCorner(DmtxDirection dir, int turn);
static void JumpScanInit(DmtxJumpScan *jump, DmtxDirection dir, int lineNbr, int start, int length);
static int  JumpScanNextRegion(DmtxJumpScan *jumpScan, DmtxDecode *decode);
static void JumpRegionIncrement(DmtxJumpRegion *jumpRegion);
static int  JumpRegionValid(DmtxJumpRegion *jumpRegion);
static void EdgeScanInit(DmtxEdgeScan *edgeScan, DmtxJumpScan *jumpScan);
static int  EdgeScanNextEdge(DmtxEdgeScan *edgeScan, DmtxDecode *decode, DmtxGradient *gradient);
static void EdgeFollowerInit(DmtxEdgeFollower *follow, DmtxEdgeScan *edgeScan, DmtxGradient *gradient, DmtxDirection dir);
static DmtxVector2 EdgeFollowerGetLoc(DmtxEdgeFollower *follow);
static int EdgeFollowerFollow(DmtxEdgeFollower *follow, DmtxDecode *decode);
static int EdgeFollowerIncrement(DmtxEdgeFollower *follow, DmtxDecode *decode);
static void MatrixRegionInit(DmtxMatrixRegion *matrixRegion, DmtxGradient *gradient);
static void Matrix3ChainXfrm(DmtxMatrix3 m, DmtxChain *chain);
static void Matrix3ChainInvXfrm(DmtxMatrix3 m, DmtxChain *chain);
static void MatrixRegionUpdateXfrms(DmtxMatrixRegion *matrixRegion);
static int MatrixRegionAlignFinderBars(DmtxMatrixRegion *matrixRegion, DmtxDecode *decode, DmtxEdgeFollower *f0, DmtxEdgeFollower *f1);
static int MatrixRegionAlignTop(DmtxMatrixRegion *matrixRegion, DmtxDecode *decode);
static int MatrixRegionAlignSide(DmtxMatrixRegion *matrixRegion, DmtxDecode *decode);
static int MatrixRegionEstimateSize(DmtxMatrixRegion *matrixRegion, DmtxDecode *decode);

/* dmtxdecode.c */
static int MatrixRegionRead(DmtxMatrixRegion *matrixRegion, DmtxDecode *decode);
static int PatternReadNonDataModules(DmtxMatrixRegion *matrixRegion, DmtxDecode *decode);
static void PopulateArrayFromImage(DmtxMatrixRegion *matrixRegion, DmtxDecode *decode);
static int CheckErrors(DmtxMatrixRegion *matrixRegion);
static void DataStreamDecode(DmtxMatrixRegion *matrixRegion);
static unsigned char *NextEncodationScheme(DmtxEncScheme *encScheme, unsigned char *ptr);
static unsigned char *DecodeSchemeAsciiStd(DmtxMatrixRegion *matrixRegion, unsigned char *ptr, unsigned char *dataEnd);
static unsigned char *DecodeSchemeAsciiExt(DmtxMatrixRegion *matrixRegion, unsigned char *ptr, unsigned char *dataEnd);
static unsigned char *DecodeSchemeC40Text(DmtxMatrixRegion *matrixRegion, unsigned char *ptr, unsigned char *dataEnd, DmtxEncScheme encScheme);
static unsigned char *DecodeSchemeX12(DmtxMatrixRegion *matrixRegion, unsigned char *ptr, unsigned char *dataEnd);
static unsigned char *DecodeSchemeEdifact(DmtxMatrixRegion *matrixRegion, unsigned char *ptr, unsigned char *dataEnd);
static unsigned char *DecodeSchemeBase256(DmtxMatrixRegion *matrixRegion, unsigned char *ptr, unsigned char *dataEnd);

/* dmtxencode.c */
static void EncodeText(DmtxMatrixRegion *matrixRegion, unsigned char *inputText);
static unsigned char *WriteAscii2Digit(unsigned char **dest, unsigned char *src, unsigned char *srcEnd);
static unsigned char *WriteAsciiChar(unsigned char **dest, unsigned char *src);
static void AddPadChars(DmtxMatrixRegion *matrix);
static unsigned char Randomize253State(unsigned char codewordValue, int codewordPosition);
static void PatternInit(DmtxMatrixRegion *matrixRegion);
static void PrintPattern(DmtxEncode *encode);

/* dmtxplacemod.c */
static int ModulePlacementEcc200(DmtxMatrixRegion *matrixRegion);
static void PatternShapeStandard(DmtxMatrixRegion *matrixRegion, int row, int col, unsigned char *codeword);
static void PatternShapeSpecial1(DmtxMatrixRegion *matrixRegion, unsigned char *codeword);
static void PatternShapeSpecial2(DmtxMatrixRegion *matrixRegion, unsigned char *codeword);
static void PatternShapeSpecial3(DmtxMatrixRegion *matrixRegion, unsigned char *codeword);
static void PatternShapeSpecial4(DmtxMatrixRegion *matrixRegion, unsigned char *codeword);
static void PlaceModule(DmtxMatrixRegion *matrixRegion, int row, int col, unsigned char *codeword, DmtxBitMask mask);

/* dmtxgalois.c */
static int GfSum(int a, int b);
static int GfProduct(int a, int b);
static int GfDoublify(int a, int b);

/* dmtxreedsol.c */
static void GenReedSolEcc(DmtxMatrixRegion *matrixRegion);
static void SetEccPoly(unsigned char *g, int errorWordCount);

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

/* Data Matrix ECC200 valid sizes */
static int symbolSize[] = { 10, 12, 14, 16, 18, 20,  22,  24,  26,
                                        32, 36, 40,  44,  48,  52,
                                        64, 72, 80,  88,  96, 104,
                                               120, 132, 144 };

static int dataWordLength[] = { 3, 5, 8, 12,  18,  22,   30,   36,  44,
                                         62,  86, 114,  144,  174, 204,
                                        280, 368, 456,  576,  696, 816,
                                                 1050, 1304, 1558 };

static int errorWordLength[] = { 5, 7, 10, 12,  14,  18,  20,  24,  28,
                                           36,  42,  48,  56,  68,  84,
                                          112, 144, 192, 224, 272, 336,
                                                    408, 496, 620 };

static int dataRegionSize[] = {  8, 10, 12, 14, 16, 18, 20, 22, 24,
                                            14, 16, 18, 20, 22, 24,
                                            14, 16, 18, 20, 22, 24,
                                                    18, 20, 22 };
