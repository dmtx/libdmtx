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

/* $Id$ */

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import javax.imageio.ImageIO;
import java.io.File;
import org.libdmtx.*;

public class GUIExample extends Frame {
  BufferedImage testImage;
  DMTXTag []tags;

  // Initialisation
  public GUIExample(String aFile) {
    super("DMTX Test");

    try {
      testImage = ImageIO.read(new File(aFile));

      DMTXImage lDImg = new DMTXImage(testImage);

      setSize(lDImg.width, lDImg.height);

      tags = lDImg.getTags(4);
    } catch (Exception e) {
      System.out.println(e);
    }

    setVisible(true);
  }

  // Paint Method
  public void paint(Graphics aGraphics) {
    aGraphics.drawImage(testImage, 0, 0, this);

    int lXMin = tags[0].corner1.x;
    int lXMax = tags[0].corner1.x;
    int lYMin = tags[0].corner1.y;
    int lYMax = tags[0].corner1.y;

    for(DMTXTag lTag : tags) {
      aGraphics.setColor(Color.red);
      aGraphics.drawLine(lTag.corner1.x, lTag.corner1.y, lTag.corner2.x, lTag.corner2.y);
      aGraphics.setColor(Color.blue);
      aGraphics.drawLine(lTag.corner2.x, lTag.corner2.y, lTag.corner3.x, lTag.corner3.y);
      aGraphics.drawLine(lTag.corner3.x, lTag.corner3.y, lTag.corner4.x, lTag.corner4.y);
      aGraphics.drawLine(lTag.corner4.x, lTag.corner4.y, lTag.corner1.x, lTag.corner1.y);

      lXMin = Math.min(lXMin, lTag.corner1.x);
      lXMin = Math.min(lXMin, lTag.corner2.x);
      lXMin = Math.min(lXMin, lTag.corner3.x);
      lXMin = Math.min(lXMin, lTag.corner4.x);

      lXMax = Math.max(lXMax, lTag.corner1.x);
      lXMax = Math.max(lXMax, lTag.corner2.x);
      lXMax = Math.max(lXMax, lTag.corner3.x);
      lXMax = Math.max(lXMax, lTag.corner4.x);

      lYMin = Math.min(lYMin, lTag.corner1.y);
      lYMin = Math.min(lYMin, lTag.corner2.y);
      lYMin = Math.min(lYMin, lTag.corner3.y);
      lYMin = Math.min(lYMin, lTag.corner4.y);

      lYMax = Math.max(lYMax, lTag.corner1.y);
      lYMax = Math.max(lYMax, lTag.corner2.y);
      lYMax = Math.max(lYMax, lTag.corner3.y);
      lYMax = Math.max(lYMax, lTag.corner4.y);
    }

    aGraphics.setColor(Color.green);
    aGraphics.drawRect(lXMin, lYMin, lXMax - lXMin, lYMax - lYMin);
  }

  public static void main(String []args) {
    new GUIExample(args[0]);
  }
}
