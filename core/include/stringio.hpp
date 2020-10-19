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

namespace fz {

bool string_to_file(std::string path, std::string & s, std::ofstream::iostate * writestate = nullptr);

bool string_to_file_with_backup(std::string path, std::string & s, std::string backupext, bool & backedup, std::ofstream::iostate * writestate = nullptr);

bool file_to_string(std::string path, std::string & s, std::ifstream::iostate * readstate = nullptr);

inline std::string string_from_file(std::string path, std::ifstream::iostate * readstate = nullptr) {
    std::string s;
    file_to_string(path,s,readstate);
    return s;
}

bool stream_to_string(std::istream &in, std::string & s);

} // namespace fz

#endif // __STRINGIO_HPP