/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_immutablestr.cpp
 COPYRIGHT    :   Yakov Markovitch, 2006-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittest for pcomn::immutable_str<> template class

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   23 Nov 2006
*******************************************************************************/
#include <pcomn_immutablestr.h>
#include <pcomn_strslice.h>
#include <pcomn_unittest.h>
#include <vector>
#include <set>
#include <list>

using namespace pcomn ;

template<class ImmutableString>
class ImmutableStringTests : public CppUnit::TestFixture {

      typedef ImmutableString                   istring ;
      typedef typename istring::value_type      char_type ;
      typedef typename istring::traits_type     traits_type ;
      typedef pcomn::mutable_strbuf<char_type>  strbuf ;

      typedef basic_strslice<char_type>         string_slice ;
      typedef std::basic_string<char_type>      std_string ;

      static std_string random_string(size_t len)
      {
         std_string r (len, char_type()) ;
         while(len--)
            r[len] = 'a' + std::rand() % 25 ;
         return r ;
      }

      void Test_Constructors_Compilation() ;
      void Test_Constructors_Invariants() ;
      void Test_Concatenation() ;
      void Test_Mutable_Strbuf() ;

      void Test_To_Upper_Lower() ;

      void Test_Comparison() ;
      void Test_Find() ;

      CPPUNIT_TEST_SUITE(ImmutableStringTests<ImmutableString>) ;

      CPPUNIT_TEST(Test_Constructors_Compilation) ;
      CPPUNIT_TEST(Test_Constructors_Invariants) ;
      CPPUNIT_TEST(Test_Concatenation) ;
      CPPUNIT_TEST(Test_Mutable_Strbuf) ;

      CPPUNIT_TEST(Test_To_Upper_Lower) ;

      CPPUNIT_TEST(Test_Comparison) ;
      CPPUNIT_TEST(Test_Find) ;

      CPPUNIT_TEST_SUITE_END() ;

   public:
      void setUp()
      {
         // Ensure that random strings were not so random - we need reproducible
         // results.
         std::srand(1) ;
      }
} ;

template<class CharT>
struct literals {
      static const CharT empty_string[] ;
      static const CharT some_string[] ;
      static const CharT lower_case[] ;
      static const CharT upper_case[] ;
      static const CharT line[] ;

      static const CharT foo[] ;
      static const CharT Foo[] ;
      static const CharT bar[] ;
      static const CharT BAR[] ;
      static const CharT s123[] ;
      static const CharT s1567[] ;

      static const CharT fmt1[] ;
      static const CharT fmt2[] ;
      static const CharT fmt3[] ;
} ;

#define STRING_LITERAL(name, value)                            \
template<> char    const literals<char>::name[] = value ;      \
template<> wchar_t const literals<wchar_t>::name[] = L##value

STRING_LITERAL(empty_string, "") ;
STRING_LITERAL(some_string, "0123456789abcdefghijklmnopqrstuvwxyz") ;
STRING_LITERAL(lower_case, "0123456789abcdefghijklmnopqrstuvwxyz") ;
STRING_LITERAL(upper_case, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ") ;

STRING_LITERAL(line, "abc\n") ;

STRING_LITERAL(foo,  "foo") ;
STRING_LITERAL(Foo,  "Foo") ;
STRING_LITERAL(bar,  "bar") ;
STRING_LITERAL(BAR,  "BAR") ;
STRING_LITERAL(s123, "123") ;
STRING_LITERAL(s1567,"1567") ;

STRING_LITERAL(fmt1, "abcd") ;
STRING_LITERAL(fmt2, "0x%08x0x%08x0x%08x0x%08x") ;

template<> char    const literals<char>::fmt3[] = "%.*s%.*s" ;
template<> wchar_t const literals<wchar_t>::fmt3[] = L"%.*ls%.*ls" ;

#undef STRING_LITERAL

// To pacify a compiler and to avoid "unused variable" warnings
static void dummy_strfn(const char *) {}
static void dummy_strfn(const wchar_t *) {}

/*******************************************************************************
 ImmutableStringTest
*******************************************************************************/
template<class ImmutableString>
void ImmutableStringTests<ImmutableString>::Test_Constructors_Compilation()
{
   if (!((intptr_t)this & 1))
      return ;

   const char_type (&empty)[1] = literals<char_type>::empty_string;
   const char_type (&some_string)[37] = literals<char_type>::some_string;

   // From string literal
   istring cs101 ;
   istring cs102(empty) ;
   istring cs103(empty, 0, size_t(0)) ;
   istring cs106(some_string) ;
   istring cs107(some_string, 0, size_t(0)) ;

   // From std::basic_string<>
   std_string ss21(random_string(8)) ;
   istring cs21(ss21) ;
   istring cs22(ss21, 4, istring::npos) ;
   istring cs23(ss21, 4, 2) ;

   // From const char_type *
   const char_type* const cb(some_string) ;
   dummy_strfn(cb) ;
   const char_type* b(some_string) ;
   const char_type* e(some_string + sizeof(some_string) / sizeof(*some_string)) ;

   istring cs301(b) ;
   istring cs302(b, 0, 10) ;
   istring cs307(b, e) ;

   // From immutable_string
   istring cs41(cs301) ;
   istring cs42(cs301, 0, 10) ;
   istring cs43(cs301, 10, 10) ;

   // other
   istring from_char_rep(3, 'x') ;
   istring from_mutable_strbuf ((strbuf)some_string) ;
}

template<class ImmutableString>
void ImmutableStringTests<ImmutableString>::Test_Constructors_Invariants()
{
   const char_type (&empty)[1] = literals<char_type>::empty_string;
   //const char_type (&some_string)[37] = literals<char_type>::some_string;

   { // equality / inequality
      istring a;
      CPPUNIT_LOG_ASSERT(!a.size()) ;
      CPPUNIT_LOG_ASSERT(a.empty()) ;
      CPPUNIT_LOG_ASSERT(a.begin() == a.end()) ;
      CPPUNIT_LOG_ASSERT(a.rbegin() == a.rend()) ;

      CPPUNIT_LOG_ASSERT(a == empty) ;
      CPPUNIT_LOG_IS_FALSE(a != empty) ;
      CPPUNIT_LOG_IS_FALSE(a < empty) ;
      CPPUNIT_LOG_IS_FALSE(a > empty) ;

      istring b(empty);
      CPPUNIT_LOG_ASSERT(!b.size()) ;
      CPPUNIT_LOG_ASSERT(b.empty()) ;
      CPPUNIT_LOG_ASSERT(b.begin() == b.end()) ;
      CPPUNIT_LOG_ASSERT(b.rbegin() == b.rend()) ;

      CPPUNIT_LOG_ASSERT(b == empty) ;
      CPPUNIT_LOG_IS_FALSE(b != empty) ;
      CPPUNIT_LOG_IS_FALSE(b < empty) ;
      CPPUNIT_LOG_IS_FALSE(b > empty) ;

      CPPUNIT_LOG_ASSERT(a == b) ;
      // Empty immutable strings created in the same module _should_ point
      // to the same data! Reference counting assumed.
      CPPUNIT_LOG_EQUAL(a.c_str(), b.c_str()) ;
   }

   { // construction
      for(size_t n = 128 ; n ; --n)
      {
         std_string const random_stdstr(random_string(n));

         istring from_stdstr(random_stdstr);
         CPPUNIT_ASSERT(from_stdstr == random_stdstr) ;
         CPPUNIT_IS_FALSE(from_stdstr != random_stdstr) ;
         CPPUNIT_IS_FALSE(from_stdstr < random_stdstr) ;
         CPPUNIT_IS_FALSE(from_stdstr > random_stdstr) ;
         CPPUNIT_EQUAL(random_stdstr.size(),
                       from_stdstr.size()) ;
         CPPUNIT_EQUAL(random_stdstr.size(),
                       (size_t)std::distance(from_stdstr.begin(), from_stdstr.end())) ;
         CPPUNIT_EQUAL(random_stdstr.size(),
                       (size_t)std::distance(from_stdstr.rbegin(), from_stdstr.rend())) ;


         const char_type* const random_cstr(random_stdstr.c_str());
         istring from_cstr(random_cstr) ;
         CPPUNIT_ASSERT(from_cstr == random_cstr) ;
         CPPUNIT_IS_FALSE(from_cstr != random_cstr) ;
         CPPUNIT_IS_FALSE(from_cstr < random_cstr) ;
         CPPUNIT_IS_FALSE(from_cstr > random_cstr) ;
         CPPUNIT_EQUAL(std::char_traits<char_type>::length(random_cstr),
                       from_cstr.size()) ;
         CPPUNIT_EQUAL(std::char_traits<char_type>::length(random_cstr),
                       (size_t)std::distance(from_cstr.begin(), from_cstr.end())) ;
         CPPUNIT_EQUAL(std::char_traits<char_type>::length(random_cstr),
                       (size_t)std::distance(from_cstr.rbegin(), from_cstr.rend())) ;

         CPPUNIT_ASSERT_THROW(istring bad(random_stdstr, random_stdstr.size() + 1, random_stdstr.size()),
                              std::out_of_range) ;
         CPPUNIT_EQUAL(istring(random_stdstr, random_stdstr.size(), istring::npos),
                       istring()) ;
         CPPUNIT_EQUAL(istring(random_stdstr, random_stdstr.size() - 1, 10),
                       istring(random_stdstr.substr(random_stdstr.size() - 1, 1))) ;
         CPPUNIT_EQUAL(istring(random_stdstr, random_stdstr.size() - 1, 1),
                       istring(random_stdstr.substr(random_stdstr.size() - 1, 1))) ;
         CPPUNIT_EQUAL(istring(random_stdstr, 0, random_stdstr.size() + 10),
                       istring(random_stdstr)) ;
         CPPUNIT_EQUAL(istring(random_stdstr, 0, random_stdstr.size()),
                       istring(random_stdstr)) ;
      }
   }
}

template<class ImmutableString>
void ImmutableStringTests<ImmutableString>::Test_Concatenation()
{
   // concatenation
   std_string s1(random_string(8));
   std_string s2(random_string(8));
   std_string s3(random_string(8));
   std_string s4(random_string(8));
   std_string s5(random_string(8));
   std_string s6(random_string(8));
   std_string s7(random_string(8));
   std_string s8(random_string(8));
   const char_type* const p1(s1.c_str());
   const char_type* const p2(s2.c_str());
   const char_type* const p3(s3.c_str());
   const char_type* const p4(s4.c_str());
   const char_type* const p5(s5.c_str());
   const char_type* const p6(s6.c_str());
   const char_type* const p7(s7.c_str());
   const char_type* const p8(s8.c_str());

   CPPUNIT_ASSERT(p1) ;

   const std_string a1(s1 + p2 + s3 + p4 + s5 + p6 + s7 + p8) ;
   const istring b1(istring(s1) + p2 + s3 + p4 + s5 + p6 + s7 + p8) ;
   const istring b2(istring(s1) + s2 + p3 + s4 + p5 + s6 + p7 + s8) ;
   CPPUNIT_LOG_EQUAL(b1.size(), a1.size()) ;
   CPPUNIT_LOG_EQUAL(std_string(pcomn::str::cstr(b1)), a1) ;
   CPPUNIT_LOG_ASSERT(b1 == a1) ;
   CPPUNIT_LOG_ASSERT(a1 == b1) ;
   CPPUNIT_LOG_EQUAL(b1, b2) ;

   strbuf mstr (P_CSTR(char_type, "Hello")) ;
   CPPUNIT_LOG_EQUAL(istring(((mstr += char_type(',')) += P_CSTR(char_type, " world")) += char_type('!')),
                     istring(P_CSTR(char_type, "Hello, world!"))) ;
   CPPUNIT_LOG_EQUAL(istring(istring(P_CSTR(char_type, "Hello")) + char_type(',')
                             + P_CSTR(char_type, " world") + char_type('!')),
                     istring(P_CSTR(char_type, "Hello, world!"))) ;
   CPPUNIT_LOG_ASSERT(istring(P_CSTR(char_type, "Hello")) + char_type(',')
                      + P_CSTR(char_type, " world") + char_type('!')
                      == istring(P_CSTR(char_type, "Hello, world!"))) ;
}

template<class ImmutableString>
void ImmutableStringTests<ImmutableString>::Test_Mutable_Strbuf()
{
   strbuf buf1 ;
   CPPUNIT_LOG_ASSERT(buf1.empty()) ;
   CPPUNIT_LOG_RUN(buf1.resize(0, char_type('A'))) ;
   CPPUNIT_LOG_ASSERT(buf1.empty()) ;
   CPPUNIT_LOG_RUN(buf1.resize(10, char_type('A'))) ;
   CPPUNIT_LOG_IS_FALSE(buf1.empty()) ;
   CPPUNIT_LOG_EQUAL(buf1.size(), (size_t)10) ;
   CPPUNIT_LOG_EQUAL(std::char_traits<char_type>::length(buf1.c_str()), (size_t)10) ;
   CPPUNIT_LOG_EQUAL(std_string(buf1.c_str()), std_string(P_CSTR(char_type, "AAAAAAAAAA"))) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(buf1.resize((size_t)5, char_type('A'))) ;
   CPPUNIT_LOG_IS_FALSE(buf1.empty()) ;
   CPPUNIT_LOG_EQUAL(buf1.size(), (size_t)5) ;
   CPPUNIT_LOG_EQUAL(std::char_traits<char_type>::length(buf1.c_str()), (size_t)5) ;
   CPPUNIT_LOG_EQUAL(std_string(buf1.c_str()), std_string(P_CSTR(char_type, "AAAAA"))) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_IS_FALSE(buf1.append(10, char_type('C')).empty()) ;
   CPPUNIT_LOG_EQUAL(buf1.size(), (size_t)15) ;
   CPPUNIT_LOG_EQUAL(std::char_traits<char_type>::length(buf1.c_str()), (size_t)15) ;
   CPPUNIT_LOG_EQUAL(std_string(buf1.c_str()), std_string(P_CSTR(char_type, "AAAAACCCCCCCCCC"))) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(buf1.resize(0, char_type('A'))) ;
   CPPUNIT_LOG_ASSERT(buf1.empty()) ;
   CPPUNIT_LOG_EQUAL(buf1.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(std::char_traits<char_type>::length(buf1.c_str()), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf1.append(P_CSTR(char_type, "Hello, world!"), 5).size(), (size_t)5) ;
   CPPUNIT_LOG_EQUAL(buf1, strbuf(P_CSTR(char_type, "Hello"))) ;
   const std_string w (P_CSTR(char_type, ", world!")) ;
   std::list<char_type> world_list (w.begin(), w.end()) ;
   CPPUNIT_LOG_EQUAL(buf1.append(world_list.begin(), world_list.size()),
                     strbuf(P_CSTR(char_type, "Hello, world!"))) ;
}

template<class ImmutableString>
void ImmutableStringTests<ImmutableString>::Test_To_Upper_Lower()
{
   istring locase (literals<char_type>::lower_case) ;
   istring upcase (literals<char_type>::upper_case) ;
   const char_type * const lo_cstr = locase.c_str() ;
   const char_type * const up_cstr = upcase.c_str() ;

   CPPUNIT_LOG_ASSERT(upcase != locase) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::to_lower(upcase), locase) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::to_lower(locase), locase) ;
   CPPUNIT_LOG_ASSERT(upcase != locase) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::to_upper(locase), upcase) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::to_upper(upcase), upcase) ;
   CPPUNIT_LOG_ASSERT(upcase != locase) ;
   CPPUNIT_LOG_ASSERT(locase.c_str() == lo_cstr) ;
   CPPUNIT_LOG_ASSERT(upcase.c_str() == up_cstr) ;
}

template<class ImmutableString>
void ImmutableStringTests<ImmutableString>::Test_Comparison()
{
   typedef literals<char_type> lit ;
   const istring foo = lit::foo ;
   const std_string stdfoo = (std_string)foo ;
   const string_slice sfoo = foo ;

   CPPUNIT_LOG_EQUAL(std_string(foo), std_string(lit::foo)) ;
   CPPUNIT_LOG_NOT_EQUAL(istring(lit::foo), istring(lit::Foo)) ;

   CPPUNIT_LOG_EQ(sfoo, string_slice(lit::foo)) ;

   CPPUNIT_LOG_ASSERT(istring(lit::foo) > string_slice(lit::foo)(0, 1)) ;
}

template<class ImmutableString>
void ImmutableStringTests<ImmutableString>::Test_Find()
{
}


int main(int argc, char *argv[])
{
   return pcomn::unit::run_tests
       <
          ImmutableStringTests<pcomn::immutable_string<char>>,
          ImmutableStringTests<pcomn::immutable_string<wchar_t>>
       >
       (argc, argv) ;
}
