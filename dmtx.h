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

/* $Id: dmtx.h,v 1.7 2006-10-03 05:57:27 mblaughton Exp $ */

#ifndef __DMTX_H__
#define __DMTX_H__

#define DMTX_FAILURE           0
#define DMTX_SUCCESS           1

#define DMTX_DISPLAY_SQUARE    1
#define DMTX_DISPLAY_POINT     2
#define DMTX_DISPLAY_CIRCLE    3

#define DMTX_MIN_JUMP_COUNT    2
#define DMTX_MIN_STEP_RANGE    10
#define DMTX_MIN_JUMP_DISTANCE 10.0  // Minimum color difference for step region
#define DMTX_MAX_COLOR_DEVN    20.0  // Maximum deviation from color gradient
#define DMTX_FOLLOW_STEPS      5

/* IMPORTANT: The DmtxDirection enum defines values in a way that facilitates
   turning left or right (x<<1 x>>1).  Do not alter this enum unless you
   understand the full implications of doing so. */
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
   DmtxSingleScanOnly = 0x01
} DmtxOptions;

typedef float DmtxMatrix3[3][3];

typedef struct {
   DmtxMatrix3 m;
} DmtxMatrix3Struct;

typedef struct {
   double X;
   double Y;
   double Z;
} DmtxVector3;

typedef struct {
   double X;
   double Y;
} DmtxVector2;

typedef struct {
   DmtxVector3 p;
   DmtxVector3 v;
} DmtxRay3;

typedef struct {
   char        isDefined;
   float       tMin, tMax;
   DmtxVector2 p;
   DmtxVector2 v;
} DmtxRay2;

typedef struct {
   char        isDefined;
   float       tMin, tMax, tMid;
   DmtxRay3    ray;
   DmtxVector3 color, colorPrev; // XXX maybe these aren't appropriate variables for a gradient?
} DmtxGradient;

typedef struct {
   unsigned char R;
   unsigned char G;
   unsigned char B;
} DmtxPixel;

typedef struct {
   unsigned int width;
   unsigned int height;
   DmtxPixel    *pxl;
} DmtxImage;

typedef struct {
   DmtxDirection   dir;
   int             lineNbr;
   int             firstPos;
   int             lastPos;
} DmtxScanRange;

typedef struct {
   DmtxScanRange   range;
   DmtxGradient    gradient;
   int             jumpCount;
   int             anchor1;
   int             anchor2;
   int             lastJump;
} DmtxJumpRegion;

typedef struct {
   DmtxScanRange   range;
   DmtxJumpRegion  region;
} DmtxJumpScan;

typedef struct {
   int             offset;
   float           t;
   DmtxVector3     color;
} DmtxEdge;

typedef struct {
   DmtxScanRange   range;
   DmtxEdge        edge;
   DmtxEdge        edgeNext;
   float           subPixelOffset; // XXX implement it this way first, then refactor to offsetFloat (offset + subP..)
} DmtxEdgeScan;

typedef struct {
   float tx, ty;
   float phi, shx;
   float scx, scy;
   float bx0, bx1;
   float by0, by1;
   float sz;
} DmtxChain;

typedef struct {
   DmtxGradient    gradient;
   DmtxChain       chain;
   DmtxMatrix3     raw2fit;
   DmtxMatrix3     fit2raw;
   DmtxVector2     calibTopP0;
   DmtxVector2     calibTopP1;
   int             sizeIdx;
   int             dataRows;
   int             dataCols;
   int             arraySize;
   int             codeSize;
   int             dataSize;
   int             padSize;
   int             outputSize;
   int             outputIdx;
   unsigned char   *array;
   unsigned char   *code;
   unsigned char   *error;
   unsigned char   *output;
} DmtxMatrixRegion;

typedef struct {
   int             slope;
   int             turnCount;
   int             paraOffset;
   float           perpOffset;
   float           tMin, tMid, tMax;
   DmtxRay3        ray;
   DmtxRay2        line0, line1;
   DmtxDirection   dir;
} DmtxEdgeFollower;

typedef struct {
   DmtxVector2 p0, p1, p2;
} DmtxFinderBar;

typedef struct _DmtxDecode DmtxDecode;
struct _DmtxDecode {
   int option;
   int matrixCount;
   DmtxMatrixRegion matrix[16];
   DmtxImage image;
   void (* stepScanCallback)(DmtxDecode *, DmtxScanRange *, DmtxJumpScan *);
   void (* crossScanCallback)(DmtxScanRange *, DmtxGradient *, DmtxEdgeScan *);
   void (* followScanCallback)(DmtxEdgeFollower *);
   void (* finderBarCallback)(DmtxRay2 *);
   void (* buildMatrixCallback2)(DmtxFinderBar *, DmtxMatrixRegion *);
   void (* buildMatrixCallback3)(DmtxMatrix3);
   void (* buildMatrixCallback4)(DmtxMatrix3);
   void (* plotPointCallback)(DmtxVector2, int, int, int);
   void (* xfrmPlotPointCallback)(DmtxVector2, DmtxMatrix3, int, int);
   void (* finalCallback)(DmtxMatrixRegion *);
   void (* plotModuleCallback)(DmtxDecode *, DmtxMatrixRegion *, int, int, DmtxVector3);
};

typedef struct {
   int option;
   int scheme;
   int moduleSize;
   int marginSize;
   DmtxImage image;
   DmtxMatrix3 xfrm;
   DmtxMatrixRegion matrix;
} DmtxEncode;

extern DmtxVector3 *dmtxVector3AddTo(DmtxVector3 *v1, DmtxVector3 *v2);
extern DmtxVector3 *dmtxVector3Add(DmtxVector3 *vOut, DmtxVector3 *v1, DmtxVector3 *v2);
extern DmtxVector3 *dmtxVector3SubFrom(DmtxVector3 *v1, DmtxVector3 *v2);
extern DmtxVector3 *dmtxVector3Sub(DmtxVector3 *vOut, DmtxVector3 *v1, DmtxVector3 *v2);
extern DmtxVector3 *dmtxVector3ScaleBy(DmtxVector3 *v, float s);
extern DmtxVector3 *dmtxVector3Scale(DmtxVector3 *vOut, DmtxVector3 *v, float s);
extern DmtxVector3 *dmtxVector3Cross(DmtxVector3 *vOut, DmtxVector3 *v1, DmtxVector3 *v2);
extern float dmtxVector3Norm(DmtxVector3 *v);
extern float dmtxVector3Dot(DmtxVector3 *v1, DmtxVector3 *v2);
extern float dmtxVector3Mag(DmtxVector3 *v);
extern float dmtxDistanceFromRay3(DmtxRay3 *r, DmtxVector3 *q);
extern float dmtxDistanceAlongRay3(DmtxRay3 *r, DmtxVector3 *q);
extern int dmtxPointAlongRay3(DmtxVector3 *point, DmtxRay3 *r, float t);

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
extern DmtxVector2 *dmtxMatrix3VMultiplyBy(DmtxVector2 *v, DmtxMatrix3 m);
extern DmtxVector2 *dmtxMatrix3VMultiply(DmtxVector2 *vOut, DmtxVector2 *vIn, DmtxMatrix3 m);
extern void dmtxMatrix3Multiply(DmtxMatrix3 mOut, DmtxMatrix3 m0, DmtxMatrix3 m1);
extern void dmtxMatrix3MultiplyBy(DmtxMatrix3 m0, DmtxMatrix3 m1);
extern void dmtxMatrix3LineSkewTop(DmtxMatrix3 m, float b0, float b1, float sz);
extern void dmtxMatrix3LineSkewTopInv(DmtxMatrix3 m, float b0, float b1, float sz);
extern void dmtxMatrix3LineSkewSide(DmtxMatrix3 m, float b0, float b1, float sz);
extern void dmtxMatrix3LineSkewSideInv(DmtxMatrix3 m, float b0, float b1, float sz);
extern void dmtxMatrix3Print(DmtxMatrix3 m);

extern void dmtxColorFromImage(DmtxVector3 *color, DmtxImage *image, int x, int y);
extern void dmtxColorFromImage2(DmtxVector3 *color, DmtxImage *image, DmtxVector2 p);
extern void dmtxColorFromPixel(DmtxVector3 *color, DmtxPixel *pxl);
extern void dmtxPixelFromColor(DmtxPixel *pxl, DmtxVector3 *color);
extern DmtxVector3 dmtxColorAlongRay3(DmtxRay3 *ray, float dist);

extern int dmtxImageInit(DmtxImage *image);
extern int dmtxImageDeInit(DmtxImage *image);
extern int dmtxImageGetWidth(DmtxImage *image);
extern int dmtxImageGetHeight(DmtxImage *image);
extern int dmtxImageGetOffset(DmtxImage *image, DmtxDirection dir, int lineNbr, int offset);

extern int dmtxScanLine(DmtxDecode *decode, DmtxDirection dir, int lineNbr);
extern void dmtxMatrixRegionDeInit(DmtxMatrixRegion *matrixRegion);

extern DmtxDecode *dmtxDecodeStructCreate(void);
extern void dmtxDecodeStructDestroy(DmtxDecode **decode);
extern DmtxMatrixRegion *dmtxDecodeGetMatrix(DmtxDecode *decode, int index);
extern int dmtxDecodeGetMatrixCount(DmtxDecode *decode);
extern void dmtxScanStartNew(DmtxDecode *decode);

extern DmtxEncode *dmtxEncodeCreate(void);
extern void dmtxEncodeDestroy(DmtxEncode **encode);
extern int dmtxEncodeData(DmtxEncode *encode, unsigned char *inputString);

extern void dmtxSetStepScanCallback(DmtxDecode *decode, void (* func)(DmtxDecode *, DmtxScanRange *, DmtxJumpScan *));
extern void dmtxSetCrossScanCallback(DmtxDecode *decode, void (* func)(DmtxScanRange *, DmtxGradient *, DmtxEdgeScan *));
extern void dmtxSetFollowScanCallback(DmtxDecode *decode, void (* func)(DmtxEdgeFollower *));
extern void dmtxSetFinderBarCallback(DmtxDecode *decode, void (* func)(DmtxRay2 *));
extern void dmtxSetBuildMatrixCallback2(DmtxDecode *decode, void (* func)(DmtxFinderBar *, DmtxMatrixRegion *));
extern void dmtxSetBuildMatrixCallback3(DmtxDecode *decode, void (* func)(DmtxMatrix3));
extern void dmtxSetBuildMatrixCallback4(DmtxDecode *decode, void (* func)(DmtxMatrix3));
extern void dmtxSetPlotPointCallback(DmtxDecode *decode, void (* func)(DmtxVector2, int, int, int));
extern void dmtxSetXfrmPlotPointCallback(DmtxDecode *decode, void (* func)(DmtxVector2, DmtxMatrix3, int, int));
extern void dmtxSetFinalCallback(DmtxDecode *decode, void (* func)(DmtxMatrixRegion *));
extern void dmtxSetPlotModuleCallback(DmtxDecode *decode, void (* func)(DmtxDecode *, DmtxMatrixRegion *, int, int, DmtxVector3));

#endif
