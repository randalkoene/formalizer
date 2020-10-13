// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Inspect memory resident servers.
 * 
 * For example, you may want to check the memory-resident existence and state of Graph data.
 * 
 * This tool can provide such information on the command line. It can also work with a small
 * Python CGI script to deliver the information in a browser-visual form.
 * 
 * For more about this, see https://trello.com/c/E9nq8lBk.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Server:Graph:Status"

// std
#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"
#include "proclock.hpp"
#include "jsonlite.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"

// local
#include "version.hpp"
#include "fzserver-info.hpp"
#include "render.hpp"

using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzserver_info fzsi;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzserver_info::fzserver_info() : formalizer_standard_program(false), config(*this) { //ga(*this, add_option_args, add_usage_top)
    add_option_args += "G";
    add_usage_top += " [-G]";
    //usage_head.push_back("Description at the head of usage information.\n");
    //usage_tail.push_back("Extra usage information.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzserver_info::usage_hook() {
    //ga.usage_hook();
    FZOUT("    -G Get Graph server status.\n");
}

/**
 * Handler for command line options that are defined in the derived class
 * as options specific to the program.
 * 
 * Include case statements for each option. Typical handlers do things such
 * as collecting parameter values from `cargs` or setting `flowcontrol` choices.
 * 
 * @param c is the character that identifies a specific option.
 * @param cargs is the optional parameter value provided for the option.
 */
bool fzserver_info::options_hook(char c, std::string cargs) {
    //if (ga.options_hook(c,cargs))
    //        return true;

    switch (c) {

    case 'G': {
        flowcontrol = flow_graph_server;
        return true;
    }

    }

    return false;
}


/// Configure configurable parameters.
bool fzsi_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    //CONFIG_TEST_AND_SET_PAR(example_par, "examplepar", parlabel, parvalue);
    //CONFIG_TEST_AND_SET_FLAG(example_flagenablefunc, example_flagdisablefunc, "exampleflag", parlabel, parvalue);
    CONFIG_PAR_NOT_FOUND(parlabel);
}


/**
 * Initialize configuration parameters.
 * Call this at the top of main().
 * 
 * @param argc command line parameters count forwarded from main().
 * @param argv command line parameters array forwarded from main().
 */
void fzserver_info::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

int graph_server_status() {
    Graph * graph_ptr = graphmemman.find_Graph_in_shared_memory();
    if (!graph_ptr) {
        ADDERROR(__func__, "Memory resident Graph not found");
        FZERR("Memory resident Graph not found.\n");
        standard.exit(exit_general_error);
    }

    FZOUT(graphmemman.info()+'\n');

    FZOUT(Graph_Info(*graph_ptr));

    FZOUT('\n'+Nodes_statistics_string(Nodes_statistics(*graph_ptr))+'\n');

    std::string lockfilecontent;
    int lockfile_status = check_and_read_lockfile(fzsi.lockfilepath, lockfilecontent);
    std::string lockfile_status_str(fzsi.lockfilepath);
    std::string process_status_str;
    switch (lockfile_status) {

        case 0: {
            lockfile_status_str += " not found";
            process_status_str = "inactive";
        }

        case 1: {
            lockfile_status_str += " exists";
            jsonlite_label_value_pairs jsonpars = json_get_label_value_pairs_from_string(lockfilecontent);
            if (jsonpars.find("pid") == jsonpars.end()) {
                lockfile_status_str += " (MISSING PID)";
            } else {
                lockfile_status_str += " (PID="+jsonpars["pid"]+')';
                pid_t pid = atoi(jsonpars["pid"].c_str());
                if (test_process_running(pid)) {
                    process_status_str = "active";
                } else {
                    process_status_str = "NOT FOUND (inactive)";
                }
            }
        }

        default: {
            lockfile_status_str += " ERROR";
            process_status_str = "UNKNOWN";
        }
    }

    FZOUT("\nfzserverpq lock file status: "+lockfile_status_str);
    FZOUT("\nfzserverpq process status  : "+process_status_str+"\n\n");

    return standard.completed_ok();
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzsi.init_top(argc, argv);

    switch (fzsi.flowcontrol) {

    case flow_graph_server: {
        return graph_server_status();
    }

    default: {
        fzsi.print_usage();
    }

    }

    return standard.completed_ok();
}
