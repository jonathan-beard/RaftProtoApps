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

int
main( int argc, char **argv )
{
   
   const std::string filename( "randomarray.csv" );
   //const std::string filename( "/project/mercury/svardata/randommatrix.csv" );

   auto *A = Matrix< float >::initFromFile( filename );
//   auto *x = Matrix< float >::initFromFile( filename );
   //same matrix, avoid reading from disk again 
   auto *x = new Matrix< float >( *A );
   auto *output = MatrixOp< float >::multiply( A, x );
   
   output->print( std::cout, Format::CSV );

   delete( A );
   delete( x );
   delete( output );

   return( EXIT_SUCCESS );
}
