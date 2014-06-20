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

enum SearchAlgorithm 
{ 
   RabinKarp,
   KnuthMorrisPratt,
   Automata
};

#define CHUNKSIZE 1007

struct Line
{
   Line() : line_no( 0 ),
            start_position( 0 ),
            length( 0 )
   {
      std::memset( line_chunk, '\0', CHUNKSIZE ); 
   }

   Line( const Line &other ) : line_no( other.line_no ),
                               start_position( other.start_position ),
                               length( other.length )
   {
      std::strncpy( line_chunk, other.line_chunk, CHUNKSIZE );
   }

   void operator = ( const Line &other )
   {
      line_no        = other.line_no;
      start_position = other.start_position;
      length         = other.length;
      std::strncpy( line_chunk, other.line_chunk, CHUNKSIZE );
   }

   size_t line_no;
   size_t start_position;
   size_t length;
   char   line_chunk[ CHUNKSIZE ];
};

struct Hit
{
   Hit() : line_no( 0 ),
           position( 0 )
   {
   }

   Hit( const Hit &other ) : line_no( other.line_no ),
                             position( other.position )
   {

   }

   void operator = (const Hit &other )
   {
      line_no  = other.line_no;
      position = other.position;
   }

   size_t line_no;
   size_t position;
};

typedef RingBuffer< Line > InputBuffer;
typedef RingBuffer< Hit  > OutputBuffer;

template < SearchAlgorithm algorithm, size_t THREADS, size_t BUFFSIZE = 100 > class Search
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

      std::function< void( InputBuffer*, OutputBuffer* ) > worker_function;
      switch( algorithm )
      {
         case( RabinKarp ):
         {
            const auto prime_number( 3571 );
            auto rk_hash = [&]( const char *src, size_t len )
            {  
               uint64_t output( 0 );
               while( len )
               {
                  output += src[ len ] * std::pow( prime_number, len );
                  len--;
               }
            };
            auto rkfunction = [&]( Line &line, std::vector< Hit > &hits )
            {
                
            };
            worker_function = std::bind( worker_function_base, _1, _2, std::ref( rkfunction ) );
         }
         break;
         case( KnuthMorrisPratt ):
         {
            auto kmpfunction = []( Line &line, std::vector< Hit > &hits )
            {
               
            };
            worker_function = std::bind( worker_function_base, _1, _2, std::ref( kmpfunction ) );
         }
         break;
         case( Automata ):
         {
            auto atfunction = []( Line &line, std::vector< Hit > &hits )
            {
               
            };
            worker_function = std::bind( worker_function_base, _1, _2, std::ref( atfunction ) );
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
   }
private:

   static void worker_function_base( InputBuffer *input, 
                                     OutputBuffer *output,
                                     std::function< void ( Line&, std::vector< Hit >& ) > search_function )
   {
      assert( input != nullptr );
      assert( output != nullptr );
      bool exit( false );
      std::vector< Hit > local_hits;
      bool has_data( false );
      Line line;
      RBSignal &sig( buffer->get_signal() );
      while( ! exit || ( has_data = buffer->size() > 0 ) )
      {
         if( has_data )
         {
            input->pop( line );
            search_function( line, local_hits );
            sig = buffer->get_signal();
            if( local_hits.size() > 0 )
            {
               for( auto it( local_hits.begin() ); it != local_hits.end(); ++it )
               {
                  
                  if( it == ( local_hits.end() - 1 ) )
                  {
                     /** pass signal **/
                     output->push( (*it), sig );
                  }
                  else
                  {
                     output->push( (*it) );
                  }
               }
               local_hits.clear();
            }
            else
            {
               /** no hits, pass signal on **/
               output->send_signal( sig );
            }
         }
         exit |= (sig == RBSignal::RBEOF );
      }
   }

   static void consumer_function( std::array< OutputBuffer*, THREADS > &input,
                                  std::vector< Hit >                   &hits )
   {

   }
};
#endif /* END _SEARCH_TCC_ */
