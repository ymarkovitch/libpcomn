/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_filesys.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for filesystem routines

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

#include "pcomn_testhelpers.h"

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
using pcomn::path::mkdirpath ;
using pcomn::path::posix::path_dots ;
using pcomn::unipair ;
using pcomn::strslice ;

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
   CPPUNIT_LOG_EQUAL(joinpath<std::string>(".", ""), std::string("./")) ;
   CPPUNIT_LOG_EQUAL(joinpath<std::string>("", "."), std::string(".")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("/", "a/b"), std::string("/a/b")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("a", "b/c"), std::string("a/b/c")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("/a", "b/c"), std::string("/a/b/c")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("/a/", "b/c"), std::string("/a/b/c")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("/a", "/b/c"), std::string("/b/c")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("/a/", "/b/c"), std::string("/b/c")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("/a/", "/b/c", "d/"), std::string("/b/c/d/")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("/a", "/", "b", ""), std::string("/b/")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("", "", "d"), std::string("d")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("", "", "d", ""), std::string("d/")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("", "d", "", ""), std::string("d/")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("", "b/c"), std::string("b/c")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("abc", ""), std::string("abc/")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("", "b/c", "d/e", "", "f", ""), std::string("b/c/d/e/f/")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("", "b/c", "d/e", "", "f", "g"), std::string("b/c/d/e/f/g")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("", "b/c", "d/e", "qqq", "f", "g"), std::string("b/c/d/e/qqq/f/g")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("", "b/c", "d/e", "/qqq", "f", "g"), std::string("/qqq/f/g")) ;
   CPPUNIT_LOG_EQUAL(joinpath<>("", "b/c", "d/e", "qqq", "f", "/g"), std::string("/g")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(mkdirpath<std::string>(""), std::string()) ;
   CPPUNIT_LOG_EQUAL(mkdirpath<std::string>("/"), std::string("/")) ;
   CPPUNIT_LOG_EQUAL(mkdirpath<>("."), std::string("./")) ;
   CPPUNIT_LOG_EQUAL(mkdirpath<>(std::string("abc/de/")), std::string("abc/de/")) ;
   CPPUNIT_LOG_EQUAL(mkdirpath<>(std::string("abc/de")), std::string("abc/de/")) ;
   CPPUNIT_LOG_EQUAL(mkdirpath<>(strslice("abc/de")), std::string("abc/de/")) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQUAL(normpath<>(""), std::string()) ;
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
   CPPUNIT_LOG_EQUAL(abspath<>("."), std::string(cwd)) ;
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

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_IS_FALSE(path::posix::is_root_of("", "")) ;
   CPPUNIT_LOG_IS_FALSE(path::posix::is_root_of("", "/")) ;
   CPPUNIT_LOG_IS_TRUE(path::posix::is_root_of("/", "/")) ;
   CPPUNIT_LOG_IS_TRUE(path::posix::is_root_of("/", "/a")) ;
   CPPUNIT_LOG_IS_TRUE(path::posix::is_root_of("/", "/hello/world")) ;
   CPPUNIT_LOG_IS_TRUE(path::posix::is_root_of("/", "/hello/world/")) ;
   CPPUNIT_LOG_IS_FALSE(path::posix::is_root_of("/", "hello/world/")) ;

   CPPUNIT_LOG_IS_TRUE(path::posix::is_root_of("hello", "hello/world/")) ;
   CPPUNIT_LOG_IS_TRUE(path::posix::is_root_of("hello/", "hello/world/")) ;
   CPPUNIT_LOG_IS_TRUE(path::posix::is_root_of("hello/world/", "hello/world/")) ;
   CPPUNIT_LOG_IS_TRUE(path::posix::is_root_of("hello/world", "hello/world/")) ;

   CPPUNIT_LOG_IS_FALSE(path::posix::is_root_of("hello/worl", "hello/world/")) ;
   CPPUNIT_LOG_IS_FALSE(path::posix::is_root_of("hello/worl/", "hello/world/")) ;
   CPPUNIT_LOG_IS_FALSE(path::posix::is_root_of("hello/worlm/", "hello/world/")) ;
   CPPUNIT_LOG_IS_FALSE(path::posix::is_root_of("hello/worlm", "hello/world/")) ;
   CPPUNIT_LOG_IS_FALSE(path::posix::is_root_of("hell", "hello/world/")) ;

   CPPUNIT_LOG_IS_TRUE(path::posix::is_root_of(" ", " ")) ;
   CPPUNIT_LOG_IS_FALSE(path::posix::is_root_of(" ", "  ")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(path::posix::split(""), unipair<strslice>()) ;
   CPPUNIT_LOG_EQUAL(path::posix::split("."), unipair<strslice>(".", "")) ;
   CPPUNIT_LOG_EQUAL(path::posix::split(".."), unipair<strslice>("..", "")) ;
   CPPUNIT_LOG_EQUAL(path::posix::split("/"), unipair<strslice>("/", "")) ;
   CPPUNIT_LOG_EQUAL(path::posix::split("hello"), unipair<strslice>("", "hello")) ;
   CPPUNIT_LOG_EQUAL(path::posix::split("/hello"), unipair<strslice>("/", "hello")) ;
   CPPUNIT_LOG_EQUAL(path::posix::split("/hello/"), unipair<strslice>("/hello", "")) ;
   CPPUNIT_LOG_EQUAL(path::posix::split("/hello/world"), unipair<strslice>("/hello", "world")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(path::posix::basename("/hello/world.txt"), strslice("world.txt")) ;
   CPPUNIT_LOG_EQUAL(path::posix::basename("world.txt"), strslice("world.txt")) ;
   CPPUNIT_LOG_EQUAL(path::posix::basename("/hello/"), strslice()) ;
   CPPUNIT_LOG_EQUAL(path::posix::basename("/"), strslice()) ;
   CPPUNIT_LOG_EQUAL(path::posix::basename("."), strslice()) ;
   CPPUNIT_LOG_EQUAL(path::posix::basename(".."), strslice()) ;
   CPPUNIT_LOG_EQUAL(path::posix::basename("/hello"), strslice("hello")) ;

   CPPUNIT_LOG_EQUAL(path::posix::basename(strslice("world.txt/")(0, -1)), strslice("world.txt")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(path::posix::dirname("/hello/world.txt"), strslice("/hello")) ;
   CPPUNIT_LOG_EQUAL(path::posix::dirname("world.txt"), strslice()) ;
   CPPUNIT_LOG_EQUAL(path::posix::dirname("/hello/"), strslice("/hello")) ;
   CPPUNIT_LOG_EQUAL(path::posix::dirname("/"), strslice("/")) ;
   CPPUNIT_LOG_EQUAL(path::posix::dirname("."), strslice(".")) ;
   CPPUNIT_LOG_EQUAL(path::posix::dirname(".."), strslice("..")) ;
   CPPUNIT_LOG_EQUAL(path::posix::dirname("/hello"), strslice("/")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(path::splitext(""), unipair<strslice>()) ;
   CPPUNIT_LOG_EQUAL(path::splitext("."), unipair<strslice>(".", "")) ;
   CPPUNIT_LOG_EQUAL(path::splitext(".."), unipair<strslice>("..", "")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("../"), unipair<strslice>("../", "")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("../hello.world/"), unipair<strslice>("../hello.world/", "")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("abc.txt"), unipair<strslice>("abc", ".txt")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("abc.d"), unipair<strslice>("abc", ".d")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("abc.d.ef"), unipair<strslice>("abc.d", ".ef")) ;
   CPPUNIT_LOG_EQUAL(path::splitext(path::splitext("abc.d.ef").first), unipair<strslice>("abc", ".d")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("hello.world/abc.d"), unipair<strslice>("hello.world/abc", ".d")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("hello.world/abc.d.ef"), unipair<strslice>("hello.world/abc.d", ".ef")) ;
   CPPUNIT_LOG_EQUAL(path::splitext(".abc"), unipair<strslice>(".abc", "")) ;
   CPPUNIT_LOG_EQUAL(path::splitext("hello.world/.abc"), unipair<strslice>("hello.world/.abc", "")) ;
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

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(FilesystemTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "pcomn filesystem routines tests") ;
}
