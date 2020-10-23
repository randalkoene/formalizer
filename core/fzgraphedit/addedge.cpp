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
#include "fzgraphedit.hpp"
#include "addedge.hpp"


using namespace fz;

Edge * add_Edge_request(Graph_modifications & gm, const Node_ID_key & depkey, const Node_ID_key & supkey, Edge_data & ed) {
    ERRTRACE;

    Edge * edge_ptr = gm.request_add_Edge(depkey, supkey);
    if (!edge_ptr)
        standard_exit_error(exit_general_error, "Unable to create new Edge object in shared segment ("+graphmemman.get_active_name()+')', __func__);

    VERBOSEOUT("Making Edge "+edge_ptr->get_id_str()+'\n');

    // Set up Edge parameters
    edge_ptr->set_dependency(ed.dependency);
    edge_ptr->set_significance(ed.significance);
    edge_ptr->set_importance(ed.importance);
    edge_ptr->set_urgency(ed.urgency);
    edge_ptr->set_priority(ed.priority);

    // *** let's find out how much space is consumed in shared memory when a Node is created, improve our estimate!

    return edge_ptr;
}
