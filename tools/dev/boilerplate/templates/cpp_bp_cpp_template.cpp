// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ brief_description }}
 * 
 * {{ long_description }}
 * 
 * For more about this, see {{ doc_reference }}.
 */

#define FORMALIZER_MODULE_ID "Formalizer:{{ module_id }}"

// std
#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"

// local
#include "version.hpp"
#include "{{ this }}.hpp"
{{ render_hpp }}


using namespace fz;

/// The local class derived from `formalizer_standard_program`.
{{ this }} {{ th }};

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
{{ this }}::{{ this }}() {
    //add_option_args += "x:";
    //add_usage_top += " [-x <something>]";
    //usage_head.push_back("Description at the head of usage information.\n");
    //usage_tail.push_back("Extra usage information.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void {{ this }}::usage_hook() {
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
bool {{ this }}::options_hook(char c, std::string cargs) {

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
void {{ this }}::init_top(int argc, char *argv[]) {
    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

int main(int argc, char *argv[]) {
    {{ th }}.init_top(argc, argv);
    {{ th }}.init(argc,argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);

    FZOUT("\nThis is a stub.\n\n");
    key_pause();

    switch ({{ th }}.flowcontrol) {

    /*
    case flow_something: {
        return something();
    }
    */

    default: {
        {{ th }}.print_usage();
    }

    }

    return {{ th }}.completed_ok();
}
