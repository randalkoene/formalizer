// Copyright 2020 Randal A. Koene
// License TBD

/** @file TimeStamp.hpp
 * This header file declares Formalizer TimeStamp format and operations.
 * 
 * The corresponding source file is at core/lib/TimeStamp.cpp.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __TIMESTAMP_HPP.
 */

#ifndef __TIMESTAMP_HPP
#include "coreversion.hpp"
#define __TIMESTAMP_HPP (__COREVERSION_HPP)

#include <ctime>
#include <tuple>

#include "ReferenceTime.hpp"

namespace fz {

enum day_of_week: unsigned int {
    dow_Sunday = 0,
    dow_Monday = 1,
    dow_Tuesday = 2,
    dow_Wednesday = 3,
    dow_Thursday = 4,
    dow_Friday = 5,
    dow_Saturday = 6,
    _num_dow
};

struct time_of_day_t {
    unsigned int hour;
    unsigned int minute;
    time_of_day_t(unsigned int hr = 0, unsigned int min = 0): hour(hr), minute(min) {}
    unsigned int num_minutes() const { return 60*hour + minute; }
    std::string str() const;
};

extern const std::string weekday_str[day_of_week::_num_dow];

/**
 * A Formalizer standardized version of the localtime() function that always
 * returns a usable value, but which may log errors or warnings as needed.
 * 
 * @param t_ptr Pointer to a (time_t) variable containing the UNIX epoch time to convert.
 * @param errorcode_ptr Optional pointer to a buffer for an errno error code.
 * @return Pointer to a local calendar time structure.
 */
const std::tm * safe_localtime(const std::time_t * t_ptr, int * errorcode_ptr = nullptr);

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
 * elicit such behavior. Instead, the special code RTt_invalid_time_stamp is
 * returned in that case.
 * 
 * @param timestr a string containing the Formalizer time stamp.
 * @param noerror suppresses ADDERROR() for invalid time stamps.
 * @return local Unix time in seconds, negative integer code, or
 *         RTt_invalid_time_stamp.
 */
std::time_t time_stamp_time(std::string timestr, bool noerror = false);

/**
 * UNIX epoch time equivalent of a Year-Month-Day date-stamp such as
 * 20200914 or 202009140100.
 * 
 * This function uses `time_stamp_time()` for time stamp format validation
 * and conversion, but it allows pure date stamps consisting of just 8
 * year, month and day digits, as well as 12 digit time stamps. In the
 * 12 digit case, the last 4 digits can be optionally ignored (set to 0000)
 * during conversion to UNIX epoch time.
 * 
 * @param timestr a string containing the Formalizer time stamp or date stamp.
 * @param noerror suppresses ADDERROR() for invalid time stamps.
 * @param ignoreHM optionally set any hour and minute digits to 0 during conversion.
 * @return local Unix time in seconds, negative integer code, or
 *         RTt_invalid_time_stamp.
 */
std::time_t ymd_stamp_time(std::string timestr, bool noerror = false, bool ignoreHM = false);

std::string TimeStamp(const char *dateformat, std::time_t t);

/// Generate a Formalizer standardized date and time stamp (YYYYmmddHHMM).
inline std::string TimeStampYmdHM(std::time_t t) { return TimeStamp("%Y%m%d%H%M",t); }

/// Generate a Formalizer standardized date stamp (YYYYmmdd).
inline std::string DateStampYmd(std::time_t t) { return TimeStamp("%Y%m%d",t); }

inline std::time_t day_start_time(std::time_t t) { return ymd_stamp_time(DateStampYmd(t)); }
inline std::time_t day_end_time(std::time_t t) { return ymd_stamp_time(DateStampYmd(t)+"2359"); }
inline std::time_t today_start_time() { return day_start_time(ActualTime()); }
inline std::time_t today_end_time() { return day_end_time(ActualTime()); }

/// Generate a Formalizer standardized file backup extension (YYYYmmdd.bak).
inline std::string BackupStampYmd() { return DateStampYmd(ActualTime())+".bak"; }

/// Generate a Formalizer standardized file backup precise extension (YYYYmmddHHMM.bak).
inline std::string BackupStampYmdHM() { return TimeStampYmdHM(ActualTime())+".bak"; }

std::string WeekDay(time_t t);

time_t time_add_day(time_t t, int days = 1);
time_t time_add_month(time_t t, int months = 1);
day_of_week time_day_of_week(time_t t);
unsigned int time_hour(time_t t);
unsigned int time_minute(time_t t);
time_of_day_t time_of_day(time_t t);
time_t seconds_since_day_start(time_t t);
time_t seconds_remaining_in_day(time_t t);

unsigned long date_as_ulong(time_t t);
int time_month_length(time_t t);
time_t time_add_month_EOMoffset(time_t t);

typedef std::tuple<unsigned int, unsigned int, unsigned int> ymd_tuple;
struct year_month_day_t: public ymd_tuple {
/*
    unsigned int year() {
        unsigned int _year;
        std::tie(_year, std::ignore, std::ignore) = (*this);
        return _year;
    }
*/
    year_month_day_t(ymd_tuple ymd): ymd_tuple(ymd) {}
    year_month_day_t(unsigned int y, unsigned int m, unsigned int d): ymd_tuple( {y, m, d} ) {}
    year_month_day_t(std::tm & tm): ymd_tuple( {tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday} ) {}
    year_month_day_t(std::time_t t);
    unsigned int year() { return std::get<0>(*this); }
    unsigned int month() { return std::get<1>(*this); }
    unsigned int day() { return std::get<2>(*this); }
    void set_year(unsigned int y) { std::get<0>(*this) = y; }
    void set_month(unsigned int m) { std::get<1>(*this) = m; }
    void set_day(unsigned int d) { std::get<2>(*this) = d; }

    // helper functions
    bool is_Jan() { return month() == 1; }
    bool is_Feb() { return month() == 2; }
    bool is_Mar() { return month() == 3; }
    bool is_Apr() { return month() == 4; }
    bool is_May() { return month() == 5; }
    bool is_Jun() { return month() == 6; }
    bool is_Jul() { return month() == 7; }
    bool is_Aug() { return month() == 8; }
    bool is_Sep() { return month() == 9; }
    bool is_Oct() { return month() == 10; }
    bool is_Nov() { return month() == 11; }
    bool is_Dec() { return month() == 12; }

};

/**
 * A simple leap year test.
 * 
 * @param year the year.
 * @return true if it is a leap year.
 */
inline bool is_leapyear(unsigned int year) {
    return (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0));
}

bool valid_year_month_day(year_month_day_t ymd);

year_month_day_t years_months_days(year_month_day_t ymd1, year_month_day_t ymd2);

year_month_day_t years_months_days(std::time_t t1, std::time_t t2);

} // namespace fz

#endif // __TIMESTAMP_HPP
