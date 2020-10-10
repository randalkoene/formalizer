// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ brief_description }}
 * 
 * {{ long_description }}
 * 
 * For more about this, see {{ doc_reference }}.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Log:MakeEntry"

// std
#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"

// local
#include "version.hpp"
#include "fzlog.hpp"



using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzlog fzl;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzlog::fzlog() : formalizer_standard_program(false), config(*this) { //ga(*this, add_option_args, add_usage_top)
    add_option_args += "n:";
    add_usage_top += " [-n <node-ID>]";
    //usage_head.push_back("Description at the head of usage information.\n");
    //usage_tail.push_back("Extra usage information.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzlog::usage_hook() {
    //ga.usage_hook();
    FZOUT("    -n entry belongs to Node with <node-ID>\n");
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
bool fzlog::options_hook(char c, std::string cargs) {
    //if (ga.options_hook(c,cargs))
    //        return true;

    switch (c) {

    case 'n': {
        //*** try to replace a null-key with a key initialized with cargs
        break;
    }

    }

    return false;
}


/// Configure configurable parameters.
bool fzl_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
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
void fzlog::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

/*
The following is the function call used to add a Log entry to the Log:

    bool add_Logentry_pq(const active_pq & apq, const Log_entry & entry);

*/

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzl.init_top(argc, argv);

    FZOUT("\nThis is a stub.\n\n");
    key_pause();

    switch (fzl.flowcontrol) {

    /*
    case flow_something: {
        return something();
    }
    */

    default: {
        fzl.print_usage();
    }

    }

    return standard.completed_ok();
}
