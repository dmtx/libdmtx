/*
libdmtx-net - .NET wrapper for libdmtx

Copyright (C) 2009 Joseph Ferner / Tom Vali

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

Contact: libdmtx@fernsroth.com
*/

/* $Id$ */

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Drawing.Imaging;

namespace Libdmtx {
    /// <summary>
    /// Wrapper for decoding and encoding DataMatrix barcodes.
    /// </summary>
    public static class Dmtx {
        const byte RETURN_NO_MEMORY = 1;
        const byte RETURN_INVALID_ARGUMENT = 2;
        const byte RETURN_ENCODE_ERROR = 3;
        public const int DmtxUndefined = -1; // defined in "dmtx.h"

        /// <summary>
        /// Gets the version of the underlying libdmtx used.
        /// </summary>
        public static string Version {
            get {
                return DmtxVersion();
            }
        }

        /// <summary>
        /// Decodes a bitmap returning all symbols found in the image.
        /// </summary>
        /// <param name="b">The bitmap to decode.</param>
        /// <param name="options">The options used for decoding.</param>
        /// <returns>An array of decoded symbols, one for each symbol found.</returns>
        /// <example>
        /// This example shows a basic decoding.
        /// <code>
        ///   Bitmap bm = (Bitmap)Bitmap.FromFile("barcode.bmp");
        ///   DecodeOptions decodeOptions = new DecodeOptions();
        ///   DmtxDecoded[] decodeResults = Dmtx.Decode(bm, decodeOptions);
        ///   for (int i = 0; i &lt; decodeResults.Length; i++) {
        ///     string str = Encoding.ASCII.GetString(decodeResults[i].Data).TrimEnd('\0');
        ///     Console.WriteLine("Decode " + i + ": \"" + str + "\"");
        ///   }
        /// </code>
        /// </example>
        public static DmtxDecoded[] Decode(Bitmap b, DecodeOptions options) {
            List<DmtxDecoded> results = new List<DmtxDecoded>();
            Decode(b, options, delegate(DmtxDecoded d) { results.Add(d); });
            return results.ToArray();
        }

        public delegate void DecodeCallback(DmtxDecoded decoded);
        public static void Decode(Bitmap b, DecodeOptions options, DecodeCallback Callback) {
            Exception decodeException = null;
            byte status;
            try {
                byte[] pxl = BitmapToByteArray(b);

                status = DmtxDecode(
                    pxl,
                    (UInt32)b.Width,
                    (UInt32)b.Height,
                    options,
                    delegate(DecodedInternal dmtxDecodeResult) {
                        DmtxDecoded result;
                        try {
                            result = new DmtxDecoded();
                            result.Corners = dmtxDecodeResult.Corners;
                            result.SymbolInfo = dmtxDecodeResult.SymbolInfo;
                            result.Data = new byte[dmtxDecodeResult.DataSize];
                            for (int dataIdx = 0; dataIdx < dmtxDecodeResult.DataSize; dataIdx++) {
                                result.Data[dataIdx] = Marshal.ReadByte(dmtxDecodeResult.Data, dataIdx);
                            }
                            Callback(result);
                            return true;
                        } catch (Exception ex) {
                            decodeException = ex;
                            return false;
                        }
                    });
            } catch (Exception ex) {
                throw new DmtxException("Error calling native function.", ex);
            }
            if (decodeException != null) {
                throw decodeException;
            }
            if (status == RETURN_NO_MEMORY) {
                throw new DmtxOutOfMemoryException("Not enough memory.");
            } else if (status == RETURN_INVALID_ARGUMENT) {
                throw new DmtxInvalidArgumentException("Invalid options configuration.");
            } else if (status > 0) {
                throw new DmtxException("Unknown error.");
            }
        }

        private static byte[] BitmapToByteArray(Bitmap b) {
            Rectangle rect = new Rectangle(0, 0, b.Width, b.Height);
            BitmapData bd = b.LockBits(rect, ImageLockMode.ReadOnly, PixelFormat.Format24bppRgb);
            try {
                byte[] pxl = new byte[b.Width * b.Height * 3];
                DmtxBitmapToByteArray(bd.Scan0, bd.Stride, b.Width, b.Height, pxl);
                return pxl;
            } finally {
                b.UnlockBits(bd);
            }
        }

        /// <summary>
        /// Encodes data into a DataMatrix barcode.
        /// </summary>
        /// <param name="data">The data to encode.</param>
        /// <param name="options">The options used for encoding.</param>
        /// <returns>The results from encoding.</returns>
        /// <example>
        /// This example shows a basic encoding.
        /// <code>
        ///   byte[] data = Encoding.ASCII.GetBytes("test");
        ///   EncodeOptions o = new EncodeOptions();
        ///   DmtxEncoded encodeResults = Dmtx.Encode(data, o);
        ///   encodeResults.Bitmap.Save("barcode.bmp", ImageFormat.Bmp);
        /// </code>
        /// </example>
        public static DmtxEncoded Encode(byte[] data, EncodeOptions options) {
            IntPtr result;
            byte status;
            try {
                status = DmtxEncode(data, (UInt16)data.Length, out result, options);
            } catch (Exception ex) {
                throw new DmtxException("Encoding error.", ex);
            }
            if (status == RETURN_NO_MEMORY) {
                throw new DmtxOutOfMemoryException("Not enough memory.");
            } else if (status == RETURN_INVALID_ARGUMENT) {
                throw new DmtxInvalidArgumentException("Invalid options configuration.");
            } else if (status == RETURN_ENCODE_ERROR) {
                throw new DmtxException("Error while encoding.");
            } else if ((status > 0) || (result == IntPtr.Zero)) {
                throw new DmtxException("Unknown error.");
            }

            DmtxEncoded ret;
            EncodedInternal intResult = null;
            try {
                intResult = (EncodedInternal)Marshal.PtrToStructure(result, typeof(EncodedInternal));
                ret = new DmtxEncoded();
                ret.SymbolInfo = intResult.SymbolInfo;
                ret.Bitmap = new Bitmap((int)intResult.Width, (int)intResult.Height, PixelFormat.Format24bppRgb);
                Rectangle rect = new Rectangle(0, 0, ret.Bitmap.Width, ret.Bitmap.Height);
                BitmapData bd = ret.Bitmap.LockBits(rect, ImageLockMode.WriteOnly, PixelFormat.Format24bppRgb);
                try {
                    DmtxCopyEncodeResult(intResult.Data, (UInt32)Math.Abs(bd.Stride), bd.Scan0);
                } finally {
                    ret.Bitmap.UnlockBits(bd);
                }
            } catch (Exception ex) {
                throw new DmtxException("Error parsing encode result.", ex);
            } finally {
                try {
                    if (intResult != null) {
                        DmtxFreeEncodeResult(intResult.Data);
                    }
                } catch (Exception ex) {
                    throw new DmtxException("Error freeing memory.", ex);
                }
            }
            return ret;
        }

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate bool DmtxDecodeCallback(DecodedInternal dmtxDecodeResult);

        [DllImport("libdmtx.dll", EntryPoint = "dmtx_decode")]
        private static extern byte
        DmtxDecode(
            [In] byte[] image,
            [In] UInt32 width,
            [In] UInt32 height,
            [In] DecodeOptions options,
            [In] DmtxDecodeCallback callback);

        [DllImport("libdmtx.dll", EntryPoint = "dmtx_bitmap_to_byte_array")]
        private static extern void DmtxBitmapToByteArray(
            [In] IntPtr image,
            [In] Int32 stride,
            [In] Int32 width,
            [In] Int32 height,
            [Out] byte[] dest
            );

        [DllImport("libdmtx.dll", EntryPoint = "dmtx_encode")]
        private static extern byte
        DmtxEncode(
            [In] byte[] plain_text,
            [In] UInt16 text_size,
            [Out] out IntPtr result,
            [In] EncodeOptions options);

        [DllImport("libdmtx.dll", EntryPoint = "dmtx_copy_encode_result")]
        private static extern void
        DmtxCopyEncodeResult(
            [In] IntPtr data,
            [In] UInt32 stride,
            [In] IntPtr bitmap);

        [DllImport("libdmtx.dll", EntryPoint = "dmtx_free_encode_result")]
        private static extern void
        DmtxFreeEncodeResult([In] IntPtr data);

        [DllImport("libdmtx.dll", EntryPoint = "dmtx_version", CharSet = CharSet.Ansi)]
        private static extern string
        DmtxVersion();
    }

    /// <summary>
    /// Enumeration of symbol sizes.
    /// </summary>
    public enum CodeSize : short {
        SymbolRectAuto = -3,
        SymbolSquareAuto = -2,
        SymbolShapeAuto = -1,
        Symbol10x10 = 0,
        Symbol12x12 = 1,
        Symbol14x14,
        Symbol16x16,
        Symbol18x18,
        Symbol20x20,
        Symbol22x22,
        Symbol24x24,
        Symbol26x26,
        Symbol32x32,
        Symbol36x36,
        Symbol40x40,
        Symbol44x44,
        Symbol48x48,
        Symbol52x52,
        Symbol64x64,
        Symbol72x72,
        Symbol80x80,
        Symbol88x88,
        Symbol96x96,
        Symbol104x104,
        Symbol120x120,
        Symbol132x132,
        Symbol144x144,
        Symbol8x18,
        Symbol8x32,
        Symbol12x26,
        Symbol12x36,
        Symbol16x36,
        Symbol16x48
    }

    /// <summary>
    /// Symbol type.
    /// </summary>
    public enum CodeType : ushort {
        DataMatrix = 0,

        /// <summary>
        /// Composite of multiple data matrixes.
        /// </summary>
        Mosaic = 1
    }

    /// <summary>
    /// Encodings applied to the data.
    /// </summary>
    public enum Scheme : ushort {
        /// <summary>
        /// ASCII character 0 to 127. 1 byte per CW.
        /// </summary>
        Ascii = 0,

        /// <summary>
        /// Upper-case alphanumeric. 1.5 byte per CW.
        /// </summary>
        C40 = 1,

        /// <summary>
        /// Lower-case alphanumeric. 1.5 byte per CW.
        /// </summary>
        Text,

        /// <summary>
        /// ANSI X12. 1.5 byte per CW.
        /// </summary>
        X12,

        /// <summary>
        /// ASCII character 32 to 94. 1.33 bytes per CW.
        /// </summary>
        Edifact,

        /// <summary>
        /// ASCII character 0 to 255. 1 byte per CW.
        /// </summary>
        Base256,
        AutoBest,
        AutoFast
    }

    /// <summary>
    /// Options used for decoding using <see cref="Dmtx.Decode(Bitmap,DecodeOptions)"/>
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public class DecodeOptions {
        public Int16 EdgeMin = Dmtx.DmtxUndefined;
        public Int16 EdgeMax = Dmtx.DmtxUndefined;
        public Int16 ScanGap = Dmtx.DmtxUndefined;
        public Int16 SquareDevn = Dmtx.DmtxUndefined;
        public Int32 TimeoutMS = Dmtx.DmtxUndefined;
        public CodeSize SizeIdxExpected = CodeSize.SymbolShapeAuto;
        public Int16 EdgeThresh = Dmtx.DmtxUndefined;
        public Int16 MaxCodes = Dmtx.DmtxUndefined;
        public Int16 XMin = Dmtx.DmtxUndefined;
        public Int16 XMax = Dmtx.DmtxUndefined;
        public Int16 YMin = Dmtx.DmtxUndefined;
        public Int16 YMax = Dmtx.DmtxUndefined;
        public Int16 CorrectionsMax = Dmtx.DmtxUndefined;
        public CodeType CodeType = CodeType.DataMatrix;
        public Int16 Shrink = 1;
    }

    /// <summary>
    /// Options used for encoding using <see cref="Dmtx.Encode"/>.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public class EncodeOptions {
        /// <summary>
        /// Whitespace padding in pixels around symbol.
        /// </summary>
        public UInt16 MarginSize;

        /// <summary>
        /// The size in pixels of each element in the symbol.
        /// </summary>
        public UInt16 ModuleSize;

        /// <summary>
        /// The encoding to apply to the symbol.
        /// </summary>
        public Scheme Scheme;
        public UInt16 Rotate;

        /// <summary>
        /// Size of the symbol to generate.
        /// </summary>
        public CodeSize SizeIdx;

        /// <summary>
        /// Type of symbol to generate.
        /// </summary>
        public CodeType CodeType;

        public EncodeOptions() {
            MarginSize = 10;
            ModuleSize = 5;
            Scheme = Scheme.Ascii;
            Rotate = 0;
            SizeIdx = CodeSize.SymbolSquareAuto;
            CodeType = CodeType.DataMatrix;
        }
    }

    /// <summary>
    /// 2D coordinate.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public class DmtxPoint {
        public UInt16 X;
        public UInt16 Y;
    }

    /// <summary>
    /// The four corners in 2D space of the symbol.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public class Corners {
        public DmtxPoint Corner0;
        public DmtxPoint Corner1;
        public DmtxPoint Corner2;
        public DmtxPoint Corner3;
    }

    /// <summary>
    /// Information about the DMTX symbol.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public class SymbolInfo {
        public UInt16 Rows;
        public UInt16 Cols;
        public UInt16 Capacity;
        public UInt16 DataWords;
        public UInt16 PadWords;
        public UInt16 ErrorWords;
        public UInt16 HorizDataRegions;
        public UInt16 VertDataRegions;
        public UInt16 InterleavedBlocks;
        public UInt16 Angle;
    }

    /// <summary>
    /// Returned from <see cref="Dmtx.Decode(Bitmap,DecodeOptions)"/>.
    /// </summary>
    public class DmtxDecoded {
        /// <summary>
        /// Information about the symbol that was decoded.
        /// </summary>
        public SymbolInfo SymbolInfo;
        public Corners Corners;

        /// <summary>
        /// The data contained in the symbol. If the contains a string
        /// use the following code to convert it.
        /// <code>
        /// string str = Encoding.ASCII.GetString(decodeResults[0].Data).TrimEnd('\0');
        /// </code>
        /// </summary>
        public byte[] Data;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal class DecodedInternal {
        public SymbolInfo SymbolInfo;
        public Corners Corners;
        public IntPtr Data;
        public UInt32 DataSize;
    }

    /// <summary>
    /// Returned from <see cref="Dmtx.Encode"/>.
    /// </summary>
    public class DmtxEncoded {
        /// <summary>
        /// Information about the symbol that was created.
        /// </summary>
        public SymbolInfo SymbolInfo;

        /// <summary>
        /// The bitmap containing the symbol.
        /// </summary>
        public Bitmap Bitmap;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal class EncodedInternal {
        public SymbolInfo SymbolInfo;
        public UInt32 Width;
        public UInt32 Height;
        public IntPtr Data;
    }

    /// <summary>
    /// Base Dmtx exception.
    /// </summary>
    public class DmtxException : ApplicationException {
        public DmtxException() { }
        public DmtxException(string message) : base(message) { }
        public DmtxException(string message, Exception innerException)
            : base(message, innerException) { }
    }

    /// <summary>
    /// Out of memory exception.
    /// </summary>
    public class DmtxOutOfMemoryException : DmtxException {
        public DmtxOutOfMemoryException() { }
        public DmtxOutOfMemoryException(string message) : base(message) { }
        public DmtxOutOfMemoryException(string message, Exception innerException)
            : base(message, innerException) { }
    }

    // Invalid argument exception.
    public class DmtxInvalidArgumentException : DmtxException {
        public DmtxInvalidArgumentException() { }
        public DmtxInvalidArgumentException(string message) : base(message) { }
        public DmtxInvalidArgumentException(string message, Exception innerException)
            : base(message, innerException) { }
    }
}