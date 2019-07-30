/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_strshims.cpp
 COPYRIGHT    :   Yakov Markovitch, 2006-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittest for pcommon string "access shim" functions (i.e.
                  functions that cast objects of different string types to
                  common "simple" types, e.g. char *, wchar_t *)

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   7 Dec 2006
*******************************************************************************/
#include <pcomn_string.h>
#include <pcomn_immutablestr.h>
#include <pcomn_unittest.h>
#include <pcomn_strslice.h>
#include <pcomn_function.h>
#include <pcomn_cstrptr.h>
#include <pcstring.h>

#include <sstream>
#include <memory>

template<typename C>
using primitive_array = std::unique_ptr<C[], pcomn::identity> ;

typedef char char_buffer[80] ;

template<typename T>
struct string_init {
      T &operator ()(T &dest, const T &src) const
      {
         return dest = src ;
      }
} ;

template<typename C, size_t n>
struct string_init<C[n]> {
      const char *operator ()(C (&dest)[n], const C *src) const
      {
         return strncpy(dest, src, n) ;
      }
} ;

/*******************************************************************************
                     class StringTraitsTests
*******************************************************************************/
class StringTraitsTests : public CppUnit::TestFixture {

      void Test_Traits() ;

      CPPUNIT_TEST_SUITE(StringTraitsTests) ;

      CPPUNIT_TEST(Test_Traits) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
                     class StringShimTests
*******************************************************************************/
class StringShimTests : public CppUnit::TestFixture {

      template<class T>
      void Test_Shims() ;

      void Test_Cstr_Ptr() ;

      CPPUNIT_TEST_SUITE(StringShimTests) ;

      CPPUNIT_TEST(Test_Shims<const char *>) ;
      CPPUNIT_TEST(Test_Shims<std::string>) ;
      CPPUNIT_TEST(Test_Shims<pcomn::cstrptr>) ;
      CPPUNIT_TEST(Test_Shims<const wchar_t *>) ;
      CPPUNIT_TEST(Test_Shims<std::wstring>) ;
      CPPUNIT_TEST(Test_Shims<pcomn::cwstrptr>) ;

      CPPUNIT_TEST(Test_Cstr_Ptr) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
                     class StringFunctionTests
*******************************************************************************/
class StringFunctionTests : public CppUnit::TestFixture {

      template<class T>
      void Test_Narrow_Output() ;
      template<class T>
      void Test_Strip() ;
      template<class T>
      void Test_Strip_Inplace() ;
      template<class T>
      void Test_To_Upper_Lower_Inplace() ;
      template<class T>
      void Test_To_Upper_Lower() ;

      CPPUNIT_TEST_SUITE(StringFunctionTests) ;

      CPPUNIT_TEST(Test_Narrow_Output<std::string>) ;
      CPPUNIT_TEST(Test_Narrow_Output<std::wstring>) ;
      CPPUNIT_TEST(Test_Narrow_Output<pcomn::istring>) ;
      CPPUNIT_TEST(Test_Narrow_Output<pcomn::iwstring>) ;
      CPPUNIT_TEST(Test_Strip_Inplace<std::string>) ;
      CPPUNIT_TEST(Test_Strip_Inplace<std::wstring>) ;
      //CPPUNIT_TEST(Test_Strip_Inplace<pcomn::istring>) ;
      //CPPUNIT_TEST(Test_Strip_Inplace<pcomn::iwstring>) ;
      CPPUNIT_TEST(Test_Strip<std::string>) ;
      CPPUNIT_TEST(Test_Strip<std::wstring>) ;
      CPPUNIT_TEST(Test_Strip<pcomn::cstrptr>) ;
      CPPUNIT_TEST(Test_Strip<pcomn::cwstrptr>) ;
      CPPUNIT_TEST(Test_Strip<pcomn::istring>) ;
      CPPUNIT_TEST(Test_Strip<pcomn::iwstring>) ;
      CPPUNIT_TEST(Test_Strip<const char *>) ;
      CPPUNIT_TEST(Test_Strip<const wchar_t *>) ;
      CPPUNIT_TEST(Test_Strip<std::unique_ptr<char[]> >) ;
      CPPUNIT_TEST(Test_Strip<std::unique_ptr<const char[]> >) ;
      CPPUNIT_TEST(Test_Strip<primitive_array<const char> >) ;
      CPPUNIT_TEST(Test_To_Upper_Lower_Inplace<std::string>) ;
      CPPUNIT_TEST(Test_To_Upper_Lower_Inplace<char_buffer>) ;
      CPPUNIT_TEST(Test_To_Upper_Lower<std::string>) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 StringTraitsTests
*******************************************************************************/
void StringTraitsTests::Test_Traits()
{
   CPPUNIT_LOG_IS_TRUE(pcomn::is_string<const char *>::value) ;
   CPPUNIT_LOG_IS_TRUE(pcomn::is_string<std::string>::value) ;
   CPPUNIT_LOG_IS_FALSE(pcomn::is_string<int>::value) ;
   CPPUNIT_LOG_IS_FALSE(pcomn::is_string<void>::value) ;

   CPPUNIT_LOG_IS_TRUE((pcomn::is_strchar<const char *, char>::value)) ;
   CPPUNIT_LOG_IS_TRUE((pcomn::is_strchar<const wchar_t *, wchar_t>::value)) ;
   CPPUNIT_LOG_IS_FALSE((pcomn::is_strchar<const char *, wchar_t>::value)) ;
   CPPUNIT_LOG_IS_FALSE((pcomn::is_strchar<const wchar_t *, char>::value)) ;
   CPPUNIT_LOG_IS_FALSE((pcomn::is_strchar<void, char>::value)) ;
   CPPUNIT_LOG_IS_FALSE((pcomn::is_strchar<int, char>::value)) ;
   CPPUNIT_LOG_IS_TRUE((pcomn::is_strchar<std::string, char>::value)) ;
   CPPUNIT_LOG_IS_FALSE((pcomn::is_strchar<std::string, wchar_t>::value)) ;
   CPPUNIT_LOG_IS_FALSE((pcomn::is_strchar<std::string, void>::value)) ;
   CPPUNIT_LOG_IS_FALSE((pcomn::is_strchar<std::string, int>::value)) ;
}

/*******************************************************************************
 StringShimTests
*******************************************************************************/
template<typename T>
struct TestData {
      static const T empty_string ;
      static const T hello_world_string ;
      static const T single_char_string ;
      static const T whitespaces_string ;
      static const T non_stripped_string ;
      static const T left_stripped_string ;
      static const T right_stripped_string ;
      static const T stripped_string ;
      static const T upper_string ;
      static const T lower_string ;
      static const T mixed_string ;
      typedef typename pcomn::string_traits<T>::char_type char_type ;
} ;

#define TEST_ITEM(type, strpfx, name, value) \
template<> type const TestData<type>::name (strpfx##value)

#define TEST_DATA(type, strpfx)                                         \
   TEST_ITEM(type, strpfx, empty_string, "") ;                          \
   TEST_ITEM(type, strpfx, hello_world_string, "Hello, world!") ;       \
   TEST_ITEM(type, strpfx, single_char_string, "a") ;                   \
   TEST_ITEM(type, strpfx, whitespaces_string, " \t\n\r") ;             \
   TEST_ITEM(type, strpfx, non_stripped_string, "\n\n\t Foo, bar! \n") ; \
   TEST_ITEM(type, strpfx, left_stripped_string, "Foo, bar! \n") ;      \
   TEST_ITEM(type, strpfx, right_stripped_string, "\n\n\t Foo, bar!") ; \
   TEST_ITEM(type, strpfx, stripped_string, "Foo, bar!") ;              \
   TEST_ITEM(type, strpfx, upper_string, "FOO, BAR!") ;                 \
   TEST_ITEM(type, strpfx, lower_string, "foo, bar!") ;                 \
   TEST_ITEM(type, strpfx, mixed_string, "Foo, bar!")

TEST_DATA(const char *,) ;
TEST_DATA(std::string,) ;
TEST_DATA(const wchar_t *, L) ;
TEST_DATA(std::wstring, L) ;

TEST_DATA(char_buffer,) ;

TEST_DATA(pcomn::istring,) ;
TEST_DATA(pcomn::iwstring, L) ;

TEST_DATA(primitive_array<const char>,) ;

TEST_DATA(pcomn::cstrptr,) ;
TEST_DATA(pcomn::cwstrptr, L) ;

#undef TEST_ITEM

#define TEST_ITEM(type, strpfx, name, value)                       \
template<> type const TestData<type>::name { strnew(strpfx##value) }

TEST_DATA(std::unique_ptr<char[]>,) ;
TEST_DATA(std::unique_ptr<const char[]>,) ;

template<typename T>
void StringShimTests::Test_Shims()
{
   CPPUNIT_LOG_EQUAL(pcomn::str::len(TestData<T>::empty_string), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::len(TestData<T>::hello_world_string),
                     strlen(TestData<const char *>::hello_world_string)) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::len(TestData<T>::single_char_string), (size_t)1) ;
}

void StringShimTests::Test_Cstr_Ptr()
{
   using namespace pcomn ;
   CPPUNIT_LOG_EQUAL(cstrptr("hello"), cstrptr("hello")) ;
   CPPUNIT_LOG_EQUAL(cstrptr("hello"), cstrptr(std::string("hello"))) ;
   CPPUNIT_LOG_EQUAL(cstrptr("hello"), cstrptr(std::string("hello"))) ;
}

/*******************************************************************************
 StringFunctionTests
*******************************************************************************/
template<class T>
void StringFunctionTests::Test_Narrow_Output()
{
   std::ostringstream narrow_stream ;
   CPPUNIT_LOG_ASSERT(narrow_stream << TestData<T>::hello_world_string) ;
   CPPUNIT_LOG_EQUAL(std::string(narrow_stream.str()).size(),
                     TestData<std::string>::hello_world_string.size()) ;
   CPPUNIT_LOG_EQUAL(std::string(narrow_stream.str()), TestData<std::string>::hello_world_string) ;
}

template<class T>
void StringFunctionTests::Test_Strip_Inplace()
{
   T local_whitespaces_1 (TestData<T>::whitespaces_string) ;
   T local_whitespaces_2 (local_whitespaces_1) ;
   T local_whitespaces_3 (local_whitespaces_1) ;

   T local_nonstripped_1 (TestData<T>::non_stripped_string) ;
   T local_nonstripped_2 (local_nonstripped_1) ;
   T local_nonstripped_3 (local_nonstripped_1) ;

   T local_empty ;

   CPPUNIT_LOG_EQUAL(pcomn::str::lstrip_inplace(local_whitespaces_1),
                     TestData<T>::empty_string) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::rstrip_inplace(local_whitespaces_2),
                     TestData<T>::empty_string) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::strip_inplace(local_whitespaces_3),
                     TestData<T>::empty_string) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::lstrip_inplace(local_empty), TestData<T>::empty_string) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::rstrip_inplace(local_empty), TestData<T>::empty_string) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::strip_inplace(local_empty), TestData<T>::empty_string) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::lstrip_inplace(local_nonstripped_1),
                     TestData<T>::left_stripped_string) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::rstrip_inplace(local_nonstripped_2),
                     TestData<T>::right_stripped_string) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::strip_inplace(local_nonstripped_3),
                     TestData<T>::stripped_string) ;
}

template<class T>
void StringFunctionTests::Test_Strip()
{
   typedef TestData<T> data ;
   typedef pcomn::basic_strslice<typename data::char_type> slc ;
   static const slc empty_string ;

   CPPUNIT_LOG_EQUAL(pcomn::str::lstrip(data::whitespaces_string), empty_string) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::rstrip(data::whitespaces_string), empty_string) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::strip(data::whitespaces_string), empty_string) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::lstrip(empty_string), empty_string) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::rstrip(empty_string), empty_string) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::strip(empty_string), empty_string) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::lstrip(data::non_stripped_string), slc(data::left_stripped_string)) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::rstrip(data::non_stripped_string), slc(data::right_stripped_string)) ;
   CPPUNIT_LOG_EQUAL(pcomn::str::strip(data::non_stripped_string), slc(data::stripped_string)) ;
}


template<class T>
void StringFunctionTests::Test_To_Upper_Lower_Inplace()
{
   using namespace pcomn ;
   typedef TestData<T> data ;
   string_init<T> init ;

   T ustr ;
   init(ustr, data::upper_string) ;

   CPPUNIT_LOG_EQ(strslice(str::to_lower_inplace(ustr)), data::lower_string) ;
   CPPUNIT_LOG_EQ(strslice(ustr), data::lower_string) ;
   CPPUNIT_LOG_NOT_EQUAL(strslice(ustr), strslice(data::upper_string)) ;
   CPPUNIT_LOG_EQ(strslice(str::to_upper_inplace(ustr)), data::upper_string) ;
}

template<class T>
void StringFunctionTests::Test_To_Upper_Lower()
{
   using namespace pcomn ;
   typedef TestData<T> data ;

   T ustr (data::mixed_string) ;

   CPPUNIT_LOG_NOT_EQUAL(ustr, data::upper_string) ;
   CPPUNIT_LOG_NOT_EQUAL(ustr, data::lower_string) ;
   CPPUNIT_LOG_EQ(ustr, data::mixed_string) ;

   CPPUNIT_LOG_EQ(str::to_lower(ustr), data::lower_string) ;
   CPPUNIT_LOG_EQ(ustr, data::mixed_string) ;
   CPPUNIT_LOG_EQ(str::to_upper(ustr), data::upper_string) ;
   CPPUNIT_LOG_EQ(ustr, data::mixed_string) ;

   CPPUNIT_LOG_EQ(str::to_lower(T(data::mixed_string)), data::lower_string) ;
   CPPUNIT_LOG_EQ(str::to_upper(T(data::mixed_string)), data::upper_string) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(StringTraitsTests::suite()) ;
   runner.addTest(StringShimTests::suite()) ;
   runner.addTest(StringFunctionTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.diag.ini",
                             "String shims tests") ;
}
