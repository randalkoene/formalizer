// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Shared memory server handler functions for fzserverpq.
 * 
 * For more about this, see https://trello.com/c/S7SZUyeU.
 */

// std
//#include <iostream>
//#include <sys/types.h>
#include <sys/socket.h>

// core
#include "error.hpp"
#include "standard.hpp"
//#include "general.hpp"
//#include "stringio.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "Graphpostgres.hpp"

// local
#include "shm_server_handlers.hpp"
#include "fzserverpq.hpp"

using namespace fz;

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
    ERRTRACE;

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
    ERRTRACE;

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

            // *** We could add shared memory handlers for:
            //       NNL add requests WITH features and maxsize
            //       NNL copy requests from List to List or from sorted incomplete Nodes list to List
            //     At present, this is provided via the direct port-API only, and tools such as
            //     fzgraph provide a means of delivering 'GET url' style request for that purpose.

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

void fzserverpq::handle_request_with_data_share(int new_socket, const std::string & segment_name) {
    ERRTRACE;

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
