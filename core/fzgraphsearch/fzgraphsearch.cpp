// Copyright 20210224 Randal A. Koene
// License TBD

/**
 * {{ brief_description }}
 * 
 * {{ long_description }}
 * 
 * For more about this, see {{ doc_reference }}.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Graph:Filter:Search"

// std
#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"
#include "Graphinfo.hpp"
#include "Graphmodify.hpp"
#include "tcpclient.hpp"

// local
#include "version.hpp"
#include "fzgraphsearch.hpp"



using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzgraphsearch fzgs;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzgraphsearch::fzgraphsearch() : formalizer_standard_program(false), config(*this) { //ga(*this, add_option_args, add_usage_top)
    add_option_args += "s:l:";
    add_usage_top += " [-s <search-string>] -l <list-name>";
    //usage_head.push_back("Description at the head of usage information.\n");
    //usage_tail.push_back("Extra usage information.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzgraphsearch::usage_hook() {
    //ga.usage_hook();
    FZOUT("    -s Description contains <search-string>.\n"
          "    -l Named Node List to receive the search results.\n");
}

/**
 * Handler for command line options that are defined in the derived class
 * as options specific to the program.
 * 
 * Include case statements for each option. Typical handlers do things such
 * as collecting parameter values from `cargs` or setting `flowcontrol` choices.
 * 
 * @param c is the character that identifies a specific option.
 * @param cargs is the optional parameter value provided for the option.
 */
bool fzgraphsearch::options_hook(char c, std::string cargs) {
    //if (ga.options_hook(c,cargs))
    //        return true;

    switch (c) {

    case 's': {
        search_string = cargs;
        return true;
    }

    case 'l': {
        listname = cargs;
        flowcontrol = flow_find_nodes;
        return true;
    }

    }

    return false;
}


/// Configure configurable parameters.
bool fzgs_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    //CONFIG_TEST_AND_SET_PAR(example_par, "examplepar", parlabel, parvalue);
    //CONFIG_TEST_AND_SET_FLAG(example_flagenablefunc, example_flagdisablefunc, "exampleflag", parlabel, parvalue);
    CONFIG_PAR_NOT_FOUND(parlabel);
}


/**
 * Initialize configuration parameters.
 * Call this at the top of main().
 * 
 * @param argc command line parameters count forwarded from main().
 * @param argv command line parameters array forwarded from main().
 */
void fzgraphsearch::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

Graph & fzgraphsearch::graph() {
    ERRTRACE;
    if (!graphmemman.get_Graph(graph_ptr)) {
        standard_exit_error(exit_resident_graph_missing, "Memory resident Graph not found.", __func__);
    }
    return *graph_ptr;
}

int find_nodes() {
    ERRTRACE;
    if (fzgs.listname.empty()) {
        standard_exit_error(exit_missing_data, "No Named Node List name specified", __func__);
    }

    // set up filter
    Node_Filter nodefilter;
    nodefilter.lowerbound.utf8_text = fzgs.search_string;
    nodefilter.filtermask.set_Edit_text();
    //nodefilter.lowerbound.completion = 0.0;
    //nodefilter.upperbound.completion = 0.99999;
    //nodefilter.lowerbound.tdproperty = variable;
    //nodefilter.upperbound.tdproperty = variable;
    //nodefilter.lowerbound.targetdate = t;
    //nodefilter.upperbound.targetdate = t;
    //nodefilter.filtermask.set_Edit_completion();
    //nodefilter.filtermask.set_Edit_tdproperty();
    //nodefilter.filtermask.set_Edit_targetdate();

    // find subset of Nodes
    targetdate_sorted_Nodes matched_nodes = Nodes_subset(fzgs.graph(), nodefilter);

    size_t total_found = matched_nodes.size();
    if (total_found < 1) {
        VERBOSEOUT("0 matching Nodes found.\n");
        return standard_exit_success(""); // We need verbose output, not very verbose.
    }

    VERBOSEOUT("\nAdding "+std::to_string(total_found)+" matching Nodes to List "+fzgs.listname+".\n");
    unsigned long segsize = 1024+(sizeof(Named_Node_List_Element)+sizeof(Graphmod_result))*2*total_found;
    // Determine a unique segment name to share with `fzserverpq`
    std::string segname(unique_name_Graphmod());
    Graph_modifications * graphmod_ptr = allocate_Graph_modifications_in_shared_memory(segname, segsize);
    if (!graphmod_ptr)
        standard_exit_error(exit_general_error, "Unable to create shared segment for modifications requests (name="+segname+", size="+std::to_string(segsize)+')', __func__);

    VERYVERBOSEOUT(graphmemman.info_str());
    for (const auto & [t_match, match_ptr] : matched_nodes) {
        Named_Node_List_Element * listelement_ptr = graphmod_ptr->request_Named_Node_List_Element(namedlist_add, fzgs.listname, match_ptr->get_id().key());
        if (!listelement_ptr)
            standard_exit_error(exit_general_error, "Unable to create new Named Node List Element in shared segment ("+graphmemman.get_active_name()+')', __func__);

        VERYVERBOSEOUT("\nAdding Node "+match_ptr->get_id_str()+" to Named Node List "+fzgs.listname);
    }
    VERYVERBOSEOUT("\n\n");

    VERYVERBOSEOUT(graphmemman.info_str());
    auto ret = server_request_with_shared_data(segname, fzgs.graph().get_server_port());

    return standard_exit(ret == exit_ok, "Finding Nodes in Graph done.", ret, "Unable to store search results in Named Node List", __func__);
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzgs.init_top(argc, argv);

    /*
    There is a lot to do here, but let's just start with something very simple and immediately
    useful.

    Just an easy way to look for Nodes that have a description that contains a string.

    Eventually, this can include full use of Node_Filter, Node categories and category files or
    category building (as in fzupdate).
    */

    switch (fzgs.flowcontrol) {

    case flow_find_nodes: {
        return find_nodes();
    }

    default: {
        fzgs.print_usage();
    }

    }

    return standard.completed_ok();
}
