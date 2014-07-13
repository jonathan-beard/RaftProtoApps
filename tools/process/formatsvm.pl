#!/usr/bin/env perl
use strict;
use warnings;

##
# model functions, use M/M/1, M/D/1
##
##
# @params pass in ( service time server a, service time server b )
# @returns single float value
##
sub mmone( $$ );
##
# @params pass in ( service time server a, service time server b )
# @returns single float value
##
sub mdone( $$ );
##
# @params pass in ( service time server a, service time server b )
# @returns two float values( lambda, mu )
##
sub rateconvert( $$ );
##
# @params - header file; (column, name)
# @returns - hash w/keys as column numbers
##
sub getheadings( $ );
sub distance( $$ );
sub assignclass( $$$$ );
sub distanceModel( $ );
sub calcrho( $$ );
sub getgradient( $$$ );
##
# @param - reverse heading hash
# @param - heading to insert at max index
##
sub insertheading( $$ );

my $headingsfile = "allheadings.csv";
my $headingshash = getheadings( $headingsfile );
my %reverseheadingshash = reverse( %$headingshash );

my %linesToSkip = (
   $headingshash->{ "NodeName" }    => 1,
   $headingshash->{ "OSVersion" }   => 1,
   $headingshash->{ "QueueMeanOccupancy" } => 1,
   $headingshash->{ "39ExponentialDistribution" } => 1,
   $headingshash->{ "39NormalDistribution" } =>1,
   $headingshash->{ "39Deterministic" } => 1,
   $headingshash->{ "41Deterministic" } =>1,
   $headingshash->{ "41NormalDistribution" } => 1,
   $headingshash->{ "41ExponentialDistribution" } => 1
);

my %models = ( 
"MM1" => 1
#,"MD1" => 2
);

my %classes = ( "None" => 0 );
@classes{ keys %models } = (values %models);

my $precision = 5;
open OUTPUTFILE, ">svmkeys.csv";
my $printsvmkeys = 1;
my $initialized = 0;
while( <> )
{
   my $line = $_;
   chomp( $line );
   my @arr = split /,/, $line;
   my $arrivalServiceTime = $arr[ $headingshash->{ "ProducerDistributionMean" } ];
   my $serverServiceTime  = $arr[ $headingshash->{ "ConsumerDistributionMean" } ];
   my $mm1 = mmone( $arrivalServiceTime, $serverServiceTime );
   #my $md1 = mdone( $arrivalServiceTime, $serverServiceTime );
   my $meanOccupancy = $arr[ $headingshash->{"QueueMeanOccupancy"} ];
   my $dist = distance( $meanOccupancy, [ $mm1 ] );
   my $rho  = 0;
   if( $arrivalServiceTime == 0 || $serverServiceTime == 0 )
   {
      goto END;
   }
   else
   {
      $rho = $serverServiceTime / $arrivalServiceTime;
   }
   ##
   # assign class
   ##
   my $class = assignclass( $dist , $precision, \%classes, $rho );
   my $modelDist = distanceModel( [ ["MM1",$mm1] ] );
   foreach my $arr ( @$modelDist )
   {
      my ($name, $val) = @$arr;
      if( $initialized == 0 )
      {
         insertheading( \%reverseheadingshash, $name ); 
      }
      push( @arr, $val );
   }
   
   ##
   # push out model gradient
   ##

   foreach my $key ( keys %models )
   {
      
      my $gradient = getgradient(   $classes{ $key }, 
                                    $arrivalServiceTime, 
                                    $serverServiceTime );
      ##
      # first index is name, second value
      ##
      foreach my $partial( @$gradient )
      {
         my ($name, $val) = @$partial;
         if( $initialized == 0 )
         {
            insertheading( \%reverseheadingshash, $name ); 
         }
         push( @arr, $val );
      }
   }
   
   
   $initialized = 1;
   

   for( my $i = 0; $i < 0+ @arr; $i++ )
   {
      ## only numeric lines exist
      if( ! exists $linesToSkip{ $i } ){
         if( $arr[$i] > 0 && $arr[ $i ] < 1e-37 )
         {
            $arr[ $i ] = 0;
         }
         elsif( $arr[ $i ] > 1e37 )
         {
            $arr[ $i ] = 1e37;
         }
      }
   }

   my @outputline;
   push( @outputline, $class );
   my $index = 1;
   for( my $i = 0; $i < 0+@arr; $i++ )
   {
      if( ! exists $linesToSkip{ $i } )
      {
         push( @outputline, ($index).":".$arr[ $i ] );
         if( $printsvmkeys == 1 )
         {
            print OUTPUTFILE $index.",$reverseheadingshash{ $i }\n";  
         }
         $index++;
      }
   }
   $printsvmkeys = 0;
   print STDOUT join(" ", @outputline )."\n";
   END:;
}

exit( 0 );

sub mmone ( $$ )
{
   my ( $arrival, $server ) = @_;
   my ( $lambda , $mu ) = rateconvert( $arrival, $server );
   my $output = 0;
   eval{
      $output = ($lambda * $lambda) / ( $mu * ( $mu - $lambda ) );
   }; warn "Error with lambda ($lambda) and mu ($mu)\n", if( $@ );
   return( $output );
}

sub mdone ( $$ )
{
   my ( $arrival, $server ) = @_;
   my ( $lambda , $mu ) = rateconvert( $arrival, $server );
   my $output = 0;
   eval{
      $output = 
         ($lambda * $lambda) / (2 * (1 - ($lambda / $mu ) ) * ($mu * $mu) );
   }; warn "Error with lambda ($lambda) and mu ($mu)\n", if( $@ );
   return( $output );
}

sub rateconvert( $$ )
{
   my ( $arrival, $server ) = @_;
   ##
   # Note: we make the assumption that a service time
   # mean of zero means that the service rate is going
   # to simply be very high, so we set it to some obsurdly
   # large value.  Perhaps fix till we can come up with 
   # a better approximation of the service rate since 
   # theoretically it isn't zero, its determined by the
   # scheduler, processor, application combo.
   ##
   return( ($arrival == 0 ? 0 : 1 / $arrival ) , 
           ($server == 0  ? 1e36: 1 / $server  )  );
}

sub getheadings( $ )
{
   my ($infile) = @_;
   open INFILE, "<$infile" or die "Couldn't open $infile.";
   my @data = <INFILE>;
   close( INFILE );

   my %hash;
   foreach my $line ( @data )
   {
      my @arr = split /,/, $line;
      chomp( $_ ), foreach( @arr );
      $hash{ $arr[ 1 ] } = $arr[ 0 ];
   }
   return( \%hash );
}

sub distance( $$ )
{
   my ($val, $arr ) = @_;
   my @distance = ( 1 .. 0+@$arr );
   $_ = 0, foreach( @distance );
   for( my $i = 0; $i < 0+@$arr; $i++ )
   {
      $distance[ $i ] = abs( $arr->[ $i ] - $val );
   }
   return( \@distance );
}

sub distanceModel( $ )
{
   my ( $arr ) = @_;
   my @distance;
   for( my $i = 0; $i < 0+@$arr; $i++)
   {
      for( my $j = $i; $j < 0+@$arr; $j++ )
      {
         push( @distance, [ $arr->[ $i ]->[ 0 ]."->".$arr->[ $j ]->[ 0 ], abs( $arr->[ $i ]->[ 1 ] - $arr->[ $j ]->[ 1 ] ) ] );
      }
   }
   return( \@distance );
}

sub assignclass( $$$$ )
{
   my ( $arr, $precision, $classes, $rho ) = @_;
   
   my $output = $classes->{ "None" };
   
   if( $rho >= 1 )
   {
      return( $output );
   }

   my $minval = 1e36;
   my $minindex = -1;
   my $found = 0;
   for( my $i = 0; $i < 0 + @$arr; $i++ )
   {
      if( $arr->[$i] < $precision )
      {
         if( $arr->[$i] < $minval )
         {
            $found      = 1;
            $minval     = $arr->[$i];
            $minindex   = $i;
         }
      }
   }
   ##
   # assume classes are 1 offset from index,
   # i.e., zero is "None" therefore everything
   # is plus one from that.
   ##
   if( $found  == 1 )
   {
      $output = ( $minindex + 1 );
   }
   return( $output );
}

sub calcrho( $$ )
{
   my ( $lambda, $mu ) = @_;
   return( $lambda / $mu );
}

## will add gradient as a flattened array ## 
sub getgradient( $$$ )
{
   my ( $type, $serverA, $serverB ) = @_;
   my ( $lambda, $mu ) = rateconvert( $serverA, $serverB );
   my @gradient;
   if( $type == (exists $classes{ "MM1" } ? $classes{ "MM1" } : -1 ) )
   {
      $gradient[ 0 ] = 
         [ "pMu", ( ($lambda**2) / 
            ( $mu * ($mu - $lambda) ** 2)  ) + 
               (( 2 * $lambda) / ( $mu * ( $mu - $lambda )) ) ];
      $gradient[ 1 ] = ["pLambda", 
      (-(($lambda**2) / ( $mu * ( $mu - $lambda)**2 )) - (($lambda**2) / ( ( $mu**2 ) * ($mu - $lambda )) )) ];
   }
   elsif( $type == ( exists $classes{ "MD1" } ? $classes{ "MD1" } : -1 ) )
   {
      $gradient[ 0 ] = [ "pMu", 0 ];
      $gradient[ 1 ] = [ "pLambda", 0 ];
   }
   else
   {
      $gradient[ 0 ] = [ "pMu", 0 ];
      $gradient[ 1 ] = [ "pLambda", 0 ];
   }
   return( \@gradient );
}

sub insertheading( $$ )
{
   my ($hash, $name ) = @_;
   my $max = -1;
   for my $keys ( keys %$hash )
   {
      if( $max < $keys )
      {
         $max = $keys;
      }
   }
   $hash->{ ++$max } = $name;
}
