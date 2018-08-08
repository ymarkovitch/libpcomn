/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_xattr.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test for LINUX filesystem extended attributes.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Sep 2008
*******************************************************************************/
#include <pcomn_unittest.h>
#include <pcomn_safeptr.h>
#include <pcomn_handle.h>
#include <unix/pcomn_xattr.h>

#include <sys/stat.h>
#include <unistd.h>

const char TSTFILE[] = "XattrTests.lst" ;

/*******************************************************************************
                     class XattrFail
*******************************************************************************/
class XattrFail : public CppUnit::TestFixture {
   private:
      void Test_Xattr_AlwaysFail() ;

      CPPUNIT_TEST_SUITE(XattrFail) ;

      CPPUNIT_TEST(Test_Xattr_AlwaysFail) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

void XattrFail::Test_Xattr_AlwaysFail()
{
}

/*******************************************************************************
                     class XattrTests
*******************************************************************************/
class XattrTests : public CppUnit::TestFixture {
      void Test_FileXattr() ;

      CPPUNIT_TEST_SUITE(XattrTests) ;

      CPPUNIT_TEST(Test_FileXattr) ;

      CPPUNIT_TEST_SUITE_END() ;

   public:
      void setUp()
      {
         unlink(TSTFILE) ;
      }
} ;

void XattrTests::Test_FileXattr()
{
   CPPUNIT_LOG_ASSERT(access(TSTFILE, F_OK) != 0) ;
   CPPUNIT_LOG_RUN(pcomn::fd_safehandle(PCOMN_ENSURE_POSIX(::open(TSTFILE, O_CREAT|O_WRONLY|O_EXCL, 0644), "open"))) ;
   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(access(TSTFILE, W_OK), "access")) ;

   pcomn::fd_safehandle File ;
   CPPUNIT_LOG_RUN(File.reset(PCOMN_ENSURE_POSIX(::open(TSTFILE, O_WRONLY), "open"))) ;
   CPPUNIT_LOG_ASSERT(pcomn::xattr_supported(TSTFILE)) ;
   CPPUNIT_LOG_ASSERT(pcomn::xattr_supported(File.handle())) ;
   CPPUNIT_LOG_IS_FALSE(pcomn::has_xattr(TSTFILE, "user.foobar")) ;
   CPPUNIT_LOG_IS_FALSE(pcomn::has_xattr(File.handle(), "user.foobar")) ;
   CPPUNIT_LOG_ASSERT(pcomn::xattr_set(pcomn::XA_CREATE, TSTFILE, "user.foobar", "")) ;
   CPPUNIT_LOG_ASSERT(pcomn::has_xattr(TSTFILE, "user.foobar")) ;
   CPPUNIT_LOG_ASSERT(pcomn::has_xattr(File.handle(), "user.foobar")) ;
   CPPUNIT_LOG_IS_FALSE(pcomn::xattr_set(pcomn::XA_CREATE, TSTFILE, "user.foobar", "")) ;
   CPPUNIT_LOG_IS_FALSE(pcomn::xattr_set(pcomn::XA_CREATE, File.handle(), "user.foobar", "")) ;

   CPPUNIT_LOG_EQUAL(pcomn::xattr_get(File.handle(), "user.foobar"), std::string()) ;
   CPPUNIT_LOG_EQUAL(pcomn::xattr_get(File.handle(), "user.foobar"), std::string()) ;
   CPPUNIT_LOG_EXCEPTION(pcomn::xattr_get(File.handle(), "user.bar"), pcomn::system_error) ;

   CPPUNIT_LOG_ASSERT(pcomn::xattr_set(pcomn::XA_REPLACE, TSTFILE, "user.foobar", "Hello, world!")) ;
   CPPUNIT_LOG_IS_FALSE(pcomn::xattr_set(pcomn::XA_REPLACE, File.handle(), "user.bar", "Bye, baby!")) ;
   CPPUNIT_LOG_EQUAL(pcomn::xattr_get(File.handle(), "user.foobar"), std::string("Hello, world!")) ;
   CPPUNIT_LOG_EQUAL(pcomn::xattr_size(File.handle(), "user.foobar"), (ssize_t)13) ;
   CPPUNIT_LOG_EQUAL(pcomn::xattr_size(TSTFILE, "user.bar"), (ssize_t)-1) ;

   CPPUNIT_LOG_IS_FALSE(pcomn::xattr_del(TSTFILE, "user.bar")) ;
   CPPUNIT_LOG_ASSERT(pcomn::xattr_set(pcomn::XA_SET, File.handle(), "user.bar", "Bye, baby!")) ;
   CPPUNIT_LOG_EQUAL(pcomn::xattr_get(File.handle(), "user.bar"), std::string("Bye, baby!")) ;
   CPPUNIT_LOG_ASSERT(pcomn::xattr_del(TSTFILE, "user.bar")) ;
   CPPUNIT_LOG_IS_FALSE(pcomn::has_xattr(TSTFILE, "user.bar")) ;
   CPPUNIT_LOG_EQUAL(pcomn::xattr_get(File.handle(), "user.foobar"), std::string("Hello, world!")) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   const pcomn::malloc_ptr<char[]> cwd (getcwd(NULL, 0)) ;
   runner.addTest(XattrFail::suite()) ;
   if (pcomn::xattr_supported("."))
   {
      CPPUNIT_LOG("Extended attributes are supported on '" << cwd << "'" << std::endl) ;
      runner.addTest(XattrTests::suite()) ;
   }
   else
      CPPUNIT_LOG("Extended attributes are NOT supported on '" << cwd << "'") ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "Extended file attributes test") ;
}
