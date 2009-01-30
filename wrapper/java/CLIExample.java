import java.awt.image.*;
import javax.imageio.ImageIO;
import java.io.File;
import org.libdmtx.*;

public class CLIExample {
  BufferedImage testImage;
  DMTXTag []tags;

  // Initialisation
  public CLIExample(String aFile) {

    try {
      testImage = ImageIO.read(new File(aFile));

      DMTXImage lDImg = new DMTXImage(testImage);

      tags = lDImg.getTags(4);
    } catch (Exception e) {
      System.out.println(e);
    }

    System.out.println("Hello World!");
  }

  public static void main(String []args) {
    new CLIExample(args[0]);
  }
}
