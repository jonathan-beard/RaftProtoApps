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

#include "SystemClock.tcc"

Clock *system_clock = new SystemClock< System >( 1 /* assigned core */ );

typedef double thetype_t;

int
main( int argc, char **argv )
{
   
   
   const std::string filename( "randomarray.csv" );
   //const std::string filename( "/project/mercury/svardata/randommatrix.csv" );
   //const std::string filename( "supersmall.csv" );

   auto *A = Matrix< thetype_t >::initFromFile( filename );
//   auto *x = Matrix< thetype_t >::initFromFile( filename );
   
   //same matrix, avoid reading from disk again 
   auto *x = new Matrix< thetype_t >( *A );
   auto *output = new Matrix< thetype_t >( A->height, x->width );

   const auto start_time( system_clock->getTime() );
   MatrixOp< thetype_t, 2 >::multiply( A, x, output );
   const auto end_time( system_clock->getTime() );
   std::cerr << ( end_time - start_time ) << "\n";
   //output->print( std::cout, Format::CSV );

   delete( A );
   delete( x );
   delete( output );

   return( EXIT_SUCCESS );
}
