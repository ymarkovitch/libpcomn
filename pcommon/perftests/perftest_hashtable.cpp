/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   perftest_hashtable.cpp
 COPYRIGHT    :   Yakov Markovitch, 2016-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Compare performance of std::unordered_map and closed_hashtable.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   9 Aug 2016
*******************************************************************************/
#include <pcomn_hashclosed.h>
#include <pcomn_stopwatch.h>
#include <pcomn_unittest.h>
#include <pcomn_hash.h>

#include <numeric>
#include <unordered_map>

#include <malloc.h>

using namespace pcomn ;
using namespace std ;

typedef pair<md5hash_t, int64_t> data_type ;

static vector<md5hash_t> hit_data ;
static vector<md5hash_t> miss_data ;

static vector<data_type> insert_order ;

static const size_t MAX_DATA_COUNT = 20000000 ;

static void prepare_data(size_t count)
{
   std::cerr << "Preparing " << count << " test data points..." << std::endl ;
   if (!count || count > MAX_DATA_COUNT)
   {
      if (!count)
         std::cerr << "Zero data count specified" << std::endl ;
      else
         std::cerr << "Data count is too large (" << MAX_DATA_COUNT << "), maximum allowed is " << MAX_DATA_COUNT << std::endl ;
      exit(3) ;
   }

   hit_data.reserve(count) ;
   miss_data.reserve(count) ;
   insert_order.reserve(count) ;

   size_t data[32] ;
   for (size_t i = 0 ; i < count ; ++i)
   {
      iota(begin(data), end(data), i) ;
      hit_data.push_back(md5hash(data, sizeof data)) ;
      insert_order.emplace_back(hit_data.back(), i) ;

      iota(begin(data), end(data), i + count) ;
      miss_data.push_back(md5hash(data, sizeof data)) ;
   }

   std::cerr << "OK" << std::endl ;

   std::cerr << "Generating random insert order..." << std::endl ;

   random_shuffle(begin(insert_order), end(insert_order)) ;

   std::cerr << "OK" << std::endl ;
}


template<typename Table>
static void run_hashtable(size_t rounds)
{
   PCpuStopwatch stopwatch ;
   for (size_t r = 0 ; r < rounds ; ++r)
   {
      size_t hits = 0 ;
      size_t misses = 0 ;
      double hit_interval ;
      double miss_interval ;
      double build_interval ;

      std::cout << PCOMN_TYPENAME(Table) << ", round " << r+1 << std::endl ;
      malloc_stats() ;
      {
         Table table ; //(insert_order.size() + 1024) ;

         std::cout << "inserting  " << insert_order.size() << " items..." << std::endl ;

         stopwatch.start() ;
         for (const auto &d: insert_order)
            table.insert(d) ;
         build_interval = stopwatch.stop() ;

         std::cout << build_interval << "s" << std::endl ;
         malloc_stats() ;

         std::cout << "searching..." << std::endl ;

         stopwatch.restart() ;
         for (const auto &d: hit_data)
            hits += table.count(d) ;
         hit_interval = stopwatch.stop() ;

         std::cout << hits << " hits for " << hit_interval << "s" << std::endl ;

         stopwatch.restart() ;
         for (const auto &d: miss_data)
            misses += (unsigned)!table.count(d) ;
         miss_interval = stopwatch.stop() ;

         std::cout << hits << " misses for " << miss_interval << "s" << std::endl ;

         std::cout << "destroying table..." << std::endl ;

         stopwatch.restart() ;
      }
      double destroy_interval = stopwatch.reset() ;

      std::cout << "\nDestroyed for " << destroy_interval << "s" << std::endl ;

      if (!hit_interval || !miss_interval || !destroy_interval)
         continue ;

      std::cout << hits/hit_interval << " hits/s\n"
                << misses/miss_interval << " misses/s\n"
                << insert_order.size()/build_interval << " inserts/s\n"
                << std::endl ;
   }
}

struct data_state_extractor {

      static bucket_state state(const data_type &v)
      {
         return inrange<int64_t>(v.second, -(int64_t)bucket_state::End, 0)
            ?  (bucket_state)-v.second : bucket_state::Valid ;
      }

      static data_type value(bucket_state s)
      {
         return {{}, -(int)s} ;
      }
} ;

/*******************************************************************************

*******************************************************************************/
int main(int argc, char *argv[])
{
   if (argc != 3)
   {
      std::cerr << "Usage: " << PCOMN_PROGRAM_SHORTNAME << " DATA_COUNT ROUND_COUNT" << std::endl ;
      return 1 ;
   }

   size_t count = atoll(argv[1]) ;
   size_t rounds = atoll(argv[2]) ;

   prepare_data(count) ;

   typedef closed_hashtable<data_type, pcomn::select<0>, void, void, data_state_extractor> pcommon_hashtable ;
   typedef std::unordered_map<md5hash_t, int64_t> stl_hashtable ;

   run_hashtable<pcommon_hashtable>(rounds) ;
   run_hashtable<stl_hashtable>(rounds) ;

   return 0 ;
}
