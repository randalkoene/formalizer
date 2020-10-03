// Copyright 2020 Randal A. Koene
// License TBD

/**
 * fzquerypq is the (typically non-daemonized) non-caching alternative to fzserverpq.
 * 
 */

#define FORMALIZER_MODULE_ID "Formalizer:GraphServer:Postgres:SingleQuery"

//#define USE_COMPILEDPING

// std
#include <filesystem>
#include <iostream>
#include <ostream>

// corelib
//#include "Graphtypes.hpp"
//#include "Logtypes.hpp"
#include "error.hpp"
#include "standard.hpp"

// local
#include "fzquerypq.hpp"
#include "servenodedata.hpp"
#include "refresh.hpp"


using namespace fz;

fzquerypq fzq;

int main(int argc, char *argv[]) {
    ERRTRACE;
    COMPILEDPING(std::cout, "PING-main().1\n");
    ERRHERE(".init");
    fzq.init_top(argc, argv);

    COMPILEDPING(std::cout, "PING-main().2\n");
    switch (fzq.flowcontrol) {

    case flow_node: {
        serve_request_Node_data();
        break;
    }

    case flow_refresh_histories: {
        refresh_Node_histories_cache_table();
        break;
    }

    default: {
        fzq.print_usage();
    }

    }

    COMPILEDPING(std::cout,"PING-main().3\n");
    ERRHERE(".exitok");
    return standard.completed_ok();
}
