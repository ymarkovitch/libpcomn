/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_omanip.cpp
 COPYRIGHT    :   Yakov Markovitch, 2015-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for binary iostreams.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 Mar 2015
*******************************************************************************/
#include <pcomn_omanip.h>
#include <pcomn_unittest.h>
#include <pcomn_tuple.h>

#include <vector>
#include <list>
#include <sstream>

/*******************************************************************************
                     class OmanipTests
*******************************************************************************/
class OmanipTests : public CppUnit::TestFixture {
   private:
      void Test_OSequence() ;
      void Test_OContainer() ;
      void Test_OHRSize() ;
      void Test_OException() ;

      CPPUNIT_TEST_SUITE(OmanipTests) ;

      CPPUNIT_TEST(Test_OSequence) ;
      CPPUNIT_TEST(Test_OHRSize) ;
      CPPUNIT_TEST(Test_OContainer) ;
      CPPUNIT_TEST(Test_OException) ;

      CPPUNIT_TEST_SUITE_END() ;
   protected:
      const std::vector<std::string>   strvec {"zero", "one", "two", "three"} ;
      const std::vector<int>           intvec {1, 3, 5, 7, 11} ;
} ;

void OmanipTests::Test_OSequence()
{
   {
      std::ostringstream os ;
      CPPUNIT_LOG_ASSERT(os << pcomn::osequence(strvec.begin() + 0, strvec.begin() + 2)) ;
      CPPUNIT_LOG_EQ(os.str(), "zero\none\n") ;
   }
   {
      std::ostringstream os ;
      CPPUNIT_LOG_ASSERT(os << pcomn::osequence(strvec.begin() + 0, strvec.begin() + 2, "->")) ;
      CPPUNIT_LOG_EQ(os.str(), "zero->one->") ;
   }
   {
      std::ostringstream os ;
      CPPUNIT_LOG_ASSERT(os << pcomn::osequence(strvec.begin() + 0, strvec.begin() + 2, 0)) ;
      CPPUNIT_LOG_EQ(os.str(), "zero0one0") ;
   }
   {
      std::ostringstream os ;
      CPPUNIT_LOG_ASSERT(os << pcomn::osequence(strvec.begin() + 0, strvec.begin() + 2, "[[", "]]")) ;
      CPPUNIT_LOG_EQ(os.str(), "[[zero]][[one]]") ;
   }
   {
      std::ostringstream os ;
      CPPUNIT_LOG_ASSERT(os << pcomn::ocontdelim(intvec, ":-:")) ;
      CPPUNIT_LOG_EQ(os.str(), "1:-:3:-:5:-:7:-:11") ;
   }
   {
      std::ostringstream os ;
      CPPUNIT_LOG_ASSERT(os << pcomn::ocontdelim(strvec)) ;
      CPPUNIT_LOG_EQ(os.str(), "zero, one, two, three") ;
   }
}

void OmanipTests::Test_OContainer()
{
   {
      const std::list<int> c {3, -1, 2} ;
      pcomn::omemstream os ;
      CPPUNIT_LOG_ASSERT(os << pcomn::ocontdelim(c, '|')) ;
      CPPUNIT_LOG_EQ(os.checkout(), "3|-1|2") ;
      CPPUNIT_LOG_ASSERT(os << pcomn::ocontdelim(c, "|")) ;
      CPPUNIT_LOG_EQ(os.checkout(), "3|-1|2") ;
      CPPUNIT_LOG_ASSERT(os << pcomn::ocontdelim(c, ", ")) ;
      CPPUNIT_LOG_EQ(os.checkout(), "3, -1, 2") ;
   }
   {
      const std::vector<int> c ;
      pcomn::omemstream os ;
      CPPUNIT_LOG_ASSERT(os << pcomn::ocontdelim(c, '|')) ;
      CPPUNIT_LOG_EQ(os.checkout(), "") ;
      CPPUNIT_LOG_ASSERT(os << pcomn::ocontdelim(c, "|")) ;
      CPPUNIT_LOG_EQ(os.checkout(), "") ;
      CPPUNIT_LOG_ASSERT(os << pcomn::ocontdelim(c, ", ")) ;
      CPPUNIT_LOG_EQ(os.checkout(), "") ;
   }
   {
      const char * const c[] = {"Hello", "world!" } ;
      pcomn::omemstream os ;
      CPPUNIT_LOG_ASSERT(os << pcomn::ocontdelim(c, ", ")) ;
      CPPUNIT_LOG_EQ(os.checkout(), "Hello, world!") ;
   }
   {
      const char * const c[] = {"Hello", "world!" } ;
      const std::list<int> n {3, -1, 2} ;
      pcomn::omemstream os ;
      CPPUNIT_LOG_ASSERT(print_range(c, os, "", [](std::ostream &s, const char *v) { s << '(' << strlen(v) << ')' ; })) ;
      CPPUNIT_LOG_EQ(os.checkout(), "(5)(6)") ;
      CPPUNIT_LOG_ASSERT(print_range(c, os, pcomn::ocontdelim(n, '?'),
                                     [](std::ostream &s, const char *v) { s << strlen(v) ; })) ;
      CPPUNIT_LOG_EQ(os.checkout(), "53?-1?26") ;
   }
   {
      const std::list<const char *> c {"Hello", "world!"} ;
      CPPUNIT_LOG_EQ(string_cast(pcomn::ocontdelim
                                 (c, ',', [](std::ostream &s, const char *v) { s << '(' << strlen(v) << ')' ; })),
                     "(5),(6)") ;
   }
}

void OmanipTests::Test_OHRSize()
{
   using namespace pcomn ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsize(0)), "0") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsize(900)), "900") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsize(1023)), "1023") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsize(1024)), "1.00K") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsize(1025)), "1.00K") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsize(1536)), "1.50K") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsize(1023*KiB)), "1023.00K") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsize(1024*KiB)), "1.00M") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsize(1024*KiB + 1)), "1.00M") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsize(1024*MiB)), "1.00G") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsize(1024*MiB + 1)), "1.00G") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsize(1100*MiB)), "1.07G") ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsizex(0)), "0") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsizex(900)), "900") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsizex(1023)), "1023") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsizex(1024)), "1K") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsizex(1025)), "1025") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsizex(1536)), "1536") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsizex(1023*KiB)), "1023K") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsizex(1024*KiB)), "1M") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsizex(1024*KiB + 1)), "1048577") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsizex(1025*KiB)), "1025K") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsizex(1024*MiB)), "1G") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsizex(1024*MiB + 1)), "1073741825") ;
   CPPUNIT_LOG_EQ(string_cast(pcomn::ohrsizex(1100*MiB)), "1100M") ;
}

void OmanipTests::Test_OException()
{
   using namespace pcomn ;
   std::exception_ptr xptr ;

   try { throw std::runtime_error("Hello!") ; }
   catch(...)
   {
      CPPUNIT_LOG_EQ(string_cast(oexception({})), "") ;
      CPPUNIT_LOG_EQ(string_cast(oexception()), "std::runtime_error: Hello!") ;
      CPPUNIT_LOG_RUN(xptr = std::current_exception()) ;
   }
   CPPUNIT_LOG_ASSERT(xptr) ;
   CPPUNIT_LOG_EQ(string_cast(oexception(xptr)), "std::runtime_error: Hello!") ;

   CPPUNIT_LOG_EQ(string_cast(oexception({})), "") ;
   CPPUNIT_LOG_EQ(string_cast(oexception()), "") ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(OmanipTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.trace.ini",
                             "Testing ostream manipulators") ;
}
