#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <cstdint>
#include <cinttypes>
#include "search.tcc"

int
main( int argc, char **argv )
{
   /** TODO add an actual menu **/
   if( argc < 2 )
   {
      std::cerr << "There should be more than a single command line argument!!\n";
   }
   std::string input_file  = argv[ 2 ];
   std::string search_term = argv[ 1 ];

   int runs( 20 );
   while( runs-- )
   {
      std::vector< Hit > hits;
      Search< 12, 512 >::search< RabinKarp >( input_file,
                                        search_term,
                                        hits );

      std::ofstream ofs( "/dev/null" );
      if( ! ofs.is_open() )
      {
         std::cerr << "Didn't open output stream, exiting!!\n";
         exit( EXIT_FAILURE );
      }
      /** else **/
      for( const auto &val : hits )
      {
         ofs << val << ": " << argv[ 2 ] << "\n";;
      }
   }
   return( EXIT_FAILURE );
}
