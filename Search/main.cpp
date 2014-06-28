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
   std::string search_term = argv[ 2 ];
   std::vector< Hit > hits;
   
   Search< 2, 4 >::search< RabinKarp >( input_file,
                                     search_term,
                                     hits );

   for( const auto &val : hits )
   {
      std::cout<< val << "\n";
   }
   return( EXIT_FAILURE );
}
