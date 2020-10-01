/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_cfgparser.cpp
 COPYRIGHT    :   Yakov Markovitch, 2007-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test configuration file parsing/writing functions.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   11 Sep 2007
*******************************************************************************/
#include <pcomn_cfgparser.h>
#include <pcomn_handle.h>
#include <pcomn_unittest.h>
#include <pcomn_string.h>
#include <pcomn_path.h>
#include <pcomn_safeptr.h>
#include <pcomn_unistd.h>

#include <errno.h>
#include <stdlib.h>

/*******************************************************************************
                     class CfgParserTests
*******************************************************************************/
class CfgParserTests : public CppUnit::TestFixture {

   protected:
      bool Cleanup_CfgFile(const char *filename) ;

   private:
      void Test_CfgFileRead() ;
      void Test_CfgIterators() ;
      void Test_CfgFileWrite() ;

      CPPUNIT_TEST_SUITE(CfgParserTests) ;

      CPPUNIT_TEST(Test_CfgFileRead) ;
      CPPUNIT_TEST(Test_CfgIterators) ;
      CPPUNIT_TEST(Test_CfgFileWrite) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

bool CfgParserTests::Cleanup_CfgFile(const char *filename)
{
   return (access(filename, 0) && errno == ENOENT) || !unlink(filename) ;
}

void CfgParserTests::Test_CfgFileRead()
{
   char buf[65535] ;
   CPPUNIT_LOG_ASSERT(Cleanup_CfgFile("foobar.ini")) ;
   pcomn::unit::fillbuf(buf) ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.ini", "Foo", "Bar", buf), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(*buf, '\0') ;
   pcomn::unit::fillbuf(buf) ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.ini", "Foo", NULL, buf), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf[0], '\0') ; CPPUNIT_LOG_EQUAL(buf[1], '\0') ;
   pcomn::unit::fillbuf(buf) ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.ini", NULL, NULL, buf), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf[0], '\0') ; CPPUNIT_LOG_EQUAL(buf[1], '\0') ;
   pcomn::unit::fillbuf(buf) ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.ini", "", "Bar", buf), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf[0], '\0') ; CPPUNIT_LOG_EQUAL(buf[1], '\0') ;
   pcomn::unit::fillbuf(buf) ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.ini", "", "Bar", buf, 0, ""), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf[0], '\xCC') ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.ini", "", "Bar", buf, 1, ""), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf[0], '\0') ; CPPUNIT_LOG_EQUAL(buf[1], '\xCC') ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.ini", "", "Bar", buf, 2, ""), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf[0], '\0') ; CPPUNIT_LOG_EQUAL(buf[1], '\0') ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.ini", "", "Bar", NULL, 2, ""), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.ini", "", "Bar", NULL, 10, "Troll"), (size_t)0) ;

   CPPUNIT_LOG(std::endl) ;
   pcomn::unit::fillbuf(buf) ;
   static const char * const CfgSource[] =
   {
      "CfgParserTests.TestRead.unix.eol.lst",
      "CfgParserTests.TestRead.windows.eol.lst",
      NULL
   } ;
   for (const char * const *file = CfgSource ; *file ; ++file)
   {
      const std::string cfgfile_path (CPPUNIT_AT_TESTDIR(*file)) ;
      const char * const fname = cfgfile_path.c_str() ;

      CPPUNIT_LOG("Reading ") << fname << std::endl ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_value(fname, "", "Leben", NULL, 20, ""), (size_t)0) ;

      CPPUNIT_LOG_EQUAL(cfgfile_get_value(fname, "", "Leben", buf), strlen("ist wunderschoen")) ;
      CPPUNIT_LOG_EQUAL(std::string(buf), std::string("ist wunderschoen")) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_value(fname, "Restaurant", "of", buf), strlen("the Universe")) ;
      CPPUNIT_LOG_EQUAL(std::string(buf), std::string("the Universe")) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_value(fname, "Restaurant", "by", buf, "Troll"), strlen("Troll")) ;
      CPPUNIT_LOG_EQUAL(std::string(buf), std::string("Troll")) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_value(fname, "Restaurant", "OF", buf), strlen("the Universe")) ;
      CPPUNIT_LOG_EQUAL(std::string(buf), std::string("the Universe")) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_value(fname, "RESTAURANT", "OF", buf, "Troll"), strlen("the Universe")) ;
      CPPUNIT_LOG_EQUAL(std::string(buf), std::string("the Universe")) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_intval(fname, "Restaurant", "by", -1), -1) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_intval(fname, "Restaurant", "by", 20), 20) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_intval(fname, "Bar", "2x2", -1), 4) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_intval(fname, "Bar", "2x2", 20), 4) ;
      pcomn::unit::fillbuf(buf) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_value(fname, "Bar", "quux", buf), strlen("foobar")) ;
      CPPUNIT_LOG_EQUAL(std::string(buf), std::string("foobar")) ;
      pcomn::unit::fillbuf(buf) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_value(fname, "Bar", "quux", buf, "however"), strlen("foobar")) ;
      CPPUNIT_LOG_EQUAL(std::string(buf), std::string("foobar")) ;
      pcomn::unit::fillbuf(buf) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_value(fname, "Bar", "15", buf, "however"), (size_t)0) ;
      CPPUNIT_LOG_EQUAL(std::string(buf), std::string()) ;

      CPPUNIT_LOG(std::endl) ;
      pcomn::unit::fillbuf(buf) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_value(fname, "Bar", NULL, buf), strlen("hello=world 15= 2x2=4 quux=foobar ")) ;
      CPPUNIT_LOG_EQUAL(memcmp(buf, "hello=world\0""15=\0""2x2=4\0quux=foobar\0\0", strlen("hello=world 15= 2x2=4 quux=foobar ") + 1), 0) ;

      // cfgfile_get_section() is a trivial wrapper around cfgfile_get_value(), but check
      // it anyway
      pcomn::unit::fillbuf(buf) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_section(fname, "Bar", buf), strlen("hello=world 15= 2x2=4 quux=foobar ")) ;
      CPPUNIT_LOG_EQUAL(memcmp(buf, "hello=world\0""15=\0""2x2=4\0quux=foobar\0\0", strlen("hello=world 15= 2x2=4 quux=foobar ") + 1), 0) ;
      // Not enough place
      pcomn::unit::fillbuf(buf) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_section(fname, "Bar", buf + 0, 6), (size_t)6) ;
      CPPUNIT_LOG_EQUAL(memcmp(buf, "hell\0\0", 6), 0) ;

      pcomn::unit::fillbuf(buf) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_value(fname, NULL, NULL, buf), strlen("Bar Quux Restaurant ")) ;
      CPPUNIT_LOG_EQUAL(memcmp(buf, "Bar\0Quux\0Restaurant\0\0", strlen("Bar Quux Restaurant ") + 1), 0) ;
      pcomn::unit::fillbuf(buf) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_value(fname, "Nothing", NULL, buf), (size_t)0) ;
      CPPUNIT_LOG_EQUAL(buf[0], '\0') ; CPPUNIT_LOG_EQUAL(buf[1], '\0') ;
      pcomn::unit::fillbuf(buf) ;
      CPPUNIT_LOG_EQUAL(cfgfile_get_value(fname, "", NULL, buf), strlen("Leben=ist wunderschoen ")) ;
      CPPUNIT_LOG_EQUAL(memcmp(buf, "Leben=ist wunderschoen\0\0", strlen("Leben=ist wunderschoen ") + 1), 0) ;
   }
}

void CfgParserTests::Test_CfgIterators()
{
   typedef pcomn::cstrseq_iterator<>         section_iterator ;
   typedef pcomn::cstrseq_keyval_iterator<>  keyval_iterator ;

   // Test empty iterators
   CPPUNIT_IS_TRUE(section_iterator() == section_iterator()) ;
   CPPUNIT_IS_FALSE(section_iterator() != section_iterator()) ;
   CPPUNIT_IS_TRUE(keyval_iterator() == keyval_iterator()) ;
   CPPUNIT_IS_FALSE(keyval_iterator() != keyval_iterator()) ;

   CPPUNIT_LOG(std::endl) ;

   const std::string cfgfile_path (CPPUNIT_AT_TESTDIR("CfgParserTests.TestRead.windows.eol.lst")) ;
   const char * const fname = cfgfile_path.c_str() ;
   char buf[65535] ;
   pcomn::unit::fillbuf(buf) ;

   CPPUNIT_LOG("\nTest section names" << std::endl) ;
   CPPUNIT_LOG_ASSERT(cfgfile_get_sectnames(fname, buf) > 2) ;
   CPPUNIT_LOG_EQUAL(std::vector<std::string>(section_iterator(buf), section_iterator()),
                     CPPUNIT_STRVECTOR(("Bar") ("Quux") ("Restaurant"))) ;
   CPPUNIT_LOG_EQUAL(std::vector<std::string>(pcomn::cstrseq_iterator<std::string>(buf), pcomn::cstrseq_iterator<std::string>()),
                     CPPUNIT_STRVECTOR(("Bar") ("Quux") ("Restaurant"))) ;
   CPPUNIT_LOG_ASSERT(section_iterator(buf) == section_iterator(buf)) ;
   CPPUNIT_LOG_ASSERT(section_iterator(buf) == section_iterator(buf)++) ;
   CPPUNIT_LOG_ASSERT(section_iterator(buf)++ == section_iterator(buf)++) ;
   CPPUNIT_LOG_ASSERT(++section_iterator(buf) != section_iterator(buf)++) ;
   CPPUNIT_LOG_ASSERT(++section_iterator(buf) == ++section_iterator(buf)) ;

   CPPUNIT_LOG(std::endl) ;
   const std::string manysections_path (CPPUNIT_AT_TESTDIR("CfgParserTests.TestReadManySections.lst")) ;
   pcomn::malloc_ptr<char[]> sbuf (cfgfile_get_sectnames(manysections_path.c_str())) ;
   CPPUNIT_LOG_ASSERT(sbuf) ;
   CPPUNIT_LOG_EQUAL(std::vector<std::string>(section_iterator(sbuf.get()), section_iterator()),
                     CPPUNIT_STRVECTOR(("Bar")
                                       ("Quux")
                                       ("Restaurant")
                                       ("Universe")
                                       ("VeryLongSection.Name")
                                       ("Extremely.Long.Section.Name.Delimited.With.Dots")
                                       ("Another.Extremely.Long.Section.Name.Delimited.With.Dots")
                                       ("YetMore.Very.Very.Very.Long.Section.Name.Delimited.With.Dots")
                                       ("And_This_Section_Name_Is_With_Underscores")
                                       ("Another_Section_Name_With_Underscores"))) ;

   CPPUNIT_LOG("\nTest an empty sequence" << std::endl) ;
   buf[0] = buf[1] = 0 ;
   CPPUNIT_LOG_ASSERT(section_iterator(buf) == section_iterator()) ;
   CPPUNIT_LOG_ASSERT(section_iterator(buf) == section_iterator(buf)) ;

   CPPUNIT_LOG("\nTest key/value pairs" << std::endl) ;
   // Test an empty sequence
   CPPUNIT_LOG_EQUAL(cfgfile_get_section(fname, "NoSection", buf), (size_t)0) ;
   CPPUNIT_LOG_ASSERT(keyval_iterator(buf) == keyval_iterator(buf)) ;
   CPPUNIT_LOG_ASSERT(keyval_iterator(buf) == keyval_iterator()) ;
   CPPUNIT_LOG_ASSERT(keyval_iterator() == keyval_iterator(buf)) ;

   typedef std::pair<pcomn::strslice, pcomn::strslice> slicepair ;

   CPPUNIT_LOG_ASSERT(cfgfile_get_section(fname, "Bar", buf) > 2) ;

   CPPUNIT_LOG_EQUAL(slicepair(*keyval_iterator(buf)), slicepair("hello", "world")) ;
   CPPUNIT_LOG_EQUAL(slicepair(*keyval_iterator(buf)++), slicepair("hello", "world")) ;
   CPPUNIT_LOG_EQUAL(slicepair(*++keyval_iterator(buf)), slicepair("15", "")) ;
   CPPUNIT_LOG_EQUAL(slicepair(*++ ++keyval_iterator(buf)), slicepair("2x2", "4")) ;
   CPPUNIT_LOG_EQUAL(slicepair(*++ ++ ++keyval_iterator(buf)), slicepair("quux", "foobar")) ;

   CPPUNIT_LOG_EQUAL(std::distance(keyval_iterator(buf), keyval_iterator()), (ptrdiff_t)4) ;

   CPPUNIT_LOG_EQUAL(std::vector<std::string>(section_iterator(buf), section_iterator()),
                     CPPUNIT_STRVECTOR(("hello=world") ("15=") ("2x2=4") ("quux=foobar"))) ;

   typedef std::pair<std::string, std::string> strpair ;
   // Using std::string as a key type, we can create std::map in one pass
   CPPUNIT_LOG_EQUAL((std::map<std::string, std::string>
                      (pcomn::cstrseq_keyval_iterator<std::string>(buf), pcomn::cstrseq_keyval_iterator<std::string>())),

                     CPPUNIT_STRMAP(std::string,
                                    (strpair("hello", "world"))
                                    (strpair("15", ""))
                                    (strpair("2x2", "4"))
                                    (strpair("quux", "foobar")))) ;

   CPPUNIT_LOG(std::endl) ;
   sbuf.reset() ;
   CPPUNIT_LOG_ASSERT((sbuf.reset(cfgfile_get_section(fname, "Bar")), sbuf)) ;
   CPPUNIT_LOG_EQUAL(std::vector<std::string>(section_iterator(sbuf.get()), section_iterator()),
                     CPPUNIT_STRVECTOR(("hello=world") ("15=") ("2x2=4") ("quux=foobar"))) ;
}

void CfgParserTests::Test_CfgFileWrite()
{
   char buf[65536] ;
   CPPUNIT_LOG_ASSERT(Cleanup_CfgFile("foobar.write.ini")) ;
   FILE *f = fopen("foobar.write.ini", "w") ;
   CPPUNIT_LOG_ASSERT(f) ;
   CPPUNIT_LOG_ASSERT(fclose(f) >= 0) ;
   pcomn::unit::fillbuf(buf) ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.write.ini", "Foo", "Bar", buf), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(*buf, '\0') ;
   pcomn::unit::fillbuf(buf) ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.write.ini", "Foo", NULL, buf), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf[0], '\0') ; CPPUNIT_LOG_EQUAL(buf[1], '\0') ;
   pcomn::unit::fillbuf(buf) ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.write.ini", NULL, NULL, buf), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf[0], '\0') ; CPPUNIT_LOG_EQUAL(buf[1], '\0') ;
   pcomn::unit::fillbuf(buf) ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.write.ini", "", "Bar", buf), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf[0], '\0') ; CPPUNIT_LOG_EQUAL(buf[1], '\0') ;
   pcomn::unit::fillbuf(buf) ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.write.ini", "", "Bar", buf, 0, ""), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf[0], '\xCC') ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.write.ini", "", "Bar", buf, 1, ""), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf[0], '\0') ; CPPUNIT_LOG_EQUAL(buf[1], '\xCC') ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.write.ini", "", "Bar", buf, 2, ""), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf[0], '\0') ; CPPUNIT_LOG_EQUAL(buf[1], '\0') ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.write.ini", "", "Bar", NULL, 2, ""), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(cfgfile_get_value("foobar.write.ini", "", "Bar", NULL, 10, "Troll"), (size_t)0) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(Cleanup_CfgFile("foobar.write.ini")) ;
   CPPUNIT_LOG_ASSERT(cfgfile_write_value("foobar.write.ini", NULL, "Hello", "world")) ;
   CPPUNIT_LOG_EQUAL(pcomn::unit::full_file("foobar.write.ini"), std::string("Hello = world\n\n")) ;
   CPPUNIT_LOG_ASSERT(cfgfile_write_value("foobar.write.ini", "Bar", "hello", "world")) ;
   CPPUNIT_LOG_ASSERT(cfgfile_write_value("foobar.write.ini", "Bar", "world", "15")) ;
   CPPUNIT_LOG_EQUAL(pcomn::unit::full_file("foobar.write.ini"),
                     std::string("Hello = world\n\n[Bar]\nhello = world\nworld = 15\n\n")) ;
   CPPUNIT_LOG_ASSERT(cfgfile_write_value("foobar.write.ini", NULL, "I am", "lucky")) ;
   CPPUNIT_LOG_EQUAL(pcomn::unit::full_file("foobar.write.ini"),
                     std::string("Hello = world\nI am = lucky\n\n[Bar]\nhello = world\nworld = 15\n\n")) ;
   CPPUNIT_LOG_ASSERT(cfgfile_write_value("foobar.write.ini", "Quux", "Bye", "baby")) ;
   CPPUNIT_LOG_EQUAL(pcomn::unit::full_file("foobar.write.ini"),
                     std::string("Hello = world\nI am = lucky\n\n"
                                 "[Bar]\nhello = world\nworld = 15\n\n"
                                 "[Quux]\nBye = baby\n\n")) ;
   CPPUNIT_LOG_ASSERT(cfgfile_write_value("foobar.write.ini", "Bar", "hello", "12")) ;
   CPPUNIT_LOG_EQUAL(pcomn::unit::full_file("foobar.write.ini"),
                     std::string("Hello = world\nI am = lucky\n\n"
                                 "[Bar]\nhello = 12\nworld = 15\n\n"
                                 "[Quux]\nBye = baby\n\n")) ;
   CPPUNIT_LOG_ASSERT(cfgfile_write_value("foobar.write.ini", "Bar", NULL, NULL)) ;
   CPPUNIT_LOG_EQUAL(pcomn::unit::full_file("foobar.write.ini"),
                     std::string("Hello = world\nI am = lucky\n\n\n"
                                 "[Quux]\nBye = baby\n\n")) ;
   CPPUNIT_LOG_ASSERT(cfgfile_write_value("foobar.write.ini", NULL, "HELLO", "all")) ;
   CPPUNIT_LOG_EQUAL(pcomn::unit::full_file("foobar.write.ini"),
                     std::string("HELLO = all\nI am = lucky\n\n\n"
                                 "[Quux]\nBye = baby\n\n")) ;
   CPPUNIT_LOG_ASSERT(cfgfile_write_value("foobar.write.ini", NULL, NULL, NULL)) ;
   CPPUNIT_LOG_EQUAL(pcomn::unit::full_file("foobar.write.ini"),
                     std::string("[Quux]\nBye = baby\n\n")) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(CfgParserTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             NULL, // We cannot use tracing config, since were _are_ testing config files!
                             "Configuration files parser tests.") ;
}
