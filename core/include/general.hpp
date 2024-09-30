// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares functions for general use with core and tool Formalizer C++ code.
 * 
 * The corresponding source file is at core/lib/general.cpp.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __GENERAL_HPP.
 */

#ifndef __GENERAL_HPP
#include "coreversion.hpp"
#define __GENERAL_HPP (__COREVERSION_HPP)

#include <vector>
#include <fstream>

namespace fz {

/// Invere of <cctype>:isdigit().
inline bool is_not_digit(int c) {
    return !isdigit(c);
}

std::string shellcmd2str(std::string cmd);

std::string to_precision_string(double d, unsigned int p = 2, char fillchar = ' ', unsigned int w = 0);

template <typename Out>
void split(const std::string &s, char delim, Out result);

std::vector<std::string> split(const std::string &s, char delim);

std::string join(const std::vector<std::string> & svec, const std::string delim = "");

/**
 * Trim whitespace from the front of a string.
 * 
 * @param s a string.
 * @param t a string containing whitespace characters.
 * @return reference to the trimmed string.
 */
inline std::string &ltrim(std::string &s, const char *t = " \t\n\r\f\v") {
    s.erase(0, s.find_first_not_of(t));
    return s;
}

/**
 * Trim whitespace from the end of a string.
 * 
 * @param s a string.
 * @param t a string containing whitespace characters.
 * @return reference to the trimmed string.
 */
inline std::string &rtrim(std::string &s, const char *t = " \t\n\r\f\v") {
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

/**
 * Trim whitespace on both ends of a string.
 * 
 * @param s a string.
 * @param t a string containing whitespace characters.
 * @return reference to the trimmed string.
 */
inline std::string &trim(std::string &s, const char *t = " \t\n\r\f\v") {
    return ltrim(rtrim(s, t), t);
}

bool find_in_vec_of_strings(const std::vector<std::string> & svec, const std::string & s);

std::string replace_char(const std::string & s, char c, char replacement = '_');

std::string get_enclosed_substring(const std::string & s, char open_enclosure, char close_enclosure, const std::string & alt_return);

/**
 * Safely copy from a string (not null-terminated) to a char buffer of limited
 * size. This can even include null-characters that were in the string. This
 * is safer than std::copy(), string::copy(), strcpy() or strncpy().
 * 
 * @param str A string.
 * @param buf Pointer to a limited-size character buffer.
 * @param bufsize Size of the character buffer (counting all chars, even if one is meant for '\0').
 */
void safecpy(std::string & str, char * buf, size_t bufsize);

/**
 * Find a character in a character array.
 * Searches up to maxlen or until the '\0' character is encountered.
 * 
 * @param c The character to find.
 * @param str The character array.
 * @param maxlen The size of the array.
 */
ssize_t find_in_char_array(char c, char* str, size_t maxlen);

} // namespace fz

#endif // __GENERAL_HPP
