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
#include "ParallelMatrixMult.tcc"
#include "Matrix.tcc"

#define MONITOR      1


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
                       RingBufferType::Infinite, 
                       true >                      PBuffer;
   
   typedef RingBuffer< OutputValue< T >,
                       RingBufferType::Infinite,
                       true >                      OutputBuffer;

#else   
   typedef RingBuffer< ParallelMatrixMult< T > >   PBuffer;
   typedef RingBuffer< OutputValue< T > >          OutputBuffer;
#endif
   
   /**
    * multiply - takes in matrices a and b, stores the output
    * in matrix "output."  Uses the number of threads specified
    * in the class template parameter when referencing this static
    * function.  The current implementation uses a relatively 
    * naive dot product / blocking implementation, however this will
    * slowly change once the transition to the Raft framework is 
    * complete.
    * @param   a, Matrix< T >*
    * @param   b, Matrix< T >*
    * @param   output, Matrix< T >* - must be of the correct dimensions
    */
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
         buffer_list[ i ] = new PBuffer(        100 );
         output_list[ i ] = new OutputBuffer(   100 );
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
            index = (index + 1 ) % THREADS ;
            auto &mem( buffer_list[ index ]->allocate() );
            mem.a            = a;
            mem.a_start      = a_row_index * a->width;
            mem.a_end        = (a_row_index * a->width ) + a->width;
            mem.b            = b_rotated;
            mem.b_start      = b_row_index * b_rotated->width;
            mem.b_end        = (b_row_index * b_rotated->width ) + a->width;
            mem.output       = output;
            mem.output_index = output_index;
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
      ParallelMatrixMult< T > val;
      RBSignal sig( RBSignal::NONE );
      while( sig != RBSignal::RBEOF )
      {
         /** consume data **/
         buffer->pop( val, &sig );

         OutputValue< T > &scratch( output->allocate() );
         scratch.index = val.output_index;

         for( size_t a_index( val.a_start ), b_index( val.b_start );
               a_index < val.a_end && b_index < val.b_end;
                  a_index++, b_index++ )
         {
            scratch.value +=
               val.a->matrix[ a_index ] *
                  val.b->matrix[ b_index ];
         }
         output->push( sig );
         scratch.value = 0;
      }
      return;
   }

   static void mult_thread_consumer( std::array< OutputBuffer*, THREADS > &buffer,
                                     Matrix< T > *output )
   {
      int sig_count( 0 );
      OutputValue< T > data;
      RBSignal sig( RBSignal::NONE );
      /** TODO, change THREADS to used threads so that buffers
       *  that aren't used will still be terminated 
       */
      while( sig_count <  THREADS )
      {
         for( auto it( buffer.begin() ); it != buffer.end(); ++it )
         {
            if( (*it)->size() > 0 )
            {
               (*it)->pop( data, &sig );
               output->matrix[ data.index ] = data.value;
               if( sig == RBSignal::RBEOF )
               {
                  sig_count++;
               }
            }
         }
      }
      return;
   }
};
#endif /* END _MATRIXOP_TCC_ */
