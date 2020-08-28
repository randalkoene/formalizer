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

#include <ctime>
#include <tuple>

#include "ReferenceTime.hpp"

namespace fz {

std::time_t time_stamp_time(std::string timestr, bool noerror = false);

std::string TimeStamp(const char *dateformat, std::time_t t);

/// Generate a Formalizer standardized date and time stamp (YYYYmmddHHMM).
inline std::string TimeStampYmdHM(std::time_t t) { return TimeStamp("%Y%m%d%H%M",t); }

/// Generate a Formalizer standardized date stamp (YYYYmmdd).
inline std::string DateStampYmd(std::time_t t) { return TimeStamp("%Y%m%d",t); }

/// Generate a Formalizer standardized file backup extension (YYYYmmdd.bak).
inline std::string BackupStampYmd() { return DateStampYmd(ActualTime())+".bak"; }

/// Generate a Formalizer standardized file backup precise extension (YYYYmmddHHMM.bak).
inline std::string BackupStampYmdHM() { return TimeStampYmdHM(ActualTime())+".bak"; }

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
