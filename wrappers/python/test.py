from pydmtx import DataMatrix
from PIL import Image
import time

# Write a Data Matrix barcode
dm_write = DataMatrix( scheme=DataMatrix.ASCII, module_size=5, margin_size=10 )
dm_write.encode( "hello, world" * 20 )
dm_write.save( 'hello.png', 'png' )

# Read a Data Matrix barcode using the original method
dm_read = DataMatrix( gap_size=10 )
timeStart = time.time()
print dm_read.decode( 'hello.png' )
print "Old decode: ", time.time() - timeStart, "s"

# Read a Data Matrix barcode using the newer method
timeStart = time.time()
img = Image.open( 'hello.png' )
print dm_read.decode2( img.size[0], img.size[1], buffer(img.tostring()) )
print "New decode: ", time.time() - timeStart, "s"
