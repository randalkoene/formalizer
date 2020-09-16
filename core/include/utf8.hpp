// Copyright 2020 Randal A. Koene
// License TBD

/** @file utf8.hpp
 * This header serves as a core utility wrapper for the open source cpputf library.
 * 
 * This header and the file utf8.cpp ensure that the library functions can easily be
 * used in other core components, such as Graphtypes and Logtypes, without later
 * instantiation collisions (as when the utfcpp/source/utf8.h header is compiled
 * directly into each).
 * 
 * The cpputf library a fast, efficient way to ensure that strings are UTF8 safe.
 * Unfortunately, the maker of the library did not properly test that all
 * non-member functions are inlined, and variables static. The tests and examples
 * included with the library also contained none that showed a multi-file project
 * where the header was used in more than one file. Consequently, it doesn't work
 * well when fully exposed and included in more than one file.
 * 
 * This wrapper will have to delicately make select functions available in a way
 * that preserves the ability to include in many files and still compile.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __UTF8_HPP.
 */

#ifndef __UTF8_HPP
#include "coreversion.hpp"
#define __UTF8_HPP (__COREVERSION_HPP)

namespace fz {

/**
 * Filter a text string to ensure that it contains valid UTF8 encoded content.
 * 
 * Attempts to assign content to the text parameter as provided, and
 * replaces any invalid UTF8 code points with a replacement
 * character. The default replacement character is the UTF8
 * 'REPLACEMENT CHARACTER' 0xfffd.
 * 
 * If invalid UTF8 code points were encountered and replaced then that is
 * optionally reported through ADDWARNING.
 * 
 * Note 1: ASCII text is valid UTF8 text and will be assigned
 *         unaltered.
 * Note 2: This does not test for valid HTML5 at this time.
 * 
 * @param utf8str A string that should contain UTF8 encoded text.
 * @param warn If true then warn about invalid UTF8 code replacements.
 * @return A guaranteed utf8 compliant string.
 */
std::string utf8_safe(const std::string & utf8str, bool warn = true);

} // namespace fz

#endif // __UTF8_HPP
