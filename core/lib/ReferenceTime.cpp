// Copyright 2020 Randal A. Koene
// License TBD

#include <ctime>

#include "error.hpp"
#include "ReferenceTime.hpp"

namespace fz {

/**
 * Every program that uses core Formalizer functions that depend on time stamping
 * includes ReferenceTime.hpp (it is automatically included through TimeStamp.hpp).
 * 
 * The `maintime` object that is defined here provides time status for the main
 * scope of the program. Additioan ReferenceTime objects can be created as needed.
 */

ReferenceTime maintime;

/**
 * Report the current time according to the ReferenceTime frame being used.
 * 
 * Note: This function returns actual time event for emulated_time == 0. It
 *       is a precaution, in case unspecified time values are sometimes left
 *       zero-valued. (This is subject to change and may be set to
 *       emulated_time > RTt_unspecified.)
 * 
 * @return current time, either emulated as specified or actual.
 */
std::time_t ReferenceTime::Time() {
    if (emulated_time > 0)
        return emulated_time;

    return ActualTime();
}

} // namespace fz
