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
#include "Graphmodify.hpp"
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
    flow_delete_list = 6, /// request: delete Named Node List
    flow_port_api = 7,    /// request: send API string to server port
    flow_NUMoptions
};

enum NNL_after_use {
    nnl_delete = 0, /// delete Named Node Lists after using them
    nnl_keep = 1, /// keep Named Node Lists after using them
    nnl_ask = 2, /// ask whether to delete or keep Named Node Lists after using them
    nnl_NUMoptions
};

typedef std::vector<Node_ID_key> Node_ID_key_Vector;

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
    NNL_after_use supdep_after_use = nnl_delete;
    std::string listname;        ///< Default Named Node List (e.g. "superiors").
};

class fzgraphedit: public formalizer_standard_program {
protected:
    Graph * graph_ptr;

public:
    fzge_configurable config;

    flow_options flowcontrol;

    bool supdep_from_cmdline;
    bool nnl_supdep_used;

    bool update_shortlist = false;

    std::string api_string;
    std::string outfile;
    bool minimum_API_output = false;

    // Graph_access ga; // to include Graph or Log access support

    fzgraphedit();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    Graph * get_Graph() { return graphmemman.get_Graph(graph_ptr); } ///< Get a pointer to the resident-memory Graph (or from cache in graph_ptr).

};

extern fzgraphedit fzge;

#endif // __FZGRAPHEDIT_HPP
