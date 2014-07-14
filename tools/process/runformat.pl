#!/usr/bin/env perl
use strict;
use warnings;

my $machine = shift;

`./format.pl ../../search_data/$machine/$machine\_search_infinite_12.csv ../../search_data/$machine/$machine\_search_heap_12.csv > ../../search_data/$machine/$machine\_search_data.csv`;

`cat ../../search_data/$machine/$machine\_search_data.csv | ./formatsvm.pl > ../../search_data/$machine/$machine\_search_svm`;

exit( 0 );
