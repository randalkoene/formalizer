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
#include "version.hpp"
#include "error.hpp"
#include "standard.hpp"

// local
#include "{{ this }}.hpp"


using namespace fz;

{{ this }} {{ th }};

{{ this }}::{{ this }}() {
    //add_option_args += "x:";
    //add_usage_top += " [-x <something>]";
}

void {{ this }}::usage_hook() {
    //FZOUT("    -x something explanation\n");        
}

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

int main(int argc, char *argv[]) {
    {{ th }}.init(argc,argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);

    FZOUT("\nThis a the stub.\n\n");
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
