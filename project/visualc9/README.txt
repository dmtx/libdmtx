=================================================================
            libdmtx - Open Source Data Matrix Software
=================================================================

               libdmtx README file (Win32 Binaries)

1. Introduction
-----------------------------------------------------------------

libdmtx is open source software for reading and writing Data
Matrix barcodes of the modern ECC200 variety. The libdmtx-win32
package contains library files necessary for programs to use
libdmtx on Windows, along with the command line utilities
dmtxread.exe and dmtxwrite.exe.

The instructions below will install the libdmtx utilities and
supporting files to your system, after which they can be called
from the DOS or Cygwin prompt.

Alternatively, if you are a software developer hoping to compile
your own programs against libdmtx on Windows then please refer to
the README.visualc, README.mingw, or README.cygwin file in the
main directory of the source package.


2. ImageMagick Installation
-----------------------------------------------------------------

Both dmtxread.exe and dmtxwrite.exe also require the ImageMagick to be
installed on your computer. This package can be downloaded from
either sourceforge.net or imagemagick.org:

  http://downloads.sourceforge.net/imagemagick/ImageMagick-6.4.8-10-Q8-windows-dll.exe


3. libdmtx-win32 Installation
-----------------------------------------------------------------

The libdmtx-win32 package is installed by extracting the package
contents into the ImageMagick directory created above. If you
used the default options in the ImageMagick installer this
location will be:

  C:\Program Files\ImageMagick-6.4.8-Q8

The libdmtx-win32 package contains 5 files:

  * dmtxread.exe    dmtxread executable
  * dmtxwrite.exe   dmtxwrite executable
  * libdmtx.dll     libdmtx Windows DLL (Dynamic-link library)
  * libdmtx.lib     libdmtx Windows import library
  * README.txt      This file

After this you should be able to call dmtxread and dmtxwrite from
the DOS or Cygwin prompt.


4. Feedback
-----------------------------------------------------------------

This is an early version of the libdmtx-win32 package, and it is
still considered experimental. Please consider emailing me at

  mike@dragonflylogic.com

to let me know if it works or doesn't work. I'm curious to know
which versions of Windows this has been used on, and if there are
any problems that we did not anticipate.
