from pydmtx import DataMatrix
from PIL import Image

# Write a Data Matrix barcode
dm_write = DataMatrix()
dm_write.encode("Hello, world!")
dm_write.save("hello.png", "png")

# Read a Data Matrix barcode
dm_read = DataMatrix()
img = Image.open("hello.png")

print dm_read.decode(img.size[0], img.size[1], buffer(img.tostring()))
print dm_read.count()
print dm_read.message(1)
print dm_read.stats(1)
