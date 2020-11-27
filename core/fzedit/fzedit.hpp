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
#include "Graphbase.hpp"
// #include "Graphaccess.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0,   /// no recognized request
    flow_edit_node = 1, /// request: edit a node
    flow_edit_edge = 2, /// request: edit an edge
    flow_NUMoptions
};

class fze_configurable: public configurable {
public:
    fze_configurable(formalizer_standard_program & fsp): configurable("fzedit", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    std::string content_file;   ///< a possible default location when text is fetched from file
};


struct fzedit: public formalizer_standard_program {

    fze_configurable config;

    flow_options flowcontrol;

    // Graph_access ga; // to include Graph or Log access support

    Node_data nd;
    Edge_data ed;

    bool edit_text = false; // we need this, because fetching text comes before estimating shared memory segment size
    Edit_flags editflags;
    std::string idstr;

protected:
    std::string segname;
    unsigned long segsize = 0;
    Graph_modifications * graphmod_ptr = nullptr;

public:
    fzedit();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    void prepare_Graphmod_shared_memory(unsigned long _segsize);
    
    Graph_modifications & graphmod();

    std::string get_segname() { return segname; }

};

extern fzedit fze;

#endif // __FZEDIT_HPP
