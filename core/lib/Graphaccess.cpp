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

#ifdef TEMPORARY_DIRECT_GRAPH_LOAD_IN_USE

/**
 * A temporary stand-in while access to Graph data through fzserverpq is not yet
 * available.
 * 
 * Replace this function as soon as possible!
 */
std::unique_ptr<Graph> Graph_access::request_Graph_copy() {
    if (!is_server) {
        VERBOSEOUT("\n*** This program is still using a temporary direct-load of Graph data.");
        VERBOSEOUT("\n*** Please replace that with access through fzserverpq as soon as possible!\n\n");
    }

    access_initialize();

    std::unique_ptr<Graph> graphptr = std::make_unique<Graph>();

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

    if (!load_partial_Log_pq(*logptr, *this, filter)) {
        FZERR("\nSomething went wrong! Unable to load Log from Postgres database.\n");
        standard.exit(exit_database_error);
    }

    return logptr;
}

void Graph_access::rapid_access_init(Graph &graph, Log &log) {
    log.setup_Chain_nodeprevnext();
    log.setup_Entry_node_caches(graph);
    log.setup_Chunk_node_caches(graph);
}

std::pair<std::unique_ptr<Graph>, std::unique_ptr<Log>> Graph_access::request_Graph_and_Log_copies_and_init() {
    std::unique_ptr<Graph> graphptr = request_Graph_copy();
    std::unique_ptr<Log> logptr = request_Log_copy();

    if ((graphptr != nullptr) && (logptr != nullptr)) {
        rapid_access_init(*(graphptr.get()),*(logptr.get()));
        return std::make_pair(std::move(graphptr), std::move(logptr));
        
    } else {
        return std::make_pair(nullptr,nullptr);
    }
}

#endif // TEMPORARY_DIRECT_GRAPH_LOAD_IN_USE

} // namespace fz
