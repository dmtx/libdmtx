PHP_ARG_ENABLE(libdmtx, whether to enable lib datamatrix support, [  --enable-libdmtx   Enable lib datamatrix support])

if test "$PHP_LIBDMTX" = "yes"; then
  AC_DEFINE(HAVE_LIBDMTX, 1, [Whether you have lib Datamatrix])

  PHP_ADD_LIBRARY_WITH_PATH(dmtx, /usr/lib/, DMTX_SHARED_LIBADD)

  PHP_NEW_EXTENSION(dmtx, dmtx_write.c, $ext_shared)
  PHP_SUBST(DMTX_SHARED_LIBADD)
fi
