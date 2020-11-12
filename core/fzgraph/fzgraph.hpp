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
#ifndef __GRAPHMODIFY_HPP
class Graph_modifications;
#endif

enum flow_options {
    flow_unknown = 0,     /// no recognized request
    flow_make_node = 1,   /// request: make new Node
    flow_make_edge = 2,   /// request: make new Edge
    flow_stop_server = 3, /// request: stop the Graph server
    flow_add_to_list = 4, /// request: add Node(s) to Named Node List
    flow_remove_from_list = 5, /// request: remove Node(s) from Named Node List
    flow_delete_list = 6, // request: delete Named Node List
    flow_NUMoptions
};

typedef std::vector<Node_ID_key> Node_ID_key_Vector;

/// Data structure used when building an Add-Node request, initialized to compile-time default values.
struct Node_data {
    std::string utf8_text;
    Graphdecimal hours = 0.0;
    Graphdecimal valuation = 0.0;
    std::vector<std::string> topics;
    time_t targetdate = RTt_unspecified;
    td_property tdproperty = variable;
    td_pattern tdpattern = patt_nonperiodic;
    Graphsigned tdevery = 0;
    Graphsigned tdspan = 1;
};

/// Data structure used when building an Add-Edge request, initialized to compile-time default values.
struct Edge_data {
    Graphdecimal dependency = 0.0;
    Graphdecimal significance = 0.0;
    Graphdecimal importance = 0.0;
    Graphdecimal urgency = 0.0;
    Graphdecimal priority = 0.0;
};

class fzge_configurable : public configurable {
public:
    fzge_configurable(formalizer_standard_program &fsp) : configurable("fzgraph", fsp) {}
    bool set_parameter(const std::string &parlabel, const std::string &parvalue);

    uint16_t port_number = 8090; ///< Default port number at which server is expected.
    std::string content_file;    ///< Optional file path for description text content.
    Node_data nd;                ///< Default values that can be used for Add-Node requets.
    Edge_data ed;                ///< Default values that can be used for Add-Edge requets.
    Node_ID_key_Vector superiors;
    Node_ID_key_Vector dependencies;
    std::string listname;        ///< Default Named Node List (e.g. "superiors").
};

class fzgraphedit: public formalizer_standard_program {
protected:
    Graph * graph_ptr;

public:
    fzge_configurable config;

    flow_options flowcontrol;

    // Graph_access ga; // to include Graph or Log access support

    fzgraphedit();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    Graph * get_Graph() { return graphmemman.get_Graph(graph_ptr); } ///< Get a pointer to the resident-memory Graph.

};

extern fzgraphedit fzge;

#endif // __FZGRAPHEDIT_HPP
