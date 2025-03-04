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
  /fz/ipport
  /fz/tzadjust
  /fz/ErrQ
  /fz/ReqQ
  /fz/_stop
  /fz/verbosity?set=<normal|quiet|very>

  /fz/db/mode
  /fz/db/mode?set=<run|log|sim>

  /fz/graph/logtime?<node-id>=<mins>[&T=<emulated-time>]

  /fz/graph/nodes/logtime?<node-id>=<mins>[&T=<emulated-time>]

  /fz/graph/nodes/<node-id>?skip=<<num>|toT>[&T=<emulated-time>]
  /fz/graph/nodes/<node-id>?targetdate=<YmdHM>&required=<hours>
  /fz/graph/nodes/<node-id>/completion?set=<ratio>
  /fz/graph/nodes/<node-id>/completion?add=[-]<ratio>|<minutes>m
  /fz/graph/nodes/<node-id>/required?set=<minutes>m|<hours>h
  /fz/graph/nodes/<node-id>/required?add=[-]<minutes>m|<hours>h
  /fz/graph/nodes/<node-id>/valuation.<raw|txt|html|json>
  /fz/graph/nodes/<node-id>/completion.<raw|txt|html|json>
  /fz/graph/nodes/<node-id>/required.<raw|txt|html|json>
  /fz/graph/nodes/<node-id>/targetdate.<raw|txt|html|json>
  /fz/graph/nodes/<node-id>/effectivetd.<raw|txt|html|json>
  /fz/graph/nodes/<node-id>/text.<raw|txt|html|json>
  /fz/graph/nodes/<node-id>/tdproperty.<raw|txt|html|json>
  /fz/graph/nodes/<node-id>/repeats.<raw|txt|html|json>
  /fz/graph/nodes/<node-id>/tdpattern.<raw|txt|html|json>
  /fz/graph/nodes/<node-id>/tdevery.<raw|txt|html|json>
  /fz/graph/nodes/<node-id>/tdspan.<raw|txt|html|json>
  /fz/graph/nodes/<node-id>/topics.<raw|txt|html|json>
  /fz/graph/nodes/<node-id>/in_NNLs.<raw|txt|html|json>

  /fz/graph/nodes/<node-id>/superiors/<add|remove>?<node-id>=
  /fz/graph/nodes/<node-id>/superiors/addlist?<superiors|<list-name>>=
  /fz/graph/nodes/<node-id>/dependencies/<add|remove>?<node-id>=
  /fz/graph/nodes/<node-id>/dependencies/addlist?<dependencies|<list-name>>=

  /fz/graph/namedlists/<list-name>.<json|raw>
  /fz/graph/namedlists/<list-name>?add=<node-id>[&FEATURES/MAXSIZE]
  /fz/graph/namedlists/<list-name>?remove=<node-id>
  /fz/graph/namedlists/<list-name>?delete=
  /fz/graph/namedlists/<list-name>?copy=<from_name>[&from_max=N][&to_max=M]
  /fz/graph/namedlists/<list-name>?move=<from_pos>&<up=|down=|to=<to_pos>>
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
        configuration variable 'www_file_root'. See the 'FILE SERVING'
        section below for the configured mapping.
Note E: URLs beginning with /cgi-bin are translated to server-side CGI
        calls, the responses of which are returned.

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
  CGIbg_run_as_user(cgiprog,uriargs,outfile)
    Run the cgiprog program as the same user as the one running fzserverpq
    in the background, using command line arguments obtained by parsing
    the string in uriargs that is given in URI argument format. The
    specified cgiprog can only run if it was included in the ampersand (&)
    delimited list of permitted programs in fzserverpq configuration
    variable 'predefined_CGIbg'. Note that the URI arguments are
    converted into shell arguments, and the program being called needs
    to be able to handle that. E.g. "cgioutput=on&earliest=2025.02.20"
    becomes "-cgioutput on -earliest 2025.02.20".

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

CGI FORWARDING
--------------

E.g. Calling 'http://<server-ip>:8090/cgi-bin/fzuistate.py' will
forward the request (with any arguments) to the fzuistate.py CGI script
and will return the result of that call.

At present, only GET CGI calls are supported, not POST CGI calls.

FILE SERVING
------------

The configured mapping applied to 'http:server-ip:port/path' requests (as
per Note D above) is:

)UTAIL";

std::string print_www_file_roots() {
    std::string res;
    for (const auto & [rootkey, pathbase] : fzs.config.www_file_root) {
        res += "  (/'"+rootkey+"') -> "+pathbase+'\n';
    }
    return res;
}

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzserverpq::fzserverpq() : formalizer_standard_program(false), config(*this), ga(*this, add_option_args, add_usage_top, true),
                           flowcontrol(flow_unknown), graph_ptr(nullptr), ReqQ(config.reqqfilepath) {
    add_option_args += "Gp:L:";
    add_usage_top += " [-G] [-p <port-number>] [-L <request-log>]";
    usage_tail.push_back(usage_tail_str_A); // root path mapping cannot be inserted here, because config is parsed later
}

Graph & fzserverpq::graph() {
    if (graph_ptr) return *graph_ptr;
    throw NoGraph_exception(" in fzserverpq.");
}

/**
 * Extract a map of recognized roots and target file paths from the 'www_file_root'
 * configuration string.
 * Note: This is a very simple format, e.g. "=/var/www/html&doc=/home/randalk/doc".
 *       This format does not support paths that contain ampersands or equal signs.
 *       Commas are also not allowed, because the super-simple JSON-lite function used
 *       for configuration parsing uses those to separate config lines.
 *       *** It could be extended to actual JSON if there is good cause to do so.
 * @param www_file_roots String containing encoded roots and paths.
 * @return A map of string pairs, one for each root and path pair.
 */
root_path_map_type parse_www_file_roots(const std::string & www_file_roots) {
    auto roots_paths_vec = split(www_file_roots, '&');
    root_path_map_type roots_paths_map;
    for (const auto & roots_paths_str : roots_paths_vec) {
        auto colon_pos = roots_paths_str.find('=');
        if (colon_pos == std::string::npos) {
            standard_exit_error(exit_bad_config_value, "Invalid www_file_root syntax: "+www_file_roots, __func__);
        }
        roots_paths_map.emplace(roots_paths_str.substr(0, colon_pos), roots_paths_str.substr(colon_pos+1));
    }
    return roots_paths_map;
}

std::vector<std::string> parse_predefined_CGIbg(const std::string & parvalue) {
    ERRTRACE;

    std::vector<std::string> CGIbg;
    if (parvalue.empty())
        return CGIbg;

    CGIbg = split(parvalue, '&');
    // for (auto & cgibg_str : cgibg_vec) {
    //     trim(cgibg_str);
    //     try {
    //         CGIbg.emplace_back(cgibg_str);
    //     } catch (ID_exception idexception) {
    //         standard_exit_error(exit_bad_request_data, "invalid CGI name (" + cgibg_str + ")\n" + idexception.what(), __func__);
    //     }
    // }

    return CGIbg;
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
    CONFIG_TEST_AND_SET_PAR(default_to_localhost, "default_to_localhost", parlabel, (parvalue == "true"));
    CONFIG_TEST_AND_SET_PAR(port_number, "port_number", parlabel, std::stoi(parvalue));
    CONFIG_TEST_AND_SET_PAR(www_file_root, "www_file_root", parlabel, parse_www_file_roots(parvalue));
    CONFIG_TEST_AND_SET_PAR(request_log, "request_log", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(predefined_CGIbg, "predefined_CGIbg", parlabel, parse_predefined_CGIbg(parvalue)); // E.g. from "fzbackup-mirror-to-github.sh,fzinfo"
    CONFIG_TEST_AND_SET_PAR(graphconfig.persistent_NNL, "persistent_NNL", parlabel, (parvalue != "false"));    
    CONFIG_TEST_AND_SET_PAR(graphconfig.tzadjust_seconds, "timezone_offset_hours", parlabel, -3600*std::stoi(parvalue));
    CONFIG_TEST_AND_SET_PAR(graphconfig.batchmode_constraints_active, "batchmode_constraints_active", parlabel, (parvalue != "false"));
    CONFIG_TEST_AND_SET_PAR(graphconfig.T_suspiciously_large, "T_suspiciously_large", parlabel, std::stol(parvalue));
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
    // Now the mapping should be available:
    usage_tail.push_back(print_www_file_roots());
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
    fzs.graph_ptr = fzs.ga.request_Graph_copy(true, &fzs.config.graphconfig);
    if (!fzs.graph_ptr) {
        standard_error("Unable to load Graph", __func__);
        RETURN_AFTER_UNLOCKING;
    }

    VERYVERBOSEOUT(graphmemman.info_str());
    VERYVERBOSEOUT(Graph_Info_str(*fzs.graph_ptr));

    if (fzs.config.default_to_localhost) {
        VERYVERBOSEOUT("Configured to default to localhost. Local server access only.");
        fzs.ipaddrstr = "127.0.0.1";
    } else {
        if (!find_server_address(fzs.ipaddrstr)) {
            standard_error("Unable to determine server IP address (launching only for localhost)", __func__);
            VERYVERBOSEOUT("No Internet. Launching server only for localhost access.");
            //RETURN_AFTER_UNLOCKING;
            fzs.ipaddrstr = "127.0.0.1";
        }
    }

    fzs.graph_ptr->set_server_IPaddr(fzs.ipaddrstr);
    fzs.graph_ptr->set_server_port(fzs.config.port_number);
    VERYVERBOSEOUT("The server will be available on:\n  localhost:"+fzs.graph_ptr->get_server_port_str()+"\n  "+fzs.graph_ptr->get_server_full_address()+'\n');
    std::string serveraddresspath(FORMALIZER_ROOT "/server_address");
    fzs.ipaddrstr = fzs.graph_ptr->get_server_full_address();
    if (!string_to_file(serveraddresspath, fzs.ipaddrstr)) {
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
