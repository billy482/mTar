#! /usr/bin/perl

use strict;
use warnings;

my ( $symbol, $filename ) = @ARGV;
my $new_value = 0;

my ($version) = qx/git describe/;
chomp $version;
my ($branch) = grep {s/^\* (?:.*\/)?(\w+)/$1/} qx/git branch/;
chomp $branch;
$version .= $branch if $branch ne 'stable';

my ($git_commit) = qx/git log -1 --format=%H/;
chomp $git_commit;

my @lines;
if ( open my $fd, '<', $filename ) {
    @lines = <$fd>;
    close $fd;
}

my ( $found_version, $found_commit ) = ( 0, 0 );
foreach (@lines) {
    if (/^#define ${symbol}_VERSION "(.+)"/) {
        $found_version = 1;
        $_ = "#define ${symbol}_VERSION \"$version\"\n" if $version ne $1;
    }
    elsif (/^#define ${symbol}_GIT_COMMIT "(\w+)"/) {
        $found_commit = 1;
        $_            = "#define ${symbol}_GIT_COMMIT \"$git_commit\"\n"
            if $git_commit ne $1;
    }
}

unless ($found_version) {
    $new_value = 1;
    push @lines, "#define ${symbol}_VERSION \"$version\"\n";
}

unless ($found_commit) {
    $new_value = 1;
    push @lines, "#define ${symbol}_GIT_COMMIT \"$git_commit\"\n";
}

if ($new_value) {
	open my $fd, '>', $filename;
	print $fd @lines;
	close $fd;
}

