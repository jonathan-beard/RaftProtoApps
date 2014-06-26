#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include "search.tcc"

int
main( int argc, char **argv )
{
   /** TODO add an actual menu **/
   if( argc < 2 )
   {
      std::cerr << "There should be more than a single command line argument!!\n";
   }
   std::string input_file = argv[ 1 ];
   std::vector< std::string > search_terms;
   for( int i( 2 ); i < argc; i++ )
   {
      search_terms.push_back( argv[ i ] );
   }
   std::vector< std::string > hits;
   
   Search< 1 >::search< RabinKarp >( input_file,
                                     search_terms,
                                     hits );

   for( const std::string &str : hits )
   {
      std::cerr << str << "\n";
   }
   return( EXIT_FAILURE );
}
