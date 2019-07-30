/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_file_lock.cpp
 COPYRIGHT    :   Yakov Markovitch, 2011-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for native_file_mutex

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   14 Nov 2011
*******************************************************************************/
#include <pcomn_syncobj.h>

#include <pcomn_sys.h>
#include <pcomn_path.h>
#include <pcomn_unittest.h>
#include <pcomn_string.h>
#include <pcomn_except.h>
#include <pcomn_fstream.h>
#include <pcomn_exec.h>

#include "pcomn_testhelpers.h"

#include <iostream>

using namespace pcomn::path ;
using namespace pcomn::sys ;
using namespace pcomn::unit ;

extern const char FILELOCK_FIXTURE[] = "filelock" ;

/*******************************************************************************
                     class NativeFileLockTests
*******************************************************************************/
class NativeFileLockTests : public pcomn::unit::TestFixture<FILELOCK_FIXTURE> {

      void Test_Exclusive_File_Lock() ;
      void Test_Shared_File_Lock() ;

      CPPUNIT_TEST_SUITE(NativeFileLockTests) ;

      CPPUNIT_TEST(Test_Exclusive_File_Lock) ;
      CPPUNIT_TEST(Test_Shared_File_Lock) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

void NativeFileLockTests::Test_Exclusive_File_Lock()
{
#ifdef PCOMN_PL_LINUX
   struct local {
         static void run_child(NativeFileLockTests &test, forkcmd &process)
         {
            if (!process.is_child())
               return ;

            try
            {
               CPPUNIT_LOG("\n-------- Child started --------" << std::endl) ;
               int fd = -1 ;
               for (int i = 10 ; i-- && fd < 0 ;)
               {
                  CPPUNIT_LOG("CHILD: -------- Attempting to open file " << test.at_data_dir("seq100.lst") << "..." << std::endl) ;
                  fd = ::open(test.at_data_dir("seq100.lst").c_str(), O_APPEND|O_WRONLY) ;
                  if (fd < 0 && errno != ENOENT)
                     PCOMN_THROW_SYSERROR("open") ;
                  msleep(300) ;
               }
               if (fd < 0)
               {
                  CPPUNIT_LOG("CHILD: -------- FAILURE" << std::endl) ;
                  exit(3) ;
               }
               CPPUNIT_LOG("CHILD: -------- OK" << std::endl) ;

               pcomn::binary_ofdstream stream (fd, true) ;
               msleep(500) ;
               CPPUNIT_LOG("CHILD: -------- Creating mutex" << std::endl) ;
               native_file_mutex mutex (fd, false) ;

               CPPUNIT_LOG_EQUAL(mutex.fd(), fd) ;
               CPPUNIT_LOG_IS_FALSE(mutex.owned()) ;
               CPPUNIT_LOG("CHILD: -------- Locking mutex" << std::endl) ;

               mutex.lock() ;

               const int n = 8 ;
               char buf[n + 1] ;
               int begin = 25, end = 50 ;
               while (begin < end)
               {
                  snprintf(buf, sizeof buf, "%*d\n", n - 1, begin++)  ;
                  stream.write(buf, n) ;
                  msleep(50) ;
               }

               CPPUNIT_LOG_ASSERT(mutex.unlock()) ;
               sleep(1) ;
               CPPUNIT_LOG_ASSERT(mutex.try_lock_shared()) ;
               checked_read_seqn_file<8>(test.at_data_dir("seq100.lst"), 0, 50) ;
               CPPUNIT_LOG_ASSERT(mutex.unlock()) ;

               CPPUNIT_LOG("CHILD: -------- Exiting" << std::endl) ;
               exit(0) ;
            }
            catch (const std::exception &x)
            {
               std::cerr << getpid() << " " << STDEXCEPTOUT(x) << std::endl ;
               exit(3) ;
            }
         }
   } ;

   CPPUNIT_LOG_EXCEPTION(native_file_mutex(-1, false), std::invalid_argument) ;
   CPPUNIT_LOG_EXCEPTION(native_file_mutex(NULL), std::invalid_argument) ;
   CPPUNIT_LOG_EXCEPTION(native_file_mutex("/tmppp/hello.world", O_RDONLY), pcomn::system_error) ;

   forkcmd writer_child ;
   local::run_child(*this, writer_child) ;

   native_file_mutex mx (at_data_dir("seq100.lst"), O_APPEND|O_WRONLY|O_CREAT|O_EXCL) ;
   CPPUNIT_LOG_ASSERT(mx.fd() >= 0) ;
   CPPUNIT_LOG_ASSERT(mx.owned()) ;

   CPPUNIT_LOG("******** Locking mutex" << std::endl) ;
   CPPUNIT_LOG_RUN(mx.lock()) ;
   CPPUNIT_LOG("******** Locked" << std::endl) ;

   pcomn::binary_ofdstream stream (mx.fd(), false) ;
   const int n = 8 ;
   char buf[n + 1] ;
   int begin = 0, end = 25 ;
   while (begin < end)
   {
      snprintf(buf, sizeof buf, "%*d\n", n - 1, begin++)  ;
      stream.write(buf, n) ;
      msleep(50) ;
   }

   CPPUNIT_LOG("******** Unlocking mutex" << std::endl) ;
   CPPUNIT_LOG_ASSERT(mx.unlock()) ;
   CPPUNIT_LOG("******** Unlocked" << std::endl) ;

   for (int i = 20 ; i-- ;)
   {
      msleep(200) ;
      if (!mx.try_lock_shared())
         CPPUNIT_LOG("******** Cannot acquire reader..." << std::endl) ;
      else
      {
         CPPUNIT_LOG("******** Acquired reader!" << std::endl) ;
         checked_read_seqn_file<8>(at_data_dir("seq100.lst"), 0, 50) ;
         CPPUNIT_ASSERT_EQUAL(writer_child.close(), 0) ;
         CPPUNIT_LOG_RUN(mx.unlock()) ;
         return ;
      }
   }
   CPPUNIT_ASSERT(false) ;
#endif
}

void NativeFileLockTests::Test_Shared_File_Lock()
{
   native_file_mutex mx_1 (at_data_dir("test.lock"), O_CREAT|O_WRONLY) ;
   native_file_mutex mx_2 (at_data_dir("test.lock")) ;

   CPPUNIT_LOG_ASSERT(mx_1.try_lock()) ;
   CPPUNIT_LOG_IS_FALSE(mx_2.try_lock()) ;
   CPPUNIT_LOG_IS_FALSE(mx_2.try_lock_shared()) ;

   // Demote to reader
   CPPUNIT_LOG_ASSERT(mx_1.try_lock_shared()) ;
   CPPUNIT_LOG_IS_FALSE(mx_2.try_lock()) ;
   CPPUNIT_LOG_ASSERT(mx_2.try_lock_shared()) ;

   CPPUNIT_LOG_ASSERT(mx_1.unlock()) ;
   CPPUNIT_LOG_ASSERT(mx_2.try_lock()) ;
   CPPUNIT_LOG_ASSERT(mx_2.unlock()) ;

#ifdef PCOMN_PL_LINUX
   {
      pcomn::binary_ofdstream stream (mx_1.fd(), false) ;
      generate_seqn<8>(stream, 0, 50) ;
   }
   native_file_mutex mx_3 (mx_1, O_RDONLY) ;
   pcomn::binary_ifdstream stream (mx_3.fd(), false) ;
   checked_read_seqn<8>(stream, 0, 50) ;

   CPPUNIT_LOG_ASSERT(mx_3.try_lock()) ;
   CPPUNIT_LOG_IS_FALSE(mx_1.try_lock()) ;
   CPPUNIT_LOG_IS_FALSE(mx_1.try_lock_shared()) ;
   CPPUNIT_LOG_ASSERT(mx_3.try_lock_shared()) ;
   CPPUNIT_LOG_IS_FALSE(mx_1.try_lock()) ;
   CPPUNIT_LOG_ASSERT(mx_1.try_lock_shared()) ;
#endif
}

int main(int argc, char *argv[])
{
   TestRunner runner ;
   runner.addTest(NativeFileLockTests::suite()) ;

   return
      run_tests(runner, argc, argv, NULL, "File mutex tests") ;
}
