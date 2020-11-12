// Copyright 2020 Randal A. Koene
// License TBD

/**
 * The Named Node List request functions of the fzgraph tool.
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
#include "tcpclient.hpp"

// local
#include "fzgraph.hpp"
#include "namednodelist.hpp"


using namespace fz;

int add_to_list() {
    ERRTRACE;
    if (fzge.config.listname.empty())
        standard_exit_error(exit_missing_data, "No Named Node List name specified", __func__);
    if (fzge.config.superiors.empty() && fzge.config.dependencies.empty())
        standard_exit_error(exit_missing_data, "No Node IDs specified", __func__);

    // Determine probable memory space needed.
    // *** MORE HERE TO BETTER ESTIMATE THAT
    unsigned long segsize = 1024+256*(fzge.config.superiors.size()+fzge.config.dependencies.size()); // *** wild guess, should be affected by number of Node IDs in -S and -D
    // Determine a unique segment name to share with `fzserverpq`
    std::string segname(unique_name_Graphmod());
    Graph_modifications * graphmod_ptr = allocate_Graph_modifications_in_shared_memory(segname, segsize);
    if (!graphmod_ptr)
        standard_exit_error(exit_general_error, "Unable to create shared segment for modifications requests (name="+segname+", size="+std::to_string(segsize)+')', __func__);

    for (const auto & supkey : fzge.config.superiors) {
        Named_Node_List_Element * listelement_ptr = graphmod_ptr->request_Named_Node_List_Element(namedlist_add, fzge.config.listname, supkey);
        if (!listelement_ptr)
            standard_exit_error(exit_general_error, "Unable to create new Named Node List Element in shared segment ("+graphmemman.get_active_name()+')', __func__);

        VERBOSEOUT("\nAdding Node "+supkey.str()+" to Named Node List "+fzge.config.listname+'\n');
    }

    for (const auto & depkey : fzge.config.dependencies) {
        Named_Node_List_Element * listelement_ptr = graphmod_ptr->request_Named_Node_List_Element(namedlist_add, fzge.config.listname, depkey);
        if (!listelement_ptr)
            standard_exit_error(exit_general_error, "Unable to create new Named Node List Element in shared segment ("+graphmemman.get_active_name()+')', __func__);

        VERBOSEOUT("\nAdding Node "+depkey.str()+" to Named Node List "+fzge.config.listname+'\n');
    }

    auto ret = server_request_with_shared_data(segname, fzge.config.port_number);
    standard.exit(ret);
}

int remove_from_list() {
    ERRTRACE;
    if (fzge.config.listname.empty())
        standard_exit_error(exit_missing_data, "No Named Node List name specified", __func__);
    if (fzge.config.superiors.empty() && fzge.config.dependencies.empty())
        standard_exit_error(exit_missing_data, "No Node IDs specified", __func__);

    // Determine probable memory space needed.
    // *** MORE HERE TO BETTER ESTIMATE THAT
    unsigned long segsize = 1024+256*(fzge.config.superiors.size()+fzge.config.dependencies.size()); // *** wild guess, should be affected by number of Node IDs in -S and -D
    // Determine a unique segment name to share with `fzserverpq`
    std::string segname(unique_name_Graphmod());
    Graph_modifications * graphmod_ptr = allocate_Graph_modifications_in_shared_memory(segname, segsize);
    if (!graphmod_ptr)
        standard_exit_error(exit_general_error, "Unable to create shared segment for modifications requests (name="+segname+", size="+std::to_string(segsize)+')', __func__);

    for (const auto & supkey : fzge.config.superiors) {
        Named_Node_List_Element * listelement_ptr = graphmod_ptr->request_Named_Node_List_Element(namedlist_remove, fzge.config.listname, supkey);
        if (!listelement_ptr)
            standard_exit_error(exit_general_error, "Unable to create new Named Node List Element in shared segment ("+graphmemman.get_active_name()+')', __func__);

        VERBOSEOUT("\nRemoving Node "+supkey.str()+" from Named Node List "+fzge.config.listname+'\n');
    }

    for (const auto & depkey : fzge.config.dependencies) {
        Named_Node_List_Element * listelement_ptr = graphmod_ptr->request_Named_Node_List_Element(namedlist_remove, fzge.config.listname, depkey);
        if (!listelement_ptr)
            standard_exit_error(exit_general_error, "Unable to create new Named Node List Element in shared segment ("+graphmemman.get_active_name()+')', __func__);

        VERBOSEOUT("\nRemoving Node "+depkey.str()+" from Named Node List "+fzge.config.listname+'\n');
    }

    auto ret = server_request_with_shared_data(segname, fzge.config.port_number);
    standard.exit(ret);
}

int delete_list() {
    ERRTRACE;
    if (fzge.config.listname.empty())
        standard_exit_error(exit_missing_data, "No Named Node List name specified", __func__);

    // Determine probable memory space needed.
    // *** MORE HERE TO BETTER ESTIMATE THAT
    unsigned long segsize = 2048; // *** wild guess, should be affected by number of Node IDs in -S and -D
    // Determine a unique segment name to share with `fzserverpq`
    std::string segname(unique_name_Graphmod());
    Graph_modifications * graphmod_ptr = allocate_Graph_modifications_in_shared_memory(segname, segsize);
    if (!graphmod_ptr)
        standard_exit_error(exit_general_error, "Unable to create shared segment for modifications requests (name="+segname+", size="+std::to_string(segsize)+')', __func__);

    Node_ID_key dummykey;
    Named_Node_List_Element * listelement_ptr = graphmod_ptr->request_Named_Node_List_Element(namedlist_delete, fzge.config.listname, dummykey);
    if (!listelement_ptr)
        standard_exit_error(exit_general_error, "Unable to create new Named Node List Element in shared segment ("+graphmemman.get_active_name()+')', __func__);

    VERBOSEOUT("\nDeleting Named Node List "+fzge.config.listname+'\n');

    auto ret = server_request_with_shared_data(segname, fzge.config.port_number);
    standard.exit(ret);
}
