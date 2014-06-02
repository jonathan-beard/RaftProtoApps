/**
 * matrixop.tcc - 
 * @author: Jonathan Beard
 * @version: Thu May 15 10:33:27 2014
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
#ifndef _MATRIXOP_TCC_
#define _MATRIXOP_TCC_  1

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <utility>
#include <sstream>
#include <typeinfo>
#include <fstream>
#include <vector>
#include <random>
#include <functional>
#include "ringbuffer.tcc"

#define PARALLEL     1
#define NUMWORKERS   4


enum Format { CSV, SPACE };
   

template< typename T > struct Matrix{
   
   Matrix( size_t height, size_t width ) :   height( height ),
                                             width( width )
   {
      matrix = new T[height * width]();
      std::memset( matrix, 0, sizeof( T ) * height * width );
   }

   Matrix( const Matrix< T > &other ) : height( other.height ),
                                         width(  other.width  )
   {
      matrix = new T[ height * width ]();
      std::memcpy( matrix, 
                   other.matrix, 
                   sizeof( T ) * height * width );
   }

   template< size_t H, size_t W >
   static
      Matrix< T >* initFromArray( T input[H][W] )
   {
      Matrix< T > *output = new Matrix< T >( H, W );
      size_t index( 0 );
      for( size_t i( 0 ); i < H; i++ )
      {
         for( size_t j( 0 ); j < W; j++ )
         {
            output->matrix[ index++ ] = input[i][j];
         }
      }
      return( output );
   }
  
   


   static Matrix< T >*
   initFromFile( const std::string filename )
   {
      
      std::ifstream input( filename );
      if( input.is_open() )
      {
         std::vector< std::vector< T >* > temp_matrix;
         std::string line;
         size_t num_columns( 0 );
         while( input.good() )
         {
            temp_matrix.push_back( new std::vector< T >() );
            std::getline( input, line, '\n' );
            std::istringstream iss( line );
            std::string val;
            while( iss.good() )
            {
               std::getline( iss, val, ',' );
               std::istringstream conv( val );
               T realval( 0 );
               conv >> realval ;
               temp_matrix.back()->push_back( realval );
            }
            if( num_columns == 0 )
            {
               num_columns = temp_matrix.back()->size();
            }
            else
            {
               if( temp_matrix.back()->size() != num_columns )
               {
                  std::cerr << 
                     "Number of columns are not equal at row (" << 
                     temp_matrix.size() << ").  Exiting!!\n";
                  exit( EXIT_FAILURE );
               }
            }
         }
         input.close();
         auto *output( new Matrix< T >( temp_matrix.size(), num_columns ) );
         size_t index( 0 );
         for( std::vector< T >* v : temp_matrix )
         {
            for( T ele : *v )
            {
               output->matrix[ index++ ] = ele;
            }
            delete( v );
            v = nullptr;
         }
         return( output );
      }
      //TODO, probably should throw an exception
      return( nullptr );
   }

   template< size_t H >
   static
      Matrix< T >* initFromArray( T input[H] )
   {
      Matrix< T > *output = new Matrix< T >( H, 1 );
      size_t index( 0 );
      for( size_t i( 0 ); i < H; i++ )
      {
         output->matrix[ index++ ] = input[ i ];
      }
      return( output );
   }

   virtual ~Matrix()
   {
      delete[]( matrix );
   }


   std::ostream& print( std::ostream &stream, Format format )
   {
      for( size_t row_index( 0 ); row_index < height; row_index++ )
      {
         for( size_t column_index( 0 ); column_index < width; column_index++ )
         {
            stream << matrix[ ( row_index * width ) + column_index ];
            if( column_index != (width - 1 ) )
            {
               switch( format )
               {
                  case( SPACE ):
                  {
                     stream << " ";
                  }
                  break;
                  case( CSV ):
                  {
                     stream << ",";
                  }
                  break;
                  default:
                     assert( false );
                  break;
               }
            }
         }
         stream << "\n";
      }
      return( stream );
   }

   Matrix< T >* rotate()
   {
      if( width == 1 )
      {
         /* return copy */
         Matrix< T > *output = new Matrix< T >( *this );
         output->height = width;
         output->width  = height;
         return( output );
      }
      Matrix< T > *output = new Matrix< T >( width, height );
      size_t output_index( 0 );
      for( size_t column_index( 0 ); column_index < width; column_index++ )
      {
         for( size_t row_index( 0 ); row_index < height; row_index++ )
         {
            output->matrix[ output_index++ ] = 
               matrix[ ( row_index * width ) + column_index ];
         }
      }
      return( output );
   }

   
   T *matrix;
   size_t height;
   size_t width;
};
   
template< typename Type > struct ParallelMatrixMult 
{
   ParallelMatrixMult( Matrix< Type >  *a, 
                       const size_t a_start,
                       const size_t a_end,
                       Matrix< Type >  *b,
                       const size_t b_start,
                       const size_t b_end,
                       Matrix< Type >  *output,
                       const size_t output_index ) : a( a ),
                                              a_start( a_start ),
                                              a_end( a_end ),
                                              b( b ),
                                              b_start( b_start ),
                                              b_end( b_end ),
                                              output( output ),
                                              output_index( output_index ),
                                              done( false )
   {
      /** nothing to do here **/
   }

   /**
    * dummy constructor to shutdown threads
    */
   ParallelMatrixMult( bool done ) : a( nullptr ),
                                     a_start( 0 ),
                                     a_end( 0 ),
                                     b( nullptr ),
                                     b_start( 0 ),
                                     b_end( 0 ),
                                     output( nullptr ),
                                     output_index( 0 ),
                                     done( done )
   {
      /* nothing to do here **/
   }

   ParallelMatrixMult( )           : a( nullptr ),
                                     a_start( 0 ),
                                     a_end( 0 ),
                                     b( nullptr ),
                                     b_start( 0 ),
                                     b_end( 0 ),
                                     output( nullptr ),
                                     output_index( 0 ),
                                     done( false )
   {
   }

   ParallelMatrixMult( const ParallelMatrixMult<Type> &other ) :
      a              ( other.a ),
      a_start        ( other.a_start ),
      a_end          ( other.a_end ),
      b              ( other.b ),
      b_start        ( other.b_start ),
      b_end          ( other.b_end ),
      output         ( other.output ),
      output_index   ( other.output_index ),
      done           ( other.done )
   {}

   ParallelMatrixMult< Type >&
      operator =( const ParallelMatrixMult< Type > &other )
   {
      a              = other.a ;
      a_start        = other.a_start ;
      a_end          = other.a_end ;
      b              = other.b ;
      b_start        = other.b_start ;
      b_end          = other.b_end ;
      output         = other.output ;
      output_index   = other.output_index ;
      done           = other.done;
      return( *this ); 
   }
   
   ParallelMatrixMult< Type >& 
      operator =( ParallelMatrixMult< Type > &other )
   {
      a              = other.a ;
      a_start        = other.a_start ;
      a_end          = other.a_end ;
      b              = other.b ;
      b_start        = other.b_start ;
      b_end          = other.b_end ;
      output         = other.output ;
      output_index   = other.output_index ;
      done           = other.done;
      return( *this ); 
   }

   ~ParallelMatrixMult()
   {
   }

   Matrix< Type > *a;
   size_t a_start;
   size_t a_end;
   Matrix< Type > *b;
   size_t b_start;
   size_t b_end;
   Matrix< Type >  *output;
   size_t output_index;
   bool         done;
};



template< typename T > class MatrixOp {
public:
   MatrixOp() = delete;
   virtual ~MatrixOp() = delete;

   typedef RingBuffer< ParallelMatrixMult< T > > PBuffer;
   
   static Matrix< T >* multiply( Matrix< T > *a, 
                                 Matrix< T > *b ) 
                                 
   {
      assert( a != nullptr );
      assert( b != nullptr );
      if( a->width != b->height )
      {
         //TODO make some proper exceptions
         std::cerr << "Matrix a's width must equal matrix b's height.\n";
         return( nullptr );
      }
#ifdef PARALLEL
      std::array< std::thread*, NUMWORKERS > thread_pool;
      std::array< PBuffer*,     NUMWORKERS > buffer_list;
      for( size_t i( 0 ); i < NUMWORKERS; i++ )
      {
         buffer_list[ i ] = new PBuffer( 1000000 );
      }
      for( size_t i( 0 ); i < NUMWORKERS; i++ )
      {
         thread_pool[ i ] = new std::thread( mult_thread_worker,
                                             buffer_list[ i ]  );
      }
      
      std::default_random_engine generator;
      std::uniform_int_distribution< int > distribution( 0, (NUMWORKERS - 1) );
      auto gen_index( std::bind( distribution, generator ) );
#endif
      Matrix< T > *output = new Matrix< T >( a->height, b->width );
      Matrix< T > *b_rotated = b->rotate();
      for( size_t b_row_index( 0 ); 
            b_row_index < b_rotated->height; b_row_index++ )
      {
         size_t output_index( b_row_index );
         for( size_t a_row_index( 0 ); a_row_index < a->height; a_row_index++ )
         {
#ifdef PARALLEL 
           ParallelMatrixMult< T > job( 
                        a                                        /* matrix a */,
                        a_row_index * a->width                     /* a_start */,
                       (a_row_index * a->width) + a->width          /* a_end   */,
                        b_rotated                                 /* matrix b */,
                        b_row_index * b_rotated->width            /* b_start  */,
                       (b_row_index * b_rotated->width) + a->width /* b_end    */,
                        output                                    /* output mat */,
                        output_index                  /* self-explanatory */ );
            auto index( 0 );
            do{
               index = gen_index();
            } while( buffer_list[ index ]->space_avail() == 0 );
            buffer_list[ index ]->push_back( job );
#else
            for( size_t a_column_index( 0 ), 
               b_column_index( 0 ); a_column_index < a->width; 
                  a_column_index++, b_column_index++ )
            {
               output->matrix[ output_index ] += 
                  (a->matrix[ (a_row_index * a->width) + a_column_index ] *
                              b_rotated->matrix[ 
                                 (b_row_index * b_rotated->width) + 
                                    b_column_index ] );
            }
#endif            
            output_index += b->width;
         }
      }
#ifdef PARALLEL
      for( size_t i( 0 ); i < NUMWORKERS; i++ )
      {
         ParallelMatrixMult< T > finaljob( true );
         buffer_list[ i ]->push_back( finaljob ); 
      }
      for( auto *thread : thread_pool )
      {
         thread->join();
         delete( thread );
         thread = nullptr;
      }
      for( auto *buffer : buffer_list )
      {
         delete( buffer );
         buffer = nullptr;
      }
#endif
      delete( b_rotated );
      return( output );
   }


   static Matrix< T >* multiply( Matrix< T > &a, T val )
   {
      Matrix< T > *output = new Matrix< T >( a.height, a.width );
      if( val == 0 )
      {
         return( output );
      }
      const auto shift( count_one_bits( val ) );
      if( 
          ( typeid( T ) == typeid( uint64_t ) || 
            typeid( T ) == typeid( uint32_t ) ||
            typeid( T ) == typeid( uint16_t ) ||
            typeid( T ) == typeid( uint8_t  ) ) 
          && shift.first == 1  )
      {
         for( size_t i( 0 ); i < ( a.height * a.width ); i++ )
         {
            output->matrix[ i ] = 
            (*reinterpret_cast< uint64_t* >(&a.matrix[ i ])) << shift.second ;
         }
      }
      else
      {
         for( size_t i( 0 ); i < ( a.height * a.width ); i++ )
         {
            output->matrix[ i ] = a.matrix[ i ] * val;
         }
      }
      return( output );
   }
      
   static Matrix< T >* add( Matrix< T > &a, Matrix< T > &b ) 
   {
      if( a.height != b.height || a.width != b.width )
      {
         //TODO, again, throw an exception
         std::cerr << "Matrices must be of the same dimensionality\n";
         return( nullptr );
      }
      Matrix< T > *output = new Matrix< T >( a.height, a.width );
      for( size_t i( 0 ); i < ( a.height * b.width ); i++ )
      {
         output->matrix[ i ] = a.matrix[ i ] + b.matrix[ i ];
      }
      return( output );
   }

   static Matrix< T >* add( Matrix< T > &a, T val )
   {
      Matrix< T > *output = new Matrix< T >( a.height, a.width );
      for( size_t i( 0 ); i < ( a.height * a.width ); i++ )
      {
         output->matrix[ i ] = a.matrix[ i ] + val;
      }
      return( output );
   }
   

   

protected:
   static std::pair< int, 
                     int > count_one_bits( T val )
   {
      const uint64_t newval( (uint64_t) val );
      int pop_count(   __builtin_popcountll( newval ) );
      int trail_zeros( __builtin_ctzll( newval ) );
      return( std::make_pair( pop_count, 
                              trail_zeros ) );
   }
   

   static void mult_thread_worker( PBuffer *buffer )
   {
      
      while( true )
      {
         auto val( buffer->pop() );
         if( val.done )
         {
            return;
         }
         for( size_t a_index( val.a_start ), b_index( val.b_start );
               a_index < val.a_end && b_index < val.b_end;
                  a_index++, b_index++ )
         {
            val.output->matrix[ val.output_index ] +=
               val.a->matrix[ a_index ] *
                  val.b->matrix[ b_index ];
         }
      }
   }
};
#endif /* END _MATRIXOP_TCC_ */
