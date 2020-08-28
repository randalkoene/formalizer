// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares Formalizer ReferenceTime objects that are used to
 * ensure operation within a well-defined time context, when a time state can
 * be either emulated or actual.
 * 
 * The corresponding source file is at core/lib/ReferenceTime.cpp.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __REFERENCETIME_HPP.
 */

#ifndef __REFERENCETIME_HPP
#include "coreversion.hpp"
#define __REFERENCETIME_HPP (__COREVERSION_HPP)

namespace fz {

/**
 * Explicitly provide the actual system time.
 * 
 * This function is called where an emulated time status cannot be applied,
 * such as when generating time stamps for backup files.
 */
inline std::time_t ActualTime() { return std::time(NULL); }

enum ReferenceTime_t : std::time_t {
    RTt_invalid_time_stamp = -34403, // a numerical rendering of 'ERROR'
    RTt_unspecified = -1,
    RTt_unix_epoch_start = 0
};

class ReferenceTime {
protected:
    std::time_t emulated_time; /// when positive then time is being emulated
public:
    ReferenceTime(): emulated_time(RTt_unspecified) {}
    ReferenceTime(std::time_t _emtime): emulated_time(_emtime) {}

    std::time_t Time();

    void switch_to_actual_time() { emulated_time = RTt_unspecified; }
    void change_reference_time(std::time_t _emtime) { emulated_time = _emtime; }
};

} // namespace fz

#endif // __REFERENCETIME_HPP
