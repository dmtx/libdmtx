/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (c) 2008 Mike Laughton

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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "dmtx.h"
#include "dmtxstatic.h"

#include "dmtxscangrid.c"
#include "dmtxregion.c"
#include "dmtxdecode.c"
#include "dmtxencode.c"
#include "dmtxplacemod.c"
#include "dmtxreedsol.c"
#include "dmtxsymbol.c"
#include "dmtxvector2.c"
#include "dmtxmatrix3.c"
#include "dmtxcolor3.c"
#include "dmtximage.c"
#include "dmtxcallback.c"
