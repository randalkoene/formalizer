// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the log2tl part of the
 * graph2dil tool.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __LOG2TL_HPP.
 */

#ifndef __LOG2TL_HPP
#include "version.hpp"
#define __LOG2TL_HPP (__VERSION_HPP)

#include <ostream>

#include "Logtypes.hpp"

using namespace fz;

struct Log2TL_conv_params {
    std::string TLdirectory; /// the directory path where TL files should be created
    std::string IndexPath;   /// the file path where a TL index should be created (empty to skip)
    std::ostream * o = nullptr;        /// an optional output stream for progress report (or nullptr)
    Log_chunk_ID_key_deque::size_type from_idx = 0; /// first section to convert (default 0)
    Log_chunk_ID_key_deque::size_type to_idx = 9999999;   /// last section to convert (default 9999999)
};

bool interactive_Log2TL_conversion(Graph & graph, Log & log, const Log2TL_conv_params & params);

#endif // __LOG2TL_HPP
