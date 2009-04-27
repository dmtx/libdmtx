# pydmtx - Python wrapper for libdmtx
#
# Copyright (C) 2006 Dan Watson
# Copyright (C) 2007, 2008, 2009 Mike Laughton
# Copyright (C) 2008 Jonathan Lung
# Copyright (C) 2009 Simon Wood
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

# $Id$

import _pydmtx
try:
	from PIL import Image, ImageDraw
	_hasPIL = True
except ImportError:
	_hasPIL = False


class DataMatrix (object):
	DmtxUndefined = -1

	# Scheme: values must be consistent with enum DmtxScheme
	DmtxSchemeAutoFast = -2
	DmtxSchemeAutoBest = -1
	DmtxSchemeAscii    =  0
	DmtxSchemeC40      =  1
	DmtxSchemeText     =  2
	DmtxSchemeX12      =  3
	DmtxSchemeEdifact  =  4
	DmtxSchemeBase256  =  5

	# Shape: values must be consistent with DmtxSymbol[Square|Rect]Auto
	DmtxSymbolRectAuto   = -3
	DmtxSymbolSquareAuto = -2
	DmtxSymbolShapeAuto  = -1
	DmtxSymbol10x10      =  0
	DmtxSymbol12x12      =  1
	DmtxSymbol14x14      =  2
	DmtxSymbol16x16      =  3
	DmtxSymbol18x18      =  4
	DmtxSymbol20x20      =  5
	DmtxSymbol22x22      =  6
	DmtxSymbol24x24      =  7
	DmtxSymbol26x26      =  8
	DmtxSymbol32x32      =  9
	DmtxSymbol36x36      =  10
	DmtxSymbol40x40      =  11
	DmtxSymbol44x44      =  12
	DmtxSymbol48x48      =  13
	DmtxSymbol52x52      =  14
	DmtxSymbol64x64      =  15
	DmtxSymbol72x72      =  16
	DmtxSymbol80x80      =  17
	DmtxSymbol88x88      =  18
	DmtxSymbol96x96      =  19
	DmtxSymbol104x104    =  20
	DmtxSymbol120x120    =  21
	DmtxSymbol132x132    =  22
	DmtxSymbol144x144    =  23
	DmtxSymbol8x18       =  24
	DmtxSymbol8x32       =  25
	DmtxSymbol12x26      =  26
	DmtxSymbol12x36      =  27
	DmtxSymbol16x36      =  28
	DmtxSymbol16x48      =  29

	def __init__( self, **kwargs ):
		self._data = None
		self._image = None
		self._draw = None

		self.width, self.height = 0, 0

		self.results = ""

		# defaults (only set mandatory values)
		self.options = {
			'module_size' : self.DmtxUndefined,
			'margin_size' : self.DmtxUndefined,
			'gap_size' : self.DmtxUndefined,
			'scheme' : self.DmtxUndefined,
			'shape' : self.DmtxUndefined
		}

		self.options.update(kwargs)

	# make these read-only
	data = property( lambda self: self._data )
	image = property( lambda self: self._image )

	# encoding methods
	def encode( self, data, **kwargs ):
		all_kwargs = self.options
		all_kwargs.update(kwargs)

		self._data = str(data)
		_pydmtx.encode( self._data, len(self._data),
			plotter=self._plot, start=self._start,
			finish=self._finish,
			**all_kwargs );

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

	def decode( self, width, height, data, **kwargs):
		all_kwargs = self.options
		all_kwargs.update(kwargs)

		self.results =  _pydmtx.decode( width, height, data, **all_kwargs)

		# return only the first message
		return self.message(1)

	def count( self ):
		return len(self.results)

	def message( self, ref ):
		if (ref <= self.count() and ref > 0):
			barcode = self.results[ref-1]
			return barcode[0]
		else:
			return

	def stats( self, ref ):
		if (ref <= self.count() and ref > 0):
			return self.results[ref-1]
		else:
			return
