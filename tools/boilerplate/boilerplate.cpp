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

#include <iostream>

#include "version.hpp"
#include "error.hpp"
#include "standard.hpp"
#include "templater.html"

using namespace fz;

struct boilerplate: public formalizer_standard_program {

    boilerplate(): {
        //add_option_args += "n:";
        //add_usage_top += " [-n <Node-ID>]";
    }

    virtual void usage_hook() {
        
    }

    virtual bool options_hook(char c, char * cargs) {
        //if (ga.options_hook(c,cargs))
        //    return true;

        /*
        switch (c) {

        }
        */

       return false;
    }
} bp;

int main(int argc, char *argv[]) {
    bp.init(argc,argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);

    FZOUT("\nThis is the stub of the stub generator.\n\n");
    key_pause();

    switch (bp.flowcontrol) {

    case flow_: {
        ;
        break;
    }

    default: {
        bp.print_usage();
    }

    }

    return bp.completed_ok();
}

