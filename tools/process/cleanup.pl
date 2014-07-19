#!/usr/bin/env perl
use strict;
use warnings;

my $i       = 0;
my $length  = "";
my @fields  = {};

my %obsByProcessor;

while( <> )
{
   chomp( $_ );
   @fields = split /,/, $_;
   $length = @fields;
   if( $length >= 43 )
   {
      ##
      # keep line
      ##
STARTOVER:
      my $processorKey = "";
      my $line         = "";
      for( $i = 0 ; $i < $length; $i++ )
      {
         ##
         # check each field
         ##
         chomp($fields[ $i ]);
         ##
         # trim leading and trailing whitespace 
         ##
         $fields[ $i ] =~ s/^\s+//;
         $fields[ $i ] =~ s/\s+$//;
         if( $i == 20 || $i == 16 )
         {
            ##
            # Various fixes for processors that don't conform 
            ##
            if( $fields[ $i ] =~ m/zed/ )
            {
               ##
               # Fix for zedboard
               ##
               my $name = "ARM Cortex-A9";
               #level 1 icache
               $fields[ 0 ] = "32768";
               #level 1 icache associativity
               $fields[ 1 ] = "4";
               #level 1 icache line size
               $fields[ 2 ] = "64";
               #level one dcache size
               $fields[ 3 ] = "32768";
               #level 1 dcache associativity;
               $fields[ 4 ] = "4";
               #level 1 dcache line size
               $fields[ 5 ] = "64";
               #level 2 cache size
               $fields[ 6 ] = "524288";
               #level 2 cache associativity
               $fields[ 7 ] = "8";
               #level 2 line size
               $fields[ 8 ] = "64";
               $fields[ 19 ] = "zedboard";
               $processorKey = $name;
               $fields[ 16 ] = $name;
               $fields[ 17 ] = 667 * 1e6;
               $fields[ $i ] =~ tr/zb/ZB/;
               ##
               # Start this puppy over again 
               ##
               $line = "";
               $i = 0;
               goto STARTOVER;
            }
            if( $fields[ $i ] =~ m/ARMv6/ )
            {
               ##
               # Fix for zedboard
               ##
               my $name = "ARM1176JZF-S";
               #level 1 icache
               $fields[ 0 ] = "16384";
               #level 1 icache associativity
               $fields[ 1 ] = "4";
               #level 1 icache line size
               $fields[ 2 ] = "64";
               #level one dcache size
               $fields[ 3 ] = "16384";
               #level 1 dcache associativity;
               $fields[ 4 ] = "4";
               #level 1 dcache line size
               $fields[ 5 ] = "64";
               #level 2 cache size
               $fields[ 6 ] = "0";
               #level 2 cache associativity
               $fields[ 7 ] = "0";
               #level 2 line size
               $fields[ 8 ] = "0";
               $processorKey = $name;
               $fields[ $i ] = $name;
               $fields[ 17 ] = 700 * 1e6;
               ##
               # Start this puppy over again 
               ##
               $line = "";
               $i = 0;
               goto STARTOVER;
            }
            if( $fields[ $i ] =~ m/powerpc64/ )
            {
               my $name = "PPC970";
               #level 1 icache
               $fields[ 0 ] = "65536";
               #level 1 icache associativity
               $fields[ 1 ] = "0";
               #level 1 icache line size
               $fields[ 2 ] = "128";
               #level one dcache size
               $fields[ 3 ] = "32768";
               #level 1 dcache associativity;
               $fields[ 4 ] = "2";
               #level 1 dcache line size
               $fields[ 5 ] = "128";
               #level 2 cache size
               $fields[ 6 ] = "524288";
               #level 2 cache associativity
               $fields[ 7 ] = "8";
               #level 2 line size
               $fields[ 8 ] = "128";
               $processorKey = $name;
               $fields[ 16 ] = $name;
               $fields[ $i ] =~ tr/smp/SMP/;

               $fields[ 17 ] = 1600 * 1e6;
               ##
               # Start this puppy over again 
               ##
               $line = "";
               $i = 0;
               goto STARTOVER;
            }
         }
         if( $i == 16 )
         {
            $processorKey = $fields[ $i ];
         }

         if( $i != $length - 1 )
         {
            $line .= $fields[ $i ].",";
         }
         else
         {
            $line .= $fields[ $i ];
         }
      }
      my $good = 1;
      if( $fields[ 39 ] =~ m/[0-9]/ || $fields[ 41 ] =~ m/[0-9]/ )
      {
         $good = 0;
      }
      if( $good == 1 )
      {
         if( exists $obsByProcessor{ $processorKey } )
         {
            my $arr = $obsByProcessor{ $processorKey };
            push( @$arr , $line );
         }
         else
         {
            my @arr = ( $line );
            $obsByProcessor{ $processorKey } = \@arr;
         }
      }
   }
}

foreach my $key ( keys %obsByProcessor )
{
   my $arr = $obsByProcessor{ $key };
   my $arrLength = @$arr;
   if( $arrLength > 10 )
   {
      foreach my $line ( @$arr )
      {
         print STDOUT $line."\n";
      }
   }
   else
   {
      print STDERR "Eliminating entries for processor ( $key ) because there are only $arrLength entries.\n";
   }
}

exit( 0 );
