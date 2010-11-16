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
#define CTRL_ROW5_Y          259
#define CTRL_ROW6_Y          324
#define CTRL_ROW7_Y          388

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

struct PixelEdgeCache_struct {
   int width;
   int height;
   int *v;
   int *b;
   int *h;
   int *s;
};
typedef struct PixelEdgeCache_struct PixelEdgeCache;

typedef enum {
   SobelEdgeVertical,
   SobelEdgeBackslash,
   SobelEdgeHorizontal,
   SobelEdgeSlash
} SobelEdgeType;

struct DmtxEdgeCache_struct {
   int vDir[4096];
   int bDir[4096];
   int hDir[4096];
   int sDir[4096]; /* 64x64 */
};
typedef struct DmtxEdgeCache_struct DmtxEdgeCache;

struct DmtxHoughCache_struct {
   int offExtent;
   int phiExtent;
   char isMax[HOUGH_D_EXTENT * HOUGH_PHI_EXTENT];
   unsigned int mag[HOUGH_D_EXTENT * HOUGH_PHI_EXTENT];
};
typedef struct DmtxHoughCache_struct DmtxHoughCache;

struct DmtxHoughCache2_struct {
/* int offExtent;
   int phiExtent; */
   unsigned int mag[65 * 128];
   unsigned int dGlobalOffset[128];
/* will need floating point local offset too */
};
typedef struct DmtxHoughCache2_struct DmtxHoughCache2;

typedef struct HoughMaximaSort_struct {
   int count;
   int mag[MAXIMA_SORT_MAX_COUNT];
} HoughMaximaSort;

struct DmtxHoughCompact_struct {
   int phi;
   int d;
};
typedef struct DmtxHoughCompact_struct DmtxHoughCompact;

typedef struct VanishPointSum_struct {
   int phi;
   int mag;
} VanishPointSum;

typedef struct VanishPointSort_struct {
   int count;
   VanishPointSum vanishSum[ANGLE_SORT_MAX_COUNT];
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

typedef enum {
   DmtxSobelDirVertical,
   DmtxSobelDirHorizontal,
   DmtxSobelDirSlash,
   DmtxSobelDirBackslash
} DmtxSobelDir;

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
   void (*edgeCacheCallback)(DmtxEdgeCache *, int);
   void (*pixelEdgeCacheCallback)(PixelEdgeCache *, int);
   void (*zeroCrossingCallback)(double, double, int, int);
   void (*houghCacheCallback)(DmtxHoughCache *, int);
   void (*houghCompactCallback)(DmtxHoughCompact, int);
   void (*vanishPointCallback)(VanishPointSort *, int);
   void (*timingCallback)(Timing *, Timing *, int);
   void (*gridCallback)(AlignmentGrid *, int);
   void (*perimeterCallback)(GridRegion *, DmtxDirection, DmtxBarType);
};
typedef struct DmtxCallbacks_struct DmtxCallbacks;

struct DmtxDecode2_struct {
   DmtxImage       *image;
   PixelEdgeCache  *sobel;
   PixelEdgeCache  *accelV;
   PixelEdgeCache  *accelH;
   DmtxHoughCache2  hough; /* for now just one -- later a pointer to many */
   int              houghRows;
   int              houghCols;
   DmtxCallbacks    fn;
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
void dmtxScanImage(DmtxDecode *dec, DmtxImage *imgActive, DmtxCallbacks *fn);
DmtxPassFail dmtxRegion2FindNext(DmtxDecode2 *dec);
DmtxPassFail RegisterZeroCrossing(DmtxHoughCache *hough, DmtxDirection edgeType,
      int zCol, int zRow, double smidge, PixelEdgeCache *sobel, int s, DmtxCallbacks *fn);
DmtxPassFail FindZeroCrossings(DmtxHoughCache *hough, PixelEdgeCache *accel,
      PixelEdgeCache *sobel, DmtxDirection edgeType, DmtxCallbacks *fn);
void InitHoughCache2(DmtxHoughCache2 *hough);
DmtxPassFail dmtxBuildSobelCache(DmtxEdgeCache *edgeCache, DmtxImage *img);
int GetCompactOffset(int x, int y, int phiIdx, int extent);
double UncompactOffset(double d, int phiIdx, int extent);
DmtxPassFail dmtxBuildHoughCache(DmtxHoughCache *hough, DmtxEdgeCache *edgeCache);
DmtxPassFail dmtxNormalizeHoughCache(DmtxHoughCache *hough, DmtxEdgeCache *edgeCache);
void dmtxMarkHoughMaxima(DmtxHoughCache *hough);
void AddToVanishPointSort(VanishPointSort *sort, VanishPointSum vanishSum);
VanishPointSort dmtxFindVanishPoints(DmtxHoughCache *hough);
void AddToMaximaSort(HoughMaximaSort *sort, int maximaMag);
VanishPointSum GetAngleSumAtPhi(DmtxHoughCache *hough, int phi);
void AddToTimingSort(DmtxTimingSort *sort, Timing timing);
DmtxTimingSort dmtxFindGridTiming(DmtxHoughCache *hough, VanishPointSort *sort);
DmtxRay2 HoughCompactToRay(int phi, double d);
DmtxPassFail dmtxBuildGridFromTimings(AlignmentGrid *grid, Timing vp0, Timing vp1);
StripStats GenStripPatternStats(unsigned char *strip, int stripLength, int startState, int contrast);
GridRegion NudgeStripLimits(GridRegion *region, DmtxDirection side, int nudgeStyle);

DmtxPassFail dmtxFindRegionWithinGrid(GridRegion *region, AlignmentGrid *grid, DmtxHoughCache *houghCache, DmtxDecode *dec, DmtxCallbacks *fn);
int dmtxReadModuleColor(DmtxImage *img, AlignmentGrid *grid, int symbolRow, int symbolCol, int colorPlane);
DmtxBarType TestSideForPattern(GridRegion *region, DmtxImage *img, DmtxDirection side, int offset);
DmtxPassFail RegionExpand(GridRegion *region, DmtxDirection dir, DmtxHoughCache *houghCache, DmtxCallbacks *fn);
int dmtxGetSizeIdx(int a, int b);
DmtxPassFail RegionUpdateCorners(DmtxMatrix3 fit2raw, DmtxMatrix3 raw2fit, DmtxVector2 p00, DmtxVector2 p10, DmtxVector2 p11, DmtxVector2 p01);
DmtxPassFail dmtxDecodeSymbol(GridRegion *region, DmtxDecode *dec);
DmtxPassFail GetOnOffColors(GridRegion *region, const DmtxDecode *dec, int *onColor, int *offColor);
ColorTally GetTimingColors(GridRegion *region, const DmtxDecode *dec, int colBeg, int rowBeg, DmtxDirection dir);

/* Process visualization functions */
void EdgeCacheCallback(DmtxEdgeCache *edgeCache, int id);
void PixelEdgeCacheCallback(PixelEdgeCache *cache, int id);
void ZeroCrossingCallback(double xImg, double yImg, int sValue, int id);
void HoughCacheCallback(DmtxHoughCache *hough, int id);
void HoughCompactCallback(DmtxHoughCompact h, int id);
void VanishPointCallback(VanishPointSort *vPoints, int id);
void TimingCallback(Timing *timing0, Timing *timing1, int id);
void GridCallback(AlignmentGrid *grid, int id);
void PerimeterCallback(GridRegion *region, DmtxDirection side, DmtxBarType type);

int FindMaxEdgeIntensity(DmtxEdgeCache *edgeCache);
void BlitFlowCache(SDL_Surface *screen, int *cache, int maxFlowMag, int screenY, int screenX);
void BlitSobelCache(SDL_Surface *screen, PixelEdgeCache *cache, DmtxSobelDir dir, int x, int y, int screenY, int screenX);
void BlitHoughCache(SDL_Surface *screen, DmtxHoughCache *hough, int screenY, int screenX);
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

/* sobelcache.c */
PixelEdgeCache *PixelEdgeCacheCreate(int cacheWidth, int cacheHeight);
DmtxPassFail PixelEdgeCacheDestroy(PixelEdgeCache **sobel);
int PixelEdgeCacheGetWidth(PixelEdgeCache *sobel);
int PixelEdgeCacheGetHeight(PixelEdgeCache *sobel);
int PixelEdgeCacheGetValue(PixelEdgeCache *sobel, DmtxSobelDir dir, int x, int y);

PixelEdgeCache *SobelCacheCreate(DmtxImage *img);
DmtxPassFail SobelCachePopulate(PixelEdgeCache *sobel, DmtxImage *img);
PixelEdgeCache *AccelCacheCreate(PixelEdgeCache *sobel, DmtxDirection edgeType);
PixelEdgeCache *ZeroCrossingCacheCreate(PixelEdgeCache *zXing, DmtxDirection edgeType);
int SobelCacheGetValue(PixelEdgeCache *sobel, int sobelType, int sIdx);
int SobelCacheGetIndexFromZXing(PixelEdgeCache *sobel, DmtxDirection edgeType, int zCol, int zRow);

DmtxDecode2 *dmtxDecode2Create();
DmtxPassFail dmtxDecode2Destroy(DmtxDecode2 **dec);
DmtxPassFail dmtxDecode2SetImage(DmtxDecode2 *dec, DmtxImage *img);
DmtxPassFail decode2ReleaseCacheMemory(DmtxDecode2 *dec);

extern AppState gState;
