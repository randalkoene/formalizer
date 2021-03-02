// Copyright 20210226 Randal A. Koene
// License TBD

/**
 * {{ header_description }}
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZDASHBOARD_HPP.
 */

#ifndef __FZDASHBOARD_HPP
#include "version.hpp"
#define __FZDASHBOARD_HPP (__VERSION_HPP)

// core
#include "config.hpp"
#include "standard.hpp"
// #include "Graphaccess.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0, /// no recognized request
    flow_dashboard = 1,     /// request: render dashboard
    flow_NUMoptions
};


class fzdsh_configurable: public configurable {
public:
    fzdsh_configurable(formalizer_standard_program & fsp): configurable("fzdashboard", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    //std::string example_par;   ///< example of configurable parameter
    std::string top_path = ".";
};


struct fzdashboard: public formalizer_standard_program {

    fzdsh_configurable config;

    flow_options flowcontrol;

    // Graph_access ga; // to include Graph or Log access support

    fzdashboard();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    /* (uncomment to include access to memory-resident Graph)
    Graph_ptr graph_ptr = nullptr;
    Graph & graph();
    */

};

extern fzdashboard fzdsh;

#endif // __FZDASHBOARD_HPP
