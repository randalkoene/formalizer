// Copyright 2020 Randal A. Koene
// License TBD

/**
 * The Add Node request functions of the fzgraphedit tool.
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
#include "addnode.hpp"


using namespace fz;

Node * add_Node_request(Graph_modifications & gm, Node_data & nd) {
    ERRTRACE;

    Node * node_ptr = gm.request_add_Node();
    if (!node_ptr)
        standard_exit_error(exit_general_error, "Unable to create new Node object in shared segment ("+graphmemman.get_active_name()+')', __func__);

    VERBOSEOUT("Making Node "+node_ptr->get_id_str()+'\n');

    // Set up Node parameters
    node_ptr->set_text(nd.utf8_text);
    node_ptr->set_required((unsigned int) nd.hours*3600);
    node_ptr->set_valuation(nd.valuation);
    node_ptr->set_targetdate(nd.targetdate);
    node_ptr->set_tdproperty(nd.tdproperty);
    node_ptr->set_tdpattern(nd.tdpattern);
    node_ptr->set_tdevery(nd.tdevery);
    node_ptr->set_tdspan(nd.tdspan);
    node_ptr->set_repeats((nd.tdpattern != patt_nonperiodic) && (nd.tdproperty != variable) && (nd.tdproperty != unspecified));

    // main topic
    // *** THIS REQUIRES A BIT OF THOUGHT

    // *** let's find out how much space is consumed in shared memory when a Node is created, improve our estimate!

    return node_ptr;
}
