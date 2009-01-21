from pydmtx import DataMatrix
from PIL import Image

# Write a Data Matrix barcode
dm_write = DataMatrix( scheme=DataMatrix.ASCII, module_size=5, margin_size=10 )
dm_write.encode( "hello, world" * 20 )
dm_write.save( "hello.png", "png" )

# Read a Data Matrix barcode
dm_read = DataMatrix( gap_size=10 )
img = Image.open( "hello.png" )
print dm_read.decode( img.size[0], img.size[1], buffer(img.tostring()) )
