// Copyright 2020 Randal A. Koene
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
#include "standard.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0, /// no recognized request
    //flow_something = 1,     /// request: make boilerplate for C++ program
    flow_NUMoptions
};

struct {{ this }}: public formalizer_standard_program {

    flow_options flowcontrol;

    {{ this }}();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

};

extern {{ this }} {{ th }};

#endif // __{{ CAPSthis }}_HPP
