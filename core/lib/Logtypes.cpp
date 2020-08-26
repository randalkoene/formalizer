// Copyright 2020 Randal A. Koene
// License TBD

#include <cstdint>
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
}

// Use appropriately!
void Log_chunk_ID::_nullID() {
    idkey.idT = Log_TimeStamp(); // null-key
    idS_cache = idkey.str();
}

void Log_chunk::set_Node_prev_chunk(Log_chunk * prevptr) { // give nullptr to clear
    if (prevptr!=this) {
        node_prev_chunk = prevptr;
        if (prevptr!=nullptr) {
            node_prev_chunk_id = prevptr->get_tbegin();
        } else {
            node_prev_chunk_id._nullID();
        }
    }
}

void Log_chunk::set_Node_next_chunk(Log_chunk * nextptr) { // give nullptr to clear
    if (nextptr!=this) {
        node_next_chunk = nextptr;
        if (nextptr!=nullptr) {
            node_next_chunk_id = nextptr->get_tbegin();
        } else {
            node_next_chunk_id._nullID();
        }
    }
}

/**
 * Find index of Log chunk from its ID.
 * 
 * This implementation attempts to be quick about it by relying on the sorted
 * order of Log chunks to apply a quick search method.
 * (This is actually probably the same as using std::binary_search.)
 * 
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

        if (lowerbound > upperbound) {
            return size(); // not found
        }

        tryidx = lowerbound + ((upperbound - lowerbound) / 2);
    }

    // never gets here
}

/**
 * Find index of Log chunk by its ID closest to time t.
 * 
 * This search will return either the index for the Log chunk with the
 * same start time, or the nearest above or below, depending on the
 * `later` flag.
 * 
 * @param t the Log chunk start time to search for.
 * @param later find start time >= t, otherwise find start time <= t.
 * @return the closest index in the list, or ::size() if not found.
 */
Log_chunk_ptr_deque::size_type Log_chunks_Deque::find(std::time_t t, bool later) const {
    if (size()<1)
        return 0;

    const Log_TimeStamp t_stamp(t);

    if (later) { // test if all Log chunks are later than t
        if (t_stamp < get_tbegin_idT(0))
            return 0;
    } else { // test if all Log chunks are earlier than t
        if (get_tbegin_idT(size()-1) < t_stamp)
            return size()-1;
    }
    
    long lowerbound = 0;
    long upperbound = size()-1;
    long tryidx = size()/2;

    while (true) {

        const Log_TimeStamp t_candidate(get_tbegin_idT(tryidx));
        if (t_candidate == t_stamp)
            return tryidx; // return match

        if (t_candidate < t_stamp) {
            lowerbound = tryidx + 1;
        } else {
            upperbound = tryidx - 1;
        }

        if (lowerbound > upperbound) { // return nearest
            if (later) { // determine which one is nearest
                if (t_stamp < t_candidate)
                    return tryidx;
                else
                    return tryidx+1;
            } else {
                if (t_candidate < t_stamp)
                    return tryidx;
                else
                    return tryidx-1;
            }
        }

        tryidx = lowerbound + ((upperbound - lowerbound) / 2);
    }

    // never gets here
}

/**
 * Find the Breakpoint section to which the Log chunk with the given
 * ID key belongs.
 * 
 * @param key is the Log chunk ID key.
 * @return the Breakpoint start time identifying Log chunk ID key.
 */
const Log_chunk_ID_key & Log_Breakpoints::find_Breakpoint_tstamp_before_chunk(const Log_chunk_ID_key key) {
    for (auto it = begin(); it != end(); ++it) {
        if (key < (*it)) {
            return (*std::prev(it));
        }
    }
    return (*std::prev(end()));
}

/**
 * Find the Breakpoint section to which the Log chunk with the given
 * ID key belongs.
 * 
 * @param key is the Log chunk ID key.
 * @return the Breakpoint index in the table of breakpoints.
 */
Log_chunk_ID_key_deque::size_type Log_Breakpoints::find_Breakpoint_index_before_chunk(const Log_chunk_ID_key key) {
    for (auto i = 0; i < size(); ++i) {
        if (key < at(i)) {
            return (i>0) ? (i-1) : 0;
        }
    }
    return size()-1;
}

/**
 * Parse all chunks connected to the Log and find the sequence that
 * belongs to a specific Node.
 * 
 * This is used by Log::setup_Chunk_nodeprevnext(), and it can also
 * be used independent of the reference parameters in the Log_chunk
 * objects, which even works for arbitrary loaded lists of Log chunks.
 * 
 * @param node_id of the Node for which to collect Log chunks.
 * @return a deque sorted list of smart pointers to all chunks found.
 */
std::deque<Log_chunk *> Log::get_Node_chunks_fullparse(const Node_ID node_id) {
    std::deque<Log_chunk *> res;
    for (auto it = chunks.begin(); it<chunks.end(); ++it) {
        if ((*it)->get_NodeID().key().idT == node_id.key().idT) {
            res.emplace_back(it->get());
        }
    }
    return res;
}

/**
 * Quickly walk through the reference chain that belongs to the
 * specified Node and return all of its Log chunks.
 * 
 * Note that this function depends on valid rapid-access
 * `node_prev_chunk` and `node_next_chunk` variables in each
 * Log chunk.
 * 
 * @param node_id of the Node for which to collect Log chunks.
 * @return a deque sorted list of smart pointers to all chunks found.
 */
std::deque<Log_chunk *> Log::get_Node_chunks(const Node_ID node_id) {
    std::deque<Log_chunk *> res;
    for (auto it = chunks.begin(); it<chunks.end(); ++it) {
        if ((*it)->get_NodeID().key().idT == node_id.key().idT) {
            res.emplace_back(it->get());
            break; // Do the rest via the rapid-access pointer chain
        }
    }

    if (res.empty())
        return res;

    for (Log_chunk * chunk = res.front()->get_node_next_chunk(); chunk != nullptr; chunk = chunk->get_node_next_chunk()) {
        res.emplace_back(chunk);
    }
    return res;
}

struct Node_Chunks_cursor {
    Log_chunk * head;
    Log_chunk * tail;
    unsigned long count;

    Node_Chunks_cursor(Log_chunk * tailhead): head(tailhead), tail(tailhead), count(1) {
        tailhead->set_Node_prev_chunk(nullptr); // clear in case previously attached
        tailhead->set_Node_next_chunk(nullptr); // clear in case previously attached
    }

    bool append(Log_chunk & chunk) {
        if ((&chunk==tail) || (chunk.get_node_prev_chunk()!=nullptr))
            ERRRETURNFALSE(__func__,"unable to append a Log chunk that was already chained");

        chunk.set_Node_next_chunk(nullptr); // clearing any old attachements as we go
        // Link into the chain
        chunk.set_Node_prev_chunk(tail);
        tail->set_Node_next_chunk(&chunk);
        count++;
        // New tail of chain
        tail = &chunk;
    }
};

/**
 * Parse the deque list of Log chunks and assign all references in
 * `node_prev_chunk_id`, `node_next_chunk_id`, and their rapid-access
 * pointers.
 */
void Log::setup_Chunk_nodeprevnext() {
    std::map<Node_ID_key,Node_Chunks_cursor> cursors;
    for (const auto& chunkptr : chunks) { // .begin(); it != chunks.end(); ++it) {
        Log_chunk * chunk = chunkptr.get();
        auto [node_cursor_it, was_new] = cursors.emplace(chunk->get_NodeID().key(),chunk); // first of a Node
        if (!was_new) node_cursor_it->second.append(*chunk); // adding to a Node's chain
    }
}

/**
 * Get a pair of iterators that indicate the begin and end of an interval of
 * Log entry objects. Both `interval_front` and `interval_back` must exist in
 * the map of Log entries. Otherwise, a pair of out-of-range iterators set to
 * entries.end() are returned.
 * 
 * Note that the second iterator returned behaves like all end() iterators
 * of C++ STL containers. In other words, it points to the object 1 beyond
 * the object identified by `interval_back`. If that the `interval_back`
 * object was the last entry then the second iterator == entries.end().
 * 
 * @param interval_front is the Log entry ID key of an entry in the Log.
 * @param interval_back is the Log entry ID key of an entry in the Log.
 * @return pair of iterators describing the begin and end of a subset of entries.
 */
Log_entry_iterator_interval Log::get_Entries_interval(const Log_entry_ID_key interval_front, const Log_entry_ID_key interval_back) {
    if (interval_back < interval_front)
        return std::make_pair(entries.end(),entries.end());

    auto from_it = entries.find(interval_front);    
    if (from_it==entries.end())
        return std::make_pair(from_it,from_it);
    
    auto to_it = entries.find(interval_back);
    if (to_it==entries.end())
        return std::make_pair(to_it,to_it);
    
    return make_pair(from_it,to_it);
}

/**
 * Get a pair of iterators that indicate the begin and end of an interval of
 * Log entry objects, where those Log entry objects all have ID keys that
 * represent times t, with t_from <= t < t_before. If the interval is
 * entirely outside of the times represented by exissting Log entries then
 * a pair of out-of-range iterators set to entries.end() are returned.
 * 
 * Note that the second iterator returned behaves like all end() iterators
 * of C++ STL containers. In other words, it points to the object with
 * Log entry ID key representing a time at or beyond t_before, which may
 * be entries.end().
 * 
 * @param t_from is the earliest UNIX epoch time within the interval.
 * @param t_before is the UNIX epoch time 1 second after the interval.
 * @return pair of iterators describing the begin and end of a subset of entries.
 */
Log_entry_iterator_interval Log::get_Entries_t_interval(std::time_t t_from, std::time_t t_before) {
    if (t_before <= t_from)
        return std::make_pair(entries.end(),entries.end());

    Log_TimeStamp lowerbound_stamp(t_from,false,1);
    Log_TimeStamp upperbound_stamp(t_before-1,false,1);
    Log_entry_ID_key lowerbound_key, upperbound_key;
    lowerbound_key.idT = lowerbound_stamp; // this circumvention is really only justifiable here
    upperbound_key.idT = upperbound_stamp;

    auto from_it = entries.lower_bound(lowerbound_key);
    auto before_it = entries.upper_bound(upperbound_key);
    return make_pair(from_it,before_it);
}

/**
 * Get a pair of iterators that indicate the begin and end of an interval of
 * Log entry objects. The Log entry with Log entry ID key `interval_front`
 * must exist in the map of Log entries and n > 0. Otherwise, a pair of
 * out-of-range iterators set to entries.end() are returned.
 * 
 * Note that the second iterator returned behaves like all end() iterators
 * of C++ STL containers. In other words, it points to the object 1 beyond
 * the nth object in the interval, or it is entries.end().
 * 
 * @param interval_front is the Log entry ID key of an entry in the Log.
 * @param n is the number of Log entries to include in the interval.
 * @return pair of iterators describing the begin and end of a subset of entries.
 */
Log_entry_iterator_interval Log::get_Entries_interval(const Log_entry_ID_key interval_front, unsigned long n) {
    if (n==0)
        return std::make_pair(entries.end(),entries.end());

    auto from_it = entries.find(interval_front);    
    if (from_it==entries.end())
        return std::make_pair(from_it,from_it);  

    auto before_it = std::next(from_it,n);
    return make_pair(from_it,before_it);      
}


/**
 * Get a pair of iterators that indicate the begin and end of an interval of
 * up to n Log entry objects. The Log entry objects all have ID keys that
 * represent times t, with t_from <= t, and n > 0. Aa pair of out-of-range
 * iterators set to entries.end() are returned if these conditions cannot
 * be met or if the interval is empty.
 * 
 * Note that the second iterator returned behaves like all end() iterators
 * of C++ STL containers. In other words, it points to the object with
 * Log entry ID key representing a time at or beyond t_before, which may
 * be entries.end().
 * 
 * @param t_from is the earliest UNIX epoch time within the interval.
 * @param n is the number of Log entries to include in the interval.
 * @return pair of iterators describing the begin and end of a subset of entries.
 */
Log_entry_iterator_interval Log::get_Entries_n_interval(std::time_t t_from, unsigned long n) {
    if (n==0)
        return std::make_pair(entries.end(),entries.end());

    Log_TimeStamp lowerbound_stamp(t_from,false,1);
    Log_entry_ID_key lowerbound_key;
    lowerbound_key.idT = lowerbound_stamp; // this circumvention is really only justifiable here

    auto from_it = entries.lower_bound(lowerbound_key);
    auto before_it = std::next(from_it,n);
    return make_pair(from_it,before_it);
}

/**
 * Get the interval of Log chunks with t_from <= t_begin < t_before.
 * 
 * The interval is returned as a pair of Log_chunk_ID_key objects and is
 * inclusive. Both ID keys belong to Log chunks within the interval.
 * If the interval is empty then a pair of null-key are returned.
 * 
 * @param t_from is the earliest UNIX epoch time within the interval.
 * @param t_before is the UNIX epoch time 1 second after the interval.
 * @return a pair of Log chunk ID keys.
 */
Log_chunk_ID_interval Log::get_Chunks_ID_t_interval(std::time_t t_from, std::time_t t_before) {
    auto [from_idx, to_idx] = get_Chunks_index_t_interval(t_from,t_before);

    if (from_idx>=chunks.size())
        return std::make_pair(Log_chunk_ID_key(),Log_chunk_ID_key()); // returning null-key pair

    return std::make_pair(get_chunk_id_key(from_idx),get_chunk_id_key(to_idx));
}

/**
 * Get an interval of n Log chunks with t_begin >= t_from.
 * 
 * The interval is returned as a pair of Log_chunk_ID_key objects and is
 * inclusive. Both ID keys belong to Log chunks within the interval.
 * If the interval is empty then a pair of null-key are returned.
 * 
 * @param t_from is the earliest UNIX epoch time within the interval.
 * @param n is the number of Log chunks to include in the interval.
 * @return a pair of Log chunk ID keys.
 */
Log_chunk_ID_interval Log::get_Chunks_ID_n_interval(std::time_t t_from, unsigned long n) {
    auto [from_idx, to_idx] = get_Chunks_index_t_interval(t_from,n);

    if (from_idx>=chunks.size())
        return std::make_pair(Log_chunk_ID_key(),Log_chunk_ID_key()); // returning null-key pair

    return std::make_pair(get_chunk_id_key(from_idx),get_chunk_id_key(to_idx));
}

/**
 * Get the interval of Log chunks with t_from <= t_begin < t_before.
 * 
 * The interval is returned as a pair of indices and is inclusive. Both
 * indices are within the interval. If the interval is empty then the
 * indices are set to chunks.size(), beyond the range of valid indices.
 * 
 * @param t_from is the earliest UNIX epoch time within the interval.
 * @param t_before is the UNIX epoch time 1 second after the interval.
 * @return a pair of indices bounding Log chunks within the interval, or
 *         a pair of indices beyond the range of existing Log chunks if
 *         no Log chunks have start times within the interval.
 */
Log_chunk_index_interval Log::get_Chunks_index_t_interval(std::time_t t_from, std::time_t t_before) {
    std::time_t t_to = t_before - 1;

    if (t_to<t_from)
        return std::make_pair(num_Chunks(),num_Chunks());

    Log_chunk_ptr_deque::size_type from_idx = chunks.find(t_from,true);
    if (from_idx>=chunks.size())
        return std::make_pair(from_idx,from_idx);

    Log_chunk_ptr_deque::size_type to_idx = chunks.find(t_to,false);

    return std::make_pair(from_idx,to_idx);
}

/**
 * Get an interval of n Log chunks with t_begin >= t_from.
 * 
 * The interval is returned as a pair of indices and is inclusive. Both
 * indices are within the interval. If the interval is empty then the
 * indices are set to chunks.size(), beyond the range of valid indices.
 * 
 * @param t_from is the earliest UNIX epoch time within the interval.
 * @param n is the number of Log chunks to include in the interval.
 * @return a pair of indices bounding Log chunks within the interval, or
 *         a pair of indices beyond the range of existing Log chunks if
 *         no Log chunks were found for the interval.
 */
Log_chunk_index_interval Log::get_Chunks_index_n_interval(std::time_t t_from, unsigned long n) {
    if (n==0)
        return std::make_pair(num_Chunks(),num_Chunks());

    Log_chunk_ptr_deque::size_type from_idx = chunks.find(t_from,true);
    if (from_idx>=chunks.size())
        return std::make_pair(from_idx,from_idx);

    Log_chunk_ptr_deque::size_type to_idx = from_idx + (n-1);
    if (to_idx >= chunks.size())
        to_idx = chunks.size()-1;
    
    return std::make_pair(from_idx,to_idx);
}

// +----- begin: friend functions -----+

/**
 * Find the main Topic of a Log chunk's Node, as indicated by the maximum
 * Topic_Relevance value.
 * 
 * Note: This is a friend function in order to ensure that the search for
 *       the Topic object is called only when a valid Topic_Tags list
 *       can provid pointers to them.
 * 
 * @param _graph a valid Graph with Topic_Tags list.
 * @param chunk a Log_chunk for which the main Topic is requested.
 * @return a pointer to the Topic object (or nullptr if not found).
 */
Topic * main_topic(Graph & _graph, Log_chunk & chunk) {
    return _graph.main_Topic_of_Node(*chunk.get_Node(_graph));
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

// +----- end  : friend functions -----+

} // namespace fz
