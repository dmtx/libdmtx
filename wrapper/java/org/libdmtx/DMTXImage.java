/*
Java wrapper for libdmtx

Copyright (C) 2009 Pete Calvert
Copyright (C) 2009 Dikran Seropian

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

package org.libdmtx;

import java.awt.image.BufferedImage;

public class DMTXImage {
  /**
   * Load external library
   */
  static {
    System.loadLibrary("dmtx");
  }

  /**
   * Image Data - 1 'int' per pixel, packed with RGB (8bits each, 8bits padding)
   */
  public int[] data;

  /**
   * Image Dimensions
   */
  public int width;
  public int height;

  /**
   * Simple constructor from raw data (saves on JNI code)
   */
  public DMTXImage(int aWidth, int aHeight, int[] aData) {
    width  = aWidth;
    height = aHeight;
    data   = aData;
  }

  /**
   * Construct from BufferedImage
   */
  public DMTXImage(BufferedImage aImage) {
    width  = aImage.getWidth();
    height = aImage.getHeight();
    data   = aImage.getRGB(0, 0, width, height, null, 0, width);
  }

  /**
   * Construct from ID (static factory method since JNI doesn't allow native
   * constructors).
   */
  public static native DMTXImage createTag(String aID);

  /**
   * Decode the image, returning tags found (as DMTXTag objects)
   */
  public native DMTXTag[] getTags(int aMaxTagCount, int searchTimeout);

  /**
   * Generate a BufferedImage from the image and return it
   */
  public BufferedImage toBufferedImage() {
    BufferedImage lReturn;

    lReturn = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);

    lReturn.setRGB(0, 0, width, height, data, 0, width);

    return lReturn;
  }
}
