#!/usr/bin/env perl
use strict;
use warnings;

##
# call to initialize output array format, each
# field is a column in the output.
# @param file name with SVM keys
# @param array reference to store results
##
sub processOutputFields($$);
##
# @param heap observation file name
# @param rate observation file name
# @param array reference to store observations
##
sub initializeObservations($$$);
##
# @param rate observation file name
# @param array reference to store rates
##
sub initializeRates($$);

##
# @param array ref with observations
# @param filename with SVM keys
##
sub reformatColumns( $$ );

my $outputfields = "headings_formatted.csv";
my $svmkeysFile  = "svmkeys.csv";

##
# kindof hacky, but to make my life easier lets go ahead
# and generate the ratefile here so we have it
##



my ( $obsInfiniteFile, $obsFile ) = @ARGV;

`cat $obsInfiniteFile | ./get_service_time.pl > /tmp/ratefile`;

my $rateFile = "/tmp/ratefile";

my @fieldsArr;
my @observations;

initializeObservations( $obsFile, $rateFile, \@observations);
reformatColumns( \@observations, $outputfields );

foreach my $line (@observations)
{
   print join( ',', @{$line} )."\n"; 
}




##
# Begin functions
##

sub processOutputFields( $$ )
{
   my ($filename,$arroutput) = @_;
   open FILE, "<$filename" or die "Couldn't open input file $filename\n";
   my $index = 0;
   while( <FILE> )
   {
      chomp( $_ );
      my @line = split /,/, $_;
      if( ( 0 + @line ) == 2 )
      {
         push( @{$arroutput}, $line[ 1 ] );
      }
      else
      {
         print STDERR "Invalid heading line ($index): $_\n";
      }
      $index++;
   }
   close( FILE );
}

sub initializeRates( $$ )
{
   my ($filename, $arroutput ) = @_;
   open FILE, "<$filename" or die "Couldn't open input file $filename\n";
   my $index = 0;
   while( <FILE> )
   {
      chomp( $_ );
      my @line = split /,/, $_;
      if( ( 0 + @line ) == 3 )
      {
         push( @{$arroutput}, \@line );
      }
      else
      {
         print STDERR "Invalid rate found at line ($index): $_\n";
      }
      $index++;
   }
   close( FILE );
}

sub initializeObservations($$$)
{
   ##
   # I'm hard coding these b/c well its quick.  The columns we want to replace 
   # in the input are: 39 -> arrival rate, 40 -> departure rate, 42 -> rho
   ##
   my %replaceHash = 
   (
      39 => 0,
      40 => 1,
      42 => 2
   );
   my ( $filename,$ratefilename, $arroutput ) = @_;
   ##
   # get rates first
   ##
   my @rates;
   initializeRates( $ratefilename, \@rates );
   my $queues = 0 + @rates;
   open FILE, "<$filename" or die "Couldn't open input file $filename\n";
   my $index = 0;
   my $queue_index = 0;
   while( <FILE> )
   {
      chomp( $_ );
      my @line = split /,/, $_;
      if( ( 0 + @line ) == 43 )
      {
         foreach my $field ( @line )
         {
            $field =~ s/^\s+//;
            $field =~ s/\s+$//;
         }
         ##
         # while we're here update the rates and distribution fields
         ##
         foreach my $key ( keys %replaceHash )
         {
            $line[ $key ] = $rates[ $queue_index ]->[ $replaceHash{ $key } ]; 
         }
         push( @{$arroutput}, \@line );
      }
      else
      {
         print STDERR "Invalid observation found at line ($index): $_\n";
      }
      $queue_index = ($queue_index + 1) % $queues;
      $index++;
   }
   close( FILE );
}

sub reformatColumns( $$ )
{
   my ($observations, $newheadingsfile ) = @_;
   open INFILE, "<$newheadingsfile" or die "Couldn't open SVM keys file: $svmkeysFile\n";
   my @svmkeys = <INFILE>;
   close( INFILE );
   ##
   # OK, some columns have three and others have two.  The ones that have three are 
   # the ones we've split into multiple columns so that they're boolean 0/1 ranges
   # instead of text.
   ##
   my %hashlist;
   ##
   # format for hashlist
   # oldcolumn => \% ( strkey => new column )
   ##
   my $svmKeyCount = 0;
   foreach my $line (@svmkeys)
   {
      chomp( $line );
      my @fields = split /,/, $line;
      if( ( 0+@fields ) == 2 )
      {
         ## ignore these for now ##
         $svmKeyCount++;
      }
      elsif( (0+@fields) == 3 )
      {
         if( ! exists $hashlist{ $fields[ 1 ] } )
         {
            ## 
            # add new anonymous hash
            ##
            $hashlist{ $fields[ 1 ] } = ();
         }
         $hashlist{ $fields[ 1 ] }->{ $fields[ 2 ] } = $fields[ 0 ];
         $svmKeyCount++;
      }
      else
      {
         print STDERR "Invalid line in svmkey file ($svmkeysFile) : $line\n";
      }
   }
   
   for my $oldindex ( keys %hashlist )
   {
      for my $hash ( keys %{ $hashlist{ $oldindex } } )
      {
         print STDERR $oldindex." - ".$hash." - ".$hashlist{ $oldindex }->{ $hash }."\n";
      }
   }
  
   my %insertcolumns = 
   (
      73 => "",
      74 => "",
      75 => "",
      77 => "",
      78 => "",
      79 => ""
   );

   
   foreach my $obs ( @$observations )
   {
      my @newobsline = ( ( 0 ) x $svmKeyCount );
      my $newIndex = 0;
      for( my $index = 0; $index < (@$obs + 0); )
      {
         if( exists $insertcolumns{ $newIndex } )
         {
            $newIndex++;
            goto END;
         }
         if( exists $hashlist{ $index }->{ $obs->[ $index ] })
         {
            ##
            # now we need to get the value of the observation 
            # and get the new index to set to 1
            ##
            $newobsline[ $hashlist{ $index }->{ $obs->[ $index ] } ] = 1;
            $newIndex += 0 + ( keys $hashlist{ $index } );
            print STDERR "$newIndex\n";
         }
         else
         {
            $newobsline[ $newIndex++ ] = $obs->[ $index ];
         }
         #print STDERR "$newIndex - $newobsline[ $newIndex ]\n";
         $index++;
         END:;
      }
      pop( @newobsline );
      $obs = \@newobsline;
   }
}
