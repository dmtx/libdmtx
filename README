=================================================================
            libdmtx - Open Source Data Matrix Software
=================================================================

               libdmtx README file (all platforms)

This summary of the libdmtx package applies generally to all
platforms. For instructions regarding your specific platform,
also see the README.xxx file in this directory that matches your
system (e.g., README.linux, README.osx, etc...).


1. Introduction
-----------------------------------------------------------------

libdmtx is a software library that enables programs to read and
write Data Matrix barcodes of the modern ECC200 variety. The
library runs natively on several platforms and can be accessed by
multiple languages using the libdmtx language wrappers. The
utility programs dmtxread and dmtxwrite also provide a command
line interface for libdmtx, and serve as a good reference for
developers writing their own libdmtx-enabled programs.

This package (libdmtx) contains only the core library, and is
distributed under a Simplified BSD license with an alternate
waiver option. See the LICENSE file in the main project directory
for full terms of use and distribution.

The non-library components related to libdmtx are available as
separate downloads, and are distributed under a different license
(typically LGPLv2). Please contact support@dragonflylogic.com if
you require clarification on licensing. It's not complicated, but
it's important to us that all license terms are respected (not
just ours).


2. Project Components
-----------------------------------------------------------------

The libdmtx project serves a diverse audience and contains many
components -- some of which may not be useful to you. Components
fall into one of four categories:

  Description        Package        Audience
  -----------------  -------------  ----------------------
  Core library       libdmtx        libdmtx programs
  Test programs      libdmtx        libdmtx developers
  Utility programs   dmtx-utils     Shell and command line
  Language Wrappers  dmtx-wrappers  Non-C/C++ developers


3. Installation
-----------------------------------------------------------------

libdmtx uses GNU Autotools so installation should be familiar to
free software veterans. If your platform cannot easily run the
Autotools scripts, refer to the appropriate platform-specific
README.xxx located in this directory for alternate instructions.

In theory the following 3 steps would build and install libdmtx
on your system:

  $ ./configure
  $ make
  $ sudo make install

However, you may need to install additional software or make
other changes for these steps to work properly. The details below
will help to address errors and/or customize beyond the defaults.

Problems with "configure" step
----------------------------------------
If you obtained libdmtx from Git you may have received an error
like "./configure: No such file or directory". Run this command
before trying again:

  $ ./autogen.sh

The autogen.sh command requires autoconf, automake, libtool, and
pkgconfig to be installed on your system.

The configure script also offers many options for customizing the
build process, described in detail by running:

  $ ./configure --help

Problems with "make" step
----------------------------------------
Errors encountered during the "make" step are often a result of
missing software dependencies. Install any missing software
mentioned in the error message(s) and try again.

Problems with "sudo make install" step
----------------------------------------
If the 'sudo' command is not configured on your system, you can
alternatively yell "Yeeehaww!" as you log in as root and run it
like this:

  # make install

And finally...
----------------------------------------
If you want to verify that everything is working properly you can
optionally build the test programs:

  $ make check

This command will not perform any tests, but will build the
programs that contain test logic: multi_test, rotate_test,
simple_test, and unit_test.

Note: multi_test and rotate_test contain extra dependencies due
to their graphical nature, and are not terribly useful unless you
need to test the library's internals.


5. Contact
-----------------------------------------------------------------

Documentation wiki:    libdmtx.wikidot.com
GitHub page:           github.com/dmtx/libdmtx
OhLoh.net page:        www.ohloh.net/projects/libdmtx
Open mailing list:     libdmtx-open_discussion@lists.sourceforge.net


6. This Document
-----------------------------------------------------------------

This document is derived from the wiki page located at:

  http://libdmtx.wikidot.com/general-instructions

If you find an error or have additional helpful information,
please edit the wiki directly with your updates.
