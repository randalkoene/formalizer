// Copyright 2020 Randal A. Koene
// License TBD

#include <iomanip>

#include "Logtypes.hpp"

namespace fz {

#define VALID_LOG_ID_FAIL(f) \
    {                         \
        formerror = f;        \
        return false;         \
    }

/**
 * Test if a Log_TimeStamp can be used as a valid Log_entry_ID.
 * 
 * Note that years before 1999 are disqualified,
 * since the Formalizer did not exist before then.
 * 
 * @param idT reference to an Log_TimeStamp object.
 * @param formerror a string that collects specific error information if there is any.
 * @return true if valid.
 */
bool valid_Log_entry_ID(const Log_TimeStamp &idT, std::string &formerror) {
    if (idT.year < 1999)
        VALID_LOG_ID_FAIL("year");
    if ((idT.month < 1) || (idT.month > 12))
        VALID_LOG_ID_FAIL("month");
    if ((idT.day < 1) || (idT.day > 31))
        VALID_LOG_ID_FAIL("day");
    if (idT.hour > 23)
        VALID_LOG_ID_FAIL("hour");
    if (idT.minute > 59)
        VALID_LOG_ID_FAIL("minute");
    if (idT.minor_id < 1)
        VALID_LOG_ID_FAIL("minor_id");
    return true;
}

/**
 * Test if a Log_TimeStamp can be used as a valid Log_chunk_ID.
 * 
 * Note that years before 1999 are disqualified,
 * since the Formalizer did not exist before then.
 * 
 * @param idT reference to an Log_TimeStamp object.
 * @param formerror a string that collects specific error information if there is any.
 * @return true if valid.
 */
bool valid_Log_chunk_ID(const Log_TimeStamp &idT, std::string &formerror) {
    if (idT.year < 1999)
        VALID_LOG_ID_FAIL("year");
    if ((idT.month < 1) || (idT.month > 12))
        VALID_LOG_ID_FAIL("month");
    if ((idT.day < 1) || (idT.day > 31))
        VALID_LOG_ID_FAIL("day");
    if (idT.hour > 23)
        VALID_LOG_ID_FAIL("hour");
    if (idT.minute > 59)
        VALID_LOG_ID_FAIL("minute");
    return true;
}


/**
 * Test if a string can be used to form a valid Log_entry_ID.
 * 
 * Checks string length, period separating time stamp from minor ID,
 * all digits in time stamp and minor ID, and time stamp components
 * within valid ranges. Note that years before 1999 are disqualified,
 * since the Formalizer did not exist before then.
 * 
 * @param id_str a string of the format YYYYmmddHHMM.num.
 * @param formerror a string that collects specific error information if there is any.
 * @param id_timestamp if not NULL, receives valid components.
 * @return true if valid.
 */
bool valid_Log_entry_ID(std::string id_str, std::string &formerror, Log_TimeStamp *id_timestamp) {

    if (id_str.length() < 14)
        VALID_LOG_ID_FAIL("string size");
    if (id_str[12] != '.')
        VALID_LOG_ID_FAIL("format");
    for (int i = 0; i < 12; i++)
        if (!isdigit(id_str[i]))
            VALID_LOG_ID_FAIL("digits");

    Log_TimeStamp idT;
    idT.year = stoi(id_str.substr(0, 4));
    idT.month = stoi(id_str.substr(4, 2));
    idT.day = stoi(id_str.substr(6, 2));
    idT.hour = stoi(id_str.substr(8, 2));
    idT.minute = stoi(id_str.substr(10, 2));
    idT.minor_id = stoi(id_str.substr(13));
    if (!valid_Log_entry_ID(idT, formerror))
        return false;

    if (id_timestamp)
        *id_timestamp = idT;
    return true;
}

/**
 * Test if a string can be used to form a valid Log_chunk_ID.
 * 
 * Checks string length, all digits in time stamp, and time stamp components
 * within valid ranges. Note that years before 1999 are disqualified,
 * since the Formalizer did not exist before then.
 * 
 * @param id_str a string of the format YYYYmmddHHMM.
 * @param formerror a string that collects specific error information if there is any.
 * @param id_timestamp if not NULL, receives valid components.
 * @return true if valid.
 */
bool valid_Log_chunk_ID(std::string id_str, std::string &formerror, Log_TimeStamp *id_timestamp) {

    if (id_str.length() < 12)
        VALID_LOG_ID_FAIL("string size");
    for (int i = 0; i < 12; i++)
        if (!isdigit(id_str[i]))
            VALID_LOG_ID_FAIL("digits");

    Log_TimeStamp idT;
    idT.year = stoi(id_str.substr(0, 4));
    idT.month = stoi(id_str.substr(4, 2));
    idT.day = stoi(id_str.substr(6, 2));
    idT.hour = stoi(id_str.substr(8, 2));
    idT.minute = stoi(id_str.substr(10, 2));
    idT.minor_id = 0;
    if (!valid_Log_chunk_ID(idT, formerror))
        return false;

    if (id_timestamp)
        *id_timestamp = idT;
    return true;
}

std::string Log_entry_ID_TimeStamp_to_string(const Log_TimeStamp idT) {
    std::stringstream ss;
    ss << std::setfill('0')
       << std::setw(4) << (int) idT.year
       << std::setw(2) << (int) idT.month
       << std::setw(2) << (int) idT.day
       << std::setw(2) << (int) idT.hour
       << std::setw(2) << (int) idT.minute
       << '.' << std::setw(1) << (int) idT.minor_id;
    return ss.str();
}

std::string Log_chunk_ID_TimeStamp_to_string(const Log_TimeStamp idT) {
    std::stringstream ss;
    ss << std::setfill('0')
       << std::setw(4) << (int) idT.year
       << std::setw(2) << (int) idT.month
       << std::setw(2) << (int) idT.day
       << std::setw(2) << (int) idT.hour
       << std::setw(2) << (int) idT.minute;
    return ss.str();
}

std::tm Log_TimeStamp::get_local_time() const {
    std::tm tm = { 0 };
    tm.tm_year = year-1900;
    tm.tm_mon = month-1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    return tm;
}

Log_entry_ID_key::Log_entry_ID_key(const Log_TimeStamp& _idT) {
    idT=_idT;
    std::string formerror;
    if (!valid_Log_entry_ID(_idT,formerror)) throw(ID_exception(formerror));
}

Log_entry_ID_key::Log_entry_ID_key(std::string _idS) {
    std::string formerror;
    if (!valid_Log_entry_ID(_idS,formerror,&idT)) throw(ID_exception(formerror));
}

Log_chunk_ID_key::Log_chunk_ID_key(const Log_TimeStamp& _idT) {
    idT=_idT;
    std::string formerror;
    if (!valid_Log_chunk_ID(_idT,formerror)) throw(ID_exception(formerror));
}

Log_chunk_ID_key::Log_chunk_ID_key(std::string _idS) {
    std::string formerror;
    if (!valid_Log_chunk_ID(_idS,formerror,&idT)) throw(ID_exception(formerror));
}

Log_entry_ID::Log_entry_ID(const Log_TimeStamp _idT): idkey(_idT) {
    idS_cache = Log_entry_ID_TimeStamp_to_string(idkey.idT);
}

Log_chunk_ID::Log_chunk_ID(const Log_TimeStamp _idT): idkey(_idT) {
    idS_cache = Log_chunk_ID_TimeStamp_to_string(idkey.idT);
}

} // namespace fz
