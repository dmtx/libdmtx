#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include "dmtxutil.h"

extern char *programName;

/**
 * Display error message and exit with error status.
 *
 * @param errorCode error code returned to OS
 * @param fmt       error message format for printing
 * @return          void
 */
extern void
FatalError(int errorCode, char *fmt, ...)
{
   va_list va;

   va_start(va, fmt);
   fprintf(stderr, "%s: ", programName);
   vfprintf(stderr, fmt, va);
   va_end(va);
   fprintf(stderr, "\n\n");
   fflush(stderr);

   exit(errorCode);
}

/**
 * Convert a string of characters to an integer.  If string cannot be
 * converted then the function will abort the program.
 *
 * @param numberString pointer to string of numbers
 * @return             converted long
 */
extern int
StringToInt(int *numberInt, char *numberString, char **terminate)
{
   long numberLong;

   errno = 0;
   numberLong = strtol(numberString, terminate, 10);

   while(isspace((int)**terminate))
      (*terminate)++;

   if(errno != 0 || (**terminate != '\0' && **terminate != '%')) {
      *numberInt = -1;
      return DMTXUTIL_ERROR;
   }

   *numberInt = (int)numberLong;
   return DMTXUTIL_SUCCESS;
}
