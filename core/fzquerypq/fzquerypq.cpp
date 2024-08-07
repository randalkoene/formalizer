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
#include <chrono>

// corelib
//#include "Graphtypes.hpp"
//#include "Logtypes.hpp"
#include "error.hpp"
#include "standard.hpp"

// local
#include "fzquerypq.hpp"
#include "serializedAPIrequest.hpp"
#include "servenodedata.hpp"
#include "refresh.hpp"


using namespace fz;

fzquerypq fzq;

fzquerypq::fzquerypq() : formalizer_standard_program(true), output_format(output_txt), ga(*this, add_option_args, add_usage_top, true), flowcontrol(flow_unknown) {
    COMPILEDPING(std::cout, "PING-fzquerypq().1\n");
    add_option_args += "n:F:R:Z:T";
    add_usage_top += " [-n <Node-ID>] [-F txt|html] [-R histories] [-Z <serialized-request>] [-T]";
}

void fzquerypq::usage_hook() {
    ga.usage_hook();
    FZOUT("    -n request data for Node <Node-ID>\n"
          "    -F specify output format\n"
          "       available formats:\n"
          "       txt = basic ASCII text template\n"
          "       html = HTML template\n"
          "    -R refresh:\n"
          "         histories = Node histories cache table\n"
          "         namedlists = Named Node Lists cache table\n"
          "    -Z make serialized data request <serialized_request> (see fzserverpq -h)\n"
          "    -T show current time time-stamp in Formalizer format and UNIX epoch seconds\n"
    );
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
    standard_exit_error(exit_command_line_error, "unknown output format: "+cargs, __func__);
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
        if (cargs=="namedlists") {
            flowcontrol = flow_refresh_namedlists;
            return true;
        }
        return standard_error("Unknown option -R "+cargs, __func__);
    }

    case 'Z': {
        flowcontrol = flow_serialized_request;
        request_str = cargs;
        return true;
    }

    case 'T': {
        flowcontrol = flow_formalizer_time;
        return true;
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


void show_formalizer_time() {
    VERYVERBOSEOUT("FZ preferred time zone time-stamp, UNIX epoch seconds, and unique TAI time:\n");
    time_t t = ActualTime(); // *** keep ActualTime() as is or derive from chrono::system_time()::now()?
    //auto tp = std::chrono::system_clock::from_time_t(t);
    //using tai_time = std::chrono::time_point<std::chrono::tai_clock, long long>;
    //auto tz = std::chrono::tai_clock::tai_time();

    FZOUT(TimeStampYmdHM(t)+' '+std::to_string(t)+'\n');
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

    case flow_refresh_namedlists: {
        refresh_Named_Node_Lists_cache_table();
        break;
    }

    case flow_serialized_request: {
        make_serialized_data_API_request();
        break;
    }

    case flow_formalizer_time: {
        show_formalizer_time();
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
