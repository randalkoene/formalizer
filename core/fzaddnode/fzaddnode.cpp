// Copyright 2020 Randal A. Koene
// License TBD

/**
 * The fzaddnode tool is the authoritative core component with which to
 * add new Nodes and their initial Edges to the Graph.
 * 
 * {{ long_description }}
 * 
 * For more about this, see https://trello.com/c/FQximby2.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Graph:MakeNode"

// std
#include <iostream>
#include <iterator>

// core
#include "error.hpp"
#include "standard.hpp"

// local
#include "version.hpp"
#include "ReferenceTime.hpp"
#include "Graphbase.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "fzaddnode.hpp"


using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzaddnode fzan;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzaddnode::fzaddnode() : formalizer_standard_program(false), graph_ptr(nullptr), config(*this) { //ga(*this, add_option_args, add_usage_top)
    //add_option_args += "x:";
    //add_usage_top += " [-x <something>]";
    //usage_head.push_back("Description at the head of usage information.\n");
    //usage_tail.push_back("Extra usage information.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzaddnode::usage_hook() {
    //ga.usage_hook();
    //FZOUT("    -x something explanation\n");
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
bool fzaddnode::options_hook(char c, std::string cargs) {
    //if (ga.options_hook(c,cargs))
    //        return true;

    switch (c) {

    /*
    case 'x': {

        break;
    }
    */

    }

    return false;
}


/// Configure configurable parameters.
bool fzan_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
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
void fzaddnode::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}


Graph * fzaddnode::get_Graph() {
    if (!graph_ptr) {

        graph_ptr = graphmemman.find_Graph_in_shared_memory();
        if (!graph_ptr) {
            standard_exit_error(exit_general_error, "Memory resident Graph not found in shared segment ("+graphmemman.get_active_name()+')', __func__);
        }

    }
    return graph_ptr;
}

std::string make_node_id() {
    time_t t_now = ActualTime();
    // Check for other Nodes with that time stamp.
    key_sorted_Nodes nodes = Nodes_created_in_time_interval(*(fzan.get_Graph()), t_now, t_now+1);

    // Is there still a minor_id available (single digit)?
    uint8_t minor_id = 1;
    if (!nodes.empty()) {
        minor_id = std::prev(nodes.end())->second->get_id().key().idT.minor_id + 1;
        if (minor_id > 9) {
            standard_exit_error(exit_general_error, "No minor_id digits remaining for Node ID with time stamp "+Node_ID_TimeStamp_from_epochtime(t_now), __func__);
        }
    }

    // Generate Node ID time stamp with minor-ID.
    std::string nodeid_str = Node_ID_TimeStamp_from_epochtime(t_now, minor_id);

    return nodeid_str;
}

int make_node() {
    std::string nodeid_str = make_node_id();
    VERBOSEOUT("Making Node "+nodeid_str+'\n');

    FZOUT("THIS IS JUST A STUB\n");

    return standard.completed_ok();
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzan.init_top(argc, argv);

    fzan.flowcontrol = flow_make_node; // *** There are no other options yet.

    switch (fzan.flowcontrol) {

    case flow_make_node: {
        return make_node();
    }

    default: {
        fzan.print_usage();
    }

    }

    return standard.completed_ok();
}
