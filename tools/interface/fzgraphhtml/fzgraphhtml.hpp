// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ header_description }}
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZGRAPHHTML_HPP.
 */

#ifndef __FZGRAPHHTML_HPP
#include "version.hpp"
#define __FZGRAPHHTML_HPP (__VERSION_HPP)

// core
#include "config.hpp"
#include "standard.hpp"
#include "Graphtypes.hpp"
// #include "Graphaccess.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0,    ///< no recognized request
    flow_node = 1,       ///< request: show data for Node
    flow_incomplete = 2, ///< request: show incomplete Nodes by effective targetdate
    flow_incomplete_with_repeats = 3, ///< request: show incomplete Nodes by effective targetdate with repeats
    flow_named_list = 4, ///< request: show Nodes in Named Node List
    flow_NUMoptions
};

enum output_format {
    output_html = 0,
    output_txt = 1,
    output_node = 2,
    output_desc = 3,
    output_NUM
};

class fzgh_configurable: public configurable {
public:
    fzgh_configurable(formalizer_standard_program & fsp): configurable("fzgraphhtml", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    unsigned int num_to_show = 256;   ///< number of Nodes (or other elements) to render
    unsigned int excerpt_length = 80; ///< number of characters to include in excerpts
    time_t t_max = -1; ///< maximum time for which to render Nodes (or other elements), default set in fzgraphhtml::init_top()
    std::string rendered_out_path = "STDOUT";
    bool embeddable = false;
    output_format outputformat = output_html;
};


struct fzgraphhtml: public formalizer_standard_program {

    fzgh_configurable config;

    flow_options flowcontrol;

    std::string node_idstr;

    std::string list_name;

    bool test_cards = false;

    bool update_shortlist = false;

    Graph * graph_ptr = nullptr;

    // Graph_access ga; // to include Graph or Log access support

    fzgraphhtml();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    Graph & graph();

};

extern fzgraphhtml fzgh;

#endif // __FZGRAPHHTML_HPP
