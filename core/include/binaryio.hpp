// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares functions for binary I/O to and from files or streams.
 * 
 * The corresponding source file is at core/lib/binaryio.cpp.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __BINARYIO_HPP.
 */

#ifndef __BINARYIO_HPP
#include "coreversion.hpp"
#define __BINARYIO_HPP (__COREVERSION_HPP)

// std
#include <fstream>
#include <set>
#include <tuple>

// core (needed here for exit_status_code)
#include "error.hpp"

namespace fz {

struct uninitialized_char {
  char c;
  uninitialized_char() {}
};

class uninitialized_buffer: public std::vector<uninitialized_char> {
public:
    uninitialized_buffer() {}
    char * data() { return reinterpret_cast<char *>(data()); }
};

/**
 * Read the full contents of a binary file into a buffer.
 * 
 * @param path of the file.
 * @param buf reference to the receiving buffer.
 * @param readstate returns the iostate flags when provided (default: nullptr)
 * @return true if the read into buffer was successful.
 */
bool file_to_buffer(std::string path, std::vector<char> & buf, std::ifstream::iostate * readstate = nullptr);

enum path_test_result: int {
  path_does_not_exist = -1,
  path_is_file = 0,
  path_is_directory = 1,
  path_is_symlink = 2,
  path_is_unknown_type = 3,
};

path_test_result path_test(const std::string& path);

struct directory_content {
  std::string filename;
  path_test_result type = path_is_unknown_type;
  directory_content(const std::string& _filename, path_test_result _type): filename(_filename), type(_type) {}
};

std::tuple<std::set<std::string>, std::set<std::string>, std::set<std::string>> get_directory_content(const std::string& path);

} // namespace fz

#endif // __BINARYIO_HPP
