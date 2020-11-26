// Copyright 2020 Randal A. Koene
// License TBD

/**
 * TCP port direct API handler functions for fzserverpq.
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
#include "tcp_server_handlers.hpp"
#include "fzserverpq.hpp"

using namespace fz;

bool handle_named_lists_reload(std::string & response_html) {
    ERRTRACE;

    VERYVERBOSEOUT("Reloading Named Node Lists cache.\n");
    if (!load_Named_Node_Lists_pq(*fzs.graph_ptr, fzs.ga.dbname(), fzs.ga.pq_schemaname())) {
        return false;
    }
    response_html = "<html>\n<body>\n"
                    "<p>Reloaded Named Node Lists cache from database.</p>\n"
                    "</body>\n</html>\n";
    return true;
}

bool handle_update_shortlist(std::string & response_html) {
    ERRTRACE;

    VERYVERBOSEOUT("Updating the 'shortlist' Named Node List\n");
    size_t copied = update_shortlist_List(*fzs.graph_ptr);
    if (fzs.graph_ptr->persistent_Lists()) {
        if (!Update_Named_Node_List_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), "shortlist", *fzs.graph_ptr)) {
            return standard_error("Synchronizing 'shortlist' Named Node List update to database failed", __func__);
        }
    }
    response_html = "<html>\n<body>\n"
                    "<p>Named Node List 'shortlist' updated with "+std::to_string(copied)+" Nodes.</p>\n"
                    "</body>\n</html>\n";

    return true;
}

bool handle_named_list_parameters(const GET_token_value_vec & token_value_vec, std::string & response_html) {
    ERRTRACE;

    response_html = "<html>\n<body>\n"
                    "<p>Named Node List parameter set.</p>\n";
    for (const auto &GETel : token_value_vec) {
        if (GETel.token == "persistent") {
            if (GETel.value == "true") {
                VERYVERBOSEOUT("Setting persistent Named Node Lists cache.\n");
                fzs.graph_ptr->set_Lists_persistence(true);
            } else if (GETel.value == "false") {
                VERYVERBOSEOUT("Setting non-persistent Named Node Lists cache.\n");
                fzs.graph_ptr->set_Lists_persistence(false);
            } else {
                return standard_error("Expected boolean 'true'/'false' instead of: "+GETel.token+'='+GETel.value, __func__);
            }

            response_html += "<p>persistent_NNL = <b>"+GETel.value+"</b></p>\n";

        } else {
            return standard_error("Unexpected token: "+GETel.token, __func__);
        }
    }
    response_html += "</body>\n</html>\n";

    return true;
}

/// For convenience, this recognizes a shorthand for adding a Node to the 'selected" Named Node List.
bool handle_selected_list(const GET_token_value_vec & token_value_vec, std::string & response_html) {
    ERRTRACE;

    Node_ID_key nkey;
    for (const auto & GETel : token_value_vec) {
        if (GETel.token == "id") {
            try {
                nkey = Node_ID_key(GETel.value);
            } catch (ID_exception idexception) {
                return standard_error("Add to 'selected' request has invalid Node ID ["+GETel.value+"], "+idexception.what(), __func__);
            }

        } else {
            return standard_error("Unexpected token: "+GETel.token, __func__);
        }
    }

    // confirm that the Node ID to add to the Named Node List exists in the Graph
    Node * node_ptr = fzs.graph_ptr->Node_by_id(nkey);
    if (!node_ptr) {
        return standard_error("Node ID ("+nkey.str()+") not found in Graph for add to 'selected' request", __func__);
    }

    if (!fzs.graph_ptr->add_to_List("selected", *node_ptr, Named_Node_List::fifo_mask, 1)) {
        return standard_error("Unable to add Node "+nkey.str()+" to 'selected'.", __func__);
    }
    // synchronize with stored List
    if (fzs.graph_ptr->persistent_Lists()) {
        if (!Update_Named_Node_List_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), "selected", *fzs.graph_ptr)) {
            return standard_error("Synchronizing 'selected' update to database failed", __func__);
        }
    }
    response_html = "<html>\n<body>\n"
                    "<p>Named Node List modified.</p>\n"
                    "<p><b>Added</b> "+nkey.str()+" to List 'selected'.</p>\n"
                    "</body>\n</html>\n";

    return true;
}

struct NNL_copy_data {
    std::string from_name;
    size_t from_max = 0;
    size_t to_max = 0;
    int16_t features = -1; // special meaning in copy function
    int32_t maxsize = -1;
};

bool get_copy_data(const GET_token_value_vec & token_value_vec, NNL_copy_data & copydata) {
    bool features_specified = false;
    int16_t features = 0; // need this for mask ORs
    int32_t maxsize = 0;
    for (const auto & GETel : token_value_vec) {
        if (GETel.token == "copy") {
            copydata.from_name = GETel.value;
        } else if (GETel.token == "from_max") {
            copydata.from_max = std::atoi(GETel.value.c_str());
        } else if (GETel.token == "to_max") {
            copydata.to_max = std::atoi(GETel.value.c_str());
        } if (GETel.token == "maxsize") {
            maxsize = std::atoi(GETel.value.c_str());
            features_specified = true;
        } if (GETel.token == "unique") {
            if (GETel.value == "true") {
                features = copydata.features | Named_Node_List::unique_mask;
                features_specified = true;
            }
        } if (GETel.token == "fifo") {
            if (GETel.value == "true") {
                features = copydata.features | Named_Node_List::fifo_mask;
                features_specified = true;
            }
        } if (GETel.token == "prepend") {
            if (GETel.value == "true") {
                features = copydata.features | Named_Node_List::prepend_mask;
                features_specified = true;
            }
        } else {
            return standard_error("Unexpected token: "+GETel.token, __func__);
        }
    }
    if (features_specified) {
        copydata.features = features;
        copydata.maxsize = maxsize;
    }
    return true;
}

bool handle_copy_to_list(const std::string & list_name, const GET_token_value_vec & token_value_vec, std::string & response_html) {
    ERRTRACE;

    VERYVERBOSEOUT("Copying to Named Node List "+list_name+'\n');

    NNL_copy_data copydata;
    if (!get_copy_data(token_value_vec, copydata)) {
        return standard_error("Unable to carry out 'copy' to Named Node List", __func__);
    }

    size_t copied = 0;
    if (copydata.from_name == "_incomplete") {
        if (copydata.features < 0) {
            copydata.features = 0; // the copy_Incomplete_to_List function uses 0 defaults, not -1 defaults
            copydata.maxsize = 0;
        }
        copied = copy_Incomplete_to_List(*fzs.graph_ptr, list_name, copydata.from_max, copydata.to_max, copydata.features, copydata.maxsize);
    } else {
        copied = fzs.graph_ptr->copy_List_to_List(copydata.from_name, list_name, copydata.from_max, copydata.to_max, copydata.features, copydata.maxsize);
    }

    if ((copied>0) && (fzs.graph_ptr->persistent_Lists())) {
        if (!Update_Named_Node_List_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), list_name, *fzs.graph_ptr)) {
            return standard_error("Synchronizing Named Node List copy to database failed", __func__);
        }
    }

    response_html = "<html>\n<body>\n"
                    "<p>Named Node List modified.</p>\n"
                    "<p><b>Copied</b> "+std::to_string(copied)+"Nodes from "+copydata.from_name+" to List "+list_name+".</p>\n"
                    "</body>\n</html>\n";

    return true;
}


struct NNL_add_data {
    Node_ID_key nkey;
    int16_t features = 0;
    int32_t maxsize = 0;
};

bool get_add_data(const GET_token_value_vec & token_value_vec, NNL_add_data & adddata) {
    for (const auto & GETel : token_value_vec) {
        if (GETel.token == "add") {
            try {
                adddata.nkey = Node_ID_key(GETel.value);
            } catch (ID_exception idexception) {
                return standard_error("Add Named Node List request has invalid Node ID ["+GETel.value+"], "+idexception.what(), __func__);
            }
        } if (GETel.token == "maxsize") {
            adddata.maxsize = std::atoi(GETel.value.c_str());
        } if (GETel.token == "unique") {
            if (GETel.value == "true") {
                adddata.features = adddata.features | Named_Node_List::unique_mask;
            }
        } if (GETel.token == "fifo") {
            if (GETel.value == "true") {
                adddata.features = adddata.features | Named_Node_List::fifo_mask;
            }
        } if (GETel.token == "prepend") {
            if (GETel.value == "true") {
                adddata.features = adddata.features | Named_Node_List::prepend_mask;
            }
        } else {
            return standard_error("Unexpected token: "+GETel.token, __func__);
        }
    }
    return true;
}

bool handle_add_to_list(const std::string & list_name, const GET_token_value_vec & token_value_vec, std::string & response_html) {
    ERRTRACE;

    VERYVERBOSEOUT("Adding Node to Named Node List "+list_name+'\n');

    NNL_add_data adddata;
    if (!get_add_data(token_value_vec, adddata)) {
        return standard_error("Unable to carry out 'add' to Named Node List", __func__);
    }

    // confirm that the Node ID to add to the Named Node List exists in the Graph
    Node * node_ptr = fzs.graph_ptr->Node_by_id(adddata.nkey);
    if (!node_ptr) {
        return standard_error("Node ID ("+adddata.nkey.str()+") not found in Graph for add to list request", __func__);
    }

    if (!fzs.graph_ptr->add_to_List(list_name, *node_ptr, adddata.features, adddata.maxsize)) {
        return standard_error("Unable to add Node "+adddata.nkey.str()+" to Named Node List "+list_name, __func__);
    }

    // synchronize with stored List
    if (fzs.graph_ptr->persistent_Lists()) {
        if (!Update_Named_Node_List_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), list_name, *fzs.graph_ptr)) {
            return standard_error("Synchronizing Named Node List update to database failed", __func__);
        }
    }

    response_html = "<html>\n<body>\n"
                    "<p>Named Node List modified.</p>\n"
                    "<p><b>Added</b> "+adddata.nkey.str()+" to List "+list_name+".</p>\n"
                    "</body>\n</html>\n";

    return true;
}

bool handle_remove_from_list(const std::string & list_name, const GET_token_value_vec & token_value_vec, std::string & response_html) {
    ERRTRACE;

    VERYVERBOSEOUT("Removing Node from Named Node List "+list_name+'\n');

    Node_ID_key nkey;
    for (const auto & GETel : token_value_vec) {
        if (GETel.token == "remove") {
            try {
                nkey = Node_ID_key(GETel.value);
            } catch (ID_exception idexception) {
                return standard_error("Remove Named Node List request has invalid Node ID ["+GETel.value+"], "+idexception.what(), __func__);
            }
        } else {
            return standard_error("Unexpected token: "+GETel.token, __func__);
        }
    }

    if (!fzs.graph_ptr->remove_from_List(list_name, nkey)) {
        return standard_error("Unable to remove Node "+nkey.str()+" from Named Node List "+list_name, __func__);
    }

    response_html = "<html>\n<body>\n"
                    "<p>Named Node List modified.</p>\n";

    if (fzs.graph_ptr->persistent_Lists()) {
        // Beware! Empty Lists are deleted by Graph::remove_from_List(), so you have to test for that!
        // Note: This is the same precaution as why Graphpostgres:handle_Graph_modifications_pq() tests
        //       if a List disappeared before deciding if an update or a delete is in order.
        if (fzs.graph_ptr->get_List(list_name)) {
            if (!Update_Named_Node_List_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), list_name, *fzs.graph_ptr)) {
                return standard_error("Synchronizing Named Node List update to database failed", __func__);
            }
            response_html += "<p><b>Removed</b> " + nkey.str() + " from List " + list_name + ".</p>\n";
        } else {
            if (!Delete_Named_Node_List_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), list_name)) {
                return standard_error("Synchronizing empty Named Node List deletion in database failed", __func__);
            }
            response_html += "<p><b>Removed</b> " + nkey.str() + " from List " + list_name + " and Deleted empty List.</p>\n";
        }
    }

    response_html += "</body>\n</html>\n";

    return true;
}

bool handle_delete_list(const std::string & list_name, const GET_token_value_vec & token_value_vec, std::string & response_html) {
    ERRTRACE;

    VERYVERBOSEOUT("Deleting Named Node List "+list_name+'\n');

    if (!fzs.graph_ptr->delete_List(list_name)) {
        return standard_error("Unable to delete Named Node List "+list_name, __func__);
    }

    if (fzs.graph_ptr->persistent_Lists()) {
        if (!Delete_Named_Node_List_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), list_name)) {
            return standard_error("Synchronizing Named Node List deletion in database failed", __func__);
        }
    }

    response_html = "<html>\n<body>\n"
                    "<p>Named Node List modified.</p>\n"
                    "<p><b>Deleted</b> List " + list_name + ".</p>\n"
                    "</body>\n</html>\n";

    return true;
}

typedef std::map<std::string, unsigned int> Command_Token_Map;

const Command_Token_Map NNL_noargs_commands = {
    {"_reload", NNLnoargcmd_reload},
    {"_shortlist", NNLnoargcmd_shortlist}
};

const Command_Token_Map NNL_underscore_commands = {
    {"_set", NNLuscrcmd_set},
    {"_select", NNLuscrcmd_select}
};

const Command_Token_Map NNL_list_commands = {
    {"add", NNLlistcmd_add},
    {"remove", NNLlistcmd_remove},
    {"copy", NNLlistcmd_copy},
    {"delete", NNLlistcmd_delete}
};

unsigned int find_in_command_map(const std::string & cmd_candidate, const Command_Token_Map & CTmap) {
    auto CT_it = CTmap.find(cmd_candidate);
    if (CT_it != CTmap.end()) {
        return CT_it->second;
    }
    return 0; // not a known command
}

unsigned int find_token_values_command(const GET_token_value_vec & token_value_vec, const Command_Token_Map & CTmap) {
    for (const auto & GETel : token_value_vec) {
        unsigned int known_command = find_in_command_map(GETel.token, CTmap);
        if (known_command != 0) {
            return known_command;
        }
    }
    return 0; // no command found
}

// *** Note: This could be improved by putting the standardized handler functions into a const
//     map with the command as a key and the function to be called. This would replace all of
//     the switch statements.
bool handle_named_list_direct_request(std::string namedlistreqstr, std::string & response_html) {
    ERRTRACE;

    VERYVERBOSEOUT("Handling Named Node List request.\n");
    size_t name_endpos = namedlistreqstr.find('?');

    if (name_endpos == std::string::npos) { // handle requests that have no arguments

        NNL_noarg_cmd noargs_cmd = static_cast<NNL_noarg_cmd>(find_in_command_map(namedlistreqstr, NNL_noargs_commands));
        if (noargs_cmd != NNLnoargcmd_unknown) {
            switch (noargs_cmd) {

                case NNLnoargcmd_reload: {
                    if (fzs.graph_ptr->persistent_Lists()) {
                        return handle_named_lists_reload(response_html);
                    } else {
                        return standard_error("Named Node List '_reload' request is not available when mode is non-persistent.", __func__);
                    }
                }

                case NNLnoargcmd_shortlist: {
                    return handle_update_shortlist(response_html);
                }

                default: {
                    // nothing to do here
                }
            }
        }

        return standard_error("Unrecognized Named Node List special request identifier: "+namedlistreqstr, __func__);
    }

    if (name_endpos == 0) {
        return standard_error("Request has zero-length Named Node List name or special request identifier.", __func__);
    }

    // handle requests with arguments
    std::string list_name(namedlistreqstr.substr(0,name_endpos));
    ++name_endpos;
    auto token_value_vec = GET_token_values(namedlistreqstr.substr(name_endpos));

    NNL_underscore_cmd underscore_cmd = static_cast<NNL_underscore_cmd>(find_in_command_map(list_name, NNL_underscore_commands));
    if (underscore_cmd != NNLuscrcmd_unknown) {
        switch (underscore_cmd) {

            case NNLuscrcmd_set: {
                return handle_named_list_parameters(token_value_vec, response_html);
            }

            case NNLuscrcmd_select: {
                return handle_selected_list(token_value_vec, response_html);
            }

            default: {
                // nothing to do here
            }
        }
    }

    // handle requests with list_name and arguments
    NNL_list_cmd list_cmd = static_cast<NNL_list_cmd>(find_token_values_command(token_value_vec, NNL_list_commands));
    if (list_cmd != NNLlistcmd_unknown) {
        switch (list_cmd) {

            case NNLlistcmd_add: {
                return handle_add_to_list(list_name, token_value_vec, response_html);
            }

            case NNLlistcmd_remove: {
                return handle_remove_from_list(list_name, token_value_vec, response_html);
            }

            case NNLlistcmd_copy: {
                return handle_copy_to_list(list_name, token_value_vec, response_html);
            }

            case NNLlistcmd_delete: {
                return handle_delete_list(list_name, token_value_vec, response_html);
            }

            default: {
                // nothing to do here
            }
        }
    }

    return standard_error("No known request token found: "+namedlistreqstr, __func__);
}

void show_db_mode(int new_socket) {
    ERRTRACE;

    VERYVERBOSEOUT("Database mode: "+SimPQ.PQChanges_Mode_str()+'\n');
    fzs.log("TCP", "DB mode request successful");
    std::string response_str("HTTP/1.1 200 OK\nServer: aether\nContent-Type: text/html;charset=UTF-8\nContent-Length: ");
    std::string mode_html("<html>\n<body>\n<p>Database mode: "+SimPQ.PQChanges_Mode_str()+"</p>\n");
    if (SimPQ.LoggingPQChanges()) {
        mode_html += "<p>Logging to: "+SimPQ.simPQfile+"</p>\n";
    }
    mode_html += "</body>\n</html>\n";
    response_str += std::to_string(mode_html.size()) + "\r\n\r\n" + mode_html;
    send(new_socket, response_str.c_str(), response_str.size()+1, 0);
}

void show_db_log(int new_socket) {
    ERRTRACE;

    VERYVERBOSEOUT("Showing database log.\n");
    fzs.log("TCP", "DB log request sucessful");
    std::string response_str("HTTP/1.1 200 OK\nServer: aether\nContent-Type: text/html;charset=UTF-8\nContent-Length: ");
    std::string log_html("<html>\n<head>\n<link rel=\"stylesheet\" href=\"http://"+fzs.graph_ptr->get_server_IPaddr()+"/fz.css\">\n<title>fz: Database Call Log</title>\n</head>\n<body>\n<h3>fz: Database Call Log</h3>\n");
    log_html += "<p>When fzserverpq exits, the DB call log will be flushed to: "+SimPQ.simPQfile+"</p>\n\n";
    log_html += "<p>Current status of the DB call log:</p>\n<hr>\n<pre>\n" + SimPQ.GetLog() + "</pre>\n<hr>\n</body>\n</html>\n";
    response_str += std::to_string(log_html.size()) + "\r\n\r\n" + log_html;
    send(new_socket, response_str.c_str(), response_str.size()+1, 0);
}

bool show_ErrQ(int new_socket) {
    ERRTRACE;

    VERYVERBOSEOUT("Showing ErrQ.\n");
    fzs.log("TCP", "ErrQ request sucessful");
    std::string response_str("HTTP/1.1 200 OK\nServer: aether\nContent-Type: text/html;charset=UTF-8\nContent-Length: ");
    std::string errq_html("<html>\n<head>\n<link rel=\"stylesheet\" href=\"http://"+fzs.graph_ptr->get_server_IPaddr()+"/fz.css\">\n<title>fz: ErrQ</title>\n</head>\n<body>\n<h3>fz: ErrQ</h3>\n");
    errq_html += "<p>When fzserverpq exits, ErrQ will be flushed to: "+ErrQ.get_errfilepath()+"</p>\n\n";
    errq_html += "<p>Current status of ErrQ:</p>\n<hr>\n<pre>\n" + ErrQ.pretty_print() + "</pre>\n<hr>\n</body>\n</html>\n";
    response_str += std::to_string(errq_html.size()) + "\r\n\r\n" + errq_html;
    return (send(new_socket, response_str.c_str(), response_str.size()+1, 0) >= 0);
}

/**
 * Handle a database request in the Formalizer /fz/ virtual filesystem.
 * 
 * @param new_socket The communication socket file handler to respond to.
 * @param fzrequesturl The URL-like string containing the request to handle.
 * @return True if the request was handled successfully.
 */
bool handle_fz_vfs_database_request(int new_socket, const std::string & fzrequesturl) {
    VERYVERBOSEOUT("Handling database request.\n");
    if (fzrequesturl.substr(7,4) == "mode") {

        if (fzrequesturl.size()>11) { // change mode

            if (fzrequesturl.substr(11,8) == "?set=run") {
                SimPQ.ActualChanges();
                show_db_mode(new_socket);
                return true;
            } else if (fzrequesturl.substr(11,8) == "?set=log") {
                SimPQ.LogChanges();
                show_db_mode(new_socket);
                return true;
            } else if (fzrequesturl.substr(11,8) == "?set=sim") {
                SimPQ.SimulateChanges();
                show_db_mode(new_socket);
                return true;
            }

        } else { // report mode
            show_db_mode(new_socket);
            return true;
        }

    } else if (fzrequesturl.substr(7,3) == "log") {
        show_db_log(new_socket);
        return true;
    }

    return false;
}

/**
 * Handle a Graph request in the Formalizer /fz/ virtual filesystem.
 * 
 * @param new_socket The communication socket file handler to respond to.
 * @param fzrequesturl The URL-like string containing the request to handle.
 * @return True if the request was handled successfully.
 */
bool handle_fz_vfs_graph_request(int new_socket, const std::string & fzrequesturl) {
    VERYVERBOSEOUT("Handling Graph request.\n");
    if (fzrequesturl.substr(10,11) == "namedlists/") {

        std::string response_str("HTTP/1.1 200 OK\nServer: aether\nContent-Type: text/html;charset=UTF-8\nContent-Length: ");
        std::string response_html;
        if (handle_named_list_direct_request(fzrequesturl.substr(21), response_html)) {
            VERYVERBOSEOUT("Named Node List modification / parameter request handled. Responding.\n");
            fzs.log("TCP", "NNL request successful");
            response_str += std::to_string(response_html.size()) + "\r\n\r\n" + response_html;
            send(new_socket, response_str.c_str(), response_str.size()+1, 0);
            return true;
        }

    }

    return false;
}

const Command_Token_Map general_noargs_commands = {
    {"status", fznoargcmd_status},
    {"ErrQ", fznoargcmd_errq},
    {"_stop", fznoargcmd_stop}
};

bool handle_status(int new_socket) {
    VERYVERBOSEOUT("Status request received. Responding.\n");
    fzs.log("TCP", "Status reported");
    std::string response_str("HTTP/1.1 200 OK\nServer: aether\nContent-Type: text/html;charset=UTF-8\nContent-Length: ");
    std::string status_html("<html>\n<body>\nServer status: LISTENING\n</body>\n</html>\n");
    response_str += std::to_string(status_html.size()) + "\r\n\r\n" + status_html;
    return (send(new_socket, response_str.c_str(), response_str.size()+1, 0) >= 0);
}

bool handle_stop(int new_socket) {
    fzs.listen = false;
    VERYVERBOSEOUT("STOP request received. Exiting server listen loop.\n");
    fzs.log("TCP", "Stopping");
    std::string response_str("HTTP/1.1 200 OK\nServer: aether\nContent-Type: text/html;charset=UTF-8\nContent-Length: ");
    std::string status_html("<html>\n<body>\nServer status: STOPPING\n</body>\n</html>\n");
    response_str += std::to_string(status_html.size()) + "\r\n\r\n" + status_html;
    return (send(new_socket, response_str.c_str(), response_str.size()+1, 0) >= 0);
}

/**
 * Handle a request in the Formalizer /fz/ virtual filesystem.
 * 
 * @param new_socket The communication socket file handler to respond to.
 * @param fzrequesturl The URL-like string containing the request to handle.
 * @return True if the request was handled successfully.
 */
bool handle_fz_vfs_request(int new_socket, const std::string & fzrequesturl) {

    fz_general_noarg_cmd fznoargs_cmd = static_cast<fz_general_noarg_cmd>(find_in_command_map(fzrequesturl.substr(4), general_noargs_commands));
    if (fznoargs_cmd != fznoargcmd_unknown) {
        switch (fznoargs_cmd) {

            case fznoargcmd_status: {
                return handle_status(new_socket);
            }

            case fznoargcmd_errq: {
                return show_ErrQ(new_socket);
            }

            case fznoargcmd_stop: {
                return handle_stop(new_socket);
            }

            default: {
                // nothing to do here
            }
        }
    }

    if (fzrequesturl.substr(4,3) == "db/") {
        return handle_fz_vfs_database_request(new_socket, fzrequesturl);
    } 

    if (fzrequesturl.substr(4,6) == "graph/") {
        return handle_fz_vfs_graph_request(new_socket, fzrequesturl);
    }

    return false;
}

void fzserverpq::handle_special_purpose_request(int new_socket, const std::string & request_str) {
    ERRTRACE;

    VERYVERBOSEOUT("Received Special Purpose request "+request_str+".\n");
    log("TCP","Received: "+request_str);
    auto requestvec = split(request_str,' ');
    if (requestvec.size()<2) {
        VERYVERBOSEOUT("Missing request. Responding with: 400 Bad Request.\n");
        log("TCP","Insufficient data");
        std::string response_str("HTTP/1.1 400 Bad Request\r\n\r\n");
        send(new_socket, response_str.c_str(), response_str.size()+1, 0);
        return;
    }

    // Note that we're not really bothering to distinguish GET and PATCH here.
    // Instead, we're just using CGI FORM GET-method URL encoding to identify modification requests.

    if (requestvec[1].substr(0,4) == "/fz/") { // (one type of) recognized Formalizer special purpose request

        if (handle_fz_vfs_request(new_socket, requestvec[1])) {
            return;
        } else {
            VERBOSEOUT("Formalizer Virtual Filesystem /fz/ request failed.\nResponding with: 404 Not Found.\n");
            log("TCP", "/fz/ request error");
            std::string response_str("HTTP/1.1 404 Not Found\r\n\r\n");
            send(new_socket, response_str.c_str(), response_str.size()+1, 0);   
            return;         
        }

    }

    // no known request encountered and handled
    VERBOSEOUT("Request type is unrecognized. Responding with: 400 Bad Request.\n");
    log("TCP", "Unrecognized request");
    std::string response_str("HTTP/1.1 400 Bad Request\r\n\r\n");
    send(new_socket, response_str.c_str(), response_str.size()+1, 0);
}
