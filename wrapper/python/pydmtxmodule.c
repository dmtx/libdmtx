/*
pydmtx - Python wrapper for libdmtx

Copyright (C) 2006 Dan Watson
Copyright (C) 2008, 2009 Mike Laughton
Copyright (C) 2008 Jonathan Lung
Copyright (C) 2009 David Turner

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

#include <string.h>
#include <Python.h>
#include <dmtx.h>

/* Define Py_ssize_t for earlier Python versions */
#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

static PyObject *dmtx_encode(PyObject *self, PyObject *args, PyObject *kwargs);
static PyObject *dmtx_decode(PyObject *self, PyObject *args, PyObject *kwargs);

static PyMethodDef dmtxMethods[] = {
   { "encode",
     (PyCFunction)dmtx_encode,
     METH_VARARGS | METH_KEYWORDS,
     "Encodes data into a matrix and calls back to plot." },
   { "decode",
     (PyCFunction)dmtx_decode,
     METH_VARARGS | METH_KEYWORDS,
     "Decodes data from a bitmap stored in a buffer and returns the encoded data." },
   { NULL,
     NULL,
     0,
     NULL }
};

static PyObject *dmtx_encode(PyObject *self, PyObject *arglist, PyObject *kwargs)
{
   const unsigned char *data;
   int data_size;
   int module_size;
   int margin_size;
   int scheme;
   int shape;
   PyObject *plotter = NULL;
   PyObject *start_cb = NULL;
   PyObject *finish_cb = NULL;
   PyObject *context = Py_None;
   PyObject *args;
   DmtxEncode *enc;
   int row, col;
   int rgb[3];
   static char *kwlist[] = { "data", "data_size", "module_size", "margin_size",
                             "scheme", "shape", "plotter", "start", "finish",
                             "context", NULL };

   if(!PyArg_ParseTupleAndKeywords(arglist, kwargs, "siiiii|OOOO", kwlist,
         &data, &data_size, &module_size, &margin_size, &scheme, &shape,
         &plotter, &start_cb, &finish_cb, &context))
      return NULL;

   Py_INCREF(context);

   /* Plotter is required, and must be callable */
   if(plotter == NULL || !PyCallable_Check(plotter)) {
      PyErr_SetString(PyExc_TypeError, "plotter must be callable");
      return NULL;
   }

   enc = dmtxEncodeCreate();
   if(enc == NULL)
      return NULL;

   dmtxEncodeSetProp(enc, DmtxPropScheme, scheme);
   dmtxEncodeSetProp(enc, DmtxPropSizeRequest, shape);
   dmtxEncodeSetProp(enc, DmtxPropImageFlip, DmtxFlipY);
   dmtxEncodeSetProp(enc, DmtxPropMarginSize, margin_size);
   dmtxEncodeSetProp(enc, DmtxPropModuleSize, module_size);

   dmtxEncodeDataMatrix(enc, data_size, (unsigned char *)data);

   if((start_cb != NULL) && PyCallable_Check(start_cb)) {
      args = Py_BuildValue("(iiO)", enc->image->width, enc->image->height, context);
      (void)PyEval_CallObject(start_cb, args);
      Py_DECREF(args);
   }

   for(row = 0; row < enc->image->height; row++) {
      for(col = 0; col < enc->image->width; col++) {
         dmtxImageGetPixelValue(enc->image, col, row, 0, &rgb[0]);
         dmtxImageGetPixelValue(enc->image, col, row, 1, &rgb[1]);
         dmtxImageGetPixelValue(enc->image, col, row, 2, &rgb[2]);
         args = Py_BuildValue("(ii(iii)O)", col, row, rgb[0], rgb[1], rgb[2], context);
         (void)PyEval_CallObject(plotter, args);
         Py_DECREF(args);
      }
   }

   if((finish_cb != NULL) && PyCallable_Check(finish_cb)) {
      args = Py_BuildValue("(O)", context);
      (void)PyEval_CallObject(finish_cb, args);
      Py_DECREF(args);
   }

   dmtxEncodeDestroy(&enc);
   Py_DECREF(context);

   return Py_None;
}

static PyObject *dmtx_decode(PyObject *self, PyObject *arglist, PyObject *kwargs)
{
   int width;
   int height;
   int gap_size;

   PyObject *dataBuffer = NULL;
   Py_ssize_t dataLen;
   PyObject *context = Py_None;
   PyObject *output = NULL;
   DmtxImage *img;
   DmtxDecode *dec;
   DmtxRegion *reg;
   DmtxMessage *msg;
   const char *pxl;  /* Input image buffer */

   static char *kwlist[] = { "width", "height", "gap_size", "data", "context", NULL };

   /* Get parameters from Python for libdmtx */
   if(!PyArg_ParseTupleAndKeywords(arglist, kwargs, "iii|OO", kwlist,
         &width, &height, &gap_size, &dataBuffer, &context)) {
      PyErr_SetString(PyExc_TypeError, "decode takes at least 3 arguments");
      return NULL;
   }

   Py_INCREF(context);

   if(dataBuffer == NULL) {
      PyErr_SetString(PyExc_TypeError, "Interleaved bitmapped data in buffer missing");
      return NULL;
   }

   PyObject_AsCharBuffer(dataBuffer, &pxl, &dataLen);

   img = dmtxImageCreate((unsigned char *)pxl, width, height, 24, DmtxPackRGB, DmtxFlipY);
   if(img == NULL)
      return NULL;

   dec = dmtxDecodeCreate(img);
   if(dec == NULL) {
      dmtxImageDestroy(&img);
      return NULL;
   }

   dmtxDecodeSetProp(dec, DmtxPropScanGap, gap_size);

   for(;;) {
      reg = dmtxRegionFindNext(dec, NULL);
      if(reg != NULL) {
         msg = dmtxDecodeMatrixRegion(dec, reg, -1);
         if(msg != NULL) {
            output = Py_BuildValue("s", msg->output);
            Py_INCREF(output);
            dmtxMessageDestroy(&msg);
         }
         dmtxRegionDestroy(&reg);
      }
      break; /* XXX for now, break after first barcode is found in image */
   }

   dmtxDecodeDestroy(&dec);
   dmtxImageDestroy(&img);
   Py_DECREF(context);
   if(output == NULL) {
      Py_INCREF(Py_None);
      return Py_None;
   }

   return output;
}

PyMODINIT_FUNC init_pydmtx(void)
{
   (void)Py_InitModule("_pydmtx", dmtxMethods);
}

int main(int argc, char *argv[])
{
   Py_SetProgramName(argv[0]);
   Py_Initialize();
   init_pydmtx();
   return 0;
}
