using System;
using System.Collections.Generic;
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
