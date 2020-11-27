// Copyright 2020 Randal A. Koene
// License TBD

/**
 * The Add Node request functions of the fzgraph tool.
 * 
 * {{ long_description }}
 * 
 * For more about this, see https://trello.com/c/FQximby2.
 */

// std


// core
/*
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "ReferenceTime.hpp"
#include "Graphbase.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "utf8.hpp"
*/
#include "stringio.hpp"
#include "Graphmodify.hpp"
#include "tcpclient.hpp"

// local
#include "fzgraph.hpp"
#include "addnode.hpp"
#include "addedge.hpp"


using namespace fz;

Node * add_Node_request(Graph_modifications & gm, Node_data & nd) {
    ERRTRACE;

    Node * node_ptr = gm.request_add_Node();
    if (!node_ptr)
        standard_exit_error(exit_general_error, "Unable to create new Node object in shared segment ("+graphmemman.get_active_name()+')', __func__);

    VERBOSEOUT("\nMaking Node "+node_ptr->get_id_str()+'\n');

    Graph * graph_ptr = fzge.get_Graph();
    if (!graph_ptr)
        standard_exit_error(exit_resident_graph_missing, "Unable to access the memory-resident Graph", __func__);

    // Set up Node parameters and main topic
    fzge.config.nd.copy(*graph_ptr, *node_ptr);

    Topic_ID mainid = node_ptr->main_topic_id();
    VERYVERBOSEOUT("  main Topic of new Node is: "+std::to_string(mainid)+" ("+graph_ptr->find_Topic_Tag_by_id(mainid)+")\n");

    // *** let's find out how much space is consumed in shared memory when a Node is created, improve our estimate!

    return node_ptr;
}

/**
 * Select the source for superiors and dependencies.
 * 
 * 1. The command line takes precedence.
 * 2. Named Node Lists 'superiors' and 'dependencies' are next in line.
 * 3. Anything specified in the configuration file is the default.
 */
void sup_dep_source() {
    if (fzge.supdep_from_cmdline) {
        VERYVERBOSEOUT("Using superiors and/or dependencies specified on the command line.\n");
        return;
    }

    Graph * graph_ptr = fzge.get_Graph();
    if (!graph_ptr) {
        standard_exit_error(exit_resident_graph_missing, "Unable to access the memory-resident Graph", __func__);
    }

    Named_Node_List_ptr suplist_ptr = graph_ptr->get_List("superiors");
    Named_Node_List_ptr deplist_ptr = graph_ptr->get_List("dependencies");
    if ((!suplist_ptr) && (!deplist_ptr)) {
        VERYVERBOSEOUT("Using superiors and/or dependencies configured defaults.\n");
        return;
    }

    fzge.config.superiors.clear();
    fzge.config.dependencies.clear();
    if (suplist_ptr) {
        for (const auto & nkey : suplist_ptr->list) {
            fzge.config.superiors.emplace_back(nkey);
        }
    }
    if (deplist_ptr) {
        for (const auto & nkey : deplist_ptr->list) {
            fzge.config.dependencies.emplace_back(nkey);
        }
    }
    VERYVERBOSEOUT("Using superiors and/or dependencies from Named Node Lists.\n");
}

void sup_dep_postop(exit_status_code ret) {
    if (!fzge.nnl_supdep_used) {
        return;
    }
    if (fzge.config.supdep_after_use == nnl_keep) {
        return;
    }
    if (ret != exit_ok) {
        VERBOSEOUT("The superiors and dependencies Named Node Lists have been retained for potential retry.\n");
        return;
    }
    if (fzge.config.supdep_after_use == nnl_ask) {
        if (!default_choice("Delete or keep superiors and dependencies Named Node Lists used (D/k)? ",'k')) {
            VERBOSEOUT("Retaining superiors and dependencies Named Node Lists.\n");
            return;
        }
    }
    Graph * graph_ptr = fzge.get_Graph();
    if (!graph_ptr) {
        standard_exit_error(exit_resident_graph_missing, "Unable to access the memory-resident Graph", __func__);
    }
    graph_ptr->delete_List("superiors");
    graph_ptr->delete_List("dependencies");
    VERBOSEOUT("The superiors and dependencies Named Node Lists have been deleted after use.\n");
}

int make_node() {
    ERRTRACE;
    auto [exit_code, errstr] = get_content(fzge.config.nd.utf8_text, fzge.config.content_file, "Node description");
    if (exit_code != exit_ok)
        standard_exit_error(exit_code, errstr, __func__);

    // Determine probable memory space needed.
    // *** MORE HERE TO BETTER ESTIMATE THAT
    unsigned long segsize = sizeof(fzge.config.nd.utf8_text)+fzge.config.nd.utf8_text.capacity() + 10240; // *** wild guess
    // Determine a unique segment name to share with `fzserverpq`
    std::string segname(unique_name_Graphmod());
    Graph_modifications * graphmod_ptr = allocate_Graph_modifications_in_shared_memory(segname, segsize);
    if (!graphmod_ptr)
        standard_exit_error(exit_general_error, "Unable to create shared segment for modifications requests (name="+segname+", size="+std::to_string(segsize)+')', __func__);

    Node * node_ptr = nullptr;
    if ((node_ptr = add_Node_request(*graphmod_ptr, fzge.config.nd)) == nullptr) {
        standard_exit_error(exit_general_error, "Unable to prepare Add-Node request", __func__);
    }

    sup_dep_source();

    for (const auto & supkey : fzge.config.superiors) {
        if (!add_Edge_request(*graphmod_ptr, node_ptr->get_id().key(), supkey, fzge.config.ed)) {
            standard_exit_error(exit_general_error, "Unable to prepare Add-Edge request to superior", __func__);
        }
    }

    for (const auto & depkey : fzge.config.dependencies) {
        if (!add_Edge_request(*graphmod_ptr, depkey, node_ptr->get_id().key(), fzge.config.ed)) {
            standard_exit_error(exit_general_error, "Unable to prepare Add-Edge request from dependency", __func__);
        }
    }

    auto ret = server_request_with_shared_data(segname, fzge.config.port_number);
    sup_dep_postop(ret);
    standard.exit(ret);

    //return standard.completed_ok(); // *** could put standard_exit(ret==1, ...) here instead
}
