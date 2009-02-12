using System;
using System.Text;
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
        }

        [Test]
        public void TestEndode() {
            Bitmap expectedBitmap = GetBitmapFromResource("Libdmtx.TestImages.Test001.png");
            byte[] data = Encoding.ASCII.GetBytes("Test");
            EncodeOptions opt = new EncodeOptions();
            DmtxEncoded encodeResults = Dmtx.Encode(data, opt);
            Assert.IsNotNull(encodeResults);
            AssertAreEqual(expectedBitmap, encodeResults.Bitmap);
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
