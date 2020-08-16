// Copyright 2020 Randal A. Koene
// License TBD

#include "error.hpp"
#include "TimeStamp.hpp"

namespace fz {

/**
 * Convert a Formalizer time stamp string into local Unix time.
 * 
 * Formalizer time stamp strings must have this format:
 * 1) [^0-9]YYYYmmddHHMM (e.g. "202008140613", "__202109150714"), and,
 * 2) the year must be >= 1900, or,
 * 3) a negative integer code (e.g. "-2").
 * 
 * Negative integer codes are returned the equivalent negative integer so that
 * special codes can be detected.
 * 
 * Non-digit characters preceding the digits of a proper time stamp are
 * ignored and discarded. (This does not apply to negative integer codes.)
 * 
 * When the 'noerror' flag is set then no error message will be added
 * if an invalid time stamp is encountered. This can be useful when this
 * function is explicitly used to check for empty/unfinished/etc time
 * stamps.
 * 
 * The 1900 test is a useful sanity check, because mangled time stamp
 * strings can often lead to unlikely dates preceding the computing era.
 * 
 * This function is derived from dil2al/utilities.cc:time_stamp_time().
 * Unlike that function, this one does not terminate the program when an
 * invalid time stamp is encountered and uses no configuration flag to
 * elicit such behavior. Instead, the special code INVALID_TIME_STAMP is
 * returned in that case.
 * 
 * @param timestr a string containing the Formalizer time stamp.
 * @param noerror suppresses ADDERROR() for invalid time stamps.
 * @return local Unix time in seconds, negative integer code, or
 *         INVALID_TIME_STAMP.
 */
#define INVALID_TIME_STAMP -34403 // a numerical rendering of 'ERROR'
time_t time_stamp_time(std::string timestr, bool noerror) {
    struct tm ts;
    int tl = timestr.size();

    // Detect and return special codes
    if (tl > 0)
        if (timestr.front() == '-')
            return (time_t)atol(timestr.c_str());

    // Confirm [^0-9]*YYYYmmddHHMM format
    if (tl < 12) {
        if (!noerror) {
            ADDERROR(__func__, "invalid [^0-9]YYYYmmddHHMM time stamp (" + timestr + ") [failed test#1]");
        }
        return (time_t)INVALID_TIME_STAMP;
    }
    const char *tschars = timestr.c_str();
    for (int i = tl - 12; i < tl; i++) {
        if ((tschars[i] < '0') || (tschars[i] > '9')) {
            if (!noerror) {
                ADDERROR(__func__, "invalid [^0-9]YYYYmmddHHMM time stamp (" + timestr + ") [failed test#2]");
            }
            return (time_t)INVALID_TIME_STAMP;
        }
    }

    // Since we've already ensured that they are digits, we might as well compute
    // directly instead of slowing ourselves down with atoi()
    ts.tm_sec = 0;
    ts.tm_min = (tschars[tl - 2] - '0') * 10 + (tschars[tl - 1] - '0');
    ts.tm_hour = (tschars[tl - 4] - '0') * 10 + (tschars[tl - 3] - '0');
    ts.tm_mday = (tschars[tl - 6] - '0') * 10 + (tschars[tl - 5] - '0');
    ts.tm_mon = (tschars[tl - 8] - '0') * 10 + (tschars[tl - 7] - '0') - 1;
    ts.tm_year = ((tschars[tl - 12] - '0') * 1000 + (tschars[tl - 11] - '0') * 100 + (tschars[tl - 10] - '0') * 10 + (tschars[tl - 9] - '0'));
    if (ts.tm_year < 1900) {
        if (!noerror) {
            ADDERROR(__func__, "time stamp before the year 1900 (" + timestr + ") [failed test#3]");
        }
        return (time_t)INVALID_TIME_STAMP;
    }
    ts.tm_year -= 1900;
    ts.tm_wday = 0;
    ts.tm_yday = 0;
    ts.tm_isdst = -1; // computed by mktime since indicated "unknown" here
    return mktime(&ts);
}

/**
 * Generate time stamp.
 * 
 * @param dateformat a date and time format specifier, e.g. "%Y%m%d%H%M".
 * @param t a date and time expressed in seconds.
 * @return The time stamp string. Returns an empty string if t<0.
 */
std::string TimeStamp(const char * dateformat, time_t t) {
    if (t<0) return "";

    char dstr[80];
    strftime(dstr,80,dateformat,localtime(&t));
    return dstr;
}

} // namespace fz
