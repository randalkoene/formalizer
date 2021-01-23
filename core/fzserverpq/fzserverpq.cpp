// Copyright 2020 Randal A. Koene
// License TBD

/**
 * fzserverpq is the Postgres-compatible version of the C++ implementation of the Formalizer data server.
 * 
 * For more information see:
 * https://docs.google.com/document/d/1rYPFgzFgjkF1xGx3uABiXiaDR5sfmOzqYQRqSntcyyY/edit#heading=h.tarhfe395l5v
 * https://trello.com/c/S7SZUyeU
 * 
 * For more about this, see https://trello.com/c/S7SZUyeU.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Server:Graph:Postgres"

// std
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>

// core
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "proclock.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "Graphmodify.hpp"
#include "Graphpostgres.hpp"

// local
#include "fzserverpq.hpp"
#include "shm_server_handlers.hpp"
#include "tcp_server_handlers.hpp"

using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzserverpq fzs;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzserverpq::fzserverpq() : formalizer_standard_program(false), config(*this), ga(*this, add_option_args, add_usage_top, true),
                           flowcontrol(flow_unknown), graph_ptr(nullptr), ReqQ(config.reqqfilepath) {
    add_option_args += "Gp:L:";
    add_usage_top += " [-G] [-p <port-number>] [-L <request-log>]";
    usage_tail.push_back(
        "The limited number of command line options are generally used only to start\n"
        "the server. The server interacts with clients through the combination of a\n"
        "TCP socket at the specified port and shared memory data exchanges.\n"
        "\n"
        "API AT TCP TOP LEVEL\n"
        "--------------------\n"
        "The following are recognized requests:\n"
        "\n"
        "  'GET ' followed by a path such as '/fz/status', browser HTTP-like request\n"
        "  'PATCH ' is much like GET, but used to request a modification\n"
        "  'FZ ' followed by request directives, serialized data request\n"
        "  'STOP' stops and terminates the server (`fzgraph -z` sends this)\n"
        "  'PING' requests a readiness response from the server\n"
        "\n"
        "Any other string read at the socket is interpreted as the name of a shared\n"
        "memory segment that contains a Graph_modifications object named 'graphmod'\n"
        "with a stack of modification requests.\n"
        "\n"
        "API USING 'GET' AND 'PATCH' REQUESTS\n"
        "------------------------------------\n"
        "The '/fz/' paths are special Formalizer handles for which the response\n"
        "content in HTML format is generated dynamically. (This is somewhat analogous\n"
        "to file content read at /proc.)\n"
        "\n"
        "The GET/PATCH port API includes the following:\n"
        "\n"
        "  /fz/status\n"
        "  /fz/ErrQ\n"
        "  /fz/ReqQ\n"
        "  /fz/_stop\n"
        "  /fz/verbosity?set=<normal|quiet|very>\n"
        "  /fz/db/mode\n"
        "  /fz/db/mode?set=<run|log|sim>\n"
        "  /fz/graph/logtime?<node-id>=<mins>[&T=<emulated-time>]\n"
        "  /fz/graph/nodes/logtime?<node-id>=<mins>[&T=<emulated-time>]\n"
        "  /fz/graph/namedlists/<list-name>?add=<node-id>[&FEATURES/MAXSIZE]\n"
        "  /fz/graph/namedlists/<list-name>?remove=<node-id>\n"
        "  /fz/graph/namedlists/<list-name>?delete=\n"
        "  /fz/graph/namedlists/<list-name>?copy=<from_name>[&from_max=N][&to_max=M]\n"
        "  /fz/graph/namedlists/_select?id=<node-id>\n"
        "  /fz/graph/namedlists/_recent?id=<node-id>\n"
        "  /fz/graph/namedlists/_shortlist\n"
        "  /fz/graph/namedlists/_set?persistent=\n"
        "  /fz/graph/namedlists/_reload\n"
        "\n"
        "Note A: The 'persistent' switch is only available through port requests and\n"
        "        through the configuration file. There is no command line option.\n"
        "Note B: The <from_name> '_incomplete' copies Nodes from the effective\n"
        "        targetdate sorted list of incomplete Nodes.\n"
        "Note C: Both <list-name>?add and <list-name>?copy can receive optional\n"
        "        arguments to specify FEATURES and MAXSIZE if the <list-name> is new.\n"
        "        The available specifiers are:\n"
        "          [&maxsize=N], [&unique=true|false], [&fifo=true|false],"
        "          [&prepend=true|false]\n"
        "Note D: Using the direct TCP-port API, absolute URLs are translated so that\n"
        "        the root '/' is at the actual filesystem location specified by the\n"
        "        configuration variable 'www_file_root'. It presently evaluates to\n"
        "        "+fzs.config.www_file_root+".\n"
        "\n"
        "API USING 'FZ' REQUEST\n"
        "----------------------\n"
        "The API requests are in many ways similar to those used with 'GET' or 'PATCH',\n"
        "but the response is a serialized data format intended for rapid communication,\n"
        "not for formatted display.\n"
        "\n"
        "Recognized requests are:\n"
        "\n"
        "  NNLlen(list-name)   Returns the number of Nodes in NNL 'list-name' (or 0).\n"
        "\n"
        "FZ serialized data requests can be batched via simple concatenation, e.g.:\n"
        "  FZ NNLlen(superiors);NNLlen(dependencies)\n"
        "\n"
        "Extra spacing and a terminating semicolon are possible and ignored.\n"
        "\n");
}

/**
 * Configure configurable parameters.
 * 
 * Note that this can throw exceptions, such as std::invalid_argument when a
 * conversion was not poossible. That is a good precaution against otherwise
 * hard to notice bugs in configuration files.
 */
bool fzs_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    // *** You could also implement try-catch here to gracefully report problems with configuration files.
    CONFIG_TEST_AND_SET_PAR(port_number, "port_number", parlabel, std::stoi(parvalue));
    CONFIG_TEST_AND_SET_PAR(persistent_NNL, "persistent_NNL", parlabel, (parvalue != "false"));
    CONFIG_TEST_AND_SET_PAR(www_file_root, "www_file_root", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(request_log, "request_log", parlabel, parvalue);
    //CONFIG_TEST_AND_SET_FLAG(example_flagenablefunc, example_flagdisablefunc, "exampleflag", parlabel, parvalue);
    CONFIG_PAR_NOT_FOUND(parlabel);
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzserverpq::usage_hook() {
    ga.usage_hook();
    FZOUT("    -G Load Graph and stay resident in memory\n"
          "    -p Specify <port-number> on which the sever will listen\n"
          "    -L Log requests received in <request-log> (or STDOUT), currently set\n"
          "       to: "+ReqQ.get_errfilepath()+'\n');
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
bool fzserverpq::options_hook(char c, std::string cargs) {
    if (ga.options_hook(c,cargs))
        return true;

    switch (c) {

    case 'G': {
        flowcontrol = flow_resident_graph;
        return true;
    }

    case 'p': {
        config.port_number = std::stoi(cargs);
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
void fzserverpq::init_top(int argc, char *argv[]) {
    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
    ReqQ.disable_caching();
    ReqQ.enable_timestamping();
    ReqQ.set_errfilepath(config.request_log);
}

void load_Graph_and_stay_resident() {
    ERRTRACE;

    // create the lockfile to indicate the presence of this server
    int lockfile_ret = check_and_make_lockfile(fzs.lockfilepath, "");
    if (lockfile_ret != 0) {
        if (lockfile_ret == 1) {
            standard_exit_error(exit_general_error, "The lock file already exists at "+std::string(fzs.lockfilepath)+".\nAnother instance of this server may be running.", __func__);
        }
        standard_exit_error(exit_general_error, "Unable to make lockfile at "+std::string(fzs.lockfilepath), __func__);
    }      

    #define RETURN_AFTER_UNLOCKING { \
        if (remove_lockfile(fzs.lockfilepath) != 0) { \
            standard_error("Unable to remove lockfile before exiting", __func__); \
        } \
        return; \
    }

    // Load the graph and make the pointer available for handlers to use.
    fzs.graph_ptr = fzs.ga.request_Graph_copy(true, fzs.config.persistent_NNL);
    if (!fzs.graph_ptr) {
        standard_error("Unable to load Graph", __func__);
        RETURN_AFTER_UNLOCKING;
    }

    VERYVERBOSEOUT(graphmemman.info_str());
    VERYVERBOSEOUT(Graph_Info_str(*fzs.graph_ptr));

    std::string ipaddrstr;
    if (!find_server_address(ipaddrstr)) {
        standard_error("Unable to determine server IP address", __func__);
        RETURN_AFTER_UNLOCKING;   
    }

    fzs.graph_ptr->set_server_IPaddr(ipaddrstr);
    fzs.graph_ptr->set_server_port(fzs.config.port_number);
    VERYVERBOSEOUT("The server will be available on:\n  localhost:"+fzs.graph_ptr->get_server_port_str()+"\n  "+fzs.graph_ptr->get_server_full_address()+'\n');
    std::string serveraddresspath(FORMALIZER_ROOT "/server_address");
    ipaddrstr = fzs.graph_ptr->get_server_full_address();
    if (!string_to_file(serveraddresspath, ipaddrstr)) {
        standard_error("Unable to store server IP address in ", __func__);
        RETURN_AFTER_UNLOCKING;
    }

    server_socket_listen(fzs.config.port_number, fzs);

    RETURN_AFTER_UNLOCKING;
}

int main(int argc, char *argv[]) {
    ERRTRACE;
    fzs.init_top(argc, argv);

    switch (fzs.flowcontrol) {

    case flow_resident_graph: {
        load_Graph_and_stay_resident();
        break;
    }

    default: {
        fzs.print_usage();
    }

    }

    return standard.completed_ok();
}
