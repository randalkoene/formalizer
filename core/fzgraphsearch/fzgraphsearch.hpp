// Copyright 20210224 Randal A. Koene
// License TBD

/**
 * {{ header_description }}
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZGRAPHSEARCH_HPP.
 */

#ifndef __FZGRAPHSEARCH_HPP
#include "version.hpp"
#define __FZGRAPHSEARCH_HPP (__VERSION_HPP)

// core
#include "config.hpp"
#include "standard.hpp"
// #include "Graphaccess.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0, /// no recognized request
    flow_find_nodes = 1,     /// request: find Nodes that match the filter
    flow_NUMoptions
};


class fzgs_configurable: public configurable {
public:
    fzgs_configurable(formalizer_standard_program & fsp): configurable("fzgraphsearch", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    //std::string example_par;   ///< example of configurable parameter
};


struct fzgraphsearch: public formalizer_standard_program {

    fzgs_configurable config;

    flow_options flowcontrol;

    // Graph_access ga; // to include Graph or Log access support

    Node_Filter nodefilter;
    std::string listname;
    Boolean_Tag_Flags::boolean_flag btf = Boolean_Tag_Flags::none;
    std::string btf_nnl;

    Graph_ptr graph_ptr = nullptr;

    fzgraphsearch();

    virtual void usage_hook();

    bool parse_tdproperty_binary_pattern(const std::string & cargs);

    bool get_superiors_specification(const std::string & cargs);

    bool get_subtree(const std::string & cargs);

    bool get_nnltree(const std::string & cargs);

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    Graph & graph();

};

extern fzgraphsearch fzgs;

#endif // __FZGRAPHSEARCH_HPP
