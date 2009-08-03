/* dmtxCairoWrite
 * Write a libdmtx barcode to a Cairo context.
 *
 * enc: dmtx data to write
 * cr: Cairo context to write to
 * x: horizontal margin
 * y: vertical margin
 * width: width of barcode
 * height: height of barcode
 */
public void dmtxCairoWrite (Dmtx.Encode enc, Cairo.Context cr, double x, double y, double width, double height) {
  Cairo.Matrix old_matrix;
  cr.get_matrix(out old_matrix);

  /* Save a copy of the old matrix so we can restore the setting when we're done. */
  var matrix = old_matrix;

  /* Account for the margin. */
  matrix.translate(x, y);
  /* Allows us to work within the specified area more easily. */
  matrix.scale(width / enc.region.symbol_columns, height / enc.region.symbol_rows);
  cr.set_matrix(matrix);

  for ( int row = 0 ; row < enc.region.symbol_rows ; row++ ) {
    int rowInv = enc.region.symbol_rows - row - 1;
    for ( int col = 0 ; col < enc.region.symbol_columns ; col++ ) {
      var module = enc.message.status(enc.region.size_idx, row, col);
      if ( (module & Dmtx.Module.ON) == Dmtx.Module.ON ) {
        cr.rectangle(col, rowInv, 1, 1);
      }
    }
  }
  cr.fill();

  /* Restore the old settings. */
  cr.set_matrix(old_matrix);
}

public int main (string[] args) {
  /* Create and initialize libdmtx encoding struct */
  var enc = new Dmtx.Encode();
  if ( enc == null )
    GLib.error("create error");

  if ( args.length != 3 ) {
    GLib.stderr.printf("Usage: %s \"Text to encode\" /path/to/output.pdf\n", args[0]);
    return -1;
  }

  /* Set output image properties */
  enc.set_property(Dmtx.Property.PIXEL_PACKING, Dmtx.PackOrder.24BPP_RGB);
  enc.set_property(Dmtx.Property.IMAGE_FLIP, Dmtx.Flip.NONE);
  enc.set_property(Dmtx.Property.ROW_PAD_BYTES, 0);

  /* Set encoding options */
  enc.set_property(Dmtx.Property.MARGIN_SIZE, 10);
  enc.set_property(Dmtx.Property.MODULE_SIZE, 5);
  enc.set_property(Dmtx.Property.SCHEME, Dmtx.Scheme.ASCII);
  enc.set_property(Dmtx.Property.SIZE_REQUEST, Dmtx.SymbolSize.SQUARE_AUTO);

  unowned uchar[] data = (uchar[])args[1];
  data.length = (int)args[1].size();

  if ( !enc.data_matrix(data) )
    GLib.error("Unable to encode message (possibly too large for requested size).");

  /* Create a 60 mm x 60 mm surface to draw on. */
  double[] surface_size = { 170.07874, 170.07874 };
  var surface = new Cairo.PdfSurface(args[2], surface_size[0], surface_size[1]);
  var cr = new Cairo.Context(surface);

  /* Encode */
  dmtxCairoWrite(enc, cr, surface_size[0] * 0.25, surface_size[1] * 0.25, surface_size[0] * 0.5, surface_size[0] * 0.5);

  return 0;
}