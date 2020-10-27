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

    VERBOSEOUT("\nMaking Node "+node_ptr->get_id_str()+'\n');

    // Set up Node parameters
    node_ptr->set_text(nd.utf8_text);
    node_ptr->set_required((unsigned int) (nd.hours*3600.0));
    node_ptr->set_valuation(nd.valuation);
    node_ptr->set_targetdate(nd.targetdate);
    node_ptr->set_tdproperty(nd.tdproperty);
    node_ptr->set_tdpattern(nd.tdpattern);
    node_ptr->set_tdevery(nd.tdevery);
    node_ptr->set_tdspan(nd.tdspan);
    node_ptr->set_repeats((nd.tdpattern != patt_nonperiodic) && (nd.tdproperty != variable) && (nd.tdproperty != unspecified));

    // main topic
    Graph * graph_ptr = fzge.get_Graph();
    if (!graph_ptr)
        standard_exit_error(exit_resident_graph_missing, "Unable to access the memory-resident Graph", __func__);

    for (const auto & tag_str : nd.topics) {
        VERYVERBOSEOUT("\n  adding Topic tag: "+tag_str+'\n');
        Topic * topic_ptr = graph_ptr->find_Topic_by_tag(tag_str);
        if (!topic_ptr) {
            standard_exit_error(exit_general_error, "Unknown Topic: "+tag_str, __func__);
        }
        Topic_Tags & topictags = *(const_cast<Topic_Tags *>(&graph_ptr->get_topics())); // We need the list of Topics from the memory-resident Graph.
        node_ptr->add_topic(topictags, topic_ptr->get_id(), 1.0);
    }
    Topic_ID mainid = node_ptr->main_topic_id();
    VERYVERBOSEOUT("  main Topic of new Node is: "+std::to_string(mainid)+" ("+graph_ptr->find_Topic_Tag_by_id(mainid)+")\n");

    // *** let's find out how much space is consumed in shared memory when a Node is created, improve our estimate!

    return node_ptr;
}
