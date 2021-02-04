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

static const char usage_tail_str_A[] = R"UTAIL(
The limited number of command line options are generally used only to start
the server. The server interacts with clients through the combination of a
TCP socket at the specified port and shared memory data exchanges.

API AT TCP TOP LEVEL
--------------------
The following are recognized requests:

  'GET '   followed by a path such as '/fz/status', browser HTTP-like request
  'PATCH ' is much like GET, but used to request a modification
  'FZ '    followed by request directives, serialized data request
  'STOP'   stops and terminates the server (`fzgraph -z` sends this)
  'PING'   requests a readiness response from the server

Any other string read at the socket is interpreted as the name of a shared
memory segment that contains a Graph_modifications object named 'graphmod'
with a stack of modification requests.

API USING 'GET' AND 'PATCH' REQUESTS
------------------------------------
The '/fz/' paths are special Formalizer handles for which the response
content in HTML format is generated dynamically. (This is somewhat analogous
to file content read at /proc.)

The GET/PATCH port API includes the following:

  /fz/status
  /fz/ErrQ
  /fz/ReqQ
  /fz/_stop
  /fz/verbosity?set=<normal|quiet|very>
  /fz/db/mode
  /fz/db/mode?set=<run|log|sim>
  /fz/graph/logtime?<node-id>=<mins>[&T=<emulated-time>]
  /fz/graph/nodes/logtime?<node-id>=<mins>[&T=<emulated-time>]
  /fz/graph/nodes/<node-id>?skip=<<num>|toT>[&T=<emulated-time>]
  /fz/graph/namedlists/<list-name>?add=<node-id>[&FEATURES/MAXSIZE]
  /fz/graph/namedlists/<list-name>?remove=<node-id>
  /fz/graph/namedlists/<list-name>?delete=
  /fz/graph/namedlists/<list-name>?copy=<from_name>[&from_max=N][&to_max=M]
  /fz/graph/namedlists/_select?id=<node-id>
  /fz/graph/namedlists/_recent?id=<node-id>
  /fz/graph/namedlists/_shortlist
  /fz/graph/namedlists/_set?persistent=
  /fz/graph/namedlists/_reload

Note A: The 'persistent' switch is only available through port requests and
        through the configuration file. There is no command line option.
Note B: The <from_name> '_incomplete' copies Nodes from the effective
        targetdate sorted list of incomplete Nodes.
Note C: Both <list-name>?add and <list-name>?copy can receive optional
        arguments to specify FEATURES and MAXSIZE if the <list-name> is new.
        The available specifiers are:
          [&maxsize=N], [&unique=true|false], [&fifo=true|false],
          [&prepend=true|false]
Note D: Using the direct TCP-port API, absolute URLs are translated so that
        the root '/' is at the actual filesystem location specified by the
        configuration variable 'www_file_root'. It presently evaluates to
        )UTAIL";
static const char usage_tail_str_B[] = R"UTAIL(.

API USING 'FZ' REQUEST
----------------------
The API requests are in many ways similar to those used with 'GET' or 'PATCH',
but the response is a serialized data format intended for rapid communication,
not for formatted display.

Recognized requests are:

  NNLlen(list-name)
    Returns the number of Nodes in NNL 'list-name' (or 0).
  nodes_match(tdproperty=variable,targetdate=202101292346)
    Returns a comma-separated list of Nodes matching specified conditions.
    Recognized filter arguments are: completion, lower_completion,
    upper_completion, tdproperty, tdproperty_A, tdproperty_B,
    targetdate, lower_targetdate, upper_targetdate, repeats.
    The special target dates 'NOW', 'MIN' and 'MAX' are also recognized.
    The singular (not lower_ or upper_) arguments can take one value that
    is applied to both bounds, or can take a range, e.g. '[MIN,NOW]'.
  NNLadd_match(a_list,tdproperty=[fixed-exact],repeats=false)
    Adds to `a_list` the Nodes that match specified conditions and returns
    the number that were added.
  NNLedit_nodes(passed_fixed,tdproperty,variable)
    Edit the specified parameter (e.g. tdproperty) to the specified value
    (e.g. variable) on all Nodes in the specified List (e.g. passed_fixed).

FZ serialized data requests can be batched via simple concatenation, e.g.:

  FZ NNLlen(superiors);NNLlen(dependencies)

Extra spacing and a terminating semicolon are possible and ignored.

Any TCP client, TCP library function, or a special-purpose Formalizer query
utility can be used to make FZ requests. Some examples:

  |~$ nc 127.0.0.1 8090
  |FZ NNLlen(dependencies);NNLlen(superiors)

  |~$ fzquerypq -Z 'NNLlen(dependencies);NNLlen(superiors)'

  |~$ python3
  |>> import socket
  |>> s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  |>> s.connect(("127.0.0.1",8090))
  |>> s.send('NNLlen(dependencies);NNLlen(superiors)'.encode()) 
  |>> data = ''
  |>> data = s.recv(1024).decode()
)UTAIL";

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzserverpq::fzserverpq() : formalizer_standard_program(false), config(*this), ga(*this, add_option_args, add_usage_top, true),
                           flowcontrol(flow_unknown), graph_ptr(nullptr), ReqQ(config.reqqfilepath) {
    add_option_args += "Gp:L:";
    add_usage_top += " [-G] [-p <port-number>] [-L <request-log>]";
    usage_tail.push_back(usage_tail_str_A+fzs.config.www_file_root+usage_tail_str_B);
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
