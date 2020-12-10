// Copyright 20201126 Randal A. Koene
// License TBD

/**
 * Update the Node Schedule.
 * 
 * {{ long_description }}
 * 
 * For more about this, see {{ doc_reference }}.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Graph:Update"

// std
#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"
#include "Graphinfo.hpp"
#include "Graphmodify.hpp"

// local
#include "version.hpp"
#include "fzupdate.hpp"



using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzupdate fzu;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzupdate::fzupdate() : formalizer_standard_program(false), config(*this),
                 reftime(add_option_args, add_usage_top) { //ga(*this, add_option_args, add_usage_top)
    add_option_args += "ru";
    add_usage_top += " [-r] [-u]";
    //usage_head.push_back("Description at the head of usage information.\n");
    //usage_tail.push_back("Extra usage information.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzupdate::usage_hook() {
    //ga.usage_hook();
    reftime.usage_hook();
    FZOUT("    -r update repeating Nodes\n"
          "    -u update variable target date Nodes\n");
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
bool fzupdate::options_hook(char c, std::string cargs) {
    //if (ga.options_hook(c,cargs))
    //        return true;
    if (reftime.options_hook(c, cargs)) {
        if (reftime.Time() == RTt_invalid_time_stamp) {
            standard_exit_error(exit_general_error, "Invalid emulated time specification ("+cargs+')', __func__);
        }
        return true;
    }

    switch (c) {

    case 'r': {
        flowcontrol = flow_update_repeating;
        break;
    }

    case 'u': {
        flowcontrol = flow_update_variable;
        break;
    }

    }

    return false;
}


/// Configure configurable parameters.
bool fzu_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
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
void fzupdate::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

Graph & fzupdate::graph() {
    ERRTRACE;
    if (!graphmemman.get_Graph(graph_ptr)) {
        standard_exit_error(exit_resident_graph_missing, "Memory resident Graph not found.", __func__);
    }
    return *graph_ptr;
}

int update_repeating(time_t t_pass) {
    targetdate_sorted_Nodes incomplete_repeating = Nodes_incomplete_and_repeating_by_targetdate(fzu.graph());
    Edit_flags editflags;
    targetdate_sorted_Nodes updated_repeating = Update_repeating_Nodes(incomplete_repeating, t_pass, editflags);
    if (!editflags.None()) {
        // *** here, call the Graphpostgres function that can update multiple Nodes
    }
    return standard_exit_success("Update repeating done.");
}

int update_variable(time_t t_pass) {
    Edit_flags editflags;

    // *** You can probably copy the method used in dil2al, because placing all of the immovables in a fine-grained
    //     map and then putting the variable target date Nodes where they have to be was already a good strategy.
    FZOUT("NOT YET IMPLEMENTED!\n");

    if (!editflags.None()) {
        // *** here, call the Graphpostgres function that can update multiple Nodes
    }

    return standard_exit_success("Update variable target date Nodes done.");
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzu.init_top(argc, argv);

    FZOUT("\nThis is a stub.\n\n");
    key_pause();

    switch (fzu.flowcontrol) {

    case flow_update_repeating: {
        return update_repeating(fzu.reftime.Time());
    }

    case flow_update_variable: {
        return update_variable(fzu.reftime.Time());
    }

    default: {
        fzu.print_usage();
    }

    }

    return standard.completed_ok();
}
