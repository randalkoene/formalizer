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
#include "proclock.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "Graphmodify.hpp"

// local
#include "fzserverpq.hpp"

using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzserverpq fzs;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzserverpq::fzserverpq() : formalizer_standard_program(false), config(*this), ga(*this, add_option_args, add_usage_top, true),
                           flowcontrol(flow_unknown), graph_ptr(nullptr) {
    add_option_args += "Gp:";
    add_usage_top += " [-G] [-p <port-number>]";
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
    //CONFIG_TEST_AND_SET_FLAG(example_flagenablefunc, example_flagdisablefunc, "exampleflag", parlabel, parvalue);
    CONFIG_PAR_NOT_FOUND(parlabel);
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzserverpq::usage_hook() {
    ga.usage_hook();
    FZOUT("    -G Load Graph and stay resident in memory\n");
    FZOUT("    -p Specify <port-number> on which the sever will listen\n");
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
}

void fzserverpq::handle_request_with_data_share(int new_socket, const std::string & segment_name) {
    VERYVERBOSEOUT("Received Graph request with data share "+segment_name+".\n");
    if (handle_request_stack(segment_name)) {
        // send back results
        VERYVERBOSEOUT("Sending response with successful results data.\n");
        std::string response_str("RESULTS");
        send(new_socket, response_str.c_str(), response_str.size()+1, 0);
    } else {
        // send back error
        VERYVERBOSEOUT("Sending error response. An 'error' data structure may or may not exist.\n");
        std::string response_str("ERROR");
        send(new_socket, response_str.c_str(), response_str.size()+1, 0);
    }
    graphmemman.forget_manager(segment_name); // remove shared memory references that likely become stale when client is done
}

void fzserverpq::handle_special_purpose_request(int new_socket, const std::string & request_str) {
    VERYVERBOSEOUT("Received Special Purpose request "+request_str+".\n");
    auto requestvec = split(request_str,' ');
    if (requestvec.size()<2) {
        VERYVERBOSEOUT("Missing request. Responding with: 400 Bad Request.\n");
        std::string response_str("400 Bad Request\n");
        send(new_socket, response_str.c_str(), response_str.size()+1, 0);
        return;
    }

    if (requestvec[1].substr(0,4) == "/fz/") { // (one type of) recognized Formalizer special purpose request
        if (requestvec[1].substr(4) == "status") {
            VERYVERBOSEOUT("Stutus request received. Responding.\n");
            std::string response_str("HTTP/1.1 200 OK\nServer: aether\nContent-Type: text/html;charset=UTF-8\nContent-Length: ");
            std::string status_html("<html>\n<body>\nServer status: LISTENING\n</body>\n</html>\n");
            response_str += std::to_string(status_html.size()) + "\n\n" + status_html;
            send(new_socket, response_str.c_str(), response_str.size()+1, 0);
            return;
        }
    }

    // no known request encountered and handled
    VERYVERBOSEOUT("Request type is unrecognized. Responding with: 400 Bad Request.\n");
    std::string response_str("400 Bad Request\n");
    send(new_socket, response_str.c_str(), response_str.size()+1, 0);
}

/// See if a Node ID key is one of the Nodes being added in this request.
bool node_id_in_request_stack(Graph_modifications & graphmod, const Node_ID_key & nkey) {
    for (const auto & gmoddata : graphmod.data) {
        if (gmoddata.request == graphmod_add_node) {
            if (gmoddata.node_ptr) {
                if (gmoddata.node_ptr->get_id().key() == nkey)
                    return true;
            }
        }
    }
    return false;
}

/**
 * Ensure that all of the requests contain valid data.
 * 
 * Otherwise reject the whole stack to avoid partial changes.
 * For more information, see https://trello.com/c/FQximby2/174-fzgraphedit-adding-new-nodes-to-the-graph-with-initial-edges#comment-5f8faf243d74b8364fac7739.
 * 
 * @param graphmod A requests stack with one or more requests.
 * @param segname Shared segment name where any error response data should be delivered.
 * @return True if everything is valid, false if the stack should be rejected.
 */
bool request_stack_valid(Graph_modifications & graphmod, std::string segname) {
    if (!fzs.graph_ptr) {
        prepare_error_response(segname, exit_resident_graph_missing, "Memory resident Graph not available for request validation");
        return false;
    }

    for (const auto & gmoddata : graphmod.data) {

        switch(gmoddata.request) {

            case graphmod_add_node: {
                // confirm that the proposed Node ID is available.
                if (!gmoddata.node_ptr) {
                    prepare_error_response(segname, exit_missing_data, "Missing Node data in add node request");
                    return false;
                }
                if (fzs.graph_ptr->Node_by_id(gmoddata.node_ptr->get_id().key())) {
                    prepare_error_response(segname, exit_bad_request_data, "Proposed Node ID ("+gmoddata.node_ptr->get_id_str()+") for add node request already exists");
                    return false;
                }

                // confirm that the supplied topic tags are all known
                if (!fzs.graph_ptr->topics_exist(gmoddata.node_ptr->get_topics())) {
                    prepare_error_response(segname, exit_bad_request_data, "Unknown Topic ID(s) in add node request");
                    return false;
                }
                // *** does anything else need to be validated?
                break;
            }

            case graphmod_add_edge: {
                // confirm that both dep and sup Node IDs exist in either the Graph or as nodes being added
                if (!gmoddata.edge_ptr) {
                    prepare_error_response(segname, exit_missing_data, "Missing Edge date in add node request");
                    return false;
                }
                if ((!fzs.graph_ptr->Node_by_id(gmoddata.edge_ptr->get_dep_key())) && (!node_id_in_request_stack(graphmod, gmoddata.edge_ptr->get_dep_key()))) {
                    prepare_error_response(segname, exit_bad_request_data, "Source (dependency) Node ID ("+gmoddata.edge_ptr->get_dep_str()+") not found in Graph for add edge request");
                    return false;
                }
                if ((!fzs.graph_ptr->Node_by_id(gmoddata.edge_ptr->get_sup_key())) && (!node_id_in_request_stack(graphmod, gmoddata.edge_ptr->get_sup_key()))) {
                    prepare_error_response(segname, exit_bad_request_data, "Destination (superior) Node ID ("+gmoddata.edge_ptr->get_sup_str()+") not found in Graph for add edge request");
                    return false;
                }
                break;
            }

            default: {
                prepare_error_response(segname, exit_unknown_option, "Unrecognized Graph modification request ("+std::to_string(gmoddata.request)+')');
                return false;
            }

        }

    }
    VERYVERBOSEOUT("Request stack is valid.\n");
    return true;
}

/**
 * Find the shared memory segment with the indicated request
 * stack, then process that stack by first carrying out
 * validity checks on all stack elements and then responding
 * to each request.
 * 
 * @param segname The shared memory segment name provided for the request stack.
 * @return 
 */
bool handle_request_stack(std::string segname) {
    std::string graph_segname(graphmemman.get_active_name()); // preserve, so that we can work with an explicitly named Graph if we want to
    if (segname.empty())
        ERRRETURNFALSE(__func__, "Missing shared segment name for request stack");
    
    Graph_modifications * graphmod_ptr = find_Graph_modifications_in_shared_memory(segname);
    if (!graphmod_ptr)
        ERRRETURNFALSE(__func__, "Unable to find and access shared segment "+segname);
    
    if (!request_stack_valid(*graphmod_ptr, segname))
        ERRRETURNFALSE(__func__, "Request stack contains invalid request data");
    
    Graphmod_results * results_ptr = initialized_results_response(segname);
    if (!results_ptr)
        ERRRETURNFALSE(__func__, "Unable to initialize results object");

    for (const auto & gmoddata : graphmod_ptr->data) {

        switch(gmoddata.request) {

            case graphmod_add_node: {
                Node_ptr node_ptr = Graph_modify_add_node(*fzs.graph_ptr, graph_segname, gmoddata);
                if (!node_ptr)
                    ERRRETURNFALSE(__func__, "Graph modify add node failed. Warning! Parts of the requested stack of modifications may have been carried out!");
                
                results_ptr->results.emplace_back(node_ptr->get_id().key());
                break;
            }

            case graphmod_add_edge: {
                Edge_ptr edge_ptr = Graph_modify_add_edge(*fzs.graph_ptr, graph_segname, gmoddata);
                if (!edge_ptr)
                    ERRRETURNFALSE(__func__, "Graph modify add edge failed. Warning! Parts of the requested stack of modifications may have been carried out!");
                
                results_ptr->results.emplace_back(edge_ptr->get_id().key());
                break;
            }

            default: {
                // This should never happen, especially after being detected in request_stack_valid(). 
            }

        }

    }

    return true;
}

void load_Graph_and_stay_resident() {

    // create the lockfile to indicate the presence of this server
    int lockfile_ret = check_and_make_lockfile(fzs.lockfilepath, "");
    if (lockfile_ret != 0) {
        if (lockfile_ret == 1) {
            ADDERROR(__func__, "Another instance of this server may be running. The lock file already exists at "+std::string(fzs.lockfilepath));
            VERBOSEERR("The lock file already exists at "+std::string(fzs.lockfilepath)+".\nAnother instance of this server may be running.\n");
            standard.exit(exit_general_error);
        }
        ADDERROR(__func__, "Unable to make lockfile at "+std::string(fzs.lockfilepath));
        VERBOSEERR("Unable to make lockfile at "+std::string(fzs.lockfilepath)+".\n");
        standard.exit(exit_general_error);
    }      

    #define RETURN_AFTER_UNLOCKING { \
        if (remove_lockfile(fzs.lockfilepath) != 0) { \
            ADDERROR(__func__, "Unable to remove lockfile before exiting"); \
        } \
        return; \
    }

    // Load the graph and make the pointer available for handlers to use.
    fzs.graph_ptr = fzs.ga.request_Graph_copy();
    if (!fzs.graph_ptr) {
        ADDERROR(__func__,"unable to load Graph");
        RETURN_AFTER_UNLOCKING;
    }

    VERYVERBOSEOUT(graphmemman.info_str());
    VERYVERBOSEOUT(Graph_Info_str(*fzs.graph_ptr));

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
