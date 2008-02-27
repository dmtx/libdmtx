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

#ifndef __DMTX_H__
#define __DMTX_H__

#define DMTX_FAILURE                   0
#define DMTX_SUCCESS                   1

#define DMTX_STATUS_NOT_SCANNED        0
#define DMTX_STATUS_VALID              1
#define DMTX_STATUS_INVALID            2

#define DMTX_DISPLAY_SQUARE            1
#define DMTX_DISPLAY_POINT             2
#define DMTX_DISPLAY_CIRCLE            3

#define DMTX_REGION_EOF               -1
#define DMTX_REGION_NOT_FOUND          0
#define DMTX_REGION_FOUND              1

#define DMTX_MODULE_OFF             0x00
#define DMTX_MODULE_ON_RED          0x01
#define DMTX_MODULE_ON_GREEN        0x02
#define DMTX_MODULE_ON_BLUE         0x04
#define DMTX_MODULE_ON_RGB          0x07  /* ON_RED | ON_GREEN | ON_BLUE */
#define DMTX_MODULE_UNSURE          0x08  /* ON_RED | ON_GREEN | ON_BLUE */
#define DMTX_MODULE_ASSIGNED        0x10
#define DMTX_MODULE_VISITED         0x20
#define DMTX_MODULE_DATA            0x40

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
   DmtxSymAttribDataWordLength,
   DmtxSymAttribErrorWordLength,
   DmtxSymAttribInterleavedBlocks
} DmtxSymAttribute;

typedef enum {
   DmtxCorner00 = 0x01 << 0,
   DmtxCorner10 = 0x01 << 1,
   DmtxCorner11 = 0x01 << 2,
   DmtxCorner01 = 0x01 << 3
} DmtxCornerLoc;

typedef float DmtxMatrix3[3][3];

typedef struct DmtxMatrix3Struct_struct {
   DmtxMatrix3 m;
} DmtxMatrix3Struct;

typedef struct DmtxColor3_struct {
   double R;
   double G;
   double B;
} DmtxColor3;

typedef struct DmtxPixelLoc_struct {
   int X;
   int Y;
} DmtxPixelLoc;

typedef struct DmtxVector2_struct {
   double X;
   double Y;
} DmtxVector2;

typedef struct DmtxRay3_struct {
   DmtxColor3 p;
   DmtxColor3 c;
} DmtxRay3;

typedef struct DmtxRay2_struct {
   char        isDefined;
   float       tMin, tMax;
   DmtxVector2 p;
   DmtxVector2 v;
} DmtxRay2;

typedef struct DmtxGradient_struct {
   char        isDefined;
   float       tMin, tMax, tMid;
   DmtxRay3    ray;
   DmtxColor3 color, colorPrev; /* XXX maybe these aren't appropriate variables for a gradient? */
} DmtxGradient;

typedef struct DmtxPixel_struct {
   unsigned char R;
   unsigned char G;
   unsigned char B;
} DmtxPixel;

typedef struct DmtxImage_struct {
   unsigned int pageCount;
   unsigned int width;
   unsigned int height;
   DmtxPixel    *pxl;
} DmtxImage;

typedef struct DmtxEdge_struct {
   int             offset;
   float           t;
   DmtxColor3     color;
} DmtxEdge;

typedef struct DmtxChain_struct {
   float tx, ty;
   float phi, shx;
   float scx, scy;
   float bx0, bx1;
   float by0, by1;
   float sz;
} DmtxChain;

typedef struct DmtxCorners_struct {
   DmtxCornerLoc known; /* combination of (DmtxCorner00 | DmtxCorner10 | DmtxCorner11 | DmtxCorner01) */
   DmtxVector2 c00;
   DmtxVector2 c10;
   DmtxVector2 c11;
   DmtxVector2 c01;
} DmtxCorners;

typedef struct DmtxRegion_struct {
   int             valid;         /* XXX temporary variable */
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
   int             arraySize;     /* mappingRows * mappingCols */
   int             codeSize;      /* Size of encoded data (data words + error words) */
   int             outputSize;    /* Size of buffer used to hold decoded data */
   int             outputIdx;     /* Internal index used to store output progress */
   unsigned char   *array;        /* Pointer to internal representation of scanned Data Matrix modules */
   unsigned char   *code;         /* Pointer to internal storage of code words (data and error) */
   unsigned char   *output;       /* Pointer to internal storage of decoded output */
} DmtxRegion;

typedef struct DmtxEdgeFollower_struct {
   int             slope;
   int             turnCount;
   int             paraOffset;
   float           perpOffset;
   float           tMin, tMid, tMax;
   DmtxRay3        ray;
   DmtxRay2        line0, line1;
   DmtxDirection   dir;
} DmtxEdgeFollower;

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

typedef struct DmtxDecode_struct DmtxDecode;
struct DmtxDecode_struct {
   DmtxImage *image;
   DmtxScanGrid grid;

   int mosaic;
   DmtxRegion region; /* XXX delete this from struct */
   void (* buildMatrixCallback2)(DmtxRegion *);
   void (* buildMatrixCallback3)(DmtxMatrix3);
   void (* buildMatrixCallback4)(DmtxMatrix3);
   void (* plotPointCallback)(DmtxVector2, int, int, int);
   void (* xfrmPlotPointCallback)(DmtxVector2, DmtxMatrix3, int, int);
   void (* finalCallback)(DmtxDecode *, DmtxRegion *);
   void (* plotModuleCallback)(DmtxDecode *, DmtxRegion *, int, int, DmtxColor3);
};

typedef struct DmtxEncode_struct {
   int moduleSize;
   int marginSize;
   DmtxEncodeMethod method;
   DmtxSchemeEncode scheme;
   DmtxImage *image;
   DmtxMatrix3 xfrm;
   DmtxMatrix3 rxfrm;
   DmtxRegion region;
} DmtxEncode;

typedef struct DmtxChannel_struct {
   DmtxSchemeEncode  encScheme;          /* current encodation scheme */
   int               invalid;            /* channel status (invalid if non-zero) */
   unsigned char     *inputPtr;          /* pointer to current input character */
   unsigned char     *inputStop;         /* pointer to position after final input character */
   int               encodedLength;      /* encoded length (units of 2/3 bits) */
   int               currentLength;      /* current length (units of 2/3 bits) */
   int               schemeStart;        /* currentLength value before writing 1st encoded word in current scheme */
   unsigned char     encodedWords[1558]; /* array of encoded codewords */
} DmtxChannel;

/* Wrap in a struct for fast copies */
typedef struct DmtxChannelGroup_struct {
   DmtxChannel channel[6];
} DmtxChannelGroup;

typedef struct DmtxTriplet_struct {
   unsigned char value[3];
} DmtxTriplet;

typedef struct DmtxQuadruplet_struct {
   unsigned char value[4];
} DmtxQuadruplet;

extern DmtxVector2 *dmtxVector2AddTo(DmtxVector2 *v1, DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2Add(DmtxVector2 *vOut, DmtxVector2 *v1, DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2SubFrom(DmtxVector2 *v1, DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2Sub(DmtxVector2 *vOut, DmtxVector2 *v1, DmtxVector2 *v2);
extern DmtxVector2 *dmtxVector2ScaleBy(DmtxVector2 *v, float s);
extern DmtxVector2 *dmtxVector2Scale(DmtxVector2 *vOut, DmtxVector2 *v, float s);
extern float dmtxVector2Cross(DmtxVector2 *v1, DmtxVector2 *v2);
extern float dmtxVector2Norm(DmtxVector2 *v);
extern float dmtxVector2Dot(DmtxVector2 *v1, DmtxVector2 *v2);
extern float dmtxVector2Mag(DmtxVector2 *v);
extern float dmtxDistanceFromRay2(DmtxRay2 *r, DmtxVector2 *q);
extern float dmtxDistanceAlongRay2(DmtxRay2 *r, DmtxVector2 *q);
extern int dmtxRay2Intersect(DmtxVector2 *point, DmtxRay2 *p0, DmtxRay2 *p1);
extern int dmtxPointAlongRay2(DmtxVector2 *point, DmtxRay2 *r, float t);

extern void dmtxMatrix3Copy(DmtxMatrix3 m0, DmtxMatrix3 m1);
extern void dmtxMatrix3Identity(DmtxMatrix3 m);
extern void dmtxMatrix3Translate(DmtxMatrix3 m, float tx, float ty);
extern void dmtxMatrix3Rotate(DmtxMatrix3 m, double angle);
extern void dmtxMatrix3Scale(DmtxMatrix3 m, float sx, float sy);
extern void dmtxMatrix3Shear(DmtxMatrix3 m, float shx, float shy);
extern int dmtxMatrix3VMultiplyBy(DmtxVector2 *v, DmtxMatrix3 m);
extern int dmtxMatrix3VMultiply(DmtxVector2 *vOut, DmtxVector2 *vIn, DmtxMatrix3 m);
extern void dmtxMatrix3Multiply(DmtxMatrix3 mOut, DmtxMatrix3 m0, DmtxMatrix3 m1);
extern void dmtxMatrix3MultiplyBy(DmtxMatrix3 m0, DmtxMatrix3 m1);
extern void dmtxMatrix3LineSkewTop(DmtxMatrix3 m, float b0, float b1, float sz);
extern void dmtxMatrix3LineSkewTopInv(DmtxMatrix3 m, float b0, float b1, float sz);
extern void dmtxMatrix3LineSkewSide(DmtxMatrix3 m, float b0, float b1, float sz);
extern void dmtxMatrix3LineSkewSideInv(DmtxMatrix3 m, float b0, float b1, float sz);
extern void dmtxMatrix3Print(DmtxMatrix3 m);

extern DmtxPixel dmtxPixelFromImage(DmtxImage *image, int x, int y);
extern void dmtxColor3FromImage2(DmtxColor3 *color, DmtxImage *image, DmtxVector2 p);
extern DmtxColor3 *dmtxColor3FromPixel(DmtxColor3 *color, DmtxPixel *pxl);
extern void dmtxPixelFromColor3(DmtxPixel *pxl, DmtxColor3 *color);
extern DmtxColor3 dmtxColor3AlongRay3(DmtxRay3 *ray, float dist);
extern DmtxColor3 *dmtxColor3AddTo(DmtxColor3 *v1, DmtxColor3 *v2);
extern DmtxColor3 *dmtxColor3Add(DmtxColor3 *vOut, DmtxColor3 *v1, DmtxColor3 *v2);
extern DmtxColor3 *dmtxColor3SubFrom(DmtxColor3 *v1, DmtxColor3 *v2);
extern DmtxColor3 *dmtxColor3Sub(DmtxColor3 *vOut, DmtxColor3 *v1, DmtxColor3 *v2);
extern DmtxColor3 *dmtxColor3ScaleBy(DmtxColor3 *v, float s);
extern DmtxColor3 *dmtxColor3Scale(DmtxColor3 *vOut, DmtxColor3 *v, float s);
extern DmtxColor3 *dmtxColor3Cross(DmtxColor3 *vOut, DmtxColor3 *v1, DmtxColor3 *v2);
extern float dmtxColor3Norm(DmtxColor3 *v);
extern float dmtxColor3Dot(DmtxColor3 *v1, DmtxColor3 *v2);
extern float dmtxColor3Mag(DmtxColor3 *v);
extern float dmtxDistanceFromRay3(DmtxRay3 *r, DmtxColor3 *q);
extern float dmtxDistanceAlongRay3(DmtxRay3 *r, DmtxColor3 *q);
extern int dmtxPointAlongRay3(DmtxColor3 *point, DmtxRay3 *r, float t);

extern int dmtxImageInit(DmtxImage *image);
extern int dmtxImageDeInit(DmtxImage **image);
extern int dmtxImageGetWidth(DmtxImage *image);
extern int dmtxImageGetHeight(DmtxImage *image);
extern int dmtxImageGetOffset(DmtxImage *image, DmtxDirection dir, int lineNbr, int offset);

extern int dmtxFindNextRegion(DmtxDecode *decode);
extern int dmtxScanPixel(DmtxDecode *decode, DmtxPixelLoc loc);
extern void dmtxMatrixRegionDeInit(DmtxRegion *region);

extern DmtxDecode dmtxDecodeInit(DmtxImage *image, DmtxPixelLoc p0, DmtxPixelLoc p1, int minGapSize);

extern DmtxEncode *dmtxEncodeStructCreate(void);
extern void dmtxEncodeStructDestroy(DmtxEncode **encode);
extern int dmtxEncodeDataMatrix(DmtxEncode *encode, int inputSize, unsigned char *inputString, int sizeIdxRequest);
extern int dmtxEncodeDataMosaic(DmtxEncode *encode, int inputSize, unsigned char *inputString, int sizeIdxRequest);

extern void dmtxSetBuildMatrixCallback2(DmtxDecode *decode, void (* func)(DmtxRegion *));
extern void dmtxSetBuildMatrixCallback3(DmtxDecode *decode, void (* func)(DmtxMatrix3));
extern void dmtxSetBuildMatrixCallback4(DmtxDecode *decode, void (* func)(DmtxMatrix3));
extern void dmtxSetPlotPointCallback(DmtxDecode *decode, void (* func)(DmtxVector2, int, int, int));
extern void dmtxSetXfrmPlotPointCallback(DmtxDecode *decode, void (* func)(DmtxVector2, DmtxMatrix3, int, int));
extern void dmtxSetFinalCallback(DmtxDecode *decode, void (* func)(DmtxDecode *, DmtxRegion *));
extern void dmtxSetPlotModuleCallback(DmtxDecode *decode, void (* func)(DmtxDecode *, DmtxRegion *, int, int, DmtxColor3));

extern int dmtxSymbolModuleStatus(DmtxRegion *region, int row, int col);
extern int dmtxGetSymbolAttribute(int attribute, int sizeIdx);

#endif
