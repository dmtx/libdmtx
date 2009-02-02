/*
Rdmtx - Ruby wrapper for libdmtx

Copyright (C) 2008 Romain Goyet

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

Contact: r.goyet@gmail.com
*/

/* $Id:$ */

#include <ruby.h>
#include <dmtx.h>

static VALUE rdmtx_init(VALUE self) {
    return self;
}

static VALUE rdmtx_decode(VALUE self, VALUE image /* Image from RMagick (Magick::Image) */, VALUE timeout /* Timeout in msec */) {

    VALUE rawImageString = rb_funcall(image, rb_intern("export_pixels_to_str"), 0);

    VALUE safeImageString = StringValue(rawImageString);

    char * imageBuffer = RSTRING(safeImageString)->ptr;

    int width = NUM2INT(rb_funcall(image, rb_intern("columns"), 0));
    int height = NUM2INT(rb_funcall(image, rb_intern("rows"), 0));

    DmtxImage * dmtxImage = dmtxImageCreate((unsigned char *)imageBuffer, width, height, 24, DmtxPackRGB);
    dmtxImageSetProp(dmtxImage, DmtxPropScaledXmin, 0);
    dmtxImageSetProp(dmtxImage, DmtxPropScaledXmax, width);
    dmtxImageSetProp(dmtxImage, DmtxPropScaledYmin, 0);
    dmtxImageSetProp(dmtxImage, DmtxPropScaledYmax, height);

    VALUE results = rb_ary_new();

    /* Initialize decode struct for newly loaded image */
    DmtxDecode * decode = dmtxDecodeCreate(dmtxImage);

    DmtxRegion * region;

    int intTimeout = NUM2INT(timeout);
    DmtxTime dmtxTimeout = dmtxTimeAdd(dmtxTimeNow(), intTimeout);

    for(;;) {
        if (intTimeout == 0) {
            region = dmtxRegionFindNext(decode, NULL);
        } else {
            region = dmtxRegionFindNext(decode, &dmtxTimeout);
        }

        if (region == NULL )
            break;

        DmtxMessage * message = dmtxDecodeMatrixRegion(decode, region, -1);
        if (message != NULL) {
            VALUE outputString = rb_str_new2((char *)message->output);
            rb_ary_push(results, outputString);
            dmtxMessageDestroy(&message);
        }

        dmtxRegionDestroy(&region);
    }

    dmtxDecodeDestroy(&decode);
    dmtxImageDestroy(&dmtxImage);

    return results;
}

static VALUE rdmtx_encode(VALUE self, VALUE string) {

    /* Create and initialize libdmtx structures */
    DmtxEncode * enc = dmtxEncodeCreate();

    VALUE safeString = StringValue(string);

    dmtxEncodeSetProp(enc, DmtxPropSizeRequest, DmtxSymbolSquareAuto);

    /* Create barcode image */
    if (dmtxEncodeDataMatrix(enc, RSTRING(safeString)->len,
            (unsigned char *)RSTRING(safeString)->ptr) == DmtxFail) {
//        printf("Fatal error !\n");
        dmtxEncodeDestroy(&enc);
        return Qnil;
    }

    int width = dmtxImageGetProp(enc->image, DmtxPropWidth);
    int height = dmtxImageGetProp(enc->image, DmtxPropHeight);

    VALUE magickImageClass = rb_path2class("Magick::Image");
    VALUE outputImage = rb_funcall(magickImageClass, rb_intern("new"), 2, INT2NUM(width), INT2NUM(height));

    rb_funcall(outputImage, rb_intern("import_pixels"), 7,
               INT2NUM(0),
               INT2NUM(0),
               INT2NUM(width),
               INT2NUM(height),
               rb_str_new("RGB", 3),
               rb_str_new((char *)enc->image->pxl, 3*width*height),
//               rb_const_get("Magick" ,rb_intern("CharPixel"))
               rb_eval_string("Magick::CharPixel"));

    /* Clean up */
    dmtxEncodeDestroy(&enc);

    return outputImage;
}

VALUE cRdmtx;
void Init_Rdmtx() {
    cRdmtx = rb_define_class("Rdmtx", rb_cObject);
    rb_define_method(cRdmtx, "initialize", rdmtx_init, 0);
    rb_define_method(cRdmtx, "decode", rdmtx_decode, 2);
    rb_define_method(cRdmtx, "encode", rdmtx_encode, 1);
}
