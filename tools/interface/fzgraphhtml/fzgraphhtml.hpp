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
    output_json = 4,
    output_NUM
};

enum special_url_replacements: int {
    fzserverpq_address = 0,
    special_url_NUM
};

class fzgh_configurable: public configurable {
public:
    fzgh_configurable(formalizer_standard_program & fsp): configurable("fzgraphhtml", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    unsigned int num_to_show = 256;   ///< number of Nodes (or other elements) to render
    unsigned int excerpt_length = 80; ///< number of characters to include in excerpts
    bool excerpt_requested = false;   ///< used with -n
    time_t t_max = -1; ///< maximum time for which to render Nodes (or other elements), default set in fzgraphhtml::init_top()
    std::string rendered_out_path = "STDOUT";
    bool embeddable = false;
    output_format outputformat = output_html;
    bool show_still_required = true;
    text_interpretation interpret_text = text_interpretation::raw;
    bool show_current_time = true;
    bool include_daysummary = true;
    bool sort_by_targetdate = false; // Note: Right now, this is used only by Named Node List output.
    int timezone_offset_hours = 0;
    bool tzadjust_day_separators = false;
    bool show_tzadjust = true; // Show effects of @TZADJUST@ in next Nodes lists generated.
    unsigned int max_do_links = 0; // 0 means no maximum.
    bool include_checkboxes = false;
    std::string subtrees_list_name;
};


struct fzgraphhtml: public formalizer_standard_program {

    fzgh_configurable config;

    flow_options flowcontrol;

    std::string node_idstr;

    std::string list_name;

    bool detect_BTF = false;

    bool test_cards = false;

    bool update_shortlist = false;

    Topic_ID topic_id = 0;

    bool add_to_node = false;

    int num_days = 0; // used only as a cache to render in accordance with arguments received

    bool no_javascript = false;

    bool with_repeats = false;

    bool nnl_with_counter = false;

    time_t t_last_rendered = 0;

    unsigned int do_links_rendered = 0;

    float req_suggested = 0.0;

    time_t td_suggested = RTt_unspecified;

    td_property init_tdprop = _tdprop_num;

    std::string data; // optional pre-seeded data for a new Node (URI encoded)

    Graph * graph_ptr = nullptr;

    std::vector<std::string> replacements;

    bool cache_it = false;
    std::string cache_str;

    // Graph_access ga; // to include Graph or Log access support

    fzgraphhtml();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    Graph & graph();

    std::string output_prep_text(const std::string& text, bool do_excerpt = false);

};

extern fzgraphhtml fzgh;

#endif // __FZGRAPHHTML_HPP
