// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares functions for string I/O to and from files or streams.
 * 
 * The corresponding source file is at core/lib/stringio.cpp.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __STRINGIO_HPP.
 */

#ifndef __STRINGIO_HPP
#include "coreversion.hpp"
#define __STRINGIO_HPP (__COREVERSION_HPP)

// std
#include <fstream>

// core (needed here for exit_status_code)
#include "error.hpp"

namespace fz {

bool string_to_file(const std::string path, const std::string & s, std::ofstream::iostate * writestate = nullptr);

bool string_to_file_with_backup(std::string path, std::string & s, std::string backupext, bool & backedup, std::ofstream::iostate * writestate = nullptr);

bool file_to_string(const std::string & path, std::string & s, std::ifstream::iostate * readstate = nullptr);

inline std::string string_from_file(std::string path, std::ifstream::iostate * readstate = nullptr) {
    std::string s;
    file_to_string(path,s,readstate);
    return s;
}

bool stream_to_string(std::istream &in, std::string & s);

/**
 * Try to fill a string with text content if it was empty.
 * 
 * E.g. see how this is used in fzaddnode.cpp.
 * 
 * @param utf8_text A string reference to receive the content (if not already present).
 * @param content_path A possible path to a file containing text content.
 * @param errmsg_target A short string to be used in error messages.
 * @return A pair with a proposed exit code (exit_ok if no problem) and possible error message (empty if no problem).
 */
std::pair<exit_status_code, std::string> get_content(std::string & utf8_text, const std::string content_path, const std::string errmsg_target);

} // namespace fz

#endif // __STRINGIO_HPP
