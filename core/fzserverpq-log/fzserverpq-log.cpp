// Copyright 20210501 Randal A. Koene
// License TBD

/**
 * fzserverpq-log is the Postgres-compatible version of the C++ implementation of the Formalizer Log data server.
 * 
 * For more about this, see https://trello.com/c/6obEcUJS.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Server:Log:Postgres"

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

// local
#include "version.hpp"
#include "fzserverpq-log.hpp"
// *** #include "shm_server_handlers.hpp"
// *** #include "tcp_server_handlers.hpp"


using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzserverpqlog fzsl;

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
  'STOP'   stops and terminates the server (`fzlog -z` sends this)
  'PING'   requests a readiness response from the server

Any other string read at the socket is interpreted as the name of a shared
memory segment that contains a Log_modifications object named 'logmod'
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
  /fz/log/logtime?<node-id>=<mins>[&T=<emulated-time>]
  /fz/log/chunks/logtime?<node-id>=<mins>[&T=<emulated-time>]
  /fz/log/chunks/<chunk-id>?(TBD)
  /fz/log/chunks/<chunk-id>/(TBD)
  /fz/log/namedlists/<list-name>?add=<entry-id>[&FEATURES/MAXSIZE]
  /fz/log/namedlists/<list-name>?remove=<entry-id>
  /fz/log/namedlists/<list-name>?delete=
  /fz/log/namedlists/<list-name>?copy=<from_name>[&from_max=N][&to_max=M]
  /fz/log/namedlists/_select?id=<entry-id>
  /fz/log/namedlists/_set?persistent=
  /fz/log/namedlists/_reload

Note A: The 'persistent' switch is only available through port requests and
        through the configuration file. There is no command line option.
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

  NELlen(list-name)
    Returns the number of Entries in NEL 'list-name' (or 0).
  entries_match(node=node-id,lower_t_open=202101292346,upper_t_open=202101292346)
    Returns a comma-separated list of Entries matching specified conditions.
    Recognized filter arguments are: node, chunkminutes, lower_chunkminutes,
    upper_chunkminutes, t_open, lower_t_open, upper_t_open.
    The special t_open times 'NOW', 'MIN' and 'MAX' are also recognized.
    The singular (not lower_ or upper_) arguments can take one value that
    is applied to both bounds, or can take a range, e.g. '[MIN,NOW]'.
  NELadd_match(a_list,t_open=[202101292346,202101292346])
    Adds to `a_list` the Entries that match specified conditions and returns
    the number that were added.
  NELedit_nodes(extreme_intervals,node,20171128203820.1)
    Edit the specified parameter (e.g. node) to the specified value
    (e.g. 20171128203820.1) on all Entries in the specified List (e.g.
    extreme_intervals).

FZ serialized data requests can be batched via simple concatenation, e.g.:

  FZ NELlen(burningman);NNLlen(workshops)

Extra spacing and a terminating semicolon are possible and ignored.

Any TCP client, TCP library function, or a special-purpose Formalizer query
utility can be used to make FZ requests. Some examples:

  |~$ nc 127.0.0.1 8091
  |FZ NELlen(burningman);NELlen(workshops)

  |~$ fzquerypq -Z -p 8091 'NNLlen(dependencies);NNLlen(superiors)'

  |~$ python3
  |>> import socket
  |>> s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  |>> s.connect(("127.0.0.1",8091))
  |>> s.send('NELlen(burningman);NELlen(workshops)'.encode()) 
  |>> data = ''
  |>> data = s.recv(1024).decode()
)UTAIL";


/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzserverpqlog::fzserverpqlog() : formalizer_standard_program(false), config(*this), flowcontrol(flow_unknown), log_ptr(nullptr), ReqQ(config.reqqfilepath) { //ga(*this, add_option_args, add_usage_top)
    add_option_args += "lp:L:";
    add_usage_top += " [-l] [-p <port-number>] [-L <request-log>]";
    usage_tail.push_back(usage_tail_str_A+fzsl.config.www_file_root+usage_tail_str_B);
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzserverpqlog::usage_hook() {
    //ga.usage_hook();
    FZOUT("    -l Load Log and stay resident in memory\n"
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
bool fzserverpqlog::options_hook(char c, std::string cargs) {
    //if (ga.options_hook(c,cargs))
    //    return true;

    switch (c) {

    case 'l': {
        flowcontrol = flow_resident_log;
        return true;
    }

    case 'p': {
        config.port_number = std::stoi(cargs);
        return true;
    }

    }

    return false;
}


/// Configure configurable parameters.
bool fzsl_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    // *** You could also implement try-catch here to gracefully report problems with configuration files.
    CONFIG_TEST_AND_SET_PAR(port_number, "port_number", parlabel, std::stoi(parvalue));
    CONFIG_TEST_AND_SET_PAR(persistent_NEL, "persistent_NEL", parlabel, (parvalue != "false"));
    CONFIG_TEST_AND_SET_PAR(www_file_root, "www_file_root", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(request_log, "request_log", parlabel, parvalue);
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
void fzserverpqlog::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
    ReqQ.disable_caching();
    ReqQ.enable_timestamping();
    ReqQ.set_errfilepath(config.request_log);
}

/* (uncomment to include access to memory-resident Graph)
Graph & fzgraphsearch::graph() {
    ERRTRACE;
    if (!graphmemman.get_Graph(graph_ptr)) {
        standard_exit_error(exit_resident_graph_missing, "Memory resident Graph not found.", __func__);
    }
    return *graph_ptr;
}
*/

void load_Log_and_stay_resident() {
    ERRTRACE;

    // create the lockfile to indicate the presence of this server
    int lockfile_ret = check_and_make_lockfile(fzsl.lockfilepath, "");
    if (lockfile_ret != 0) {
        if (lockfile_ret == 1) {
            standard_exit_error(exit_general_error, "The lock file already exists at "+std::string(fzsl.lockfilepath)+".\nAnother instance of this server may be running.", __func__);
        }
        standard_exit_error(exit_general_error, "Unable to make lockfile at "+std::string(fzsl.lockfilepath), __func__);
    }      

    #define RETURN_AFTER_UNLOCKING { \
        if (remove_lockfile(fzsl.lockfilepath) != 0) { \
            standard_error("Unable to remove lockfile before exiting", __func__); \
        } \
        return; \
    }

    // Load the log and make the pointer available for handlers to use.
    fzsl.log_ptr = fzsl.ga.request_Log_copy(true, fzsl.config.persistent_NEL);
    if (!fzsl.log_ptr) {
        standard_error("Unable to load Log", __func__);
        RETURN_AFTER_UNLOCKING;
    }

    VERYVERBOSEOUT(logmemman.info_str());
    VERYVERBOSEOUT(Log_Info_str(*fzsl.log_ptr));

    std::string ipaddrstr;
    if (!find_server_address(ipaddrstr)) {
        standard_error("Unable to determine server IP address", __func__);
        RETURN_AFTER_UNLOCKING;   
    }

    fzsl.log_ptr->set_server_IPaddr(ipaddrstr);
    fzsl.log_ptr->set_server_port(fzsl.config.port_number);
    VERYVERBOSEOUT("The server will be available on:\n  localhost:"+fzsl.log_ptr->get_server_port_str()+"\n  "+fzsl.log_ptr->get_server_full_address()+'\n');
    std::string serveraddresspath(FORMALIZER_ROOT "/logserver_address");
    ipaddrstr = fzsl.log_ptr->get_server_full_address();
    if (!string_to_file(serveraddresspath, ipaddrstr)) {
        standard_error("Unable to store server IP address in ", __func__);
        RETURN_AFTER_UNLOCKING;
    }

    server_socket_listen(fzsl.config.port_number, fzsl);

    RETURN_AFTER_UNLOCKING;
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzsl.init_top(argc, argv);

    FZOUT("\nThis is a stub.\n\n");
    key_pause();

    switch (fzsl.flowcontrol) {

    case flow_resident_log: {
        load_Log_and_stay_resident();
        break;
    }

    default: {
        fzsl.print_usage();
    }

    }

    return standard.completed_ok();
}
