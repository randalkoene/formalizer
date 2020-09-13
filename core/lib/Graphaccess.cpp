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
        FZOUT("\n*** This program is still using a temporary direct-load of Graph data.");
        FZOUT("\n*** Please replace that with access through fzserverpq as soon as possible!\n\n");
    }

    access_initialize();

    std::unique_ptr<Graph> graphptr = std::make_unique<Graph>();

    if (!load_Graph_pq(*graphptr, dbname, pq_schemaname)) {
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
        FZOUT("\n*** This program is still using a temporary direct-load of Log data.");
        FZOUT("\n*** Please replace that with access through fzserverpq(-log) as soon as possible!\n\n");
    }

    access_initialize();

    std::unique_ptr<Log> logptr = std::make_unique<Log>();

    if (!load_Log_pq(*logptr, *this)) {
        FZERR("\nSomething went wrong! Unable to load Log from Postgres database.\n");
        standard.exit(exit_database_error);
    }

    return logptr;
}

#endif // TEMPORARY_DIRECT_GRAPH_LOAD_IN_USE

} // namespace fz
