// Copyright 20201126 Randal A. Koene
// License TBD

/**
 * Update the Node Schedule.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZUPDATE_HPP.
 */

#ifndef __FZUPDATE_HPP
#include "version.hpp"
#define __FZUPDATE_HPP (__VERSION_HPP)

// core
#include "config.hpp"
#include "standard.hpp"
// #include "Graphaccess.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0,          /// no recognized request
    flow_update_repeating = 1, /// request: update repeating Nodes
    flow_update_variable = 2,  /// request: update variable target date Nodes
    flow_NUMoptions
};

class fzu_configurable: public configurable {
public:
    fzu_configurable(formalizer_standard_program & fsp): configurable("fzupdate", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    //std::string example_par;   ///< example of configurable parameter
};


struct fzupdate: public formalizer_standard_program {

    fzu_configurable config;

    flow_options flowcontrol;

    Graph * graph_ptr = nullptr;

    // Graph_access ga; // to include Graph or Log access support

    ReferenceTime reftime;

    fzupdate();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    Graph & graph();

};

extern fzupdate fzu;

#endif // __FZUPDATE_HPP
