/*
pydmtx - Python wrapper for libdmtx

Copyright (c) 2006 Dan Watson
Copyright (c) 2008 Mike Laughton
Copyright (c) 2008 Jonathan Lung

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
static PyObject *dmtx_decode2(PyObject *self, PyObject *args, PyObject *kwargs);

static PyMethodDef dmtxMethods[] = {
   { "encode",
     (PyCFunction)dmtx_encode,
     METH_VARARGS | METH_KEYWORDS,
     "Encodes data into a matrix and calls back to plot." },
   { "decode",
     (PyCFunction)dmtx_decode,
     METH_VARARGS | METH_KEYWORDS,
     "Decodes data from a matrix and returns data." },
   { "decode2",
     (PyCFunction)dmtx_decode2,
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
   DmtxEncode enc;
   int row, col;
   DmtxRgb rgb;
   static char *kwlist[] = { "data", "data_size", "module_size", "margin_size",
                             "scheme", "shape", "plotter", "start", "finish",
                             "context", NULL };

   if(!PyArg_ParseTupleAndKeywords(arglist, kwargs, "siiiii|OOOO", kwlist,
         &data, &data_size, &module_size, &margin_size, &scheme, &shape,
         &plotter, &start_cb, &finish_cb, &context))
      return NULL;

   Py_INCREF(context);

   /* Plotter is required, and must be callable */
   if((plotter == NULL) || !PyCallable_Check(plotter)) {
      PyErr_SetString(PyExc_TypeError, "plotter must be callable");
      return NULL;
   }

   enc = dmtxEncodeStructInit();
   enc.moduleSize = module_size;
   enc.marginSize = margin_size;
   enc.scheme = scheme;

   dmtxEncodeDataMatrix(&enc, data_size, (unsigned char *)data, shape);

   if((start_cb != NULL) && PyCallable_Check(start_cb)) {
      args = Py_BuildValue("(iiO)", enc.image->width, enc.image->height, context);
      (void)PyEval_CallObject(start_cb, args);
      Py_DECREF(args);
   }

   for(row = 0; row < enc.image->height; row++) {
      for(col = 0; col < enc.image->width; col++) {
         dmtxImageGetRgb(enc.image, col, row, rgb);
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

   dmtxEncodeStructDeInit(&enc);
   Py_DECREF(context);

   return Py_None;
}

static PyObject *dmtx_decode(PyObject *self, PyObject *arglist, PyObject *kwargs)
{
   int width;
   int height;
   int gap_size;
   PyObject *picker = NULL;
   PyObject *context = Py_None;
   PyObject *args;
   PyObject *output = NULL;
   DmtxImage *img;
   DmtxDecode dec;
   DmtxRegion reg;
   DmtxMessage *msg;
   PyObject *pilPixel;
   DmtxRgb dmtxRgb;
   int x, y;
   static char *kwlist[] = { "width", "height", "gap_size", "picker", "context", NULL };

   /* Individual pixel picker is probably a slow way to do this */
   if(!PyArg_ParseTupleAndKeywords(arglist, kwargs, "iii|OO", kwlist,
         &width, &height, &gap_size, &picker, &context))
      return NULL;

   Py_INCREF(context);

   /* Picker is required, and must be callable */
   if((picker == NULL) || !PyCallable_Check(picker)) {
      PyErr_SetString(PyExc_TypeError, "picker must be callable");
      return NULL;
   }

   img = dmtxImageMalloc(width, height);

   /* Populate libdmtx image with PIL image data */
   for(y = 0; y < img->height; y++) {
      for(x = 0; x < img->width; x++) {
         args = Py_BuildValue("(iiO)", x, y, context);
         pilPixel = PyEval_CallObject(picker, args);
         Py_DECREF(args);

         if(pilPixel == NULL)
            return NULL;

         if(!PyArg_ParseTuple(pilPixel, "iii", &dmtxRgb[0], &dmtxRgb[1], &dmtxRgb[2]))
            return NULL;

         dmtxImageSetRgb(img, x, y, dmtxRgb);

         Py_DECREF(pilPixel);
      }
   }

   dec = dmtxDecodeStructInit(img);
   dmtxDecodeSetProp(&dec, DmtxPropScanGap, gap_size);

   for(;;) {
      reg = dmtxDecodeFindNextRegion(&dec, NULL);

      if(reg.found != DMTX_REGION_FOUND)
         break;

      msg = dmtxDecodeMatrixRegion(img, &reg, -1);
      if(msg == NULL)
         continue;

      output = Py_BuildValue("s", msg->output);
      Py_INCREF(output);

      dmtxMessageFree(&msg);
      break; /* XXX for now, break after first barcode is found in image */
   }

   dmtxImageFree(&img);
   Py_DECREF(context);

   return output;
}

static PyObject *dmtx_decode2(PyObject *self, PyObject *arglist, PyObject *kwargs)
{
   int width;  // Width of image
   int height;  // Height of image
   int gap_size;

   PyObject *dataBuffer = NULL;
   Py_ssize_t dataLen;
   PyObject *context = Py_None;
   PyObject *output = NULL;
   DmtxImage *img;
   char *imageAsPtrArray;  // The libdmtx array for the image
   DmtxDecode dec;
   DmtxRegion reg;
   DmtxMessage *msg;
   const char *pixelData;  // The input image buffer

   static char *kwlist[] = { "width", "height", "gap_size", "data", "context", NULL };

   // Get parameters from Python for libdmtx.
   if(!PyArg_ParseTupleAndKeywords(arglist, kwargs, "iii|OO", kwlist,
         &width, &height, &gap_size, &dataBuffer, &context))
      return NULL;

   Py_INCREF(context);

   if((dataBuffer == NULL)) {
      PyErr_SetString(PyExc_TypeError, "Interleaved bitmapped data in buffer missing");
      return NULL;
   }

   img = dmtxImageMalloc(width, height);
   imageAsPtrArray = (char*) &img->pxl[0][0];

   PyObject_AsCharBuffer(dataBuffer, &pixelData, &dataLen);

   /* Populate libdmtx image with image data */
   memcpy(img->pxl, pixelData, img->width * img->height * 3);

   dec = dmtxDecodeStructInit(img);
   dmtxDecodeSetProp(&dec, DmtxPropScanGap, gap_size);

   for(;;) {
      reg = dmtxDecodeFindNextRegion(&dec, NULL);

      if(reg.found != DMTX_REGION_FOUND)
         break;

      msg = dmtxDecodeMatrixRegion(img, &reg, -1);
      if(msg == NULL)
         continue;

      output = Py_BuildValue("s", msg->output);
      Py_INCREF(output);

      dmtxMessageFree(&msg);
      break; /* XXX for now, break after first barcode is found in image */
   }

   dmtxImageFree(&img);
   Py_DECREF(context);

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
