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
#include <filesystem>

// core
#include "error.hpp"
#include "proclock.hpp"
#include "jsonlite.hpp"
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
    add_option_args += "GPMo:F:";
    add_usage_top += " [-G] [-P] [-M] [-o <info-output-path>] [-F txt|html|json|csv|raw]";
    //usage_head.push_back("Description at the head of usage information.\n");
    usage_tail.push_back("The <info-output-path> STDOUT sends to standard out and is the default.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzserver_info::usage_hook() {
    //ga.usage_hook();
    FZOUT("    -G Get Graph server status.\n"
          "    -P Ping the server.\n"
          "    -M POSIX named shared memory blocks.\n"
          "    -o Send info output to <info-output-path>.\n"
          "    -F specify output format for Graph server status\n"
          "       available formats:\n"
          "       txt = basic ASCII text template\n"
          "       html = HTML template\n"
          "       json = JSON template\n"
          "       csv = Comma separated values\n"
          "       raw = raw data rows\n");
}

bool fzserver_info::set_output_format(const std::string & cargs) {
    ERRTRACE;
    if (cargs == "html") {
        output_format = output_html;
        return true;
    }
    if (cargs == "txt") {
        output_format = output_txt;
        return true;
    }
    if (cargs == "json") {
        output_format = output_json;
        return true;
    }
    if (cargs == "csv") {
        output_format = output_csv;
        return true;
    }
    if (cargs == "raw") {
        output_format = output_raw;
        return true;
    }
    return standard_exit_error(exit_command_line_error, "Unknown output format: "+cargs, __func__);
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

    case 'P': {
        flowcontrol = flow_ping_server;
        return true;
    }

    case 'M': {
        flowcontrol = flow_shared_memory;
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
    CONFIG_TEST_AND_SET_PAR(port_number, "port_number", parlabel, std::stoi(parvalue)); // can be overridden in graph_server_status()
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

Graph & fzserver_info::graph() {
    ERRTRACE;
    if (!graphmemman.get_Graph(graph_ptr)) {
        standard_exit_error(exit_resident_graph_missing, "Memory resident Graph not found.", __func__);
    }
    return *graph_ptr;
}

bool ping_server() {
    VERYVERBOSEOUT("Sending PING request to Graph server.\n");
    std::string response_str;
    // *** Could replace the hard-coded localhost with fzsi.graph().get_server_IPaddr_str() if
    //     this tool should be usable on a different machine, corresponding with a remote
    //     Formalizer server.
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
        serverinfo["ping_status"] = "OFFLINE";
    }
    serverinfo["server_address"] = fzsi.graph().get_server_full_address();
}

int graph_server_status() {
    ERRTRACE;

    Graph_info_label_value_pairs meminfo;
    Graph_info_label_value_pairs graphinfo;
    Graph_info_label_value_pairs nodesinfo;
    Graph_info_label_value_pairs serverinfo;

    Graph_Info(fzsi.graph(), graphinfo); // this one first - fzsi.graph() sets the active shared memory
    if (!fzsi.graph().get_server_IPaddr().empty()) { // address and port info cached by server overrides config
        fzsi.config.port_number = fzsi.graph().get_server_port();
    }
    graphmemman.info(meminfo);
    Nodes_statistics_pairs(Nodes_statistics(fzsi.graph()), nodesinfo);
    server_process_info(serverinfo);

    meminfo.merge(graphinfo);
    meminfo.merge(nodesinfo);
    meminfo.merge(serverinfo);
    meminfo["shm_blocks"] = render_shared_memory_blocks(named_POSIX_shared_memory_blocks());

    if (!render_graph_server_status(meminfo)) {
        return standard_exit_error(exit_file_error, "Unable to deliver rendered Graph server status.", __func__);
    }

    return standard.completed_ok();
}

int ping_server_response() {
    ERRTRACE;

    std::string rendered_str;
    if (ping_server()) {
        rendered_str = "LISTENING";
    } else {
        rendered_str = "OFFLINE";
    }

    if (!output_response(rendered_str)) {
        return standard_exit_error(exit_file_error, "Unable to deliver server ping status.", __func__);
    }

    return standard.completed_ok();
}

POSIX_shm_data_vec named_POSIX_shared_memory_blocks() {
    POSIX_shm_data_vec shmblocksvec;
    std::string shmpath = "/dev/shm";
    for (const auto & entry : std::filesystem::directory_iterator(shmpath)) {
        shmblocksvec.emplace_back(entry.path().filename().string(), entry.file_size());
    }
    return shmblocksvec;
}

int shared_memory_blocks() {
    ERRTRACE;

    auto shmblocksvec = named_POSIX_shared_memory_blocks();

    std::string rendered_str = render_shared_memory_blocks(shmblocksvec);

    if (!output_response(rendered_str)) {
        return standard_exit_error(exit_file_error, "Unable to deliver POSIX named shared memory blocks info.", __func__);
    }

    return standard.completed_ok(); 
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzsi.init_top(argc, argv);

    switch (fzsi.flowcontrol) {

    case flow_graph_server: {
        return graph_server_status();
    }

    case flow_ping_server: {
        return ping_server_response();
    }

    case flow_shared_memory: {
        return shared_memory_blocks();
    }

    default: {
        fzsi.print_usage();
    }

    }

    return standard.completed_ok();
}
