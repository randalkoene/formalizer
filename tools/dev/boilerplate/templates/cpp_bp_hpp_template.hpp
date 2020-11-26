// Copyright {{ thedate }} Randal A. Koene
// License TBD

/**
 * {{ header_description }}
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __{{ CAPSthis }}_HPP.
 */

#ifndef __{{ CAPSthis }}_HPP
#include "version.hpp"
#define __{{ CAPSthis }}_HPP (__VERSION_HPP)

// core
{{ config_include }}
#include "standard.hpp"
// #include "Graphaccess.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0, /// no recognized request
    //flow_something = 1,     /// request: make boilerplate for C++ program
    flow_NUMoptions
};

{{ th_configurable_or_configbase }}

struct {{ this }}: public formalizer_standard_program {

    {{ config_support }}

    flow_options flowcontrol;

    // Graph_access ga; // to include Graph or Log access support

    {{ this }}();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

};

extern {{ this }} {{ th }};

#endif // __{{ CAPSthis }}_HPP
