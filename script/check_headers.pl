#!/usr/bin/perl -w

use strict;

# TODO: Test still misses first function of each file

my $errorCount = 0;
undef my $closeLineNbr;

while(<>) {
   chomp;

   if(m/^}$/) {
      $closeLineNbr = $.;
   }
   elsif(!defined($closeLineNbr) || m/^$/ || m/^\*/) {
      next;
   }
   elsif(m/^\/\*\*$/) {
      undef $closeLineNbr;
   }
   else {
      print "$. "; # XXX fix this later
      $errorCount++;
      undef $closeLineNbr;
   }
}

# Since this script exists to find errors, an exit code of zero means that
# errors were successfully found (this is consistent with other scripts
# that use grep for this purpose)
exit(($errorCount > 0) ? 0 : 1);
