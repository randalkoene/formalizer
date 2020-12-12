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

} // namespace fz

#endif // __BINARYIO_HPP
