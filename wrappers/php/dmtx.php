<?php
	$dmtx = dmtx_write($_GET["d"]);

	$size = dmtx_getSize($dmtx);
	$gd = imagecreatetruecolor($size['width'], $size['height']);

	$bg = imagecolorallocate($gd, 255, 255, 255);
	$fg = imagecolorallocate($gd, 0, 0, 0);

	$line = 0;
	while (($row = dmtx_getRow($dmtx)) != null) {

		for ($x = count($row) - 1; $x >= 0; $x--) {
			if ($row[$x]['R'] == 0)
				imagesetpixel($gd, $x, $line, $fg);
			else
				imagesetpixel($gd, $x, $line, $bg);
		}
		$line++;
	}

	$output_image = $gd;

	header("Content-type: image/png");
	imagepng($output_image);
?>
