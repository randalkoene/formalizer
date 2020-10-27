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
#include "tcpclient.hpp"

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
fzserver_info::fzserver_info() : formalizer_standard_program(false), config(*this), output_format(output_txt) { //ga(*this, add_option_args, add_usage_top)
    add_option_args += "Go:F:";
    add_usage_top += " [-G] [-o <info-output-path>] [-F txt|html]";
    //usage_head.push_back("Description at the head of usage information.\n");
    usage_tail.push_back("The <info-output-path> STDOUT sends to standard out and is the default.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzserver_info::usage_hook() {
    //ga.usage_hook();
    FZOUT("    -G Get Graph server status.\n");
    FZOUT("    -o Send info output to <info-output-path>.\n");
    FZOUT("    -F specify output format\n");
    FZOUT("       available formats:\n");
    FZOUT("       txt = basic ASCII text template\n")
    FZOUT("       html = HTML template\n");
}

bool fzserver_info::set_output_format(const std::string & cargs) {
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

    case 'o': {
        config.info_out_path = cargs;
        return true;
    }

    case 'F': {
        set_output_format(cargs);
        return true;
    }

    }

    return false;
}


/// Configure configurable parameters.
bool fzsi_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(info_out_path, "info_out_path", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(port_number, "port_number", parlabel, std::stoi(parvalue));
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

bool ping_server() {
    VERYVERBOSEOUT("Sending PING request to Graph server.\n");
    std::string response_str;
    if (!client_socket_shmem_request("PING", "127.0.0.1", fzsi.config.port_number, response_str)) {
        return standard_error("Communication error.", __func__);
    }

    if (response_str != "LISTENING") {
        return standard_error("Unknown response: "+response_str, __func__);
        VERBOSEOUT("Server stopping.\n");
        standard.completed_ok();
    }

    VERYVERBOSEOUT("Server response: LISTENING\n");
    return true;
}


void server_process_info(Graph_info_label_value_pairs & serverinfo) {
    std::string lockfile_status_str;
    std::string process_status_str;
    std::string lockfilecontent;

    int lockfile_status = check_and_read_lockfile(fzsi.lockfilepath, lockfilecontent);

    switch (lockfile_status) {

        case 0: {
            lockfile_status_str = "not found";
            process_status_str = "inactive";
            break;
        }

        case 1: {
            lockfile_status_str = "exists";
            jsonlite_label_value_pairs jsonpars = json_get_label_value_pairs_from_string(lockfilecontent);
            if (jsonpars.find("pid") == jsonpars.end()) {
                lockfile_status_str += " (MISSING PID)";
            } else {
                lockfile_status_str += " (PID="+jsonpars["pid"]+')';
                pid_t pid = atoi(jsonpars["pid"].c_str());
                int proc_status = get_process_status(pid);
                if (proc_status == 1) {
                    process_status_str = "active";
                } else {
                    process_status_str = "NOT FOUND (inactive)";
                }
            }
            break;
        }

        default: {
            lockfile_status_str = "ERROR";
            process_status_str = "UNKNOWN";
        }
    }

    serverinfo["lockfile"] = fzsi.lockfilepath;
    serverinfo["lock_status"] = lockfile_status_str;
    serverinfo["proc_status"] = process_status_str;
    if (ping_server()) {
        serverinfo["ping_status"] = "LISTENING";
    } else {
        serverinfo["ping_status"] = "OFF-LINE";
    }
}

int graph_server_status() {
    Graph * graph_ptr = graphmemman.find_Graph_in_shared_memory();
    if (!graph_ptr) {
        ADDERROR(__func__, "Memory resident Graph not found");
        FZERR("Memory resident Graph not found.\n");
        standard.exit(exit_general_error);
    }

    Graph_info_label_value_pairs meminfo;
    Graph_info_label_value_pairs graphinfo;
    Graph_info_label_value_pairs nodesinfo;
    Graph_info_label_value_pairs serverinfo;

    graphmemman.info(meminfo);
    Graph_Info(*graph_ptr, graphinfo);
    Nodes_statistics_pairs(Nodes_statistics(*graph_ptr), nodesinfo);
    server_process_info(serverinfo);

    meminfo.merge(graphinfo);
    meminfo.merge(nodesinfo);
    meminfo.merge(serverinfo);

    render_graph_server_status(meminfo);

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
