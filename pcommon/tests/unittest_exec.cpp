/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_exec.cpp
 COPYRIGHT    :   Yakov Markovitch, 2011-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for exec/spawn/fork utilities and shell utilities.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   1 Jun 2011
*******************************************************************************/
#include <pcomn_exec.h>
#include <pcomn_shutil.h>
#include <pcomn_sys.h>
#include <pcomn_path.h>
#include <pcomn_unittest.h>
#include <pcomn_string.h>
#include <pcomn_except.h>

#include <iostream>

using namespace pcomn::path ;
using namespace pcomn::sys ;
using namespace pcomn::unit ;

/*******************************************************************************
                     class ExecTests
*******************************************************************************/
class ExecTests : public CppUnit::TestFixture {

      void Test_PopenCmd() ;
      void Test_ShellCmd() ;

      CPPUNIT_TEST_SUITE(ExecTests) ;

      CPPUNIT_TEST(Test_PopenCmd) ;
      CPPUNIT_TEST(Test_ShellCmd) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

void ExecTests::Test_PopenCmd()
{
}

void ExecTests::Test_ShellCmd()
{
   const std::string &echo_stdout = abspath<std::string>(CPPUNIT_AT_TESTDIR("echo_stdout.sh")) ;
   const std::string &echo_stderr = abspath<std::string>(CPPUNIT_AT_TESTDIR("echo_stderr.sh")) ;
   const std::string &echo_both = abspath<std::string>(CPPUNIT_AT_TESTDIR("echo_both.sh")) ;
   shellcmd_result result ;

   CPPUNIT_LOG_EQUAL(shellcmd(pcomn::strprintf("%s 0 'Hello, world!'", echo_stdout.c_str()), pcomn::DONT_RAISE_ERROR),
                     shellcmd_result(0, "Hello, world!\n")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EXPRESSION(result = shellcmd(pcomn::strprintf("%s 12 'Hello, world!'", echo_stdout.c_str()), pcomn::DONT_RAISE_ERROR)) ;
   CPPUNIT_LOG_ASSERT(WIFEXITED(result.first)) ;
   CPPUNIT_LOG_EQUAL(WEXITSTATUS(result.first), 12) ;
   CPPUNIT_LOG_EQUAL(result.second, std::string("Hello, world!\n")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EXPRESSION(result = shellcmd("/foobar 12 'Hello, world!'", pcomn::DONT_RAISE_ERROR)) ;
   CPPUNIT_LOG_ASSERT(WIFEXITED(result.first)) ;
   CPPUNIT_LOG_EXPRESSION(WEXITSTATUS(result.first)) ;
   CPPUNIT_LOG_EXPRESSION(result.second) ;

   CPPUNIT_LOG_EXCEPTION(shellcmd("/foobar 12 'Hello, world!'", pcomn::RAISE_ERROR), pcomn::shell_error) ;
   try {
      shellcmd("/foobar 12 'Hello, world!'", pcomn::RAISE_ERROR) ;
   }
   catch (const pcomn::shell_error &x) {
      CPPUNIT_LOG_EXPRESSION(x.what()) ;
      CPPUNIT_LOG_EQUAL(x.exit_code(), result.first) ;
      CPPUNIT_LOG_EQUAL(x.exit_status(), 127) ;
   }

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EXPRESSION(result = shellcmd(pcomn::strprintf("%s 13 'Bye, baby!'", echo_stderr.c_str()), pcomn::DONT_RAISE_ERROR)) ;
   CPPUNIT_LOG_ASSERT(WIFEXITED(result.first)) ;
   CPPUNIT_LOG_EQUAL(WEXITSTATUS(result.first), 13) ;
   CPPUNIT_LOG_EQUAL(result.second, std::string()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EXPRESSION(result = shellcmdf(pcomn::DONT_RAISE_ERROR, "%s 13 'Bye, baby!' 2>&1", echo_stderr.c_str())) ;
   CPPUNIT_LOG_ASSERT(WIFEXITED(result.first)) ;
   CPPUNIT_LOG_EQUAL(WEXITSTATUS(result.first), 13) ;
   CPPUNIT_LOG_EQUAL(result.second, std::string("Bye, baby!\n")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(shellcmd(pcomn::strprintf("%s 0 'Hello, world!' >&2", echo_stdout.c_str()), pcomn::DONT_RAISE_ERROR), shellcmd_result()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(shellcmd(pcomn::strprintf("%s 0 'Hello, world!' 'Bye, baby!'", echo_both.c_str()), pcomn::RAISE_ERROR),
                     shellcmd_result(0, "Hello, world!\n")) ;
   CPPUNIT_LOG_EQUAL(shellcmd(pcomn::strprintf("%s 0 'Hello, world!' 'Bye, baby!' 2>&1", echo_both.c_str()), pcomn::RAISE_ERROR),
                     shellcmd_result(0, "Hello, world!\nBye, baby!\n")) ;
   CPPUNIT_LOG_EQUAL(shellcmd(pcomn::strprintf("%s 0 'Hello, world!' 'Bye, baby!' 1>&2", echo_both.c_str()), pcomn::RAISE_ERROR),
                     shellcmd_result(0, "")) ;
   CPPUNIT_LOG_EQUAL(shellcmd(pcomn::strprintf("%s 0 'Hello, world!' 'Bye, baby!' 2>&1 1>/dev/null", echo_both.c_str()), pcomn::RAISE_ERROR),
                     shellcmd_result(0, "Bye, baby!\n")) ;

   // Test stdout data limit
   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(shellcmd(pcomn::strprintf("%s 0 'Hello, world!'", echo_stdout.c_str()), pcomn::RAISE_ERROR, 5),
                     shellcmd_result(0, "Hello")) ;
   CPPUNIT_LOG_EQUAL(shellcmdf(pcomn::RAISE_ERROR, 5, "%s 0 'Hello, world!'", echo_stdout.c_str()), shellcmd_result(0, "Hello")) ;

   CPPUNIT_LOG_EQUAL(shellcmd("echo Test a standard command", pcomn::RAISE_ERROR),
                     shellcmd_result(0, "Test a standard command\n")) ;
}

/*******************************************************************************
                     class ShutilTests
*******************************************************************************/
extern const char SHUTIL_FIXTURE[] = "shutil" ;
class ShutilTests : public pcomn::unit::TestFixture<SHUTIL_FIXTURE> {

      void Test_Shutil_Copy() ;
      void Test_Shutil_Rm() ;

      CPPUNIT_TEST_SUITE(ShutilTests) ;

      CPPUNIT_TEST(Test_Shutil_Copy) ;
      CPPUNIT_TEST(Test_Shutil_Rm) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

void ShutilTests::Test_Shutil_Copy()
{
   using pcomn::strprintf ;

   const std::string &datadir = abspath<std::string>(dataDir()) ;
   const std::string &fromdir = abspath<std::string>(datadir + "/from") ;
   const char *const ddir = datadir.c_str() ;
   const char *const fdir = fromdir.c_str() ;

   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(mkdir(fdir, 0755), "mkdir")) ;
   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(mkdir(strprintf("%s/source", fdir).c_str(), 0755), "mkdir")) ;
   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(mkdir(strprintf("%s/links", fdir).c_str(), 0755), "mkdir")) ;

   CPPUNIT_LOG_RUN(generate_seqn_file<4>(pcomn::strprintf("%s/source/10.txt", fdir), 1, 11)) ;
   CPPUNIT_LOG_RUN(generate_seqn_file<4>(pcomn::strprintf("%s/source/20.txt", fdir), 21, 41)) ;
   CPPUNIT_LOG_RUN(generate_seqn_file<4>(pcomn::strprintf("%s/source/15.txt", fdir), 5, 21)) ;
   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX
                   (symlink(abspath<std::string>(pcomn::strprintf("%s/source/15.txt", fdir)).c_str(),
                            pcomn::strprintf("%s/links/15.txt", fdir).c_str()),
                    "symlink")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(copyfile(strprintf("%s/source/10.txt", fdir), strprintf("%s/source/10.1.txt", fdir))) ;
   CPPUNIT_LOG_EXCEPTION(copyfile(strprintf("%s/source/10.txt", fdir), strprintf("%s/source/10.1.txt", fdir), CP_DST_REQUIRE_DIR),
                         pcomn::shell_error) ;
   CPPUNIT_LOG_EXCEPTION(copyfile(strprintf("%s/source/21.txt", fdir), strprintf("%s/source/21.1.txt", fdir)),
                         pcomn::shell_error) ;
   CPPUNIT_LOG_RUN(copyfile(pcomn::strprintf("%s/source/15.txt", fdir), strprintf("%s/source/10.txt", fdir))) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(checked_read_seqn_file<4>(pcomn::strprintf("%s/source/10.1.txt", fdir), 1, 11)) ;
   CPPUNIT_LOG_RUN(checked_read_seqn_file<4>(pcomn::strprintf("%s/source/15.txt", fdir), 5, 21)) ;
   // We've copied 15.txt to 10.txt above
   CPPUNIT_LOG_RUN(checked_read_seqn_file<4>(pcomn::strprintf("%s/source/10.txt", fdir), 5, 21)) ;
   CPPUNIT_LOG_RUN(checked_read_seqn_file<4>(pcomn::strprintf("%s/links/15.txt", fdir), 5, 21)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(mkdir(strprintf("%s/newdir", ddir).c_str(), 0755), "mkdir")) ;
   CPPUNIT_LOG_RUN(copyfile(strprintf("%s/links/15.txt", fdir), strprintf("%s/newdir", ddir), CP_DST_REQUIRE_DIR)) ;
   CPPUNIT_LOG_RUN(copyfile(strprintf("%s/links/15.txt", fdir), strprintf("%s/newdir/15.1.txt", ddir))) ;
   CPPUNIT_LOG_RUN(copyfile(strprintf("%s/links/15.txt", fdir), strprintf("%s/newdir/15.2.txt", ddir), CP_FOLLOW_SRC_LINKS)) ;
   CPPUNIT_LOG_RUN(copyfile(strprintf("%s/links/15.txt", fdir), strprintf("%s/newdir/15.3.txt", ddir), CP_FOLLOW_ALL_LINKS)) ;

   CPPUNIT_LOG_RUN(checked_read_seqn_file<4>(pcomn::strprintf("%s/newdir/15.txt", ddir), 5, 21)) ;
   CPPUNIT_LOG_RUN(checked_read_seqn_file<4>(pcomn::strprintf("%s/newdir/15.1.txt", ddir), 5, 21)) ;
   CPPUNIT_LOG_RUN(checked_read_seqn_file<4>(pcomn::strprintf("%s/newdir/15.2.txt", ddir), 5, 21)) ;
   CPPUNIT_LOG_RUN(checked_read_seqn_file<4>(pcomn::strprintf("%s/newdir/15.3.txt", ddir), 5, 21)) ;

   CPPUNIT_LOG_ASSERT(S_ISLNK(linkstat(pcomn::strprintf("%s/newdir/15.txt", ddir).c_str()).st_mode)) ;
   CPPUNIT_LOG_ASSERT(S_ISLNK(linkstat(pcomn::strprintf("%s/newdir/15.1.txt", ddir).c_str()).st_mode)) ;
   CPPUNIT_LOG_ASSERT(S_ISREG(linkstat(pcomn::strprintf("%s/newdir/15.2.txt", ddir).c_str()).st_mode)) ;
   CPPUNIT_LOG_ASSERT(S_ISREG(linkstat(pcomn::strprintf("%s/newdir/15.3.txt", ddir).c_str()).st_mode)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(symlink(fdir, strprintf("%s/link-from", ddir).c_str()), "symlink")) ;
   CPPUNIT_LOG_RUN(copyfile(strprintf("%s/link-from", ddir), strprintf("%s/link-to.1", ddir))) ;

   CPPUNIT_LOG_ASSERT(S_ISLNK(linkstat(pcomn::strprintf("%s/link-to.1", ddir).c_str()).st_mode)) ;
   CPPUNIT_LOG_ASSERT(S_ISDIR(filestat(pcomn::strprintf("%s/link-to.1", ddir).c_str()).st_mode)) ;

   CPPUNIT_LOG_EXCEPTION(copyfile(strprintf("%s/link-from", ddir), strprintf("%s/link-to.2", ddir), CP_FOLLOW_SRC_LINKS),
                         pcomn::shell_error) ;

   CPPUNIT_LOG_RUN(copyfile(strprintf("%s/link-from", ddir), strprintf("%s/link-to.2", ddir), CP_SRC_ALLOW_DIR)) ;
   CPPUNIT_LOG_ASSERT(S_ISLNK(linkstat(pcomn::strprintf("%s/link-to.2", ddir).c_str()).st_mode)) ;
   CPPUNIT_LOG_ASSERT(S_ISDIR(filestat(pcomn::strprintf("%s/link-to.2", ddir).c_str()).st_mode)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(copyfile(strprintf("%s/link-from", ddir), strprintf("%s/link-to.3", ddir),
                            CP_FOLLOW_SRC_LINKS|CP_SRC_ALLOW_DIR)) ;
   // Top-level link is dereferenced
   CPPUNIT_LOG_ASSERT(S_ISDIR(linkstat(pcomn::strprintf("%s/link-to.3", ddir).c_str()).st_mode)) ;
   // But all lower-level links are copied as links
   CPPUNIT_LOG_ASSERT(S_ISLNK(linkstat(pcomn::strprintf("%s/link-to.3/links/15.txt", ddir).c_str()).st_mode)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(copytree(strprintf("%s/link-from", ddir), strprintf("%s/link-to.4", ddir))) ;
   // Top-level link is dereferenced
   CPPUNIT_LOG_ASSERT(S_ISDIR(linkstat(pcomn::strprintf("%s/link-to.4", ddir).c_str()).st_mode)) ;
   // But all lower-level links are copied as links
   CPPUNIT_LOG_ASSERT(S_ISLNK(linkstat(pcomn::strprintf("%s/link-to.4/links/15.txt", ddir).c_str()).st_mode)) ;

   CPPUNIT_LOG_RUN(copytree(strprintf("%s/link-from", ddir), strprintf("%s/link-to.5", ddir), CP_FOLLOW_SRC_LINKS)) ;
   // Top-level link is dereferenced
   CPPUNIT_LOG_ASSERT(S_ISDIR(linkstat(pcomn::strprintf("%s/link-to.5", ddir).c_str()).st_mode)) ;
   // But all lower-level links are copied as links
   CPPUNIT_LOG_ASSERT(S_ISLNK(linkstat(pcomn::strprintf("%s/link-to.5/links/15.txt", ddir).c_str()).st_mode)) ;

   CPPUNIT_LOG_RUN(copytree(strprintf("%s/link-from", ddir), strprintf("%s/link-to.6", ddir), CP_FOLLOW_ALL_LINKS)) ;
   // All links are dereferenced
   CPPUNIT_LOG_ASSERT(S_ISDIR(linkstat(pcomn::strprintf("%s/link-to.6", ddir).c_str()).st_mode)) ;
   CPPUNIT_LOG_ASSERT(S_ISREG(linkstat(pcomn::strprintf("%s/link-to.6/links/15.txt", ddir).c_str()).st_mode)) ;

   CPPUNIT_LOG(std::endl) ;
   // Attempt to copy a file as a tree, must be shell_error
   CPPUNIT_LOG_EXCEPTION(copytree(strprintf("%s/link-from/links/15.txt", ddir), strprintf("%s/15.dir.txt", ddir)),
                         pcomn::shell_error) ;
}

void ShutilTests::Test_Shutil_Rm()
{
   using namespace pcomn ;

   const std::string &datadir = abspath<std::string>(dataDir()) ;
   const std::string &cwd = abspath<std::string>(".") ;
   const char *const ddir = datadir.c_str() ;

#define DEFNAME(filenum) const std::string &f_##filenum = datadir + ("/"#filenum".txt")
   DEFNAME(10) ;
   DEFNAME(20) ;
   DEFNAME(15) ;
#undef DEFNAME
   const std::string &f_q = datadir + "/?0.txt" ;
   const std::string &f_star = datadir + "/20*.txt" ;

   CPPUNIT_LOG_RUN(generate_seqn_file<4>(f_10, 1, 11)) ;
   CPPUNIT_LOG_RUN(generate_seqn_file<4>(f_20, 21, 41)) ;
   CPPUNIT_LOG_RUN(generate_seqn_file<4>(f_15, 5, 21)) ;
   CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(f_10)), sys::ACC_EXISTS) ;
   CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(f_15)), sys::ACC_EXISTS) ;
   CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(f_20)), sys::ACC_EXISTS) ;

   // File doesn't exist
   CPPUNIT_LOG_EXCEPTION(rm(f_q), environment_error) ;
   CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(f_10)), sys::ACC_EXISTS) ;
   CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(f_15)), sys::ACC_EXISTS) ;

   CPPUNIT_RUN(PCOMN_ENSURE_POSIX(chdir(ddir), "chdir")) ;
   try {
      // Relative path is not allowed
      CPPUNIT_LOG_EXCEPTION(rm("10.txt"), std::invalid_argument) ;
      CPPUNIT_LOG_IS_FALSE(rm("10.txt", RM_IGNORE_ERRORS)) ;
      CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(f_10)), sys::ACC_EXISTS) ;
      CPPUNIT_LOG_ASSERT(rm("10.txt", RM_ALLOW_RELPATH)) ;
      CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(f_10)), sys::ACC_NOEXIST) ;

      CPPUNIT_LOG_EXCEPTION(rm(f_10), environment_error) ;
      CPPUNIT_LOG_EXCEPTION(rm("10.txt", RM_ALLOW_RELPATH), environment_error) ;
      CPPUNIT_LOG_IS_FALSE(rm("10.txt", RM_IGNORE_ERRORS)) ;
      CPPUNIT_LOG_EXCEPTION(rm("10.txt", RM_ALLOW_RELPATH), environment_error) ;
      CPPUNIT_LOG_EXCEPTION(rm("10.txt", RM_IGNORE_NEXIST), std::invalid_argument) ;
      // If RM_IGNORE_NEXIST is set, returns "true"
      CPPUNIT_LOG_ASSERT(rm("10.txt", RM_ALLOW_RELPATH|RM_IGNORE_NEXIST)) ;

      CPPUNIT_LOG(std::endl) ;
      CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(f_15)), sys::ACC_EXISTS) ;
      CPPUNIT_LOG_ASSERT(rm(f_15)) ;
      CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(f_15)), sys::ACC_NOEXIST) ;

      CPPUNIT_LOG_EXCEPTION(rm(f_star), environment_error) ;
      CPPUNIT_LOG_RUN(generate_seqn_file<4>(f_star, 0, 0)) ;
      CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(f_star)), sys::ACC_EXISTS) ;
      CPPUNIT_LOG_ASSERT(rm(f_star)) ;
      CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(f_star)), sys::ACC_NOEXIST) ;
      CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(f_20)), sys::ACC_EXISTS) ;

      CPPUNIT_LOG(std::endl) ;
      // Read-only file
      CPPUNIT_LOG_RUN(generate_seqn_file<4>(f_15, 5, 21)) ;
      CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(chmod(str::cstr(f_15), 0400), "chmod")) ;
      CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(f_15), W_OK), sys::ACC_DENIED) ;
      CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(f_15), R_OK), sys::ACC_EXISTS) ;
      CPPUNIT_LOG_ASSERT(rm(f_15)) ;
      CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(f_15), R_OK), sys::ACC_NOEXIST) ;

      CPPUNIT_LOG(std::endl) ;
      // Directory
      CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(mkdir(strprintf("%s/newdir", ddir).c_str(), 0755), "mkdir")) ;
      CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(mkdir(strprintf("%s/newdir/dir01", ddir).c_str(), 0755), "mkdir")) ;
      CPPUNIT_LOG_RUN(generate_seqn_file<4>(strprintf("%s/newdir/15.txt", ddir), 5, 21)) ;
      CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(strprintf("%s/newdir/15.txt", ddir))), sys::ACC_EXISTS) ;
      CPPUNIT_LOG_EXCEPTION(rm(strprintf("%s/newdir", ddir)), environment_error) ;
      CPPUNIT_LOG_IS_FALSE(rm(strprintf("%s/newdir", ddir), RM_IGNORE_ERRORS)) ;
      CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(strprintf("%s/newdir", ddir)), X_OK), sys::ACC_EXISTS) ;
      CPPUNIT_LOG_ASSERT(rm(strprintf("%s/newdir", ddir), RM_RECURSIVE)) ;
      CPPUNIT_LOG_EQUAL(sys::fileaccess(str::cstr(strprintf("%s/newdir", ddir))), sys::ACC_NOEXIST) ;

      CPPUNIT_LOG(std::endl) ;
      // Remove from the root directory
      CPPUNIT_LOG_EXCEPTION(rm("/foo"), std::invalid_argument) ;
      CPPUNIT_LOG_EXCEPTION(rm("/foo/bar/.."), std::invalid_argument) ;

      CPPUNIT_LOG_EXCEPTION(rm("/foo/bar"), environment_error) ;
      CPPUNIT_LOG_EXCEPTION(rm("/foo", RM_ALLOW_ROOTDIR), environment_error) ;
   }
   catch (const std::exception &)
   {
      chdir(cwd.c_str()) ;
      throw ;
   }
   chdir(cwd.c_str()) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(ExecTests::suite()) ;
   runner.addTest(ShutilTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest_exec.diag.ini", "Exec/spawn/fork and shell utils tests") ;
}
