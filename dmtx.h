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
 * \file dmtx.h
 * \brief Main libdmtx header
 */

#ifndef __DMTX_H__
#define __DMTX_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Time headers required for DmtxTime struct below */
#include <time.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifndef M_PI
#define M_PI      3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2    1.57079632679489661923
#endif

#define DmtxVersion              "0.7.5"

#define DmtxUndefined                 -1

#define DmtxPassFail        unsigned int
#define DmtxPass                       1
#define DmtxFail                       0

#define DmtxBoolean         unsigned int
#define DmtxTrue                       1
#define DmtxFalse                      0

#define DmtxFormatMatrix               0
#define DmtxFormatMosaic               1

#define DmtxSymbolSquareCount         24
#define DmtxSymbolRectCount            6

#define DmtxModuleOff               0x00
#define DmtxModuleOnRed             0x01
#define DmtxModuleOnGreen           0x02
#define DmtxModuleOnBlue            0x04
#define DmtxModuleOnRGB             0x07  /* OnRed | OnGreen | OnBlue */
#define DmtxModuleOn                0x07
#define DmtxModuleUnsure            0x08
#define DmtxModuleAssigned          0x10
#define DmtxModuleVisited           0x20
#define DmtxModuleData              0x40

#define DMTX_CHECK_BOUNDS(l,i) (assert((i) >= 0 && (i) < (l)->length && (l)->length <= (l)->capacity))

typedef enum {
   DmtxStatusEncoding, /* Encoding is currently underway */
   DmtxStatusComplete, /* Encoding is done and everything went well */
   DmtxStatusInvalid,  /* Something bad happened that sometimes happens */
   DmtxStatusFatal     /* Something happened that should never happen */
} DmtxStatus;

typedef enum {
   DmtxSchemeAutoFast        = -2,
   DmtxSchemeAutoBest        = -1,
   DmtxSchemeAscii           =  0,
   DmtxSchemeC40,
   DmtxSchemeText,
   DmtxSchemeX12,
   DmtxSchemeEdifact,
   DmtxSchemeBase256
} DmtxScheme;

typedef enum {
   DmtxSymbolRectAuto        = -3,
   DmtxSymbolSquareAuto      = -2,
   DmtxSymbolShapeAuto       = -1,
   DmtxSymbol10x10           =  0,
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
   DmtxDirNone               = 0x00,
   DmtxDirUp                 = 0x01 << 0,
   DmtxDirLeft               = 0x01 << 1,
   DmtxDirDown               = 0x01 << 2,
   DmtxDirRight              = 0x01 << 3,
   DmtxDirHorizontal         = DmtxDirLeft  | DmtxDirRight,
   DmtxDirVertical           = DmtxDirUp    | DmtxDirDown,
   DmtxDirRightUp            = DmtxDirRight | DmtxDirUp,
   DmtxDirLeftDown           = DmtxDirLeft  | DmtxDirDown
} DmtxDirection;

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
   DmtxCorner00              = 0x01 << 0,
   DmtxCorner10              = 0x01 << 1,
   DmtxCorner11              = 0x01 << 2,
   DmtxCorner01              = 0x01 << 3
} DmtxCornerLoc;

typedef enum {
   /* Encoding properties */
   DmtxPropScheme            = 100,
   DmtxPropSizeRequest,
   DmtxPropMarginSize,
   DmtxPropModuleSize,
   DmtxPropFnc1,
   /* Decoding properties */
   DmtxPropEdgeMin           = 200,
   DmtxPropEdgeMax,
   DmtxPropScanGap,
   DmtxPropSquareDevn,
   DmtxPropSymbolSize,
   DmtxPropEdgeThresh,
   /* Image properties */
   DmtxPropWidth             = 300,
   DmtxPropHeight,
   DmtxPropPixelPacking,
   DmtxPropBitsPerPixel,
   DmtxPropBytesPerPixel,
   DmtxPropRowPadBytes,
   DmtxPropRowSizeBytes,
   DmtxPropImageFlip,
   DmtxPropChannelCount,
   /* Image modifiers */
   DmtxPropXmin              = 400,
   DmtxPropXmax,
   DmtxPropYmin,
   DmtxPropYmax,
   DmtxPropScale
} DmtxProperty;

typedef enum {
   /* Custom format */
   DmtxPackCustom            = 100,
   /* 1 bpp */
   DmtxPack1bppK             = 200,
   /* 8 bpp grayscale */
   DmtxPack8bppK             = 300,
   /* 16 bpp formats */
   DmtxPack16bppRGB          = 400,
   DmtxPack16bppRGBX,
   DmtxPack16bppXRGB,
   DmtxPack16bppBGR,
   DmtxPack16bppBGRX,
   DmtxPack16bppXBGR,
   DmtxPack16bppYCbCr,
   /* 24 bpp formats */
   DmtxPack24bppRGB          = 500,
   DmtxPack24bppBGR,
   DmtxPack24bppYCbCr,
   /* 32 bpp formats */
   DmtxPack32bppRGBX         = 600,
   DmtxPack32bppXRGB,
   DmtxPack32bppBGRX,
   DmtxPack32bppXBGR,
   DmtxPack32bppCMYK
} DmtxPackOrder;

typedef enum {
  DmtxFlipNone               = 0x00,
  DmtxFlipX                  = 0x01 << 0,
  DmtxFlipY                  = 0x01 << 1
} DmtxFlip;

typedef double DmtxMatrix3[3][3];

/**
 * @struct DmtxPixelLoc
 * @brief DmtxPixelLoc
 */
typedef struct DmtxPixelLoc_struct {
   int X;
   int Y;
} DmtxPixelLoc;

/**
 * @struct DmtxVector2
 * @brief DmtxVector2
 */
typedef struct DmtxVector2_struct {
   double          X;
   double          Y;
} DmtxVector2;

/**
 * @struct DmtxRay2
 * @brief DmtxRay2
 */
typedef struct DmtxRay2_struct {
   double          tMin;
   double          tMax;
   DmtxVector2     p;
   DmtxVector2     v;
} DmtxRay2;

typedef unsigned char DmtxByte;

/**
 * @struct DmtxByteList
 * @brief DmtxByteList
 * Use signed int for length fields instead of size_t to play nicely with RS
 * arithmetic
 */
typedef struct DmtxByteList_struct DmtxByteList;
struct DmtxByteList_struct
{
   int length;
   int capacity;
   DmtxByte *b;
};

typedef struct DmtxEncodeStream_struct DmtxEncodeStream;
struct DmtxEncodeStream_struct
{
   int currentScheme;         /* Current encodation scheme */
   int inputNext;             /* Index of next unprocessed input word in queue */
   int outputChainValueCount; /* Count of output values pushed within current scheme chain */
   int outputChainWordCount;  /* Count of output words pushed within current scheme chain */
   char *reason;              /* Reason for status */
   int sizeIdx;               /* Symbol size of completed stream */
   int fnc1;                  /* Character to represent FNC1, or DmtxUndefined */
   DmtxStatus status;
   DmtxByteList *input;
   DmtxByteList *output;
};

/**
 * @struct DmtxImage
 * @brief DmtxImage
 */
typedef struct DmtxImage_struct {
   int             width;
   int             height;
   int             pixelPacking;
   int             bitsPerPixel;
   int             bytesPerPixel;
   int             rowPadBytes;
   int             rowSizeBytes;
   int             imageFlip;
   int             channelCount;
   int             channelStart[4];
   int             bitsPerChannel[4];
   unsigned char  *pxl;
} DmtxImage;

/**
 * @struct DmtxPointFlow
 * @brief DmtxPointFlow
 */
typedef struct DmtxPointFlow_struct {
   int             plane;
   int             arrive;
   int             depart;
   int             mag;
   DmtxPixelLoc    loc;
} DmtxPointFlow;

/**
 * @struct DmtxBestLine
 * @brief DmtxBestLine
 */
typedef struct DmtxBestLine_struct {
   int             angle;
   int             hOffset;
   int             mag;
   int             stepBeg;
   int             stepPos;
   int             stepNeg;
   int             distSq;
   double          devn;
   DmtxPixelLoc    locBeg;
   DmtxPixelLoc    locPos;
   DmtxPixelLoc    locNeg;
} DmtxBestLine;

/**
 * @struct DmtxRegion
 * @brief DmtxRegion
 */
typedef struct DmtxRegion_struct {

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
   size_t          arraySize;     /* mappingRows * mappingCols */
   size_t          codeSize;      /* Size of encoded data (data words + error words) */
   size_t          outputSize;    /* Size of buffer used to hold decoded data */
   int             outputIdx;     /* Internal index used to store output progress */
   int             padCount;
   int             fnc1;          /* Character to represent FNC1, or DmtxUndefined */
   unsigned char  *array;         /* Pointer to internal representation of Data Matrix modules */
   unsigned char  *code;          /* Pointer to internal storage of code words (data and error) */
   unsigned char  *output;        /* Pointer to internal storage of decoded output */
} DmtxMessage;

/**
 * @struct DmtxScanGrid
 * @brief DmtxScanGrid
 */
typedef struct DmtxScanGrid_struct {
   /* set once */
   int             minExtent;     /* Smallest cross size used in scan */
   int             maxExtent;     /* Size of bounding grid region (2^N - 1) */
   int             xOffset;       /* Offset to obtain image X coordinate */
   int             yOffset;       /* Offset to obtain image Y coordinate */
   int             xMin;          /* Minimum X in image coordinate system */
   int             xMax;          /* Maximum X in image coordinate system */
   int             yMin;          /* Minimum Y in image coordinate system */
   int             yMax;          /* Maximum Y in image coordinate system */

   /* reset for each level */
   int             total;         /* Total number of crosses at this size */
   int             extent;        /* Length/width of cross in pixels */
   int             jumpSize;      /* Distance in pixels between cross centers */
   int             pixelTotal;    /* Total pixel count within an individual cross path */
   int             startPos;      /* X and Y coordinate of first cross center in pattern */

   /* reset for each cross */
   int             pixelCount;    /* Progress (pixel count) within current cross pattern */
   int             xCenter;       /* X center of current cross pattern */
   int             yCenter;       /* Y center of current cross pattern */
} DmtxScanGrid;

/**
 * @struct DmtxTime
 * @brief DmtxTime
 */
typedef struct DmtxTime_struct {
   time_t          sec;
   unsigned long   usec;
} DmtxTime;

/**
 * @struct DmtxDecode
 * @brief DmtxDecode
 */
typedef struct DmtxDecode_struct {
   /* Options */
   int             edgeMin;
   int             edgeMax;
   int             scanGap;
   int             fnc1;
   double          squareDevn;
   int             sizeIdxExpected;
   int             edgeThresh;

   /* Image modifiers */
   int             xMin;
   int             xMax;
   int             yMin;
   int             yMax;
   int             scale;

   /* Internals */
/* int             cacheComplete; */
   unsigned char  *cache;
   DmtxImage      *image;
   DmtxScanGrid    grid;
} DmtxDecode;

/**
 * @struct DmtxEncode
 * @brief DmtxEncode
 */
typedef struct DmtxEncode_struct {
   int             method;
   int             scheme;
   int             sizeIdxRequest;
   int             marginSize;
   int             moduleSize;
   int             pixelPacking;
   int             imageFlip;
   int             rowPadBytes;
   int             fnc1;
   DmtxMessage    *message;
   DmtxImage      *image;
   DmtxRegion      region;
   DmtxMatrix3     xfrm;  /* XXX still necessary? */
   DmtxMatrix3     rxfrm; /* XXX still necessary? */
} DmtxEncode;

/**
 * @struct DmtxChannel
 * @brief DmtxChannel
 */
typedef struct DmtxChannel_struct {
   int             encScheme;     /* current encodation scheme */
   int             invalid;       /* channel status (invalid if non-zero) */
   unsigned char  *inputPtr;      /* pointer to current input character */
   unsigned char  *inputStop;     /* pointer to position after final input character */
   int             encodedLength; /* encoded length (units of 2/3 bits) */
   int             currentLength; /* current length (units of 2/3 bits) */
   int             firstCodeWord; /* */
   unsigned char   encodedWords[1558];
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
   unsigned char   value[3];
} DmtxTriplet;

/**
 * @struct DmtxQuadruplet
 * @brief DmtxQuadruplet
 */
typedef struct DmtxQuadruplet_struct {
   unsigned char   value[4];
} DmtxQuadruplet;

/* dmtxtime.c */
extern DmtxTime dmtxTimeNow(void);
extern DmtxTime dmtxTimeAdd(DmtxTime t, long msec);
extern int dmtxTimeExceeded(DmtxTime timeout);

/* dmtxencode.c */
extern DmtxEncode *dmtxEncodeCreate(void);
extern DmtxPassFail dmtxEncodeDestroy(DmtxEncode **enc);
extern DmtxPassFail dmtxEncodeSetProp(DmtxEncode *enc, int prop, int value);
extern int dmtxEncodeGetProp(DmtxEncode *enc, int prop);
extern DmtxPassFail dmtxEncodeDataMatrix(DmtxEncode *enc, int n, unsigned char *s);
extern DmtxPassFail dmtxEncodeDataMosaic(DmtxEncode *enc, int n, unsigned char *s);

/* dmtxdecode.c */
extern DmtxDecode *dmtxDecodeCreate(DmtxImage *img, int scale);
extern DmtxPassFail dmtxDecodeDestroy(DmtxDecode **dec);
extern DmtxPassFail dmtxDecodeSetProp(DmtxDecode *dec, int prop, int value);
extern int dmtxDecodeGetProp(DmtxDecode *dec, int prop);
extern /*@exposed@*/ unsigned char *dmtxDecodeGetCache(DmtxDecode *dec, int x, int y);
extern DmtxPassFail dmtxDecodeGetPixelValue(DmtxDecode *dec, int x, int y, int channel, /*@out@*/ int *value);
extern DmtxMessage *dmtxDecodeMatrixRegion(DmtxDecode *dec, DmtxRegion *reg, int fix);
extern DmtxMessage *dmtxDecodePopulatedArray(int sizeIdx, DmtxMessage *msg, int fix);
extern DmtxMessage *dmtxDecodeMosaicRegion(DmtxDecode *dec, DmtxRegion *reg, int fix);
extern unsigned char *dmtxDecodeCreateDiagnostic(DmtxDecode *dec, /*@out@*/ int *totalBytes, /*@out@*/ int *headerBytes, int style);

/* dmtxregion.c */
extern DmtxRegion *dmtxRegionCreate(DmtxRegion *reg);
extern DmtxPassFail dmtxRegionDestroy(DmtxRegion **reg);
extern DmtxRegion *dmtxRegionFindNext(DmtxDecode *dec, DmtxTime *timeout);
extern DmtxRegion *dmtxRegionScanPixel(DmtxDecode *dec, int x, int y);
extern DmtxPassFail dmtxRegionUpdateCorners(DmtxDecode *dec, DmtxRegion *reg, DmtxVector2 p00,
      DmtxVector2 p10, DmtxVector2 p11, DmtxVector2 p01);
extern DmtxPassFail dmtxRegionUpdateXfrms(DmtxDecode *dec, DmtxRegion *reg);

/* dmtxmessage.c */
extern DmtxMessage *dmtxMessageCreate(int sizeIdx, int symbolFormat);
extern DmtxPassFail dmtxMessageDestroy(DmtxMessage **msg);

/* dmtximage.c */
extern DmtxImage *dmtxImageCreate(unsigned char *pxl, int width, int height, int pack);
extern DmtxPassFail dmtxImageDestroy(DmtxImage **img);
extern DmtxPassFail dmtxImageSetChannel(DmtxImage *img, int channelStart, int bitsPerChannel);
extern DmtxPassFail dmtxImageSetProp(DmtxImage *img, int prop, int value);
extern int dmtxImageGetProp(DmtxImage *img, int prop);
extern int dmtxImageGetByteOffset(DmtxImage *img, int x, int y);
extern DmtxPassFail dmtxImageGetPixelValue(DmtxImage *img, int x, int y, int channel, /*@out@*/ int *value);
extern DmtxPassFail dmtxImageSetPixelValue(DmtxImage *img, int x, int y, int channel, int value);
extern DmtxBoolean dmtxImageContainsInt(DmtxImage *img, int margin, int x, int y);
extern DmtxBoolean dmtxImageContainsFloat(DmtxImage *img, double x, double y);

/* dmtxvector2.c */
extern DmtxVector2 *dmtxVector2AddTo(DmtxVector2 *v1, const DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2Add(/*@out@*/ DmtxVector2 *vOut, const DmtxVector2 *v1, const DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2SubFrom(DmtxVector2 *v1, const DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2Sub(/*@out@*/ DmtxVector2 *vOut, const DmtxVector2 *v1, const DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2ScaleBy(DmtxVector2 *v, double s);
extern DmtxVector2 *dmtxVector2Scale(/*@out@*/ DmtxVector2 *vOut, const DmtxVector2 *v, double s);
extern double dmtxVector2Cross(const DmtxVector2 *v1, const DmtxVector2 *v2);
extern double dmtxVector2Norm(DmtxVector2 *v);
extern double dmtxVector2Dot(const DmtxVector2 *v1, const DmtxVector2 *v2);
extern double dmtxVector2Mag(const DmtxVector2 *v);
extern double dmtxDistanceFromRay2(const DmtxRay2 *r, const DmtxVector2 *q);
extern double dmtxDistanceAlongRay2(const DmtxRay2 *r, const DmtxVector2 *q);
extern DmtxPassFail dmtxRay2Intersect(/*@out@*/ DmtxVector2 *point, const DmtxRay2 *p0, const DmtxRay2 *p1);
extern DmtxPassFail dmtxPointAlongRay2(/*@out@*/ DmtxVector2 *point, const DmtxRay2 *r, double t);

/* dmtxmatrix3.c */
extern void dmtxMatrix3Copy(/*@out@*/ DmtxMatrix3 m0, DmtxMatrix3 m1);
extern void dmtxMatrix3Identity(/*@out@*/ DmtxMatrix3 m);
extern void dmtxMatrix3Translate(/*@out@*/ DmtxMatrix3 m, double tx, double ty);
extern void dmtxMatrix3Rotate(/*@out@*/ DmtxMatrix3 m, double angle);
extern void dmtxMatrix3Scale(/*@out@*/ DmtxMatrix3 m, double sx, double sy);
extern void dmtxMatrix3Shear(/*@out@*/ DmtxMatrix3 m, double shx, double shy);
extern void dmtxMatrix3LineSkewTop(/*@out@*/ DmtxMatrix3 m, double b0, double b1, double sz);
extern void dmtxMatrix3LineSkewTopInv(/*@out@*/ DmtxMatrix3 m, double b0, double b1, double sz);
extern void dmtxMatrix3LineSkewSide(/*@out@*/ DmtxMatrix3 m, double b0, double b1, double sz);
extern void dmtxMatrix3LineSkewSideInv(/*@out@*/ DmtxMatrix3 m, double b0, double b1, double sz);
extern void dmtxMatrix3Multiply(/*@out@*/ DmtxMatrix3 mOut, DmtxMatrix3 m0, DmtxMatrix3 m1);
extern void dmtxMatrix3MultiplyBy(DmtxMatrix3 m0, DmtxMatrix3 m1);
extern int dmtxMatrix3VMultiply(/*@out@*/ DmtxVector2 *vOut, DmtxVector2 *vIn, DmtxMatrix3 m);
extern int dmtxMatrix3VMultiplyBy(DmtxVector2 *v, DmtxMatrix3 m);
extern void dmtxMatrix3Print(DmtxMatrix3 m);

/* dmtxsymbol.c */
extern int dmtxSymbolModuleStatus(DmtxMessage *mapping, int sizeIdx, int row, int col);
extern int dmtxGetSymbolAttribute(int attribute, int sizeIdx);
extern int dmtxGetBlockDataSize(int sizeIdx, int blockIdx);
extern int getSizeIdxFromSymbolDimension(int rows, int cols);

/* dmtxbytelist.c */
extern DmtxByteList dmtxByteListBuild(DmtxByte *storage, int capacity);
extern void dmtxByteListInit(DmtxByteList *list, int length, DmtxByte value, DmtxPassFail *passFail);
extern void dmtxByteListClear(DmtxByteList *list);
extern DmtxBoolean dmtxByteListHasCapacity(DmtxByteList *list);
extern void dmtxByteListCopy(DmtxByteList *dst, const DmtxByteList *src, DmtxPassFail *passFail);
extern void dmtxByteListPush(DmtxByteList *list, DmtxByte value, DmtxPassFail *passFail);
extern DmtxByte dmtxByteListPop(DmtxByteList *list, DmtxPassFail *passFail);
extern void dmtxByteListPrint(DmtxByteList *list, char *prefix);

extern char *dmtxVersion(void);

#ifdef __cplusplus
}
#endif

#endif
