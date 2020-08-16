// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares Formalizer TimeStamp format and operations.
 * 
 * The corresponding source file is at core/lib/TimeStamp.cpp.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __TIMESTAMP_HPP.
 */

#ifndef __TIMESTAMP_HPP
#include "coreversion.hpp"
#define __TIMESTAMP_HPP (__COREVERSION_HPP)

namespace fz {

#define INVALID_TIME_STAMP -34403 // a numerical rendering of 'ERROR'

time_t time_stamp_time(std::string timestr, bool noerror = false);

std::string TimeStamp(const char *dateformat, time_t t);

/// Generate a Formalizer standardized date and time stamp (YYYYmmddHHMM).
inline std::string TimeStampYmdHM(time_t t) { return TimeStamp("%Y%m%d%H%M",t); }

} // namespace fz

#endif // __TIMESTAMP_HPP
