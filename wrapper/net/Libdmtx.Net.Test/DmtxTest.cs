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
using System.Drawing.Imaging;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using NUnit.Framework;
using System.Drawing;
using System.IO;

namespace Libdmtx {
    [TestFixture]
    public class DmtxTest {
        [Test]
        public void TestDecode() {
            Bitmap bm = GetBitmapFromResource("Libdmtx.TestImages.Test001.png");
            DecodeOptions opt = new DecodeOptions();
            DmtxDecoded[] decodeResults = Dmtx.Decode(bm, opt);
            Assert.AreEqual(1, decodeResults.Length);
            string data = Encoding.ASCII.GetString(decodeResults[0].Data).TrimEnd('\0');
            Assert.AreEqual("Test", data);
        }

        [Test]
        public void TestDecodeTwoBarCodes() {
            Bitmap bm = GetBitmapFromResource("Libdmtx.TestImages.Test002.png");
            DecodeOptions opt = new DecodeOptions();
            DmtxDecoded[] decodeResults = Dmtx.Decode(bm, opt);
            Assert.AreEqual(2, decodeResults.Length);
            string data1 = Encoding.ASCII.GetString(decodeResults[0].Data).TrimEnd('\0');
            Assert.AreEqual("Test1", data1);
            string data2 = Encoding.ASCII.GetString(decodeResults[1].Data).TrimEnd('\0');
            Assert.AreEqual("Test2", data2);
        }

        [Test]
        public void TestDecodeWithCallback() {
            List<DmtxDecoded> decodeResults = new List<DmtxDecoded>();
            Bitmap bm = GetBitmapFromResource("Libdmtx.TestImages.Test002.png");
            DecodeOptions opt = new DecodeOptions();
            Dmtx.Decode(bm, opt, d => decodeResults.Add(d));
            Assert.AreEqual(2, decodeResults.Count);
            string data1 = Encoding.ASCII.GetString(decodeResults[0].Data).TrimEnd('\0');
            Assert.AreEqual("Test1", data1);
            string data2 = Encoding.ASCII.GetString(decodeResults[1].Data).TrimEnd('\0');
            Assert.AreEqual("Test2", data2);
        }

        [Test]
        public void TestDecodeWithCallbackThrowException() {
            Bitmap bm = GetBitmapFromResource("Libdmtx.TestImages.Test002.png");
            DecodeOptions opt = new DecodeOptions();
            int callCount = 0;
            try {
                Dmtx.Decode(bm, opt, d => {
                    callCount++;
                    throw new Exception("Test Exception");
                });
                Assert.Fail("Should have gotten an exception.");
            } catch (Exception ex) {
                Assert.AreEqual(1, callCount);
                Assert.AreEqual("Test Exception", ex.Message);
            }
        }

        [Test]
        public void TestEncode() {
            Bitmap expectedBitmap = GetBitmapFromResource("Libdmtx.TestImages.Test001.png");
            byte[] data = Encoding.ASCII.GetBytes("Test");
            EncodeOptions opt = new EncodeOptions();
            DmtxEncoded encodeResults = Dmtx.Encode(data, opt);
            Assert.IsNotNull(encodeResults);
            AssertAreEqual(expectedBitmap, encodeResults.Bitmap);
        }

        [Test]
        public void TestVersion() {
            string version = Dmtx.Version;
            Assert.IsNotNull(version);
            Assert.IsTrue(Regex.IsMatch(version, @"[0-9]+\.[0-9]+\.[0-9]+"));
        }

        [Test]
        public void TestBitmapToByteArray() {
            Bitmap bm = new Bitmap(2, 2, PixelFormat.Format24bppRgb);
            bm.SetPixel(0, 0, Color.FromArgb(1, 2, 3));
            bm.SetPixel(1, 0, Color.FromArgb(4, 5, 6));
            bm.SetPixel(0, 1, Color.FromArgb(7, 8, 9));
            bm.SetPixel(1, 1, Color.FromArgb(10, 11, 12));
            byte[] pxl = ExecuteBitmapToByteArray(bm);
            Assert.AreEqual(12, pxl.Length);
            Assert.AreEqual(3, pxl[0]);
            Assert.AreEqual(2, pxl[1]);
            Assert.AreEqual(1, pxl[2]);
            Assert.AreEqual(6, pxl[3]);
            Assert.AreEqual(5, pxl[4]);
            Assert.AreEqual(4, pxl[5]);
            Assert.AreEqual(9, pxl[6]);
            Assert.AreEqual(8, pxl[7]);
            Assert.AreEqual(7, pxl[8]);
            Assert.AreEqual(12, pxl[9]);
            Assert.AreEqual(11, pxl[10]);
            Assert.AreEqual(10, pxl[11]);
        }

        private byte[] ExecuteBitmapToByteArray(Bitmap bm) {
            MethodInfo method = typeof(Dmtx).GetMethod("BitmapToByteArray", BindingFlags.Static | BindingFlags.NonPublic);
            if (method == null) {
                Assert.Fail("Method 'BitmapToByteArray' could not be found.");
            }
            return (byte[])method.Invoke(null, new object[] { bm });
        }

        private static void AssertAreEqual(Bitmap expectedBitmap, Bitmap foundBitmap) {
            Assert.AreEqual(expectedBitmap.Width, foundBitmap.Width, "Bitmap Width");
            Assert.AreEqual(expectedBitmap.Height, foundBitmap.Height, "Bitmap Height");
            for (int y = 0; y < expectedBitmap.Height; y++) {
                for (int x = 0; x < expectedBitmap.Width; x++) {
                    Color expectedPixel = expectedBitmap.GetPixel(x, y);
                    Color foundPixel = foundBitmap.GetPixel(x, y);
                    Assert.AreEqual(expectedPixel, foundPixel, string.Format("Pixel mismatch at location ({0},{1})", x, y));
                }
            }
        }

        private static Bitmap GetBitmapFromResource(string resourceName) {
            Stream stream = typeof(DmtxTest).Assembly.GetManifestResourceStream(resourceName);
            if (stream == null) {
                throw new NullReferenceException(string.Format("Invalid resource \"{0}\".", resourceName));
            }
            Bitmap bm = (Bitmap)Image.FromStream(stream);
            return bm;
        }
    }
}
