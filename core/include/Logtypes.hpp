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
#include "LogtypesID.hpp"

namespace fz {

//class Log_TimeStamp;
class Log;
class Log_data;
class Log_entry;
class Log_chunk;

/// Log data base class providing shared parameters and functions.
class Log_data {
    friend class Log;
    // We may move some things in here later. Right now, this exists mostly to
    // provide some additional support to the `log_chain_target` struct.
};

/// Base class for Log components that are chainable as belonging to a Node.
class Log_by_Node_chainable {
    friend class Log;
protected:
    Log_chain_target node_prev;   /// Previous in chain-by-Node.
    Log_chain_target node_next;   /// Next in chain-by-Node.

public:
    Log_by_Node_chainable() {}
    Log_by_Node_chainable(const Log_chain_target & _prev, const Log_chain_target & _next): node_prev(_prev), node_next(_next) {}
    Log_by_Node_chainable(bool previschunk, const Log_TimeStamp & _prev, bool nextischunk, const Log_TimeStamp & _next): node_prev(_prev,previschunk), node_next(_next,nextischunk) {}

    bool node_prev_isnullptr() { return node_prev.isnulltarget_byptr(); }
    bool node_next_isnullptr() { return node_next.isnulltarget_byptr(); }
    const Log_chain_target & get_node_prev() const { return node_prev; }
    const Log_chain_target & get_node_next() const { return node_next; }
    Log_ptr_pair get_node_prev_ptr() { return node_prev.get_data_ptr(); }
    Log_ptr_pair get_node_next_ptr() { return node_next.get_data_ptr(); }

    /// Set functions that (each) set BOTH target .ptr and .key values
    void set_Node_prev_null() { node_prev.set_to_nulltarget(); }
    void set_Node_next_null() { node_next.set_to_nulltarget(); }
    void set_Node_prev(Log_chain_target _prev) { node_prev = _prev; }
    void set_Node_next(Log_chain_target _next) { node_next = _next; }
    void set_Node_prev_ptr(Log_chunk * prevptr);
    void set_Node_prev_ptr(Log_entry * prevptr);
    void set_Node_next_ptr(Log_chunk * nextptr);
    void set_Node_next_ptr(Log_entry * nextptr);

};

class Log_entry: public Log_by_Node_chainable, public Log_data {
    friend class Log;
    friend class Log_chunk;
protected:
    const Log_entry_ID id;        /// The id that extends the chunk time stamp.
    const Node_ID_key node_idkey; /// Reference to the Node this belongs to or zero-stamp.
    std::string entrytext;        /// Text content (typically in HTML).

    // The following are maintained for rapid access where possible.
    Node *node = nullptr;
    Log_chunk *chunk;

public:
    Log_entry(const Log_TimeStamp &_id, std::string _entrytext, const Node_ID_key &_nodeidkey, Log_chunk * _chunk = NULL): id(_id), node_idkey(_nodeidkey), entrytext(_entrytext), node(nullptr), chunk(_chunk) {
        // Notice that we are not immediately attempting to set the `node` rapid-access cache that
        // would correspond with `node_idkey`. That is, beause we cannot be certain that this object
        // is being created within a context where the corresponding Node object actually ecists and
        // a pointer to it is identifiable. The rapid-access pointer must be set explicitly via
        // `set_Node_rapid_access()`, which can also be called by the parametrized overload of the
        // `get_Node()` member function.
    }
    Log_entry(const Log_TimeStamp &_id, std::string _entrytext, Log_chunk * _chunk = NULL): id(_id), entrytext(_entrytext), node(nullptr), chunk(_chunk) {}
    Log_entry(const Log_TimeStamp &_id, std::string _entrytext, const Node_ID_key &_nodeidkey, bool previschunk, const Log_TimeStamp & _prev, bool nextischunk, const Log_TimeStamp & _next, Log_chunk * _chunk = NULL) : id(_id), node_idkey(_nodeidkey), entrytext(_entrytext), node(nullptr), chunk(_chunk) {}
    Log_entry(const Log_TimeStamp &_id, std::string _entrytext, bool previschunk, const Log_TimeStamp & _prev, bool nextischunk, const Log_TimeStamp & _next, Log_chunk * _chunk = NULL): Log_by_Node_chainable(previschunk,_prev,nextischunk,_next), id(_id), entrytext(_entrytext), node(nullptr), chunk(_chunk) {}

    /// Set Node pointer to the same Node as node_idkey if it was not set during construction.
    bool set_Node_rapid_access(Node & _node); // inlined below
    void set_Chunk(Log_chunk * _chunk) { chunk = _chunk; }

    const Log_entry_ID & get_id() const { return id; }
    uint8_t get_minor_id() const { return id.idkey.idT.minor_id; }
    const Node_ID_key & get_nodeidkey() const { return node_idkey; }
    std::string & get_entrytext() { return entrytext; }
    Node * get_Node() { return node; }
    Node * get_Node(Graph & graph); // inlined below
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
class Log_chunk: public Log_by_Node_chainable, public Log_data {
protected:
    // These three must be provided when the object is created.
    const Log_chunk_ID t_begin;   /// The time stamp when a Log chunk begins.
    const Node_ID node_id;        /// The Node to which the Log chunk belongs.
    std::time_t t_close;          /// The time when a Log chunk was closed (-1 if not closed).
    Log_entry_ID_key first_entry; /// ID of the first Log_entry in the chunk (once created).

    // The following are maintained for rapid access where possible.
    Node *node;
    std::vector<Log_entry *> entries;

public:
    Log_chunk(const Log_TimeStamp &_tbegin, const Node_ID &_nodeid, std::time_t _tclose) : t_begin(_tbegin), node_id(_nodeid), t_close(_tclose), node(NULL) {}
    Log_chunk(const Log_TimeStamp &_tbegin, Node &_node, std::time_t _tclose): t_begin(_tbegin), node_id(_node.get_id()), t_close(_tclose), node(&_node) {}
    Log_chunk(const Log_TimeStamp &_tbegin, const Node_ID &_nodeid, std::time_t _tclose, bool previschunk, const Log_TimeStamp & _prev, bool nextischunk, const Log_TimeStamp & _next) : Log_by_Node_chainable(previschunk,_prev,nextischunk,_next), t_begin(_tbegin), node_id(_nodeid), t_close(_tclose), node(NULL) {}
    Log_chunk(const Log_TimeStamp &_tbegin, Node &_node, std::time_t _tclose, bool previschunk, const Log_TimeStamp & _prev, bool nextischunk, const Log_TimeStamp & _next): Log_by_Node_chainable(previschunk,_prev,nextischunk,_next), t_begin(_tbegin), node_id(_node.get_id()), t_close(_tclose), node(&_node) {}

    /// rapid-access setup
    bool set_Node_rapid_access(Node & _node); // inlined below

    /// tables references
    std::vector<Log_entry *> & get_entries() { return entries; }

    /// entries table: extend
    void add_Entry(Log_entry &entry); // inlined below

    /// safely inspect data
    const Log_chunk_ID & get_tbegin() const { return t_begin; }
    const Node_ID & get_NodeID() const { return node_id; }
    Node * get_Node() const { return node; }
    Node * get_Node(Graph & graph); // inlined below
    const Log_chunk_ID_key & get_tbegin_key() const { return t_begin.idkey; }
    const Log_TimeStamp & get_tbegin_idT() const { return t_begin.idkey.idT; }
    std::string get_tbegin_str() const { return t_begin.str(); } /// Log_chunk ID string
    std::time_t get_open_time() const { return t_begin.get_epoch_time(); }
    std::time_t get_close_time() const { return t_close; }
    Log_entry_ID_key & get_first_entry() { return first_entry; }

    /// change parameters
    void set_close_time(std::time_t _tclose) { t_close = _tclose; }

    // helper (utility) functions
    std::time_t duration_seconds() const; // inline below
    int duration_minutes() const; // inline below

    /// friend (utility) functions
    friend Topic * main_topic(Graph & _graph, Log_chunk & chunk);
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
    const Log_chunk_ID_key & get_tbegin_key(Log_chunk_ptr_deque::size_type idx) const;

    /// Get a copy of the ID key of Log chunk by index, or null-key if out of range.
    Log_chunk_ID_key get_key_copy(Log_chunk_ptr_deque::size_type idx) const;

    /// Get reference to TimeStamp of Log chunk by index. Throws an @exception if out of range.
    const Log_TimeStamp & get_tbegin_idT(Log_chunk_ptr_deque::size_type idx) const;

    /// Get pointer to Log chunk by index. Returns nullptr if out of range.
    Log_chunk * get_chunk(Log_chunk_ptr_deque::size_type idx) const;

    /// Find index of Log chunk pointer by ID key by brute force sequential search. Returns size() if not found.
    std::pair<Log_chunk_ptr_deque::size_type, Log_chunk*> Log_chunks_Deque::slow_find(const Log_chunk_ID_key chunk_id) const;

    /// Fast search for index and pointer to Log chunk by ID key. Returns [size, nullptr] if not found.
    std::pair<Log_chunk_ptr_deque::size_type, Log_chunk*> Log_chunks_Deque::find_index_and_pointer(const Log_chunk_ID_key chunk_id) const;

    /// Get index of Log chunk pointer by ID key. Returns size() if not found.
    Log_chunk_ptr_deque::size_type find(const Log_chunk_ID_key chunk_id) const;

    /// Get pointer to Log chunk by ID key. Returns nullptr if not found.
    Log_chunk * get_chunk(const Log_chunk_ID_key chunk_id) const;

    /// Find index of Log chunk by its ID closest to time t.
    Log_chunk_ptr_deque::size_type find(std::time_t t, bool later) const;

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
    // breakpoints table: extend
    void add_earlier_Breakpoint(Log_chunk & chunk) { push_front(chunk.get_tbegin_key()); }
    void add_later_Breakpoint(Log_chunk & chunk) { push_back(chunk.get_tbegin_key()); }

    // breakpoints table: get breakpoint
    Log_chunk_ID_key & get_chunk_id_key(Log_chunk_ID_key_deque::size_type idx) { return at(idx); }
    std::string get_chunk_id_str(Log_chunk_ID_key_deque::size_type idx) { return Log_chunk_ID_TimeStamp_to_string( at(idx).idT ); }
    std::string get_Ymd_str(Log_chunk_ID_key_deque::size_type idx) { return Log_TimeStamp_to_Ymd_string( at(idx).idT ); }

    // crossref tables: breakpoints x chunks
    const Log_chunk_ID_key & find_Breakpoint_tstamp_before_chunk(const Log_chunk_ID_key key);
    Log_chunk_ID_key_deque::size_type find_Breakpoint_index_before_chunk(const Log_chunk_ID_key key);
    Log_chunk_ID_key_deque::size_type find_Breakpoint_index_before_entry(const Log_entry_ID_key key);
    Log_chunk_ID_key_deque::size_type find_Breakpoint_index_before_chaintarget(const Log_chain_target & chaintarget);
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
    /// finalizing setup
    void setup_Chain_nodeprevnext(); /// Call this after loading chunks and enties into the Log.
    void setup_Entry_node_caches(Graph & graph); /// Call this after loading entries into the Log with valid Graph.
    void setup_Chunk_node_caches(Graph & graph); /// Call this after loading entries into the Log with valid Graph.

    /// tables: sizes
    Log_entries_Map::size_type num_Entries() const { return entries.size(); }
    Log_chunks_Deque::size_type num_Chunks() const { return chunks.size(); }
    Log_chunk_ID_key_deque::size_type num_Breakpoints() const { return breakpoints.size(); }

    /// tables: references
    Log_entries_Map & get_Entries() { return entries; }
    Log_chunks_Deque & get_Chunks() { return chunks; }
    Log_Breakpoints & get_Breakpoints() { return breakpoints; }

    /// chunks table: extend
    void add_earlier_Chunk(const Log_TimeStamp &_tbegin, const Node_ID &_nodeid, std::time_t _tclose) { chunks.push_front(std::make_unique<Log_chunk>(_tbegin,_nodeid,_tclose)); }
    void add_later_Chunk(const Log_TimeStamp &_tbegin, const Node_ID &_nodeid, std::time_t _tclose) { chunks.push_back(std::make_unique<Log_chunk>(_tbegin,_nodeid, _tclose)); }

    /// chunks table: get chunk
    Log_chunk * get_chunk(Log_chunk_ptr_deque::size_type idx) const { return chunks.get_chunk(idx); }
    Log_chunk * get_chunk(const Log_chunk_ID_key chunk_idkey) const { return chunks.get_chunk(chunk_idkey); }
    Log_chunk_ID_key get_chunk_id_key(Log_chunk_ptr_deque::size_type idx) const { return chunks.get_key_copy(idx); }
    Log_chunk_ptr_deque::size_type find_chunk_by_key(const Log_chunk_ID_key chunk_idkey) const { return chunks.find(chunk_idkey); }
    std::pair<Log_chunk_ptr_deque::size_type, Log_chunk*> find_chunk_index_and_pointer(const Log_chunk_ID_key chunk_id) const { return chunks.find_index_and_pointer(chunk_id); }

    /// breakpoints table: get breakpoint
    Log_chunk_ID_key & get_Breakpoint_first_chunk_id_key(Log_chunk_ID_key_deque::size_type idx) { return breakpoints.at(idx); }
    std::string get_Breakpoint_first_chunk_id_str(Log_chunk_ID_key_deque::size_type idx) { return Log_chunk_ID_TimeStamp_to_string( breakpoints.at(idx).idT ); }
    std::string get_Breakpoint_Ymd_str(Log_chunk_ID_key_deque::size_type idx) { return Log_TimeStamp_to_Ymd_string( breakpoints.at(idx).idT ); }

    /// crossref tabless: chunks x breakpoints
    Log_chunk_ptr_deque::size_type get_chunk_first_at_Breakpoint(Log_chunk_ID_key_deque::size_type idx) { return find_chunk_by_key(get_Breakpoint_first_chunk_id_key(idx)); }

    /// crossref tables: breakpoints x chunks
    const Log_chunk_ID_key & find_Breakpoint_tstamp_before_chunk(const Log_chunk_ID_key key) { return breakpoints.find_Breakpoint_tstamp_before_chunk(key); }
    Log_chunk_ID_key_deque::size_type find_Breakpoint_index_before_chunk(const Log_chunk_ID_key key) { return breakpoints.find_Breakpoint_index_before_chunk(key); }
    Log_chunk_ID_key_deque::size_type find_Breakpoint_index_before_entry(const Log_entry_ID_key key) { return breakpoints.find_Breakpoint_index_before_entry(key); }
    Log_chunk_ID_key_deque::size_type find_Breakpoint_index_before_chaintarget(const Log_chain_target & chaintarget) { return breakpoints.find_Breakpoint_index_before_chaintarget(chaintarget); }
    
    /// entries table: select interval, from chunk / time, to chunk / time / count 
    Log_entry_iterator_interval get_Entries_interval(const Log_entry_ID_key interval_front, const Log_entry_ID_key interval_back);
    Log_entry_iterator_interval get_Entries_t_interval(std::time_t t_from, std::time_t t_before);
    Log_entry_iterator_interval get_Entries_interval(const Log_entry_ID_key interval_front, unsigned long n);
    Log_entry_iterator_interval get_Entries_n_interval(std::time_t t_from, unsigned long n);

    /// chunks table: select interval, from chunk / time, to chunk / time / count
    Log_chunk_ID_interval get_Chunks_ID_t_interval(std::time_t t_from, std::time_t t_before);
    Log_chunk_ID_interval get_Chunks_ID_n_interval(std::time_t t_from, unsigned long n);
    Log_chunk_index_interval get_Chunks_index_t_interval(std::time_t t_from, std::time_t t_before);
    Log_chunk_index_interval get_Chunks_index_n_interval(std::time_t t_from, unsigned long n);

    /// chunks table: select subset by Node
    std::deque<Log_chain_target> get_Node_chain_fullparse(const Node_ID node_id, bool onlyfirst = false);
    std::deque<Log_chain_target> get_Node_chain(const Node_ID node_id); /// This version requires valid prev/next references.

    /// helper (utility) functions
    std::time_t oldest_chunk_t() { return (num_Chunks()>0) ? chunks.front()->get_open_time() : RTt_unspecified; }
    std::time_t newest_chunk_t() { return (num_Chunks()>0) ? chunks.back()->get_open_time() : RTt_unspecified; }

    // friend functions
    friend std::vector<Log_chunks_Deque::size_type> Breakpoint_Indices(Log & log);
    friend std::vector<Log_chunks_Deque::size_type> Chunks_per_Breakpoint(Log & log);
    friend unsigned long Log_span_in_seconds(Log & log);
    friend double Log_span_in_days(Log & log);
    friend ymd_tuple Log_span_years_months_days(Log & log);
  
    //*** Can define a helper function to `refresh_Chunk_entries(Log_chunk_ID_key_deque::size_type idx)`
    //*** in case the rapid-access vector was not initialized or was corrupted.
};

// +----- begin: inline functions -----+

/// Set rapid-access node pointer. (See detailed description for Log_chunk below.)
inline bool Log_entry::set_Node_rapid_access(Node & _node) {
    if (!(node_idkey.idT == _node.get_id().key().idT)) {
        return false;
    }

    node = &_node;
    return true;
}

inline Node * Log_entry::get_Node(Graph & graph) {
    if (!node)
        node = graph.Node_by_id(node_idkey); // could still be nullptr!

    return node;
}

/**
 * Set rapid-access node pointer to the same Node as the one indicated by
 * node_id.key(), e.g. if that was not set during Log_chunk object construction.
 * 
 * Note: This function needs a valid Node reference to assign, but it will
 *       only accept one that matches node_id.key().
 * 
 * @param _node reference that must match node_id.key().
 * @return true if successful, false if there was a mismatch.
 */
inline bool Log_chunk::set_Node_rapid_access(Node & _node) { /// 
    if (!(node_id.key().idT == _node.get_id().key().idT))
        return false;

    node = &_node;
    return true;
}

/**
 * Extend the Log chunk entries list a Log entry. The entry is appended,
 * and if it is the first one in the list then its ID key is copied to
 * `first_entry`.
 * 
 * @param entry is a new Log entry record.
 */
inline void Log_chunk::add_Entry(Log_entry &entry) {
    if (entries.empty()) {
        first_entry = entry.id.key();
    }
    entries.push_back(&entry);
}

/**
 * Find a Node in the Graph that has Node ID with key matching
 * node_id.key().
 * 
 * Note: If the rapid-access node pointer is set then that
 *       is returned.
 * 
 * @param graph is a valid Graph.
 * @return pointer to the Node (or nullptr if not found).
 */
inline Node * Log_chunk::get_Node(Graph & graph) {
    if (!node)
        node = graph.Node_by_id(node_id.key());

    return node;
}

/**
 * Report the duration of the Log chunk in seconds.
 * 
 * @return duration in seconds (or -1 if the chunk is still open).
 */
inline std::time_t Log_chunk::duration_seconds() const {
    if (t_close < 0)
        return -1;

    return t_close - get_open_time();
}

/**
 * Report the duration of the Log chunk in minutes.
 * 
 * @return duration in minutes (or -1 if the chunk is still open).
 */
inline int Log_chunk::duration_minutes() const {
    if (t_close < 0)
        return -1;

    return duration_seconds() / 60;
}

/// Get reference to ID key of Log chunk by index. Throws an @exception if out of range.
inline const Log_chunk_ID_key & Log_chunks_Deque::get_tbegin_key(Log_chunk_ptr_deque::size_type idx) const {
    return at(idx)->get_tbegin_key();
}

/// Get a copy of the ID key of Log chunk by index, or null-key if out of range.
inline Log_chunk_ID_key Log_chunks_Deque::get_key_copy(Log_chunk_ptr_deque::size_type idx) const {
    if (idx>=size())
        return Log_chunk_ID_key(); // return a null-key

    return at(idx)->get_tbegin_key();
}

/// Get reference to TimeStamp of Log chunk by index. Throws an @exception if out of range.
inline const Log_TimeStamp & Log_chunks_Deque::get_tbegin_idT(Log_chunk_ptr_deque::size_type idx) const {
    return at(idx)->get_tbegin_idT();
}

/// Get pointer to Log chunk by index. Returns nullptr if out of range.
inline Log_chunk * Log_chunks_Deque::get_chunk(Log_chunk_ptr_deque::size_type idx) const {
    if (idx>=size())
        return nullptr;

    return at(idx).get();
}

// +----- end  : inline functions -----+

unsigned long Entries_total_text(Log_entries_Map & entries);

} // namespace fz

#endif // __LOGTYPES_HPP