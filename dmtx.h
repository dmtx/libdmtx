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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923
#endif

#define DMTX_VERSION                   "0.5.2"

#define DMTX_FAILURE                   0
#define DMTX_SUCCESS                   1

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
#define DMTX_REGION_DROPPED_1ST        5
#define DMTX_REGION_DROPPED_2ND        6
#define DMTX_REGION_DROPPED_RIGHT      7
#define DMTX_REGION_DROPPED_TOP        8
#define DMTX_REGION_DROPPED_SIZE       9

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

#define DMTX_SYMBOL_SQUARE_AUTO       -1
#define DMTX_SYMBOL_SQUARE_COUNT      24
#define DMTX_SYMBOL_RECT_AUTO         -2
#define DMTX_SYMBOL_RECT_COUNT         6

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
   DmtxSymAttribBlockDataWords,
   DmtxSymAttribBlockErrorWords,
   DmtxSymAttribBlockTotalWords,
   DmtxSymAttribBlockMaxCorrectable,
   DmtxSymAttribSymbolDataWords,
   DmtxSymAttribSymbolErrorWords,
   DmtxSymAttribSymbolTotalWords,
   DmtxSymAttribSymbolMaxCorrectable
} DmtxSymAttribute;

typedef enum {
   DmtxCorner00 = 0x01 << 0,
   DmtxCorner10 = 0x01 << 1,
   DmtxCorner11 = 0x01 << 2,
   DmtxCorner01 = 0x01 << 3
} DmtxCornerLoc;

typedef enum {
   DmtxPropEdgeThresh,
   DmtxPropSquareDevn,
   DmtxPropScanGap,
   DmtxPropXmin,
   DmtxPropXmax,
   DmtxPropYmin,
   DmtxPropYmax
} DmtxProperty;

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
   char        isDefined;
   double      tMin, tMax, tMid;
   DmtxRay3    ray;
   DmtxColor3 color, colorPrev; /* XXX maybe these aren't appropriate variables for a gradient? */
} DmtxGradient;

typedef enum {
   DmtxCompassDirNone  = 0x00,
   DmtxCompassDir0     = 0x01,
   DmtxCompassDir90    = 0x02,
   DmtxCompassDirBoth  = DmtxCompassDir0 | DmtxCompassDir90
} DmtxCompassDir;

/**
 * @struct DmtxCompassEdge
 * @brief DmtxCompassEdge
 */
typedef struct DmtxCompassEdge_struct {
   char            tested;
   char            edgeDir;
   double          magnitude;  /* sqrt(R^2 + G^2 + B^2) */
   DmtxColor3      intensity;
} DmtxCompassEdge;

/**
 * @struct DmtxImage
 * @brief DmtxImage
 */
typedef struct DmtxImage_struct {
   unsigned int    pageCount;
   unsigned int    width;
   unsigned int    height;
   DmtxRgb         *pxl;
   DmtxCompassEdge *compass;
} DmtxImage;

/**
 * @struct DmtxEdge
 * @brief DmtxEdge
 */
typedef struct DmtxEdge_struct {
   int        offset;
   double     t;
   DmtxColor3 color;
} DmtxEdge;

/**
 * @struct DmtxChain
 * @brief DmtxChain
 */
typedef struct DmtxChain_struct {
   double tx, ty;
   double phi, shx;
   double scx, scy;
   double bx0, bx1;
   double by0, by1;
   double sz;
} DmtxChain;

/**
 * @struct DmtxCorners
 * @brief DmtxCorners
 */
typedef struct DmtxCorners_struct {
   DmtxCornerLoc known; /* combination of (DmtxCorner00 | DmtxCorner10 | DmtxCorner11 | DmtxCorner01) */
   DmtxVector2 c00;
   DmtxVector2 c10;
   DmtxVector2 c11;
   DmtxVector2 c01;
} DmtxCorners;

/**
 * @struct DmtxRegion
 * @brief DmtxRegion
 */
typedef struct DmtxRegion_struct {
   int             found;         /* DMTX_REGION_FOUND | DMTX_REGION_NOT_FOUND | DMTX_REGION_EOF */
   DmtxGradient    gradient;      /* Linear blend of colors between background and symbol color */
   DmtxChain       chain;         /* List of values that are used to build a transformation matrix */
   DmtxCorners     corners;       /* Corners of barcode region */
   DmtxMatrix3     raw2fit;       /* 3x3 transformation from raw image to fitted barcode grid */
   DmtxMatrix3     fit2raw;       /* 3x3 transformation from fitted barcode grid to raw image */
   int             sizeIdx;       /* Index of arrays that store Data Matrix constants */
   int             symbolRows;    /* Number of total rows in symbol including alignment patterns */
   int             symbolCols;    /* Number of total columns in symbol including alignment patterns */
   int             mappingRows;   /* Number of data rows in symbol */
   int             mappingCols;   /* Number of data columns in symbol */
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
   double          squareDevn;
   int             scanGap;
   int             xMin;
   int             xMax;
   int             yMin;
   int             yMax;
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
   DmtxMatrix3 xfrm; /* XXX still necessary? */
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
extern DmtxEncode dmtxEncodeStructInit(void);
extern void dmtxEncodeStructDeInit(DmtxEncode *enc);
extern int dmtxEncodeDataMatrix(DmtxEncode *enc, int n, unsigned char *s, int sizeIdxRequest);
extern int dmtxEncodeDataMosaic(DmtxEncode *enc, int n, unsigned char *s, int sizeIdxRequest);

/* dmtxdecode.c */
extern DmtxDecode dmtxDecodeStructInit(DmtxImage *img);
extern void dmtxDecodeStructDeInit(DmtxDecode *dec);
extern int dmtxDecodeSetProp(DmtxDecode *dec, DmtxProperty prop, int value);
extern DmtxMessage *dmtxDecodeMatrixRegion(DmtxDecode *dec, DmtxRegion *reg, int fix);
extern DmtxMessage *dmtxDecodeMosaicRegion(DmtxDecode *dec, DmtxRegion *reg, int fix);
extern DmtxMessage *dmtxMessageMalloc(int sizeIdx, int symbolFormat);
extern void dmtxMessageFree(DmtxMessage **mesg);

/* dmtxregion.c */
extern DmtxRegion dmtxDecodeFindNextRegion(DmtxDecode *decode, DmtxTime *timeout);
extern DmtxRegion dmtxScanPixel(DmtxDecode *decode, DmtxPixelLoc loc);

/* dmtximage.c */
extern DmtxImage *dmtxImageMalloc(int width, int height);
extern int dmtxImageFree(DmtxImage **img);
extern int dmtxImageGetWidth(DmtxImage *img);
extern int dmtxImageGetHeight(DmtxImage *img);
extern int dmtxImageGetOffset(DmtxImage *img, DmtxDirection dir, int lineNbr, int offset);

extern DmtxVector2 *dmtxVector2AddTo(DmtxVector2 *v1, DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2Add(DmtxVector2 *vOut, DmtxVector2 *v1, DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2SubFrom(DmtxVector2 *v1, DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2Sub(DmtxVector2 *vOut, DmtxVector2 *v1, DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2ScaleBy(DmtxVector2 *v, double s);
extern DmtxVector2 *dmtxVector2Scale(DmtxVector2 *vOut, DmtxVector2 *v, double s);
extern double dmtxVector2Cross(DmtxVector2 *v1, DmtxVector2 *v2);
extern int dmtxVector2Norm(DmtxVector2 *v);
extern double dmtxVector2Dot(DmtxVector2 *v1, DmtxVector2 *v2);
extern double dmtxVector2Mag(DmtxVector2 *v);
extern double dmtxDistanceFromRay2(DmtxRay2 *r, DmtxVector2 *q);
extern double dmtxDistanceAlongRay2(DmtxRay2 *r, DmtxVector2 *q);
extern int dmtxRay2Intersect(DmtxVector2 *point, DmtxRay2 *p0, DmtxRay2 *p1);
extern int dmtxPointAlongRay2(DmtxVector2 *point, DmtxRay2 *r, double t);

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

extern void dmtxPixelFromImage(DmtxRgb rgb, DmtxImage *img, int x, int y);
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
extern int dmtxColor3Norm(DmtxColor3 *v);
extern double dmtxColor3Dot(DmtxColor3 *v1, DmtxColor3 *v2);
extern double dmtxColor3Mag(DmtxColor3 *v);
extern double dmtxDistanceFromRay3(DmtxRay3 *r, DmtxColor3 *q);
extern double dmtxDistanceAlongRay3(DmtxRay3 *r, DmtxColor3 *q);
extern int dmtxPointAlongRay3(DmtxColor3 *point, DmtxRay3 *r, double t);

extern int dmtxSymbolModuleStatus(DmtxMessage *mapping, int sizeIdx, int row, int col);
extern int dmtxGetSymbolAttribute(int attribute, int sizeIdx);

extern char *dmtxVersion(void);

#ifdef __cplusplus
}
#endif

#endif
