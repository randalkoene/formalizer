// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <unistd.h>
//#include <utility>

// core
#include "standard.hpp"
#include "Graphaccess.hpp"
#include "Graphtypes.hpp"
#include "Logtypes.hpp"
#include "Graphpostgres.hpp"
#include "Logpostgres.hpp"

namespace fz {

/**
 * Graph access for server programs.
 * 
 * This helpful function initializes proper access parameters, sets up a shared
 * memory segment, and loads the Graph from the database.
 * 
 * Note: When `persistent_cache == true` then the call to `load_Graph_pq()` will
 *       also result in loading of the Named Node Lists cache.
 * 
 * @param remove_on_exit The shared memory is deleted when the calling program exits.
 * @param persistent_cache Initial value for Graph::persistent_NNL and determines if the cache is also loaded.
 * @return Pointer to a valid Graph data structure in shared memory.
 */
Graph * Graph_access::request_Graph_copy(bool remove_on_exit, bool persistent_cache, long tzadjust_seconds) {
//std::unique_ptr<Graph> Graph_access::request_Graph_copy() {
    if (!is_server) {
        VERBOSEOUT("\n*** This program is still using a temporary direct-load of Graph data.");
        VERBOSEOUT("\n*** Please replace that with access through fzserverpq as soon as possible!\n\n");
    }

    access_initialize();

    graphmemman.set_remove_on_exit(remove_on_exit);
    //std::unique_ptr<Graph> graphptr = graphmemman.allocate_Graph_in_shared_memory();
    Graph * graphptr = graphmemman.allocate_Graph_in_shared_memory();
    if (!graphptr)
        return nullptr;

    graphptr->set_Lists_persistence(persistent_cache);
    graphptr->set_tz_adjust(tzadjust_seconds);

    //std::unique_ptr<Graph> graphptr = std::make_unique<Graph>();

    if (!load_Graph_pq(*graphptr, dbname(), pq_schemaname())) {
        FZERR("\nSomething went wrong! Unable to load Graph from Postgres database.\n");
        standard.exit(exit_database_error);
    }
  
    return graphptr;
}

/**
 * A temporary stand-in while access to Log data through fzserverpq(-log) is not yet
 * available.
 * 
 * Replace this function as soon as possible!
 */
std::unique_ptr<Log> Graph_access::request_Log_copy() {
    if (!is_server) {
        VERBOSEOUT("\n*** This program is still using a temporary direct-load of Log data.");
        VERBOSEOUT("\n*** Please replace that with access through fzserverpq(-log) as soon as possible!\n\n");
    }

    access_initialize();

    std::unique_ptr<Log> logptr = std::make_unique<Log>();

    if (!load_Log_pq(*logptr, *this)) {
        FZERR("\nSomething went wrong! Unable to load Log from Postgres database.\n");
        standard.exit(exit_database_error);
    }

    return logptr;
}

std::unique_ptr<Log> Graph_access::request_Log_excerpt(const Log_filter & filter){
    /* We're actually presently testing if this is fine.
    if (!is_server) {
        VERBOSEOUT("\n*** This program is still using a temporary direct-load of Log data.");
        VERBOSEOUT("\n*** Please replace that with access through fzserverpq(-log) as soon as possible!\n\n");
    }*/

    access_initialize(); // this can handle being called multiple times (in case of additive filtering)

    std::unique_ptr<Log> logptr = std::make_unique<Log>();

    //VERBOSEOUT(filter.info_str());

    if (!load_partial_Log_pq(*logptr, *this, filter)) {
        FZERR("\nSomething went wrong! Unable to load Log from Postgres database.\n");
        standard.exit(exit_database_error);
    }

    return logptr;
}

/**
 * This is often called right after the Log has been loaded, and when the
 * memory-resident Graph is present. See, for example, how this is
 * called in `Graph_access::access_shared_Graph_and_request_Log_copy_with_init()`.
 * 
 * This does the following:
 * 1. Set up all of the Node-specific chains in the Log by setting the `node_prev`
 *    and `node_next` parameters in Log chunks and Log entries. This step, calling
 *    `Log::setup_Chain_nodeprevnext()` works independent of the Graph.
 * 2. Set up all Log_entry::node caches to point to their Nodes (requires Graph).
 * 3. Set up all Log_chunk::node caches to point to their Nodes (requires Graph).
 * 
 * Note that the Nodes themselves to not have cache variables with which to
 * reference Log chains (histories). A `Logtypes:Node_history` could be used
 * for this instead.
 */
void Graph_access::rapid_access_init(Graph &graph, Log &log) {
    log.setup_Chain_nodeprevnext();
    log.setup_Entry_node_caches(graph);
    log.setup_Chunk_node_caches(graph);
}

std::pair<Graph*, std::unique_ptr<Log>> Graph_access::request_Graph_and_Log_copies_and_init(bool remove_on_exit, bool persistent_cache) {
//std::pair<std::unique_ptr<Graph>, std::unique_ptr<Log>> Graph_access::request_Graph_and_Log_copies_and_init() {
    //std::unique_ptr<Graph> graphptr = request_Graph_copy();
    Graph * graphptr = request_Graph_copy(remove_on_exit, persistent_cache);
    std::unique_ptr<Log> logptr = request_Log_copy();

    if ((graphptr != nullptr) && (logptr != nullptr)) {
        //rapid_access_init(*(graphptr.get()),*(logptr.get()));
        rapid_access_init(*graphptr,*(logptr.get()));
        return std::make_pair(std::move(graphptr), std::move(logptr));
        
    } else {
        return std::make_pair(nullptr,nullptr);
    }
}

/**
 * This is similar to `request_Graph_and_Log_copies_and_init()` but uses the memory resident
 * Graph instead of loading one from the database.
 */
std::pair<Graph *, std::unique_ptr<Log>> Graph_access::access_shared_Graph_and_request_Log_copy_with_init() {

    Graph * graphptr = graphmemman.find_Graph_in_shared_memory();
    if (!graphptr) {
        ADDERROR(__func__, "Memory resident Graph not found");
        VERBOSEERR("Memory resident Graph not found.\n");
        return std::make_pair(nullptr,nullptr);;
    }

    std::unique_ptr<Log> logptr = request_Log_copy();

    if ((graphptr != nullptr) && (logptr != nullptr)) {
        //rapid_access_init(*(graphptr.get()),*(logptr.get()));
        rapid_access_init(*graphptr,*(logptr.get()));
        return std::make_pair(std::move(graphptr), std::move(logptr));
        
    } else {
        return std::make_pair(nullptr,nullptr);
    }

}



//#endif // TEMPORARY_DIRECT_GRAPH_LOAD_IN_USE

} // namespace fz
