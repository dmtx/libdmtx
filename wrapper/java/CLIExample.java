/*
Java wrapper for libdmtx

Copyright (C) 2009 Pete Calvert
Copyright (C) 2009 Mike Laughton
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

import java.awt.image.*;
import javax.imageio.ImageIO;
import java.io.File;
import org.libdmtx.*;

public class CLIExample {
  public static final int SEARCH_TIMEOUT = 10000;
  BufferedImage testImage;
  DMTXTag []tags;

  // Initialization
  public CLIExample(String aFile) {
    long decodingTime = 0;

    try {
      testImage = ImageIO.read(new File(aFile));

      DMTXImage lDImg = new DMTXImage(testImage);

      long startTime = System.currentTimeMillis();
      tags = lDImg.getTags(4, SEARCH_TIMEOUT);
      decodingTime = System.currentTimeMillis() - startTime;
    } catch (Exception e) {
      System.out.println(e);
    }

    if (tags != null) {
      for (DMTXTag tag : tags) {
        System.out.println("Tag Found! Coord: " + tag.corner1.getX() + "," +
            tag.corner1.getY() + " Tag value: " + tag.id);
      }
      System.out.println("Decoding time: " + decodingTime);
    }
    else {
      System.out.println("No tag found");
    }
  }

  public static void main(String []args) {
    new CLIExample(args[0]);
  }
}
