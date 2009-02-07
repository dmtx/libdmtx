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

Contact: mike@dragonflylogic.com
*/

/* $Id$ */

using System;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Drawing.Imaging;

namespace LibDmtx {
    public static class Dmtx {
        const byte RETURN_NO_MEMORY = 1;
        const byte RETURN_INVALID_ARGUMENT = 2;
        const byte RETURN_ENCODE_ERROR = 3;

        public static string Version {
            get {
                return DmtxVersion();
            }
        }

        public static DmtxDecoded[] Decode(Bitmap b, DecodeOptions options) {
            IntPtr results;
            UInt32 resultcount;
            byte status;
            try {
                BitmapData bd = b.LockBits(
                     new Rectangle(0, 0, b.Width, b.Height),
                     ImageLockMode.ReadOnly,
                     PixelFormat.Format24bppRgb);
                try {
                    status = DmtxDecode(
                                bd.Scan0,
                                (UInt32)bd.Stride / 3,//b.Width,
                                (UInt32)b.Height,
                                out results,
                                out resultcount,
                                options);
                } finally {
                    b.UnlockBits(bd);
                }
            } catch (Exception ex) {
                throw new DmtxException("Error calling native function.", ex);
            }
            if (status == RETURN_NO_MEMORY)
                throw new DmtxOutOfMemoryException("Not enough memory.");
            else if (status == RETURN_INVALID_ARGUMENT)
                throw new DmtxInvalidArgumentException("Invalid options configuration.");
            else if ((status > 0) || (results == IntPtr.Zero))
                throw new DmtxException("Unknown error.");

            DmtxDecoded[] result;
            try {
                result = new DmtxDecoded[resultcount];
                IntPtr accResult = results;
                for (uint i = 0; i < resultcount; i++) {
                    DecodedInternal intResult = (DecodedInternal)
                        Marshal.PtrToStructure(accResult, typeof(DecodedInternal));
                    result[i] = new DmtxDecoded();
                    result[i].Corners = intResult.Corners;
                    result[i].SymbolInfo = intResult.SymbolInfo;
                    result[i].Data = Marshal.PtrToStringAnsi(
                        intResult.Data, (int)intResult.DataSize);
                    accResult = (IntPtr)((long)accResult + Marshal.SizeOf(intResult));
                }
            } catch (Exception ex) {
                throw new DmtxException("Error parsing decode results.", ex);
            } finally {
                try {
                    DmtxFreeDecodeResult(results, resultcount);
                } catch (Exception ex) {
                    throw new DmtxException("Error freeing memory.", ex);
                }
            }
            return result;
        }

        public static DmtxEncoded Encode(String s, EncodeOptions options) {
            IntPtr result;
            byte status;
            try {
                System.Text.ASCIIEncoding ascii = new System.Text.ASCIIEncoding();
                byte[] bytes = ascii.GetBytes(s);
                status = DmtxEncode(
                    bytes,
                    (UInt16)bytes.Length,
                    out result,
                    options);
            } catch (Exception ex) {
                throw new DmtxException("Encoding error.", ex);
            }
            if (status == RETURN_NO_MEMORY)
                throw new DmtxOutOfMemoryException("Not enough memory.");
            else if (status == RETURN_INVALID_ARGUMENT)
                throw new DmtxInvalidArgumentException("Invalid options configuration.");
            else if (status == RETURN_ENCODE_ERROR)
                throw new DmtxException("Error while encoding.");
            else if ((status > 0) || (result == IntPtr.Zero))
                throw new DmtxException("Unknown error.");

            DmtxEncoded ret;
            EncodedInternal intResult = null;
            try {
                intResult = (EncodedInternal)
                    Marshal.PtrToStructure(result, typeof(EncodedInternal));
                ret = new DmtxEncoded();
                ret.SymbolInfo = intResult.SymbolInfo;
                ret.Bitmap = new Bitmap(
                    (int)intResult.Width,
                    (int)intResult.Height,
                    PixelFormat.Format24bppRgb);
                BitmapData bd = ret.Bitmap.LockBits(
                    new Rectangle(0, 0, ret.Bitmap.Width, ret.Bitmap.Height),
                    ImageLockMode.WriteOnly,
                    PixelFormat.Format24bppRgb);
                try {
                    DmtxCopyEncodeResult(
                        intResult.Data,
                        (UInt32)Math.Abs(bd.Stride),
                        bd.Scan0);
                } finally {
                    ret.Bitmap.UnlockBits(bd);
                }
            } catch (Exception ex) {
                throw new DmtxException("Error parsing encode result.", ex);
            } finally {
                try {
                    if (intResult != null)
                        DmtxFreeEncodeResult(intResult.Data);
                } catch (Exception ex) {
                    throw new DmtxException("Error freeing memory.", ex);
                }
            }
            return ret;
        }

        [DllImport("libdmtx.dll", EntryPoint = "dmtx_decode")]
        private static extern byte
        DmtxDecode(
            [In] IntPtr image,
            [In] UInt32 width,
            [In] UInt32 height,
            [Out] out IntPtr results,
            [Out] out UInt32 resultcount,
            [In] DecodeOptions options);

        [DllImport("libdmtx.dll", EntryPoint = "dmtx_free_decode_result")]
        private static extern void
        DmtxFreeDecodeResult(
            [In] IntPtr result,
            [In] UInt32 count);

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

    public enum CodeType : ushort {
        DataMatrix = 0,
        Mosaic = 1
    }

    public enum Scheme : ushort {
        Ascii = 0,
        C40 = 1,
        Text,
        X12,
        Edifact,
        Base256,
        AutoBest,
        AutoFast
    }

    [StructLayout(LayoutKind.Sequential)]
    public class DecodeOptions {
        public Int16 EdgeMin = -1;
        public Int16 EdgeMax = -1;
        public Int16 ScanGap = -1;
        public Int16 SquareDevn = -1;
        public Int32 TimeoutMS = -1;
        public CodeSize SizeIdxExpected = CodeSize.SymbolShapeAuto;
        public Int16 EdgeTresh = -1;
        public Int16 MaxCodes = -1;
        public Int16 XMin = -1;
        public Int16 XMax = -1;
        public Int16 YMin = -1;
        public Int16 YMax = -1;
        public Int16 CorrectionsMax = -1;
        public CodeType CodeType = CodeType.DataMatrix;
        public Int16 ShrinkMin = -1;
        public Int16 ShrinkMax = -1;
    }

    [StructLayout(LayoutKind.Sequential)]
    public class EncodeOptions {
        public UInt16 MarginSize;
        public UInt16 ModuleSize;
        public Scheme Scheme;
        public UInt16 Rotate;
        public CodeSize SizeIdx;
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

    [StructLayout(LayoutKind.Sequential)]
    public class DmtxPoint {
        public UInt16 X;
        public UInt16 Y;
    }

    [StructLayout(LayoutKind.Sequential)]
    public class Corners {
        public DmtxPoint Corner0;
        public DmtxPoint Corner1;
        public DmtxPoint Corner2;
        public DmtxPoint Corner3;
    }

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

    public class DmtxDecoded {
        public SymbolInfo SymbolInfo;
        public Corners Corners;
        public string Data;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal class DecodedInternal {
        public SymbolInfo SymbolInfo;
        public Corners Corners;
        public IntPtr Data;
        public UInt32 DataSize;
    }

    public class DmtxEncoded {
        public SymbolInfo SymbolInfo;
        public Bitmap Bitmap;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal class EncodedInternal {
        public SymbolInfo SymbolInfo;
        public UInt32 Width;
        public UInt32 Height;
        public IntPtr Data;
    }

    public class DmtxException : ApplicationException {
        public DmtxException() { }
        public DmtxException(string message) : base(message) { }
        public DmtxException(string message, Exception innerException)
            : base(message, innerException) { }
    }
    public class DmtxOutOfMemoryException : DmtxException {
        public DmtxOutOfMemoryException() { }
        public DmtxOutOfMemoryException(string message) : base(message) { }
        public DmtxOutOfMemoryException(string message, Exception innerException)
            : base(message, innerException) { }
    }
    public class DmtxInvalidArgumentException : DmtxException {
        public DmtxInvalidArgumentException() { }
        public DmtxInvalidArgumentException(string message) : base(message) { }
        public DmtxInvalidArgumentException(string message, Exception innerException)
            : base(message, innerException) { }
    }
}