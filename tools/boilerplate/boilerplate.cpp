// Copyright 2020 Randal A. Koene
// License TBD

/**
 * boilerplate is the authoritative source code stub generator for Formalizer development.
 * 
 * Source code stubs for multple programming languages can be generated and the process
 * can include everything from setting up a directory in the correct place within the
 * Formalizer source code hierarchy, to initializing the main source files for a
 * standardized Formalizer program, to auto-generating necessary additional files,
 * such as `version.hpp` and a Makefile.
 * 
 * For more about this, see the Trello card at https://trello.com/c/8czp28zx.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Development:AutoGeneration:Init"

// std
#include <iostream>

// core
#include "version.hpp"
#include "error.hpp"
#include "standard.hpp"

// local
#include "boilerplate.hpp"
#include "cpp_boilerplate.hpp"

using namespace fz;

boilerplate bp;

boilerplate::boilerplate() {
    add_option_args += "T:";
    add_usage_top += " [-T <target>]";
}

void boilerplate::usage_hook() {
    FZOUT("    -T target (language): cpp, python\n");        
}

bool boilerplate::options_hook(char c, std::string cargs) {

    switch (c) {

    case 'T':
        if (cargs == "cpp") {
            flowcontrol = flow_cpp;
            return true;
        }
        if (cargs == "python") {
            flowcontrol = flow_python;
            return true;
        }

    }

    return false;
}

int main(int argc, char *argv[]) {
    bp.init(argc,argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);

    FZOUT("\nThis is the stub of the stub generator.\n\n");
    key_pause();

    switch (bp.flowcontrol) {

    case flow_cpp: {
        return make_cpp_boilerplate();
    }

    case flow_python: {

        break;
    }

    default: {
        bp.print_usage();
    }

    }

    return bp.completed_ok();
}

