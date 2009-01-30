/*
Java wrapper for libdmtx

Copyright (C) 2009 Pete Calvert

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

Contact: xxxxx@xxx.xx.xx
*/

/* $Id: org_libdmtx_DMTXImage.c 600 2009-01-20 17:51:16Z mblaughton $ */

package org.libdmtx;

import java.awt.Point;

public class DMTXTag {
  /**
   * Integer ID for Tag (although tag represents string, using number simplifies
   * interface with C code)
   */
  public int id;

  /**
   * Corners are numbered from bottom left in an anti-clockwise fashion
   */
  public Point corner1;
  public Point corner2;
  public Point corner3;
  public Point corner4;

  /**
   * Simple constructor (saves a lot of JNI code)
   */
  public DMTXTag(int aID, Point aCorner1, Point aCorner2, Point aCorner3, Point aCorner4) {
    id = aID;
    corner1 = aCorner1;
    corner2 = aCorner2;
    corner3 = aCorner3;
    corner4 = aCorner4;
  }
}
