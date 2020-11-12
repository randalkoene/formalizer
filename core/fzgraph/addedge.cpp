// Copyright 2020 Randal A. Koene
// License TBD

/**
 * The Add Edge request functions of the fzgraphedit tool.
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
#include "stringio.hpp"
#include "general.hpp"
#include "ReferenceTime.hpp"
#include "Graphbase.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "utf8.hpp"
*/
#include "Graphmodify.hpp"

// local
#include "fzgraph.hpp"
#include "addedge.hpp"


using namespace fz;

Edge * add_Edge_request(Graph_modifications & gm, const Node_ID_key & depkey, const Node_ID_key & supkey, Edge_data & ed) {
    ERRTRACE;

    Edge * edge_ptr = gm.request_add_Edge(depkey, supkey);
    if (!edge_ptr)
        standard_exit_error(exit_general_error, "Unable to create new Edge object in shared segment ("+graphmemman.get_active_name()+')', __func__);

    VERBOSEOUT("\nMaking Edge "+edge_ptr->get_id_str()+'\n');

    // Set up Edge parameters
    edge_ptr->set_dependency(ed.dependency);
    edge_ptr->set_significance(ed.significance);
    edge_ptr->set_importance(ed.importance);
    edge_ptr->set_urgency(ed.urgency);
    edge_ptr->set_priority(ed.priority);

    // *** let's find out how much space is consumed in shared memory when a Node is created, improve our estimate!

    return edge_ptr;
}

int make_edges() {
    ERRTRACE;

    if (fzge.config.superiors.size() != fzge.config.dependencies.size()) {
        standard_exit_error(exit_general_error, "The list of superiors and list of dependencies must be of equal size", __func__);
    }
    if (fzge.config.superiors.empty()) {
        standard_exit_error(exit_general_error, "At least one superior and dependency pair are needed", __func__);
    }

    // Determine probably memory space needed.
    // *** MORE HERE TO BETTER ESTIMATE THAT
    unsigned long segsize = fzge.config.superiors.size()*10240; // *** wild guess
    // Determine a unique segment name to share with `fzserverpq`
    std::string segname(unique_name_Graphmod());
    Graph_modifications * graphmod_ptr = allocate_Graph_modifications_in_shared_memory(segname, segsize);
    if (!graphmod_ptr)
        standard_exit_error(exit_general_error, "Unable to create shared segment for modifications requests (name="+segname+", size="+std::to_string(segsize)+')', __func__);

    for (size_t i = 0; i < fzge.config.superiors.size(); ++i) {
        if (!add_Edge_request(*graphmod_ptr, fzge.config.dependencies[i], fzge.config.superiors[i], fzge.config.ed)) {
            standard_exit_error(exit_general_error, "Unable to prepare Add-Edge request from "+fzge.config.dependencies[i].str()+" to "+fzge.config.superiors[i].str(), __func__);
        }
    }

    auto ret = server_request_with_shared_data(segname, fzge.config.port_number);
    standard.exit(ret);

    //return standard.completed_ok(); // *** could put standard_exit(ret==1, ...) here instead
}
