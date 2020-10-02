// Copyright 2020 Randal A. Koene
// License TBD

/**
 * fzserverpq is the Postgres-compatible version of the C++ implementation of teh Formalizer data server.
 * 
 * For more information see:
 * https://docs.google.com/document/d/1rYPFgzFgjkF1xGx3uABiXiaDR5sfmOzqYQRqSntcyyY/edit#heading=h.tarhfe395l5v
 * https://trello.com/c/S7SZUyeU
 * 
 * For more about this, see {{ doc_reference }}.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Server:Graph:Postgres"

// std
#include <iostream>

// core
#include "version.hpp"
#include "error.hpp"
#include "standard.hpp"
#include "Graphtypes.hpp"

// local
#include "fzserverpq.hpp"


using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzserverpq fzs;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzserverpq::fzserverpq(): formalizer_standard_program(false), ga(*this, add_option_args, add_usage_top, true), flowcontrol(flow_unknown) {
    //add_option_args += "x:";
    //add_usage_top += " [-x <something>]";
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzserverpq::usage_hook() {
    ga.usage_hook();
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
bool fzserverpq::options_hook(char c, std::string cargs) {
    if (ga.options_hook(c,cargs))
        return true;

    switch (c) {

    /*
    case 'x': {

        break;
    }
    */

    }

    return false;
}

/**
 * Initialize configuration parameters.
 * Call this at the top of main().
 * 
 * @param argc command line parameters count forwarded from main().
 * @param argv command line parameters array forwarded from main().
 */
void fzserverpq::init_top(int argc, char *argv[]) {
    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

std::unique_ptr<Graph> init_resident_Graph() {
    std::unique_ptr<Graph> graph = fzs.ga.request_Graph_copy();
    if (!graph) {
        ADDERROR(__func__,"unable to load Graph");
        standard.exit(exit_database_error);
    }
    return graph;
}

int main(int argc, char *argv[]) {
    fzs.init_top(argc, argv);
    fzs.init(argc,argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);

    std::unique_ptr<Graph> graph = init_resident_Graph();

    FZOUT("\nThis is a stub.\n\n");
    key_pause();

    switch (fzs.flowcontrol) {

    /*
    case flow_something: {
        return something();
    }
    */

    default: {
        fzs.print_usage();
    }

    }

    return standard.completed_ok();
}
