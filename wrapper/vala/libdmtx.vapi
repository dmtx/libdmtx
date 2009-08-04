[CCode (cheader_filename = "dmtx.h")]
namespace Dmtx {
	[CCode (cname = "DmtxVersion")]
	public const string VERSION;

	[CCode (cname = "DmtxUndefined")]
	public const int UNDEFINED;

	namespace Format {
		[CCode (cname = "DmtxFormatMatrix")]
		public const int MATRIX;
		[CCode (cname = "DmtxFormatMosaic")]
		public const int MOSAIC;
	}

	namespace SymbolCount {
		[CCode (cname = "DmtxSymbolSquareCount")]
		public const int SQUARE;
		[CCode (cname = "DmtxSymborRectCount")]
		public const int RECTANGLE;
	}

	namespace Module {
		[CCode (cname = "DmtxModuleOff")]
		public const int OFF;
		[CCode (cname = "DmtxModuleOnRed")]
		public const int RED;
		[CCode (cname = "DmtxModuleOnGreen")]
		public const int GREEN;
		[CCode (cname = "DmtxModuleOnBlue")]
		public const int BLUE;
		[CCode (cname = "DmtxModuleOnRGB")]
		public const int RGB;
		[CCode (cname = "DmtxModuleOn")]
		public const int ON;
		[CCode (cname = "DmtxModuleUnsure")]
		public const int UNSURE;
		[CCode (cname = "DmtxModuleAssigned")]
		public const int ASSIGNED;
		[CCode (cname = "DmtxModuleVisited")]
		public const int VISITED;
		[CCode (cname = "DmtxModuleData")]
		public const int DATA;
	}

	public enum Scheme {
		[CCode (cname = "DmtxSchemeAutoFast")]
		AUTO_FAST,
		[CCode (cname = "DmtxSchemeAutoBest")]
		AUTO_BEST,
		[CCode (cname = "DmtxSchemeAscii")]
		ASCII,
		[CCode (cname = "DmtxSchemeC40")]
		C40,
		[CCode (cname = "DmtxSchemeText")]
		TEXT,
		[CCode (cname = "DmtxSchemeX12")]
		X12,
		[CCode (cname = "DmtxSchemeEdifact")]
		EDIFACT,
		[CCode (cname = "DmtxSchemeBase256")]
		BASE256
	}

	public enum SymbolSize {
		[CCode (cname = "DmtxSymbolRectAuto")]
		RECT_AUTO,
		[CCode (cname = "DmtxSymbolSquareAuto")]
		SQUARE_AUTO,
		[CCode (cname = "DmtxSymbolShapeAuto")]
		SHAPE_AUTO,
		[CCode (cname = "DmtxSymbol10x10")]
		10X10,
		[CCode (cname = "DmtxSymbol12x12")]
		12X12,
		[CCode (cname = "DmtxSymbol14x14")]
		14X14,
		[CCode (cname = "DmtxSymbol16x16")]
		16X16,
		[CCode (cname = "DmtxSymbol18x18")]
		18X18,
		[CCode (cname = "DmtxSymbol20x20")]
		20X20,
		[CCode (cname = "DmtxSymbol22x22")]
		22X22,
		[CCode (cname = "DmtxSymbol24x24")]
		24X24,
		[CCode (cname = "DmtxSymbol26x26")]
		26X26,
		[CCode (cname = "DmtxSymbol32x32")]
		32X32,
		[CCode (cname = "DmtxSymbol36x36")]
		36X36,
		[CCode (cname = "DmtxSymbol40x40")]
		40X40,
		[CCode (cname = "DmtxSymbol44x44")]
		44X44,
		[CCode (cname = "DmtxSymbol48x48")]
		48X48,
		[CCode (cname = "DmtxSymbol52x52")]
		52X52,
		[CCode (cname = "DmtxSymbol64x64")]
		66X64,
		[CCode (cname = "DmtxSymbol72x72")]
		72X72,
		[CCode (cname = "DmtxSymbol80x80")]
		80X80,
		[CCode (cname = "DmtxSymbol88x88")]
		88X88,
		[CCode (cname = "DmtxSymbol96x96")]
		96X96,
		[CCode (cname = "DmtxSymbol104x104")]
		04X104,
		[CCode (cname = "DmtxSymbol120x120")]
		20X120,
		[CCode (cname = "DmtxSymbol132x132")]
		32X132,
		[CCode (cname = "DmtxSymbol144x144")]
		44X144,
		[CCode (cname = "DmtxSymbol8x18")]
		8X18,
		[CCode (cname = "DmtxSymbol8x32")]
		8X32,
		[CCode (cname = "DmtxSymbol12x26")]
		12X26,
		[CCode (cname = "DmtxSymbol12x36")]
		12X36,
		[CCode (cname = "DmtxSymbol16x36")]
		16X36,
		[CCode (cname = "DmtxSymbol16x48")]
		16X48
	}

	public enum Direction {
		[CCode (cname = "DmtxDirNone")]
		NONE,
		[CCode (cname = "DmtxDirUp")]
		UP,
		[CCode (cname = "DmtxDirLeft")]
		LEFT,
		[CCode (cname = "DmtxDirDown")]
		DOWN,
		[CCode (cname = "DmtxDirRight")]
		RIGHT,
		[CCode (cname = "DmtxDirHorizontal")]
		HORIZONTAL,
		[CCode (cname = "DmtxDirVertical")]
		VERTICAL,
		[CCode (cname = "DmtxDirRightUp")]
		RIGHT_UP,
		[CCode (cname = "DmtxDirLeftDown")]
		RIGHT_DOWN
	}

	[CCode (cname = "DmtxSymAttribute")]
	public enum SymbolAttribute {
		[CCode (cname = "DmtxSymAttribSymbolRows")]
		SYMXOL_ROWS,
		[CCode (cname = "DmtxSymAttribSymbolCols")]
		SYMBOL_COLS,
		[CCode (cname = "DmtxSymAttribDataRegionRows")]
		DATA_REGION_ROWS,
		[CCode (cname = "DmtxSymAttribDataRegionCols")]
		DATA_REGION_COLUMNS,
		[CCode (cname = "DmtxSymAttribHorizDataRegions")]
		HORIZONTAL_DATA_REGIONS,
		[CCode (cname = "DmtxSymAttribVertDataRegions")]
		VERTICAL_DATA_REGIONS,
		[CCode (cname = "DmtxSymAttribMappingMatrixRows")]
		MAPPING_MATRIX_ROWS,
		[CCode (cname = "DmtxSymAttribMappingMatrixCols")]
		MAPPING_MATRIX_COLUMNS,
		[CCode (cname = "DmtxSymAttribInterleavedBlocks")]
		INTERLEAVED_BLOCKS,
		[CCode (cname = "DmtxSymAttribBlockErrorWords")]
		BLOCK_ERROR_WORDS,
		[CCode (cname = "DmtxSymAttribBlockMaxCorrectable")]
		BLOCK_MAX_CORRECTABLE,
		[CCode (cname = "DmtxSymAttribSymbolDataWords")]
		SYMBOL_DATA_WORDS,
		[CCode (cname = "DmtxSymAttribSymbolErrorWords")]
		SYMBOL_ERROR_WORDS,
		[CCode (cname = "DmtxSymAttribSymbolMaxCorrectable")]
		MAX_CORRECTABLE;

		[CCode (cname = "dmtxGetSymbolAttribute")]
		public int get_property (int size_idx);
	}

	public enum CornerLoc {
		[CCode (cname = "DmtxCorner00")]
		CORNER_00,
		[CCode (cname = "DmtxCorner10")]
		CORNER_10,
		[CCode (cname = "DmtxCorner11")]
		CORNER_11,
		[CCode (cname = "DmtxCorner01")]
		CORNER_01
	}

	public enum Property {
		/* Encoding properties */
		[CCode (cname = "DmtxPropScheme")]
		SCHEME,
		[CCode (cname = "DmtxPropSizeRequest")]
		SIZE_REQUEST,
		[CCode (cname = "DmtxPropMarginSize")]
		MARGIN_SIZE,
		[CCode (cname = "DmtxPropModuleSize")]
		MODULE_SIZE,

		/* Decoding properties */
		[CCode (cname = "DmtxPropEdgeMin")]
		EDGE_MIN,
		[CCode (cname = "DmtxPropEdgeMax")]
		EDGE_MAX,
		[CCode (cname = "DmtxPropScanGap")]
		SCAN_GAP,
		[CCode (cname = "DmtxPropSquareDevn")]
		SQUARE_DEVIATION,
		[CCode (cname = "DmtxPropSymbolSize")]
		SIZE,
		[CCode (cname = "DmtxPropEdgeThresh")]
		EDGE_THRESHOLD,

		/* Image properties */
		[CCode (cname = "DmtxPropWidth")]
		WIDTH,
		[CCode (cname = "DmtxPropHeight")]
		HEIGHT,
		[CCode (cname = "DmtxPropPixelPacking")]
		PIXEL_PACKING,
		[CCode (cname = "DmtxPropBitsPerPixel")]
		BITS_PER_PIXEL,
		[CCode (cname = "DmtxPropBytesPerPixel")]
		BYTES_PER_PIXEL,
		[CCode (cname = "DmtxPropRowPadBytes")]
		ROW_PAD_BYTES,
		[CCode (cname = "DmtxPropRowSizeBytes")]
		ROW_SIZE_BYTES,
		[CCode (cname = "DmtxPropImageFlip")]
		IMAGE_FLIP,
		[CCode (cname = "DmtxPropChannelCount")]
		CHANNEL_COUNT,

		/* Image modifiers */
		[CCode (cname = "DmtxPropXmin")]
		X_MIN,
		[CCode (cname = "DmtxPropXmax")]
		X_MAX,
		[CCode (cname = "DmtxPropYmin")]
		Y_MIN,
		[CCode (cname = "DmtxPropYmax")]
		Y_MAX,
		[CCode (cname = "DmtxPropScale")]
		SCALE
	}

	public enum PackOrder {
	 /* Custom format */
		[CCode (cname = "DmtxPackCustom")]
		CUSTOM,
		/* 1 bpp */
		[CCode (cname = "DmtxPack1bppK")]
		1BPP_K,
		/* 8 bpp grayscale */
		[CCode (cname = "DmtxPack8bppK")]
		8BPP_K,
		/* 16 bpp formats */
		[CCode (cname = "DmtxPack16bppRGB")]
		16BPP_RGB,
		[CCode (cname = "DmtxPack16bppRGBX")]
		16BPP_RBGX,
		[CCode (cname = "DmtxPack16bppXRGB")]
		16BPP_XRGB,
		[CCode (cname = "DmtxPack16bppBGR")]
		16BPP_BGR,
		[CCode (cname = "DmtxPack16bppBGRX")]
		16BPP_BGRX,
		[CCode (cname = "DmtxPack16bppXBGR")]
		16BPP_XBGR,
		[CCode (cname = "DmtxPack16bppYCbCr")]
		16BPP_YCBCR,
		/* 24 bpp formats */
		[CCode (cname = "DmtxPack24bppRGB")]
		24BPP_RGB,
		[CCode (cname = "DmtxPack24bppBGR")]
		24BPP_BGR,
		[CCode (cname = "DmtxPack24bppYCbCr")]
		24BPP_YCBCR,
		/* 32 bpp formats */
		[CCode (cname = "DmtxPack32bppRGBX")]
		32BPP_RGBX,
		[CCode (cname = "DmtxPack32bppXRGB")]
		32BPP_XRGB,
		[CCode (cname = "DmtxPack32bppBGRX")]
		32BPP_BGRX,
		[CCode (cname = "DmtxPack32bppXBGR")]
		32BPP_XBGR,
		[CCode (cname = "DmtxPack32bppCMYK")]
		32BPP_CMYK
	}

	public enum Flip {
		[CCode (cname = "DmtxFlipNone")]
		NONE,
		[CCode (cname = "DmtxFlipX")]
		X,
		[CCode (cname = "DmtxFlipY")]
		Y
	}

	public struct Matrix3 {
		[CCode (cname = "dmtxMatrix3Copy")]
		public void copy (Matrix3 dest, Matrix3 src);
		[CCode (cname = "dmtxMatrix3Identity")]
		public void identity ();
		[CCode (cname = "dmtxMatrix3Translate")]
		public void translate (double tx, double ty);
		[CCode (cname = "dmtxMatrix3Rotate")]
		public void rotate (double angle);
		[CCode (cname = "dmtxMatrix3Scale")]
		public void scale (double sx, double sy);
		[CCode (cname = "dmtxMatrix3Shear")]
		public void shear (double shx, double shy);
		[CCode (cname = "dmtxMatrix3LineSkewTop")]
		public void line_skew_top (double b0, double b1, double sz);
		[CCode (cname = "dmtxMatrix3LineSkewTopInv")]
		public void line_skew_top_inv (double b0, double b1, double sz);
		[CCode (cname = "dmtxMatrix3LineSkewSide")]
		public void line_skew_side (double b0, double b1, double sz);
		[CCode (cname = "dmtxMatrix3LineSkewSideInv")]
		public void line_skew_side_inv (double b0, double b1, double sz);
		[CCode (cname = "dmtxMatrix3Multiply")]
		public static void multiply (Matrix3 dest, Matrix3 src1, Matrix3 src2);
		[CCode (cname = "dmtxMatrix3MultiplyBy")]
		public void multiply_by (Matrix3 matrix);
		[CCode (cname = "dmtxMatrix3VMultiply", instance_pos = -1)]
		public void vmultiply (Vector2 dest, Vector2 src_vector);
		[CCode (cname = "dmtxMatrix3Print")]
		public void print ();
	}

	public struct PixelLoc {
		public int X;
		public int Y;
	}

	public struct Vector2 {
		public double X;
		public double Y;

		[CCode (cname = "dmtxVector2AddTo")]
		public unowned Vector2 add_to (Vector2 vector);
		[CCode (cname = "dmtxVector2SubFrom")]
		public unowned Vector2 subtract_from (Vector2 vector);
		[CCode (cname = "dmtxVector2ScaleBy")]
		public unowned Vector2 scale_by (double s);
		[CCode (cname = "dmtxVector2Cross")]
		public double cross (Vector2 vector);
		[CCode (cname = "dmtxVector2Norm")]
		public double norm ();
		[CCode (cname = "dmtxVector2Dot")]
		public double dot (Vector2 vector);
		[CCode (cname = "dmtxVector2Mag")]
		public double mag ();
		[CCode (cname = "dmtxDistanceFromRay2", instance_pos = -1)]
		public double distance_from (Ray2 ray);
		[CCode (cname = "dmtxDistanceAlongRay2", instance_pos = -1)]
		public double distance_along (Ray2 ray);
	}

	public struct Ray2 {
		[CCode (cname = "tMin")]
		public double t_min;
		[CCode (cname = "tMax")]
		public double t_max;
		public Vector2 p;
		public Vector2 v;

		[CCode (cname = "dmtxDistanceFromRay2")]
		public double distance_from (Vector2 vector);
		[CCode (cname = "dmtxDistanceAlongRay2")]
		public double distance_along (Vector2 vector);
	}

	[Compact]
	public class Image {
		public int width;
		public int height;
		[CCode (cname = "pixelPacking")]
		public int pixel_packing;
		[CCode (cname = "bitsPerPixel")]
		public int bits_per_pixel;
		[CCode (cname = "bytesPerPixel")]
		public int bytes_per_pixel;
		[CCode (cname = "rowPadBytes")]
		public int row_pad_bytes;
		[CCode (cname = "rowSizeBytes")]
		public int row_size_bytes;
		[CCode (cname = "imageFlip")]
		public int image_flip;
		[CCode (cname = "channelCount")]
		public int channel_count;
		[CCode (cname = "channelStart")]
		public int channel_start[4];
		[CCode (cname = "bitsPerChannel")]
		public int bits_per_channel;
		[CCode (array_length = false)]
		public uchar[] pxl;

		[CCode (cname = "dmtxImageSetChannel")]
		public bool set_channel (int channel_start, int bits_per_channel);
		[CCode (cname = "dmtxImageSetProp")]
		public bool set_property (Property prop, int value);
		[CCode (cname = "dmtxImageGetProp")]
		public int get_property (Property prop);
		[CCode (cname = "dmtxImageGetByteOffset")]
		public int get_byte_offset (int x, int y);
		[CCode (cname = "dmtxImageGetPixelValue")]
		public bool get_pixel_value (int x, int y, int channel, out int value);
		[CCode (cname = "dmtxImageSetPixelValue")]
		public bool set_pixel_value (int x, int f, int channel, int value);
		[CCode (cname = "dmtxImageContainsInt")]
		public bool contains_int (int margin, int x, int y);
		[CCode (cname = "dmtxImageContainsFloat")]
		public bool contains_float (double x, double y);
	}

	public struct PointFlow {
		public int plane;
		public int arrive;
		public int depart;
		public int mag;
		public int loc;
	}

	public struct BestLine {
		public int angle;
		[CCode (cname = "hOffset")]
		public int horizontal_offset;
		public int mag;
		[CCode (cname = "stepBeg")]
		public int step_beg;
		[CCode (cname = "stepPos")]
		public int step_pos;
		[CCode (cname = "stepNeg")]
		public int step_neg;
		[CCode (cname = "distSq")]
		public int dist_sq;
		public double devn;
		[CCode (cname = "locBeg")]
		public PixelLoc loc_beg;
		[CCode (cname = "locPos")]
		public PixelLoc loc_pos;
		[CCode (cname = "locNeg")]
		public PixelLoc loc_neg;
	}

	/* This has to be a struct, since Encode.region is not a pointer. */
	[CCode (destroy_function = "dtmxRegionDestroy", free_function_address_of = true)]
	public struct Region {
		/* Trail blazing values */
		[CCode (cname = "jumpToPos")]
		public int jump_to_pos;
		[CCode (cname = "jumpToNeg")]
		public int jump_to_neg;
		[CCode (cname = "stepsTotal")]
		public int steps_total;
		[CCode (cname = "finalPos")]
		public PixelLoc final_pos;
		[CCode (cname = "finalNeg")]
		public PixelLoc final_neg;
		[CCode (cname = "boundMin")]
		public PixelLoc bound_min;
		[CCode (cname = "boundMax")]
		public PixelLoc bound_max;
		[CCode (cname = "flowBegin")]
		public PointFlow flow_begin;

		/* Orientation values */
		public int polarity;
		[CCode (cname = "stepR")]
		public int step_r;
		[CCode (cname = "stepT")]
		public int step_t;
		[CCode (cname = "locR")]
		public PixelLoc loc_r;
		[CCode (cname = "locT")]
		public PixelLoc loc_t;

		/* Region fitting values */
		[CCode (cname = "leftKnown")]
		public int left_known;
		[CCode (cname = "leftAngle")]
		public int left_angle;
		[CCode (cname = "leftLoc")]
		public PixelLoc left_loc;
		[CCode (cname = "leftLine")]
		public BestLine left_line;
		[CCode (cname = "bottomKnown")]
		public int bottom_known;
		[CCode (cname = "bottomAngle")]
		public int bottom_angle;
		[CCode (cname = "bottomLoc")]
		public PixelLoc bottom_loc;
		[CCode (cname = "bottomLine")]
		public BestLine bottom_line;
		[CCode (cname = "topKnown")]
		public int top_known;
		[CCode (cname = "topAngle")]
		public int top_angle;
		[CCode (cname = "topLoc")]
		public PixelLoc top_loc;
		[CCode (cname = "rightKnown")]
		public int right_known;
		[CCode (cname = "rightAngle")]
		public int right_angle;
		[CCode (cname = "rightLoc")]
		public PixelLoc right_loc;

		/* Region calibration values */
		[CCode (cname = "onColor")]
		public int on_color;
		[CCode (cname = "offColor")]
		public int off_color;
		[CCode (cname = "sizeIdx")]
		public int size_idx;
		[CCode (cname = "symbolRows")]
		public int symbol_rows;
		[CCode (cname = "symbolCols")]
		public int symbol_columns;
		[CCode (cname = "mappingRows")]
		public int mapping_rows;
		[CCode (cname = "mappingCols")]
		public int mapping_columns;

		/* Transform values */
		[CCode (cname = "raw2fit")]
		public Matrix3 raw_2_fit;
		[CCode (cname = "fit2raw")]
		public Matrix3 fit_2_raw;

		[CCode (cname = "dmtxRegionCreate")]
		public Region? copy ();
		[CCode (cname = "dmtxRegionFindNext")]
		public Region? find_next (ref Time timeout);
		[CCode (cname = "dmtxRegionScanPixel")]
		public Region? scan_pixel (int x, int y);
		[CCode (cname = "dmtxRegionUpdateCorners")]
		public bool update_corners (Region reg, Vector2 p00, Vector2 p10, Vector2 p11, Vector2 p01);
		[CCode (cname = "dmtxRegionUpdateXfrms", instance_pos = -1)]
		public bool update_xfrms (Decode dec);
	}

	[Compact, CCode(free_function = "dmtxMessageDestroy", free_function_address_of = true)]
	public class Message {
		[CCode (cname = "arraySize")]
		public size_t array_size;
		[CCode (cname = "codeSize")]
		public size_t code_size;
		[CCode (cname = "outputSize")]
		public size_t output_size;
		[CCode (cname = "outputIdx")]
		public int output_idx;
		[CCode (cname = "padCount")]
		public int pad_count;

		[CCode (cname = "dtmxMessageCreate")]
		public Message (int size_idx, int symbol_format);
		[CCode (cname = "dmtxSymbolModuleStatus")]
		public int status (int size_idx, int row, int col);
	}

	public struct ScanGrid {
		[CCode (cname = "minExtent")]
		public int min_extent;
		[CCode (cname = "maxExtent")]
		public int max_extent;
		[CCode (cname = "xOffset")]
		public int x_offset;
		[CCode (cname = "yOffset")]
		public int y_offset;
		[CCode (cname = "xMin")]
		public int x_min;
		[CCode (cname = "xMax")]
		public int x_max;
		[CCode (cname = "yMin")]
		public int y_min;
		[CCode (cname = "yMax")]
		public int y_max;

		public int total;
		[CCode (cname = "extent")]
		public int extent;
		[CCode (cname = "jumpSize")]
		public int jump_size;
		[CCode (cname = "pixelTotal")]
		public int pixel_total;
		[CCode (cname = "startPos")]
		public int start_pos;

		[CCode (cname = "pixelCount")]
		public int pixel_count;
		[CCode (cname = "xCenter")]
		public int x_center;
		[CCode (cname = "yCenter")]
		public int y_center;
	}

	[SimpleType]
	public struct Time {
		public time_t sec;
		public ulong usec;

		[CCode (cname = "dmtxTimeNow")]
		public Time ();
		[CCode (cname = "dmtxTimeAdd")]
		public Time @add (long msec);
		[CCode (cname = "dmtxTimeExceeded")]
		public int exceeded ();
	}

	[Compact, CCode (free_function = "dmtxDecodeDestroy", free_function_address_of = true)]
	public class Decode {
		[CCode (cname = "edgeMin")]
		public int edge_min;
		[CCode (cname = "edgeMax")]
		public int edge_max;
		[CCode (cname = "scanGap")]
		public int scan_gap;
		[CCode (cname = "squareDevn")]
		public double square_devn;
		[CCode (cname = "sizeIdxExpected")]
		public int size_idx_expected;
		[CCode (cname = "edgeThresh")]
		public int edge_thresh;

		[CCode (cname = "xMin")]
		public int x_min;
		[CCode (cname = "xMax")]
		public int x_max;
		[CCode (cname = "yMin")]
		public int y_min;
		[CCode (cname = "yMax")]
		public int y_max;
		public int scale;

		[CCode (array_length = false)]
		public uchar[] cache;
		public Image image;
		public ScanGrid grid;

		public Decode (Image img, int scale);
		[CCode (cname = "dmtxDecodeSetProp")]
		public bool set_property (Property prop, int value);
		[CCode (cname = "dmtxDecodeGetProp")]
		public int get_property (Property prop);
		[CCode (cname = "dmtxDecodeGetCache", array_length = false)]
		public uchar[] get_cache (out int header_bytes, int style);
		[CCode (cname = "dmtxDecodeGetPixelValue")]
		public bool get_pixel_value (int x, int y, int channel, out int value);
		[CCode (cname = "dmtxDecodeMatrixRegion")]
		public Message matrix_region (Region reg, int fix);
		[CCode (cname = "dmtxDecodeMosaicRegion")]
		public Message mosaic_region (Region reg, int fix);
		[CCode (cname = "dmtxDecodeCreateDiagnostic")]
		public uchar[] create_diagnostic (out int header_bytes, int style);
	}

	[Compact, CCode (free_function = "dmtxEncodeDestroy", free_function_address_of = true)]
	public class Encode {
		public int method;
		public int scheme;
		[CCode (cname = "sizeIdxRequest")]
		public int size_index_request;
		[CCode (cname = "marginSize")]
		public int marginSize;
		[CCode (cname = "moduleSize")]
		public int module_size;
		[CCode (cname = "pixelPacking")]
		public int pixel_packing;
		[CCode (cname = "imageFlip")]
		public int image_flip;
		[CCode (cname = "rowPadBytes")]
		public int row_pad_bytes;
		public Message message;
		public Image image;
		public Region region;
		public Matrix3 xfrm;
		public Matrix3 rxfrm;

		[CCode (cname = "dmtxEncodeCreate")]
		public Encode ();
		[CCode (cname = "dmtxEncodeSetProp")]
		public bool set_property (Property prop, int value);
		[CCode (cname = "dmtxEncodeGetProp")]
		public int get_property (Property prop);
		[CCode (cname = "dmtxEncodeDataMatrix")]
		public bool data_matrix ([CCode (array_length_pos = 0.9)] uchar[] s);
		[CCode (cname = "dmtxEncodeDataMosaic")]
		public bool data_mosaic ([CCode (array_length_pos = 0.9)] uchar[] s);
	}

	public struct Channel {
		[CCode (cname = "encScheme")]
		public int enc_scheme;
		public int invalid;
		[CCode (cname = "inputPtr", array_length = false)]
		public uchar[] inputPtr;
		[CCode (cname = "inputStop", array_length = false)]
		public uchar[] inputStop;
		[CCode (cname = "encodedLength")]
		public int encoded_length;
		[CCode (cname = "currentLength")]
		public int current_length;
		[CCode (cname = "firstCodeWord")]
		public int first_code_word;
		[CCode (cname = "encodedWords")]
		public uchar encoded_words[1558];
	}

	public struct ChannelGroup {
		public Channel channel[6];
	}

	public struct Triplet {
		public uchar value[3];
	}

	public struct Quadruplet {
		public uchar value[4];
	}

	public static int get_symbol_attribute (int attribute, int size_idx);
	public static int get_block_data_size (int size_idx, int block_idx);
}
