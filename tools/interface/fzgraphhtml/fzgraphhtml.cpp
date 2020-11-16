// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ brief_description }}
 * 
 * {{ long_description }}
 * 
 * For more about this, see {{ doc_reference }}.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Interface:Graph:HTML"

//#define USE_COMPILEDPING

// std
#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"
#include "stringio.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"

// local
#include "version.hpp"
#include "fzgraphhtml.hpp"
#include "render.hpp"

using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzgraphhtml fzgh;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzgraphhtml::fzgraphhtml() : formalizer_standard_program(false), config(*this) { //ga(*this, add_option_args, add_usage_top)
    add_option_args += "n:IN:o:eC";
    add_usage_top += " [-n <node-ID>] [-I] [-L <name|?>] [-N <num>] [-o <output-path>] [-e] [-C]";
    //usage_head.push_back("Description at the head of usage information.\n");
    usage_tail.push_back("When no [N <num>] is provided then the configured value is used.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzgraphhtml::usage_hook() {
    //ga.usage_hook();
    FZOUT("    -n Show data for Node with <node-ID>\n");
    FZOUT("    -I Show data for incomplete Nodes\n");
    FZOUT("    -L Show data for Nodes in Named Node List, or show Names if '?'\n");
    FZOUT("    -N Show data for [num] elements (all=no limit)\n");
    FZOUT("    -o Rendered output to <output-path> (\"STDOUT\" is default)\n");
    FZOUT("    -e Embeddable, no head and tail templates\n");
    FZOUT("    -C (TEST) card output format\n");
}

unsigned int parvalue_to_num_to_show(const std::string & parvalue) {
    if (parvalue.empty())
        return 0;

    unsigned int vmax = std::numeric_limits<unsigned int>::max();
    if (parvalue == "all")
        return vmax;

    long val = atol(parvalue.c_str());
    if (val > vmax)
        return vmax;

    return val;
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
bool fzgraphhtml::options_hook(char c, std::string cargs) {
    //if (ga.options_hook(c,cargs))
    //        return true;

    switch (c) {

    case 'n': {
        flowcontrol = flow_node;
        node_idstr = cargs;
        return true;
    }

    case 'I': {
        flowcontrol = flow_incomplete;
        return true;
    }

    case 'L': {
        flowcontrol = flow_named_list;
        list_name = cargs;
        return true;
    }

    case 'N': {
        config.num_to_show = parvalue_to_num_to_show(cargs);
        return true;
    }

    case 'o': {
        config.rendered_out_path = cargs;
        return true;
    }

    case 'e': {
        config.embeddable = true;
        return true;
    }

    case 'C': {
        test_cards = true;
        return true;
    }
   
    }

    return false;
}

bool parvalue_to_bool(const std::string & parvalue) {
    if (parvalue == "true") {
        return true;
    } else {
        return false;
    }
}

/// Configure configurable parameters.
bool fzgh_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(num_to_show, "num_to_show", parlabel, parvalue_to_num_to_show(parvalue));
    CONFIG_TEST_AND_SET_PAR(excerpt_length, "excerpt_length", parlabel, atoi(parvalue.c_str()));
    CONFIG_TEST_AND_SET_PAR(rendered_out_path, "rendered_out_path", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(embeddable, "embeddable", parlabel, parvalue_to_bool(parvalue));
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
void fzgraphhtml::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

void test_other_graph_info(Graph & graph) {

    //VERYVERBOSEOUT(List_Topics(graph, "\n"));

    VERYVERBOSEOUT('\n'+Nodes_statistics_string(Nodes_statistics(graph)));

    VERYVERBOSEOUT("\nNumber of Edges with non-zero edge data = "+std::to_string(Edges_with_data(graph))+'\n');

}

void get_node_info() {
    ERRTRACE;

    auto [node_ptr, graph_ptr] = find_Node_by_idstr(fzgh.node_idstr, nullptr);

    if (graph_ptr) {

        VERYVERBOSEOUT(Graph_Info_str(*graph_ptr));

        test_other_graph_info(*graph_ptr);

    } else {
        standard_exit_error(exit_resident_graph_missing, "Unable to access memory-resident Graph.", __func__);
    }

    ERRHERE(".render");
    if (node_ptr) {

        std::string rendered_node_data(render_Node_data(*graph_ptr, *node_ptr));

        if (fzgh.config.rendered_out_path == "STDOUT") {
            FZOUT(rendered_node_data);
        } else {
            if (!string_to_file(fzgh.config.rendered_out_path, rendered_node_data))
                standard_exit_error(exit_file_error, "Unable to write rendered page to file.", __func__);
        }

    } else {
        standard_exit_error(exit_bad_request_data, "Invalid Node ID: "+fzgh.node_idstr, __func__);
    }

}


int main(int argc, char *argv[]) {
    ERRTRACE;

    fzgh.init_top(argc, argv);

    switch (fzgh.flowcontrol) {

    case flow_node: {
        get_node_info();
        break;
    }

    case flow_incomplete: {
        render_incomplete_nodes();
        break;
    }

    case flow_named_list: {
        render_named_node_list();
        break;
    }

    default: {
        fzgh.print_usage();
    }

    }

    return standard.completed_ok();
}
