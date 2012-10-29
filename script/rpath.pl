#! /usr/bin/perl

use strict;
use warnings;

open my $fd, '<', 'config.h' or print "lib";
my $rpath = 'lib';
while (<$fd>) {
	if (/^#.+PLUGINS_PATH\s+"(\.+)"/) {
		$rpath = $1;
	}
}
close $fd;

print $rpath;

