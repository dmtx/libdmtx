#!/usr/bin/perl -w

use strict;
use File::Basename;

# TODO: Test still misses first function of each file

my $errorCount = 0;
undef my $closeLineNbr;
undef my $lineNbrs;

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
      $lineNbrs = (defined $lineNbrs) ? "$lineNbrs, $." : $.;
      $errorCount++;
      undef $closeLineNbr;
   }
}

if($errorCount > 0) {
   print "Missing header comment in file \"" . basename($ARGV) .
         "\" at line(s) $lineNbrs\n";
   exit(1);
}

exit(0);
