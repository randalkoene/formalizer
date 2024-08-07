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
#include "Logtypes.hpp"
#include "fzpostgres.hpp"

namespace fz {

// foward declarations of classes external to this file
class Graph;
class Log;

/**
 * A standardized way to access the Graph database.
 * 
 */
struct Graph_access: public Postgres_access {

    /**
     * Carry out initializations needed to enable access to the Graph data structure.
     * 
     * @param add_option_args_here receiving string where "d:" is appended to extend
     *                             command line options recognized.
     * @param add_usage_top_here receiving string where the option format is appended
     *                           to extend the top line of usage output.
     */
    Graph_access(formalizer_standard_program & fps, std::string & add_option_args_here, std::string & add_usage_top_here, bool _isserver = false): Postgres_access(fps, add_option_args_here,add_usage_top_here,_isserver) {
        //COMPILEDPING(std::cout, "PING-Graph_access().1\n");
        // This is better called just before a Graph request: graph_access_initialize();
    }

    //void usage_hook(); // *** presently identical to Postgres_access::usage_hook()
    //bool options_hook(char c, std::string cargs); // *** presently identical to Postgres_access::options_hook()

public:
    //std::unique_ptr<Graph> request_Graph_copy();
    Graph * request_Graph_copy(bool remove_on_exit = true, Graph_Config_Options * graph_config_ptr = nullptr); // *** switched to this, because Boost Interprocess has difficulty with smart pointers
    std::unique_ptr<Log> request_Log_copy();
    std::unique_ptr<Log> request_Log_excerpt(const Log_filter & filter);
    void rapid_access_init(Graph &graph, Log &log);                                                  ///< Once both Graph and Log instances have been loaded.
    //std::pair<std::unique_ptr<Graph>, std::unique_ptr<Log>> request_Graph_and_Log_copies_and_init(); ///< Combine the three functions above.
    std::pair<Graph*, std::unique_ptr<Log>> request_Graph_and_Log_copies_and_init(bool remove_on_exit = true, Graph_Config_Options * graph_config_ptr = nullptr); ///< Combine the three functions above.
    std::pair<Graph *, std::unique_ptr<Log>> access_shared_Graph_and_request_Log_copy_with_init();
};

} // namespace fz

#endif // __GRAPHACCESS_HPP
