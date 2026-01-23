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
#include <algorithm>
#include <map>
#include <ranges>

// core
#include "error.hpp"
#include "standard.hpp"
#include "Graphinfo.hpp"
#include "Graphmodify.hpp"
#include "Graphtoken.hpp"
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
    add_option_args += "s:l:i:I:zc:C:m:M:t:T:rRp:P:b:d:D:S:B:N:F:f:X:";
    add_usage_top += " [-s <search-string>] [-z] [-i <date-time>] [-I <date-time>] [-c <comp_min>] [-C <comp_max>] [-m <mins_min>]"
                     " [-M <mins_max>] [-t <TD_min>] [-T <TD_max>] [-p <tdprop_1>] [-P <tdprop_2>] [-b <tdprop-list>] [-r|-R]"
                     " [-d <tdpatt_1>] [-D <tdpatt_2>] [-S <sup-spec>] [-B <top-node>] [-N <listname>] [-F <BTF-category>] [-f <BTF-NNL>]"
                     " [-X <N[:order]]"
                     " -l <list-name>";
    //usage_head.push_back("Description at the head of usage information.\n");
    usage_tail.push_back("Target date property options are: unspecified, variable, inherit, fixed, exact.\n"
                         "Repeat pattern options are: nonrepeating, weekly, biweekly, monthly,\n"
                         "endofmonth_offset, yearly.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzgraphsearch::usage_hook() {
    //ga.usage_hook();
    FZOUT("    -l Named Node List to receive the search results.\n"
          "    -s Description contains <search-string>.\n"
          "    -z Not case sensitive.\n"
          "    -i Nodes created from <date-time>.\n"
          "    -I Nodes created through <date-time>.\n"
          "    -c Nodes with completion ratio greater than or equal to <comp_min>.\n"
          "    -C Nodes with completion ratio smaller than or equal to <comp_max>.\n"
          "    -m Nodes with required minutes greater than or equal to <mins_min>.\n"
          "    -M Nodes with required minutes greater than or equal to <mins_min>.\n"
          "    -t Nodes with effective target date at or after <TD_min>, or 'invalid'.\n"
          "    -T Nodes with effective target date at or before <TD_max>, or 'invalid'.\n"
          "    -p Nodes with target date property <tdprop_1>.\n"
          "    -P Nodes with target date property <tdprop_2>.\n"
          "    -b Nodes with TD property one of comma separated <tdprop-list>.\n"
          "       List possibilities are: u,i,v,f,e\n"
          "    -r Nodes that repeat.\n"
          "    -R Nodes that do not repeat.\n"
          "    -d Nodes with repeat pattern <tdpatt_1>.\n"
          "    -D Nodes with repeat pattern <tdpatt_2>.\n"
          "    -S Nodes with superiors: self, 0, n+.\n"
          "    -B Nodes belonging to the subtree of <top-node>.\n"
          "    -N Nodes in the subtree map of NNL <listname>.\n"
          "    -F Nodes belonging to <BTF-category>, requires -f.\n"
          "    -f NNL to use for BTF mapping.\n"
          "    -X Return N counted by order: oldest (default), newest, nearest,\n"
          "       furthest\n"
          );
}

/**
 * Parses a comma separated list of flags to set which TD properties to include
 * in the search.
 */
bool fzgraphsearch::parse_tdproperty_binary_pattern(const std::string & cargs) {
    bool res = false;
    auto binpatvec = split(cargs, ',');
    for (auto & binpatspec : binpatvec) {
        if (trim(binpatspec)=="u") {
            nodefilter.tdpropbinpattern.set_unspecified();
            res = true;
        } else if (trim(binpatspec)=="i") {
            nodefilter.tdpropbinpattern.set_inherit();
            res = true;
        } else if (trim(binpatspec)=="v") {
            nodefilter.tdpropbinpattern.set_variable();
            res = true;
        } else if (trim(binpatspec)=="f") {
            nodefilter.tdpropbinpattern.set_fixed();
            res = true;
        } else if (trim(binpatspec)=="e") {
            nodefilter.tdpropbinpattern.set_exact();
            res = true;
        } else {
            return standard_error("Unrecognized TD property binary pattern specifier: "+trim(binpatspec), __func__);
        }
    }
    return res;
}

bool fzgraphsearch::get_superiors_specification(const std::string & cargs) {
    bool res = false;
    if (cargs=="self") {
        nodefilter.self_is_superior = true;
        res = true;
    } else if (cargs=="0") {
        nodefilter.has_no_superiors = true;
        res = true;
    } else if (cargs.back()=='+') {
        nodefilter.at_least_n_superiors = static_cast<unsigned int>(std::atoi(cargs.c_str()));
        res = true;
    } else {
        return standard_error("Unrecognized Superior specification: "+cargs, __func__);
    }
    nodefilter.filtermask.set_Edit_supspecmatch();
    return res;
}

bool fzgraphsearch::get_subtree(const std::string & cargs) {
    Node * node_ptr = graph().Node_by_idstr(cargs);
    if (!node_ptr) {
        return standard_error("Subtree top Node not found in Graph: "+cargs, __func__);
    }
    nodefilter.subtree_uptr = std::make_unique<Subtree_Branch_Map>();
    Subtree_Branch_Map& subtree = *(nodefilter.subtree_uptr.get());
    std::set<Node_ID_key> do_not_follow;
    do_not_follow.emplace(node_ptr->get_id().key());
    if (!Node_Dependencies_fulldepth(node_ptr, subtree, do_not_follow)) {
        return standard_error("Full depth dependencies collection failed for Node "+cargs, __func__);
    }
    nodefilter.filtermask.set_Edit_subtreematch();
    return true;
}

bool fzgraphsearch::get_nnltree(const std::string & cargs) {
    nodefilter.nnltree_uptr = std::make_unique<Map_of_Subtrees>();
    if (!nodefilter.nnltree_uptr->collect(graph(), cargs)) {
        return standard_error("Unable to collect map of subtrees for NNL: "+cargs, __func__);
    }
    nodefilter.filtermask.set_Edit_nnltreematch();
    return true;
}

const std::map<std::string, excerpt_options> str2excerptoption = {
    { "oldest", oldest },
    { "newest", newest },
    { "nearest", nearest },
    { "furthest", furthest },
};

bool fzgraphsearch::get_excerpt_specs(const std::string& cargs) {
    excerpt_size = std::atoi(cargs.c_str());
    auto colon_pos = cargs.find(':');
    if (colon_pos == std::string::npos) return true;
    std::string order_str = cargs.substr(colon_pos+1);
    auto it = str2excerptoption.find(order_str);
    if (it == str2excerptoption.end()) {
        return standard_error("Unrecognized order specifier: "+order_str, __func__);
    }
    excerpt_counting_by = it->second;
    return true;
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
        nodefilter.lowerbound.utf8_text = cargs;
        nodefilter.filtermask.set_Edit_text();
        return true;
    }

    case 'i': {
        nodefilter.t_created_lowerbound = time_stamp_time(cargs);
        nodefilter.filtermask.set_Edit_tcreated();
        return true;
    }

    case 'I': {
        nodefilter.t_created_upperbound = time_stamp_time(cargs);
        nodefilter.filtermask.set_Edit_tcreated();
        return true;
    }

    case 'z': {
        nodefilter.case_sensitive = false;
        return true;
    }

    case 'c': {
        nodefilter.lowerbound.completion = std::atof(cargs.c_str());
        nodefilter.filtermask.set_Edit_completion();
        return true;
    }

    case 'C': {
        nodefilter.upperbound.completion = std::atof(cargs.c_str());
        nodefilter.filtermask.set_Edit_completion();
        return true;
    }

    case 'm': {
        nodefilter.lowerbound.hours = ((float)std::atoi(cargs.c_str())) / 60.0;
        nodefilter.filtermask.set_Edit_required();
        return true;
    }

    case 'M': {
        nodefilter.upperbound.hours = ((float)std::atoi(cargs.c_str())) / 60.0;
        nodefilter.filtermask.set_Edit_required();
        return true;
    }

    case 't': {
        if (cargs == "invalid") {
            nodefilter.has_invalid_targetdate = true;
            nodefilter.filtermask.set_Edit_targetdate();
            return true;
        } else {
            nodefilter.lowerbound.targetdate = time_stamp_time(cargs);
            nodefilter.filtermask.set_Edit_targetdate();
            return true;
        }
    }

    case 'T': {
        if (cargs == "invalid") {
            nodefilter.has_invalid_targetdate = true;
            nodefilter.filtermask.set_Edit_targetdate();
            return true;
        } else {
            nodefilter.upperbound.targetdate = time_stamp_time(cargs);
            nodefilter.filtermask.set_Edit_targetdate();
            return true;
        }
    }

    case 'p': {
        nodefilter.lowerbound.tdproperty = interpret_config_tdproperty(cargs);
        nodefilter.filtermask.set_Edit_tdproperty();
        return true;
    }

    case 'P': {
        nodefilter.upperbound.tdproperty = interpret_config_tdproperty(cargs);
        nodefilter.filtermask.set_Edit_tdproperty();
        return true;
    }

    case 'b': {
        if (parse_tdproperty_binary_pattern(cargs)) {
            nodefilter.filtermask.set_Edit_tdpropbinpat();
            return true;
        } else {
            return false;
        }
    }

    case 'r': {
        nodefilter.lowerbound.repeats = true;
        nodefilter.upperbound.repeats = true;
        nodefilter.filtermask.set_Edit_repeats();
        return true;
    }

    case 'R': {
        nodefilter.lowerbound.repeats = false;
        nodefilter.upperbound.repeats = false;
        nodefilter.filtermask.set_Edit_repeats();
        return true;
    }

    case 'd': {
        nodefilter.lowerbound.tdpattern = interpret_config_tdpattern(cargs);
        nodefilter.filtermask.set_Edit_tdpattern();
        return true;
    }

    case 'D': {
        nodefilter.upperbound.tdpattern = interpret_config_tdpattern(cargs);
        nodefilter.filtermask.set_Edit_tdpattern();
        return true;
    }

    case 'S': {
        return get_superiors_specification(cargs);
    }

    case 'B': {
        return get_subtree(cargs);
    }

    case 'N': {
        return get_nnltree(cargs);
    }

    case 'F': {
        btf = get_btf(cargs);
        return btf != Boolean_Tag_Flags::none;
    }

    case 'f': {
        btf_nnl = cargs;
        return true;
    }

    case 'l': {
        listname = cargs;
        flowcontrol = flow_find_nodes;
        return true;
    }

    case 'X': {
        return get_excerpt_specs(cargs);
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

// Add to results list if additional conditions are met.
void fzgraphsearch::conditional_result_add(const Node* match_ptr) {
    if (btf != Boolean_Tag_Flags::none) {
        Boolean_Tag_Flags::boolean_flag boolean_tag;
        if (!map_of_subtrees.node_in_heads_or_any_subtree(match_ptr->get_id().key(), boolean_tag, true, true)) { // This uses get_PriorityCategory() on Node or Subtree header.
            boolean_tag = Boolean_Tag_Flags::none;
        }
        if (boolean_tag != btf) return;
    }

    Named_Node_List_Element * listelement_ptr = graphmod_ptr->request_Named_Node_List_Element(namedlist_add, listname, match_ptr->get_id().key());
    if (!listelement_ptr)
        standard_exit_error(exit_general_error, "Unable to create new Named Node List Element in shared segment ("+graphmemman.get_active_name()+')', __func__);

    matched_count++;
    VERYVERBOSEOUT("\nAdding Node "+match_ptr->get_id_str()+" to Named Node List "+listname);
}

int find_nodes() {
    ERRTRACE;
    if (fzgs.listname.empty()) {
        standard_exit_error(exit_missing_data, "No Named Node List name specified", __func__);
    }

    // set up filter
    // *** Already done during command line parameter parsing.
    VERYVERBOSEOUT("Node filter:\n"+fzgs.nodefilter.str());

    // Possible BTF filter
    if (fzgs.btf != Boolean_Tag_Flags::none) {
        if (fzgs.btf_nnl.empty()) {
            VERBOSEOUT("Missing list name to use for Boolean Tag Flag categorized mapping.\n");
            return standard_exit_success(""); // We need verbose output, not very verbose.
        }
        fzgs.map_of_subtrees.collect(fzgs.graph(), fzgs.btf_nnl);
        if (!fzgs.map_of_subtrees.has_subtrees) {
            VERBOSEOUT("No subtrees of "+fzgs.btf_nnl+" to use for Boolean Tag Flag categorized mapping.\n");
            return standard_exit_success(""); // We need verbose output, not very verbose.
        }
    }

    // find subset of Nodes
    targetdate_sorted_Nodes matched_nodes = Nodes_subset(
        fzgs.graph(),
        fzgs.nodefilter,
        (fzgs.excerpt_counting_by == oldest) || (fzgs.excerpt_counting_by == newest) ? fzgs.excerpt_size : 0,
        fzgs.excerpt_counting_by == newest);

    size_t total_found = matched_nodes.size();
    if (total_found < 1) {
        VERBOSEOUT("0 matching Nodes found.\n");
        return standard_exit_success(""); // We need verbose output, not very verbose.
    }

    VERBOSEOUT("\nAdding "+std::to_string(total_found)+" matching Nodes to List "+fzgs.listname+".\n");
    unsigned long segsize = 1024+(sizeof(Named_Node_List_Element)+sizeof(Graphmod_result))*2*total_found;
    // Determine a unique segment name to share with `fzserverpq`
    std::string segname(unique_name_Graphmod());
    fzgs.graphmod_ptr = allocate_Graph_modifications_in_shared_memory(segname, segsize);
    if (!fzgs.graphmod_ptr)
        standard_exit_error(exit_general_error, "Unable to create shared segment for modifications requests (name="+segname+", size="+std::to_string(segsize)+')', __func__);

    VERYVERBOSEOUT(graphmemman.info_str());
    if (fzgs.excerpt_counting_by != furthest) {
        for (const auto & [t_match, match_ptr] : matched_nodes) {
            fzgs.conditional_result_add(match_ptr);

            if ((fzgs.excerpt_size != 0) && (fzgs.excerpt_counting_by == nearest) && (fzgs.matched_count >= fzgs.excerpt_size)) {
                break;
            }
        }
    } else {
        for (const auto & [t_match, match_ptr] : matched_nodes | std::views::reverse) {
            fzgs.conditional_result_add(match_ptr);

            if ((fzgs.excerpt_size != 0) && (fzgs.excerpt_counting_by == furthest) && (fzgs.matched_count >= fzgs.excerpt_size)) {
                break;
            }
        }
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
