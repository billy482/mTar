#! /usr/bin/perl

use strict;
use warnings;

my $width = shift @ARGV;
$width = 78 unless defined $width;

my @lines = <>;

chomp @lines;
my $line = shift @lines;
$line =~ s/^\s*(.+?)\s*$/$1/;
print "$line\n";

$line = shift @lines;
$line =~ s/^\s*(.+?)\s*$/$1/;
print "  $line\n\n";
shift @lines;

my $last;
while ( $line = shift @lines ) {
    $line =~ s/^\s*(.+?)\s*$/$1/;
    print "  $line\n";

    $line = shift @lines or exit;
    my @tabs;
    my $hasShortParam = 0;
    while ( length $line > 0 ) {
        my ( $left, $right ) = split /\s*:\s*/, $line;

        $left =~ s/^\s*(.+?)\s*$/$1/;
        unless ( defined $right ) {
            $last->{right} .= ' ' . $left;
            next;
        }

        my $lLength    = length $left;
        my @param      = split /,\s*/, $left;
        my $shortParam = grep {/^-[^-]/} @param;
        $hasShortParam += $shortParam;

        $right =~ s/^\s*(.+?)\s*$/$1/;

        $last = {
            left        => $left,
            lparam      => \@param,
            short_param => $shortParam,
            right       => $right,
        };
        push @tabs, $last;

    }
    continue {
        $line = shift @lines or last;
    }

    my $length = 0;
    foreach my $t (@tabs) {
        if ( $hasShortParam and not $t->{short_param} ) {
            $t->{left} = '    ' . $t->{left};
        }

        my $tLength = length $t->{left};
        $length = $tLength if ( $length < $tLength );
    }

    foreach my $t (@tabs) {
        my $rLength = length $t->{right};
        my $space   = $width - $length - 7;

        if ( $rLength > $space * 0.92 ) {
            my @split = split /\s+/, $t->{right};

            my $tLength = 0;
            my $i       = 0;
            foreach (@split) {
                my $sLength = length;
                last if $tLength + $sLength + $i > $space;
                $tLength += $sLength;
                $i++;
            }

            my $smallSpace = ( $space - $tLength ) / ( $i - 1 );
            my $cRight     = shift @split;
            my $totalSpace = 0;
            for ( my $j = 1; $j < $i; $j++ ) {
                my $cSpace = int( $j * $smallSpace ) - $totalSpace;
                $totalSpace += $cSpace;

                $cRight .= ( ' ' x $cSpace ) . shift @split;
            }

            printf "    %-*s : %s\n", $length, $t->{left}, $cRight;

            while (@split) {
                $cRight  = join ' ', @split;
                $rLength = length $cRight;
                $space   = $width - $length - 7;

                if ( $rLength > $space * 0.92 ) {
                    $tLength = $i = 0;
                    foreach (@split) {
                        my $sLength = length;
                        last if $tLength + $sLength + $i > $space;
                        $tLength += $sLength;
                        $i++;
                    }

                    $smallSpace = ( $space - $tLength ) / ( $i - 1 );
                    $cRight     = shift @split;
                    $totalSpace = 0;
                    for ( my $j = 1; $j < $i; $j++ ) {
                        my $cSpace = int( $j * $smallSpace ) - $totalSpace;
                        $totalSpace += $cSpace;

                        $cRight .= ( ' ' x $cSpace ) . shift @split;
                    }

                    print ' ' x ( $length + 7 ), $cRight, "\n";
                }
                else {
                    print ' ' x ( $length + 7 ), $cRight, "\n";
                    last;
                }
            }
        }
        else {
            printf "    %-*s : %s\n", $length, $t->{left}, $t->{right};
        }
    }

    print "\n";
}

