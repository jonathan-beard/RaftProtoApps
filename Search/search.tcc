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

enum SearchAlgorithm 
{ 
   RabinKarp,
   KnuthMorrisPratt,
   Automata
};

#define CHUNKSIZE 65536
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

   Hit( const Hit &other ) : position( other.position ),
                             pattern_index( other.pattern_index )
   {

   }

   void operator = (const Hit &other )
   {
      position = other.position;
      pattern_index = other.pattern_index;
   }

   size_t position;
   size_t pattern_index;
};

typedef RingBuffer< Line > InputBuffer;
typedef RingBuffer< Hit  > OutputBuffer;

template < SearchAlgorithm algorithm, 
           size_t THREADS, 
           size_t BUFFSIZE = 100 > class Search
{
public:
   Search()          = delete;
   virtual ~Search() = delete;

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
         input_buffers[  buff_id ] = new InputBuffer( BUFFSIZE );
         output_buffers[ buff_id ] = new OutputBuffer( BUFFSIZE );
      }
      /** allocate a place to store the matches **/
      std::vector< Hit > hits;
      /** allocate consumer thread **/
      thread_pool[ THREADS ] = new std::thread( consumer_function,
                                                std::ref( output_buffer ),
                                                std::ref( hits ) )

      /** get input file **/
      std::ifstream file_input( filename, std::ifstream::binary );
      if( ! file_input.is_open() )
      {
         std::cerr << "Failed to open input file: " << filename << "\n";
         exit( EXIT_FAILURE );
      }
     
      /** declare iterations, needed to send stop signal **/
      size_t iterations( 0 );

      /** declare the worker thread function **/
      std::function< void( InputBuffer*, OutputBuffer* ) > worker_function;
      switch( algorithm )
      {
         case( RabinKarp ):
         {
            /**
             * get longest search term 
             */
            size_t m( 0 );
            for( const std::string &str : search_terms )
            {
               const auto l( str.length() );
               if( m < l )
               {
                  m = l;
               }
            }
            m -= 1;
            /**
             * get file length 
             */
            is.seekg( 0, is.end );
            const auto file_length( is.tellg() );
            is.seekg( 0, is.beg );

            /** 
             * calculate number of iterations needed to cover
             * entire file
             */
            iterations =  
               std::round( (double) length / (double)( CHUNKSIZE - m - 2 ) );
            
            const uint64_t q( 17 );
            const uint64_t d( 0xff );
            /**
             * h - max radix power to subtract off in rolling hash
             */
            std::vector< uint64_t > h( search_terms.length(), 1);
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
            }
            /**
             * p - pattern hash value, only computed once and read
             * only after that
             */
            std::vector< uint64_t > p( search_terms.length(), 0 );
            for( const std::string &str :  search_terms )
            {
               
            }
            /**
             * compute initial hashes for each search term
             */

            auto rkfunction = [&]( Line &line, std::vector< Hit > &hits )
            {
               /** re-start hash **/ 
            };
            worker_function = 
               std::bind( worker_function_base, _1, _2, std::ref( rkfunction ) );
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
         auto *buffer( input_buffer[ output_stream ] );
         Line &line( buffer->allocate() );
         input_file.read( line.line_chunk );
         output_stream = ( output_stream + 1 ) % THREADS;
      }
      
      file_input.close();
   }
private:

   static void worker_function_base( InputBuffer *input, 
                                     OutputBuffer *output,
                                     std::function< 
                                     void ( Line&, 
                                            std::vector< Hit >& ) > 
                                             search_function )
   {
      assert( input != nullptr );
      assert( output != nullptr );
      bool exit( false );
      std::vector< Hit > local_hits;
      bool has_data( false );
      Line line;
      RBSignal signal( RBSignal::NONE );
      while( signal != RBSignal::RBEOF )
      {
         input->pop( line, &signal );
         search_function( line, local_hits );
         if( local_hits.size() > 0 )
         {
            output->insert( local_hits.begin(), local_hits.end(), signal );
            local_hits.clear();
         }
      }
   }

   static void consumer_function( std::array< OutputBuffer*, THREADS > &input,
                                  std::vector< Hit >                   &hits )
   {

   }
};
#endif /* END _SEARCH_TCC_ */
