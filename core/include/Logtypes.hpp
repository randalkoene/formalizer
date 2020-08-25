// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares Log types for use with the Formalizer.
 * These define the authoritative version of the data structure for use in C++ code.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __LOGTYPES_HPP.
 */

/**
 * Notes about the C++ Log object structure:
 * - Every entry created (with fzlog) makes a Log_entry. These have a consecutive order
 *   in time when created. Every Log_entry also has a unique ID. In the dil2al HTML
 *   format that was the A-NAME reference.
 * - The Log as a whole has some extra data that is for dil2al format backwards
 *   compatibility, which is stored in a dil2al_files table containing: active_list
 *   file reference, TLbreakpoints array that explains how the Task Log was split into
 *   individual HTML files.
 * - In addition to the table of Log_entry records, the Log also has a table of
 *   Log_chunk records. Those records specify: t_begin for the time stamp when a chunk
 *   begins, entry_id for the first entry in the chunk, node_id for the Node that the
 *   whole chunk belongs to.
 * - The Log_chunk records also record the time when the chunk was closed. This is
 *   necessary, since Log chunks are not guaranteed to be temporally adjacent.
 *   Especially during the early use of dil2al scheduling and logging was not done
 *   for the full 24 hours of each day. Missing time intervals are tolerated.
 * - When loaded, these things are translated into rapid access containers with pointers
 *   to Node and Log_entry objects.
 * 
 * For more information see:
 * - The development Trello card at https://trello.com/c/NNSWVRFf.
 */

#ifndef __LOGTYPES_HPP
#include "coreversion.hpp"
#define __LOGTYPES_HPP (__COREVERSION_HPP)

#include <memory>
#include <map>
#include <ctime>

#include "error.hpp"
#include "TimeStamp.hpp"
#include "Graphtypes.hpp"

namespace fz {

class Log_TimeStamp;
class Log;
class Log_entry;
class Log_chunk;

std::string Log_entry_ID_TimeStamp_to_string(const Log_TimeStamp idT);

std::string Log_chunk_ID_TimeStamp_to_string(const Log_TimeStamp idT);

std::string Log_TimeStamp_to_Ymd_string(const Log_TimeStamp idT);

bool valid_Log_entry_ID(const Log_TimeStamp &idT, std::string &formerror);

bool valid_Log_chunk_ID(const Log_TimeStamp &idT, std::string &formerror);

bool valid_Log_entry_ID(std::string id_str, std::string &formerror, Log_TimeStamp *id_timestamp = NULL);

bool valid_Log_chunk_ID(std::string id_str, std::string &formerror, Log_TimeStamp *id_timestamp = NULL);

/**
 * Timestamp IDs in the format required for Log IDs.
 * These include all time components from year to minute, as well as an additional
 * minor_id (this is NOT a decimal, since .10 is higher than .9).
 * The minor_id is used only in Log Entry IDs, not in Log Chunk IDs.
 * Note that the time formatting is not the same as in the C time structure 'tm'.
 * Days and months count from 1. The year is given as is, not relative to the
 * UNIX epoch. (By contrast, tm time subtracts 1900 years.)
 * 
 * These structures are mainly used as unique IDs, but conversion to UNIX time
 * is provided through member functions.
 * Time conversion to UNIX time is carried out by the get_time() functions.
 */
struct Log_TimeStamp {
    uint8_t minor_id;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    int16_t year;
    Log_TimeStamp(): minor_id(0), minute(0), hour(0), day(0), month(0), year(0) {}
    bool operator< (const Log_TimeStamp& rhs) const {
        return std::tie(year,month,day,hour,minute,minor_id)
             < std::tie(rhs.year,rhs.month,rhs.day,rhs.hour,rhs.minute,rhs.minor_id);
    }
    bool operator== (const Log_TimeStamp& rhs) const {
        return std::tie(year,month,day,hour,minute,minor_id)
             == std::tie(rhs.year,rhs.month,rhs.day,rhs.hour,rhs.minute,rhs.minor_id);
    }
    std::tm get_local_time() const;
    time_t get_epoch_time() const {
        std::tm tm = get_local_time();
        return mktime(&tm);
    }
};

union Log_entry_ID_key {
    Log_TimeStamp idT;

    Log_entry_ID_key(): idT() {}
    Log_entry_ID_key(const Log_TimeStamp& _idT);
    Log_entry_ID_key(std::string _idS);
    bool operator< (const Log_entry_ID_key& rhs) const {
        return (idT < rhs.idT);
    }
    bool operator== (const Log_entry_ID_key& rhs) const {
        return (idT == rhs.idT);
    }

};

union Log_chunk_ID_key {
    Log_TimeStamp idT;

    Log_chunk_ID_key(): idT() {}
    Log_chunk_ID_key(const Log_TimeStamp& _idT);
    Log_chunk_ID_key(std::string _idS);
    std::time_t get_epoch_time() const { return idT.get_epoch_time(); }
    std::string str() const { return TimeStamp("%Y%m%d",idT.get_epoch_time()); }
    bool operator< (const Log_chunk_ID_key& rhs) const {
        return (idT < rhs.idT);
    }
    bool operator== (const Log_chunk_ID_key& rhs) const {
        return (idT == rhs.idT);
    }
};

class Log_entry_ID {
    friend class Log_entry;
protected:
    Log_entry_ID_key idkey;
    std::string idS_cache; // cached string version of the ID to speed things up
public:
    Log_entry_ID(std::string _idS): idkey(_idS), idS_cache(_idS) {}
    Log_entry_ID(const Log_TimeStamp _idT);
    Log_entry_ID() = delete; // explicitly forbid the default constructor, just in case
    Log_entry_ID_key key() const { return idkey; }
    std::string str() const { return idS_cache; }
    time_t get_epoch_time() const { return idkey.idT.get_epoch_time(); }
};

class Log_chunk_ID {
    friend class Log_chunk;
protected:
    Log_chunk_ID_key idkey;
    std::string idS_cache; // cached string version of the ID to speed things up
public:
    Log_chunk_ID(std::string _idS): idkey(_idS), idS_cache(_idS) {}
    Log_chunk_ID(const Log_TimeStamp _idT);
    Log_chunk_ID() = delete; // explicitly forbid the default constructor, just in case
    Log_chunk_ID_key key() const { return idkey; }
    std::string str() const { return idS_cache; }
    time_t get_epoch_time() const { return idkey.idT.get_epoch_time(); }
};

class Log_entry {
    friend class Log;
    friend class Log_chunk;
protected:
    const Log_entry_ID id;          /// The id that extends the chunk time stamp.
    const Node_ID_key node_idkey;   /// Reference to the Node this belongs to or zero-stamp.
    std::string entrytext;          /// Text content (typically in HTML).

    // The following are maintained for rapid access where possible.
    Node *node;
    Log_chunk *chunk;

public:
    Log_entry(const Log_TimeStamp &_id, std::string _entrytext, const Node_ID_key &_nodeidkey, Log_chunk * _chunk = NULL): id(_id), node_idkey(_nodeidkey), entrytext(_entrytext), chunk(_chunk) {}
    Log_entry(const Log_TimeStamp &_id, std::string _entrytext, Log_chunk * _chunk = NULL): id(_id), entrytext(_entrytext), chunk(_chunk) {}

    bool set_Node(Node & _node) {
        if (!(node_idkey.idT == _node.get_id().key().idT))
            return false;

        node = &_node;
        return true;
    }
    void set_Chunk(Log_chunk * _chunk) { chunk = _chunk; }

    const Log_entry_ID & get_id() const { return id; }
    const Node_ID_key & get_nodeidkey() const { return node_idkey; }
    std::string & get_entrytext() { return entrytext; }
    Node * get_Node() { return node; }
    Log_chunk * get_Chunk() { return chunk; }
    time_t get_epoch_time() const { return id.get_epoch_time(); }

    // helper functions
    const Log_entry_ID_key & get_id_key() const { return id.idkey; }
    std::string get_id_str() const { return id.str(); }
};

/**
 * Specifies the start time, Node, close time, and set of entries of a Log chunk.
 * 
 * Note A: The Log_chunk_ID differs from Log_entry_ID in that there is no minor_id.
 * 
 * Note B:
 * It is possible to get the sorted list of Log chunks that belong to a specific
 * Node by parsing the list log Log chunks and identifying all of the ones that
 * belong to the Node. Not only would that be slow (which would at least sugegst
 * maintaining a rapid-access list), but it is also counter-productive when you
 * expect that you will rarely load the complete Log into memory at once. For this
 * reason, `Log_chunk` also explicitly stores `node_prevchunk_id` and
 * `node_nextchunk_id`.
 */
class Log_chunk {
protected:
    // These three must be provided when the object is created.
    const Log_chunk_ID t_begin;            /// The time stamp when a Log chunk begins.
    const Node_ID node_id;                 /// The Node to which the Log chunk belongs.
    std::time_t t_close;                   /// The time when a Log chunk was closed (-1 if not closed).
    // The following are expected, but can be added after object creation.
    Log_chunk_ID node_prev_chunk_id; /// Previous chunk belonging to the same Node.
    Log_chunk_ID node_next_chunk_id; /// Next chunk belonging to the same Node.
    Log_entry_ID_key first_entry;          /// ID of the first Log_entry in the chunk (once created).

    // The following are maintained for rapid access where possible.
    Node *node;
    std::vector<Log_entry *> entries;
    Log_chunk * node_prev_chunk;
    Log_chunk * node_next_chunk;

public:
    Log_chunk(const Log_TimeStamp &_tbegin, const Node_ID &_nodeid, std::time_t _tclose) : t_begin(_tbegin), node_id(_nodeid), t_close(_tclose), node(NULL), node_prev_chunk(NULL), node_next_chunk(NULL) {}
    Log_chunk(const Log_TimeStamp &_tbegin, Node &_node, std::time_t _tclose): t_begin(_tbegin), node_id(_node.get_id()), t_close(_tclose), node(&_node), node_prev_chunk(NULL), node_next_chunk(NULL) {}
    Log_chunk(const Log_TimeStamp &_tbegin, const Node_ID &_nodeid, std::time_t _tclose, const Log_TimeStamp & _prev, const Log_TimeStamp & _next) : t_begin(_tbegin), node_id(_nodeid), t_close(_tclose), node_prev_chunk_id(_prev), node_next_chunk_id(_next), node(NULL), node_prev_chunk(NULL), node_next_chunk(NULL) {}
    Log_chunk(const Log_TimeStamp &_tbegin, Node &_node, std::time_t _tclose, const Log_TimeStamp & _prev, const Log_TimeStamp & _next): t_begin(_tbegin), node_id(_node.get_id()), t_close(_tclose), node_prev_chunk_id(_prev), node_next_chunk_id(_next), node(&_node), node_prev_chunk(NULL), node_next_chunk(NULL) {}

    bool set_Node(Node & _node) {
        if (!(node_id.key().idT == _node.get_id().key().idT))
            return false;

        node = &_node;
        return true;
    }
    void set_Node_prev_chunk(const Log_TimeStamp & _prev) { node_prev_chunk_id = _prev; }
    void set_Node_next_chunk(const Log_TimeStamp & _next) { node_next_chunk_id = _next; }
    void add_Entry(Log_entry &entry) {
        if (entries.empty()) {
            first_entry = entry.id.key();
        }
        entries.push_back(&entry);
    }

    const Log_chunk_ID & get_tbegin() const { return t_begin; }
    const Node_ID & get_NodeID() const { return node_id; }
    Node * get_Node() const { return node; }
    Log_chunk_ID get_node_prev_chunk_id() { return node_prev_chunk_id; }
    Log_chunk_ID get_node_next_chunk_id() { return node_next_chunk_id; }
    Log_chunk * get_node_prev_chunk() { return node_prev_chunk; }
    Log_chunk * get_node_next_chunk() { return node_next_chunk; }
    Log_entry_ID_key & get_first_entry() { return first_entry; }
    std::vector<Log_entry *> & get_entries() { return entries; }
    std::time_t get_open_time() const { return t_begin.get_epoch_time(); }
    std::time_t get_close_time() const { return t_close; }
    void set_close_time(std::time_t _tclose) { t_close = _tclose; }

    // helper functions
    const Log_chunk_ID_key & get_tbegin_key() const { return t_begin.idkey; }
    std::string get_tbegin_str() const { return t_begin.str(); }
    std::time_t duration_seconds() const {
        if (t_close<0)
            return -1;
        return t_close - get_open_time();
    }
    int duration_minutes() const {
        if (t_close<0)
            return -1;
        return duration_seconds() / 60;
    }
};

/**
 * ### Log entries (map)
 * 
 * A map of smart pointers to Log entries, referenced by Log entry ID key.
 * This is how entries are connected in the Log data structure.
 * 
 * Consecutive entries (ordered by ID) are the primary records of the Log,
 * as stored in database format.
 */
typedef std::map<const Log_entry_ID_key,std::unique_ptr<Log_entry>> Log_entries_Map;

/// Interval type for the Log_entries_Map;
typedef std::pair<Log_entries_Map::iterator, Log_entries_Map::iterator> Log_entry_iterator_interval;

/// Short-hand for this container type.
typedef std::deque<std::unique_ptr<Log_chunk>> Log_chunk_ptr_deque;

/**
 * ### Log chunks (deque list)
 * 
 * A deque list of smart pointers to Log chunks.
 * This is how chunks are connected in the Log data structure.
 */
struct Log_chunks_Deque: public Log_chunk_ptr_deque {

    /// Get reference to ID key of Log chunk by index. Throws an @exception if out of range.
    const Log_chunk_ID_key & get_tbegin_key(Log_chunk_ptr_deque::size_type idx) const {
        return at(idx)->get_tbegin_key();
    }
    /// Get pointer to Log chunk by index. Returns nullptr if out of range.
    Log_chunk * get_chunk(Log_chunk_ptr_deque::size_type idx) const {
        if (idx>=size())
            return nullptr;

        return at(idx).get();
    }
    /// Get index of Log chunk pointer by ID key. Returns size() if not found.
    Log_chunk_ptr_deque::size_type find(const Log_chunk_ID_key chunk_id) const;

    // friend functions
    /// Sum of durations of Log chunks.
    friend unsigned long Chunks_total_minutes(Log_chunks_Deque & chunks);
};

/// Interval types for the Log_chunks_Deque.
typedef std::pair<Log_chunk_ptr_deque::size_type, Log_chunk_ptr_deque::size_type> Log_chunk_index_interval;
typedef std::pair<Log_chunk_ID_key, Log_chunk_ID_key> Log_chunk_ID_interval;

/// Short-hand for this container type.
typedef std::deque<Log_chunk_ID_key> Log_chunk_ID_key_deque;

/**
 * ### Log Breakpoints (section starts)
 * 
 * This table exists for backward compatibility with the dil2al data storage format.
 * It contains a list of Log chunk IDs that indicate the start of each Task Log file
 * in that storage format.
 */
class Log_Breakpoints: protected Log_chunk_ID_key_deque {
    friend class Log;
public:
    void add_earlier_Breakpoint(Log_chunk & chunk) { push_front(chunk.get_tbegin_key()); }
    void add_later_Breakpoint(Log_chunk & chunk) { push_back(chunk.get_tbegin_key()); }
    //Log_chunk_ID_key_deque & get_chunk_ids() { return breakpoint_ids; }
    Log_chunk_ID_key & get_chunk_id_key(Log_chunk_ID_key_deque::size_type idx) { return at(idx); }

    // helper functions
    std::string get_chunk_id_str(Log_chunk_ID_key_deque::size_type idx) { return Log_chunk_ID_TimeStamp_to_string( at(idx).idT ); }
    std::string get_Ymd_str(Log_chunk_ID_key_deque::size_type idx) { return Log_TimeStamp_to_Ymd_string( at(idx).idT ); }
};

/**
 * ### Log
 * 
 * This data structure is used to organize Log entry elements into
 * Log chunk intervals and, for backwards compatibility with `dil2al`,
 * to indicate which Log chunks follow a Breakpoint and start a new
 * section.
 * 
 * Note that sections are not used with the database storage format.
 * Conversion back to `dil2al` Task Log files will need to add new
 * Breakpoints at sensible intervals beyond the last original
 * section Breakpoint.
 * 
 * *** At this time, the functions below have only been tested in the
 *     case where the whole Log is in-memory. Care will need to be
 *     taken to make sure things work as intended when only portions
 *     of the Log are loaded into memory. Alternatively, a separate
 *     but similar class could be defined in order to avoid any
 *     confusion.
 *     Or, the functions that find/get specific chunks or entries
 *     could be given some smarts where *not found* first leads to
 *     an attempt to find and load from database.
 *     In that case, there will also have to be some process behind
 *     the scenes that can gradually release in-memory Log portions
 *     again, otherwise it will always end up being the whole Log
 *     when a server is an active process for a long time.
 *     Log requests could be handled by a separate server, which
 *     could be explicitly *not* daemonized, so that it will always
 *     release memory after serving up some Log data.
 *     (See the Trello card at https://trello.com/c/EppSyY9Y.)
 */
class Log {
protected:
    Log_entries_Map entries;
    Log_chunks_Deque chunks;
    Log_Breakpoints breakpoints;
public:
    Log_entries_Map::size_type num_Entries() const { return entries.size(); }
    Log_chunks_Deque::size_type num_Chunks() const { return chunks.size(); }
    Log_chunk_ID_key_deque::size_type num_Breakpoints() const { return breakpoints.size(); }
    Log_entries_Map & get_Entries() { return entries; }
    Log_chunks_Deque & get_Chunks() { return chunks; }
    Log_Breakpoints & get_Breakpoints() { return breakpoints; }
    void add_earlier_Chunk(const Log_TimeStamp &_tbegin, const Node_ID &_nodeid, std::time_t _tclose) { chunks.push_front(std::make_unique<Log_chunk>(_tbegin,_nodeid,_tclose)); }
    void add_later_Chunk(const Log_TimeStamp &_tbegin, const Node_ID &_nodeid, std::time_t _tclose) { chunks.push_back(std::make_unique<Log_chunk>(_tbegin,_nodeid, _tclose)); }
    std::deque<Log_chunk *> get_Node_chunks_fullparse(const Node_ID node_id);
    std::deque<Log_chunk *> get_Node_chunks(const Node_ID node_id); /// This version requires valid prev/next references.
    void setup_Chunk_nodeprevnext(); /// Call this after loading chunks into the Log.

    // friend functions
    friend std::vector<Log_chunks_Deque::size_type> Breakpoint_Indices(Log & log);
    friend std::vector<Log_chunks_Deque::size_type> Chunks_per_Breakpoint(Log & log);
 
    // helper functions
    Log_chunk * get_chunk(Log_chunk_ptr_deque::size_type idx) const { return chunks.get_chunk(idx); }
    Log_chunk_ptr_deque::size_type find_chunk_by_key(const Log_chunk_ID_key chunk_idkey) const { return chunks.find(chunk_idkey); }
    Log_chunk_ID_key & get_Breakpoint_first_chunk_id_key(Log_chunk_ID_key_deque::size_type idx) { return breakpoints.at(idx); }
    std::string get_Breakpoint_first_chunk_id_str(Log_chunk_ID_key_deque::size_type idx) { return Log_chunk_ID_TimeStamp_to_string( breakpoints.at(idx).idT ); }
    std::string get_Breakpoint_Ymd_str(Log_chunk_ID_key_deque::size_type idx) { return Log_TimeStamp_to_Ymd_string( breakpoints.at(idx).idT ); }
    Log_chunk_ptr_deque::size_type find_Breakpoint_first_chunk(Log_chunk_ID_key_deque::size_type idx) { return find_chunk_by_key(get_Breakpoint_first_chunk_id_key(idx)); }
    
    Log_entry_iterator_interval get_Entries_interval(const Log_entry_ID_key interval_front, const Log_entry_ID_key interval_back);
    Log_entry_iterator_interval get_Entries_interval(const Log_entry_ID_key interval_front, unsigned long n);
    Log_chunk_ID_interval get_Chunks_ID_t_interval(std::time_t t_from, std::time_t t_before);
    Log_chunk_ID_interval get_Chunks_ID_n_interval(std::time_t t_from, unsigned long n);
    Log_chunk_index_interval get_Chunks_index_t_interval(std::time_t t_from, std::time_t t_before);
    Log_chunk_index_interval get_Chunks_index_n_interval(std::time_t t_from, unsigned long n);

    //*** Can define a helper function to `refresh_Chunk_entries(Log_chunk_ID_key_deque::size_type idx)`
    //*** in case the rapid-access vector was not initialized or was corrupted.
};

unsigned long Entries_total_text(Log_entries_Map & entries);

} // namespace fz

#endif // __LOGTYPES_HPP