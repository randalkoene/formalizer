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

fzquerypq::fzquerypq() : formalizer_standard_program(true), output_format(output_txt), ga(*this, add_option_args, add_usage_top, true), flowcontrol(flow_unknown) {
    COMPILEDPING(std::cout, "PING-fzquerypq().1\n");
    add_option_args += "n:F:R:";
    add_usage_top += " [-n <Node-ID>] [-F txt|html] [-R histories]";
}

void fzquerypq::usage_hook() {
    ga.usage_hook();
    FZOUT("    -n request data for Node <Node-ID>\n");
    FZOUT("    -F specify output format\n");
    FZOUT("       available formats:\n");
    FZOUT("       txt = basic ASCII text template\n")
    FZOUT("       html = HTML template\n");
    FZOUT("    -R refresh:\n");
    FZOUT("       histories = Node histories cache table\n");
}

bool fzquerypq::set_output_format(const std::string & cargs) {
    if (cargs == "html") {
        output_format = output_html;
        return true;
    }
    if (cargs == "txt") {
        output_format = output_txt;
        return true;
    }
    ADDERROR(__func__,"unknown output format: "+cargs);
    FZERR("unknown output format: "+cargs);
    exit(exit_command_line_error);
    return false; // never gets here
}

bool fzquerypq::options_hook(char c, std::string cargs) {
    if (ga.options_hook(c,cargs))
        return true;

    switch (c) {

    case 'n': {
        flowcontrol = flow_node;
        node_idstr = cargs;
        return true;
    }
    
    case 'F': {
        set_output_format(cargs);
        return true;
    }

    case 'R': {
        if (cargs=="histories") {
            flowcontrol = flow_refresh_histories;
            return true;
        }
        ADDERROR(__func__, "Unknown option -R "+cargs);
        VERBOSEERR("Unknown option -R "+cargs+'\n');
        return false;
    }

    }

    return false;
}

/**
 * Initialize configuration parameters.
 * Call this at the top of main().
 * 
 * @param argc command line parameters count forwarded from main().
 * @param argv command line parameters array forwarded from main().
 */
void fzquerypq::init_top(int argc, char *argv[]) {
    //*************** for (int i = 0; i < argc; ++i) cmdargs[i] = argv[i]; // do this before getopt mucks it up
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
}

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
