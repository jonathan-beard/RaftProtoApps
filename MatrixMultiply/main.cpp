/**
 * main.cpp - 
 * @author: Jonathan Beard
 * @version: Thu May 15 10:32:07 2014
 * 
 * Copyright 2014 Jonathan Beard
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <iostream>
#include <cstdlib>
#include "matrixop.tcc"

#include "randomstring.tcc"
#include "SystemClock.tcc"
#include <cassert>

Clock *system_clock = new SystemClock< System >( 1 /* assigned core */ );

typedef float thetype_t;

int
main( int argc, char **argv )
{
   
    
   //const std::string filename( "randomarray.csv" );
   const std::string filename( "/project/mercury/svardata/10000_10000_float.csv" );
   //const std::string filename( "intmatrix100_100.csv" );
   //const std::string filename( "supersmall.csv" );
   
   auto *A = Matrix< thetype_t >::initFromFile( filename );
   assert( A != nullptr );
   //same matrix, avoid reading from disk again 
   auto *x      = new Matrix< thetype_t >( *A );
   
   /** to test queues we don't need to re-allocate the starting matrices **/
   int runs( 20 );
   std::ofstream nullstream( "/dev/null" );
   while( runs-- )
   {
      auto *output = new Matrix< thetype_t >( A->height, x->width );
      MatrixOp< thetype_t, 3 >::multiply( A, x, output );
      output->print( nullstream , Format::CSV );
      delete( output );
   }

   delete( A );
   delete( x );

   return( EXIT_SUCCESS );
}
