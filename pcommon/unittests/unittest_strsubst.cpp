/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_strsubst.cpp
 COPYRIGHT    :   Yakov Markovitch, 2010-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   PCommon library unittests.
                  String templates teststs.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   30 Oct 2010
*******************************************************************************/
#include <pcomn_strsubst.h>
#include <pcomn_unittest.h>
#include <pcomn_textio.h>
#include <pcomn_string.h>
#include <pcomn_rawstream.h>
#include <pcomn_handle.h>
#include <pcomn_fstream.h>

#include <fstream>
#include <sstream>

using namespace pcomn ;

/*******************************************************************************
                     class StrSubstTests
*******************************************************************************/
class StrSubstTests : public CppUnit::TestFixture {

      void Test_Literal_Substitutions() ;
      void Test_Reference_Substitutions() ;
      void Test_Functor_Substitutions() ;
      void Test_Template_Sources() ;
      void Test_Substitution_Output() ;
      void Test_Removing_Comments() ;

      CPPUNIT_TEST_SUITE(StrSubstTests) ;

      CPPUNIT_TEST(Test_Literal_Substitutions) ;
      CPPUNIT_TEST(Test_Reference_Substitutions) ;
      CPPUNIT_TEST(Test_Functor_Substitutions) ;
      CPPUNIT_TEST(Test_Template_Sources) ;
      CPPUNIT_TEST(Test_Substitution_Output) ;
      CPPUNIT_TEST(Test_Removing_Comments) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 StrSubstTests
*******************************************************************************/
void StrSubstTests::Test_Literal_Substitutions()
{
   tpl::substitution_map smap ;
   std::string Bye ("Bye") ;
   unsigned long long Big = 18446744073709551615ULL ;
   CPPUNIT_LOG_RUN(smap
                   ("foo_int", 20)
                   ("foo_char", 'R')
                   ("foo_str", "Hello, ")
                   ("foo_bye", Bye)
                   ("WORLD", "world")
                   ("TheAnswer", (short)42)
                   ("foo_big", Big)) ;

   std::string result ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, strslice(), result), std::string()) ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "$$foo_bye, world!", result), std::string("$foo_bye, world!")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "$", result), std::string("$")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "$$", result), std::string("$")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "$$_", result), std::string("$_")) ;

   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "Answer to the Ultimate Question of Life, the Universe and Everything is $TheAnswer", result),
                     std::string("Answer to the Ultimate Question of Life, the Universe and Everything is 42")) ;

   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "$foo_bye, baby!", result), std::string("Bye, baby!")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "${foo_str}world!", result), std::string("Hello, world!")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "${foo_str}", result), std::string("Hello, ")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "The Big is ${foo_big}ULL, $$foo_int==$foo_int, and $unknown==$$unknown", result),
                     std::string("The Big is 18446744073709551615ULL, $foo_int==20, and $unknown==$unknown")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "${foo_char}eference to ${foo_char}eturn: $foo_charvalue", result),
                     std::string("Reference to Return: $foo_charvalue")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "$foo_str$WORLD!", result), std::string("Hello, world!")) ;
}

void StrSubstTests::Test_Reference_Substitutions()
{
   tpl::substitution_map smap ;
   strslice greeting ("Hello") ;
   std::string object ("world") ;
   unsigned answer (42) ;
   CPPUNIT_LOG_RUN(smap
                   ("GREETING", greeting)
                   ("OBJECT", object)
                   ("ANSWER", answer)) ;
   static const char Template[] = "$GREETING, $OBJECT! The answer is $ANSWER..." ;

   std::string result ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, Template, result), std::string("Hello, world! The answer is 42...")) ;
   CPPUNIT_LOG_RUN(greeting = "Bye") ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, Template, result), std::string("Hello, world! The answer is 42...")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(smap("GREETING", std::cref(greeting))) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, Template, result), std::string("Bye, world! The answer is 42...")) ;

   result.clear() ;
   CPPUNIT_LOG_RUN(greeting = "Hello") ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, Template, result), std::string("Hello, world! The answer is 42...")) ;
   CPPUNIT_LOG_RUN(smap("ANSWER", std::ref(answer))) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, Template, result), std::string("Hello, world! The answer is 42...")) ;
   result.clear() ;
   CPPUNIT_LOG_RUN(answer = 14) ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, Template, result), std::string("Hello, world! The answer is 14...")) ;
}

void StrSubstTests::Test_Functor_Substitutions()
{
   tpl::substitution_map smap ;
   strslice greeting ("Hello") ;
   std::string object ("world") ;
   unsigned answer (0) ;

   struct local {
         static unsigned inc(unsigned *var) { return ++*var ; }
   } ;

   CPPUNIT_LOG_RUN(smap
                   ("GREETING", greeting)
                   ("OBJECT", std::cref(object))
                   ("ANSWER", std::bind(local::inc, &answer))) ;
   static const char Template[] = "$GREETING, $OBJECT! The answer is $ANSWER..." ;

   std::string result ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, Template, result), std::string("Hello, world! The answer is 1...")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, Template, result), std::string("Hello, world! The answer is 2...")) ;
   result.clear() ;
   CPPUNIT_LOG_RUN(object = "baby") ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, Template, result), std::string("Hello, baby! The answer is 3...")) ;
}

void StrSubstTests::Test_Template_Sources()
{
   tpl::substitution_map smap ;
   strslice greeting ("Hello") ;
   std::string object ("world") ;
   unsigned answer (42) ;

   CPPUNIT_LOG_RUN(smap
                   ("GREETING", greeting)
                   ("OBJECT", std::cref(object))
                   ("ANSWER", answer)) ;
   static const char Template[] = "$GREETING, $OBJECT! The answer is $ANSWER..." ;

   std::string result ;
   strslice Slice (Template) ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, Slice.begin(), Slice.end(), result), std::string("Hello, world! The answer is 42...")) ;

   std::istringstream Stream (Template) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, Stream, result), std::string("Hello, world! The answer is 42...")) ;

   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, Template, result), std::string("Hello, world! The answer is 42...")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, std::string(Template), result), std::string("Hello, world! The answer is 42...")) ;

   const std::string FILE_name (CPPUNIT_AT_PROGDIR("Test_Template_Sources.TEMPLATE.txt")) ;

   CPPUNIT_LOG(std::endl) ;

   FILE_safehandle File (fopen(FILE_name.c_str(), "w")) ;
   CPPUNIT_LOG_ASSERT(File) ;
   CPPUNIT_LOG_ASSERT(fputs(Template, File) >= 0) ;
   File.reset() ;
   CPPUNIT_LOG_RUN(File.reset(fopen(FILE_name.c_str(), "r"))) ;
   CPPUNIT_LOG_ASSERT(File) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, File.get(), result), std::string("Hello, world! The answer is 42...")) ;

   CPPUNIT_LOG(std::endl) ;
   File.reset() ;
   binary_ifdstream ifdstream (PCOMN_ENSURE_POSIX(open(FILE_name.c_str(), O_RDONLY), "open")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, ifdstream, result), std::string("Hello, world! The answer is 42...")) ;
}

void StrSubstTests::Test_Substitution_Output()
{
   tpl::substitution_map smap ;
   strslice greeting ("Hello") ;
   std::string object ("world") ;
   unsigned answer (42) ;

   CPPUNIT_LOG_RUN(smap
                   ("GREETING", greeting)
                   ("OBJECT", std::cref(object))
                   ("ANSWER", answer)) ;
   static const char Template[] = "$GREETING, $OBJECT! The answer is $ANSWER..." ;
   std::string result ;

   const std::string FILE_name (CPPUNIT_AT_PROGDIR("Test_Substitution_Output.FILE.txt")) ;
   FILE_safehandle File (fopen(FILE_name.c_str(), "w")) ;

   CPPUNIT_LOG_RUN(tpl::subst(smap, Template, File)) ;
   File.reset() ;
   CPPUNIT_LOG_EQUAL(unit::full_file(FILE_name), std::string("Hello, world! The answer is 42...")) ;
   File.reset(fopen(FILE_name.c_str(), "w")) ;
   // Cannot pass File.get() directly to tpl::subst, lvalue reference required (note that
   // a constant _is_ allowed).
   FILE * const FPtr = File ;
   CPPUNIT_LOG_RUN(tpl::subst(smap, Template, FPtr)) ;
   File.reset() ;
   CPPUNIT_LOG_EQUAL(unit::full_file(FILE_name), std::string("Hello, world! The answer is 42...")) ;

   CPPUNIT_LOG(std::endl) ;
   binary_ofdstream ofdstream (PCOMN_ENSURE_POSIX(open(FILE_name.c_str(), O_WRONLY|O_TRUNC|O_CREAT, 0666), "open")) ;
   result.clear() ;
   CPPUNIT_LOG_RUN(tpl::subst(smap, Template, ofdstream)) ;
   CPPUNIT_LOG_EQUAL(unit::full_file(FILE_name), std::string("Hello, world! The answer is 42...")) ;
}

void StrSubstTests::Test_Removing_Comments()
{
   tpl::substitution_map smap ;
   std::string Bye ("Bye") ;
   unsigned long long Big = 18446744073709551615ULL ;
   CPPUNIT_LOG_RUN(smap
                   ("foo_int", 20)
                   ("foo_char", 'R')
                   ("foo_str", "Hello, ")
                   ("foo_bye", Bye)
                   ("WORLD", "world")
                   ("TheAnswer", (short)42)
                   ("foo_big", Big)) ;

   std::string result ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, strslice(), result), std::string()) ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "$$foo_bye, world!", result), std::string("$foo_bye, world!")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "$", result), std::string("$")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "$$", result), std::string("$")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "$$_", result), std::string("$_")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "$**$", result), std::string("")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "$*", result), std::string("")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "*$", result), std::string("*$")) ;

   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "Answer to the Ultimate Question of $$Life, $ *$the Universe and Everything is $TheAnswer$* not closed comment will be removed too!", result),
                     std::string("Answer to the Ultimate Question of $Life, $ *$the Universe and Everything is 42")) ;

   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "$foo_bye, baby!", result), std::string("Bye, baby!")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "${foo_str}world!", result), std::string("Hello, world!")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "${foo_str}", result), std::string("Hello, ")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "The $**$Big $* comments should be removed *$is $*rm*$${foo_big}ULL, $$foo_int==$foo_int, and $unknown==$$unknown", result),
                     std::string("The Big is 18446744073709551615ULL, $foo_int==20, and $unknown==$unknown")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "${foo_char}eference to ${foo_char}eturn: $*********nothing should be breaked*$$foo_charvalue", result),
                     std::string("Reference to Return: $foo_charvalue")) ;
   result.clear() ;
   CPPUNIT_LOG_EQUAL(tpl::subst(smap, "$foo_str$WORLD$*!@#$%^\n()*$!", result), std::string("Hello, world!")) ;
}

/*******************************************************************************
 main
*******************************************************************************/
int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(StrSubstTests::suite()) ;
   return pcomn::unit::run_tests(runner, argc, argv, "unittest_strsubst.diag.ini",
                                 "String templates tests") ;
}
