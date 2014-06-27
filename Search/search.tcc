/**
 * search.tcc - 
 * @author: Jonathan Beard
 * @version: Thu Jun 19 14:07:49 2014
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
#ifndef _SEARCH_TCC_
#define _SEARCH_TCC_  1
#include <cassert>
#include <cstring>
#include <vector>
#include <functional>
#include <cmath>
#include <sstream>
#include <fstream>
#include <thread>
#include <utility>

#include "ringbuffer.tcc"

enum SearchAlgorithm 
{ 
   RabinKarp,
   KnuthMorrisPratt,
   Automata
};

#define CHUNKSIZE 100

struct Chunk
{
   Chunk() : start_position( 0 ),
             length( 0 )
   {
      std::memset( chunk, '\0', CHUNKSIZE ); 
   }

   Chunk( const Chunk &other ) : start_position( other.start_position ),
                                 length( other.length )
   {
      std::strncpy( chunk, other.chunk, CHUNKSIZE );
   }

   void operator = ( const Chunk &other )
   {
      start_position = other.start_position;
      length         = other.length;
      std::strncpy( chunk, other.chunk, CHUNKSIZE );
   }

   size_t start_position;
   size_t length;
   char   chunk[ CHUNKSIZE ];
};

struct Hit
{
   Hit() : position( 0 ),
           pattern_index( 0 )
   {
   }

   Hit( size_t position,
        size_t pattern_index ) : position( position ),
                                 pattern_index( pattern_index )
   {
   }

   Hit( const Hit &other ) : position( other.position ),
                             pattern_index( other.pattern_index )
   {

   }

   void operator = (const Hit &other )
   {
      position = other.position;
      pattern_index = other.pattern_index;
   }

   static std::string get_string( const Hit &hit )
   {
      std::stringstream ss;
      ss << "Position: " << hit.position << "Pattern #: " << hit.pattern_index;
      return( ss.str() );
   }
   size_t position;
   size_t pattern_index;
};

typedef RingBuffer< Chunk > InputBuffer;
typedef RingBuffer< Hit   > OutputBuffer;

template < size_t THREADS, 
           size_t BUFFSIZE = 100 > class Search
{
public:
   Search()          = delete;
   virtual ~Search() = delete;
   
   template< SearchAlgorithm algorithm >
   static void search( const std::string           filename,
                       std::vector< std::string >  &search_terms,
                       std::vector< std::string >  &hits )
   {


      std::array< std::thread*,  THREADS + 1 > thread_pool;
      std::array< InputBuffer*,  THREADS > input_buffer;
      std::array< OutputBuffer*, THREADS > output_buffer;
      
      /** allocate buffers **/
      for( size_t buff_id( 0 ); buff_id < THREADS; buff_id++ )
      {
         input_buffer  [  buff_id ] = new InputBuffer( BUFFSIZE );
         output_buffer [ buff_id  ] = new OutputBuffer( BUFFSIZE );
      }
      
      /** allocate consumer thread **/
      thread_pool[ THREADS ] = new std::thread( consumer_function,
                                                std::ref( output_buffer ),
                                                std::ref( hits ) );

      /** get input file **/
      std::ifstream file_input( filename, std::ifstream::binary );
      if( ! file_input.is_open() )
      {
         std::cerr << "Failed to open input file: " << filename << "\n";
         exit( EXIT_FAILURE );
      }
     
      /** declare iterations, needed to send stop signal **/
      size_t iterations( 0 );
      /**
       * get longest & smallest search term 
       */
      uint64_t largest_p( 0 );
      uint64_t smallest_p( INT64_MAX );
      for( const std::string &str : search_terms )
      {
         const auto temp( str.length() );
         if( largest_p < temp )
         {
            largest_p = temp;
         }
         if( smallest_p > temp )
         {
            smallest_p = temp;
         }
      }
      
      /**
       * get file length 
       */
      file_input.seekg( 0, file_input.end );
      const auto file_length( file_input.tellg() );
      file_input.seekg( 0, file_input.beg );

      /** declare the worker thread function **/
      std::function< void( InputBuffer*, OutputBuffer* ) > worker_function;
      uint64_t *m( nullptr );
      switch( algorithm )
      {
         case( RabinKarp ):
         {

            /** 
             * calculate number of iterations needed to cover
             * entire file, last 1 is 2 b/c we need to subtract
             * one for the single byte overlap at the end of 
             * the chunk
             */
            iterations =  
               std::round( (double) file_length / (double)( CHUNKSIZE - largest_p - 1 ) );
            
            const uint64_t q( 17 );
            const uint64_t d( 0xff );
            /**
             * hash_function - used to compute initial hashes
             * for pattern values "p"
             * @param - line, full line to be hashed
             * @param - length, length from start ( 0 ) to be 
             * hashed.  It is assumed that this function is only
             * used for the starts of lines.
             */
            auto hash_function = [&]( const std::string line, 
                                      const size_t      length )
            {
               uint64_t t( 0 );
               for( size_t i( 0 ); i < length; i++ )
               {
                  t = ( ( t * d ) + line[ i ] ) % q;
               }
               return( t );
            };
            /** store this since it'll be used quite a bit **/
            const auto n_patterns( search_terms.size() );
            

            auto compute_constant_data = [&](){
               std::vector< uint64_t > p( n_patterns, 0 );
               std::vector< uint64_t > h( n_patterns, 1 );
               for( auto i( 0 ); i < n_patterns; i++ )
               {
                  const auto pattern( search_terms[ i ] );
                  p[ i ] = hash_function( pattern, pattern.length() ); 
                  for( auto j( 1 ); j < n_patterns; j++ )
                  {
                     h[ i ] = ( h[ i ] * d ) % q;
                  }
               }
               return( std::make_pair( h, p ) );
            };
            const auto constant_data( compute_constant_data() );
            /**
             * h - max radix power to subtract off in rolling hash
             */
            const auto h( constant_data.first );
            /**
             * p - pattern hash value, only computed once and read
             * only after that
             */
            const auto p( constant_data.second );
            /** 
             * NOTE: if there are more than a few queries then might be 
             * more efficient to return a pointer.
             */
            m = new uint64_t[ n_patterns ]();
            for( size_t i( 0 ); i < n_patterns; i++ )
            {
               m[ i ] =  search_terms[ i ].length();
            }

            auto rkfunction = [&]( Chunk &chunk, std::vector< Hit > &hits )
            {
               /**
                * here's the game plan: 
                * 1) the thread shared patterns "p" are stored in a globally
                *    accessible variable "p" as uin64_t values.
                * 2) the max radix value to subtract off is stored for each
                *    pattern length in "h".
                * 3) here we need to compute the initial hash for each length 
                *    of pattern.
                * 4) then we have to keep track of when to stop for pattern, 
                *    obviously the longest is first but the shorter ones 
                */
               /** 
                * start by calculating initial hash values for line for each
                * of the pattern lengths, might be just as easy to "roll" 
                * different lengths, but this seems like it might be a bit
                * faster to just keep |search_terms| hash values for each 
                * pattern.
                */
               uint64_t *t = new uint64_t[ n_patterns ];
               for( auto pattern( 0 ); pattern < n_patterns; pattern++ )
               {
                  t[ pattern ] = hash_function( chunk.chunk, 
                                                search_terms[ pattern ].length() );
               }
               /** temp value for use below **/
               Hit temp;
               /** increment var for do loop below **/
               size_t s( 0 );
               do{
                  for( auto p_index( 0 ); p_index < search_terms.size(); p_index++ )
                  {
                     if( s <= ( CHUNKSIZE - m[ p_index ] /* pattern length */ ) )
                     {
                        if( p[ p_index ] == t[ p_index ] )
                        {
                           temp.position      = s + chunk.start_position;
                           temp.pattern_index = p_index;
                           hits.push_back( temp /* make copy */ );
                        }
                        /** calculate new offsets for t[ p_index ] **/
                        const auto remove_val( ( chunk.chunk[ s ] * h[ p_index ] ) % q );
                        t[ p_index ] = ( t[ p_index ] - remove_val ) % q;
                        t[ p_index ] = ( d * t[ p_index ] ) % q;
                        t[ p_index ] = ( t[ p_index ] + chunk.chunk[ s + m[ p_index ] ] ) % q;
                     }
                  }
                  s++;
                  /** stop when we're at the end of the smallest pattern **/
               }while( s <= ( CHUNKSIZE - smallest_p ) );
               delete[]( t );
            };
            worker_function = 
               std::bind( worker_function_base, 
                          std::placeholders::_1, 
                          std::placeholders::_2, 
                          std::ref( rkfunction ) );
         }
         break;
         default:
            assert( false );
      }
      
      
      for( size_t thread_id( 0 ); thread_id < THREADS; thread_id++ )
      {
         thread_pool[ thread_id ] = new std::thread( worker_function,
                                                     input_buffer[  thread_id ],
                                                     output_buffer[ thread_id ] );
      }
   
      size_t output_stream( 0 );
      while( file_input.good() )
      {
         /** input stream with reference to the worker thread **/
         auto *input_stream( input_buffer[ output_stream ] );
         Chunk &chunk( input_stream->allocate() );
         chunk.start_position = file_input.tellg();
         file_input.read( chunk.chunk, CHUNKSIZE );
         chunk.length = ( size_t )file_input.gcount();
         input_stream->push( ( --iterations > THREADS ? RBSignal::NONE : RBSignal::RBEOF ) );
         file_input.seekg( (size_t) file_input.tellg() - largest_p + 1 );
         output_stream = ( output_stream + 1 ) % THREADS;
      }
      
      /** end of file, wait for results **/
      for( std::thread *th : thread_pool )
      {
         th->join();
         delete( th );
         th = nullptr;
      }

      for( InputBuffer *buff : input_buffer )
      {
         delete( buff );
         buff = nullptr;
      }

      for( OutputBuffer *buff : output_buffer )
      {
         delete( buff );
         buff = nullptr;
      }
      
      if( m != nullptr ) 
      { 
         delete[]( m );
         m = nullptr;
      }
      file_input.close();
   }
private:

   static void worker_function_base( InputBuffer *input, 
                                     OutputBuffer *output,
                                     std::function< 
                                     void ( Chunk&, 
                                            std::vector< Hit >& ) > 
                                             search_function )
   {
      assert( input != nullptr );
      assert( output != nullptr );
      std::vector< Hit > local_hits;
      Chunk chunk;
      RBSignal signal( RBSignal::NONE );
      while( signal != RBSignal::RBEOF )
      {
         input->pop( chunk, &signal );
         search_function( chunk, local_hits );
         if( local_hits.size() > 0 )
         {
            output->insert( local_hits.begin(), local_hits.end(), signal );
            local_hits.clear();
         }
      }
      /** we're at the end of file, send term signal **/
      output->send_signal( RBSignal::TERM );
      return;
   }

   static void consumer_function( std::array< OutputBuffer*, THREADS > &input,
                                  std::vector< std::string >           &hits )
   {
      int sig_count( 0 );
      Hit hit;
      RBSignal sig( RBSignal::NONE );
      while( sig_count < THREADS )
      {
         for( auto *buff : input )
         {
            if( buff->size() > 0 )
            {
               buff->pop( hit, &sig );
               hits.push_back( Hit::get_string( hit ) );
               if( sig == RBSignal::RBEOF )
               {
                  sig_count++;
               }
            }
            if( buff->get_signal() == RBSignal::TERM )
            {
               sig_count++;
            }
         }
      }
      return;
   }
};
#endif /* END _SEARCH_TCC_ */
