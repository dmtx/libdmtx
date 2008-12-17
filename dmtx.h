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
 * @file dmtx.h
 * @brief Main libdmtx header
 */

#ifndef __DMTX_H__
#define __DMTX_H__

#include <time.h>

#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923
#endif

#define DMTX_VERSION                   "0.6.1"

#define DMTX_BAD_OFFSET               -1

#define DMTX_STATUS_NOT_SCANNED        0
#define DMTX_STATUS_VALID              1
#define DMTX_STATUS_INVALID            2

#define DMTX_DISPLAY_SQUARE            1
#define DMTX_DISPLAY_POINT             2
#define DMTX_DISPLAY_CIRCLE            3

#define DMTX_REGION_FOUND              0
#define DMTX_REGION_NOT_FOUND          1
#define DMTX_REGION_TIMEOUT            2
#define DMTX_REGION_EOF                3
#define DMTX_REGION_DROPPED_EDGE       4
#define DMTX_REGION_DROPPED_FINDER     5
#define DMTX_REGION_DROPPED_TOP        6
#define DMTX_REGION_DROPPED_RIGHT      7
#define DMTX_REGION_DROPPED_SIZE       8

#define DMTX_MODULE_OFF             0x00
#define DMTX_MODULE_ON_RED          0x01
#define DMTX_MODULE_ON_GREEN        0x02
#define DMTX_MODULE_ON_BLUE         0x04
#define DMTX_MODULE_ON_RGB          0x07  /* ON_RED | ON_GREEN | ON_BLUE */
#define DMTX_MODULE_ON              0x07
#define DMTX_MODULE_UNSURE          0x08
#define DMTX_MODULE_ASSIGNED        0x10
#define DMTX_MODULE_VISITED         0x20
#define DMTX_MODULE_DATA            0x40

#define DMTX_FORMAT_MATRIX             0
#define DMTX_FORMAT_MOSAIC             1

#define DMTX_SYMBOL_SQUARE_COUNT      24
#define DMTX_SYMBOL_RECT_COUNT         6

#define DmtxPassFail unsigned int
#define DmtxPass     1
#define DmtxFail     0

#define DmtxBoolean  unsigned int
#define DmtxTrue     1
#define DmtxFalse    0

typedef enum {
   DmtxDirNone       = 0x00,
   DmtxDirUp         = 0x01 << 0,
   DmtxDirLeft       = 0x01 << 1,
   DmtxDirDown       = 0x01 << 2,
   DmtxDirRight      = 0x01 << 3,
   DmtxDirHorizontal = DmtxDirLeft  | DmtxDirRight,
   DmtxDirVertical   = DmtxDirUp    | DmtxDirDown,
   DmtxDirRightUp    = DmtxDirRight | DmtxDirUp,
   DmtxDirLeftDown   = DmtxDirLeft  | DmtxDirDown
} DmtxDirection;

typedef enum {
   DmtxEncodeAutoBest,
   DmtxEncodeAutoFast,
   DmtxEncodeSingleScheme
} DmtxEncodeMethod;

typedef enum {
   DmtxSchemeEncodeAscii,
   DmtxSchemeEncodeC40,
   DmtxSchemeEncodeText,
   DmtxSchemeEncodeX12,
   DmtxSchemeEncodeEdifact,
   DmtxSchemeEncodeBase256,
   DmtxSchemeEncodeAutoBest,
   DmtxSchemeEncodeAutoFast
} DmtxSchemeEncode;

typedef enum {
   DmtxSchemeDecodeAsciiStd,
   DmtxSchemeDecodeAsciiExt,
   DmtxSchemeDecodeC40,
   DmtxSchemeDecodeText,
   DmtxSchemeDecodeX12,
   DmtxSchemeDecodeEdifact,
   DmtxSchemeDecodeBase256
} DmtxSchemeDecode;

typedef enum {
   DmtxSymAttribSymbolRows,
   DmtxSymAttribSymbolCols,
   DmtxSymAttribDataRegionRows,
   DmtxSymAttribDataRegionCols,
   DmtxSymAttribHorizDataRegions,
   DmtxSymAttribVertDataRegions,
   DmtxSymAttribMappingMatrixRows,
   DmtxSymAttribMappingMatrixCols,
   DmtxSymAttribInterleavedBlocks,
   DmtxSymAttribBlockErrorWords,
   DmtxSymAttribBlockMaxCorrectable,
   DmtxSymAttribSymbolDataWords,
   DmtxSymAttribSymbolErrorWords,
   DmtxSymAttribSymbolMaxCorrectable
} DmtxSymAttribute;

typedef enum {
   DmtxCorner00 = 0x01 << 0,
   DmtxCorner10 = 0x01 << 1,
   DmtxCorner11 = 0x01 << 2,
   DmtxCorner01 = 0x01 << 3
} DmtxCornerLoc;

typedef enum {
   DmtxPropEdgeMin,
   DmtxPropEdgeMax,
   DmtxPropScanGap,
   DmtxPropSquareDevn,
   DmtxPropSymbolSize,
   DmtxPropEdgeThresh,
   DmtxPropWidth,
   DmtxPropHeight,
   DmtxPropArea,
   DmtxPropBitsPerPixel,
   DmtxPropBytesPerPixel,
   DmtxPropXmin,
   DmtxPropXmax,
   DmtxPropYmin,
   DmtxPropYmax,
   DmtxPropShrinkMin,
   DmtxPropShrinkMax,
   DmtxPropScale,
   DmtxPropScaledWidth,
   DmtxPropScaledHeight,
   DmtxPropScaledArea,
   DmtxPropScaledXmin,
   DmtxPropScaledXmax,
   DmtxPropScaledYmin,
   DmtxPropScaledYmax
} DmtxDecodeProperty;

typedef enum {
   DmtxSymbolRectAuto   = -3,
   DmtxSymbolSquareAuto = -2,
   DmtxSymbolShapeAuto  = -1,
   DmtxSymbol10x10      =  0,
   DmtxSymbol12x12,
   DmtxSymbol14x14,
   DmtxSymbol16x16,
   DmtxSymbol18x18,
   DmtxSymbol20x20,
   DmtxSymbol22x22,
   DmtxSymbol24x24,
   DmtxSymbol26x26,
   DmtxSymbol32x32,
   DmtxSymbol36x36,
   DmtxSymbol40x40,
   DmtxSymbol44x44,
   DmtxSymbol48x48,
   DmtxSymbol52x52,
   DmtxSymbol64x64,
   DmtxSymbol72x72,
   DmtxSymbol80x80,
   DmtxSymbol88x88,
   DmtxSymbol96x96,
   DmtxSymbol104x104,
   DmtxSymbol120x120,
   DmtxSymbol132x132,
   DmtxSymbol144x144,
   DmtxSymbol8x18,
   DmtxSymbol8x32,
   DmtxSymbol12x26,
   DmtxSymbol12x36,
   DmtxSymbol16x36,
   DmtxSymbol16x48
} DmtxSymbolSize;

typedef enum {
   DmtxPackRGB,
   DmtxPackYCbCr,
   DmtxPackCMYK,
   DmtxPackRGBX,
   DmtxPackCustom
} DmtxPackingOrder;

typedef enum {
  DmtxFlipNone = 0x00,
  DmtxFlipX    = 0x01,
  DmtxFlipY    = 0x02
} DmtxFlip;

typedef double DmtxMatrix3[3][3];
typedef unsigned char DmtxRgb[3];

/**
 * @struct DmtxColor3
 * @brief DmtxColor3
 */
typedef struct DmtxColor3_struct {
   double R;
   double G;
   double B;
} DmtxColor3;

/**
 * @struct DmtxPixelLoc
 * @brief DmtxPixelLoc
 */
typedef struct DmtxPixelLoc_struct {
   int X;
   int Y;
   int status;
} DmtxPixelLoc;

/**
 * @struct DmtxVector2
 * @brief DmtxVector2
 */
typedef struct DmtxVector2_struct {
   double X;
   double Y;
} DmtxVector2;

/**
 * @struct DmtxRay3
 * @brief DmtxRay3
 */
typedef struct DmtxRay3_struct {
   DmtxColor3 p;
   DmtxColor3 c;
} DmtxRay3;

/**
 * @struct DmtxRay2
 * @brief DmtxRay2
 */
typedef struct DmtxRay2_struct {
   char        isDefined;
   double      tMin, tMax;
   DmtxVector2 p;
   DmtxVector2 v;
} DmtxRay2;

/**
 * @struct DmtxGradient
 * @brief DmtxGradient
 */
typedef struct DmtxGradient_struct {
   char       isDefined;
   double     tMin, tMax;
   DmtxRay3   ray;
} DmtxGradient;

/**
 * @struct DmtxImage
 * @brief DmtxImage
 */
typedef struct DmtxImage_struct {
   int             width;  /* unscaled */
   int             height; /* unscaled */
   int             bpp;
   int             pack;
   int             flip;
   char            mallocByDmtx;
   int             xMin;   /* unscaled */
   int             xMax;   /* unscaled */
   int             yMin;   /* unscaled */
   int             yMax;   /* unscaled */
   int             scale;
   int             widthScaled;
   int             heightScaled;
   int             xMinScaled;
   int             xMaxScaled;
   int             yMinScaled;
   int             yMaxScaled;
   int             channelCount;
   int             channelStart[4];
   int             bitsPerChannel[4];
   unsigned char   *cache;
/* DmtxRgb         *pxl; */
   unsigned char   *pxl;
} DmtxImage;

/**
 * @struct DmtxPointFlow
 * @brief DmtxPointFlow
 */
typedef struct DmtxPointFlow_struct {
   int plane;
   int arrive;
   int depart;
   int mag;
   DmtxPixelLoc loc;
} DmtxPointFlow;

/**
 * @struct DmtxBestLine
 * @brief DmtxBestLine
 */
typedef struct DmtxBestLine_struct {
   int angle;
   int hOffset;
   int mag;
   int stepBeg;
   int stepPos;
   int stepNeg;
   int distSq;
   double devn;
   DmtxPixelLoc locBeg;
   DmtxPixelLoc locPos;
   DmtxPixelLoc locNeg;
} DmtxBestLine;

/**
 * @struct DmtxRegion
 * @brief DmtxRegion
 */
typedef struct DmtxRegion_struct {
   int             found;         /* DMTX_REGION_FOUND | DMTX_REGION_NOT_FOUND | DMTX_REGION_EOF */

   /* Trail blazing values */
   int             jumpToPos;     /* */
   int             jumpToNeg;     /* */
   int             stepsTotal;    /* */
   DmtxPixelLoc    finalPos;      /* */
   DmtxPixelLoc    finalNeg;      /* */
   DmtxPixelLoc    boundMin;      /* */
   DmtxPixelLoc    boundMax;      /* */
   DmtxPointFlow   flowBegin;     /* */

   /* Orientation values */
   int             polarity;      /* */
   int             stepR;
   int             stepT;
   DmtxPixelLoc    locR;          /* remove if stepR works above */
   DmtxPixelLoc    locT;          /* remove if stepT works above */

   /* Region fitting values */
   int             leftKnown;     /* known == 1; unknown == 0 */
   int             leftAngle;     /* hough angle of left edge */
   DmtxPixelLoc    leftLoc;       /* known (arbitrary) location on left edge */
   DmtxBestLine    leftLine;      /* */
   int             bottomKnown;   /* known == 1; unknown == 0 */
   int             bottomAngle;   /* hough angle of bottom edge */
   DmtxPixelLoc    bottomLoc;     /* known (arbitrary) location on bottom edge */
   DmtxBestLine    bottomLine;    /* */
   int             topKnown;      /* known == 1; unknown == 0 */
   int             topAngle;      /* hough angle of top edge */
   DmtxPixelLoc    topLoc;        /* known (arbitrary) location on top edge */
   int             rightKnown;    /* known == 1; unknown == 0 */
   int             rightAngle;    /* hough angle of right edge */
   DmtxPixelLoc    rightLoc;      /* known (arbitrary) location on right edge */

   /* Region calibration values */
   int             onColor;       /* */
   int             offColor;      /* */
   int             sizeIdx;       /* Index of arrays that store Data Matrix constants */
   int             symbolRows;    /* Number of total rows in symbol including alignment patterns */
   int             symbolCols;    /* Number of total columns in symbol including alignment patterns */
   int             mappingRows;   /* Number of data rows in symbol */
   int             mappingCols;   /* Number of data columns in symbol */

   /* Transform values */
   DmtxMatrix3     raw2fit;       /* 3x3 transformation from raw image to fitted barcode grid */
   DmtxMatrix3     fit2raw;       /* 3x3 transformation from fitted barcode grid to raw image */
} DmtxRegion;

/**
 * @struct DmtxMessage
 * @brief DmtxMessage
 */
typedef struct DmtxMessage_struct {
   int             arraySize;     /* mappingRows * mappingCols */
   int             codeSize;      /* Size of encoded data (data words + error words) */
   int             outputSize;    /* Size of buffer used to hold decoded data */
   int             outputIdx;     /* Internal index used to store output progress */
   unsigned char   *array;        /* Pointer to internal representation of scanned Data Matrix modules */
   unsigned char   *code;         /* Pointer to internal storage of code words (data and error) */
   unsigned char   *output;       /* Pointer to internal storage of decoded output */
} DmtxMessage;

/**
 * @struct DmtxScanGrid
 * @brief DmtxScanGrid
 */
typedef struct DmtxScanGrid_struct {
   /* set once */
   int minExtent;  /* Smallest cross size used in scan */
   int maxExtent;  /* Size of bounding grid region (2^N - 1) */
   int xOffset;    /* Offset to obtain image X coordinate */
   int yOffset;    /* Offset to obtain image Y coordinate */
   int xMin;       /* Minimum X in image coordinate system */
   int xMax;       /* Maximum X in image coordinate system */
   int yMin;       /* Minimum Y in image coordinate system */
   int yMax;       /* Maximum Y in image coordinate system */

   /* reset for each level */
   int total;      /* Total number of crosses at this size */
   int extent;     /* Length/width of cross in pixels */
   int jumpSize;   /* Distance in pixels between cross centers */
   int pixelTotal; /* Total pixel count within an individual cross path */
   int startPos;   /* X and Y coordinate of first cross center in pattern */

   /* reset for each cross */
   int pixelCount; /* Progress (pixel count) within current cross pattern */
   int xCenter;    /* X center of current cross pattern */
   int yCenter;    /* Y center of current cross pattern */
} DmtxScanGrid;

/**
 * @struct DmtxDecode
 * @brief DmtxDecode
 */
typedef struct DmtxDecode_struct {
   DmtxImage       *image;
   DmtxScanGrid    grid;
   int             edgeMin;
   int             edgeMax;
   int             scanGap;
   double          squareDevn;
   int             sizeIdxExpected;
   int             edgeThresh;
   int             shrinkMin;
   int             shrinkMax;
} DmtxDecode;

/**
 * @struct DmtxTime
 * @brief DmtxTime
 */
typedef struct DmtxTime_struct {
   time_t sec;
   unsigned long usec;
} DmtxTime;

/**
 * @struct DmtxEncode
 * @brief DmtxEncode
 */
typedef struct DmtxEncode_struct {
   int moduleSize;
   int marginSize;
   DmtxEncodeMethod method;
   DmtxSchemeEncode scheme;
   DmtxMessage *message;
   DmtxImage *image;
   DmtxRegion region;
   DmtxMatrix3 xfrm;  /* XXX still necessary? */
   DmtxMatrix3 rxfrm; /* XXX still necessary? */
} DmtxEncode;

/**
 * @struct DmtxChannel
 * @brief DmtxChannel
 */
typedef struct DmtxChannel_struct {
   DmtxSchemeEncode  encScheme;          /* current encodation scheme */
   int               invalid;            /* channel status (invalid if non-zero) */
   unsigned char     *inputPtr;          /* pointer to current input character */
   unsigned char     *inputStop;         /* pointer to position after final input character */
   int               encodedLength;      /* encoded length (units of 2/3 bits) */
   int               currentLength;      /* current length (units of 2/3 bits) */
   int               firstCodeWord;      /* */
   unsigned char     encodedWords[1558]; /* array of encoded codewords */
} DmtxChannel;

/* Wrap in a struct for fast copies */
/**
 * @struct DmtxChannelGroup
 * @brief DmtxChannelGroup
 */
typedef struct DmtxChannelGroup_struct {
   DmtxChannel channel[6];
} DmtxChannelGroup;

/**
 * @struct DmtxTriplet
 * @brief DmtxTriplet
 */
typedef struct DmtxTriplet_struct {
   unsigned char value[3];
} DmtxTriplet;

/**
 * @struct DmtxQuadruplet
 * @brief DmtxQuadruplet
 */
typedef struct DmtxQuadruplet_struct {
   unsigned char value[4];
} DmtxQuadruplet;

/* dmtxtime.c */
extern DmtxTime dmtxTimeNow(void);
extern DmtxTime dmtxTimeAdd(DmtxTime t, long msec);
extern int dmtxTimeExceeded(DmtxTime timeout);

/* dmtxencode.c */
extern DmtxEncode *dmtxEncodeStructCreate(void);
extern DmtxPassFail dmtxEncodeStructDestroy(DmtxEncode **enc);
extern DmtxPassFail dmtxEncodeDataMatrix(DmtxEncode *enc, int n, unsigned char *s, int sizeIdxRequest, int flip);
extern DmtxPassFail dmtxEncodeDataMosaic(DmtxEncode *enc, int n, unsigned char *s, int sizeIdxRequest, int flip);

/* dmtxdecode.c */
extern DmtxDecode *dmtxDecodeStructCreate(DmtxImage *img);
extern DmtxPassFail dmtxDecodeStructDestroy(DmtxDecode **dec);
extern DmtxPassFail dmtxDecodeSetProp(DmtxDecode *dec, int prop, int value);
extern DmtxMessage *dmtxDecodeMatrixRegion(DmtxImage *img, DmtxRegion *reg, int fix);
extern DmtxMessage *dmtxDecodeMosaicRegion(DmtxImage *img, DmtxRegion *reg, int fix);

/* dmtxmessage.c */
extern DmtxMessage *dmtxMessageCreate(int sizeIdx, int symbolFormat);
extern DmtxPassFail dmtxMessageDestroy(DmtxMessage **mesg);

/* dmtxregion.c */
extern DmtxRegion dmtxDecodeFindNextRegion(DmtxDecode *decode, DmtxTime *timeout);
extern DmtxRegion dmtxRegionScanPixel(DmtxDecode *decode, DmtxPixelLoc loc);
extern DmtxPassFail dmtxRegionUpdateCorners(DmtxDecode *dec, DmtxRegion *reg, DmtxVector2 p00,
      DmtxVector2 p10, DmtxVector2 p11, DmtxVector2 p01);
extern DmtxPassFail dmtxRegionUpdateXfrms(DmtxDecode *dec, DmtxRegion *reg);

/* dmtximage.c */
extern DmtxImage *dmtxImageCreate(unsigned char *pxl, int width, int height, int bpp, int packing, int flip);
extern DmtxPassFail dmtxImageDestroy(DmtxImage **img);
extern DmtxPassFail dmtxImageAddChannel(DmtxImage *img, int channelStart, int bitsPerChannel);
extern DmtxPassFail dmtxImageSetProp(DmtxImage *img, int prop, int value);
extern int dmtxImageGetProp(DmtxImage *img, int prop);
extern int dmtxImageGetPixelOffset(DmtxImage *img, int x, int y);
extern DmtxPassFail dmtxImageSetRgb(DmtxImage *img, int x, int y, DmtxRgb rgb);
extern DmtxPassFail dmtxImageGetRgb(DmtxImage *img, int x, int y, DmtxRgb rgb);
extern int dmtxImageGetColor(DmtxImage *img, int x, int y, int colorPlane);
extern DmtxBoolean dmtxImageContainsInt(DmtxImage *img, int margin, int x, int y);
extern DmtxBoolean dmtxImageContainsFloat(DmtxImage *img, double x, double y);

/* dmtxvector2.c */
extern DmtxVector2 *dmtxVector2AddTo(DmtxVector2 *v1, const DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2Add(DmtxVector2 *vOut, const DmtxVector2 *v1, const DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2SubFrom(DmtxVector2 *v1, const DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2Sub(DmtxVector2 *vOut, const DmtxVector2 *v1, const DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2ScaleBy(DmtxVector2 *v, double s);
extern DmtxVector2 *dmtxVector2Scale(DmtxVector2 *vOut, const DmtxVector2 *v, double s);
extern double dmtxVector2Cross(const DmtxVector2 *v1, const DmtxVector2 *v2);
extern double dmtxVector2Norm(DmtxVector2 *v);
extern double dmtxVector2Dot(const DmtxVector2 *v1, const DmtxVector2 *v2);
extern double dmtxVector2Mag(const DmtxVector2 *v);
extern double dmtxDistanceFromRay2(const DmtxRay2 *r, const DmtxVector2 *q);
extern double dmtxDistanceAlongRay2(const DmtxRay2 *r, const DmtxVector2 *q);
extern int dmtxRay2Intersect(DmtxVector2 *point, const DmtxRay2 *p0, const DmtxRay2 *p1);
extern DmtxPassFail dmtxPointAlongRay2(DmtxVector2 *point, const DmtxRay2 *r, double t);

/* dmtxmatrix3.c */
extern void dmtxMatrix3Copy(DmtxMatrix3 m0, DmtxMatrix3 m1);
extern void dmtxMatrix3Identity(DmtxMatrix3 m);
extern void dmtxMatrix3Translate(DmtxMatrix3 m, double tx, double ty);
extern void dmtxMatrix3Rotate(DmtxMatrix3 m, double angle);
extern void dmtxMatrix3Scale(DmtxMatrix3 m, double sx, double sy);
extern void dmtxMatrix3Shear(DmtxMatrix3 m, double shx, double shy);
extern void dmtxMatrix3LineSkewTop(DmtxMatrix3 m, double b0, double b1, double sz);
extern void dmtxMatrix3LineSkewTopInv(DmtxMatrix3 m, double b0, double b1, double sz);
extern void dmtxMatrix3LineSkewSide(DmtxMatrix3 m, double b0, double b1, double sz);
extern void dmtxMatrix3LineSkewSideInv(DmtxMatrix3 m, double b0, double b1, double sz);
extern void dmtxMatrix3Multiply(DmtxMatrix3 mOut, DmtxMatrix3 m0, DmtxMatrix3 m1);
extern void dmtxMatrix3MultiplyBy(DmtxMatrix3 m0, DmtxMatrix3 m1);
extern int dmtxMatrix3VMultiply(DmtxVector2 *vOut, DmtxVector2 *vIn, DmtxMatrix3 m);
extern int dmtxMatrix3VMultiplyBy(DmtxVector2 *v, DmtxMatrix3 m);
extern void dmtxMatrix3Print(DmtxMatrix3 m);

/* dmtxcolor3.c */
extern void dmtxColor3FromImage2(DmtxColor3 *color, DmtxImage *img, DmtxVector2 p);
extern DmtxColor3 *dmtxColor3FromPixel(DmtxColor3 *color, DmtxRgb rgb);
extern void dmtxPixelFromColor3(DmtxRgb rgb, DmtxColor3 *color);
extern DmtxColor3 dmtxColor3AlongRay3(DmtxRay3 *ray, double dist);
extern DmtxColor3 *dmtxColor3AddTo(DmtxColor3 *v1, DmtxColor3 *v2);
extern DmtxColor3 *dmtxColor3Add(DmtxColor3 *vOut, DmtxColor3 *v1, DmtxColor3 *v2);
extern DmtxColor3 *dmtxColor3SubFrom(DmtxColor3 *v1, DmtxColor3 *v2);
extern DmtxColor3 *dmtxColor3Sub(DmtxColor3 *vOut, DmtxColor3 *v1, DmtxColor3 *v2);
extern DmtxColor3 *dmtxColor3ScaleBy(DmtxColor3 *v, double s);
extern DmtxColor3 *dmtxColor3Scale(DmtxColor3 *vOut, DmtxColor3 *v, double s);
extern DmtxColor3 *dmtxColor3Cross(DmtxColor3 *vOut, DmtxColor3 *v1, DmtxColor3 *v2);
extern double dmtxColor3Norm(DmtxColor3 *v);
extern double dmtxColor3Dot(DmtxColor3 *v1, DmtxColor3 *v2);
extern double dmtxColor3MagSquared(DmtxColor3 *v);
extern double dmtxColor3Mag(DmtxColor3 *v);
extern double dmtxDistanceFromRay3(DmtxRay3 *r, DmtxColor3 *q);
extern double dmtxDistanceAlongRay3(DmtxRay3 *r, DmtxColor3 *q);
extern DmtxPassFail dmtxPointAlongRay3(DmtxColor3 *point, DmtxRay3 *r, double t);

/* dmtxsymbol.c */
extern int dmtxSymbolModuleStatus(DmtxMessage *mapping, int sizeIdx, int row, int col);
extern int dmtxGetSymbolAttribute(int attribute, int sizeIdx);
extern int dmtxGetBlockDataSize(int sizeIdx, int blockIdx);

extern char *dmtxVersion(void);

#ifdef __cplusplus
}
#endif

#endif
