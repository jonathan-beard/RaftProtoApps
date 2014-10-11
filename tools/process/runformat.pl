#!/usr/bin/env perl
use strict;
use warnings;

my $app = "search";

##
# mmult data list
##
#my %list = (
#   "rex" => 3,
#   "cinnaproa" => 3,
#   "cinnaprob" => 3,
#   "cloud019"  => 12,
#   #"cloud020"  => 12
#   "cloud023"  => 12
#);

##
# search data list
##
my %list = (
   "buzz" => 4
   #"cinnaproa" => 12,
   #"cinnaprivb" => 12,
   #"cloud019"   => 12,
   #"rex"        => 4,
   #"tamarack"   => 12
);

foreach my $machine( keys %list )
{
   my $threadcount = $list{ $machine };

`./format.pl $threadcount ../../$app\_data/$machine/$machine\_$app\_infinite_$threadcount.csv ../../$app\_data/$machine/$machine\_$app\_heap_$threadcount.csv > ../../$app\_data/$machine/$machine\_$app\_data.csv`;

`cat ../../$app\_data/$machine/$machine\_$app\_data.csv | ./formatsvm.pl > ../../$app\_data/$machine/$machine\_$app\_svm`;
}
exit( 0 );
