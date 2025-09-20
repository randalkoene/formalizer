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
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <sys/socket.h>
#include <filesystem>
#include <chrono> // FOR PROFILING AND DEBUGGING (remove this)

// core
#include "debug.hpp"
#include "error.hpp"
#include "standard.hpp"
//#include "general.hpp"
//#include "stringio.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "Graphpostgres.hpp"
//#include "stringio.hpp"
#include "binaryio.hpp"
#include "apiclient.hpp"
#include "tcpclient.hpp"

// local
#include "tcp_server_handlers.hpp"
#include "tcp_serialized_data_handlers.hpp"
#include "fzserverpq.hpp"

#define TEST_MORE_THAN_NODE_MODIFICATIONS

Set_Debug_LogFile("/dev/shm/fzserverpq-debug.log");

using namespace fz;

std::string standard_HTML_header(const std::string& titlestr, const std::string& bodytag = "") {
    std::string serverIPaddrstr(fzs.graph_ptr->get_server_IPaddr());
    std::string htmlstr("<html>\n<head>\n<link rel=\"icon\" href=\"/favicon-32x32.png\">\n<link rel=\"stylesheet\" href=\"http://");
    htmlstr += serverIPaddrstr;
    htmlstr += "/fz.css\">\n<link rel=\"stylesheet\" href=\"http://";
    htmlstr += serverIPaddrstr;
    htmlstr += "/fzuistate.css\">\n<title>";
    htmlstr += titlestr;
    htmlstr += "</title>\n</head>\n";
    if (bodytag.empty()) {
        htmlstr += "<body>\n";
    } else {
        htmlstr += bodytag;
    }
    htmlstr += "<script type=\"text/javascript\" src=\"http://";
    htmlstr += serverIPaddrstr;
    htmlstr += "/fzuistate.js\"></script>";
    htmlstr += "<h3>";
    htmlstr += titlestr;
    htmlstr += "</h3>\n";
    return htmlstr;
}

bool handle_request_response(int socket, const std::string & text, std::string msg) {
    server_response_text srvtxt(text);
    fzs.log("TCP", msg);
    VERYVERBOSEOUT(msg+'\n');
    return (srvtxt.respond(socket) >= 0);
}

void handle_request_error(int socket, http_response_code code, std::string error_msg) {
    server_response_text srvtxt(code, error_msg);
    srvtxt.respond(socket); // a VERYVERBOSEOUT is in the respond() function
    fzs.log("TCP",srvtxt.error_msg);
}

bool handle_named_lists_reload(std::string & response_html) {
    ERRTRACE;

    VERYVERBOSEOUT("Reloading Named Node Lists cache.\n");
    if (!load_Named_Node_Lists_pq(*fzs.graph_ptr, fzs.ga.dbname(), fzs.ga.pq_schemaname())) {
        return false;
    }
    response_html = standard_HTML_header("fz: NNL") + 
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
    response_html = standard_HTML_header("fz: Shortlist NNL") + 
                    "<p>Named Node List 'shortlist' updated with "+std::to_string(copied)+" Nodes.</p>\n"
                    "</body>\n</html>\n";

    return true;
}

bool handle_named_list_parameters(const GET_token_value_vec & token_value_vec, std::string & response_html) {
    ERRTRACE;

    response_html = standard_HTML_header("fz: NNL Parameters") +
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
            return standard_error("Unexpected token: '"+GETel.token+'\'', __func__);
        }
    }
    response_html += "</body>\n</html>\n";

    return true;
}

// Note: Using the fzclosing_window.js script for this does not work, because it is intended for
//       a different use-case where another script causes a window to be opened, then automatically closed.
//       You can add code here to show a nice counter, as in the other script.
#define AUTO_CLOSING_HTML_BODY_OPEN "<body onload=\"setTimeout(function() { window.close(); }, 3000);\">\n"
#define AUTO_CLOSING_HTML_BODY_CLOSE "(This window closes automatically in 3 seconds.)\n</body>\n"
#define AUTO_CLOSING_JS "<script type=\"text/javascript\" src=\"/fzclosing_window.js\"></script>"
#define AUTO_CLOSING_JS_BODY_TAG "<body onload=\"do_if_opened_by_script('Keep Page','Go to Log','/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END');\">"

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
            return standard_error("Unexpected token: '"+GETel.token+'\'', __func__);
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
    response_html = standard_HTML_header("fz: NNL", AUTO_CLOSING_HTML_BODY_OPEN) + 
                    "<p>Named Node List modified.</p>\n"
                    "<p><b>Added</b> "+nkey.str()+" to List 'selected'.</p>\n"
                    AUTO_CLOSING_HTML_BODY_CLOSE
                    "</html>\n";

    return true;
}

/// For convenience, this recognizes a shorthand for pushing a Node to the 'recent" Named Node List with features fifo, unique, maxsize=5.
bool handle_recent_list(const GET_token_value_vec & token_value_vec, std::string & response_html) {
    ERRTRACE;

    Node_ID_key nkey;
    for (const auto & GETel : token_value_vec) {
        if (GETel.token == "id") {
            try {
                nkey = Node_ID_key(GETel.value);
            } catch (ID_exception idexception) {
                return standard_error("Push to 'recent' request has invalid Node ID ["+GETel.value+"], "+idexception.what(), __func__);
            }

        } else {
            return standard_error("Unexpected token: '"+GETel.token+'\'', __func__);
        }
    }

    // confirm that the Node ID to add to the Named Node List exists in the Graph
    Node * node_ptr = fzs.graph_ptr->Node_by_id(nkey);
    if (!node_ptr) {
        return standard_error("Node ID ("+nkey.str()+") not found in Graph for push to 'recent' request", __func__);
    }

    if (!fzs.graph_ptr->add_to_List("recent", *node_ptr, Named_Node_List::fifo_mask | Named_Node_List::unique_mask, 5)) {
        return standard_error("Unable to add Node "+nkey.str()+" to 'recent'.", __func__);
    }
    // synchronize with stored List
    if (fzs.graph_ptr->persistent_Lists()) {
        if (!Update_Named_Node_List_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), "recent", *fzs.graph_ptr)) {
            return standard_error("Synchronizing 'recent' update to database failed", __func__);
        }
    }
    response_html = standard_HTML_header("fz: Recent NNL") +
                    "<p>Named Node List modified.</p>\n"
                    "<p><b>Pushed</b> "+nkey.str()+" to fifo List 'recent'.</p>\n"
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

typedef bool copy_data_func_t(const std::string&, NNL_copy_data&);
typedef std::map<std::string, copy_data_func_t*> copy_data_map_t;

bool copy_data_from_name(const std::string &value, NNL_copy_data & copydata) {
    copydata.from_name = value;
    return true;
}

bool copy_data_from_max(const std::string &value, NNL_copy_data & copydata) {
    copydata.from_max = std::atoi(value.c_str());
    return true;
}

bool copy_data_to_max(const std::string &value, NNL_copy_data & copydata) {
    copydata.to_max = std::atoi(value.c_str());
    return true;
}

bool copy_data_maxsize(const std::string &value, NNL_copy_data & copydata) {
    copydata.maxsize = std::atoi(value.c_str());
    return true;
}

bool copy_data_unique(const std::string &value, NNL_copy_data & copydata) {
    if (value == "true") {
        copydata.features |= Named_Node_List::unique_mask;
    }
    return true;
}

bool copy_data_fifo(const std::string &value, NNL_copy_data & copydata) {
    if (value == "true") {
        copydata.features |= Named_Node_List::fifo_mask;
    }
    return true;
}

bool copy_data_prepend(const std::string &value, NNL_copy_data & copydata) {
    if (value == "true") {
        copydata.features |= Named_Node_List::prepend_mask;
    }
    return true;
}

const copy_data_map_t NNL_list_copy_features = {
    {"copy", copy_data_from_name},
    {"from_max", copy_data_from_max},
    {"to_max", copy_data_to_max},
    {"maxsize", copy_data_maxsize},
    {"unique", copy_data_unique},
    {"fifo", copy_data_fifo},
    {"prepend", copy_data_prepend}
};

bool get_copy_data(const GET_token_value_vec & token_value_vec, NNL_copy_data & copydata) {
    copydata.features = 0; // need this for mask ORs
    copydata.maxsize = 0;
    for (const auto & GETel : token_value_vec) {
        auto it = NNL_list_copy_features.find(GETel.token);
        if (it == NNL_list_copy_features.end()) {
            return standard_error("Unexpected token: '" + GETel.token + '\'', __func__);
        }
        if (!(it->second(GETel.value, copydata))) {
            return false;
        }
    }
    if ((copydata.maxsize<=0) || (copydata.features == 0)) {
        copydata.features = -1;
        copydata.maxsize = -1;
    }
    return true;
}

/*
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
        } else if (GETel.token == "maxsize") {
            maxsize = std::atoi(GETel.value.c_str());
            features_specified = true;
        } else if (GETel.token == "unique") {
            if (GETel.value == "true") {
                features = copydata.features | Named_Node_List::unique_mask;
                features_specified = true;
            }
        } else if (GETel.token == "fifo") {
            if (GETel.value == "true") {
                features = copydata.features | Named_Node_List::fifo_mask;
                features_specified = true;
            }
        } else if (GETel.token == "prepend") {
            if (GETel.value == "true") {
                features = copydata.features | Named_Node_List::prepend_mask;
                features_specified = true;
            }
        } else {
            return standard_error("Unexpected token: '"+GETel.token+'\'', __func__);
        }
    }
    if (features_specified) {
        copydata.features = features;
        copydata.maxsize = maxsize;
    }
    return true;
}
*/

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

    response_html = standard_HTML_header("fz: Copy to NNL") +
                    "<p>Named Node List modified.</p>\n"
                    "<p><b>Copied</b> "+std::to_string(copied)+"Nodes from "+copydata.from_name+" to List "+list_name+".</p>\n"
                    "</body>\n</html>\n";

    return true;
}

struct NNL_add_data {
    Node_ID_key nkey;
    int16_t features = 0;
    int32_t maxsize = 0;
    char move = '_';
    unsigned int from_position = 0;
    unsigned int to_position = 0;
};

typedef bool add_data_func_t(const std::string&, NNL_add_data&);
typedef std::map<std::string, add_data_func_t*> add_data_map_t;

bool add_data_nkey(const std::string &value, NNL_add_data & adddata) {
    try {
        adddata.nkey = Node_ID_key(value);
    } catch (ID_exception idexception) {
        return standard_error("Add Named Node List request has invalid Node ID [" + value + "], " + idexception.what(), __func__);
    }
    return true;
}

bool add_data_maxsize(const std::string &value, NNL_add_data & adddata) {
    adddata.maxsize = std::atoi(value.c_str());
    return true;
}

bool add_data_unique(const std::string &value, NNL_add_data & adddata) {
    if (value == "true") {
        adddata.features = adddata.features | Named_Node_List::unique_mask;
    }
    return true;
}

bool add_data_fifo(const std::string &value, NNL_add_data & adddata) {
    if (value == "true") {
        adddata.features = adddata.features | Named_Node_List::fifo_mask;
    }
    return true;
}

bool add_data_prepend(const std::string &value, NNL_add_data & adddata) {
    if (value == "true") {
        adddata.features = adddata.features | Named_Node_List::prepend_mask;
    }
    return true;
}

bool add_data_move(const std::string &value, NNL_add_data & adddata) {
    adddata.from_position = atoi(value.c_str());
    return true;
}

bool add_data_up(const std::string &value, NNL_add_data & adddata) {
    adddata.move = 'u';
    return true;
}

bool add_data_down(const std::string &value, NNL_add_data & adddata) {
    adddata.move = 'd';
    return true;
}

bool add_data_to(const std::string &value, NNL_add_data & adddata) {
    adddata.move = 't';
    adddata.to_position = atoi(value.c_str());
    return true;
}

const add_data_map_t NNL_list_add_features = {
    {"add", add_data_nkey},
    {"maxsize", add_data_maxsize},
    {"unique", add_data_unique},
    {"fifo", add_data_fifo},
    {"prepend", add_data_prepend},
    {"move", add_data_move},
    {"up", add_data_up},
    {"down", add_data_down},
    {"to", add_data_to}
};

/*
const Command_Token_Map NNL_list_add_features = {
    {"add", NNLlistcmdfeature_add},
    {"maxsize", NNLlistcmdfeature_maxsize},
    {"unique", NNLlistcmdfeature_unique},
    {"fifo", NNLlistcmdfeature_fifo},
    {"prepend", NNLlistcmdfeature_prepend}
};
        switch (it->second) {

            case NNLlistcmdfeature_add: {
                break;
            }

            case NNLlistcmdfeature_maxsize: {
                break;
            }

            case NNLlistcmdfeature_unique: {
                break;
            }

            case NNLlistcmdfeature_fifo: {
                break;
            }

            case NNLlistcmdfeature_prepend: {
                break;
            }

            default: {
                // never gets here
            }
        }
*/

bool get_add_data(const GET_token_value_vec & token_value_vec, NNL_add_data & adddata) {
    for (const auto &GETel : token_value_vec) {
        auto it = NNL_list_add_features.find(GETel.token);
        if (it == NNL_list_add_features.end()) {
            return standard_error("Unexpected token: '" + GETel.token + '\'', __func__);
        }
        if (!(it->second(GETel.value, adddata))) {
            return false;
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

    response_html = standard_HTML_header("fz: Add to NNL", AUTO_CLOSING_JS_BODY_TAG) +
                    "<p>Named Node List modified.</p>\n"
                    "<p><b>Added</b> "+adddata.nkey.str()+" to List "+list_name+".</p>\n"
                    AUTO_CLOSING_JS
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
            return standard_error("Unexpected token: '"+GETel.token+'\'', __func__);
        }
    }

    if (!fzs.graph_ptr->remove_from_List(list_name, nkey)) {
        return standard_error("Unable to remove Node "+nkey.str()+" from Named Node List "+list_name, __func__);
    }

    response_html = standard_HTML_header("fz: Remove from NNL") +
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

    response_html = standard_HTML_header("fz: Delete NNL") +
                    "<p>Named Node List modified.</p>\n"
                    "<p><b>Deleted</b> List " + list_name + ".</p>\n"
                    "</body>\n</html>\n";

    return true;
}

bool handle_move_within_list(const std::string & list_name, const GET_token_value_vec & token_value_vec, std::string & response_html) {
    ERRTRACE;

    VERYVERBOSEOUT("Moving Node within Named Node List "+list_name+'\n');

    NNL_add_data adddata;
    if (!get_add_data(token_value_vec, adddata)) {
        return standard_error("Unable to carry out 'move' within Named Node List", __func__);
    }

    // confirm that the NNL exists and the Node ID position to move from is within the Named Node List
    auto nnl_ptr = fzs.graph_ptr->get_List(list_name);
    if (!nnl_ptr) {
        return standard_error("Named Node List ("+list_name+") not found in Graph for move within list request", __func__);
    }
    if (adddata.from_position >= nnl_ptr->size()) {
        return standard_error("Position "+std::to_string(adddata.from_position)+" does not exist in Named Node List ("+list_name+")", __func__);
    }

    bool res = false;
    switch (adddata.move) {
        case 'u': {
            res = nnl_ptr->move_toward_head(adddata.from_position);
            break;
        }
        case 'd': {
            res = nnl_ptr->move_toward_tail(adddata.from_position);
            break;
        }
        case 't': {
            res = nnl_ptr->move_to_position(adddata.from_position, adddata.to_position);
            break;
        }
        default: {
            standard_error("Unrecognized 'move' request ("+std::string(1, adddata.move)+") for Named Node List", __func__);
        }
    }
    if (!res) {
        return standard_error("Unable to move Node within Named Node List "+list_name, __func__);
    }

    // synchronize with stored List
    if (fzs.graph_ptr->persistent_Lists()) {
        if (!Update_Named_Node_List_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), list_name, *fzs.graph_ptr)) {
            return standard_error("Synchronizing Named Node List update to database failed", __func__);
        }
    }

    response_html = standard_HTML_header("fz: Move within NNL") +
                    "<p>Named Node List modified.</p>\n"
                    "<p><b>Moved</b> within List "+list_name+".</p>\n"
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
    {"_select", NNLuscrcmd_select},
    {"_recent", NNLuscrcmd_recent}
};

const Command_Token_Map NNL_list_commands = {
    {"add", NNLlistcmd_add},
    {"remove", NNLlistcmd_remove},
    {"copy", NNLlistcmd_copy},
    {"delete", NNLlistcmd_delete},
    {"move", NNLlistcmd_move}
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

/**
 * Updates a Node's completion ratio (and potentially updates required if
 * completion exceeds 1.0) in response to having logged a number of minutes
 * dedicated to the Node. For repeating Nodes, updates the targetdate if
 * specific conditions are met.
 * 
 * Notes:
 * 1. `add_minutes` is necessarily >= 0, which is different than directly
 * modifying parameters in ways that can increase or reduce. It only makes sense
 * to log positive time.
 * 2. This function assumes that `fzs.graph_ptr` points to a valid
 * Graph, meaning that this function is called sometime after `load_Graph_and_stay_resident()`.
 * 
 * *** Future improvement notes:
 * This is one of the functions in `fzserverpq` that deals directly with database
 * update calls, which would need to be modified to use a different database layer.
 * (It may be sensible to collect all of those functions in a separate source file
 * for a clear and logical separation that simplifies the addition of other
 * possible database layers.)
 * 
 * @param node_addstr A string specifying the ID of a Node in the Graph and possibly an emulated time.
 * @return True if successfully updated. False if the Node could not be identified
 *         or if another error occurred.
 */
//bool node_add_logged_time(const std::string & node_idstr, unsigned int add_minutes) {
bool node_add_logged_time(const std::string & node_addstr) {
    time_t T_ref = RTt_unspecified;
    unsigned int add_minutes = 0;
    Node * node_ptr = nullptr;

    // separate into token-value pairs
    auto token_value_vec = GET_token_values(node_addstr);
    // identify and set state variables
    for (const auto & GETel : token_value_vec) {
        if (GETel.token == "T") {
            time_t t = time_stamp_time(GETel.value);
            if (t<0) {
                return standard_error("Unable to use emulated time string "+GETel.value, __func__);
            }
            T_ref = t;
        } else if (GETel.token.size()==NODE_ID_STR_NUMCHARS) {
            node_ptr = fzs.graph_ptr->Node_by_idstr(GETel.token);
            if (!node_ptr) {
                return standard_error("Node "+GETel.token+" not found in Graph.", __func__);
            }
            add_minutes = std::atoi(GETel.value.c_str());
        }
    }

    if (!node_ptr) {
        return standard_error("Missing Node ID token", __func__);
    }
    
    To_Debug_LogFile("Add "+std::to_string(add_minutes)+" mins to "+node_ptr->get_id_str());
    std::vector<long> profiling_us; // PROFILING (remove this)
    auto t1 = std::chrono::high_resolution_clock::now(); // PROFILING (remove this)

    // carry out modification
    Edit_flags editflags = Node_apply_minutes(*node_ptr, add_minutes, T_ref);
    auto t2 = std::chrono::high_resolution_clock::now(); // PROFILING (remove this)
    profiling_us.emplace_back(std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()); // PROFILING (remove this)
    
    // update database
    if (!Update_Node_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), *node_ptr, editflags)) {
        return standard_error("Synchronizing Node update to database failed", __func__);
    }
    auto t3 = std::chrono::high_resolution_clock::now(); // PROFILING (remove this)
    profiling_us.emplace_back(std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count()); // PROFILING (remove this)
    std::string profiling_str; // PROFILING (remove this)
    for (auto& us : profiling_us) { // PROFILING (remove this)
        profiling_str += std::to_string(us/1000000)+' '; // PROFILING (remove this)
    } // PROFILING (remove this)
    To_Debug_LogFile("Seconds taken by steps: "+profiling_str);

    // post-modification validity test
    if (editflags.Edit_error()) { // check this AFTER synchronizing (see note in Graphmodify.hpp:Edit_flags)
        return standard_error("An invalid circumstance was encountered while attempting to add minutes to Node "+node_ptr->get_id_str()+", but some parameters (e.g. completion) may have been modified.", __func__);
    }

    return true;
}

/**
 * Redirects to Node presentation through fzgraphhtml in a format specified by the extension.
 * For example:
 *   /fz/graph/nodes/20200901061505.1.html leads to fzgraphhtml -n 20200901061505.1 -F html
 *   /fz/graph/nodes/20200901061505.1.desc leads to fzgraphhtml -n 20200901061505.1 -F desc
 */
bool handle_node_direct_show(Node & node, const std::string & extension, std::string & response_html) {
    response_html = shellcmd2str("fzgraphhtml -q -n "+node.get_id_str()+" -F "+extension.substr(1));
    To_Debug_LogFile("Testing formatted node info: "+response_html);
    return true;
}

/**
 * Somewhat similar to the shared-memory interface to editing multiple parameters of a Node.
 * This includes changing the values of multiple parameters and setting the Node's `Edit_flags`,
 * then calling `Update_Node_pq()`, but it does not include using a `Graph_modifications` stack
 * or responding with results in shared-memory. Edits can set or add.
 * For example:
 *   /fz/graph/nodes/20200901061505.1?completion=1.0&repeats=no [NOT YET IMPLEMENTED]
 *   /fz/graph/nodes/20200901061505.1?required=+45m [NOT YET IMPLEMENTED]
 *   /fz/graph/nodes/20200901061505.1?skip=1
 *   /fz/graph/nodes/20200901061505.1?skip=toT&T=202101271631
 *   /fz/graph/nodes/20200901061505.1?targetdate=202409162045
 */
bool handle_node_direct_edit_multiple_pars(Node & node, const std::string & extension, std::string & response_html) {
    ERRTRACE;

    if (extension.size()<2) {
        return false;
    }

    response_html = standard_HTML_header("fz: Node Edit Parameters") + "<p>Node edit multi-parameters: "+extension.substr(1)+"</p>\n</body>\n</html>\n";
    // skip '?' and separate in the token-value pairs
    auto token_value_vec = GET_token_values(extension.substr(1));
    // identify and set state variables (e.g. T=)
    time_t T_ref = RTt_unspecified;
    for (const auto & GETel : token_value_vec) {
        if (GETel.token == "T") {
            time_t t = time_stamp_time(GETel.value);
            if (t<0) {
                return standard_error("Unable to use emulated time string "+GETel.value, __func__);
            }
            T_ref = t;
        }
    }
    if (T_ref==RTt_unspecified) {
        T_ref = ActualTime();
    }
    // loop through action tokens and carry out modifications
    Edit_flags editflags;
    for (const auto & GETel : token_value_vec) {
        // *** Note that if we have a lot of recognized tokens then we could opt to do this as in get_add_data() above,
        //     or as in tcp_serialized_data_handlers.cpp.
        // *** To extend this, first, let's just build it as part of an if-then with the necessary Edit mapping,
        //     then, once that works, switch to a map.
        To_Debug_LogFile("Processing multiple parameter modification for node "+node.get_id_str()+" token="+GETel.token+" value="+GETel.value);
        if (GETel.token == "skip") {
            if (!node.get_repeats()) {
                return standard_error("Unable to skip instances of non-repeating Node "+node.get_id_str(), __func__);
            }
            if (GETel.value=="toT") {
                Node_skip_tpass(node, T_ref, editflags);
            } else {
                bool usable = !GETel.value.empty();
                if (usable) {
                    usable =  ((GETel.value[0]>='0') && (GETel.value[0]<='9'));
                }
                if (!usable) {
                    return standard_error("Invalid skip number "+GETel.value, __func__);
                }
                unsigned int num_skip = std::atoi(GETel.value.c_str());
                Node_skip_num(node, num_skip, editflags);
            }
        } else if (GETel.token == "targetdate") {
            time_t t = time_stamp_time(GETel.value);
            if (t == RTt_invalid_time_stamp) {
                return standard_error("Invalid time stamp "+GETel.value, __func__);
            }
            node.set_targetdate(t);
            editflags.set_Edit_targetdate();
        } else if (GETel.token == "required") {
            float req = std::atof(GETel.value.c_str());
            time_t req_seconds = req*3600.0;
            node.set_required(req_seconds);
            editflags.set_Edit_required();
        }
    }    
    // update database
    if (!Update_Node_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), node, editflags)) {
        return standard_error("Synchronizing Node update to database failed", __func__);
    }

    // post-modification validity test
    if (editflags.Edit_error()) { // check this AFTER synchronizing (see note in Graphmodify.hpp:Edit_flags)
        return standard_error("An invalid circumstance was encountered while attempting to edit Node "+node.get_id_str()+", but some parameters may have been modified.", __func__);
    }

    return true;
}

typedef bool node_parameter_edit_func_t(Node &, std::string);
typedef std::map<Edit_flags_type, node_parameter_edit_func_t*> node_parameter_edit_map_t;

// Note: This route was tested by:
//       1. calling: fzgraph -C "/fz/graph/nodes/20200901061505.1/completion?set=0.123"
//       2. checking the server log: cat /var/www/html/formalizer/formalizer.core.server.requests.log
//       3. checking the new completion state of Node 20200901061505.1 without explicitly reloading
//          the fzserverpq.
//       4. stopping fzserverpq and reloading (to confirm update in the database).
bool set_completion(Node & node, std::string valstr) {
    if (valstr.empty()) {
        return standard_error("Missing parameter value", __func__);
    }
    float completion = std::atof(valstr.c_str()); // here, we allow everything (including special negative value codes)
    node.set_completion(completion);
    const_cast<Edit_flags *>(&(node.get_editflags()))->set_Edit_completion();
#ifdef TEST_MORE_THAN_NODE_MODIFICATIONS
    fzs.modifications_ptr->add(graphmod_edit_node, node.get_id().key());
#endif
    return true;
}

// Note: This route was tested by:
//       1. calling: fzgraph -C "/fz/graph/nodes/20200901061505.1/completion?add=0.123"
//       2. checking the server log: cat /var/www/html/formalizer/formalizer.core.server.requests.log
//       3. checking the new completion state of Node 20200901061505.1 without explicitly reloading
//          the fzserverpq.
//       4. stopping fzserverpq and reloading (to confirm update in the database).
bool add_completion(Node & node, std::string valstr) {
    if (valstr.empty()) {
        return standard_error("Missing parameter value", __func__);
    }
    float completion = node.get_completion();
    if (valstr.back() == 'm') { // deduce ratio by comparing minutes
        valstr.pop_back();
        long add_minutes = std::atol(valstr.c_str());
        long current_minutes = node.get_required_minutes();
        if (current_minutes <= 0) {
            return standard_error("Can not deduce ratio to add by comparing minutes with current value <= 0", __func__);
        }
        float add_ratio = ((float)add_minutes) / ((float)current_minutes);
        completion += add_ratio;
    } else {
        completion += std::atof(valstr.c_str());
    }
    if (completion <= 0.0) {
        node.set_completion(0.0);
    } else {
        if (completion >= 1.0) {
            node.set_completion(1.0);
        } else {
            node.set_completion(completion);
        }
    }
    const_cast<Edit_flags *>(&(node.get_editflags()))->set_Edit_completion();
#ifdef TEST_MORE_THAN_NODE_MODIFICATIONS
    fzs.modifications_ptr->add(graphmod_edit_node, node.get_id().key());
#endif
    return true;
}

// Note: This route was tested by:
//       1. calling: fzgraph -C "/fz/graph/nodes/20200901061505.1/required?set=0.123h"
//       2. checking the server log: cat /var/www/html/formalizer/formalizer.core.server.requests.log
//       3. checking the new completion state of Node 20200901061505.1 without explicitly reloading
//          the fzserverpq.
//       4. stopping fzserverpq and reloading (to confirm update in the database).
bool set_required(Node & node, std::string valstr) {
    if (valstr.empty()) {
        return standard_error("Missing parameter value", __func__);
    }
    bool in_minutes = (valstr.back() == 'm');
    if (!in_minutes) {
        if (valstr.back() != 'h') {
            return standard_error("Unrecognized units of time in parameter value: '" + valstr + '\'', __func__);
        }
    }
    valstr.pop_back();
    time_t required;
    if (in_minutes) {
        long required_minutes = std::atol(valstr.c_str());
        required = 60*required_minutes;
    } else {
        float required_hours = std::atof(valstr.c_str());
        required = (3600.0*required_hours);
    }
    node.set_required(required); // we permit explicit setting of any value, including negative value codes
    const_cast<Edit_flags *>(&(node.get_editflags()))->set_Edit_required();
#ifdef TEST_MORE_THAN_NODE_MODIFICATIONS
    fzs.modifications_ptr->add(graphmod_edit_node, node.get_id().key());
#endif
    return true;
}

// Note: This route was tested by:
//       1. calling: fzgraph -C "/fz/graph/nodes/20200901061505.1/required?add=0.123h"
//       2. checking the server log: cat /var/www/html/formalizer/formalizer.core.server.requests.log
//       3. checking the new completion state of Node 20200901061505.1 without explicitly reloading
//          the fzserverpq.
//       4. stopping fzserverpq and reloading (to confirm update in the database).
bool add_required(Node & node, std::string valstr) {
    if (valstr.empty()) {
        return standard_error("Missing parameter value", __func__);
    }
    bool in_minutes = (valstr.back() == 'm');
    if (!in_minutes) {
        if (valstr.back() != 'h') {
            return standard_error("Unrecognized units of time in parameter value: '" + valstr + '\'', __func__);
        }
    }
    valstr.pop_back();
    time_t required;
    if (in_minutes) {
        long required_minutes = std::atol(valstr.c_str()) + node.get_required_minutes();
        required = 60*required_minutes;
    } else {
        float required_hours = std::atof(valstr.c_str()) + node.get_required_hours();
        required = (3600.0*required_hours);
    }
    if (required <= 0) {
        node.set_required(0);
    } else {
        node.set_required(required);
    }
    const_cast<Edit_flags *>(&(node.get_editflags()))->set_Edit_required();
#ifdef TEST_MORE_THAN_NODE_MODIFICATIONS
    fzs.modifications_ptr->add(graphmod_edit_node, node.get_id().key());
#endif
    return true;
}

const node_parameter_edit_map_t node_parameter_edit_add_map = {
    {Edit_flags::completion, add_completion},
    {Edit_flags::required, add_required}
};

const node_parameter_edit_map_t node_parameter_edit_set_map = {
    {Edit_flags::completion, set_completion},
    {Edit_flags::required, set_required}
};

typedef bool node_parameter_show_by_extension_func_t(std::string, std::string, std::string &);
typedef std::map<std::string, node_parameter_show_by_extension_func_t*> node_parameter_show_by_extension_map_t;

bool handle_node_parameter_show_raw(std::string parlabel, std::string parvalue, std::string & response_html) {
    response_html = parvalue;
    return true;
}

bool handle_node_parameter_show_txt(std::string parlabel, std::string parvalue, std::string & response_html) {
    response_html = parlabel+'='+parvalue;
    return true;
}

bool handle_node_parameter_show_html(std::string parlabel, std::string parvalue, std::string & response_html) {
    response_html = standard_HTML_header("fz: Node Show Parameter") +
        parlabel + " = " + parvalue
        + "\n</body>\n</html>\n";
    return true;
}

bool handle_node_parameter_show_json(std::string parlabel, std::string parvalue, std::string & response_html) {
    response_html = "{\n"
        "    \"" + parlabel + "\": " + parvalue
        + "\n}\n";
    return true;
}

const node_parameter_show_by_extension_map_t node_parameter_show_by_extension_map = {
    {"raw", handle_node_parameter_show_raw},
    {"txt", handle_node_parameter_show_txt},
    {"html", handle_node_parameter_show_html},
    {"json", handle_node_parameter_show_json},
};

bool handle_node_strdata_show_raw(std::string parlabel, std::string parvalue, std::string & response_html) {
    response_html = parvalue;
    return true;
}

bool handle_node_strdata_show_txt(std::string parlabel, std::string parvalue, std::string & response_html) {
    response_html = parlabel+'='+parvalue;
    return true;
}

bool handle_node_strdata_show_html(std::string parlabel, std::string parvalue, std::string & response_html) {
    response_html = standard_HTML_header("fz: Node Data Show") +
        parlabel + " = " + parvalue
        + "\n</body>\n</html>\n";
    return true;
}

bool handle_node_strdata_show_json(std::string parlabel, std::string parvalue, std::string & response_html) {
    response_html = "{\n"
        "    \"" + parlabel + "\": \"" + replace_char(parvalue, '"', '\'')
        + "\"\n}\n";
    return true;
}

const node_parameter_show_by_extension_map_t node_strdata_show_by_extension_map = {
    {"raw", handle_node_strdata_show_raw},
    {"txt", handle_node_strdata_show_txt},
    {"html", handle_node_strdata_show_html},
    {"json", handle_node_strdata_show_json},
};

typedef bool node_map_show_by_extension_func_t(std::string, std::map<std::string, std::string>, std::string &);
typedef std::map<std::string, node_map_show_by_extension_func_t*> node_map_show_by_extension_map_t;

bool handle_node_map_show_raw(std::string parlabel, const std::map<std::string, std::string> parmap, std::string & response_html) {
    for (const auto & [pairlabel, pairvalue] : parmap) {
        response_html += pairlabel + ',' + pairvalue + '\n';
    }
    return true;
}

bool handle_node_map_show_txt(std::string parlabel, const std::map<std::string, std::string> parmap, std::string & response_html) {
    for (const auto & [pairlabel, pairvalue] : parmap) {
        response_html += pairlabel + ':' + pairvalue + '\n';
    }
    return true;
}

bool handle_node_map_show_html(std::string parlabel, const std::map<std::string, std::string> parmap, std::string & response_html) {
    response_html = standard_HTML_header("fz: Node Show Map") + "<table>\n";
    for (const auto & [pairlabel, pairvalue] : parmap) {
        response_html += "<tr><td>"+pairlabel + "</td><td>" + pairvalue + "</td></tr>\n";
    }
    response_html += "</table>\n</body>\n</html>\n";
    return true;
}

bool handle_node_map_show_json(std::string parlabel, const std::map<std::string, std::string> parmap, std::string & response_html) {
    response_html = "{";
    for (const auto & [pairlabel, pairvalue] : parmap) {
        response_html += "\n    \"" + replace_char(pairlabel, '"', '\'') + "\": \"" + replace_char(pairvalue, '"', '\'') + "\",";
    }
    response_html.back() = '\n';
    response_html += "}\n";
    return true;
}

const node_map_show_by_extension_map_t node_map_show_by_extension_map = {
    {"raw", handle_node_map_show_raw},
    {"txt", handle_node_map_show_txt},
    {"html", handle_node_map_show_html},
    {"json", handle_node_map_show_json},
};

typedef bool node_parameter_show_func_t(Node &, std::string, std::string &);
typedef std::map<std::string, node_parameter_show_func_t*> node_parameter_show_map_t;

bool single_node_parameter_show(const std::string & show_type_extension, std::string parlabel, std::string parvalue, std::string & response_html) {
    To_Debug_LogFile("Processing node data with format "+show_type_extension);
    if (show_type_extension.empty()) {
        return standard_error("Missing parameter show type extension", __func__);
    }
    auto it = node_parameter_show_by_extension_map.find(show_type_extension);
    if (it == node_parameter_show_by_extension_map.end()) {
        return standard_error("Unrecognized parameter show type extension ("+show_type_extension+')', __func__);
    }
    return it->second(parlabel, parvalue, response_html);
}

bool handle_node_valuation_show(Node & node, std::string show_type_extension, std::string & response_html) {
    return single_node_parameter_show(show_type_extension, "valuation", to_precision_string(node.get_valuation(), 2), response_html);
}

bool handle_node_completion_show(Node & node, std::string show_type_extension, std::string & response_html) {
    return single_node_parameter_show(show_type_extension, "completion", to_precision_string(node.get_completion(), 5), response_html);
}

bool handle_node_required_show(Node & node, std::string show_type_extension, std::string & response_html) {
    return single_node_parameter_show(show_type_extension, "required", to_precision_string(node.get_required_hours(), 2), response_html);
}

bool handle_node_targetdate_show(Node & node, std::string show_type_extension, std::string & response_html) {
    return single_node_parameter_show(show_type_extension, "targetdate", node.get_targetdate_str(), response_html);
}

bool single_node_strdata_show(const std::string & show_type_extension, std::string parlabel, std::string parvalue, std::string & response_html) {
    if (show_type_extension.empty()) {
        return standard_error("Missing string data show type extension", __func__);
    }
    auto it = node_strdata_show_by_extension_map.find(show_type_extension);
    if (it == node_strdata_show_by_extension_map.end()) {
        return standard_error("Unrecognized string data show type extension ("+show_type_extension+')', __func__);
    }
    return it->second(parlabel, parvalue, response_html);
}

bool handle_node_text_show(Node & node, std::string show_type_extension, std::string & response_html) {
    return single_node_strdata_show(show_type_extension, "text", node.get_text().c_str(), response_html);
}

bool handle_node_tdproperty_show(Node & node, std::string show_type_extension, std::string & response_html) {
    return single_node_strdata_show(show_type_extension, "tdproperty", node.get_tdproperty_str(), response_html);
}

bool handle_node_repeats_show(Node & node, std::string show_type_extension, std::string & response_html) {
    std::string repeats_str = node.get_repeats() ? "true" : "false";
    return single_node_parameter_show(show_type_extension, "repeats", repeats_str, response_html);
}

bool handle_node_tdpattern_show(Node & node, std::string show_type_extension, std::string & response_html) {
    return single_node_strdata_show(show_type_extension, "tdpattern", node.get_tdpattern_str(), response_html);
}

bool handle_node_tdevery_show(Node & node, std::string show_type_extension, std::string & response_html) {
    return single_node_parameter_show(show_type_extension, "tdevery", std::to_string(node.get_tdevery()), response_html);
}

bool handle_node_tdspan_show(Node & node, std::string show_type_extension, std::string & response_html) {
    return single_node_parameter_show(show_type_extension, "tdspan", std::to_string(node.get_tdspan()), response_html);
}

bool single_node_map_show(const std::string & show_type_extension, std::string parlabel, const std::map<std::string, std::string> & parmap, std::string & response_html) {
    if (show_type_extension.empty()) {
        return standard_error("Missing string data show type extension", __func__);
    }
    auto it = node_map_show_by_extension_map.find(show_type_extension);
    if (it == node_map_show_by_extension_map.end()) {
        return standard_error("Unrecognized map show type extension ("+show_type_extension+')', __func__);
    }
    return it->second(parlabel, parmap, response_html);
}

bool handle_node_topics_show(Node & node, std::string show_type_extension, std::string & response_html) {
    auto topics = node.get_topics();
    std::map<std::string, std::string> topics_map;
    for (const auto & [topic_id, topic_rel] : topics) {
        topics_map.emplace(fzs.graph().find_Topic_Tag_by_id(topic_id), to_precision_string(topic_rel, 2));
    }
    return single_node_map_show(show_type_extension, "topics", topics_map, response_html);
}

bool handle_node_NNLs_show(Node & node, std::string show_type_extension, std::string & response_html) {
    auto nnls_set = fzs.graph().find_all_NNLs_Node_is_in(node);
    std::map<std::string, std::string> nnls_map;
    for (const auto & list_name : nnls_set) {
        nnls_map.emplace(list_name, "");
    }
    return single_node_map_show(show_type_extension, "in_NNLs", nnls_map, response_html);
}

const node_parameter_show_map_t node_parameter_show_map = {
    {"valuation", handle_node_valuation_show},
    {"completion", handle_node_completion_show},
    {"required", handle_node_required_show},
    {"targetdate", handle_node_targetdate_show},
    {"text", handle_node_text_show},
    {"tdproperty", handle_node_tdproperty_show},
    {"repeats", handle_node_repeats_show},
    {"tdpattern", handle_node_tdpattern_show},
    {"tdevery", handle_node_tdevery_show},
    {"tdspan", handle_node_tdspan_show},
    {"topics", handle_node_topics_show},
    {"in_NNLs", handle_node_NNLs_show},
};

typedef bool node_context_edit_func_t(Node &, std::string);
typedef std::map<std::string, node_context_edit_func_t*> node_context_edit_map_t;

/**
 * E.g. /fz/graph/nodes/20200901061505.1/topics/add?organization=1.0&oop-change=1.0
 *      /fz/graph/nodes/20200901061505.1/topics/remove?literature=[1.0]
 * @param node The Node to edit.
 * @param editstr String containing an edit command and parameter label-value pairs.
 */
bool handle_node_topics_edit(Node & node, std::string editstr) {
    if (editstr.empty()) {
        return standard_error("Missing parameter value", __func__);
    }
    if (editstr.substr(0,4) == "add?") {
        unsigned int num_topics_added = 0;
        auto topics_relevances_vec = split(editstr.substr(4), '&');
        for (const auto & topic_relevance : topics_relevances_vec) {
            auto topic_relevance_pair = split(topic_relevance, '=');
            if (topic_relevance_pair.size() < 2) {
                standard_error("Incomplete parameter value pair ("+topic_relevance+')', __func__);
                continue;
            }
            if (!node.add_topic(topic_relevance_pair[0], "", atof(topic_relevance_pair[1].c_str()))) {
                standard_error("Failed to add topic ("+topic_relevance_pair[0]+')', __func__);
                continue;
            }
            num_topics_added++;
        }
        if (num_topics_added==0) {
            return standard_error("Failed to add topics ("+editstr.substr(4)+')', __func__);
        }
    } else if (editstr.substr(0,7) == "remove?") {
        auto topic_relevance_pair = split(editstr.substr(7), '=');
        if (topic_relevance_pair.size() < 1) {
            return standard_error("Missing topic tag", __func__);
        }
        if (!node.remove_topic(topic_relevance_pair[0])) {
            return standard_error("Failed to remove topic ("+topic_relevance_pair[0]+')', __func__);
        }
    } else {
        return standard_error("Unrecognized Topic edit string: '" + editstr + '\'', __func__);
    }
    const_cast<Edit_flags *>(&(node.get_editflags()))->set_Edit_topics();
    const_cast<Edit_flags *>(&(node.get_editflags()))->set_Edit_topicrels();
#ifdef TEST_MORE_THAN_NODE_MODIFICATIONS
    fzs.modifications_ptr->add(graphmod_edit_node, node.get_id().key());
#endif
    return true;
}

typedef bool node_superiors_edit_func_t(Node &, std::string);
typedef std::map<std::string, node_superiors_edit_func_t*> node_superiors_edit_map_t;

// Note: This route was tested by:
//       1. calling: fzgraph -C "/fz/graph/nodes/20231106091904.1/superiors/add?20231218212505.1="
//       2. checking the server log: cat /var/www/html/formalizer/formalizer.core.server.requests.log
//       3. checking the new superiors state of Node 20231106091904.1 without explicitly reloading
//          the fzserverpq.
//       4. stopping fzserverpq and reloading (to confirm update in the database).
bool handle_node_superiors_add(Node & node, std::string superiorstr) {
    if (superiorstr.empty()) {
        return standard_error("Missing superior ID", __func__);
    }
    if (!fzs.graph().Node_by_idstr(superiorstr)) {
        return standard_error("Invalid superior ID ("+superiorstr+')', __func__);
    }
#ifdef TEST_MORE_THAN_NODE_MODIFICATIONS
    std::string edge_id_str = node.get_id_str()+'>'+superiorstr;
    Edge_ptr edge_ptr = fzs.graph().create_and_add_Edge(edge_id_str);
    if (!edge_ptr) {
        return standard_error("Creating edge failed for "+edge_id_str, __func__);
    }
    // *** set the edit flag in the Edge (when it has those)
    fzs.modifications_ptr->add(graphmod_add_edge, edge_ptr->get_id().key());
    return true;
#else
    return standard_error("MISSING IMPLEMENTATION: add superior", __func__);    
#endif
}

bool handle_node_superiors_remove(Node & node, std::string superiorstr) {
    if (superiorstr.empty()) {
        return standard_error("Missing superior ID", __func__);
    }
    Edge * edge_ptr = node.get_Edge_by_sup(superiorstr);
    if (!edge_ptr) {
        return standard_error("Node "+node.get_id_str()+" does not have superior with ID "+superiorstr, __func__);
    }
#ifdef TEST_MORE_THAN_NODE_MODIFICATIONS
    if (!fzs.graph().remove_Edge(edge_ptr)) {
        return standard_error("Failed to remove Edge "+edge_ptr->get_id_str()+" from in-memory Graph", __func__);
    }
    // *** set the edit flag in the Edge (when it has those)
    fzs.modifications_ptr->add(graphmod_remove_edge, edge_ptr->get_key());
    return true;
#else
    return standard_error("MISSING IMPLEMENTATION: remove superior", __func__);
#endif
}

bool handle_node_superiors_addlist(Node & node, std::string superiorslist) {
    if (superiorslist.empty()) {
        return standard_error("Missing superiors list", __func__);
    }
    Named_Node_List_ptr superiorsNNL_ptr = fzs.graph().get_List(superiorslist);
    if (!superiorsNNL_ptr) {
        return standard_error("Named Node List "+superiorslist+" not found.", __func__);
    }
#ifdef TEST_MORE_THAN_NODE_MODIFICATIONS
    To_Debug_LogFile("Started processing superiors NNL to add to Node "+node.get_id_str());
    for (const auto & superior_idkey : superiorsNNL_ptr->list) {
        std::string edge_id_str = node.get_id_str()+'>'+superior_idkey.str();
        To_Debug_LogFile("New Edge to add: "+edge_id_str);
        Edge_ptr edge_ptr = fzs.graph().create_and_add_Edge(edge_id_str);
        if (!edge_ptr) {
            return standard_error("Creating edge failed for "+edge_id_str, __func__);
        }
        // *** set the edit flag in the Edge (when it has those)
        fzs.modifications_ptr->add(graphmod_add_edge, edge_ptr->get_id().key());
    }
    To_Debug_LogFile("Completed processing superiors NNL to add to Node "+node.get_id_str());
    return true;
#else
    return standard_error("MISSING IMPLEMENTATION: addlist to superiors", __func__);
#endif
}

const node_superiors_edit_map_t node_superiors_edit_map = {
    {"add", handle_node_superiors_add},
    {"remove", handle_node_superiors_remove},
    {"addlist", handle_node_superiors_addlist}
};

/**
 * E.g. /fz/graph/nodes/20200901061505.1/superiors/add?20090309102906.1=
 *      /fz/graph/nodes/20200901061505.1/superiors/remove?20090309102906.1=
 *      /fz/graph/nodes/20200901061505.1/superiors/addlist?superiors=
 * @param node The Node to edit.
 * @param editstr String containing an edit command and Superior Node ID.
 */
bool handle_node_superiors_edit(Node & node, std::string editstr) {
    if (editstr.empty()) {
        return standard_error("Missing edit string", __func__);
    }
    auto parpos = editstr.find('?');
    if (parpos == std::string::npos) {
        return standard_error("Missing superiors parameters", __func__);
    }
    auto it = node_superiors_edit_map.find(editstr.substr(0, parpos));
    if (it == node_superiors_edit_map.end()) {
        return standard_error("Unsupported Node context request: '" + editstr.substr(0,parpos) + '\'', __func__);
    }
    parpos++;
    auto equalpos = editstr.find('=', parpos);
    if (equalpos == std::string::npos) equalpos = editstr.size();
    if (!it->second(node, editstr.substr(parpos, equalpos - parpos))) {
        return false;
    }
    return true;
}

typedef bool node_dependencies_edit_func_t(Node &, std::string);
typedef std::map<std::string, node_dependencies_edit_func_t*> node_dependencies_edit_map_t;

// Note: This route was tested by:
//       1. calling: fzgraph -C "/fz/graph/nodes/20231218212505.1/dependencies/add?20231205160914.1="
//       2. checking the server log: cat /var/www/html/formalizer/formalizer.core.server.requests.log
//       3. checking the new dependencies state of Node 20231218212505.1 without explicitly reloading
//          the fzserverpq.
//       4. stopping fzserverpq and reloading (to confirm update in the database).
bool handle_node_dependencies_add(Node & node, std::string dependencystr) {
    if (dependencystr.empty()) {
        return standard_error("Missing dependency ID", __func__);
    }
    if (!fzs.graph().Node_by_idstr(dependencystr)) {
        return standard_error("Invalid dependency ID ("+dependencystr+')', __func__);
    }
#ifdef TEST_MORE_THAN_NODE_MODIFICATIONS
    std::string edge_id_str = dependencystr+'>'+node.get_id_str();
    Edge_ptr edge_ptr = fzs.graph().create_and_add_Edge(edge_id_str);
    if (!edge_ptr) {
        return standard_error("Creating edge failed for "+edge_id_str, __func__);
    }
    // *** set the edit flag in the Edge (when it has those)
    fzs.modifications_ptr->add(graphmod_add_edge, edge_ptr->get_id().key());
    return true;
#else
    return standard_error("MISSING IMPLEMENTATION: add dependency", __func__);
#endif
}

bool handle_node_dependencies_remove(Node & node, std::string dependencystr) {
    if (dependencystr.empty()) {
        return standard_error("Missing dependency ID", __func__);
    }
    Edge * edge_ptr = node.get_Edge_by_dep(dependencystr);
    if (!edge_ptr) {
        return standard_error("Node "+node.get_id_str()+" does not have dependency with ID "+dependencystr, __func__);
    }
#ifdef TEST_MORE_THAN_NODE_MODIFICATIONS
    if (!fzs.graph().remove_Edge(edge_ptr)) {
        return standard_error("Failed to remove Edge "+edge_ptr->get_id_str()+" from in-memory Graph", __func__);
    }
    // *** set the edit flag in the Edge (when it has those)
    fzs.modifications_ptr->add(graphmod_remove_edge, edge_ptr->get_key());
    return true;
#else
    return standard_error("MISSING IMPLEMENTATION: remove dependency", __func__);
#endif
}

bool handle_node_dependencies_addlist(Node & node, std::string dependencieslist) {
    if (dependencieslist.empty()) {
        return standard_error("Missing dependencies list", __func__);
    }
    Named_Node_List_ptr dependenciesNNL_ptr = fzs.graph().get_List(dependencieslist);
    if (!dependenciesNNL_ptr) {
        return standard_error("Named Node List "+dependencieslist+" not found.", __func__);
    }
#ifdef TEST_MORE_THAN_NODE_MODIFICATIONS
    for (const auto & dependency_idkey : dependenciesNNL_ptr->list) {
        std::string edge_id_str = dependency_idkey.str()+'>'+node.get_id_str();
        Edge_ptr edge_ptr = fzs.graph().create_and_add_Edge(edge_id_str);
        if (!edge_ptr) {
            return standard_error("Creating edge failed for "+edge_id_str, __func__);
        }
        // *** set the edit flag in the Edge (when it has those)
        fzs.modifications_ptr->add(graphmod_add_edge, edge_ptr->get_id().key());
    }
    return true;
#else
    return standard_error("MISSING IMPLEMENTATION: addlist to dependencies", __func__);
#endif
}

const node_dependencies_edit_map_t node_dependencies_edit_map = {
    {"add", handle_node_dependencies_add},
    {"remove", handle_node_dependencies_remove},
    {"addlist", handle_node_dependencies_addlist}
    // *** Could add "edit" here for parameter edits...
};

/**
 * E.g. /fz/graph/nodes/20200901061505.1/dependencies/add?20090309102906.1=
 *      /fz/graph/nodes/20200901061505.1/dependencies/remove?20090309102906.1=
 *      /fz/graph/nodes/20200901061505.1/dependencies/addlist?dependencies=
 * @param node The Node to edit.
 * @param editstr String containing an edit command and Dependencies Node ID.
 */
bool handle_node_dependencies_edit(Node & node, std::string editstr) {
    if (editstr.empty()) {
        return standard_error("Missing edit string", __func__);
    }
    auto parpos = editstr.find('?');
    if (parpos == std::string::npos) {
        return standard_error("Missing dependencies parameters", __func__);
    }
    auto it = node_dependencies_edit_map.find(editstr.substr(0, parpos));
    if (it == node_dependencies_edit_map.end()) {
        return standard_error("Unsupported Node context request: '" + editstr.substr(0,parpos) + '\'', __func__);
    }
    parpos++;
    auto equalpos = editstr.find('=', parpos);
    if (equalpos == std::string::npos) equalpos = editstr.size();
    if (!it->second(node, editstr.substr(parpos, equalpos - parpos))) {
        return false;
    }
    return true;
}

const node_context_edit_map_t node_context_edit_map = {
    {"topics", handle_node_topics_edit},
    {"superiors", handle_node_superiors_edit},
    {"dependencies", handle_node_dependencies_edit}
};

/**
 * Address a specified parameter, then carry out a command, such as `set` or `add`, or show the parameter
 * in the format indicated by the extension. Where parameters have units, multiple units and unit conversion
 * may be available as well.
 * Also, `add` or `remove` Topics when `topics/` is the next part of the URL.
 * For example:
 *   /fz/graph/nodes/20200901061505.1/completion?set=1.0
 *   /fz/graph/nodes/20200901061505.1/completion?add=0.5
 *   /fz/graph/nodes/20200901061505.1/required?add=45m
 *   /fz/graph/nodes/20200901061505.1/required?add=-45m
 *   /fz/graph/nodes/20200901061505.1/required?add=0.75h
 *   /fz/graph/nodes/20200901061505.1/required?set=2h
 *   /fz/graph/nodes/20200901061505.1/valuation.raw
 *   /fz/graph/nodes/20200901061505.1/completion.txt
 *   /fz/graph/nodes/20200901061505.1/required.html
 *   /fz/graph/nodes/20200901061505.1/targetdate.json
 *   /fz/graph/nodes/20200901061505.1/effectivetd.json
 *   /fz/graph/nodes/20200901061505.1/text.json
 *   /fz/graph/nodes/20200901061505.1/tdproperty.json
 *   /fz/graph/nodes/20200901061505.1/repeats.json
 *   /fz/graph/nodes/20200901061505.1/tdpattern.json
 *   /fz/graph/nodes/20200901061505.1/tdevery.json
 *   /fz/graph/nodes/20200901061505.1/tdspan.json
 *   /fz/graph/nodes/20200901061505.1/topics.json
 *   /fz/graph/nodes/20200901061505.1/in_NNLs.json
 *   /fz/graph/nodes/20200901061505.1/topics/add?organization=1.0&oop-change=1.0
 *   /fz/graph/nodes/20200901061505.1/topics/remove?literature=[1.0]
 *   /fz/graph/nodes/20091115180507.1/superiors/remove?20090309102906.1=
 *   /fz/graph/nodes/20091115180507.1/dependencies/remove?20120716181425.1=
 *   /fz/graph/nodes/20091115180507.1/dependencies/add?20120716181425.1=
 *   /fz/graph/nodes/20091115180507.1/dependencies/addlist?dependencies=
 * or
 *   /fz/graph/nodes/20091115180507.1/dependencies?add=20120716181425.1
 *   /fz/graph/nodes/20091115180507.1/dependencies?addlist=dependencies
 */
bool handle_node_direct_parameter(Node & node, std::string extension, std::string & response_html) {
    ERRTRACE;

#ifdef TEST_MORE_THAN_NODE_MODIFICATIONS
    fzs.modifications_ptr = std::make_unique<Graphmod_unshared_results>();
#endif

    // skip '/'
    extension.erase(0,1);
    To_Debug_LogFile("Working with request extension "+extension);
    // identify the parameter
    auto seppos = extension.find_first_of("?./");
    if (seppos == std::string::npos) {
        return false;
    }
    // identify the command
    Edit_flags editflags;
    std::string route_extension = extension.substr(0,seppos); //  E.g. completion, required, valuation, etc.
    if (!editflags.set_Edit_flag_by_label(route_extension)) {
        return standard_error("Unrecognized Node parameter: '" + route_extension + '\'', __func__); // extension.substr(0,seppos) + '\'', __func__);
    }
    // *** Eventually, you probably want to make this more like the serial data version,
    // *** where Node_data knows how to parse string values and set itself, and then
    // *** Edit_flags are used to either set a specific Node parameter or add to it.
    switch (extension[seppos]) {
        case '?': {
            if (extension.substr(seppos+1,4) == "set=") { // completion?set=val or required?set=val
                auto it = node_parameter_edit_set_map.find(editflags.get_Edit_flags());
                if (it == node_parameter_edit_set_map.end()) {
                    return standard_error("Unsupported Node parameter set request: '" + extension.substr(0,seppos) + '\'', __func__);
                }
                if (!it->second(node, extension.substr(seppos+5))) {
                    return false;
                }
            } else {
                if (extension.substr(seppos+1,4) == "add=") { // completion?add=val or required?add=val
                    auto it = node_parameter_edit_add_map.find(editflags.get_Edit_flags());
                    if (it == node_parameter_edit_add_map.end()) {
                        return standard_error("Unsupported Node parameter add request: '" + extension.substr(0,seppos) + '\'', __func__);
                    }
                    if (!it->second(node, extension.substr(seppos+5))) {
                        return false;
                    }
                } else {
                    return standard_error("Unrecognized modification request: '" + extension.substr(seppos+1,3) + '\'', __func__);
                }
            }
            break;
        }
        case '.': { // <node-id>.html/txt/node/desc, targetdate.txt, etc
            To_Debug_LogFile("Started processing Node info request for node "+node.get_id_str()+" with route extension "+route_extension);
            auto it = node_parameter_show_map.find(route_extension);
            if (it == node_parameter_show_map.end()) {
                return standard_error("Unsupported Node parameter show request: '" + route_extension + '\'', __func__);
            }
            if (!it->second(node, extension.substr(seppos+1), response_html)) {
                return false;
            }
            return true; // No DB update, and response_html was already prepared.
            break;
        }
        case '/': { // topics, superiors, dependencies
            auto it = node_context_edit_map.find(extension.substr(0, seppos));
            if (it == node_context_edit_map.end()) {
                return standard_error("Unsupported Node context request: '" + extension.substr(0,seppos) + '\'', __func__);
            }
            if (!it->second(node, extension.substr(seppos+1))) {
                return false;
            }
            break;
        }
        default: {
            // nothing to do here
        }
    }
#ifndef TEST_MORE_THAN_NODE_MODIFICATIONS
    // update in database
    if (!Update_Node_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), node, editflags)) {
        return standard_error("Synchronizing Node update to database failed", __func__);
    }
#endif

#ifdef TEST_MORE_THAN_NODE_MODIFICATIONS
    To_Debug_LogFile("About to take modifications to databse.");
    // Note: In this call, where editflags are needed they are obtained from the modified Node.
    if (!handle_Graph_modifications_unshared_pq(fzs.graph(), fzs.ga.dbname(), fzs.ga.pq_schemaname(), *fzs.modifications_ptr.get())) {
        return standard_error("Synchronizing Graph update to database failed", __func__);
    }
    To_Debug_LogFile("Returned normally after making modifications in databse.");
#endif

    // post-modification validity test
    if (editflags.Edit_error()) { // check this AFTER synchronizing (see note in Graphmodify.hpp:Edit_flags)
        return standard_error("An invalid circumstance was encountered while attempting to edit a parameter of Node "+node.get_id_str(), __func__);
    }
    To_Debug_LogFile("Passed Edit_flags::Edit_error() check.");

    // edit response_html
    response_html = standard_HTML_header("fz: Node Modify") + "<p>Node parameter modified.</p>\n</body>\n</html>\n";
    return true;
}

/**
 * Examples:
 *   /fz/graph/nodes/logtime?<T>
 *   /fz/graph/nodes/<node-id>.<html|desc>
 *   /fz/graph/nodes/<node-id>?<completion|required|skip>...
 *   /fz/graph/nodes/<node-id>/...
 */
bool handle_node_direct_request(std::string nodereqstr, std::string & response_html) {
    ERRTRACE;

    VERYVERBOSEOUT("Handling Node request.\n");

    if ((nodereqstr.substr(0,8) == "logtime?") && (nodereqstr.size()>25)) {
        response_html = standard_HTML_header("fz: Node Request") + "<p>Node Logged time updated.</p>\n</body>\n</html>\n";
        return node_add_logged_time(nodereqstr.substr(8));
    }

    if (nodereqstr.size()>NODE_ID_STR_NUMCHARS) {
        Node_ptr node_ptr = fzs.graph_ptr->Node_by_idstr(nodereqstr.substr(0,NODE_ID_STR_NUMCHARS));
        if (node_ptr) {
            switch (nodereqstr[NODE_ID_STR_NUMCHARS]) {
                case '.': {
                    To_Debug_LogFile("Received /fz/graph/nodes/<node-id>. request "+nodereqstr);
                    return handle_node_direct_show(*node_ptr, nodereqstr.substr(NODE_ID_STR_NUMCHARS), response_html);
                }
                case '?': {
                    To_Debug_LogFile("Received /fz/graph/nodes/<node-id>? modification request"+nodereqstr);
                    return handle_node_direct_edit_multiple_pars(*node_ptr, nodereqstr.substr(NODE_ID_STR_NUMCHARS), response_html);
                }
                case '/': {
                    return handle_node_direct_parameter(*node_ptr, nodereqstr.substr(NODE_ID_STR_NUMCHARS), response_html);
                }
            }
        }
    }

    return standard_error("No known request token found: "+nodereqstr, __func__);
}

/**
 * Turns a list of strings into a response in a specified format.
 * E.g. see how this is used in handle_named_list_direct_request().
 */
bool handle_list_of_strings(const std::string list_id, const std::vector<std::string>& strings_in_list, const std::string& ext, std::string& response_html) {
    if (ext == "json") {
        if (strings_in_list.empty()) {
            response_html += "{\""+list_id+"\":[]}";
            return true;
        }

        response_html += "{\""+list_id+"\":[";
        for (const auto& s : strings_in_list) {
            response_html += '"' + s + "\",";
        }
        response_html.back() = ']';
        response_html += "}";
        return true;
    } else if (ext == "raw") {
        response_html += list_id + '\n';
        for (const auto& s : strings_in_list) {
            response_html += s + '\n';
        }
        return true;
    }

    return standard_error("Unknown NNL content extension "+ext, __func__);
}

/** Examples:
 *    /fz/graph/namedlists/selected.<json|raw>
 *    /fz/graph/namedlists/threads?add=20230912080006.1
 *    /fz/graph/namedlists/threads?remove=20230912080006.1
 *    /fz/graph/namedlists/superiors?delete=
 *    /fz/graph/namedlists/recent?copy=superiors&to_max=5
 *    /fz/graph/namedlists/threads?move=3&up=
 *    /fz/graph/namedlists/promises?move=2&down=
 *    /fz/graph/namedlists/challenges_open?move=3&to=5
 *    /fz/graph/namedlists/_set?persistent=<true|false>
 *    /fz/graph/namedlists/_select?id=<node-id>
 *    /fz/graph/namedlists/_recent?id=<node-id>
 */
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
        } else { // handle NNL content requests (e.g. /fz/graph/namedlists/selected.json)

            size_t name_endpos = namedlistreqstr.find('.');
            if (name_endpos != std::string::npos) {

                if (name_endpos == 0) {
                    return standard_error("Request has zero-length Named Node List name or special request identifier.", __func__);
                }

                std::string list_name(namedlistreqstr.substr(0,name_endpos));
                std::string ext(namedlistreqstr.substr(name_endpos+1));

                Named_Node_List_ptr namedlist_ptr = fzs.graph_ptr->get_List(list_name);
                if (!namedlist_ptr) {
                    return standard_error("Named Node List "+list_name+" not found.", __func__);
                }
                std::vector<std::string> nodes_in_list;
                for (const auto & nkey: namedlist_ptr->list) {
                    nodes_in_list.emplace_back(nkey.str());
                }
                return handle_list_of_strings(list_name, nodes_in_list, ext, response_html);

            }

        }

        return standard_error("Unrecognized Named Node List special request identifier: "+namedlistreqstr, __func__);
    }

    if (name_endpos == 0) {
        return standard_error("Request has zero-length Named Node List name or special request identifier.", __func__);
    }

    // get argments of requests with arguments
    std::string list_name(namedlistreqstr.substr(0,name_endpos));
    ++name_endpos;
    auto token_value_vec = GET_token_values(namedlistreqstr.substr(name_endpos)); // get pairs such as [(add,<node-id>), (unique,true)]

    // handle underscore requests with arguments
    NNL_underscore_cmd underscore_cmd = static_cast<NNL_underscore_cmd>(find_in_command_map(list_name, NNL_underscore_commands));
    if (underscore_cmd != NNLuscrcmd_unknown) {
        switch (underscore_cmd) {

            case NNLuscrcmd_set: {
                return handle_named_list_parameters(token_value_vec, response_html);
            }

            case NNLuscrcmd_select: {
                return handle_selected_list(token_value_vec, response_html);
            }

            case NNLuscrcmd_recent: {
                return handle_recent_list(token_value_vec, response_html);
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

            case NNLlistcmd_move: {
                return handle_move_within_list(list_name, token_value_vec, response_html);
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
    std::string mode_html(standard_HTML_header("fz: DB Mode") + "<p>Database mode: "+SimPQ.PQChanges_Mode_str()+"</p>\n");
    if (SimPQ.LoggingPQChanges()) {
        mode_html += "<p>Logging to: "+SimPQ.simPQfile+"</p>\n";
    }
    mode_html += "</body>\n</html>\n";
    handle_request_response(new_socket, mode_html, "Database mode: "+SimPQ.PQChanges_Mode_str());
}

void show_db_log(int new_socket) {
    ERRTRACE;
    std::string log_html(standard_HTML_header("fz: Database Call Log"));
    log_html += "<p>When fzserverpq exits, the DB call log will be flushed to: "+SimPQ.simPQfile+"</p>\n\n";
    log_html += "<p>Current status of the DB call log:</p>\n<hr>\n<pre>\n" + SimPQ.GetLog() + "</pre>\n<hr>\n</body>\n</html>\n";
    handle_request_response(new_socket, log_html, "DB log request sucessful");
}

bool show_ReqQ(int new_socket) {
    ERRTRACE;
    std::string reqq_html(standard_HTML_header("fz: ReqQ"));
    reqq_html += "<p>When fzserverpq exits, ReqQ will be flushed to: "+fzs.ReqQ.get_errfilepath()+"</p>\n\n";
    reqq_html += "<p>Current status of ReqQ:</p>\n<hr>\n<pre>\n" + fzs.ReqQ.pretty_print() + "</pre>\n<hr>\n</body>\n</html>\n";
    return handle_request_response(new_socket, reqq_html, "ReqQ request sucessful");
}

bool show_ErrQ(int new_socket) {
    ERRTRACE;
    std::string errq_html(standard_HTML_header("fz: ErrQ"));
    errq_html += "<p>When fzserverpq exits, ErrQ will be flushed to: "+ErrQ.get_errfilepath()+"</p>\n\n";
    errq_html += "<p>Current status of ErrQ:</p>\n<hr>\n<pre>\n" + ErrQ.pretty_print() + "</pre>\n<hr>\n</body>\n</html>\n";
    return handle_request_response(new_socket, errq_html, "ErrQ request sucessful");
}

/**
 * Handle a database request in the Formalizer /fz/ virtual filesystem.
 * 
 * Examples:
 *   /fz/db/mode
 *   /fz/db/mode?set=<run|log|sim>
 *   /fz/db/log
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
 * Examples:
 *   /fz/graph/logtime?<T>
 *   /fz/graph/nodes/...
 *   /fz/graph/namedlists/...
 * 
 * @param new_socket The communication socket file handler to respond to.
 * @param fzrequesturl The URL-like string containing the request to handle.
 * @return True if the request was handled successfully.
 */
bool handle_fz_vfs_graph_request(int new_socket, const std::string & fzrequesturl) {
    VERYVERBOSEOUT("Handling Graph request.\n");

    if ((fzrequesturl.substr(10,8) == "logtime?") && (fzrequesturl.size()>35)) {

        std::string response_html;
        if (node_add_logged_time(fzrequesturl.substr(18))) {
            response_html = standard_HTML_header("fz: Add Logged Time") + "Logged time added to Node.\n</body>\n</html>\n";
            return handle_request_response(new_socket, response_html, "Logtime request successful");
        }

    }

    if (fzrequesturl.substr(10,6) == "nodes/") {
        To_Debug_LogFile("Received /fz/graph/nodes/ request "+fzrequesturl);
        std::string response_html;
        if (handle_node_direct_request(fzrequesturl.substr(16), response_html)) {
            return handle_request_response(new_socket, response_html, "Node request successful");
        }
    }

    if (fzrequesturl.substr(10,11) == "namedlists/") {
        std::string response_html;
        if (handle_named_list_direct_request(fzrequesturl.substr(21), response_html)) {
            return handle_request_response(new_socket, response_html, "NNL request successful");
        }
    }

    return false;
}

const Command_Token_Map general_noargs_commands = {
    {"status", fznoargcmd_status},
    {"ipport", fznoargcmd_ipport},
    {"tzadjust", fznoargcmd_tzadjust},
    {"ReqQ", fznoargcmd_reqq},
    {"ErrQ", fznoargcmd_errq},
    {"_stop", fznoargcmd_stop},
    {"verbosity?set=normal",fznoargcmd_verbosity_normal},
    {"verbosity?set=quiet",fznoargcmd_verbosity_quiet},
    {"verbosity?set=very",fznoargcmd_verbosity_very}
};

bool handle_status(int new_socket) {
    std::string status_html(standard_HTML_header("fz: Server Status") + "Server status: LISTENING\n</body>\n</html>\n");
    return handle_request_response(new_socket, status_html, "Status reported");
}

bool handle_ipport(int new_socket) {
    std::string ipport_html(standard_HTML_header("fz: Server Address") + "Server address: "+fzs.ipaddrstr+"\n</body>\n</html>\n");
    return handle_request_response(new_socket, ipport_html, "IPPort reported");
}

bool handle_tzadjust(int new_socket) {
    std::string tzadjust_html(standard_HTML_header("fz: TZ Offset") + "Server time zone offset seconds: "+std::to_string(fzs.config.graphconfig.tzadjust_seconds)+"\n</body>\n</html>\n");
    return handle_request_response(new_socket, tzadjust_html, "TZadjust reported");
}

bool handle_stop(int new_socket) {
    fzs.listen = false;
    std::string status_html(standard_HTML_header("fz: Server Stop") + "Server status: STOPPING\n</body>\n</html>\n");
    return handle_request_response(new_socket, status_html, "Stopping");
}

bool handle_set_verbosity(int new_socket, std::string verbosity_str, bool veryverbose, bool quiet) {
    standard.veryverbose = veryverbose;
    standard.quiet = quiet;
    std::string success_msg("Setting verbosity: "+verbosity_str);
    std::string status_html(standard_HTML_header("fz: Verbosity") + success_msg+"\n</body>\n</html>\n");
    return handle_request_response(new_socket, status_html, success_msg);
}

/**
 * Handle a request in the Formalizer /fz/ virtual filesystem.
 * 
 * Examples:
 *   /fz/status
 *   /fz/ipport
 *   /fz/tzadjust
 *   /fz/ReqQ
 *   /fz/ErrQ
 *   /fz/_stop
 *   /fz/verbosity?set=<normal|quiet|very>
 *   /fz/db/...
 *   /fz/graph/...
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

            case fznoargcmd_ipport: {
                return handle_ipport(new_socket);
            }

            case fznoargcmd_tzadjust: {
                return handle_tzadjust(new_socket);
            }

            case fznoargcmd_reqq: {
                return show_ReqQ(new_socket);
            }

            case fznoargcmd_errq: {
                return show_ErrQ(new_socket);
            }

            case fznoargcmd_stop: {
                return handle_stop(new_socket);
            }

            case fznoargcmd_verbosity_normal: {
                return handle_set_verbosity(new_socket, "normal", false, false);
            }

            case fznoargcmd_verbosity_quiet: {
                return handle_set_verbosity(new_socket, "quiet", false, true);
            }

            case fznoargcmd_verbosity_very: {
                return handle_set_verbosity(new_socket, "very verbose", true, false);
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
        To_Debug_LogFile("Received /fz/graph/ request"+fzrequesturl);
        return handle_fz_vfs_graph_request(new_socket, fzrequesturl);
    }

    return false;
}

/**
 * Redirects local Markdown file through md2html.
 */
bool handle_markdown(const std::string& mdfilepath, std::string& response_html) {
    response_html = shellcmd2str("md2html --github "+mdfilepath);
    return true;
}

std::string add_to_path(const std::string& path, const std::string& add) {
    if (path.empty()) {
        return add;
    }

    if (path.back() == '/') {
        return path + add;
    }

    return path + '/' + add;
}

std::string basename_from_path(const std::string& path) {
    auto slash_pos = path.rfind('/');
    if (slash_pos == std::string::npos) {
        return path;
    }
    return path.substr(slash_pos+1);
}

/**
 * Translate a url into a file path by doing the following:
 * 1. Compare the first part of the path (up to the first '/') with defined roots.
 * 2. If there is a match, then the file path is the defined root-path concatenated
 *    with the remainder of the url.
 * 3. If there is no match, then see if a default root has been defined, i.e. a
 *    root with an empty key, and if so, then the file path is the corresponding
 *    root-path concatenated with the entire url.
 * 4. If there is no '/' then also use the default root-path, if defined.
 * 5. Finally, in all other cases, return http_not_found.
 * E.g:
 *    '/doc/tex/Change/Change.html' -> ('doc') -> '/home/randalk/doc/tex/Change/Change.html'
 *    '/formalizer/earlywiz.html' -> ('') -> '/var/www/html/formalizer/earlywiz.html'
 *    '/index.html' -> ('') -> '/var/www/html/index.html'
 */
void direct_tcpport_api_file_serving(int new_socket, const std::string & url) {
    std::string file_path;
    auto rootkey_end = url.find('/', 1); // skip the initial '/'
    if (rootkey_end != std::string::npos) {
        std::string rootkey = url.substr(1, rootkey_end - 1); // e.g. 'doc', do not include the initial '/', nor the '/' you just found
        FZOUT("Searching for: "+rootkey+'\n');
        auto it = fzs.config.www_file_root.find(rootkey);
        if (it != fzs.config.www_file_root.end()) {
            file_path = it->second + url.substr(rootkey_end); // e.g. '/home/randalk/doc' + '/tex/Change/Change.html'
            FZOUT("FOUND! Making file_path: "+file_path+'\n');
        }
    }
    if (file_path.empty()) {
        auto it = fzs.config.www_file_root.find("");
        if (it != fzs.config.www_file_root.end()) {
            file_path = it->second + url;
        } else {
            handle_request_error(new_socket, http_not_found, "Requested file ("+url+") not found.");
            return;
        }
    }

    auto path_type = path_test(file_path);

    if (path_type == path_is_file) {

        // Test if it has the .md Markdown extension
        if (file_path.substr(file_path.length()-3) == ".md") {

            std::string response_html;
            if (handle_markdown(file_path, response_html)) {
                handle_request_response(new_socket, standard_HTML_header("fz: Markdown: "+basename_from_path(file_path)) + response_html + "</body>\n</html>\n", "Markdown converted");
                return;
            }

        } else {

            //uninitialized_buffer buf;
            std::vector<char> buf;
            if (file_to_buffer(file_path, buf)) {

                server_response_binary srvbin(file_path, buf.data(), buf.size());
                if (srvbin.respond(new_socket)>0) {
                    return;
                }

            }
        }

    } else if (path_type == path_is_directory) {

        auto [files, directories, symlinks] = get_directory_content(file_path);

        std::string response_html = standard_HTML_header("fz: Directory: "+file_path);

        for (auto& directory : directories) {
            std::string newurl(add_to_path(url, directory));
            response_html += "[dir] <a href=\""+newurl+"\" target=\"blank\">"+directory+"</a><br>";
        }

        for (auto& symlink : symlinks) {
            std::error_code ec;
            std::string targetpath = std::filesystem::read_symlink(symlink, ec).string();
            if (!targetpath.empty()) {
                response_html += "[symlink] <a href=\""+targetpath+"\" target=\"blank\">"+symlink+"</a><br>";
            }
        }

        for (auto& file : files) {
            std::string newurl(add_to_path(url, file));
            response_html += "<a href=\""+newurl+"\" target=\"blank\">"+file+"</a><br>";
        }

        handle_request_response(new_socket, response_html, "Directory contents returned");

        return;

    }

    handle_request_error(new_socket, http_not_found, "Requested file ("+url+") not found.");
}

/**
 * Treat the URL as a CGI call and act as a proxy for that call.
 * 1. Make TCP GET request on localhost.
 * 2. Return the result verbatim.
 */
void direct_tcpport_api_cgi_call(int new_socket, const std::string & url) {
    std::string response_str;
    To_Debug_LogFile("Received CGI call request: "+url);
    http_GET_long httpget("127.0.0.1", 80, url);
    if (!httpget.valid_response) {
        handle_request_error(new_socket, http_bad_request, "API request to Server www port failed: "+url);
        return;
    }
    To_Debug_LogFile("Received forwarded CGI call response:\n"+httpget.response_str);

    ssize_t send_result;
    if (httpget.html_content) {
        server_response_text srvhtml(httpget.response_str);
        send_result = srvhtml.respond(new_socket);
    } else {
        server_response_plaintext srvtxt(httpget.response_str);
        send_result = srvtxt.respond(new_socket);
    }

    if (send_result < 0) {
        handle_request_error(new_socket, http_bad_request, "Data response returned error code: "+std::to_string(send_result));
        return;
    }

    VERYVERBOSEOUT("Forwarded response of CGI call "+url+'\n');
}

/**
 * Examples:
 *   FZ NNLlen(superiors);NNLlen(dependencies)
 *   GET/PATCH /fz/graph/nodes/<node-id>/completion?set=<ratio>
 *   /doc/txt/system.txt
 *   GET /cgi-bin/fzuistate.py
 * 
 * Note:
 *   POST is not yet supported.
 */
void fzserverpq::handle_special_purpose_request(int new_socket, const std::string & request_str) {
    ERRTRACE;

    VERYVERBOSEOUT("Received Special Purpose request "+request_str+".\n");
    FZOUT("Received Special Purpose request "+request_str+".\n");
    log("TCP","Received: "+request_str);
    auto requestvec = split(request_str,' ');
    if (requestvec.size()<2) {
        handle_request_error(new_socket, http_bad_request, "Missing request.");
        return;
    }

    if (requestvec[0] == "FZ") {
        handle_serialized_data_request(new_socket, request_str);
        return;
    }

    // Note that we're not really bothering to distinguish GET and PATCH here.
    // Instead, we're just using CGI FORM GET-method URL encoding to identify modification requests.

    if (requestvec[1].substr(0,4) == "/fz/") { // (one type of) recognized Formalizer special purpose request

        To_Debug_LogFile("Received /fz/ request"+requestvec[1]);

        if (handle_fz_vfs_request(new_socket, requestvec[1])) {
            return;
        } else {
            handle_request_error(new_socket, http_not_found, "Formalizer Virtual Filesystem /fz/ request failed.");
            return;         
        }

    }

    if (requestvec[1].substr(0,9) == "/cgi-bin/") { // Be a proxy to a CGI call.
        direct_tcpport_api_cgi_call(new_socket, requestvec[1]);
        return;
    }

    if (requestvec[1][0] == '/') { // translate to direct TCP-port API file serving root
        direct_tcpport_api_file_serving(new_socket, requestvec[1]);
        return;
    }

    // no known request encountered and handled
    handle_request_error(new_socket, http_bad_request, "Request unrecognized: "+request_str);
}

#ifdef USE_MULTI_THREADING

/**
 * This version puts the request into a FIFO queue for handling.
 */
void queing_fzserverpq::handle_special_purpose_request(int new_socket, const std::string & request_str) {
    ERRTRACE;

    std::lock_guard<std::mutex> lock(queueMutex); // thread-safety for pushing to special_FIFO and shm_FIFO
    VERYVERBOSEOUT("Adding special request to queue.\n");
    FZOUT("Adding special request to queue.\n");
    special_FIFO.emplace(new_socket, request_str);
    if (special_FIFO.size() > 10) {
        ADDWARNING(__func__, "Special requests queue size > 10");
    }
}

#endif // USE_MULTI_THREADING
