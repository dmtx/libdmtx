/*
libdmtx - Data Matrix Encoding/Decoding Library
Copyright (C) 2006 Mike Laughton

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

/* $Id: dmtximage.c,v 1.3 2006-09-25 19:14:19 mblaughton Exp $ */

extern int
dmtxImageInit(DmtxImage *image)
{
   image->width = image->height = 0;
   image->pxl = NULL;

   return 0;
}

extern int
dmtxImageDeInit(DmtxImage *image)
{
   if(image == NULL)
      return 0; // Error

   if(image->pxl != NULL) {
      free(image->pxl);
      image->pxl = NULL;
   }

   return 0;
}

extern int
dmtxImageGetWidth(DmtxImage *image)
{
   if(image == NULL)
      ; // Error

   return image->width;
}

extern int
dmtxImageGetHeight(DmtxImage *image)
{
   if(image == NULL)
      ; // Error

   return image->height;
}

extern int
dmtxImageGetOffset(DmtxImage *image, DmtxDirection dir, int lineNbr, int offset)
{
   return (dir & DmtxDirHorizontal) ? image->width * lineNbr + offset : image->width * offset + lineNbr;
}
