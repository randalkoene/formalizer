// Copyright 2020 Randal A. Koene
// License TBD

#include <cstdint>
#include <iomanip>
#include <numeric>

#include "LogtypesID.hpp"
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

std::string Log_TimeStamp_to_Ymd_string(const Log_TimeStamp idT) {
    std::stringstream ss;
    ss << std::setfill('0')
       << std::setw(4) << (int) idT.year
       << std::setw(2) << (int) idT.month
       << std::setw(2) << (int) idT.day;
    return ss.str();
}

/**
 * Generate a candidate Log time stamp from UNIX epoch time.
 * 
 * If the `testvalid` flag is set then the validity test for use
 * as a standardized Formalizer Log_chunk_ID is carried out and
 * can cause an `ID_exception`. (In essence, all this adds is a
 * test that year>=1999).
 * 
 * Note that the default `_minorid` is 0, which is expected for
 * a Log_chunk_ID, while Log_entry_ID `minor_id` values normally
 * start at 1.
 * 
 * @param t a UNIX epoch time.
 * @param testvalid if set causes year check (default: false).
 * @param _minorid to set (default: 0).
 */
Log_TimeStamp::Log_TimeStamp(std::time_t t, bool testvalid, uint8_t _minorid) {
    std::tm * tm = std::localtime(&t);
    year = tm->tm_year + 1900;
    month = tm->tm_mon + 1;
    day = tm->tm_mday;
    hour = tm->tm_hour;
    minute = tm->tm_min;
    minor_id = _minorid;
    if (testvalid) {
        std::string formerror;
        if (!valid_Log_chunk_ID((*this),formerror)) throw(ID_exception(formerror));
    }
}

/**
 * Convert the standardized Formalizer Log time stamp into equivalent localtime
 * structure.
 * 
 * @return localtime structure (contains all-zero if null-stamp).
 */
std::tm Log_TimeStamp::get_local_time() const {
    std::tm tm = { 0 };
    if (isnullstamp())
        return tm;

    tm.tm_year = year-1900;
    tm.tm_mon = month-1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    return tm;
}

/**
 * Convert the standardized Formalizer Log time stamp into equivalent UNIX epoch
 * time.
 * 
 * @return the equivalent UNIX epoch time, or -1 if null-stamp.
 */
time_t Log_TimeStamp::get_epoch_time() const {
    if (isnullstamp())
        return -1;

    std::tm tm(get_local_time());
    return mktime(&tm);
}


Log_chain_target::Log_chain_target(const Log_chunk & _chunk): ischunk(true), chunk(_chunk.get_tbegin_key(),&_chunk) {
}

Log_chain_target::Log_chain_target(const Log_entry & _entry): ischunk(false), entry(_entry.get_id_key(),&_entry) {
}

void Log_chain_target::set_chunk_target(const Log_chunk & _chunk) {
    ischunk = true;
    chunk.key = _chunk.get_tbegin_key();
    chunk.ptr = const_cast<Log_chunk *>(&_chunk);
}

void Log_chain_target::set_entry_target(const Log_entry & _entry) {
    ischunk = false;
    entry.key = _entry.get_id_key();
    entry.ptr = const_cast<Log_entry *>(&_entry);
}

/**
 * Test if two Log chain targets point to the same thing.
 * 
 * Note A: It is assumed that if a poointer is non-null, then it is meaningfully set.
 * Note B: This comparison could also have been done lexicographically.
 * 
 * @param target is the other Log chain target to compare with this one.
 * @return true if they point to the same thing (have the same value).
 */
bool Log_chain_target::same_target(Log_chain_target & target) {
    if (ischunk != target.ischunk)
        return false;

    if (ischunk) {
        if ((chunk.ptr != nullptr) || (target.chunk.ptr != nullptr)) { // test by ptr
            return (chunk.ptr == target.chunk.ptr);
        } else { // test by key
            return (chunk.key == target.chunk.key);
        }
    } else {
        if ((entry.ptr != nullptr) || (target.entry.ptr != nullptr)) {
            return (entry.ptr == target.entry.ptr);
        } else { // test by key
            return (entry.key == target.entry.key);
        }
    }
    // never gets here
}

/// Test if this target is the `chunkref`. (See more description above.)
bool Log_chain_target::same_target(Log_chunk & chunkref) {
    if (!ischunk)
        return false;

    if (chunk.ptr != nullptr) { // test by ptr
        return (chunk.ptr == &chunkref);

    } else { // test by key
        if (chunk.key == chunkref.get_tbegin_key()) {
            //chunk.ptr = &chunkref; // could grab it while handy, but what if nullptr was on purpose? (better to set explicitly)
            return true;                
        } else {
            return false;
        }
    }
    // never gets here
}

/// Test if this target is the `entryref`. (See more description above.)
bool Log_chain_target::same_target(Log_entry & entryref) {
    if (ischunk)
        return false;

    if (entry.ptr != nullptr) { // test by ptr
        return (entry.ptr == &entryref);

    } else { // test by key
        if (entry.key == entryref.get_id_key()) {
            //entry.ptr = &entryref; // could grab it while handy, but what if nullptr was on purpose? (better to set explicitly)
            return true;                
        } else {
            return false;
        }
    }
    // never gets here
}

/**
 * Use rapid-access pointers to point to next in by-Node chain.
 * 
 * This function requires that rapid-access pointers have been set up.
 * 
 * @return pointer to the next target in the chain, or nullptr if this
 *         is a null-target (e.g. end of chain).
 */
const Log_chain_target * Log_chain_target::next_in_chain() const {
    if (isnulltarget_byptr())
        return nullptr;
    
    if (ischunk) {
        return &(chunk.ptr->get_node_next());
    } else {
        return &(entry.ptr->get_node_next());
    }
}

/// See description for go_next_in_chain().
const Log_chain_target * Log_chain_target::prev_in_chain() const {
    if (isnulltarget_byptr())
        return nullptr;
    
    if (ischunk) {
        return &(chunk.ptr->get_node_prev());
    } else {
        return &(entry.ptr->get_node_prev());
    }
}

void Log_chain_target::bytargetptr_set_Node_next_ptr(Log_chunk * _next) {
    if (ischunk) {
        chunk.ptr->set_Node_next_ptr(_next);
    } else {
        entry.ptr->set_Node_next_ptr(_next);
    }
}

void Log_chain_target::bytargetptr_set_Node_next_ptr(Log_entry * _next) {
    if (ischunk) {
        chunk.ptr->set_Node_next_ptr(_next);
    } else {
        entry.ptr->set_Node_next_ptr(_next);
    }
}

void Log_chain_target::bytargetptr_set_Node_prev_ptr(Log_chunk * _prev) {
    if (ischunk) {
        chunk.ptr->set_Node_prev_ptr(_prev);
    } else {
        entry.ptr->set_Node_prev_ptr(_prev);
    }
}

void Log_chain_target::bytargetptr_set_Node_prev_ptr(Log_entry * _prev) {
    if (ischunk) {
        chunk.ptr->set_Node_prev_ptr(_prev);
    } else {
        entry.ptr->set_Node_prev_ptr(_prev);
    }
}

Log_entry_ID_key::Log_entry_ID_key(const Log_TimeStamp& _idT) {
    idT=_idT;
    std::string formerror;
    if (!valid_Log_entry_ID(_idT,formerror)) throw(ID_exception(formerror));
}

Log_entry_ID_key::Log_entry_ID_key(const Log_chunk_ID_key& _idC, uint8_t _minorid): idT(_idC.get_epoch_time(),false,_minorid) {
    // no need to test valid if Log_chunk_ID_key was valid
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

// Use very carefully!
bool Log_chunk_ID::_reassign(std::string _idS) {
    std::string formerror;
    if (!valid_Log_chunk_ID(_idS,formerror,&idkey.idT))
        ERRRETURNFALSE(__func__,"invalid Log chunk ID "+_idS+" ("+formerror+')');

    idS_cache = _idS;
    return true;
}

// Use very carefully!
bool Log_chunk_ID::_reassign(const Log_TimeStamp _idT) {
    std::string formerror;
    if (!valid_Log_chunk_ID(_idT,formerror))
        ERRRETURNFALSE(__func__,"invalid Log chunk ID "+Log_chunk_ID_TimeStamp_to_string(_idT)+" ("+formerror+')');

    idkey.idT = _idT;
    idS_cache = idkey.str();
    return true;
}

// Use appropriately!
void Log_chunk_ID::_nullID() {
    idkey.idT = Log_TimeStamp(); // null-key
    idS_cache = idkey.str();
}

// +----- begin: friend functions -----+



// +----- end  : friend functions -----+

} // namespace fz
