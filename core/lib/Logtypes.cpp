// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <cstdint>
#include <iomanip>
#include <numeric>

// core
#include "utf8.hpp"
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
 * Set the Log_entry.entrytext parameter content and ensure that it contains
 * valid UTF8 encoded content.
 * 
 * Attempts to assign content to the text parameter as provided, and
 * replaces any invalid UTF8 code points with a replacement
 * character. The default replacement character is the UTF8
 * 'REPLACEMENT CHARACTER' 0xfffd.
 * 
 * If invalid UTF8 code points were encountered and replaced then it
 * is reported through ADDWARNING.
 * 
 * Note 1: ASCII text is valid UTF8 text and will be assigned
 *         unaltered.
 * Note 2: This does not test for valid HTML5 at this time.
 * 
 * @param utf8str a string that should contain UTF8 encoded text.
 */
void Log_entry::set_text(const std::string utf8str) {
    entrytext = utf8_safe(utf8str);
}

#ifdef USING_DEQUE_CHUNKS
/**
 * Slow brute force search for index of and pointer to a Log chunk from its ID key.
 * 
 * This one should work if the ID is in there, even if the IDs are not sorted
 * correctly for some reason (i.e. if some IDs are not in correct temporal Log order).
 * 
 * @param chunk_id the Log chunk ID.
 * @return the pair of index in the list and pointer to Log chunk, or [::size(), nullptr] if not found.
 */
std::pair<Log_chunk_ptr_map::iterator, Log_chunk*> Log_chunks_Map::slow_find(const Log_chunk_ID_key chunk_id) const {
    Log_chunk_ptr_deque::size_type i = 0;
    for (const auto& chptr : (*this)) { //*** doing this because it's a deque, would use index if it were a vector
        if (chptr->get_tbegin_key() == chunk_id)
            return std::make_pair(i,chptr.get());

        ++i; // so that we can also return an index
    }
    return std::make_pair(size(),nullptr);
}

/**
 * Find index of and pointer to a Log chunk from its ID key.
 * 
 * This implementation attempts to be quick about it by relying on the sorted
 * order of Log chunks to apply a quick search method.
 * (This is actually probably the same as using std::binary_search.)
 * 
 * Note A: During the search, we are requesting pointers in order to be able
 * to return both at once (to make both index and pointer serach functions as
 * fast as possible). But we skip testing for nullptr, because a nullptr
 * should never be in the list in the first place. This is a calculated risk.
 * 
 * Note B: This function is not as fast as it could be. Checking a specific
 * index of a deque is slower than in a vector. See the proposal in the
 * card at https://trello.com/c/qYEwgsFs.
 * 
 * Note C: This function is unfortunately forced to go to slow_find() on
 * occasion, because of IDs that are out of order. See the proposal in the
 * card at https://trello.com/c/tiOWQkdP.
 * 
 * @param chunk_id the Log chunk ID.
 * @return the pair of index in the list and pointer to Log chunk, or [::size(), nullptr] if not found.
 */
std::pair<Log_chunk_ptr_deque::size_type, Log_chunk*> Log_chunks_Deque::find_index_and_pointer(const Log_chunk_ID_key chunk_id) const {
    if (size()<1)
        return std::make_pair(0,nullptr);
    
    long lowerbound = 0;
    long upperbound = size()-1;
    long tryidx = size()/2;

    while (true) {

        Log_chunk * chunkptr = get_chunk(tryidx); // skipping null-test (see notes)

        if (chunkptr->get_tbegin_key() == chunk_id)
            return std::make_pair(tryidx,chunkptr);

        if (chunkptr->get_tbegin_key() < chunk_id) {
            lowerbound = tryidx + 1;
        } else {
            upperbound = tryidx - 1;
        }

        if (lowerbound > upperbound) {
            return slow_find(chunk_id);
            //return std::make_pair((size(),nullptr); // not found
        }

        tryidx = lowerbound + ((upperbound - lowerbound) / 2);
    }

    // never gets here
}

/**
 * Find just the index of a Log chunk from its ID.
 * 
 * This uses the quick search.
 * 
 * @param chunk_id the Log chunk ID.
 * @return the index in the list, or ::size() if not found.
 */
Log_chunk_ptr_deque::size_type Log_chunks_Deque::find(const Log_chunk_ID_key chunk_id) const {
    return std::get<0>(find_index_and_pointer(chunk_id));
}

/**
 * Find just the pointer reference to a Log chunk from its ID.
 * 
 * This uses the quick search.
 * 
 * @param chunk_id the Log chunk ID.
 * @return pointer to the Log chunk, or nullptr if not found.
 */
Log_chunk * Log_chunks_Deque::get_chunk(const Log_chunk_ID_key chunk_id) const {
    return std::get<1>(find_index_and_pointer(chunk_id));
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
#endif // USING_DEQUE_CHUNKS

/**
 * Find index of Log chunk by its ID closest to time t.
 * 
 * This search will return either the index for the Log chunk with the
 * same start time, or the nearest above or below, depending on the
 * `later` flag.
 * 
 * If the start time provided would not convert to a valid Log ID time
 * stamp then this function returns an iterator to the earliest element
 * in the map if `throw_if_invalid` is false. Otherwise it throws an
 * ID_exception.
 * 
 * @param t The Log chunk start time to search for.
 * @param later Find start time >= t, otherwise find start time <= t.
 * @param throw_if_invalid If true then requests with invalid t throw an ID_exception.
 * @return The closest index in the list, or end() if not found.
 */
Log_chunk_ptr_map::const_iterator Log_chunks_Map::find_nearest(std::time_t t, bool later, bool throw_if_invalid) const {
    if (t < LOG_ID_FIRST_VALID_TEPOCH) {
        if (throw_if_invalid) {
            std::string formerror("Attempted Log ID search with epoch time ("+std::to_string(t)+") that converts to invalid Log ID\n");
            throw(ID_exception(formerror));
        }
        return begin(); // const_cast<Log_chunks_Map *>(this)->begin(); -- use this if returning ::iterator
    }

    if (empty())
        return end(); // const_cast<Log_chunks_Map *>(this)->end(); -- use this if returning ::iterator

    const Log_chunk_ID_key search_key(t);
    if (later) { // find ID key corresponding to time >= t
        auto it_after_search_key = upper_bound(search_key); // const_cast<Log_chunks_Map *>(this)->upper_bound(search_key); // this can be end()
        if (it_after_search_key != begin()) { // we're looking for >= t, not just > t
            auto it_prev_after = std::prev(it_after_search_key);
            if (it_prev_after->first == search_key) {
                return it_prev_after;
            }
        }
        return it_after_search_key;

    } else { // find ID key corresponding to time <= t
        auto it_same_or_before_search_key = lower_bound(search_key); // const_cast<Log_chunks_Map *>(this)->lower_bound(search_key); // includes equal
        return it_same_or_before_search_key;
    }
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
 * This function is also used by the rapid search version `get_Node_chain()`,
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

    for (auto it = chunks.begin(); it!=chunks.end(); ++it) {

        // check the chunk first
        if (it->second->get_NodeID().key().idT == node_id.key().idT) {
            res.emplace_back(it->first,it->second.get());
            if (onlyfirst)
                return res;
        }

        // then check its entries
        for (const auto& e : it->second->get_entries()) {
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
 * Parse all chunks connected to the Log - in reverse - and find the sequence
 * of chunks and entries that belongs to a specific Node. Reverse search is
 * probably a good option, because often referenced Node histories are likely
 * those of recent Nodes with recent Log entries.
 * 
 * This can be used independent of the reference parameters in the Log_chunk
 * objects, hence even works for arbitrary loaded lists of Log chunks.
 * 
 * This function requires that the `entries` list has been set up
 * for all Log chunks.
 * 
 * If `onlylast == true` then the deque returned will contain only one
 * element, namely the last (latest, newest) element in the chain that
 * belongs to `node_id`. A full search is not conducted in this case.
 * 
 * @param node_id of the Node for which to collect its Log chain (history).
 * @param onlylast flag if true return only the last target (do not find the whole list).
 * @return a deque sorted list of chain targets found.
 */
std::deque<Log_chain_target> Log::get_Node_chain_fullparse_reverse(const Node_ID node_id, bool onlylast) {
    std::deque<Log_chain_target> res;

    for (auto rit = chunks.rbegin(); rit!=chunks.rend(); ++rit) {

        // check the entries first
        std::vector<Log_entry *> & chunkentries = rit->second->get_entries();
        for (auto erit = chunkentries.rbegin(); erit!=chunkentries.rend(); ++erit) {
            Log_entry * e = (*erit);
            if (!(e->get_nodeidkey().isnullkey())) {
                if (e->get_nodeidkey() == node_id.key()) {
                    res.emplace_front(e->get_id_key(),e);
                    if (onlylast)
                        return res;
                }
            }
        }
        // then check the chunk
        if (rit->second->get_NodeID().key().idT == node_id.key().idT) {
            res.emplace_front(rit->first,rit->second.get());
            if (onlylast)
                return res;
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
 * Quickly walk through the reference chain - in reverse - that belongs to
 * the specified Node and return all of its Log chunks and Log entries. Reverse
 * search is probably a good option, because often referenced Node histories are
 * likely those of recent Nodes with recent Log entries.
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
std::deque<Log_chain_target> Log::get_Node_chain_reverse(const Node_ID node_id) {
    std::deque<Log_chain_target> res = get_Node_chain_fullparse_reverse(node_id,true);

    if (res.empty())
        return res;

    for (const Log_chain_target * prev = res.front().prev_in_chain(); prev != nullptr; prev = prev->prev_in_chain()) {
        res.emplace_front(*prev);
    }

    return res;
}

/**
 * These functions make no assumptions about the reliability of the cached head and
 * tail chain pointers of a Node. They begin by walking backwards in time from the
 * newest Log chunk's entries to the oldesst Log chunk, collecting chain elements
 * (entries and chunks) that belong to the Node along the way.
 * 
 * Chain elements found are emplaced at the front of the deque, meaning that the
 * oldest element will be at the front of the deque when done, and the newest
 * element will be at the back.
 * 
 * The `newest_Node_chain_element()` function then returns the `back()` of the
 * deque, i.e. the newest entry or chunk that belongs to the Node.
 * 
 * The `oldest_Node_chain_element()` function also appears to take the `back()`, i.e.
 * the newest element, after walking the whole chain - and then does a walk
 * backwards using `prev_in_chain()`, until that returns `nullptr`. This is done,
 * because calling `get_Node_chain_fullparse_reverse` with `onlylast == true` means
 * that only the neweest element is returned in the deque. Even so, this will only
 * work if the Log chains have been set up in advance. This happens, for example,
 * through `Graphaccess:Graph_access::rapid_access_init()`. See, for example, how
 * this is used via `Graph_access::access_shared_Graph_and_request_Log_copy_with_init()`
 * in `graph2dil`.
 * 
 * Either of these functions should return a nullptr only if there are no Log
 * chunks or entries associated with node_id.
 */
const Log_chain_target * Log::newest_Node_chain_element(const Node_ID node_id) {
    // *** if not cached or if the cache is invalid (out of date)
    std::deque<Log_chain_target> res = get_Node_chain_fullparse_reverse(node_id,true);

    if (res.empty())
        return nullptr;
    
    return &(res.back());
}

/**
 * Notice that the set of tests in the while-loop are necessary here. You cannot
 * simply test if prev_in_chain() is nullptr. See the explanation above
 * LogtypesID.cpp:next_in_chain() and the referenced debugging example.
 */
const Log_chain_target * Log::oldest_Node_chain_element(const Node_ID node_id) {
    // *** if not cached or if the cache is invalid (out of date)
    std::deque<Log_chain_target> res = get_Node_chain_fullparse_reverse(node_id,true);

    if (res.empty())
        return nullptr;

    const Log_chain_target * prev = &(res.back());
    const Log_chain_target * prev_prev;
    while ((prev_prev = prev->prev_in_chain()) != nullptr) {
        if (prev_prev->isnulltarget_byptr()) { // if (prev->prev_in_chain()->isnulltarget_byptr()) {
            break;
        }
        prev = prev_prev; // prev->prev_in_chain();
    }

    return prev;
}

/**
 * Find a pointer to the Node that the Log entry belongs to, either
 * directly if the Node is specified for that Log entry, or by
 * inheriting the Node from the Log chunk that the Log entry is in.
 * 
 * @return A pointer to the Node this Log entry belongs to.
 */
Node * Log_entry::get_local_or_inherited_Node() {
    if (same_node_as_chunk()) {
        return get_Chunk()->get_Node();
    } else {
        return get_Node();
    }
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

    for (const auto& [chunk_key, chunkptr] : chunks) { // .begin(); it != chunks.end(); ++it) {

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
    for (const auto& [chunk_key, chunkptr] : chunks) {
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
 * Prune duplicate chunks.
 * 
 * Note that this does not take care of entries that may be connected to a pruned
 * chunk. In other words, this is better done before add_entries_to_chunks().
 * 
 * NOTE: Now that chunks is a map this function is probably superfluous.
 */
unsigned long Log::prune_duplicate_chunks() {
    Log_chunk_ID_key_set chunkkeyset;
    unsigned long pruned = 0;
    for (auto it = chunks.begin(); it != chunks.end(); ++it) {
        auto ret = chunkkeyset.emplace(it->first);
        if (!ret.second) {
            chunks.erase(it);
            ++pruned;
        }
    }
    return pruned;
}

/**
 * If Log_entry objects were created without immediately specifying a corresponding
 * Log_chunk object (created earlier), i.e. if using a 2-pass method, then parse
 * the list of entries and connect entries with chunks.
 */
bool Log::add_entries_to_chunks() {
    for (auto & [entrykey, entryptr] : entries) {
        const Log_chunk_ID_key chunkkey(entrykey); // no need to try, this one has to be valid if the entry ID was valid
        const Log_chunk * chunk = get_chunk(chunkkey);
        if (!chunk)
            ERRRETURNFALSE(__func__,"entry ("+entrykey.str()+") refers to Log chunk not found in Log");

        entryptr->set_Chunk(chunk);
        const_cast<Log_chunk *>(chunk)->add_Entry(*entryptr);
    }
    return true;
}

/*
*** This was not quite implemented to the point where it works, as it turned out that I didn't need it yet.
void Log::add_earlier_unique_Chunk(const Log_TimeStamp &_tbegin, const Node_ID &_nodeid, std::time_t _tclose) {
    if (find_chunk_by_key(_tbegin)>=chunks.size()) {
        chunks.push_front(std::make_unique<Log_chunk>(_tbegin,_nodeid,_tclose));
    }
}
void Log::add_later_unique_Chunk(const Log_TimeStamp &_tbegin, const Node_ID &_nodeid, std::time_t _tclose) {
    if (find_chunk_by_key(_tbegin)>=chunks.size()) {
        chunks.push_back(std::make_unique<Log_chunk>(_tbegin,_nodeid,_tclose));
    }
}
*/

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
    //auto [from_idx, to_idx] = get_Chunks_index_t_interval(t_from,t_before);
    auto interval = get_Chunks_index_t_interval(t_from,t_before);

    if (interval.second == chunks.end()) //(from_idx == chunks.end())
        return std::make_pair(Log_chunk_ID_key(),Log_chunk_ID_key()); // returning null-key pair

    return std::make_pair(get_chunk_id_key(interval.first), get_chunk_id_key(interval.second)); // std::make_pair(get_chunk_id_key(from_idx),get_chunk_id_key(to_idx));
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

    if (from_idx == chunks.end())
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
Log_chunk_const_iterator_interval Log::get_Chunks_index_t_interval(std::time_t t_from, std::time_t t_before) {
    std::time_t t_to = t_before - 1;

    if (t_to<t_from)
        return std::make_pair(chunks.end(), chunks.end());

    Log_chunk_ptr_map::const_iterator from_idx = chunks.find_nearest(t_from,true);
    if (from_idx == chunks.end())
        return std::make_pair(from_idx,from_idx);

    Log_chunk_ptr_map::const_iterator to_idx = chunks.find_nearest(t_to,false);

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
Log_chunk_const_iterator_interval Log::get_Chunks_index_n_interval(std::time_t t_from, unsigned long n) {
    if (n==0)
        return std::make_pair(chunks.end(), chunks.end());

    Log_chunk_ptr_map::const_iterator from_idx = chunks.find_nearest(t_from,true);
    if (from_idx == chunks.end())
        return std::make_pair(from_idx,from_idx);

    Log_chunk_ptr_map::const_iterator to_idx = std::next(from_idx, (n-1));
    if (to_idx == chunks.end())
        to_idx = std::prev(chunks.end());
    
    return std::make_pair(from_idx,to_idx);
}

Log_chunk * Log::get_newest_Chunk() {
    if (chunks.empty())
        return nullptr;

    return std::prev(chunks.end())->second.get();
}

Log_entry * Log::get_newest_Entry() {
    if (entries.empty())
        return nullptr;

    return std::prev(entries.end())->second.get();
}

std::string Log_filter::info_str() const {
    std::string infostr("filter:");
    infostr += "\n\tt_from = "+TimeStampYmdHM(t_from);
    infostr += "\n\tt_to   = "+TimeStampYmdHM(t_to);
    infostr += "\n\tnkey   = "+nkey.str();
    infostr += "\n\tlimit  = "+std::to_string(limit);
    infostr += (back_to_front ? "\n\tback_to_front = true\n" : "\n\tback_to_front = false\n");
    return infostr;
}

/**
 * Generate a set of Log chunk ID keys that corresponds to the ID keys of all
 * entries in the Log.
 * 
 * @return A set of `Log_chunk_ID_key` objects.
 */
Log_chunk_ID_key_set Log::chunk_key_list_from_entries() {
    Log_chunk_ID_key_set chunkkeyset;
    for (const auto& [entrykey, entryptr]: entries) {
        chunkkeyset.emplace(entrykey);
    }
    return chunkkeyset;
}

/// Parse all chunks and entries, and assign them to their Node ID keys.
void Node_histories::init(Log & log) {
    for (const auto & [chunk_key, chunk] : log.get_Chunks()) {
        Node_ID_key nkey(chunk->get_NodeID().key());
        if (nkey.isnullkey())
            continue; // This would actually be an error...
        
        auto it = find(nkey);
        if (it == end()) {
            history_ptr hptr = std::make_unique<Node_history>();
            hptr->chunks.emplace(chunk->get_tbegin_key());
            emplace(nkey, std::move(hptr));
        } else {
            it->second->chunks.emplace(chunk->get_tbegin_key());
        }
    }
    for (const auto & [entrykey, entryptr] : log.get_Entries()) {
        Node_ID_key nkey(entryptr->get_nodeidkey());
        if (nkey.isnullkey()) { // belongs to the same Node as the surrounding chunk
            nkey = entryptr->get_Chunk()->get_NodeID().key(); // *** Mildly risky, there should really be no nullptr here.
        }
        if (nkey.isnullkey())
            continue; // This would actually be an error...
        
        auto it = find(nkey);
        if (it == end()) {
            history_ptr hptr = std::make_unique<Node_history>();
            hptr->entries.emplace(entrykey);
            emplace(nkey, std::move(hptr));
        } else {
            it->second->entries.emplace(entrykey);
        }
    }
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
std::vector<Log_chunks_Map::iterator> Breakpoint_Indices(Log & log) {
    std::vector<Log_chunks_Map::iterator> indices(log.num_Breakpoints());
    for (std::deque<Log_chunk_ID_key>::size_type i = 0; i < log.num_Breakpoints(); ++i) {
        Log_chunk_ptr_map::iterator idx = log.get_Chunks().find(log.breakpoints.get_chunk_id_key(i));
        if (idx == log.get_Chunks().end()) {
            ADDERROR(__func__,"Log breakpoint["+std::to_string(i)+"]="+log.breakpoints.get_chunk_id_str(i)+" is not a known Log chunk");
            indices[i] = log.chunks.end(); // indicates not found
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
std::vector<size_t> Chunks_per_Breakpoint(Log & log) {
    std::vector<size_t> chunks_per_bp;
    auto chunkindices = Breakpoint_Indices(log);
    for (auto bp_it = chunkindices.begin(); bp_it != chunkindices.end(); ++bp_it) {
        auto next_bp_it = std::next(bp_it);
        if (next_bp_it == chunkindices.end()) { // count to end
            chunks_per_bp[chunks_per_bp.size()] = distance(*bp_it,log.chunks.end());
        } else { // count to next breakpoint
            chunks_per_bp[chunks_per_bp.size()] = distance(*bp_it,*next_bp_it);
        }
    }
    return chunks_per_bp;
}
/*
    chunkindices.push_back(std::prev(log.chunks.end())); // append the latest Log chunk index
    auto diff = chunkindices;                     // easy way to make sure it's the same type and size
    std::adjacent_difference(chunkindices.begin(), chunkindices.end(), diff.begin());
    diff.erase(diff.begin()); // trim diff[0], see std::adjacent_difference()
    return diff;
*/

/**
 * Calculate the total number of minutes logged for all Log chunks in the
 * specified deque.
 * 
 * @param chunks a deque containing a sorted list Log_chunk pointers.
 * @return the sum total of time logged in minutes.
 */
unsigned long Chunks_total_minutes(Log_chunks_Map & chunks) {
    struct {
        unsigned long operator()(unsigned long total, Log_chunk_ptr_map_element & chunkptr) {
            if (chunkptr.second)
                return total + chunkptr.second->duration_minutes();
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
