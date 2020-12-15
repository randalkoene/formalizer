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
#include <filesystem>

// core
#include "error.hpp"
#include "standard.hpp"
//#include "general.hpp"
//#include "stringio.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "Graphpostgres.hpp"
//#include "stringio.hpp"
#include "binaryio.hpp"

// local
#include "tcp_server_handlers.hpp"
#include "fzserverpq.hpp"

using namespace fz;

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
    response_html = "<html>\n<head>" STANDARD_HTML_HEAD_LINKS "</head>\n<body>\n"
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
    response_html = "<html>\n<head>" STANDARD_HTML_HEAD_LINKS "</head>\n<body>\n"
                    "<p>Named Node List 'shortlist' updated with "+std::to_string(copied)+" Nodes.</p>\n"
                    "</body>\n</html>\n";

    return true;
}

bool handle_named_list_parameters(const GET_token_value_vec & token_value_vec, std::string & response_html) {
    ERRTRACE;

    response_html = "<html>\n<head>" STANDARD_HTML_HEAD_LINKS "</head>\n<body>\n"
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
    response_html = "<html>\n<head>" STANDARD_HTML_HEAD_LINKS "</head>\n<body>\n"
                    "<p>Named Node List modified.</p>\n"
                    "<p><b>Added</b> "+nkey.str()+" to List 'selected'.</p>\n"
                    "</body>\n</html>\n";

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
    response_html = "<html>\n<head>" STANDARD_HTML_HEAD_LINKS "</head>\n<body>\n"
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

    response_html = "<html>\n<head>" STANDARD_HTML_HEAD_LINKS "</head>\n<body>\n"
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

const add_data_map_t NNL_list_add_features = {
    {"add", add_data_nkey},
    {"maxsize", add_data_maxsize},
    {"unique", add_data_unique},
    {"fifo", add_data_fifo},
    {"prepend", add_data_prepend}
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

    response_html = "<html>\n<head>" STANDARD_HTML_HEAD_LINKS "</head>\n<body>\n"
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
            return standard_error("Unexpected token: '"+GETel.token+'\'', __func__);
        }
    }

    if (!fzs.graph_ptr->remove_from_List(list_name, nkey)) {
        return standard_error("Unable to remove Node "+nkey.str()+" from Named Node List "+list_name, __func__);
    }

    response_html = "<html>\n<head>" STANDARD_HTML_HEAD_LINKS "</head>\n<body>\n"
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

    response_html = "<html>\n<head>" STANDARD_HTML_HEAD_LINKS "</head>\n<body>\n"
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
    {"_select", NNLuscrcmd_select},
    {"_recent", NNLuscrcmd_recent}
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

    auto token_value_vec = GET_token_values(node_addstr);
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
        
    Edit_flags editflags = Node_apply_minutes(*node_ptr, add_minutes, T_ref);
    
    if (!Update_Node_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), *node_ptr, editflags)) {
        return standard_error("Synchronizing Node update to database failed", __func__);
    }

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
    // *** Not yet implemented
    return false;
}

/**
 * Somewhat similar to the shared-memory interface to editing multiple parameters of a Node.
 * This includes changing the values of multiple parameters and setting the Node's `Edit_flags`,
 * then calling `Update_Node_pq()`, but it does not include using a `Graph_modifications` stack
 * or responding with results in shared-memory. Edits can set or add.
 * For example:
 *   /fz/graph/nodes/20200901061505.1?completion=1.0&repeats=no
 *   /fz/graph/nodes/20200901061505.1?required=+45m
 */
bool handle_node_direct_edit_multiple_pars(Node & node, const std::string & extension, std::string & response_html) {
    // *** Not yet implemented
    return false;
}

/**
 * Address a specified parameter, then carry out a command, such as `set` or `add`, or show the parameter
 * in the format indicated by the extension. Where parameters have units, multiple units and unit conversion
 * may be available as well.
 * Also, `add` or `remove` Topics when `topics/` is the next part of the URL.
 * For example:
 *   /fz/graph/nodes/20200901061505.1/completion?set=1.0
 *   /fz/graph/nodes/20200901061505.1/required?add=45m
 *   /fz/graph/nodes/20200901061505.1/required?add=-45m
 *   /fz/graph/nodes/20200901061505.1/required?add=0.75h
 *   /fz/graph/nodes/20200901061505.1/required?set=2h
 *   /fz/graph/nodes/20200901061505.1/valuation.txt
 *   /fz/graph/nodes/20200901061505.1/topics/add?organization=1.0&oop-change=1.0
 *   /fz/graph/nodes/20200901061505.1/topics/remove?literature=[1.0]
 */
bool handle_node_direct_parameter(Node & node, const std::string & extension, std::string & response_html) {
    // *** Not yet implemented
    // identify the parameter
    // identify the command
    // identify the value
    // set the new parameter value (alternatively, build a modification neuron and use a method similar to the SHM method)
    // set edit flags
    // update in database
    // edit response_html
    return false;
}

bool handle_node_direct_request(std::string nodereqstr, std::string & response_html) {
    ERRTRACE;

    VERYVERBOSEOUT("Handling Node request.\n");

    if ((nodereqstr.substr(0,8) == "logtime?") && (nodereqstr.size()>25)) {
        response_html = "<html>\n<head>" STANDARD_HTML_HEAD_LINKS "</head>\n<body>\n<p>Node Logged time updated.</p>\n</body>\n</html>\n";
        return node_add_logged_time(nodereqstr.substr(8));
    }

    if (nodereqstr.size()>NODE_ID_STR_NUMCHARS) {
        Node_ptr node_ptr = fzs.graph_ptr->Node_by_idstr(nodereqstr.substr(0,NODE_ID_STR_NUMCHARS));
        if (node_ptr) {
            switch (nodereqstr[NODE_ID_STR_NUMCHARS]) {
                case '.': {
                    return handle_node_direct_show(*node_ptr, nodereqstr.substr(NODE_ID_STR_NUMCHARS), response_html);
                }
                case '?': {
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

            default: {
                // nothing to do here
            }
        }
    }

    return standard_error("No known request token found: "+namedlistreqstr, __func__);
}

void show_db_mode(int new_socket) {
    ERRTRACE;
    std::string mode_html("<html>\n<head>" STANDARD_HTML_HEAD_LINKS "</head>\n<body>\n<p>Database mode: "+SimPQ.PQChanges_Mode_str()+"</p>\n");
    if (SimPQ.LoggingPQChanges()) {
        mode_html += "<p>Logging to: "+SimPQ.simPQfile+"</p>\n";
    }
    mode_html += "</body>\n</html>\n";
    handle_request_response(new_socket, mode_html, "Database mode: "+SimPQ.PQChanges_Mode_str());
}

void show_db_log(int new_socket) {
    ERRTRACE;
    std::string log_html("<html>\n<head>\n<link rel=\"stylesheet\" href=\"http://"+fzs.graph_ptr->get_server_IPaddr()+"/fz.css\">\n<title>fz: Database Call Log</title>\n</head>\n<body>\n<h3>fz: Database Call Log</h3>\n");
    log_html += "<p>When fzserverpq exits, the DB call log will be flushed to: "+SimPQ.simPQfile+"</p>\n\n";
    log_html += "<p>Current status of the DB call log:</p>\n<hr>\n<pre>\n" + SimPQ.GetLog() + "</pre>\n<hr>\n</body>\n</html>\n";
    handle_request_response(new_socket, log_html, "DB log request sucessful");
}

bool show_ReqQ(int new_socket) {
    ERRTRACE;
    std::string reqq_html("<html>\n<head>\n<link rel=\"stylesheet\" href=\"http://"+fzs.graph_ptr->get_server_IPaddr()+"/fz.css\">\n<title>fz: ReqQ</title>\n</head>\n<body>\n<h3>fz: ReqQ</h3>\n");
    reqq_html += "<p>When fzserverpq exits, ReqQ will be flushed to: "+fzs.ReqQ.get_errfilepath()+"</p>\n\n";
    reqq_html += "<p>Current status of ReqQ:</p>\n<hr>\n<pre>\n" + fzs.ReqQ.pretty_print() + "</pre>\n<hr>\n</body>\n</html>\n";
    return handle_request_response(new_socket, reqq_html, "ReqQ request sucessful");
}

bool show_ErrQ(int new_socket) {
    ERRTRACE;
    std::string errq_html("<html>\n<head>\n<link rel=\"stylesheet\" href=\"http://"+fzs.graph_ptr->get_server_IPaddr()+"/fz.css\">\n<title>fz: ErrQ</title>\n</head>\n<body>\n<h3>fz: ErrQ</h3>\n");
    errq_html += "<p>When fzserverpq exits, ErrQ will be flushed to: "+ErrQ.get_errfilepath()+"</p>\n\n";
    errq_html += "<p>Current status of ErrQ:</p>\n<hr>\n<pre>\n" + ErrQ.pretty_print() + "</pre>\n<hr>\n</body>\n</html>\n";
    return handle_request_response(new_socket, errq_html, "ErrQ request sucessful");
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

    if ((fzrequesturl.substr(10,8) == "logtime?") && (fzrequesturl.size()>35)) {

        std::string response_html;
        if (node_add_logged_time(fzrequesturl.substr(18))) {
            response_html = "<html>\n<head>" STANDARD_HTML_HEAD_LINKS "</head>\n<body>\nLogged time added to Node.\n</body>\n</html>\n";
            return handle_request_response(new_socket, response_html, "Logtime request successful");
        }

    }

    if (fzrequesturl.substr(10,6) == "nodes/") {
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
    {"ReqQ", fznoargcmd_reqq},
    {"ErrQ", fznoargcmd_errq},
    {"_stop", fznoargcmd_stop},
    {"verbosity?set=normal",fznoargcmd_verbosity_normal},
    {"verbosity?set=quiet",fznoargcmd_verbosity_quiet},
    {"verbosity?set=very",fznoargcmd_verbosity_very}
};

bool handle_status(int new_socket) {
    std::string status_html("<html>\n<head>" STANDARD_HTML_HEAD_LINKS "</head>\n<body>\nServer status: LISTENING\n</body>\n</html>\n");
    return handle_request_response(new_socket, status_html, "Status reported");
}

bool handle_stop(int new_socket) {
    fzs.listen = false;
    std::string status_html("<html>\n<head>" STANDARD_HTML_HEAD_LINKS "</head>\n<body>\nServer status: STOPPING\n</body>\n</html>\n");
    return handle_request_response(new_socket, status_html, "Stopping");
}

bool handle_set_verbosity(int new_socket, std::string verbosity_str, bool veryverbose, bool quiet) {
    standard.veryverbose = veryverbose;
    standard.quiet = quiet;
    std::string success_msg("Setting verbosity: "+verbosity_str);
    std::string status_html("<html>\n<head>" STANDARD_HTML_HEAD_LINKS "</head>\n<body>\n"+success_msg+"\n</body>\n</html>\n");
    return handle_request_response(new_socket, status_html, success_msg);
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
        return handle_fz_vfs_graph_request(new_socket, fzrequesturl);
    }

    return false;
}

void direct_tcpport_api_file_serving(int new_socket, const std::string & url) {
    std::string file_path(fzs.config.www_file_root+url);

    //uninitialized_buffer buf;
    std::vector<char> buf;
    if (file_to_buffer(file_path, buf)) {

        server_response_binary srvbin(file_path, buf.data(), buf.size());
        if (srvbin.respond(new_socket)>0) {
            return;
        }

    }

    handle_request_error(new_socket, http_not_found, "Requested file ("+url+") not found.");
}

void fzserverpq::handle_special_purpose_request(int new_socket, const std::string & request_str) {
    ERRTRACE;

    VERYVERBOSEOUT("Received Special Purpose request "+request_str+".\n");
    log("TCP","Received: "+request_str);
    auto requestvec = split(request_str,' ');
    if (requestvec.size()<2) {
        handle_request_error(new_socket, http_bad_request, "Missing request.");
        return;
    }

    // Note that we're not really bothering to distinguish GET and PATCH here.
    // Instead, we're just using CGI FORM GET-method URL encoding to identify modification requests.

    if (requestvec[1].substr(0,4) == "/fz/") { // (one type of) recognized Formalizer special purpose request

        if (handle_fz_vfs_request(new_socket, requestvec[1])) {
            return;
        } else {
            handle_request_error(new_socket, http_not_found, "Formalizer Virtual Filesystem /fz/ request failed.");
            return;         
        }

    }

    if (requestvec[1][0] == '/') { // translate to direct TCP-port API file serving root
        direct_tcpport_api_file_serving(new_socket, requestvec[1]);
        return;
    }

    // no known request encountered and handled
    handle_request_error(new_socket, http_bad_request, "Request unrecognized: "+request_str);
}
