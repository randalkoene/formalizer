// Copyright 2020 Randal A. Koene
// License TBD

#include <iomanip>
#include <numeric>

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

/**
 * Find index of Log chunk from its ID.
 * 
 * This implementation attempts to be quick about it by relying on the sorted
 * order of Log chunks to apply a quick search method.
 * (This is actually probably the same as using std::binary_search.)
 * 
 * @param chunks the list of pointers to Log chunks.
 * @param chunk_id the Log chunk ID.
 * @return the index in the list, or ::size() if not found.
 */
Log_chunk_ptr_deque::size_type Log_chunks_Deque::find(const Log_chunk_ID_key chunk_id) const {
    if (size()<1)
        return 0;
    
    long lowerbound = 0;
    long upperbound = size()-1;
    long tryidx = size()/2;

    while (true) {

        if (get_tbegin_key(tryidx) == chunk_id)
            return tryidx;

        if (get_tbegin_key(tryidx) < chunk_id) {
            lowerbound = tryidx + 1;
        } else {
            upperbound = tryidx - 1;
        }

        if (lowerbound > upperbound)
            return size(); // not found

        tryidx = lowerbound + ((upperbound - lowerbound) / 2);
    }

    // never gets here
}

/**
 * This converts the list of Log breakpoint Log chunk IDs into a list of
 * indices into the list of Log chunks (in Log::chunks).
 * 
 * Note: All Log chunks must be loaded into memory before calling this function.
 * 
 * If a breakpoint was not found then the corresponding element of the vector
 * of indices has the value log::num_Chunks(), pointing beyond all valid
 * Log chunks in the deque.
 * 
 * @param log a Log object where all Log chunks are in the chunks deque.
 * @return a vector of indices into log::chunks.
 */
std::vector<Log_chunks_Deque::size_type> Breakpoint_Indices(Log & log) {
    std::vector<Log_chunks_Deque::size_type> indices(log.num_Breakpoints());
    for (std::deque<Log_chunk_ID_key>::size_type i = 0; i < log.num_Breakpoints(); ++i) {
        Log_chunk_ptr_deque::size_type idx = log.get_Chunks().find(log.breakpoints.get_chunk_id_key(i));
        if (idx>=log.get_Chunks().size()) {
            ADDERROR(__func__,"Log breakpoint["+std::to_string(i)+"]="+log.breakpoints.get_chunk_id_str(i)+" is not a known Log chunk");
            indices[i] = log.num_Chunks(); // indicates not found
        } else {
            indices[i] = idx;
        }
    }
    return indices;
}

/**
 * Find the frequency distribution for the number of Log chunks per
 * Log breakpoint.
 * 
 * Note: All Log chunks must be loaded into memory before calling this function.
 * 
 * @param log a Log object where all Log chunks are in the chunks deque.
 * @return a vector of counts.
 */
std::vector<Log_chunks_Deque::size_type> Chunks_per_Breakpoint(Log & log) {
    auto chunkindices = Breakpoint_Indices(log);
    chunkindices.push_back(log.num_Chunks() - 1); // append the latest Log chunk index
    auto diff = chunkindices;                     // easy way to make sure it's the same type and size
    std::adjacent_difference(chunkindices.begin(), chunkindices.end(), diff.begin());
    diff.erase(diff.begin()); // trim diff[0], see std::adjacent_difference()
    return diff;
}

/**
 * Calculate the total number of minutes logged for all Log chunks in the
 * specified deque.
 * 
 * @param chunks a deque containing a sorted list Log_chunk pointers.
 * @return the sum total of time logged in minutes.
 */
unsigned long Chunks_total_minutes(Log_chunks_Deque & chunks) {
    struct {
        unsigned long operator()(unsigned long total, const std::unique_ptr<Log_chunk> & chunkptr) {
            if (chunkptr)
                return total + chunkptr->duration_minutes();
            return total;
        }
    } duration_adder;
    return std::accumulate(chunks.begin(), chunks.end(), (unsigned long) 0, duration_adder);
}

/**
 * Calculate the total number of characters in Log entry description text in the
 * specified map.
 * 
 * @param entries a map containing Log_entry_ID_key and Log_entry smart pointer pairs.
 * @return the sum total of text characters.
 */
unsigned long Entries_total_text(Log_entries_Map & entries) {
    struct {
        unsigned long operator()(unsigned long total, const Log_entries_Map::value_type & entrypair) {
            if (entrypair.second)
                return total + entrypair.second->get_entrytext().size();
            return total;
        }
    } text_adder;
    return std::accumulate(entries.begin(), entries.end(), (unsigned long) 0, text_adder);
}

} // namespace fz
