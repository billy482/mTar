#! /usr/bin/perl

use strict;
use warnings;

my $iLine = 0;
while (<>) {
	chomp;
	$iLine++;

	if (length > 78) {
		printf "%03d => {%s}|{%s}\n", $iLine, substr($_, 0, 78), substr($_, 78);
	}
}

