// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ header_description }}
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZADDNODE_HPP.
 */

#ifndef __FZADDNODE_HPP
#include "version.hpp"
#define __FZADDNODE_HPP (__VERSION_HPP)

// core
#include "config.hpp"
#include "standard.hpp"
// #include "Graphaccess.hpp"

using namespace fz;

// Forward declarations
#ifndef __GRAPHTYPES_HPP
class Graph;
#endif

enum flow_options {
    flow_unknown = 0,   /// no recognized request
    flow_make_node = 1, /// request: make new Node
    flow_NUMoptions
};

class fzan_configurable: public configurable {
public:
    fzan_configurable(formalizer_standard_program & fsp): configurable("fzaddnode", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    //std::string example_par;   ///< example of configurable parameter
};


class fzaddnode: public formalizer_standard_program {
protected:
    Graph * graph_ptr;

public:
    fzan_configurable config;

    flow_options flowcontrol;


    // Graph_access ga; // to include Graph or Log access support

    fzaddnode();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    Graph * get_Graph();

};

extern fzaddnode fzan;

#endif // __FZADDNODE_HPP
