/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_sys.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for routines from pcomn::sys namespace

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Jan 2009
*******************************************************************************/
#include <pcomn_path.h>
#include <pcomn_unittest.h>
#include <pcomn_string.h>
#include <pcomn_except.h>
#include <pcomn_iterator.h>
#include <pcomn_handle.h>
#include <pcomn_sys.h>

#include <iostream>

using namespace pcomn ;

class FilesystemTests : public CppUnit::TestFixture {

      void Test_Filesystem_Path() ;
      void Test_Filesystem_RealPath() ;

      CPPUNIT_TEST_SUITE(FilesystemTests) ;

      CPPUNIT_TEST(Test_Filesystem_Path) ;
      CPPUNIT_TEST(Test_Filesystem_RealPath) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;


using pcomn::path::abspath ;
using pcomn::path::normpath ;
using pcomn::path::joinpath ;
using pcomn::path::realpath ;
using pcomn::path::posix::path_dots ;
using pcomn::strslice_pair ;

void FilesystemTests::Test_Filesystem_Path()
{
   char buf[PATH_MAX + 1] ;
   char cwd[PATH_MAX + 1] ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(path_dots("."), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(path_dots(".hello"), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(path_dots(".."), (size_t)2) ;
   CPPUNIT_LOG_EQUAL(path_dots("../hello"), (size_t)2) ;
   CPPUNIT_LOG_EQUAL(path_dots("./hello"), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(path_dots("/hello"), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(path_dots("hello"), (size_t)0) ;

   static const char hello[] = "../hello" ;
   static const char * const hello_begin = hello ;
   static const char * const hello_end = hello + sizeof hello ;

   CPPUNIT_LOG_EQUAL(path_dots(hello_begin, hello_begin), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(path_dots(NULL, NULL), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(path_dots(hello_begin, hello_begin + 1), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(path_dots(hello_begin, hello_begin + 2), (size_t)2) ;
   CPPUNIT_LOG_EQUAL(path_dots(hello_begin, hello_begin + 3), (size_t)2) ;
   CPPUNIT_LOG_EQUAL(path_dots(hello_begin + 1, hello_begin + 3), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(path_dots(hello_begin + 1, hello_begin + 2), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(path_dots(hello_begin + 2, hello_end), (size_t)0) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQUAL(joinpath<std::string>("", ""), std::string()) ;
   CPPUNIT_LOG_EQUAL(joinpath<std::string>(".", ""), std::string(".")) ;
   CPPUNIT_LOG_EQUAL(joinpath<std::string>("/", "a/b"), std::string("/a/b")) ;
   CPPUNIT_LOG_EQUAL(joinpath<std::string>("a", "b/c"), std::string("a/b/c")) ;
   CPPUNIT_LOG_EQUAL(joinpath<std::string>("/a", "b/c"), std::string("/a/b/c")) ;
   CPPUNIT_LOG_EQUAL(joinpath<std::string>("/a/", "b/c"), std::string("/a/b/c")) ;
   CPPUNIT_LOG_EQUAL(joinpath<std::string>("/a", "/b/c"), std::string("/b/c")) ;
   CPPUNIT_LOG_EQUAL(joinpath<std::string>("/a/", "/b/c"), std::string("/b/c")) ;
   CPPUNIT_LOG_EQUAL(joinpath<std::string>("", "b/c"), std::string("b/c")) ;
   CPPUNIT_LOG_EQUAL(joinpath<std::string>("abc", ""), std::string("abc")) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQUAL(normpath<std::string>(""), std::string()) ;
   CPPUNIT_LOG_EQUAL(normpath("", buf, sizeof buf), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("."), std::string(".")) ;
   CPPUNIT_LOG_EQUAL(normpath(".", buf, sizeof buf), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>(".."), std::string("..")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("..//."), std::string("..")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>(".//.."), std::string("..")) ;

   CPPUNIT_LOG_EQUAL(normpath<std::string>("./hello"), std::string("hello")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("../hello"), std::string("../hello")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("hello/.."), std::string(".")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("../hello/.."), std::string("..")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("hello/../.."), std::string("..")) ;

   CPPUNIT_LOG_EQUAL(normpath<std::string>("/"), std::string("/")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("//"), std::string("/")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("///"), std::string("/")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("/.."), std::string("/")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("/h/.."), std::string("/")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("//h/.."), std::string("/")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("//h//.."), std::string("/")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("//.."), std::string("/")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("//."), std::string("/")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("//h//."), std::string("/h")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("/../hello"), std::string("/hello")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("./hello/../world"), std::string("world")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("/..///../world/."), std::string("/world")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>("/foo/../hello"), std::string("/hello")) ;
   CPPUNIT_LOG_EQUAL(normpath<std::string>(strslice("/foo/../hello/w")(0, -2)), std::string("/hello")) ;

   CPPUNIT_LOG_ASSERT(getcwd(cwd, sizeof cwd)) ;

   CPPUNIT_LOG_EQUAL(abspath<std::string>(""), std::string()) ;
   CPPUNIT_LOG_EQUAL(abspath("", buf, sizeof buf), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(abspath<std::string>("."), std::string(cwd)) ;
   CPPUNIT_LOG_EQUAL(abspath<std::string>(strslice(".")), std::string(cwd)) ;
   CPPUNIT_LOG(abspath<std::string>(".") << std::endl) ;
   CPPUNIT_LOG_EQUAL(abspath(".", buf, sizeof buf), strlen(cwd)) ;
   CPPUNIT_LOG_EQUAL(abspath<std::string>("./hello/../world"), std::string(cwd) + "/world") ;
   CPPUNIT_LOG_EQUAL(abspath<std::string>("/..///../world/."), std::string("/world")) ;
   CPPUNIT_LOG_EQUAL(abspath<std::string>("/..///../world/m"), std::string("/world/m")) ;
   CPPUNIT_LOG_EQUAL(abspath<std::string>(strslice("/..///../world/m")(0, -2)), std::string("/world")) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_IS_TRUE(path::posix::is_absolute(std::string("/world"))) ;
   CPPUNIT_LOG_IS_TRUE(path::posix::is_absolute("/")) ;
   CPPUNIT_LOG_IS_FALSE(path::posix::is_absolute("")) ;
   CPPUNIT_LOG_IS_TRUE(path::posix::is_rooted(std::string("/world"))) ;
   CPPUNIT_LOG_IS_TRUE(path::posix::is_rooted("/")) ;
   CPPUNIT_LOG_IS_FALSE(path::posix::is_rooted("")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(path::posix::split(""), strslice_pair()) ;
   CPPUNIT_LOG_EQUAL(path::posix::split("."), strslice_pair(".", "")) ;
   CPPUNIT_LOG_EQUAL(path::posix::split(".."), strslice_pair("..", "")) ;
   CPPUNIT_LOG_EQUAL(path::posix::split("/"), strslice_pair("/", "")) ;
   CPPUNIT_LOG_EQUAL(path::posix::split("hello"), strslice_pair("", "hello")) ;
   CPPUNIT_LOG_EQUAL(path::posix::split("/hello"), strslice_pair("/", "hello")) ;
   CPPUNIT_LOG_EQUAL(path::posix::split("/hello/"), strslice_pair("/hello", "")) ;
   CPPUNIT_LOG_EQUAL(path::posix::split("/hello/world"), strslice_pair("/hello", "world")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(path::posix::basename("/hello/world.txt"), pcomn::strslice("world.txt")) ;
   CPPUNIT_LOG_EQUAL(path::posix::basename("world.txt"), pcomn::strslice("world.txt")) ;
   CPPUNIT_LOG_EQUAL(path::posix::basename("/hello/"), pcomn::strslice()) ;
   CPPUNIT_LOG_EQUAL(path::posix::basename("/"), pcomn::strslice()) ;
   CPPUNIT_LOG_EQUAL(path::posix::basename("."), pcomn::strslice()) ;
   CPPUNIT_LOG_EQUAL(path::posix::basename(".."), pcomn::strslice()) ;
   CPPUNIT_LOG_EQUAL(path::posix::basename("/hello"), pcomn::strslice("hello")) ;

   CPPUNIT_LOG_EQUAL(path::posix::basename(pcomn::strslice("world.txt/")(0, -1)), pcomn::strslice("world.txt")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(path::posix::dirname("/hello/world.txt"), pcomn::strslice("/hello")) ;
   CPPUNIT_LOG_EQUAL(path::posix::dirname("world.txt"), pcomn::strslice()) ;
   CPPUNIT_LOG_EQUAL(path::posix::dirname("/hello/"), pcomn::strslice("/hello")) ;
   CPPUNIT_LOG_EQUAL(path::posix::dirname("/"), pcomn::strslice("/")) ;
   CPPUNIT_LOG_EQUAL(path::posix::dirname("."), pcomn::strslice(".")) ;
   CPPUNIT_LOG_EQUAL(path::posix::dirname(".."), pcomn::strslice("..")) ;
   CPPUNIT_LOG_EQUAL(path::posix::dirname("/hello"), pcomn::strslice("/")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(path::splitext(""), pcomn::strslice_pair()) ;
   CPPUNIT_LOG_EQUAL(path::splitext("."), pcomn::strslice_pair(".", "")) ;
   CPPUNIT_LOG_EQUAL(path::splitext(".."), pcomn::strslice_pair("..", "")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("../"), pcomn::strslice_pair("../", "")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("../hello.world/"), pcomn::strslice_pair("../hello.world/", "")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("abc.txt"), pcomn::strslice_pair("abc", ".txt")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("abc.d"), pcomn::strslice_pair("abc", ".d")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("abc.d.ef"), pcomn::strslice_pair("abc.d", ".ef")) ;
   CPPUNIT_LOG_EQUAL(path::splitext(path::splitext("abc.d.ef").first), pcomn::strslice_pair("abc", ".d")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("hello.world/abc.d"), pcomn::strslice_pair("hello.world/abc", ".d")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("hello.world/abc.d.ef"), pcomn::strslice_pair("hello.world/abc.d", ".ef")) ;
   CPPUNIT_LOG_EQUAL(path::splitext(".abc"), pcomn::strslice_pair(".abc", "")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("hello.world/.abc"), pcomn::strslice_pair("hello.world/.abc", "")) ;
}

void FilesystemTests::Test_Filesystem_RealPath()
{
   char buf[PATH_MAX + 1] ;
   char cwd[PATH_MAX + 1] ;

   CPPUNIT_LOG_ASSERT(getcwd(cwd, sizeof cwd)) ;

   CPPUNIT_LOG_EQUAL(realpath<std::string>(""), std::string()) ;
   CPPUNIT_LOG_EQUAL(realpath("", buf, sizeof buf), (ssize_t)0) ;
   CPPUNIT_LOG_EQUAL(realpath<std::string>("."), std::string(cwd)) ;
   CPPUNIT_LOG_EXPRESSION(realpath<std::string>(".")) ;
   CPPUNIT_LOG_EQUAL(realpath(".", buf, sizeof buf), (ssize_t)strlen(cwd)) ;
   CPPUNIT_LOG_EQUAL(realpath<std::string>("./hello/../world"), std::string(cwd) + "/world") ;
   CPPUNIT_LOG_EQUAL(realpath<std::string>("/..///../world/."), std::string("/world")) ;

   CPPUNIT_LOG(std::endl) ;
   const std::string slink1 (abspath<std::string>(CPPUNIT_AT_PROGDIR("slink1"))) ;
   const std::string slink2 (abspath<std::string>(CPPUNIT_AT_PROGDIR("slink2"))) ;
   const std::string slink3 (abspath<std::string>(CPPUNIT_AT_PROGDIR("slink3"))) ;
   const std::string foobar (abspath<std::string>(CPPUNIT_AT_PROGDIR("foobar"))) ;

   unlink(slink1.c_str()) ;
   unlink(slink2.c_str()) ;
   unlink(slink3.c_str()) ;
   pcomn::FILE_safehandle(fopen(foobar.c_str(), "w")) ;

   CPPUNIT_LOG_ASSERT(access(foobar.c_str(), F_OK) == 0) ;
   CPPUNIT_LOG_EQUAL(realpath<std::string>(foobar), foobar) ;
   CPPUNIT_LOG_EQUAL(realpath<std::string>(slink1), slink1) ;

   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(symlink("slink2", slink1.c_str()), "symlink")) ;
   CPPUNIT_LOG_EQUAL(realpath<std::string>(slink1), slink2) ;

   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(symlink("slink3", slink2.c_str()), "symlink")) ;
   CPPUNIT_LOG_EQUAL(realpath<std::string>(slink1), slink3) ;

   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(symlink("foobar", slink3.c_str()), "symlink")) ;
   CPPUNIT_LOG_EQUAL(realpath<std::string>(slink1), foobar) ;

   unlink(slink3.c_str()) ;
   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(symlink("slink3", slink3.c_str()), "symlink")) ;
   CPPUNIT_LOG_EQUAL(realpath<std::string>(slink3), std::string()) ;
   CPPUNIT_LOG_EQUAL(realpath(slink3.c_str(), buf, sizeof buf), (ssize_t)-1) ;
   errno = 0 ;
   realpath(slink3.c_str(), buf, sizeof buf) ;
   CPPUNIT_LOG_EQUAL(errno, ELOOP) ;

   unlink(slink3.c_str()) ;
   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(symlink("slink1", slink3.c_str()), "symlink")) ;
   CPPUNIT_LOG_EQUAL(realpath(slink1.c_str(), buf, sizeof buf), (ssize_t)-1) ;
   CPPUNIT_LOG_EQUAL(realpath(slink2.c_str(), buf, sizeof buf), (ssize_t)-1) ;
   CPPUNIT_LOG_EQUAL(realpath(slink3.c_str(), buf, sizeof buf), (ssize_t)-1) ;
   errno = 0 ;
   realpath(slink1.c_str(), buf, sizeof buf) ;
   CPPUNIT_LOG_EQUAL(errno, ELOOP) ;

   unlink(slink3.c_str()) ;
   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(symlink(foobar.c_str(), slink3.c_str()), "symlink")) ;
   CPPUNIT_LOG_EQUAL(realpath<std::string>(slink1), foobar) ;
}

/*******************************************************************************
                     class SysDirTests
*******************************************************************************/
extern const char SYSDIR_FIXTURE[] = "sysdir" ;
class SysDirTests : public pcomn::unit::TestFixture<SYSDIR_FIXTURE> {

      void Test_Opendir() ;

      CPPUNIT_TEST_SUITE(SysDirTests) ;

      CPPUNIT_TEST(Test_Opendir) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

void SysDirTests::Test_Opendir()
{
   using namespace pcomn ;
   std::vector<std::string> content ;

   CPPUNIT_LOG_ASSERT(sys::opendir(str::cstr(dataDir()), sys::ODIR_CLOSE_DIR, appender(content), RAISE_ERROR) >= 0) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(content), CPPUNIT_STRVECTOR((".")(".."))) ;
   swap_clear(content) ;

   CPPUNIT_LOG_ASSERT(sys::opendir(str::cstr(dataDir() + "/foo"), sys::ODIR_CLOSE_DIR, appender(content), DONT_RAISE_ERROR) < 0) ;
   CPPUNIT_LOG_EQUAL(content, CPPUNIT_STRVECTOR()) ;
   CPPUNIT_LOG_EXCEPTION(sys::opendir(str::cstr(dataDir() + "/foo"), sys::ODIR_CLOSE_DIR, appender(content), RAISE_ERROR), system_error) ;
   CPPUNIT_LOG_EQUAL(content, CPPUNIT_STRVECTOR()) ;
   // Test defaults: must be DONT_RAISE_ERROR
   CPPUNIT_LOG_ASSERT(sys::opendir(str::cstr(dataDir() + "/foo"), sys::ODIR_CLOSE_DIR, appender(content)) < 0) ;
   CPPUNIT_LOG_EQUAL(content, CPPUNIT_STRVECTOR()) ;
   CPPUNIT_LOG_ASSERT(sys::opendir(str::cstr(dataDir() + "/foo"), 0, appender(content)) < 0) ;
   CPPUNIT_LOG_EQUAL(content, CPPUNIT_STRVECTOR()) ;

   CPPUNIT_LOG(std::endl) ;
   unit::generate_seqn_file<4>(dataDir() + "/bar", 1) ;
   unit::generate_seqn_file<4>(dataDir() + "/quux", 2) ;
   fd_safehandle dfd ;
   CPPUNIT_LOG_RUN(dfd.reset(sys::opendir(str::cstr(dataDir()), 0, appender(content), RAISE_ERROR))) ;
   CPPUNIT_LOG_ASSERT(dfd.handle() >= 0) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(content), CPPUNIT_STRVECTOR((".")("..")("bar")("quux"))) ;
   swap_clear(content) ;
   CPPUNIT_LOG_EQUAL(sys::filesize(dfd.handle(), "bar"), (off_t)4) ;
   CPPUNIT_LOG_EQUAL(sys::filesize(dfd.handle(), "quux"), (off_t)8) ;
   dfd.reset() ;

   swap_clear(content) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(sys::ls(str::cstr(dataDir()), 0, appender(content), RAISE_ERROR).container()),
                     CPPUNIT_STRVECTOR((".")("..")("bar")("quux"))) ;
   swap_clear(content) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(sys::ls(str::cstr(dataDir()), sys::ODIR_SKIP_DOT, appender(content), RAISE_ERROR).container()),
                     CPPUNIT_STRVECTOR(("..")("bar")("quux"))) ;
   swap_clear(content) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(sys::ls(str::cstr(dataDir()), sys::ODIR_SKIP_DOTDOT, appender(content), RAISE_ERROR).container()),
                     CPPUNIT_STRVECTOR((".")("bar")("quux"))) ;
   swap_clear(content) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(sys::ls(str::cstr(dataDir()), sys::ODIR_SKIP_DOTS, appender(content), RAISE_ERROR).container()),
                     CPPUNIT_STRVECTOR(("bar")("quux"))) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(FilesystemTests::suite()) ;
   runner.addTest(SysDirTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "pcomn::sys tests") ;
}
