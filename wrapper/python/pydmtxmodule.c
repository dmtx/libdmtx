/*
pydmtx - Python wrapper for libdmtx

Copyright (C) 2006 Dan Watson
Copyright (C) 2008, 2009 Mike Laughton
Copyright (C) 2008 Jonathan Lung
Copyright (C) 2009 David Turner
Copyright (C) 2009 Simon Wood

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

static PyObject *
dmtx_encode(PyObject *self, PyObject *arglist, PyObject *kwargs)
{
   const unsigned char *data;
   int count=0;
   int data_size;
   int module_size = DmtxUndefined;
   int margin_size = DmtxUndefined;
   int scheme = DmtxUndefined;
   int shape = DmtxUndefined;

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

   /* Parse out the options which are applicable */
   PyObject *filtered_kwargs;
   filtered_kwargs = PyDict_New();
   count = 2; /* Skip the first 2 keywords as they are sent in arglist */
   while(kwlist[count]){
      if(PyDict_GetItemString(kwargs, kwlist[count])) {
         PyDict_SetItemString(filtered_kwargs, kwlist[count],PyDict_GetItemString(kwargs, kwlist[count]));
      }
      count++;
   }

   if(!PyArg_ParseTupleAndKeywords(arglist, filtered_kwargs, "siiiii|OOOO", kwlist,
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

   dmtxEncodeSetProp(enc, DmtxPropPixelPacking, DmtxPack24bppRGB);
   dmtxEncodeSetProp(enc, DmtxPropImageFlip, DmtxFlipNone);

   if(scheme != DmtxUndefined)
      dmtxEncodeSetProp(enc, DmtxPropScheme, scheme);

   if(shape != DmtxUndefined)
      dmtxEncodeSetProp(enc, DmtxPropSizeRequest, shape);

   if(margin_size != DmtxUndefined)
      dmtxEncodeSetProp(enc, DmtxPropMarginSize, margin_size);

   if(module_size != DmtxUndefined)
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

static PyObject *
dmtx_decode(PyObject *self, PyObject *arglist, PyObject *kwargs)
{
   int count=0;
   int found=0;
   int width;
   int height;
   int gap_size = DmtxUndefined;
   int max_count = DmtxUndefined;
   int timeout = DmtxUndefined;
   int shape = DmtxUndefined;
   int deviation = DmtxUndefined;
   int threshold = DmtxUndefined;
   int shrink = 1;
   int corrections = DmtxUndefined;
   int min_edge = DmtxUndefined;
   int max_edge = DmtxUndefined;

   PyObject *dataBuf = NULL;
   Py_ssize_t dataLen;
   PyObject *context = Py_None;
   PyObject *output = PyList_New(0);

   DmtxTime dmtx_timeout;
   DmtxImage *img;
   DmtxDecode *dec;
   DmtxRegion *reg;
   DmtxMessage *msg;
   DmtxVector2 p00, p10, p11, p01;
   const char *pxl; /* Input image buffer */

   static char *kwlist[] = { "width", "height", "data", "gap_size", "max_count", "context", "timeout", "shape", "deviation", "threshold", "shrink", "corrections", "min_edge", "max_edge", NULL };

   /* Parse out the options which are applicable */
   PyObject *filtered_kwargs;
   filtered_kwargs = PyDict_New();
   count = 3; /* Skip the first 3 keywords as they are sent in arglist */
   while(kwlist[count]){
      if(PyDict_GetItemString(kwargs, kwlist[count])) {
         PyDict_SetItemString(filtered_kwargs, kwlist[count],PyDict_GetItemString(kwargs, kwlist[count]));
      }
      count++;
   }

   /* Get parameters from Python for libdmtx */
   if(!PyArg_ParseTupleAndKeywords(arglist, filtered_kwargs, "iiOi|iOiiiiiiii", kwlist,
         &width, &height, &dataBuf, &gap_size, &max_count, &context, &timeout, &shape, &deviation, &threshold, &shrink, &corrections, &min_edge, &max_edge)) {
      PyErr_SetString(PyExc_TypeError, "decode takes at least 3 arguments");
      return NULL;
   }

   Py_INCREF(context);

   /* Reset timeout for each new page */
   if(timeout != DmtxUndefined)
      dmtx_timeout = dmtxTimeAdd(dmtxTimeNow(), timeout);

   if(dataBuf == NULL) {
      PyErr_SetString(PyExc_TypeError, "Interleaved bitmapped data in buffer missing");
      return NULL;
   }

   PyObject_AsCharBuffer(dataBuf, &pxl, &dataLen);

   img = dmtxImageCreate((unsigned char *)pxl, width, height, DmtxPack24bppRGB);
   if(img == NULL)
      return NULL;

   dec = dmtxDecodeCreate(img, shrink);
   if(dec == NULL) {
      dmtxImageDestroy(&img);
      return NULL;
   }

   if(gap_size != DmtxUndefined)
      dmtxDecodeSetProp(dec, DmtxPropScanGap, gap_size);

   if(shape != DmtxUndefined)
      dmtxDecodeSetProp(dec, DmtxPropSymbolSize, shape);

   if(deviation != DmtxUndefined)
      dmtxDecodeSetProp(dec, DmtxPropSquareDevn, deviation);

   if(threshold != DmtxUndefined)
      dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, threshold);

   if(min_edge != DmtxUndefined)
      dmtxDecodeSetProp(dec, DmtxPropEdgeMin, min_edge);

   if(max_edge != DmtxUndefined)
      dmtxDecodeSetProp(dec, DmtxPropEdgeMax, max_edge);

   for(count=1; ;count++) {
      Py_BEGIN_ALLOW_THREADS
      if(timeout == DmtxUndefined)
         reg = dmtxRegionFindNext(dec, NULL);
      else
         reg = dmtxRegionFindNext(dec, &dmtx_timeout);
      Py_END_ALLOW_THREADS

      /* Finished file or ran out of time before finding another region */
      if(reg == NULL)
         break;

      msg = dmtxDecodeMatrixRegion(dec, reg, corrections);
      if(msg != NULL) {
         p00.X = p00.Y = p10.Y = p01.X = 0.0;
         p10.X = p01.Y = p11.X = p11.Y = 1.0;
         dmtxMatrix3VMultiplyBy(&p00, reg->fit2raw);
         dmtxMatrix3VMultiplyBy(&p10, reg->fit2raw);
         dmtxMatrix3VMultiplyBy(&p11, reg->fit2raw);
         dmtxMatrix3VMultiplyBy(&p01, reg->fit2raw);

         PyList_Append(output, Py_BuildValue("s((ii)(ii)(ii)(ii))", msg->output,
               (int)((shrink * p00.X) + 0.5), height - 1 - (int)((shrink * p00.Y) + 0.5),
               (int)((shrink * p10.X) + 0.5), height - 1 - (int)((shrink * p10.Y) + 0.5),
               (int)((shrink * p11.X) + 0.5), height - 1 - (int)((shrink * p11.Y) + 0.5),
               (int)((shrink * p01.X) + 0.5), height - 1 - (int)((shrink * p01.Y) + 0.5)));

         Py_INCREF(output);
         dmtxMessageDestroy(&msg);
         found++;
      }

      dmtxRegionDestroy(&reg);

      /* Stop if we've reached maximium count */
      if(max_count != DmtxUndefined)
         if(found >= max_count) break;
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

PyMODINIT_FUNC
init_pydmtx(void)
{
   (void)Py_InitModule("_pydmtx", dmtxMethods);
}

int
main(int argc, char *argv[])
{
   Py_SetProgramName(argv[0]);
   Py_Initialize();
   init_pydmtx();
   return 0;
}
