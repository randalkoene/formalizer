// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ header_description }}
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZGRAPHEDIT_HPP.
 */

#ifndef __FZGRAPHEDIT_HPP
#include "version.hpp"
#define __FZGRAPHEDIT_HPP (__VERSION_HPP)

// std
#include <vector>

// core
#include "config.hpp"
#include "standard.hpp"
#include "Graphbase.hpp"
// #include "Graphaccess.hpp"

using namespace fz;

// Forward declarations
#ifndef __GRAPHBASE_HPP
class Graph;
#endif

enum flow_options {
    flow_unknown = 0,   /// no recognized request
    flow_make_node = 1, /// request: make new Node
    flow_NUMoptions
};

typedef std::vector<const Node_ID_key> Node_ID_key_Vector;

class fzge_configurable: public configurable {
public:
    fzge_configurable(formalizer_standard_program & fsp): configurable("fzgraphedit", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    std::string content_file; ///< Optional file path for description text content.
    Graphdecimal hours = 0.0;
    Graphdecimal valuation = 0.0;
    Node_ID_key_Vector superiors;
    Node_ID_key_Vector dependencies;
    std::vector<std::string> topics;
    time_t targetdate;
    td_property tdproperty;
    td_pattern tdpattern;
    Graphsigned tdevery;
    Graphsigned tdspan;
};

class fzgraphedit: public formalizer_standard_program {
protected:
    Graph * graph_ptr;

public:
    fzge_configurable config;

    flow_options flowcontrol;

    std::string utf8_text;

    // Graph_access ga; // to include Graph or Log access support

    fzgraphedit();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    Graph * get_Graph();

};

extern fzgraphedit fzge;

#endif // __FZGRAPHEDIT_HPP
