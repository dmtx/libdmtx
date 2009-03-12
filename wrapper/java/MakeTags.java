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

Contact: mike@dragonflylogic.com
*/

/* $Id$ */

import javax.imageio.ImageIO;
import java.io.File;
import java.awt.image.*;
import org.libdmtx.*;

public class MakeTags {
  public static void main(String[] args) throws Exception {
    // Test Tag 1
    ImageIO.write(
      DMTXImage.createTag("Test Tag 1").toBufferedImage(),
      "png",
      new File("tag1.png")
    );

    // Test Tag 2
    ImageIO.write(
      DMTXImage.createTag("Test Tag 2").toBufferedImage(),
      "png",
      new File("tag2.png")
    );

    // Test Tag 3
    ImageIO.write(
      DMTXImage.createTag("Test Tag 3").toBufferedImage(),
      "png",
      new File("tag3.png")
    );
  }
}
