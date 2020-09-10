// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares standard Graph access structures and functionss that should be
 * used when a standardized Formalizer C++ program needs Graph access.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define ___GRAPHACCESS_HPP.
 */

#ifndef ___GRAPHACCESS_HPP
#include "coreversion.hpp"
#define ___GRAPHACCESS_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
//#include <ostream>
#include <memory>

// core
//#include "error.hpp"

// *** The following will be removed once the fzserverpq is ready.
#define TEMPORARY_DIRECT_GRAPH_LOAD_IN_USE

namespace fz {

class Graph; // forward declaration
/**
 * A standardized way to access the Graph database.
 * 
 * Note: While TEMPORARY_DIRECT_GRAPH_LOAD_IN_USE is defined this includes
 *       code to provide direct access to the Postgres database, which is
 *       not advisable and will be replaced.
 */
struct Graph_access {
    std::string dbname;
    std::string pq_schemaname;

    bool is_server; /// authoritative server programs should set this flag

    /**
     * Carry out initializations needed to enable access to the Graph data structure.
     * 
     * @param add_topion_args_here receiving string where "d:" is appended to extend
     *                             command line options recognized.
     * @param add_usage_top_here receiving string where the option format is appended
     *                           to extend the top line of usage output.
     */
    Graph_access(std::string & add_option_args_here, std::string & add_usage_top_here, bool _isserver = false): is_server(_isserver) {
        //COMPILEDPING(std::cout, "PING-Graph_access().1\n");
        // This is better called just before a Graph request: graph_access_initialize();
        add_option_args_here += "d:s:";
        add_usage_top_here += " [-d <dbname>] [-s <schemaname>]";
    }

    void usage_hook();

    bool options_hook(char c, std::string cargs);

protected:
    void dbname_error();
    void schemaname_error();
public:
    void graph_access_initialize();

#ifdef TEMPORARY_DIRECT_GRAPH_LOAD_IN_USE
public:
    std::unique_ptr<Graph> request_Graph_copy();
#endif

};

} // namespace fz

#endif // ___GRAPHACCESS_HPP
