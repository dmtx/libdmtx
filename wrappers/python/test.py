from pydmtx import DataMatrix

# Write a Data Matrix barcode
dm_write = DataMatrix( scheme=DataMatrix.ASCII, module_size=5, margin_size=10 )
dm_write.encode( "hello, world" )
dm_write.save( 'hello.png', 'png' )

# Read a Data Matrix barcode
dm_read = DataMatrix( gap_size=20 )
print dm_read.decode( 'hello.png' )
