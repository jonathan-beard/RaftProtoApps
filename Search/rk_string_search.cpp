#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <cmath>
#include <array>
#include <vector>
#include <type_traits>


const uint64_t prime = 17;
const uint64_t d     = 0xff;

bool 
verifyMatch( const std::string &T, size_t start, size_t end, const std::string &P )
{
   if( end - start != P.length() )
   {
      return( false );
   }
   for( size_t index( 0 );start < end; start++, index++ )
   {
      if( P[ index ] != T[ start ] )
      {
         return( false );
      }
   }
   return( true );
}

int
main( int argc, char **argv )
{
   std::string T = "everything is awesome, everything is cool when";
   std::string P = "verything";


   const auto m = P.length();
   const auto n = T.length();

   uint64_t h( 1 );
   uint64_t p( 0 );
   uint64_t t( 0 );

   for( size_t i( 0 ); i < m; i++ )
   {
      p = ( ( p * d ) + P[ i ] ) % prime ;
      t = ( ( t * d ) + T[ i ] ) % prime ;
      if( i < ( m - 1 ) )
      {
         h = (h * d ) % prime;
      }
   }
   
   
   size_t s( 0 );
   do
   {
      if ( p == t && verifyMatch( T, s, s+m, P ) )
      {
         std::cerr << "Match @ ( " << s << "," << 
            (s + m) << " ): " << T.substr( s, m ) << "\n";
      }
      const auto remove_val( ( T[ s ] * h ) % prime );
      t = ( t - remove_val ) % prime;
      t = (d * t ) % prime;
      t = ( t + T[ s + m ]) % prime;
      s++;
   }while( s <= ( n - m ) );
}
