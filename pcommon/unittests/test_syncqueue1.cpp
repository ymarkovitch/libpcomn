/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_syncqueue1.cpp
 COPYRIGHT    :   Yakov Markovitch, 2001-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   The test of the synchronous producer-consumer queue

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   9 Jul 2001
*******************************************************************************/
#include <pcomn_syncqueue.h>
#include <pcomn_thread.h>
#include <pcomn_trace.h>

#include <pcomn_ivector.h>
#include <set>
#include <fstream>
#include <iterator>

using namespace pcomn ;

typedef std::set<std::string> string_set ;
typedef synchronized_queue<std::string> string_queue ;

static string_set &read_strings(string_set &s, char **filenames)
{
   while(*filenames)
   {
      TRACEP("Reading file " << *filenames) ;
      std::ifstream f (*filenames) ;
      if (!f)
         throw std::runtime_error("Cannot open file " + std::string(*filenames)) ;
      std::string result ;
      while(f >> result)
         s.insert(result) ;
      ++filenames ;
   }
   return s ;
} ;

class StrConsumer : public BasicThread {
   public:
      StrConsumer(string_set &s, string_queue &q) :
         _queue(q),
         _result(s)
      {
         start(StartSuspended) ;
      }

   protected:
      int run()
      {
         try
         {
            TRACEP("Reading queue...") ;
            //int popped = 0 ;
            while(1)
            {
               //TRACEP("Popped=" << popped << " Popping... ") ;
               std::string str (_queue.pop()) ;
               //++popped ;
               //TRACEP("Popped " << str) ;
               //Sleep(5) ;
               _result.insert(str) ;
            }
         }
         catch(const pcomn::object_closed &)
         {
            std::cout << "Queue has been closed" << std::endl ;
            return 1 ;
         }
         catch(const std::exception &x)
         {
            std::cout << "Exception: " << x.what() << std::endl ;
         }
         return 0 ;
      }
   private:
      string_queue & _queue ;
      string_set &   _result ;
} ;

class StrProducer : public BasicThread {
   public:
      StrProducer(const char *filename, string_queue &q) :
         _queue(q),
         _filename(filename),
         _stream(filename)
      {
         if (!_stream)
            throw std::runtime_error("Cannot open file " + _filename) ;
         start(StartSuspended) ;
      }

   protected:
      int run()
      {
         try
         {
            TRACEP("Reading file " << _filename << "...") ;
            std::string result ;
            while(_stream >> result)
            {
               //TRACEP("Pushing " << result) ;
               _queue.push(result) ;
            }
            return 1 ;
         }
         catch(const std::exception &x)
         {
            std::cout << "Exception: " << x.what() << std::endl ;
         }
         return 0 ;
      }
   private:
      string_queue & _queue ;
      std::string    _filename ;
      std::ifstream  _stream ;
} ;

static string_set &read_strings_async(string_set &s, char **filenames)
{
   string_queue sq(10) ;
   ivector<StrProducer> producers(0, true) ;
   StrConsumer consumer(s, sq) ;

   while(*filenames)
      producers.push_back(new StrProducer(*filenames++, sq)) ;
   TRACEP(producers.size() << " producers created. Starting producers...") ;
   for(unsigned i = 0 ; i < producers.size() ; producers[i++]->start()) ;
   consumer.start() ;
   for(unsigned i = 0 ; i < producers.size() ; ++i)
   {
      int result = producers[i]->join() ;
      TRACEP("The producer thread " << *producers[i] << " returned " << result) ;
   }
   TRACEP("Closing the producer end of the queue...") ;
   sq.close() ;
   //sq.terminate() ;
   TRACEP("OK") ;
   TRACEP("Waiting for consumer to end...") ;
   int result = consumer.join() ;
   TRACEP("OK") ;
   TRACEP("The consumer thread " << consumer << " returned " << result) ;
   return s ;
}

int main(int argc, char *argv[])
{
   if (argc < 2)
   {
      std::cerr << "Usage: " << PCOMN_PROGRAM_SHORTNAME << " file ..." << std::endl ;
      return 1 ;
   }

   DIAG_INITTRACE("pcomntest.ini") ;

   try
   {
      string_set synchronously ;
      string_set asynchronously ;

      std::cout << "Reading files synchronously:" << std::endl ;
      read_strings(synchronously, argv + 1) ;
      std::cout << synchronously.size() << " unique words have been read" << std::endl ;

      std::cout << "Reading files asynchronously:" << std::endl ;
      read_strings_async(asynchronously, argv + 1) ;
      std::cout << asynchronously.size() << " unique words have been read" << std::endl ;

      if (synchronously == asynchronously)
      {
         std::cout << "OK. Sets are equal." << std::endl ;
         return 0 ;
      }
      std::cout << "FAILURE! Sets are unequal." << std::endl ;
      return 1 ;
   }
   catch(const std::exception &x)
   {
      std::cout << "Exception " << PCOMN_TYPENAME(x) << ": " << x.what() << std::endl ;
   }
   return 1 ;
}
