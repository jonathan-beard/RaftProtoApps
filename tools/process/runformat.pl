#!/usr/bin/env perl
use strict;
use warnings;

my $threadcount = 12;
my $app = "mmult";

foreach my $machine( @ARGV )
{
`./format.pl ../../$app\_data/$machine/$machine\_$app\_infinite_$threadcount.csv ../../$app\_data/$machine/$machine\_$app\_heap_$threadcount.csv > ../../$app\_data/$machine/$machine\_$app\_data.csv`;

#`cat ../../$app\_data/$machine/$machine\_$app\_data.csv | ./formatsvm.pl > ../../$app\_data/$machine/$machine\_$app\_svm`;
}
exit( 0 );
