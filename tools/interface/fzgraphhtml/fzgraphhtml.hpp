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
#include "html.hpp"
// #include "Graphaccess.hpp"

using namespace fz;

constexpr time_t seconds_per_day = 24*60*60;

enum flow_options {
    flow_unknown = 0,                 ///< no recognized request
    flow_node = 1,                    ///< request: show data for Node
    flow_incomplete = 2,              ///< request: show incomplete Nodes by effective targetdate
    flow_incomplete_with_repeats = 3, ///< request: show incomplete Nodes by effective targetdate with repeats
    flow_named_list = 4,              ///< request: show Nodes in Named Node List
    flow_topics = 5,                  ///< request: show Topics
    flow_topic_nodes = 6,             ///< request: show Nodes with Topic
    flow_node_edit = 7,               ///< request: editing form for Node
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
    bool show_still_required = true;
    text_interpretation interpret_text = text_interpretation::raw;
    bool show_current_time = true;
    bool include_daysummary = true;
};


struct fzgraphhtml: public formalizer_standard_program {

    fzgh_configurable config;

    flow_options flowcontrol;

    std::string node_idstr;

    std::string list_name;

    bool test_cards = false;

    bool update_shortlist = false;

    Topic_ID topic_id = 0;

    bool add_to_node = false;

    int num_days = 0; // used only as a cache to render in accordance with arguments received

    time_t t_last_rendered = 0;

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
