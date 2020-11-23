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

bool handle_named_list_parameters(std::string NNLpar_requeststr, std::string & response_html) {
    ERRTRACE;

    if (NNLpar_requeststr.substr(0,11) == "persistent=") {
        if (NNLpar_requeststr.substr(11) == "true") {
            VERYVERBOSEOUT("Setting persistent Named Node Lists cache.\n");
            fzs.graph_ptr->set_Lists_persistence(true);
            response_html = "<html>\n<body>\n"
                            "<p>Named Node List parameter set.</p>\n"
                            "<p>persistent_NNL = <b>true</b></p>\n"
                            "</body>\n</html>\n";
            return true;
        } else if (NNLpar_requeststr.substr(11) == "false") {
            VERYVERBOSEOUT("Setting non-persistent Named Node Lists cache.\n");
            fzs.graph_ptr->set_Lists_persistence(false);
            response_html = "<html>\n<body>\n"
                            "<p>Named Node List parameter set.</p>\n"
                            "<p>persistent_NNL = <b>false</b></p>\n"
                            "</body>\n</html>\n";
            return true;
        }
    }
    return false;
}

struct NNL_selected_add_data {
    Node_ID_key nkey;
    int16_t features = 0;
    int32_t maxsize = 0;
};

/// For convenience, this recognizes a shorthand for adding a Node to the 'selected" Named Node List.
bool handle_selected_list(std::string addtoselectedstr, std::string & response_html) {
    ERRTRACE;

    auto token_value_vec = GET_token_values(addtoselectedstr);
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
    } else {
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

    return true;
}

bool handle_named_lists_reload(std::string & response_html) {
    ERRTRACE;

    VERYVERBOSEOUT("Reloading Named Node Lists cache.\n");
    if (!load_Named_Node_Lists_pq(*fzs.graph_ptr, fzs.ga.dbname(), fzs.ga.pq_schemaname())) {
        return false;
    }
    response_html = "<html>\n<body>\n"
                    "<p>Named Node List parameter set.</p>\n"
                    "<p>persistent_NNL = <b>false</b></p>\n"
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

struct NNL_copy_data {
    std::string from_name;
    size_t from_max = 0;
    size_t to_max = 0;
    int16_t features = 0;
    int32_t maxsize = 0;
};

struct NNL_add_data {
    Node_ID_key nkey;
    int16_t features = 0;
    int32_t maxsize = 0;
};

bool get_copy_data(const std::string copydatastr, NNL_copy_data & copydata) {
    auto token_value_vec = GET_token_values(copydatastr);
    for (const auto & GETel : token_value_vec) {
        if (GETel.token == "copy") {
            copydata.from_name = GETel.value;
        } else if (GETel.token == "from_max") {
            copydata.from_max = std::atoi(GETel.value.c_str());
        } else if (GETel.token == "to_max") {
            copydata.to_max = std::atoi(GETel.value.c_str());
        } if (GETel.token == "maxsize") {
            copydata.maxsize = std::atoi(GETel.value.c_str());
        } if (GETel.token == "unique") {
            if (GETel.value == "true") {
                copydata.features = copydata.features | Named_Node_List::unique_mask;
            }
        } if (GETel.token == "fifo") {
            if (GETel.value == "true") {
                copydata.features = copydata.features | Named_Node_List::fifo_mask;
            }
        } if (GETel.token == "prepend") {
            if (GETel.value == "true") {
                copydata.features = copydata.features | Named_Node_List::prepend_mask;
            }
        } else {
            return standard_error("Unexpected token: "+GETel.token, __func__);
        }
    }
    return true;
}

bool get_add_data(const std::string copydatastr, NNL_add_data & adddata) {
    auto token_value_vec = GET_token_values(copydatastr);
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

bool handle_named_list_direct_request(std::string namedlistreqstr, std::string & response_html) {
    ERRTRACE;

    VERYVERBOSEOUT("Handling Named Node List request.\n");
    size_t name_endpos = namedlistreqstr.find('?');

    if (name_endpos == std::string::npos) { // handle requests that have no arguments

        if (fzs.graph_ptr->persistent_Lists()) {
            if (namedlistreqstr == "_reload") {
                return handle_named_lists_reload(response_html);
            }
        }

        if (namedlistreqstr == "_shortlist") {
            return handle_update_shortlist(response_html);
        }

        ADDWARNING(__func__, "Unrecognized Named Node List special request identifier: "+namedlistreqstr);
        return false; // unrecognized or failed
    }

    if (name_endpos == 0) {
        ADDWARNING(__func__, "Request has zero-length Named Node List name or special request identifier.");
        return false; // zero-length Named Node List name or identifier
    }

    // handle requests with arguments
    std::string list_name(namedlistreqstr.substr(0,name_endpos));
    ++name_endpos;

    if (list_name == "_set") {
        return handle_named_list_parameters(namedlistreqstr.substr(name_endpos), response_html);
    }

    if (list_name == "_select") {
        return handle_selected_list(namedlistreqstr.substr(name_endpos), response_html);
    }

    if (namedlistreqstr.substr(name_endpos,4) == "add=") {
        NNL_add_data adddata;
        if (!get_add_data(namedlistreqstr.substr(name_endpos), adddata)) {
            return standard_error("Unable to carry out 'add' to Named Node List", __func__);
        }
        // confirm that the Node ID to add to the Named Node List exists in the Graph
        Node * node_ptr = fzs.graph_ptr->Node_by_id(adddata.nkey);
        if (!node_ptr) {
            return standard_error("Node ID ("+adddata.nkey.str()+") not found in Graph for add to list request", __func__);
        } else {
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
    }

    if (namedlistreqstr.substr(name_endpos,5) == "copy=") {
        NNL_copy_data copydata;
        if (!get_copy_data(namedlistreqstr.substr(name_endpos), copydata)) {
            return standard_error("Unable to carry out 'copy' to Named Node List", __func__);
        }
        size_t copied = 0;
        if (copydata.from_name == "_incomplete") {
            copied = copy_Incomplete_to_List(*fzs.graph_ptr, list_name, copydata.from_max, copydata.to_max, copydata.features, copydata.maxsize);
        } else {
            copied = fzs.graph_ptr->copy_List_to_List(copydata.from_name, list_name, copydata.from_max, copydata.to_max);
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

    if (namedlistreqstr.substr(name_endpos,7) == "remove=") {
        try {
            Node_ID_key nkey(namedlistreqstr.substr(name_endpos+7,16));
            // Beware! Empty Lists are deleted by Graph::remove_from_List(), so you have to test for that!
            // Note: This is the same precaution as why Graphpostgres:handle_Graph_modifications_pq() tests
            //       if a List disappeared before deciding if an update or a delete is in order.
            if (!fzs.graph_ptr->remove_from_List(list_name, nkey)) {
                return standard_error("Unable to remove Node "+nkey.str()+" from Named Node List "+list_name, __func__);
            }
            if (fzs.graph_ptr->persistent_Lists()) {
                if (fzs.graph_ptr->get_List(list_name)) {
                    if (!Update_Named_Node_List_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), list_name, *fzs.graph_ptr)) {
                        return standard_error("Synchronizing Named Node List update to database failed", __func__);
                    }
                } else {
                    if (!Delete_Named_Node_List_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), list_name)) {
                        return standard_error("Synchronizing empty Named Node List deletion in database failed", __func__);
                    }
                }
            }
            response_html = "<html>\n<body>\n"
                            "<p>Named Node List modified.</p>\n"
                            "<p><b>Removed</b> " + nkey.str() + " from List " + list_name + ".</p>\n"
                            "</body>\n</html>\n";
            return true;
        } catch (ID_exception idexception) {
            return standard_error("Named Node List remove request has invalid Node ID ["+namedlistreqstr.substr(name_endpos+4,16)+"], "+idexception.what(), __func__);
        }
    }

    if (namedlistreqstr.substr(name_endpos,7) == "delete=") {
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

    return false;
}

void show_db_mode(int new_socket) {
    ERRTRACE;

    VERYVERBOSEOUT("Database mode: "+SimPQ.PQChanges_Mode_str()+'\n');
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
    std::string response_str("HTTP/1.1 200 OK\nServer: aether\nContent-Type: text/html;charset=UTF-8\nContent-Length: ");
    std::string log_html("<html>\n<head>\n<link rel=\"stylesheet\" href=\"http://"+fzs.graph_ptr->get_server_IPaddr()+"/fz.css\">\n<title>fz: Database Call Log</title>\n</head>\n<body>\n<h3>fz: Database Call Log</h3>\n");
    log_html += "<p>When fzserverpq exits, the DB call log will be flushed to: "+SimPQ.simPQfile+"</p>\n\n";
    log_html += "<p>Current status of the DB call log:</p>\n<hr>\n<pre>\n" + SimPQ.GetLog() + "</pre>\n<hr>\n</body>\n</html>\n";
    response_str += std::to_string(log_html.size()) + "\r\n\r\n" + log_html;
    send(new_socket, response_str.c_str(), response_str.size()+1, 0);
}

void show_ErrQ(int new_socket) {
    ERRTRACE;

    VERYVERBOSEOUT("Showing ErrQ.\n");
    std::string response_str("HTTP/1.1 200 OK\nServer: aether\nContent-Type: text/html;charset=UTF-8\nContent-Length: ");
    std::string errq_html("<html>\n<head>\n<link rel=\"stylesheet\" href=\"http://"+fzs.graph_ptr->get_server_IPaddr()+"/fz.css\">\n<title>fz: ErrQ</title>\n</head>\n<body>\n<h3>fz: ErrQ</h3>\n");
    errq_html += "<p>When fzserverpq exits, ErrQ will be flushed to: "+ErrQ.get_errfilepath()+"</p>\n\n";
    errq_html += "<p>Current status of ErrQ:</p>\n<hr>\n<pre>\n" + ErrQ.pretty_print() + "</pre>\n<hr>\n</body>\n</html>\n";
    response_str += std::to_string(errq_html.size()) + "\r\n\r\n" + errq_html;
    send(new_socket, response_str.c_str(), response_str.size()+1, 0);
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
            response_str += std::to_string(response_html.size()) + "\r\n\r\n" + response_html;
            send(new_socket, response_str.c_str(), response_str.size()+1, 0);
            return true;
        }

    }

    return false;
}

/**
 * Handle a request in the Formalizer /fz/ virtual filesystem.
 * 
 * @param new_socket The communication socket file handler to respond to.
 * @param fzrequesturl The URL-like string containing the request to handle.
 * @return True if the request was handled successfully.
 */
bool handle_fz_vfs_request(int new_socket, const std::string & fzrequesturl) {
    if (fzrequesturl.substr(4) == "status") {
        VERYVERBOSEOUT("Status request received. Responding.\n");
        std::string response_str("HTTP/1.1 200 OK\nServer: aether\nContent-Type: text/html;charset=UTF-8\nContent-Length: ");
        std::string status_html("<html>\n<body>\nServer status: LISTENING\n</body>\n</html>\n");
        response_str += std::to_string(status_html.size()) + "\r\n\r\n" + status_html;
        send(new_socket, response_str.c_str(), response_str.size()+1, 0);
        return true;
    }

    if (fzrequesturl.substr(4) == "ErrQ") {
        show_ErrQ(new_socket);
        return true;
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
    auto requestvec = split(request_str,' ');
    if (requestvec.size()<2) {
        VERYVERBOSEOUT("Missing request. Responding with: 400 Bad Request.\n");
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
            std::string response_str("HTTP/1.1 404 Not Found\r\n\r\n");
            send(new_socket, response_str.c_str(), response_str.size()+1, 0);   
            return;         
        }

    }

    // no known request encountered and handled
    VERBOSEOUT("Request type is unrecognized. Responding with: 400 Bad Request.\n");
    std::string response_str("HTTP/1.1 400 Bad Request\r\n\r\n");
    send(new_socket, response_str.c_str(), response_str.size()+1, 0);
}
