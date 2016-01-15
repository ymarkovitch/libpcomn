/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_CFGPARSER_H
#define __PCOMN_CFGPARSER_H
/*******************************************************************************
 FILE         :   pcomn_cfgparser.h
 COPYRIGHT    :   Yakov Markovitch, 1998-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Functions for .INI/.config file processing
                  Process files like Windows .INI

 CREATION DATE:   29 May 1998
*******************************************************************************/
/******************************************************************************/
/** @file pcomn_cfgparser.h
 Functions for handling configuration files in Windows INI (AKA UNIX config) format.
 Windows .ini and unix configuration files have the following format:

    @verbatim
    FOOKEY = Hello, world!
    BARKEY = 10

    [QuuxSection]
    FOOKEY = This is _not_ the same FOOKEY as above!

    [BarSection]
    HELLO = world
    HASTA = la Vista (tm)
    # And this is a comment

    [QuuxSection]
    What=is going on?
    This=is a continuation of QuuxSection.

    [QUUXSECTION]
    This=is _NOT_ a continuation of QuuxSection, section names are case-sensitive.
    @endverbatim

 Please note that both whitespaces around the @c '=' and empty lines between sections
 and keys are completely optional.

 Sections are case-sensitive, whereas keys are not.

 All functions are made after the Windows 3.1/95 GetPrivateProfileXXX and
 WritePrivateProfileXXX APIs. The reason for that is historical, since they were
 initially written to provide platform-independent ini-file handling between
 Windows and AS/400 (sic!).
 Actually, the only differences between corresponding Windows APIs and these
 functions are:
   - @b filename parameter goes first, not last, as it is custom for all modern APIs;
   - @b filename is normalized, both @c '/' and @c '\\' are treated equally as
     path separators, regardless of a platform;
   - a filename without a directory part (e.g. "foo.ini") refers to a file in a
     currrent working directory, as a contrast to "native" Windows 3.1 APIs that
     would place such file in a Windows directory. I.e. these function do @em not
     require to specify, for instance, @c "./foo.ini" to handle foo.ini in CWD,
     just @c "foo.ini" is enough.
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_def.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// cfgfile_get_value()
/// Retrieve a string associated with a key in the specified section of a configuration
/// file, or retrieve all section name, or retrieve all key/value pairs in specific section.
///
/// @param filename Configuration file name; NULL or empty string are OK.
/// @param section  Section name in a configuration file, case-sensitive; NULL means "get
/// all section names".
/// @param key      Key name in the @a section, case-sensitive; NULL means "get all
/// key/value pairs in the @a section"; @c "" is valid but meaningless (such key never exists).
/// @param buf      The destination buffer.
/// @param bufsize  Destination buffer size, counting the terminating null(s).
/// @param defval   A default value; copied to @a buf if either @a section or @a key is
/// not found.
/// @note This function checks all its parameter and doesn't crash if either or all
/// of its pointer arguments is/are NULL.
///
/// If neither @a section nor @a key are NULL, the function copies the value of a key,
/// terminated with single '\0', into @a buf. If either @a section does not exist or
/// @a key is not found in the @a section, the function places '\0' in the @a buf.
///
/// If @a section is NULL, the function copies all section names separated with '\0' into
/// the @a buf, delimiting the whole sequence with additional '\0' (so that the whole
/// sequence is terminated with two null characters). The @a key is ignored.
///
/// If @a section is not NULL and @a key is NULL, the function copies all key/value pair
/// for @a section separated with '\0' into the @a buf, delimiting the whole sequence
/// with additional '\0'. A key separated from a value with '=' (@em without any
/// whitespaces before/after '='), so the result may look like "key1=val1/0key2=val2/0/0"
///
/// @return If the buffer is big enough, the number of characters copied to the buffer,
/// not including the terminating null character; if the buffer is too small, then
/// @a bufsize; this gives a reliable method to detect too short buffer, since the
/// @em only case when result is equal to bufsize is when @a bufsize is too small.
///
/// @note The function @em guarantees that, except for the case @c bufsize==0, @a buf
/// will be terminated with null character upon return, no matter with success or
/// failure.
///
/// @note There is a convenient way to traverse returned buffers with multiple values:
/// cstrseq_iterator and cstrseq_keyval_iterator.
///
_PCOMNEXP size_t cfgfile_get_value(const char *filename, const char *section, const char *key,
                                   char *buf, size_t bufsize, const char *defval) ;

// cfgfile_get_intval()
/// Retrieve an integer associated with a key in the specified section of a configuration file.
_PCOMNEXP int cfgfile_get_intval(const char *filename, const char *section, const char *key,
                                 int defval) ;

// cfgfile_write_value()
/// Set value of a key in a particular section of a configuration file, or delete a key, or
/// delete an entire section.
/// @param filename the name of a configuration file.
/// @param section  the case-sensitive name of the section to write the string to, or to delete;
/// if it doesn't exist, it is created.
/// @param key      the @em case-insensitive name of the key in the section; if it is NULL, the
/// whole section is deleted.
/// @param value    the value of the key; if NULL, the key is deleted.
///
/// @return 1 on success, 0 on failure (@b beware, it @em is bool).
///
/// The @a section can be an empty string (@c ""), in which case the key/value pair goes
/// before all sections. In fact, many UNIX configuration files are "sectionless".
/// @note The function preserves comments and empty strings in a configuration file.
_PCOMNEXP int cfgfile_write_value(const char *filename, const char *section, const char *key,
                                  const char *value) ;

#ifdef __cplusplus
}
#endif

#define _PCOMN_CFGPARSER_MAXLINEBUF ((size_t)32768)
#define _cfgfile_get_section(filename, section, buf, bufsize) \
   (cfgfile_get_value((filename), (section), NULL, (buf), (bufsize), NULL))
#define _cfgfile_get_sectnames(filename, buf, bufsize) \
   (cfgfile_get_value((filename), NULL, NULL, (buf), (bufsize), NULL))
#define _cfgfile_del_value(filename, section, key) \
   ((key) && cfgfile_write_value((filename), (section), (key), NULL))
#define _cfgfile_del_section(filename, section) \
   (cfgfile_write_value((filename), (section), NULL, NULL))

#ifdef __cplusplus
/*******************************************************************************
 C++ flavour
*******************************************************************************/

/// Maximum length of a buffer for cfgfile functions returning @em single value.
/// If a larger buffer passed, only first PCOMN_CFGPARSER_MAXLINEBUF bytes are used.
const size_t PCOMN_CFGPARSER_MAXLINEBUF = _PCOMN_CFGPARSER_MAXLINEBUF ;

/// Get all key/value pairs from the specified section of a configuration file
///
/// @param filename Configuration file name; if NULL or empty string, the function
/// does nothing and returns 0
/// @param section  Section name in a configuration file, case-sensitive; if NULL, the
/// function does nothing and returns 0; if an empty string (""), the function will get
/// all keys that go before the fist section name.
/// @param buf      The destination buffer.
/// @param bufsize  Destination buffer size, counting the terminating nulls.
///
/// The function copies all key/value pairs for @a section, separated with '\0', into
/// the @a buf, delimiting the whole sequence  with additional '\0'. A key separated from
/// a value with '=' (@em without any whitespaces before/after '='), so the result may
/// look like "key1=val1/0key2=val2/0/0"
///
/// @return If the buffer is big enough, the number of characters copied to the buffer,
/// not including the terminating null character; if the buffer is too small, then
/// @a bufsize; this gives a reliable method to detect too short buffer, since the
/// @em only case when result is equal to bufsize is when @a bufsize is too small.
///
/// @note If some key is encountered in the same section more then once, only its first
/// appearance is placed in the result buffer.
///
inline size_t cfgfile_get_section(const char *filename, const char *section, char *buf, size_t bufsize)
{
   return _cfgfile_get_section(filename, section, buf, bufsize) ;
}

template<size_t bufsize>
inline size_t cfgfile_get_section(const char *filename, const char *section, char (&buf)[bufsize])
{
   return cfgfile_get_section(filename, section, buf, bufsize) ;
}

/// Get all key/value pairs from the specified section of a configuration file
///
/// @return A buffer, allocated with malloc(), with key/value pairs for @a section,
/// separated with '\0' and terminated with additional '\0'; the caller is responsible
/// for buffer deallocation with free()
char *cfgfile_get_section(const char *filename, const char *section) ;

/// Get the names of all sections in the given configuration file.
///
/// @param filename  The name of a configuration file.
/// @param buffer    A pointer to a buffer that receives the section names; section names
/// are separated with '\0', so all strings in the buffer are null-terminated; the last
/// string is followed by the @em second '\0', so the whole sequence is terminated by
/// double null characters.
/// @param bufsize   The size of @a buffer; must count for the final additional null.
///
/// @return The count of characters copied to the @a buffer, @em not including the
/// terminating '\0'; if @a bufsize is too small, the return value is equal to
/// @a bufsize.
inline size_t cfgfile_get_sectnames(const char *filename, char *buffer, size_t bufsize)
{
   return _cfgfile_get_sectnames(filename, buffer, bufsize) ;
}

template<size_t bufsize>
inline size_t cfgfile_get_sectnames(const char *filename, char (&buf)[bufsize])
{
   return cfgfile_get_sectnames(filename, buf, bufsize) ;
}

/// Get the names of all sections in the given configuration file.
///
/// @return A buffer with all section names, allocated with malloc(); the caller is
/// responsible for buffer deallocation with free()
char *cfgfile_get_sectnames(const char *filename) ;

template<size_t bufsize>
inline size_t cfgfile_get_value(const char *filename, const char *section, const char *key,
                                char (&buf)[bufsize], const char *defval = "")
{
   return cfgfile_get_value(filename, section, key, buf, bufsize, defval) ;
}

inline bool cfgfile_del_value(const char *filename, const char *section, const char *key)
{
   return _cfgfile_del_value(filename, section, key) ;
}

inline bool cfgfile_del_section(const char *filename, const char *section)
{
   return _cfgfile_del_section(filename, section) ;
}

#undef _PCOMN_CFGPARSER_MAXLINEBUF
#undef _cfgfile_get_section
#undef _cfgfile_get_sectnames
#undef _cfgfile_del_value
#undef _cfgfile_del_section

#else
/*******************************************************************************
 C flavour
*******************************************************************************/
#define PCOMN_CFGPARSER_MAXLINEBUF _PCOMN_CFGPARSER_MAXLINEBUF
#define cfgfile_get_section(filename, section, buf, bufsize) \
   (_cfgfile_get_section((filename), (section), (buf), (bufsize)))
#define _cfgfile_get_sectnames(filename, buf, bufsize) \
   (_cfgfile_get_sectnames((filename), (buf), (bufsize)))
#define cfgfile_del_value(filename, section, key) \
   (_cfgfile_del_value((filename), (section), (key)))
#define cfgfile_del_section(filename, section) \
   (_cfgfile_del_section((filename), (section)))

#endif
#endif /* __PCOMN_CFGPARSER_H */
