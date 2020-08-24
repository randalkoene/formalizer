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

std::string shellcmd2str(std::string cmd);

std::string to_precision_string(double d, unsigned int p = 2);

template <typename Out>
void split(const std::string &s, char delim, Out result);

std::vector<std::string> split(const std::string &s, char delim);

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

bool string_to_file(std::string path, std::string & s, std::ofstream::iostate * writestate = nullptr);

bool string_to_file_with_backup(std::string path, std::string & s, std::string backupext, bool & backedup, std::ofstream::iostate * writestate = nullptr);

bool file_to_string(std::string path, std::string & s, std::ifstream::iostate * readstate = nullptr);

inline std::string string_from_file(std::string path, std::ifstream::iostate * readstate = nullptr) {
    std::string s;
    file_to_string(path,s,readstate);
    return s;
}

} // namespace fz

#endif // __GENERAL_HPP
