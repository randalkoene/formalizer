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

// std
#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"

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
    //add_option_args += "x:";
    //add_usage_top += " [-x <something>]";
    //usage_head.push_back("Description at the head of usage information.\n");
    //usage_tail.push_back("Extra usage information.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzgraphhtml::usage_hook() {
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
bool fzgraphhtml::options_hook(char c, std::string cargs) {
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
bool fzgh_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
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
void fzgraphhtml::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzgh.init_top(argc, argv);

    FZOUT("\nThis is a stub.\n\n");
    key_pause();

    switch (fzgh.flowcontrol) {

    /*
    case flow_something: {
        return something();
    }
    */

    default: {
        fzgh.print_usage();
    }

    }

    return standard.completed_ok();
}
