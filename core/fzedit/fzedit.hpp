// Copyright 20201126 Randal A. Koene
// License TBD

/**
 * Edit components of the Graph (Nodes, Edges, Topics).
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZEDIT_HPP.
 */

#ifndef __FZEDIT_HPP
#include "version.hpp"
#define __FZEDIT_HPP (__VERSION_HPP)

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


class fze_configurable: public configurable {
public:
    fze_configurable(formalizer_standard_program & fsp): configurable("fzedit", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    //std::string example_par;   ///< example of configurable parameter
};


struct fzedit: public formalizer_standard_program {

    fze_configurable config;

    flow_options flowcontrol;

    // Graph_access ga; // to include Graph or Log access support

    fzedit();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

};

extern fzedit fze;

#endif // __FZEDIT_HPP
