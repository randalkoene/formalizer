// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ header_description }}
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZGRAPHHTML_HPP.
 */

#ifndef __FZGRAPHHTML_HPP
#include "version.hpp"
#define __FZGRAPHHTML_HPP (__VERSION_HPP)

// core
#include "config.hpp"
#include "standard.hpp"
// #include "Graphaccess.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0, /// no recognized request
    //flow_something = 1,     /// request: make boilerplate for C++ program
    flow_NUMoptions
};


class fzgh_configurable: public configurable {
public:
    fzgh_configurable(formalizer_standard_program & fsp): configurable("fzgraphhtml", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    //std::string example_par;   ///< example of configurable parameter
};


struct fzgraphhtml: public formalizer_standard_program {

    fzgh_configurable config;

    flow_options flowcontrol;

    // Graph_access ga; // to include Graph or Log access support

    fzgraphhtml();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

};

extern fzgraphhtml fzgh;

#endif // __FZGRAPHHTML_HPP