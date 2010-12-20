/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2010 Mike Laughton

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

Contact: mblaughton@users.sourceforge.net
*/

/* $Id$ */

#include <SDL/SDL.h>
#include "../../dmtx.h"

#define max(N,M) ((N > M) ? N : M)
#define min(N,M) ((N < M) ? N : M)
#define OPPOSITE_SIGNS(a,b) (a != 0 && b != 0 && ((a > 0) != (b > 0)))

#define LOCAL_SIZE            64
#define MAXIMA_SORT_MAX_COUNT  8
#define ANGLE_SORT_MAX_COUNT   8
#define TIMING_SORT_MAX_COUNT  8

#define NFFT                  64 /* FFT input size */
#define HOUGH_D_EXTENT        64
#define HOUGH_PHI_EXTENT     128

/* Layout constants */
#define CTRL_COL1_X          380
#define CTRL_COL2_X          445
#define CTRL_COL3_X          510
#define CTRL_COL4_X          575
#define CTRL_ROW1_Y            0
#define CTRL_ROW2_Y           65
#define CTRL_ROW3_Y          130
#define CTRL_ROW4_Y          195
#define CTRL_ROW5_Y          260
#define CTRL_ROW6_Y          325
#define CTRL_ROW7_Y          390

#define MODULE_LOW             0
#define MODULE_HIGH            1
#define MODULE_UNKNOWN         2

#define RotateCCW(N) ((N < 8) ? (N << 1) : 1)
#define RotateCW(N)  ((N > 1) ? (N >> 1) : 8)

typedef struct UserOptions_struct {
   const char *imagePath;
} UserOptions;

typedef struct AppState_struct {
   int         windowWidth;
   int         windowHeight;
   int         activeExtent;
   DmtxImage   *imgActive;
   DmtxImage   *imgFull;
   DmtxImage   *dmtxImage;
   DmtxBoolean autoNudge;
   int         displayEdge;
   DmtxBoolean displayVanish;
   DmtxBoolean displayTiming;
   DmtxBoolean displayZXings;
   DmtxBoolean printValues;
   Sint16      imageLocX;
   Sint16      imageLocY;
   Sint16      imageOffsetX; /* X offset of right-handed image origin from screen origin */
   Sint16      imageOffsetY; /* Y offset of right-handed image origin from screen origin */
   Sint16      localOffsetX; /* X offset of active area */
   Sint16      localOffsetY; /* Y offset of active area */
   Uint8       leftButton;
   Uint8       rightButton;
   Uint16      pointerX;
   Uint16      pointerY;
   DmtxBoolean quit;
   SDL_Surface *picture;
   SDL_Surface *screen;
   SDL_Surface *local;
   SDL_Surface *localTmp;
} AppState;

typedef enum {
   DmtxEdgeVertical,
   DmtxEdgeBackslash,
   DmtxEdgeHorizontal,
   DmtxEdgeSlash
} DmtxEdgeType;

typedef struct DmtxValueGrid_struct DmtxValueGrid;
struct DmtxValueGrid_struct {
   int width;
   int height;
   int type;
   int *value;
   DmtxValueGrid *ref;
};

struct ZeroCrossing_struct {
   int type;
   int iCol;
   int iRow;
   int mag;
   double x;
   double y;
};
typedef struct ZeroCrossing_struct ZeroCrossing;

typedef struct DmtxHoughBucket_struct DmtxHoughBucket;
struct DmtxHoughBucket_struct {
   int phi;
   int d;
   int val;
};

typedef struct DmtxHoughLocal_struct DmtxHoughLocal;
struct DmtxHoughLocal_struct {
   int xOrigin;
   int yOrigin;
   int dOrigin[128];
   int bucket[64][128]; /* [rows][cols] */ /* later change to 65 */
};

typedef struct DmtxHoughGrid_struct DmtxHoughGrid;
struct DmtxHoughGrid_struct {
   int rows;
   int cols;
   int count;
   DmtxHoughLocal *line;
   DmtxHoughLocal *maxima;
   DmtxHoughLocal *vanish;
};

typedef struct HoughMaximaSort_struct {
   int count;
   int mag[MAXIMA_SORT_MAX_COUNT];
} HoughMaximaSort;

typedef struct VanishPointSort_struct {
   int count;
   DmtxHoughBucket vanishSum[ANGLE_SORT_MAX_COUNT];
} VanishPointSort;

typedef struct Timing_struct {
   int phi;
   double shift;
   double period;
   double mag;
} Timing;

struct DmtxTimingSort_struct {
   int count;
   Timing timing[TIMING_SORT_MAX_COUNT];
};
typedef struct DmtxTimingSort_struct DmtxTimingSort;

typedef struct AlignmentGrid_struct {
   int rowCount;
   int colCount;
   DmtxMatrix3 raw2fitActive;
   DmtxMatrix3 raw2fitFull;
   DmtxMatrix3 fit2rawActive;
   DmtxMatrix3 fit2rawFull;
} AlignmentGrid;

typedef struct GridRegion_struct {
   AlignmentGrid grid;
   int x;
   int y;
   int width;
   int height;
   int sizeIdx;
   int onColor;
   int offColor;
   int finderSides;
   int contrast;
} GridRegion;

typedef struct RegionLines_struct {
   int gridCount;
   Timing timing;
   double dA, dB;
   DmtxRay2 line[2];
} RegionLines;

struct DmtxRegionSides_struct {
   DmtxRay2 top;
   DmtxRay2 left;
   DmtxRay2 bottom;
   DmtxRay2 right;
};
typedef struct DmtxRegionSides_struct DmtxRegionSides;

/* All values in GridRegionGrowth should be negative because list
 * is combined with the positive values of DmtxSymbolSize enum */
typedef enum {
   GridRegionGrowthUp       = -5,
   GridRegionGrowthLeft     = -4,
   GridRegionGrowthDown     = -3,
   GridRegionGrowthRight    = -2,
   GridRegionGrowthError    = -1
} GridRegionGrowth;

typedef enum {
   DmtxBarNone     = 0x00,
   DmtxBarTiming   = 0x01 << 0,
   DmtxBarFinder   = 0x01 << 1,
   DmtxBarInterior = 0x01 << 2,
   DmtxBarExterior = 0x01 << 3
} DmtxBarType;

/* Only used internally */
typedef struct ColorTally_struct {
   int evnCount;
   int oddCount;
   int evnColor;
   int oddColor;
} ColorTally;

struct StripStats_struct {
   int jumps;
   int surprises;
   int finderErrors;
   int timingErrors;
   int contrast;
   int finderBest;
   int timingBest;
};
typedef struct StripStats_struct StripStats;

struct DmtxCallbacks_struct {
   void (*dmtxValueGridCallback)(DmtxValueGrid *, int);
   void (*zeroCrossingCallback)(ZeroCrossing, int);
   void (*dmtxHoughLocalCallback)(DmtxHoughLocal *, int);
   void (*vanishPointCallback)(VanishPointSort *, int);
   void (*timingCallback)(Timing *, Timing *, int);
   void (*gridCallback)(AlignmentGrid *, int);
   void (*perimeterCallback)(GridRegion *, DmtxDirection, DmtxBarType);
};
typedef struct DmtxCallbacks_struct DmtxCallbacks;

struct DmtxDecode2_struct {
   DmtxImage     *image;
   DmtxValueGrid *vSobel;
   DmtxValueGrid *bSobel;
   DmtxValueGrid *hSobel;
   DmtxValueGrid *sSobel;
   DmtxValueGrid *vvAccel;
   DmtxValueGrid *vbAccel;
   DmtxValueGrid *vsAccel;
   DmtxValueGrid *hbAccel;
   DmtxValueGrid *hhAccel;
   DmtxValueGrid *hsAccel;
   DmtxHoughGrid *houghGrid;
   DmtxCallbacks  fn;
};
typedef struct DmtxDecode2_struct DmtxDecode2;

/* Application level functions */
DmtxPassFail HandleArgs(UserOptions *opt, int *argcp, char **argvp[]);
AppState InitAppState(void);
DmtxImage *CreateDmtxImage(SDL_Surface *sdlImage);
void AddFullTransforms(AlignmentGrid *grid);
SDL_Surface *SetWindowSize(int windowWidth, int windowHeight);
void captureLocalPortion(SDL_Surface *local, SDL_Surface *localTmp,
      SDL_Surface *picture, SDL_Surface *screen, AppState *state, SDL_Rect imageLoc);
DmtxPassFail HandleEvent(SDL_Event *event, AppState *state,
      SDL_Surface *picture, SDL_Surface **screen);
DmtxPassFail NudgeImage(int windowExtent, int pictureExtent, Sint16 *imageLoc);
/*static void WriteDiagnosticImage(DmtxDecode *dec, char *imagePath);*/

/* Image processing functions */
DmtxPassFail dmtxRegion2FindNext(DmtxDecode2 *dec);
double UncompactOffset(double d, int phiIdx, int extent);
void AddToVanishPointSort(VanishPointSort *sort, DmtxHoughBucket vanishSum);
VanishPointSort dmtxFindVanishPoints(DmtxHoughLocal *hough);
VanishPointSort dmtxFindVanishPoints2(DmtxHoughLocal *vHough);
void AddToMaximaSort(HoughMaximaSort *sort, int maximaMag);
DmtxHoughBucket GetAngleSumAtPhi(DmtxHoughLocal *local, int phi);
void AddToTimingSort(DmtxTimingSort *sort, Timing timing);
DmtxTimingSort dmtxFindGridTiming(DmtxHoughLocal *local, VanishPointSort *sort);
DmtxRay2 HoughCompactToRay(int phi, double d);
DmtxPassFail dmtxBuildGridFromTimings(AlignmentGrid *grid, Timing vp0, Timing vp1);
StripStats GenStripPatternStats(unsigned char *strip, int stripLength, int startState, int contrast);
GridRegion NudgeStripLimits(GridRegion *region, DmtxDirection side, int nudgeStyle);

DmtxPassFail dmtxFindRegionWithinGrid(GridRegion *region, AlignmentGrid *grid, DmtxHoughLocal *local, DmtxDecode *dec, DmtxCallbacks *fn);
int dmtxReadModuleColor(DmtxImage *img, AlignmentGrid *grid, int symbolRow, int symbolCol, int colorPlane);
DmtxBarType TestSideForPattern(GridRegion *region, DmtxImage *img, DmtxDirection side, int offset);
DmtxPassFail RegionExpand(GridRegion *region, DmtxDirection dir, DmtxHoughLocal *local, DmtxCallbacks *fn);
int dmtxGetSizeIdx(int a, int b);
DmtxPassFail RegionUpdateCorners(DmtxMatrix3 fit2raw, DmtxMatrix3 raw2fit, DmtxVector2 p00, DmtxVector2 p10, DmtxVector2 p11, DmtxVector2 p01);
DmtxPassFail dmtxDecodeSymbol(GridRegion *region, DmtxDecode *dec);
DmtxPassFail GetOnOffColors(GridRegion *region, const DmtxDecode *dec, int *onColor, int *offColor);
ColorTally GetTimingColors(GridRegion *region, const DmtxDecode *dec, int colBeg, int rowBeg, DmtxDirection dir);

/* Process visualization functions */
void ValueGridCallback(DmtxValueGrid *valueGrid, int id);
void ZeroCrossingCallback(ZeroCrossing zXing, int id);
void HoughLocalCallback(DmtxHoughLocal *hough, int id);
void VanishPointCallback(VanishPointSort *vPoints, int id);
void TimingCallback(Timing *timing0, Timing *timing1, int id);
void GridCallback(AlignmentGrid *grid, int id);
void PerimeterCallback(GridRegion *region, DmtxDirection side, DmtxBarType type);

void BlitSobelGrid(SDL_Surface *screen, DmtxValueGrid *cache, int x, int y, int screenY, int screenX);
void BlitHoughLocal(SDL_Surface *screen, DmtxHoughLocal *hough, int screenY, int screenX);
void ShowActiveRegion(SDL_Surface *screen, SDL_Surface *active);
void BlitActiveRegion(SDL_Surface *screen, SDL_Surface *active, int zoom, int screenY, int screenX);
Uint32 GetPixel(SDL_Surface *surface, int x, int y);
void PutPixel(SDL_Surface *surface, int x, int y, Uint32 color);
int RayIntersect(double *t, DmtxRay2 p0, DmtxRay2 p1);
int IntersectBox(DmtxRay2 ray, DmtxVector2 bb0, DmtxVector2 bb1, DmtxVector2 *p0, DmtxVector2 *p1);
void DrawActiveBorder(SDL_Surface *screen, int activeExtent);
void DrawLine(SDL_Surface *screen, int baseExtent, int screenX, int screenY, int phi, double d, int displayScale, Uint32 color);
void DrawVanishingPoints(SDL_Surface *screen, VanishPointSort *sort, int screenY, int screenX);
void DrawTimingDots(SDL_Surface *screen, Timing *timing, int screenY, int screenX);
void DrawNormalizedRegion(SDL_Surface *screen, DmtxImage *img, AlignmentGrid *grid, AppState *state, int screenY, int screenX);
Sint16 Clamp(Sint16 x, Sint16 xMin, Sint16 extent);
void DrawSymbolPreview(SDL_Surface *screen, DmtxImage *img, AlignmentGrid *grid, AppState *state, int screenY, int screenX);
void DrawPerimeterPatterns(SDL_Surface *screen, GridRegion *region, AppState *state, DmtxDirection side, DmtxBarType type);
void DrawPerimeterSide(SDL_Surface *screen, int x00, int y00, int x11, int y11, int dispModExtent, DmtxDirection side, DmtxBarType type);

/* dmtxvaluegrid.c */
DmtxValueGrid *dmtxValueGridCreate(int width, int height, int type);
DmtxPassFail dmtxValueGridDestroy(DmtxValueGrid **valueGrid);
int dmtxValueGridGetWidth(DmtxValueGrid *valueGrid);
int dmtxValueGridGetHeight(DmtxValueGrid *valueGrid);
int dmtxValueGridGetValue(DmtxValueGrid *valueGrid, int x, int y);

DmtxPassFail SobelGridPopulate(DmtxDecode2 *dec);
DmtxPassFail AccelGridPopulate(DmtxDecode2 *dec);
DmtxValueGrid *AccelGridCreate(DmtxValueGrid *sobel, DmtxEdgeType edgeType);

/* dmtxdecode2.c */
DmtxDecode2 *dmtxDecode2Create();
DmtxPassFail dmtxDecode2Destroy(DmtxDecode2 **dec);
DmtxPassFail dmtxDecode2SetImage(DmtxDecode2 *dec, DmtxImage *img);
DmtxPassFail decode2ReleaseCacheMemory(DmtxDecode2 *dec);

/* dmtxhough.c */
DmtxPassFail HoughGridPopulate(DmtxDecode2 *dec);
DmtxPassFail HoughGridDestroy(DmtxHoughGrid **grid);
DmtxPassFail LineHoughAccumulate(DmtxHoughLocal *lhRegion, DmtxDecode2 *dec);
DmtxPassFail MaximaHoughAccumulate(DmtxHoughLocal *mhRegion, DmtxHoughLocal *lhRegion, DmtxDecode2 *dec);
void myfunc(DmtxHoughLocal *mhRegion, DmtxHoughLocal *lhRegion, ZeroCrossing *zXing);
DmtxHoughBucket GetStrongestLine(DmtxHoughLocal *lhRegion, double x, double y, DmtxEdgeType edgeType);
int GetMaximaWeight(DmtxHoughLocal *lhRegion, int phi, int d);
DmtxPassFail VanishHoughAccumulate(DmtxHoughLocal *lhRegion, DmtxHoughLocal *vhRegion);
int GetVanishBucket(int phiBucket, int phiCompare, int dCompare);
ZeroCrossing GetZeroCrossing(DmtxValueGrid *accel, int iCol, int iRow);
ZeroCrossing SetZeroCrossingFromIndex(DmtxValueGrid *accel, int aCol, int aRow, double smidg);
DmtxPassFail HoughLocalAccumulateEdge(DmtxHoughLocal *local, int phi, ZeroCrossing edge);
double HoughGetLocalOffset(double xLoc, double yLoc, int phi);

extern AppState gState;
