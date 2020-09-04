// Copyright 2020 Randal A. Koene
// License TBD

#include <cstdint>
#include <iomanip>
#include <numeric>

#include "LogtypesID.hpp"
#include "Logtypes.hpp"

namespace fz {

/**
 * Set the previous in chain-by-Node based on a Log chunk pointer.
 * 
 * This assumes that you _know_ that the pointer is correct and that it is
 * indeed a chunk and not an entry. No confirmations are performed here.
 * The target is set accordingly, both ID key and rapid-access pointer.
 * 
 * A nullptr can be given to clear (by)`node_prev`.
 * 
 * @param prevptr a valid pointer to previous chunk in the chain (or nullptr).
 */
void Log_by_Node_chainable::set_Node_prev_ptr(Log_chunk * prevptr) { // give nullptr to clear
    if (prevptr!=this) {
        node_prev.ischunk = true;
        node_prev.chunk.ptr = prevptr;
        if (prevptr!=nullptr) {
            node_prev.chunk.key = *(const_cast<Log_chunk_ID_key *>(&(prevptr->get_tbegin_key())));
        } else {
            node_prev.chunk.key = Log_chunk_ID_key();
        }
    }
}

/// See the description of Log_chunk::set_Node_prev_ptr.
void Log_by_Node_chainable::set_Node_next_ptr(Log_chunk * nextptr) { // give nullptr to clear
    if (nextptr!=this) {
        node_next.ischunk = true;
        node_next.chunk.ptr = nextptr;
        if (nextptr!=nullptr) {
            node_next.chunk.key = *(const_cast<Log_chunk_ID_key *>(&(nextptr->get_tbegin_key())));
        } else {
            node_next.chunk.key = Log_chunk_ID_key();
        }
    }
}

void Log_by_Node_chainable::set_Node_prev_ptr(Log_entry * prevptr) { // give nullptr to clear
    if ((Log_chunk*)prevptr!=this) {
        node_prev.ischunk = false;
        node_prev.entry.ptr = prevptr;
        if (prevptr!=nullptr) {
            node_prev.entry.key = *(const_cast<Log_entry_ID_key *>(&(prevptr->get_id_key())));
        } else {
            node_prev.entry.key = Log_entry_ID_key();
        }
    }
}

/// See the description of Log_chunk::set_Node_prev_ptr.
void Log_by_Node_chainable::set_Node_next_ptr(Log_entry * nextptr) { // give nullptr to clear
    if ((Log_chunk*)nextptr!=this) {
        node_next.ischunk = false;
        node_next.entry.ptr = nextptr;
        if (nextptr!=nullptr) {
            node_next.entry.key = *(const_cast<Log_entry_ID_key *>(&(nextptr->get_id_key())));
        } else {
            node_next.entry.key = Log_entry_ID_key();
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
    for (Log_chunk_ID_key_deque::size_type i = 0; i < size(); ++i) {
        if (key < at(i)) {
            return (i>0) ? (i-1) : 0;
        }
    }
    return size()-1;
}

/**
 * Find the Breakpoint section to which the Log entry with the given
 * ID key belongs.
 * 
 * Log entries don't have their own start times to compare directly with
 * a Breakpoint. We also don't want to assume (just for this function)
 * that a Graph is available in order to actually check the start time
 * of the corresponding chunk. Instead, we use a trick to use the
 * Entry ID key to retrieve a Log time stamp, and convert that into an
 * assumed Log Chunk ID key. That is then used to find the section.
 * 
 * Note: This trick does impose some requirements that are noted on
 *       the Trello card at https://trello.com/c/BB3QjWOG.
 * 
 * @param key is the Log entry ID key.
 * @return the Breakpoint index in the table of breakpoints.
 */
Log_chunk_ID_key_deque::size_type Log_Breakpoints::find_Breakpoint_index_before_entry(const Log_entry_ID_key key) {
    Log_TimeStamp entrytime = key.idT;
    entrytime.minor_id = 0;
    Log_chunk_ID_key assumed_chunk_key(entrytime);
    return find_Breakpoint_index_before_chunk(assumed_chunk_key);
}

Log_chunk_ID_key_deque::size_type Log_Breakpoints::find_Breakpoint_index_before_chaintarget(const Log_chain_target & chaintarget) {
    if (chaintarget.ischunk) {
        return find_Breakpoint_index_before_chunk(chaintarget.chunk.key);
    } else {
        return find_Breakpoint_index_before_entry(chaintarget.entry.key);
    }
}

/**
 * Parse all chunks connected to the Log and find the sequence of
 * chunks and entries that belongs to a specific Node.
 * 
 * This is used by Log::setup_Chunk_nodeprevnext(), and it can also
 * be used independent of the reference parameters in the Log_chunk
 * objects, which even works for arbitrary loaded lists of Log chunks.
 * 
 * This function is also used by the rapid search version `get_Node_targets()`,
 * which calls this to find the first target, the head of the chain,
 * which is specifeid via the `onlyfirst` parameter.
 * 
 * This function requires that the `entries` list has been set up
 * for all Log chunks.
 * 
 * @param node_id of the Node for which to collect its Log chain (history).
 * @param onlyfirst flag if true return only the first target.
 * @return a deque sorted list of chain targets found.
 */
std::deque<Log_chain_target> Log::get_Node_chain_fullparse(const Node_ID node_id, bool onlyfirst) {
    std::deque<Log_chain_target> res;

    for (auto it = chunks.begin(); it<chunks.end(); ++it) {

        // check the chunk first
        if ((*it)->get_NodeID().key().idT == node_id.key().idT) {
            res.emplace_back((*it)->get_tbegin_key(),it->get());
            if (onlyfirst)
                return res;
        }

        // then check its entries
        for (const auto& e : (*it)->get_entries()) {
            if (!(e->get_nodeidkey().isnullkey())) {
                if (e->get_nodeidkey() == node_id.key()) {
                    res.emplace_back(e->get_id_key(),e);
                    if (onlyfirst)
                        return res;
                }
            }
        }

    }

    return res;
}

/**
 * Quickly walk through the reference chain that belongs to the
 * specified Node and return all of its Log chunks and Log entries.
 * 
 * Note A: This function depends on valid rapid-access pointers
 *         within `node_prev` and `node_next` variables in each
 *         Log chunk and entry.
 * Note B: This function (presently) works by building a brand-new
 *         deque list with Log_chain_target copies (instead of
 *         references). This decision is based on the assumption that
 *         the list of targets is typically informative, providing
 *         necessary access info to those targets, but you don't
 *         accidentally want to break existing chains by modifying
 *         elements of the deque list.
 * 
 * @param node_id of the Node for which to collect its Log chain (history).
 * @return a deque sorted list of chain targets found.
 */
std::deque<Log_chain_target> Log::get_Node_chain(const Node_ID node_id) {
    std::deque<Log_chain_target> res = get_Node_chain_fullparse(node_id,true);

    if (res.empty())
        return res;

    for (const Log_chain_target * next = res.front().next_in_chain(); next != nullptr; next = next->next_in_chain()) {
        res.emplace_back(*next);
    }

    return res;
}

/**
 * This structure is used by `Log::setup_Chunk_nodeprevnext()` to build a proper
 * chain.
 * 
 * This is all done using references by ID within the Log data structure. No
 * Graph object is required.
 */
struct Node_Targets_cursor {
    Log_chain_target head;
    Log_chain_target tail;
    unsigned long count;

    Node_Targets_cursor(Log_chunk & tailhead): head(tailhead), tail(tailhead), count(1) {
        tailhead.set_Node_next_null(); // clear in case previously attached
        tailhead.set_Node_prev_null(); // clear in case previously attached
    }
    Node_Targets_cursor(Log_entry & tailhead): head(tailhead), tail(tailhead), count(1) {
        tailhead.set_Node_next_null(); // clear in case previously attached
        tailhead.set_Node_prev_null(); // clear in case previously attached
    }

    bool append(Log_chunk & chunk) {
        if (tail.same_target(chunk) || (!chunk.node_prev_isnullptr()))
            ERRRETURNFALSE(__func__,"unable to append a Log chunk that was already chained");

        chunk.set_Node_next_null(); // clearing any old attachements as we go

        // Link into the chain
        if (!tail.isnulltarget_byptr()) { // testing just in case (ought never to happen)
            chunk.set_Node_prev(tail);
            tail.bytargetptr_set_Node_next_ptr(&chunk);
        }

        count++;

        // New tail of chain
        tail.set_chunk_target(chunk);
        return true;
    }

    bool append(Log_entry & entry) {
        if (tail.same_target(entry) || (!entry.node_prev_isnullptr()))
            ERRRETURNFALSE(__func__,"unable to append a Log entry that was already chained");

        entry.set_Node_next_null(); // clearing any old attachements as we go

        // Link into the chain
        if (!tail.isnulltarget_byptr()) { // testing just in case (ought never to happen)
            entry.set_Node_prev(tail);
            tail.bytargetptr_set_Node_next_ptr(&entry);
        }

        count++;

        // New tail of chain
        tail.set_entry_target(entry);
        return true;
    }
};

/**
 * Parse the deque list of Log chunks, as well as their entries, and assign
 * all references in `node_prev`, `node_next`, and their rapid-access pointers.
 * 
 * This is all done using references by ID within the Log data structure. No
 * Graph object is required.
 */
void Log::setup_Chain_nodeprevnext() {

    //std::deque<Log_chain_target> res;

    std::map<Node_ID_key,Node_Targets_cursor> cursors;

    for (const auto& chunkptr : chunks) { // .begin(); it != chunks.end(); ++it) {

        // first, link the chunk to the right chain
        Log_chunk * chunk = chunkptr.get();
        if (!chunk) {
            ADDERROR(__func__,"Log chunk pointer is nullptr in chunks list (this should never happen!)");
            continue;
        } else {
            Node_Targets_cursor c_ntc(*chunk);
            auto [c_node_cursor_it, c_was_new] = cursors.emplace(chunk->get_NodeID().key(),c_ntc); // first of a Node
            if (!c_was_new) c_node_cursor_it->second.append(*chunk); // adding to a Node's chain

            // then, link entries in the chunk with specified Nodes to the right chains
            for (const auto& entry : chunkptr->get_entries()) {
                if (!entry) {
                    ADDERROR(__func__,"Log entry pointer is nullptr in chunk.get_entries (this should never happen!)");
                    continue;
                } else {
                    if (!(entry->get_nodeidkey().isnullkey())) {
                        Node_Targets_cursor e_ntc(*entry);
                        auto [e_node_cursor_it, e_was_new] = cursors.emplace(entry->get_nodeidkey(),e_ntc); // first of a Node
                        if (!e_was_new) e_node_cursor_it->second.append(*entry); // adding to a Node's chain
                    }
                }
            }
        }

    }
}

/**
 * Parse the map of Log entries and assign all `node` cache pointers
 * in acordance with Node objects found in the Graph.
 */
void Log::setup_Entry_node_caches(Graph & graph) {
    for (const auto& [entrykey, entryptr] : entries) {
        Log_entry * entry = entryptr.get();

        if (!entry) {
            ADDERROR(__func__,"null-entry in Graph.entries at entry with ID "+entrykey.str());
            continue;
        }

        const Node_ID_key & nodeIDkey = entry->get_nodeidkey();
        if (nodeIDkey.isnullkey()) {
            if (entry->get_Node() != nullptr) {
                ADDERROR(__func__,"non-null rapid-access cache while Log_entry.node_idkey is null-key at entry with ID "+entrykey.str());
                continue;
            }

        } else {
            Node * node = graph.Node_by_id(nodeIDkey);

            if (!node) {
                ADDERROR(__func__,"no Node found that matches ID "+nodeIDkey.str()+" at entry with ID "+entrykey.str());
                continue;
            }

            if (!entry->set_Node_rapid_access(*node)) {
                ADDERROR(__func__,"Log entry at ID "+entrykey.str()+" refused rapid-access Node pointer with ID "+node->get_id_str()+" (needed Node ID "+nodeIDkey.str()+')');
                continue;
            }
        }
    }
}

/**
 * Parse the deque of Log chunks and assign all `node` cache pointers
 * in acordance with Node objects found in the Graph.
 */
void Log::setup_Chunk_node_caches(Graph & graph) {
    for (const auto& chunkptr : chunks) {
        Log_chunk * chunk = chunkptr.get();

        if (!chunk) {
            ADDERROR(__func__,"null-chunk in Graph.chunks at chunk with ID "+chunk->get_tbegin_str());
            continue;
        }

        const Node_ID_key & nodeIDkey = chunk->get_NodeID().key();
        if (nodeIDkey.isnullkey()) {
            if (chunk->get_Node() != nullptr) {
                ADDERROR(__func__,"non-null rapid-access cache while Log_chunk.node_id has null-key at chunk with ID "+chunk->get_tbegin_str());
                continue;
            }

        } else {
            Node * node = graph.Node_by_id(nodeIDkey);

            if (!node) {
                ADDERROR(__func__,"no Node found that matches ID "+nodeIDkey.str()+" at chunk with ID "+chunk->get_tbegin_str());
                continue;
            }

            if (!chunk->set_Node_rapid_access(*node)) {
                ADDERROR(__func__,"Log chunk at ID "+chunk->get_tbegin_str()+" refused rapid-access Node pointer with ID "+node->get_id_str()+" (needed Node ID "+nodeIDkey.str()+')');
                continue;
            }
        }
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
    // Be careful, in case the Node the chunk is referencing does not exist in the Graph.
    Node * node = chunk.get_Node(_graph);
    if (!node)
        return nullptr;

    return _graph.main_Topic_of_Node(*node);
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
 * Report the span in seconds from when the Log was started (first cunk)
 * to its most recent chunk.
 * 
 * @param log the Log for which to report the span.
 * @return the number of seconds.
 */
unsigned long Log_span_in_seconds(Log & log) {
    return std::difftime(log.newest_chunk_t(),log.oldest_chunk_t());
}

/**
 * Report the span in days from when the Log was started (first cunk)
 * to its most recent chunk.
 * 
 * @param log the Log for which to report the span.
 * @return the decimal number of days.
 */
double Log_span_in_days(Log & log) {
    return (double)Log_span_in_seconds(log) / (60 * 60 * 24);
}

/**
 * Report the span in years, months and days from when the Log was started
 * (first cunk) to its most recent chunk.
 * 
 * @param log the Log for which to report the span.
 * @return a tuple of [year, month, day].
 */
ymd_tuple Log_span_years_months_days(Log & log) {
    return static_cast<ymd_tuple>(years_months_days(log.oldest_chunk_t(),log.newest_chunk_t()));
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
