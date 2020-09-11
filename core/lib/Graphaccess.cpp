// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <unistd.h>
//#include <utility>

// core
#include "standard.hpp"
#include "Graphaccess.hpp"
#include "Graphtypes.hpp"
#include "Graphpostgres.hpp"

namespace fz {

void Graph_access::graph_access_initialize() {
    COMPILEDPING(std::cout,"PING-graph_access_initialize()\n");
    if (dbname.empty()) { // attempt to get a default
        dbname = DEFAULT_DBNAME;
        /* See how this was clarified and changed in https://trello.com/c/Lww33Lym.
        char *username = std::getenv("USER");
        if (username)
            dbname = username;
        */
    }
    if (pq_schemaname.empty()) { // attempt to get a default
        char *username = std::getenv("USER");
        if (username)
            pq_schemaname = username;
    }

    if (dbname.empty())
        dbname_error();
    if (pq_schemaname.empty())
        schemaname_error();

    if (!standard.quiet) {
        FZOUT("Postgres database selected: "+dbname+'\n');
        FZOUT("Postgres schema selected  : "+pq_schemaname+'\n');
    }
}

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

    graph_access_initialize();

    std::unique_ptr<Graph> graphptr = std::make_unique<Graph>();

    if (!load_Graph_pq(*graphptr, dbname, pq_schemaname)) {
        FZERR("\nSomething went wrong! Unable to load Graph from Postgres database.\n");
        standard.exit(exit_database_error);
    }

    return graphptr;
}

#endif // TEMPORARY_DIRECT_GRAPH_LOAD_IN_USE

} // namespace fz
