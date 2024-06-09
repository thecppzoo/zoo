#! /usr/bin/env perl

use warnings;
use v5.10;
use strict;

my $top = 16;
my $bottom = 1;
my $nLanes = 3;
my $laneIndex = $nLanes - 1;
my @counts = ($bottom) x $nLanes;

OUTER_LOOP: for(;;) {
	print "auto mult";
	print join 'x', @counts;
	print "(S x) { return mm(x, S{0x";
	printf "%02X", $counts[$_] for 0..$#counts;
	say "}); }";

	while($top <= ++$counts[$laneIndex]) {
		if(0 == $laneIndex) { last OUTER_LOOP; }
		$counts[$laneIndex] = $bottom;
		#print "-- @counts $laneIndex";
		--$laneIndex;
	}
	$laneIndex = $nLanes - 1;
}
