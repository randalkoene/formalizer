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
#include "Graphinfo.hpp"
#include "Graphmodify.hpp"
// #include "Graphaccess.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0, /// no recognized request
    flow_find_nodes = 1,     /// request: find Nodes that match the filter
    flow_NUMoptions
};

enum excerpt_options: int {
    oldest = 0,         /// N oldest matching Nodes
    newest = 1,         /// N newest matching Nodes
    nearest = 2,        /// N matching Nodes with nearest target dates
    furthest = 3,       /// N matching Nodes with furthest target dates
    NUMexcerpt_options
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
    size_t excerpt_size = 0; // 0 means N is unlimited
    excerpt_options excerpt_counting_by = oldest;

    Map_of_Subtrees map_of_subtrees;
    Graph_modifications * graphmod_ptr = nullptr;
    size_t matched_count = 0;

    Graph_ptr graph_ptr = nullptr;

    fzgraphsearch();

    virtual void usage_hook();

    bool parse_tdproperty_binary_pattern(const std::string & cargs);

    bool get_superiors_specification(const std::string & cargs);

    bool get_subtree(const std::string & cargs);

    bool get_nnltree(const std::string & cargs);

    bool get_excerpt_specs(const std::string & cargs);

    void conditional_result_add(const Node* match_ptr);

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    Graph & graph();

};

extern fzgraphsearch fzgs;

#endif // __FZGRAPHSEARCH_HPP
