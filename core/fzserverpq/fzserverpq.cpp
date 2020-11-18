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
#include "Graphpostgres.hpp"

// local
#include "fzserverpq.hpp"

#define PERSISTENT_NAMED_NODE_LISTS

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
    usage_tail.push_back(
        "The limited number of command line options are generally used only to start\n"
        "the server. The server interacts with clients through the combination of a\n"
        "TCP socket at the specified port and shared memory data exchanges.\n"
        "The following are recognized requests:\n"
        "  'GET ' followed by a path such as '/fz/status', browser HTTP-like request\n"
        "  'PATCH ' is much like GET, but used to request a modification\n"
        "  'STOP' stops and terminates the server (`fzgraph -z` sends this)\n"
        "  'PING' requests a readiness response from the server\n"
        "  'DBMODE' rotate through database modes (runsilent, log, simulate)\n"
        "Any other string read at the socket is interpreted as the name of a shared\n"
        "memory segment that contains a Graph_modifications object named 'graphmod'\n"
        "with a stack of modification requests.\n"
        "Note that '/fz/' paths are special Formalizer handles for which the response\n"
        "content is generated dynamically (somewhat analogous to files in /proc).\n");
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

bool handle_named_list_direct_request(std::string namedlistreqstr) {
    size_t name_endpos = namedlistreqstr.find('?');
    if ((name_endpos == std::string::npos) || (name_endpos == 0)) {
        return false;
    }

    std::string list_name(namedlistreqstr.substr(0,name_endpos));
    ++name_endpos;

    if (namedlistreqstr.substr(name_endpos,4) == "add=") {
        try {
            Node_ID_key nkey(namedlistreqstr.substr(name_endpos+4,16));
            // confirm that the Node ID to add to the Named Node List exists in the Graph
            Node * node_ptr = fzs.graph_ptr->Node_by_id(nkey);
            if (!node_ptr) {
                return standard_error("Node ID ("+nkey.str()+") not found in Graph for add to list request", __func__);
            } else {
                if (!fzs.graph_ptr->add_to_List(list_name, *node_ptr)) {
                    return standard_error("Unable to add Node "+nkey.str()+" to Named Node List "+list_name, __func__);
                }
                // synchronize with stored List
                #ifdef PERSISTENT_NAMED_NODE_LISTS
                if (!Update_Named_Node_List_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), list_name, *fzs.graph_ptr)) {
                    return standard_error("Synchronizing Named Node List update to database failed", __func__);
                }
                #endif
                return true;
            }
        } catch (ID_exception idexception) {
            return standard_error("Named Node List add request has invalid Node ID ["+namedlistreqstr.substr(name_endpos+4,16)+"], "+idexception.what(), __func__);
        }
    }

    if (namedlistreqstr.substr(name_endpos,7) == "remove=") {
        try {
            Node_ID_key nkey(namedlistreqstr.substr(name_endpos+7,16));
            if (!fzs.graph_ptr->remove_from_List(list_name, nkey)) {
                return standard_error("Unable to remove Node "+nkey.str()+" from Named Node List "+list_name, __func__);
            }
            #ifdef PERSISTENT_NAMED_NODE_LISTS
            if (!Update_Named_Node_List_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), list_name, *fzs.graph_ptr)) {
                return standard_error("Synchronizing Named Node List update to database failed", __func__);
            }
            #endif
            return true;
        } catch (ID_exception idexception) {
            return standard_error("Named Node List remove request has invalid Node ID ["+namedlistreqstr.substr(name_endpos+4,16)+"], "+idexception.what(), __func__);
        }
    }

    if (namedlistreqstr.substr(name_endpos,7) == "delete=") {
        if (!fzs.graph_ptr->delete_List(list_name)) {
            return standard_error("Unable to delete Named Node List "+list_name, __func__);
        }
        #ifdef PERSISTENT_NAMED_NODE_LISTS
        if (!Delete_Named_Node_List_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), list_name)) {
            return standard_error("Synchronizing Named Node List deletion in database failed", __func__);
        }
        #endif
        return true;
    }

    return false;
}

void show_db_mode(int new_socket) {
    VERYVERBOSEOUT("Database mode: "+SimPQ.PQChanges_Mode_str()+'\n');
    std::string response_str("HTTP/1.1 200 OK\nServer: aether\nContent-Type: text/html;charset=UTF-8\nContent-Length: ");
    std::string mode_html("<html>\n<body>\nDatabase mode: "+SimPQ.PQChanges_Mode_str()+"\n</body>\n</html>\n");
    response_str += std::to_string(mode_html.size()) + "\n\n" + mode_html;
    send(new_socket, response_str.c_str(), response_str.size()+1, 0);
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

    // Note that we're not really bothering to distinguish GET and PATCH here.
    // Instead, we're just using CGI FORM GET-method URL encoding to identify modification requests.

    if (requestvec[1].substr(0,4) == "/fz/") { // (one type of) recognized Formalizer special purpose request

        if (requestvec[1].substr(4) == "status") {
            VERYVERBOSEOUT("Status request received. Responding.\n");
            std::string response_str("HTTP/1.1 200 OK\nServer: aether\nContent-Type: text/html;charset=UTF-8\nContent-Length: ");
            std::string status_html("<html>\n<body>\nServer status: LISTENING\n</body>\n</html>\n");
            response_str += std::to_string(status_html.size()) + "\n\n" + status_html;
            send(new_socket, response_str.c_str(), response_str.size()+1, 0);
            return;
        }

        if (requestvec[1].substr(4,3) == "db/") {

            if (requestvec[1].substr(7,4) == "mode") {

                if (requestvec[1].size()>11) { // change mode

                    if (requestvec[1].substr(11,8) == "?set=run") {
                        SimPQ.ActualChanges();
                        show_db_mode(new_socket);
                        return;
                    } else if (requestvec[1].substr(11,8) == "?set=log") {
                        SimPQ.LogChanges();
                        show_db_mode(new_socket);
                        return;
                    } else if (requestvec[1].substr(11,8) == "?set=sim") {
                        SimPQ.SimulateChanges();
                        show_db_mode(new_socket);
                        return;
                    }

                } else { // report mode
                    show_db_mode(new_socket);
                    return;
                }

            }

        } 

        if (requestvec[1].substr(4,6) == "graph/") {

            if (requestvec[1].substr(10,11) == "namedlists/") {

                if (handle_named_list_direct_request(requestvec[1].substr(21))) {
                    VERYVERBOSEOUT("Named Node List modification request handled. Responding.\n");
                    std::string response_str("HTTP/1.1 200 OK\nServer: aether\nContent-Type: text/html;charset=UTF-8\nContent-Length: ");
                    std::string status_html("<html>\n<body>\nNamed Node List modified.\n</body>\n</html>\n");
                    response_str += std::to_string(status_html.size()) + "\n\n" + status_html;
                    send(new_socket, response_str.c_str(), response_str.size()+1, 0);
                    return;
                }

            }

        }
    }

    // no known request encountered and handled
    VERBOSEOUT("Request type is unrecognized. Responding with: 400 Bad Request.\n");
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
                    prepare_error_response(segname, exit_missing_data, "Missing Edge data in add edge request");
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

            case namedlist_add: {
                if (!gmoddata.nodelist_ptr) {
                    prepare_error_response(segname, exit_missing_data, "Missing Named Node List data in add to list request");
                    return false;
                }                
                // confirm that the Node ID to add to the Named Node List exists in the Graph
                if ((!fzs.graph_ptr->Node_by_id(gmoddata.nodelist_ptr->nkey)) && (!node_id_in_request_stack(graphmod, gmoddata.nodelist_ptr->nkey))) {
                    prepare_error_response(segname, exit_bad_request_data, "Node ID ("+gmoddata.nodelist_ptr->nkey.str()+") not found in Graph for add to list request");
                    return false;
                }
                break;
            }

            case namedlist_remove: {
                // all good, attempting to remove one that isn't there will not corrupt anything
                break;
            }

            case namedlist_delete: {
                // all good, attempting to delete a list that doesn't exist will not corrupt anything
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
   
    // carry out all in-memory modifications first, which should catch any inconsistencies
    for (const auto & gmoddata : graphmod_ptr->data) {

        switch(gmoddata.request) {

            case graphmod_add_node: {
                Node_ptr node_ptr = Graph_modify_add_node(*fzs.graph_ptr, graph_segname, gmoddata);
                if (!node_ptr)
                    ERRRETURNFALSE(__func__, "Graph modify add node failed. Warning! Parts of the requested stack of modifications may have been carried out (IN MEMORY ONLY)!");
                
                results_ptr->results.emplace_back(graphmod_add_node, node_ptr->get_id().key());
                #ifdef USE_CHANGE_HISTORY
                // this is an example of a place where a change history record can be created and where the
                // state of the change can be set to `applied-in-memory`. See https://trello.com/c/FxSP8If8.
                #endif
                break;
            }

            case graphmod_add_edge: {
                Edge_ptr edge_ptr = Graph_modify_add_edge(*fzs.graph_ptr, graph_segname, gmoddata);
                if (!edge_ptr)
                    ERRRETURNFALSE(__func__, "Graph modify add edge failed. Warning! Parts of the requested stack of modifications may have been carried out (IN MEMORY ONLY)!");
                
                results_ptr->results.emplace_back(graphmod_add_edge, edge_ptr->get_id().key());
                break;
            }

            case namedlist_add: {
                Named_Node_List_ptr nodelist_ptr = Graph_modify_list_add(*fzs.graph_ptr, graph_segname, gmoddata);
                if (!nodelist_ptr)
                    ERRRETURNFALSE(__func__, "Graph modify add to Named Node List failed.");

                results_ptr->results.emplace_back(namedlist_add, gmoddata.nodelist_ptr->name.c_str(), gmoddata.nodelist_ptr->nkey);
                // synchronization with database is done outside the for-loop to minimize the number of updates
                break;
            }

            case namedlist_remove: {
                bool res = Graph_modify_list_remove(*fzs.graph_ptr, graph_segname, gmoddata);
                if (!res)
                    ERRRETURNFALSE(__func__, "Graph modify remove from Named Node List failed.");

                results_ptr->results.emplace_back(namedlist_remove, gmoddata.nodelist_ptr->name.c_str(), gmoddata.nodelist_ptr->nkey);
                // synchronization with database is done outside the for-loop to minimize the number of updates
                break;
            }

            case namedlist_delete: {
                bool res = Graph_modify_list_delete(*fzs.graph_ptr, graph_segname, gmoddata);
                if (!res)
                    ERRRETURNFALSE(__func__, "Graph modify delete Named Node List failed.");

                results_ptr->results.emplace_back(namedlist_delete, gmoddata.nodelist_ptr->name.c_str(), gmoddata.nodelist_ptr->nkey);
                break;
            }

            default: {
                // This should never happen, especially after being detected in request_stack_valid(). 
            }

        }

    }

    // Here is the call to the database-dependent library for modifications in the database.
    if (!handle_Graph_modifications_pq(*fzs.graph_ptr, fzs.ga.config.dbname, fzs.ga.config.pq_schemaname, *results_ptr)) {
        ERRRETURNFALSE(__func__, "Unable to send in-memory Graph changes to storage.");
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
