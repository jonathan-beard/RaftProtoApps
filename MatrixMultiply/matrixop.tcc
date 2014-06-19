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

//#define MONITOR      1
#define PARALLEL     1


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
   ParallelMatrixMult(  Matrix< Type >  *a, 
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
                                              output_index( output_index )
   {
      /** nothing to do here **/
   }

   ParallelMatrixMult( )           : a( nullptr ),
                                     a_start( 0 ),
                                     a_end( 0 ),
                                     b( nullptr ),
                                     b_start( 0 ),
                                     b_end( 0 ),
                                     output( nullptr ),
                                     output_index( 0 )
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
      output_index   ( other.output_index )
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
      return( *this ); 
   }
   
   Matrix< Type > *a;
   size_t a_start;
   size_t a_end;
   Matrix< Type > *b;
   size_t b_start;
   size_t b_end;
   Matrix< Type >  *output;
   size_t output_index;
};

template < typename Type > struct  OutputValue
{
   OutputValue() : index( 0 ),
                   value( 0 )
   {
   }

   OutputValue( const OutputValue &other )
   {
      index   = other.index;
      value   = other.value;
   }

   void operator = ( const OutputValue &other )
   {
      index   = other.index;
      value   = other.value;
   }

   size_t   index;
   Type     value;
};


template< typename T, size_t THREADS = 1 > class MatrixOp {
public:
   MatrixOp() = delete;
   virtual ~MatrixOp() = delete;
#if MONITOR == 1
   typedef RingBuffer< ParallelMatrixMult< T >, 
                       RingBufferType::Heap, 
                       true >                      PBuffer;
   
   typedef RingBuffer< OutputValue< T >,
                       RingBufferType::Heap,
                       true >                      OutputBuffer;
#else   
   typedef RingBuffer< ParallelMatrixMult< T > >   PBuffer;
   typedef RingBuffer< OutputValue< T > >          OutputBuffer;
#endif
   
   static void multiply(   Matrix< T > *a, 
                           Matrix< T > *b,
                           Matrix< T > *output ) 
                                 
   {
      assert( a != nullptr );
      assert( b != nullptr );
      assert( output != nullptr );
      if( a->width != b->height )
      {
         //TODO make some proper exceptions
         std::cerr << "Matrix a's width must equal matrix b's height.\n";
         return;
      }
      
      std::array< std::thread*, THREADS + 1 /* consumer thread */ > thread_pool;
      std::array< PBuffer*,      THREADS >    buffer_list;
      std::array< OutputBuffer*, THREADS >    output_list;
      for( size_t i( 0 ); i < THREADS; i++ )
      {
         buffer_list[ i ] = new PBuffer( 100 );
         output_list[ i ] = new OutputBuffer( 100 );
      }
      for( size_t i( 0 ); i < THREADS; i++ )
      {
         thread_pool[ i ] = new std::thread( mult_thread_worker,
                                             buffer_list[ i ],
                                             output_list[ i ] );
      }
      thread_pool[ THREADS ] = new std::thread( mult_thread_consumer,
                                                    std::ref( output_list ),
                                                    output );
      
      Matrix< T > *b_rotated = b->rotate();
      int64_t stop_index( b->height * b->width );
      uint32_t index( 0 );
      for( size_t b_row_index( 0 ); 
            b_row_index < b_rotated->height; b_row_index++ )
      {
         size_t output_index( b_row_index );
         for( size_t a_row_index( 0 ); a_row_index < a->height; a_row_index++ )
         {
            do{
               index = (index + 1 ) % THREADS ;
            } while( buffer_list[ index ]->space_avail() == 0 );
            auto &mem( buffer_list[ index ]->allocate() );
            
            mem.a                            = a;
            mem.a_start                      = a_row_index * a->width;
            mem.a_end                        = (a_row_index * a->width ) + a->width;
            mem.b                            = b_rotated;
            mem.b_start                      = b_row_index * b_rotated->width;
            mem.b_end                        = (b_row_index * b_rotated->width ) + a->width;
            mem.output                       = output;
            mem.output_index                 = output_index;

            if( stop_index-- > THREADS )
            {
               buffer_list[ index ]->push();
            }
            else
            {
               /** go ahead and tell the threads we used to shutdown **/
               buffer_list[ index ]->push( RBSignal::RBEOF );
            }
            output_index += b->width;
         }
      }
      /**
       * just in case a user specifies more threads than are
       * used send asynchronous shutdown signal to rest of 
       * threads through FIFO
       */
      for( auto *buffer : buffer_list )
      {
         buffer->send_signal( RBSignal::RBEOF );
      }

      /** join threads **/
      for( auto *thread : thread_pool )
      {
         thread->join();
         delete( thread );
         thread = nullptr;
      }
      /** get info **/
      for( auto *buffer : buffer_list )
      {
#if MONITOR         
         auto &monitor_data( buffer->getQueueData() );
         Monitor::QueueData::print( monitor_data, Monitor::QueueData::MB, std::cerr, true );
         std::cerr << "\n";
#endif         
         delete( buffer );
         buffer = nullptr;
      }
      
      for( auto *buffer : output_list )
      {
#if MONITOR         
         auto &monitor_data( buffer->getQueueData() );
         Monitor::QueueData::print( monitor_data, Monitor::QueueData::MB, std::cerr, true );
         std::cerr << "\n";
#endif         
         delete( buffer );
         buffer = nullptr;
      }
      delete( b_rotated );
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
   

   static void mult_thread_worker( PBuffer *buffer, OutputBuffer *output )
   {
      assert( buffer != nullptr );
      assert( output != nullptr );
      bool exit( false );
      OutputValue< T > scratch;
      ParallelMatrixMult< T > val;
      while( ! exit || buffer->size() > 0  )
      {
         /** consume data **/
         buffer->pop( val );
         scratch.index = val.output_index;

         for( size_t a_index( val.a_start ), b_index( val.b_start );
               a_index < val.a_end && b_index < val.b_end;
                  a_index++, b_index++ )
         {
            scratch.value +=
               val.a->matrix[ a_index ] *
                  val.b->matrix[ b_index ];
         }
         output->push( scratch /** make a copy **/ );
         if( ! exit )
         {
            exit = ( buffer->get_signal() == RBSignal::RBEOF );
         }
         scratch.value = 0;
      }
      /** time to exit **/
      output->send_signal( RBSignal::RBEOF );
      return;
   }

   static void mult_thread_consumer( std::array< OutputBuffer*, THREADS > &buffer,
                                     Matrix< T > *output )
   {
      bool exit( false );
      OutputValue< T > data;
      while( ! exit  )
      {
         for( auto it( buffer.begin() ); it != buffer.end(); ++it )
         {
            /** start checking for data **/
            exit = true;
            if( (*it)->size() > 0 )
            {
               /** do something with the data **/
               (*it)->pop( data );
               fprintf( stderr, "%ld\n", data.index );
               output->matrix[ data.index ] = data.value;
            }
            if((*it)->get_signal() != RBSignal::RBEOF )
            {
               exit = false;
            }
         }
      }
      return;
   }
};
#endif /* END _MATRIXOP_TCC_ */
