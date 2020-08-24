// Copyright 2020 Randal A. Koene
// License TBD

#include <tuple>

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

/**
 * Construct a year_month_day_t date from a UNIX epoch time.
 * 
 * @param t is UNIX epoch time.
 */
year_month_day_t::year_month_day_t(std::time_t t) {
    std::tm tm(*std::localtime(&t));
    (*this) = {static_cast<unsigned>(tm.tm_year)+1900, static_cast<unsigned>(tm.tm_mon)+1, static_cast<unsigned>(tm.tm_mday)};
}

/**
 * Test if a year, month and day form a valid date.
 * 
 * The rules are:
 *   1800 <= year <= 9999
 *   1 <= month <= 12
 *   1 <= day <= max_days_in(month)
 * 
 * @param ymd a tuple of year, month, day numbers.
 * @return true if the date is valid.
 */
bool valid_year_month_day(year_month_day_t ymd) {
    if ((ymd.year()<1800) || (ymd.year()>9999))
        return false;

    if ((ymd.month()<1) || (ymd.month()>12))
        return false;

    if (ymd.day()<1)
        return false;

    switch (ymd.month()) {
        case 4: case 6: case 9: case 11:
            if (ymd.day()>30)
                return false;
            break;

        case 2:
            if (is_leapyear(ymd.year())) {
                if (ymd.day()>29)
                    return false;
            } else {
                if (ymd.day()>28)
                    return false;
            }
            break;

        default:
            if (ymd.day()>31)
                return false;
    }

    return true;
}

/**
 * Express the difference between two dates in terms of the number of
 * Years, Months and Days between them.
 * 
 * @param ymd1 an earlier year, month, day tuple.
 * @param ymd2 a later year, month, day tuple.
 * @return tuple with the number of years, months and days between them.
 */
year_month_day_t years_months_days(year_month_day_t ymd1, year_month_day_t ymd2) {

    if ((!valid_year_month_day(ymd1)) || (!valid_year_month_day(ymd2)))
        return year_month_day_t(0,0,0);

    auto [y1, m1, d1] = static_cast<ymd_tuple>(ymd1);
    auto [y2, m2, d2] = static_cast<ymd_tuple>(ymd2);

    if (d2 < d1) {

        if (ymd2.is_Mar()) { // borrow days from February
            if (is_leapyear(y2)) {
                d2 += 29;
            } else {
                d2 += 28;
            }
        } else if (ymd2.is_May() || ymd2.is_Jul() || ymd2.is_Oct() || ymd2.is_Dec()) { // borrow days from Apr, Jun, Sep, Nov
            d2 += 30;
        } else { // borrow days from Jan, Mar, May, Jul, Aug, Oct, Dec
            d2 += 31;
        }

        --m2;
    }

    if (m2 < m1) {
        m2 += 12;
        --y2;
    }

    return std::make_tuple(y2-y1,m2-m1,d2-d1);
}

year_month_day_t years_months_days(std::time_t t1, std::time_t t2) {
    return years_months_days(year_month_day_t(t1),year_month_day_t(t2));
}

} // namespace fz
