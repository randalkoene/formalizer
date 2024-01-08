// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <cerrno>
#include <tuple>

// core
#include "error.hpp"
#include "TimeStamp.hpp"

namespace fz {

const std::string weekday_str[day_of_week::_num_dow] = {
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday"
};

// defining a special 4-digit-year safe maximum local time 9999-12-31 23:59:59
constexpr const std::tm safe_max_localtime = {
    .tm_sec = 59,
    .tm_min = 59,
    .tm_hour = 23,
    .tm_mday = 31,
    .tm_mon = 11,
    .tm_year = 9999 - 1900,
    .tm_wday = dow_Friday,
    .tm_isdst = -1
};

// defining a special safe undefined local time 1900-01-01 00:00:00
constexpr const std::tm safe_undefined_localtime = {
    .tm_sec = 0,
    .tm_min = 0,
    .tm_hour = 0,
    .tm_mday = 1,
    .tm_mon = 0,
    .tm_year = 0,
    .tm_wday = dow_Monday,
    .tm_isdst = -1    
};

/**
 * A Formalizer standardized version of the localtime() function that always
 * returns a usable value, but which may log errors or warnings as needed.
 * 
 * Note that errno is only set if localtime() returns nullptr, and therefore should
 * only be tested then. The errno value is otherwise undefined. One of the safeties
 * included here, in addition to always providing a usable time structure is an
 * optional error code that is always set to a valid value.
 * 
 * @param t_ptr Pointer to a (time_t) variable containing the UNIX epoch time to convert.
 * @param errorcode_ptr Optional pointer to a buffer for an errno error code.
 * @return Pointer to a local calendar time structure.
 */
const std::tm * safe_localtime(const std::time_t * t_ptr, int * errorcode_ptr) {

    if (!t_ptr) {
        return &safe_undefined_localtime;
    }
    if (*t_ptr < 0) {
        return &safe_undefined_localtime;
    }

    std::tm * localtime_ptr = localtime(t_ptr);
    if (localtime_ptr != nullptr) {
        return localtime_ptr;
    }

    if (errorcode_ptr) {
        (*errorcode_ptr) = errno; // should only be EOVERFLOW
    }

    return &safe_max_localtime;
}

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
 *         RTt_invalid_time_stamp.
 */
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
        return RTt_invalid_time_stamp;
    }
    const char *tschars = timestr.c_str();
    for (int i = tl - 12; i < tl; i++) {
        if ((tschars[i] < '0') || (tschars[i] > '9')) {
            if (!noerror) {
                ADDERROR(__func__, "invalid [^0-9]YYYYmmddHHMM time stamp (" + timestr + ") [failed test#2]");
            }
            return RTt_invalid_time_stamp;
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
        return RTt_invalid_time_stamp;
    }
    ts.tm_year -= 1900;
    ts.tm_wday = 0;
    ts.tm_yday = 0;
    ts.tm_isdst = -1; // computed by mktime since indicated "unknown" here
    return mktime(&ts);
}

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
std::time_t ymd_stamp_time(std::string timestr, bool noerror, bool ignoreHM) {
    if ((timestr.size()>=12) && ignoreHM) {
        for (unsigned int i = 8; i < 12; ++i)
            timestr[i] = '0';
    } else {
        if (timestr.size()==8)
            timestr += "0000";
    }

    return time_stamp_time(timestr,noerror);
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
    const tm * tm_ptr = safe_localtime(&t);
    if (!tm_ptr) {
        return "";
    }
    if (strftime(dstr,80,dateformat,tm_ptr) == 0) {
        return "";
    }
    return dstr;
}

std::string WeekDay(time_t t) {
    if (t < 0) return "";

    const tm * tm_ptr = safe_localtime(&t);
    return weekday_str[tm_ptr->tm_wday];
}

time_t time_add_day(time_t t, int days) {
    // This is relatively safe, because days are added by using localtime() and mktime()
    // instead of just adding days*SECONDSPERDAY. That should keep the time correct, even
    // through daylight savings time.
    //
    // NOTE: Nevertheless, when adding many days at once, this can break. In dil2al, the
    // same function has been shown to break for some iterations of virtual periodic task
    // target dates generated in alcomp.cc:generate_AL_prepare_periodic_tasks(),
    // which subsequently caused an alert in AL_Day::Add_Target_Date().

    if (t < 0) {
        return RTt_unspecified;
    }
    if (t == RTt_maxtime) {
        return RTt_maxtime;
    }

    struct tm tm(*safe_localtime(&t)); // copy
    tm.tm_mday += days;
    tm.tm_isdst = -1; // this tells mktime to determine if DST is in effect
    return mktime(&tm);
}

time_t time_add_month(time_t t, int months) {

    if (t < 0) {
        return RTt_unspecified;
    }
    if (t == RTt_maxtime) {
        return RTt_maxtime;
    }

    struct tm tm(*safe_localtime(&t)); // copy
    int years = months/12;
    months -= (years*12);
    tm.tm_year += years;
    tm.tm_mon += months;
    if (tm.tm_mon>11) {
    tm.tm_year++;
    tm.tm_mon -= 12;
    }
    tm.tm_isdst = -1;
    return mktime(&tm);
}

day_of_week time_day_of_week(time_t t) {
    if (t < 0) {
        return dow_Sunday;
    }

    const std::tm * tm_ptr;
    tm_ptr = safe_localtime(&t);
    return (day_of_week)tm_ptr->tm_wday;
}

unsigned int time_hour(time_t t) {
    const std::tm * tm_ptr;
    tm_ptr = safe_localtime(&t);
    return tm_ptr->tm_hour;
}

unsigned int time_minute(time_t t) {
    const std::tm * tm_ptr;
    tm_ptr = safe_localtime(&t);
    return tm_ptr->tm_min;
}

time_of_day_t time_of_day(time_t t) {
    const std::tm * tm_ptr;
    tm_ptr = safe_localtime(&t);
    return time_of_day_t(tm_ptr->tm_hour, tm_ptr->tm_min);
}

int time_month_length(time_t t) {
    if (t < 0) {
        return 0;
    }

    struct tm tm(*safe_localtime(&t)); // copy
    tm.tm_mday = 32;
    tm.tm_isdst = -1;
    time_t t2 = mktime(&tm);
    tm = (*safe_localtime(&t2)); // copy
    int m2day = tm.tm_mday;
    m2day--; // the number of days shorter than 31 that this month is
    return 31 - m2day;
}

time_t time_add_month_EOMoffset(time_t t) {

    if (t < 0) {
        return RTt_unspecified;
    }
    if (t == RTt_maxtime) {
        return RTt_maxtime;
    }

    int m1len = time_month_length(t);
    int m2len = time_month_length(time_add_month(t));
    struct tm tm(*safe_localtime(&t)); // copy
    int offset = m1len - tm.tm_mday;
    tm.tm_mday = m2len - offset;
    tm.tm_mon++;
    if (tm.tm_mon > 11) {
        tm.tm_year++;
        tm.tm_mon = 0;
    }
    tm.tm_isdst = -1;
    return mktime(&tm);
}

/**
 * Construct a year_month_day_t date from a UNIX epoch time.
 * 
 * @param t is UNIX epoch time.
 */
year_month_day_t::year_month_day_t(std::time_t t) {
    struct std::tm tm(*safe_localtime(&t));
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
