#!/usr/bin/env perl
use strict;
use warnings;

my %field_index = 
(
   "arrivalRate" => 39,
   "departureRate" => 40
);

my %queueList;

my $count = 0;
my $index = 0;

my ($inputfile,$queue_count) = @ARGV;
open INPUTFILE, "<$inputfile" or die "Couldn't open $inputfile\n";
my @lines = <INPUTFILE>;
close(INPUTFILE);

foreach( @lines )
{
   chomp( $_ );
   my @fields = split /,/, $_;
   if( ! exists $queueList{ $index } )
   {
      $queueList{ $index } = [ 0, 0 ];
      $count++;
   }
   $queueList{ $index }->[ 0 ] += $fields[ $field_index{ "arrivalRate" } ];
   $queueList{ $index }->[ 1 ] += $fields[ $field_index{ "departureRate" } ];
   $index = ($index + 1 ) % $queue_count;
}

for( sort keys %queueList )
{
   my $arrivalRate =$queueList{ $_ }->[ 0 ] / $count;
   my $departureRate = $queueList{ $_ }->[ 1 ] / $count;
   my $rho = $arrivalRate / $departureRate;
   my $arrival_service = 1 / $arrivalRate;
   my $departure_service = 1 / $departureRate;

   print $arrival_service.",".$departure_service.",".$rho."\n";
}
