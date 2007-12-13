# pydmtx - Python wrapper for libdmtx
#
# Copyright (c) 2006 Dan Watson
# Copyright (c) 2007 Mike Laughton
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

# $Id: pydmtx.py,v 1.1 2006/10/06 05:29:51 mblaughton Exp $

import _pydmtx
try:
	from PIL import Image, ImageDraw
	_hasPIL = True
except ImportError:
	_hasPIL = False

class DataMatrix (object):
	# scheme: values must be consistent with enum DmtxSchemeEncode
	ASCII     = 1
	C40       = 2
	TEXT      = 3
	X12       = 4
	EDIFACT   = 5
	BASE256   = 6
	BEST_AUTO = 7
	FAST_AUTO = 8

	# shape: values must be consistent with DMTX_SYMBOL_[SQUARE|RECT]_AUTO
	SQUARE_AUTO = -1
	RECT_AUTO   = -2
	# revisit this to enable other (non auto) sizes ...

	def __init__( self, **kwargs ):
		self._data = None
		self._image = None
		self._draw = None

		self.width, self.height = 0, 0

		# defaults
		#self.symbol_color = # how best to specify 3-component color?
		#self.bg_color =     # how best to specify 3-component color?
		self.module_size = 5
		self.margin_size = 10
		self.scheme = self.ASCII
		self.shape = self.SQUARE_AUTO

		self.__dict__.update(kwargs)

	# make these read-only
	data = property( lambda self: self._data )
	image = property( lambda self: self._image )

	# encoding methods
	def encode( self, data ):
		self._data = str(data)
		_pydmtx.encode( self._data, len(self._data), self.module_size,
			self.margin_size, self.scheme, self.shape,
			plotter=self._plot, start=self._start,
			finish=self._finish )

	def save( self, path, fmt ):
		if self._image is not None:
			self._image.save( path, fmt )

	def _start( self, width, height, context ):
		self.width, self.height = width, height
		self._image = Image.new( 'RGB', (width,height), (255,255,255) )
		self._draw = ImageDraw.Draw( self._image )

	def _plot( self, x, y, color, context ):
		self._draw.point( (x,self.height-y-1), fill=color )

	def _finish( self, context ):
		del self._draw
		self._draw = None

	# decoding methods
	def decode( self, path ):
		self._image = Image.open(path)
		self.width, self.height = self._image.size
		return _pydmtx.decode( self.width, self.height, self.gap_size,
			picker=self._pick )

	def _pick( self, x, y, context ):
		return self._image.getpixel( (x,self.height-y-1) )
