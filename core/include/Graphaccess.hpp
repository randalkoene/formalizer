// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares standard Graph access structures and functionss that should be
 * used when a standardized Formalizer C++ program needs Graph access.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __GRAPHACCESS_HPP.
 */

#ifndef __GRAPHACCESS_HPP
#include "coreversion.hpp"
#define __GRAPHACCESS_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
//#include <ostream>
#include <memory>

// core
//#include "error.hpp"
#include "fzpostgres.hpp"

// *** The following will be removed once the fzserverpq is ready.
#define TEMPORARY_DIRECT_GRAPH_LOAD_IN_USE

namespace fz {

// foward declarations of classes external to this file
class Graph;
class Log;

/**
 * A standardized way to access the Graph database.
 * 
 * Note: While TEMPORARY_DIRECT_GRAPH_LOAD_IN_USE is defined this includes
 *       code to provide direct access to the Postgres database, which is
 *       not advisable and will be replaced.
 */
struct Graph_access: public Postgres_access {

    /**
     * Carry out initializations needed to enable access to the Graph data structure.
     * 
     * @param add_topion_args_here receiving string where "d:" is appended to extend
     *                             command line options recognized.
     * @param add_usage_top_here receiving string where the option format is appended
     *                           to extend the top line of usage output.
     */
    Graph_access(std::string & add_option_args_here, std::string & add_usage_top_here, bool _isserver = false): Postgres_access(add_option_args_here,add_usage_top_here,_isserver) {
        //COMPILEDPING(std::cout, "PING-Graph_access().1\n");
        // This is better called just before a Graph request: graph_access_initialize();
    }

    //void usage_hook(); // *** presently identical to Postgres_access::usage_hook()
    //bool options_hook(char c, std::string cargs); // *** presently identical to Postgres_access::options_hook()

#ifdef TEMPORARY_DIRECT_GRAPH_LOAD_IN_USE
public:
    std::unique_ptr<Graph> request_Graph_copy();
    std::unique_ptr<Log> request_Log_copy();
#endif

};

} // namespace fz

#endif // __GRAPHACCESS_HPP
