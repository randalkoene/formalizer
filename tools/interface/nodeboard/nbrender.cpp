// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Node Board rendering functions.
 * 
 * For more about this, see the Trello card at https://trello.com/c/w2XnEQcc
 */

#include <cmath>
//#include <iostream>
#include <stdexcept>

#include "error.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "html.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "templater.hpp"
#include "nbrender.hpp"
#include "nbgrid.hpp"
#include "jsonlite.hpp"

/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif


using namespace fz;


const std::vector<std::string> template_ids = {
    "Node_board",
    "Node_card",
    "Kanban_board",
    "Kanban_column",
    "Kanban_alt_column",
    "Kanban_alt_board",
    "Node_alt_card",
    "Schedule_card",
    "Schedule_board",
    "Schedule_column",
    "Node_analysis_card",
    "Options_pane"
};

std::string template_path_from_id(template_id_enum template_id) {
    return template_dir+"/"+template_ids[template_id]+".template.html";
}

bool load_templates(nodeboard_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_path_from_id(static_cast<template_id_enum>(i)), templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

nodeboard::nodeboard():
    formalizer_standard_program(false),
    ga(*this, add_option_args, add_usage_top),
    flowcontrol(flow_unknown),
    board_title("Kanban Board"),
    output_path("/var/www/html/formalizer/test_node_card.html"),
    graph_ptr(nullptr) {

    // Still available: ajkxyYz
    add_option_args += "RGgn:L:D:l:t:m:f:c:IZUi:F:u:H:TPO:e:b:M:Kp:B:N:C:r:S:Xo:z:w:";
    add_usage_top += " [-R] [-G|-g] [-n <node-ID>] [-L <name>] [-D <name>] [-l {<name>,...}] [-t {<topic>,...}]"
        " [-m {<topic>,NNL:<name>,...}] [-f <json-path>] [-c <csv-path>] [-I] [-Z] [-U] [-i <topic_id>,...] [-F <substring>]"
        " [-u <up-to>] [-w <importance-threshold>] [-H <board-header>] [-T] [-P] [-O earlier|later] [-e <errors-list>] [-b <before>] [-M <multiplier>] [-p <progress-state-file>]"
        " [-K] [-S <size-list>] [-B <topic-id>] [-N <near-term-days>] [-C <max-columns>] [-r <max-rows>] [-X] [-z <stretch-factor>] [-o <output-file|STDOUT>]";

    usage_head.push_back("Generate Kanban board representation of Nodes hierarchy.\n");
    usage_tail.push_back(
        "Notes:\n"
        "1. Specifying potential errors to detect in the hierarchy [-e <errors-list>:\n"
        "     tdorder: Highlight (red) if active superior Node(s) have earlier TD.\n"
        "     tdfar: Highlight if Node TD is very far in the future.\n"
        "     tdbad: Highlight if Node TD <= 0.\n"
        );
}

void nodeboard::usage_hook() {
    ga.usage_hook();
    FZOUT(
        "    -G Use grid to show dependencies tree.\n"
        "    -g Use grid to show superiors tree.\n"
        "    -n Node dependencies.\n"
        "    -L Use Nodes data in Named Node List.\n"
        "    -D Dependencies of Nodes in Named Node List.\n" // Used for Threads Board
        "    -l List of Named Node Lists.\n"
        "    -t List of Topics.\n"
        "\n"
        "       If <name> or <topic> contain a ':' then text preceding it is\n"
        "       used as a custom header.\n"
        "\n"
        "    -m List of mixed Topics and NNLs (prepend with 'NNL:').\n"
        "    -f Use categories from sysmet-style JSON file.\n"
        "    -c Generate schedule based on CSV file.\n"
        "    -I Include completed Nodes.\n"
        "       This also includes Nodes with completion values < 0.\n"
        "    -Z Include zero required time Nodes.\n"
        "    -U Exclude Nodes with the 'repeated' property (in Threads).\n"
        "    -i Filter to show only Nodes in one of the listed topics (by ID)\n"
        "    -F Filter to show only Nodes where the first 80 characters contain the\n"
        "       substring.\n"
        "    -u In -D mode, show only Nodes with target dates up to including <up-to>.\n"
        "    -w Show only Nodes with maximum importance connection above threshold.\n"
        "    -H Board header.\n"
        "    -T Threads.\n" // Used for Threads Board
        "    -P Progress analaysis.\n"
        "    -O Propose target date order error solutions. Use a philosophy that\n"
        "       prefers to push Nodes to 'earlier' or 'later' target dates.\n"
        "    -b Before time stamp, used with -P.\n"
        "    -M Vertical length multiplier.\n"
        "    -p Progress state file\n"
        "    -K Sort by subtree times.\n"
        "    -e Detect and visualize errors specified in comma separated list.\n"
        "       List can contain: tdorder,  tdfar, tdbad\n"
        "    -B Background highlight Nodes in <topic-id> with elevated valuation\n"
        "       highlight colors.\n"
        "    -N Highlight Nodes with 'near-term' target dates, within <near-term-days>\n"
        "       of present time.\n"
        "    -S List of grid and card sizes:\n"
        "       '<grid-column-width>,<column-container-width>,<card-width>,<card-height>'\n"
        "       E.g. '260px,250px,240px,240px'\n"
        "    -C Max number of columns (default: 100), only in -G/-g modes.\n"
        "    -r Max number of rows (default: 100), in -G/-g or -D modes.\n"
        "    -X Fully expanded grid (do not minimize rows and columns)\n"
        "    -z Place on timeline with specified stretch factor\n"
        "    -o Output to file (or STDOUT).\n"
        "       Default: /var/www/html/formalizer/test_node_card.html\n"
        "    -R Development test.\n"
        "\n"
    );
}

bool nodeboard::set_grid_and_card_sizes(const std::string & cargs) {
    auto sizes_vec = split(cargs, ',');
    if (sizes_vec.size()<4) return false;
    grid_column_width = ' '+sizes_vec[0];
    column_container_width = sizes_vec[1];
    card_width = sizes_vec[2];
    card_height = sizes_vec[3];
    return true;
}

bool nodeboard::parse_error_detection_list(const std::string & cargs) {
    auto list_vec = split(cargs, ',');
    for (auto & errspec : list_vec) {
        std::string & trimmed = trim(errspec);
        if (trimmed == "tdorder") {
            detect_tdorder = true;
        } else if (trimmed == "tdfar") {
            detect_tdfar = true;
        } else if (trimmed == "tdbad") {
            detect_tdbad = true;
        } else {
            return standard_error("Unrecognized error to detect: "+trimmed, __func__);
        }
    }
    return true;
}

bool nodeboard::options_hook(char c, std::string cargs) {

    if (ga.options_hook(c,cargs))
        return true;

    
    switch (c) {
    
        case 'n': {
            flowcontrol = flow_node;
            node_ptr = graph().Node_by_id(cargs);
            return true;
        }

        case 'R': {
            // This has been repurposed to be a general flag for running development
            // test code.
            //flowcontrol = flow_random_test;
            do_development_test = true;
            return true;
        }

        case 'G': {
            show_dependencies_tree = true;
            return true;
        }

        case 'g': {
            show_superiors_tree = true;
            return true;
        }

        case 'L': {
            flowcontrol = flow_named_list;
            list_name = cargs;
            return true;
        }

        case 'D': {
            flowcontrol = flow_NNL_dependencies; // Used for Threads Board
            list_name = cargs;
            return true;
        }

        case 'l': {
            flowcontrol = flow_listof_NNL;
            return parse_list_names(cargs);
        }

        case 't': {
            flowcontrol = flow_listof_topics;
            return parse_list_names(cargs);
        }

        case 'm': {
            flowcontrol = flow_listof_mixed;
            return parse_list_names(cargs);
        }

        case 'f': {
            flowcontrol = flow_sysmet_categories;
            source_file = cargs;
            return true;
        }

        case 'c': {
            flowcontrol = flow_csv_schedule;
            source_file = cargs;
            return true;
        }

        case 'I': {
            show_completed = true;
            return true;
        }

        case 'Z': {
            show_zero_required = true;
            return true;
        }

        case 'U': {
            norepeated = true;
            return true;
        }

        case 'i': {
            uri_encoded_filter_topics = uri_encode(cargs);
            return parse_filter_topics(cargs);
        }

        case 'F': {
            filter_substring = cargs;
            uri_encoded_filter_substring = uri_encode(filter_substring);
            return true;
        }

        case 'u': {
            nnl_deps_to_tdate = ymd_stamp_time(cargs);
            return true;
        }

        case 'w': {
            importance_threshold = atof(cargs.c_str());
            return true;
        }

        case 'H': {
            board_title_specified = true;
            board_title = cargs;
            return true;
        }

        case 'T': {
            threads = true; // Used for Threads Board
            return true;
        }

        case 'e': {
            return parse_error_detection_list(cargs);
        }

        case 'P': {
            progress_analysis = true;
            return true;
        }

        case 'O': {
            propose_td_solutions = true;
            if (cargs=="later") {
                prefer_earlier = false;
            }
            return true;
        }

        case 'b': {
            t_before = ymd_stamp_time(cargs);
            return true;
        }

        case 'M': {
            vertical_multiplier = atof(cargs.c_str());
            if (vertical_multiplier < 1.0) {
                vertical_multiplier = 1.0;
            }
            return true;
        }

        case 'K': {
            sort_by_subtree_times = true;
            return true;
        }

        case 'p': {
            progress_state_file = cargs;
            return true;
        }

        case 'S': {
            return set_grid_and_card_sizes(cargs);
        }

        case 'B': {
            highlight_topic_and_valuation = true;
            highlight_topic = atoi(cargs.c_str());
            return true;
        }

        case 'N': {
            float days_near_highlight = atof(cargs.c_str());
            seconds_near_highlight = long(days_near_highlight*86400.0);
            return true;
        }

        case 'C': {
            max_columns = atoi(cargs.c_str());
            return true;
        }

        case 'r': {
            max_rows = atoi(cargs.c_str());
            nnl_deps_apply_maxrows = true; // Used in NNL dependencies mode.
            return true;
        }

        case 'X': {
            minimize_grid = false;
            return true;
        }

        case 'z': {
            timeline = true;
            timeline_stretch = atof(cargs.c_str());
            if (timeline_stretch <= 0.0) timeline_stretch = 1.0;
            return true;
        }

        case 'o': {
            output_path = cargs;
            return true;
        }

    }

    return false;
}

bool nodeboard::parse_list_names(const std::string & arg) {
    for (unsigned int i=0; i<arg.size(); i++) {
        if (arg[i]=='{') {
            for (unsigned int j=arg.size()-1; j>i; j--) {
                if (arg[j]=='}') {
                    i++;
                    auto v = split(arg.substr(i, j-i), ',');
                    for (auto & el : v) {
                        list_names_vec.emplace_back(trim(el));
                    }
                    return list_names_vec.size()>0;
                }
            }
            return false;
        }
    }
    return false;
}

bool nodeboard::parse_filter_topics(const std::string & arg) {
    auto v = split(arg, ',');
    for (auto & el : v) {
        filter_topics.emplace_back(atoi(el.c_str()));
    }
    return filter_topics.size()>0;
}

bool nodeboard::parse_header_identifier(const std::string & arg, std::string & header, std::string & identifier) {
    auto seppos = arg.find_first_of(':');
    if (seppos==std::string::npos) {
        header = arg;
        identifier = arg;
    } else {
        header = arg.substr(0, seppos);
        identifier = arg.substr(seppos+1);
    }
    return true;
}

CSV_Data::CSV_Data(Graph & graph, const std::string & csv_line) {
    std::vector<std::string> csv_elements = split(csv_line, ',');
    if (csv_elements.size() < 4) {
        node_ptr = nullptr;
        return;
    }

    std::vector<std::string> start_components = split(csv_elements[0], ' ');
    if (start_components.size() < 2) {
        node_ptr = nullptr;
        return;
    }

    start_date = start_components[0];
    start_time = start_components[1];
    num_minutes = atoi(csv_elements[1].c_str());
    tdprop = csv_elements[2][0];
    node_ptr = graph.Node_by_idstr(csv_elements[3]);
}

bool nodeboard::parse_csv(const std::string & csv_data) {
    std::vector<std::string> csv_lines = split(csv_data, '\n');
    if (csv_lines.size() < 2) {
        standard_error("Missing data lines in CSV file.", __func__);
        return false;
    }

    std::string current_day;
    for (unsigned int i = 1; i < csv_lines.size(); i++) {
        CSV_Data data(graph(), csv_lines[i]);
        if (!data.node_ptr) {
            continue;
        }
        if (data.start_date != current_day) {
            current_day = data.start_date;
            //FZOUT("\nTESTING --> New day: "+data.start_date+'\n');
            csv_data_vec.emplace_back(CSV_Data_Day());
        }
        csv_data_vec.back().day.emplace_back(data);
        //FZOUT("+");
    }
    //FZOUT("\n");
    return true;
}

Graph & nodeboard::graph() { 
    if (graph_ptr) {
        return *graph_ptr;
    }
    graph_ptr = graphmemman.find_Graph_in_shared_memory();
    if (!graph_ptr) {
        standard_exit_error(exit_general_error, "Memory resident Graph not found.", __func__);
    }
    return *graph_ptr;
}

nodeboard_options nodeboard::get_nodeboard_options() const {
    nodeboard_options options;
    options._floption = flowcontrol;
    options._threads = threads;
    options._showcompleted = show_completed;
    options._importancethreshold = importance_threshold;
    options._shownonmilestone = shows_non_milestone_nodes();
    options._progressanalysis = progress_analysis;
    options.multiplier = vertical_multiplier;
    options.maxcols = max_columns;
    options.maxrows = max_rows;
    options.seconds_near_highlight = seconds_near_highlight;
    options.do_development_test = do_development_test;
    options.propose_td_solutions = propose_td_solutions;
    options.norepeated = norepeated;
    return options;
}

/*
*** TODO: Implement a templated build of a pane that allows on-page
          selection of the details of a nodeboard. This pane can be
          popped up when some component is hovered over.
          To do this:

          1. Create a template for the pane, start with just a few bits
             to test getting the approach right.
          2. Use the template-filling-from-map method shown in
             fzloghtml:render.cpp:chunk_to_template().
          3. Use the information in build_nodeboard_cgi_call() to
             fill the components of the template.
          4. Add the rest of the variables.
*/

const std::map<bool, std::string> sup_or_dep = {
    { false, "G" },
    { true, "g"},
};

const std::map<bool, std::string> true_checked = {
    { false, "" },
    { true, "checked" },
};

bool nodeboard::make_options_pane(std::string & rendered_pane) {
    std::map<std::string, std::string> pane_map = {
        { "node-id", node_ptr->get_id_str() },
        { "sup-or-dep", sup_or_dep.at(flowcontrol == flow_superiors_tree) },
        { "show-completed", true_checked.at(show_completed) },
        { "importance-threshold", to_precision_string(importance_threshold, 2) },
        { "threads", true_checked.at(threads) },
        { "progress-analysis", true_checked.at(progress_analysis) },
        { "hide-repeated", true_checked.at(norepeated) },
        { "detect-tdorder", true_checked.at(detect_tdorder) },
        { "detect-tdfar", true_checked.at(detect_tdfar) },
        { "detect-tdbad", true_checked.at(detect_tdbad) },
        { "timeline", true_checked.at(timeline) },
        { "timeline-stretch", to_precision_string(timeline_stretch, 2) },
        { "highlighted-topic", "" },
        { "max-rows", std::to_string(max_rows) },
        { "max-columns", std::to_string(max_columns) },
        { "days-nearterm", "" },
        { "none-checked", "" },
        { "earlier-checked", "" },
        { "later-checked", "" },
        { "filter", "" },
        { "topic-filters", "" },
    };

    if (highlight_topic_and_valuation) {
        pane_map.at("highlighted-topic") = "value=\""+std::to_string(highlight_topic)+'"';
    }

    if (propose_td_solutions) {
        if (prefer_earlier) {
            pane_map.at("earlier-checked") = "checked";
        } else {
            pane_map.at("later-checked") = "checked";
        }
    } else {
        pane_map.at("none-checked") = "checked";
    }

    if (seconds_near_highlight>0) {
        pane_map.at("days-nearterm") = "value=\""+to_precision_string(float(seconds_near_highlight)/86400.0, 1)+'"';
    }

    if (!filter_substring.empty()) {
        pane_map.at("filter") = uri_encoded_filter_substring;
    }

    std::string uri_encoded_modifed_topic_filters = encode_modified_topic_filters(shows_non_milestone_nodes());
    if (!uri_encoded_modifed_topic_filters.empty()) {
        pane_map.at("topic-filters") = uri_encoded_modifed_topic_filters;
    }

    if (!env.fill_template_from_map(
            template_path_from_id(options_pane_temp),
            pane_map,
            rendered_pane)) {
        return false;
    }
    return true;
}

std::string nodeboard::build_nodeboard_cgi_call(const nodeboard_options & options) const {
    std::string cgi_cmd("/cgi-bin/nodeboard-cgi.py?");
    switch (options._floption) {
        case flow_node: {
            if (!node_ptr) {
                return "";
            }
            cgi_cmd += "n="+node_ptr->get_id_str();
            break;    
        }
        case flow_dependencies_tree: {
            if (!node_ptr) {
                return "";
            }
            cgi_cmd += "n="+node_ptr->get_id_str()+"&G=true";
            break;
        }
        case flow_superiors_tree: {
            if (!node_ptr) {
                return "";
            }
            cgi_cmd += "n="+node_ptr->get_id_str()+"&g=true";
            break;
        }
        case flow_NNL_dependencies: {
            cgi_cmd += "D="+list_name;
            break;
        }
        case flow_csv_schedule: {
            std::string multiplier_arg;
            if (options.multiplier > 1.0) {
                multiplier_arg = "&M="+to_precision_string(options.multiplier, 1);
            }
            cgi_cmd += "c="+source_file+"&H=Proposed_Schedule"+multiplier_arg;
            return cgi_cmd;
        }
        default: {
            return "";
        }
    }
    if (options._showcompleted) {
        cgi_cmd += "&I=true";
    }
    if (options._importancethreshold > 0.0) {
        cgi_cmd += "&w="+to_precision_string(options._importancethreshold, 2);
    }
    if (options._threads) {
        cgi_cmd += "&T=true";
    }
    if (options._progressanalysis) {
        cgi_cmd += "&P=true";
    }
    if (highlight_topic_and_valuation) {
        cgi_cmd += "&B="+std::to_string(highlight_topic);
    }
    if (options.maxrows != DEFAULTMAXROWS) {
        cgi_cmd += "&r="+std::to_string(options.maxrows);
    }
    if (options.maxcols != DEFAULTMAXCOLS) {
        cgi_cmd += "&C="+std::to_string(options.maxcols);
    }
    if (options.seconds_near_highlight != 0) {
        cgi_cmd += "&N="+to_precision_string(float(options.seconds_near_highlight)/86400.0, 2); // expressed in days
    }
    if (options.do_development_test) {
        cgi_cmd += "&R=true";
    }
    if (options.propose_td_solutions) {
        if (prefer_earlier) {
            cgi_cmd += "&O=earlier";
        } else {
            cgi_cmd += "&O=later";
        }
    }
    if (options.norepeated) {
        cgi_cmd += "&U=true";
    }
    if (!filter_substring.empty()) {
        cgi_cmd += "&F="+uri_encoded_filter_substring;
    }
    std::string uri_encoded_modifed_topic_filters = encode_modified_topic_filters(options._shownonmilestone);
    if (!uri_encoded_modifed_topic_filters.empty()) {
        cgi_cmd += "&i="+uri_encoded_modifed_topic_filters;
    }
    if (detect_tdorder) {
        cgi_cmd += "&tdorder=true";
    }
    if (detect_tdfar) {
        cgi_cmd += "&tdfar=true";
    }
    if (detect_tdbad) {
        cgi_cmd += "&tdbad=true";
    }
    return cgi_cmd;
}

bool nodeboard::shows_non_milestone_nodes() const {
    for (const auto & _topic : filter_topics) {
        if (_topic == ROADMAP_TOPIC) return false;
    }
    return true;
}

std::string nodeboard::encode_modified_topic_filters(bool _shownonmilestone) const {
    if (shows_non_milestone_nodes()) {
        if (_shownonmilestone) {
            return uri_encoded_filter_topics;
        } else {
            if (filter_topics.size()==0) {
                return std::to_string(ROADMAP_TOPIC);
            } else { // The filter is more selective, and we don't want to forget it so can't switch.
                return uri_encoded_filter_topics;
            }
        }
    } else { // ROADMAP_TOPIC is in the topic filters
        if (_shownonmilestone) {
            if (filter_topics.size()==1) {
                return "";
            } else { // The filter is more selective, can't show more.
                return uri_encoded_filter_topics;
            }
        } else {
            return uri_encoded_filter_topics;
        }
    }
}

bool nodeboard::render_init() {
    return load_templates(templates);
}

bool nodeboard::to_output(const std::string & rendered_board) {
    if (output_path=="STDOUT") {
        FZOUT(rendered_board);
        return true;
    }

    if (!string_to_file(output_path, rendered_board)) {
        ERRRETURNFALSE(__func__,"unable to write rendered board to "+output_path);
    }
    FZOUT("Board rendered to "+output_path+".\n");
    return true;
}

// Returns true if filtered out or the Node text content if not filtered out.
bool nodeboard::filtered_out(const Node * node_ptr, std::string & node_text) const {
    if ((!show_completed) && (!node_ptr->is_active())) {
        return true;
    }

    if ((!show_zero_required) && (node_ptr->get_required() <= 0.0)) {
        return true;
    }

    if (show_dependencies_tree) {
        if ((importance_threshold > 0.0) && (node_ptr->superiors_max_importance() < importance_threshold)) {
            return true;
        }
    }
    if (show_superiors_tree) {
        if ((importance_threshold > 0.0) && (node_ptr->dependencies_max_importance() < importance_threshold)) {
            return true;
        }
    }

    if (!filter_topics.empty()) {
        if (!node_ptr->in_one_of_topics(filter_topics)) {
            return true;
        }
    }

    node_text = node_ptr->get_text();
    if (!filter_substring.empty()) {
        std::string excerpt = node_text.substr(0, filter_substring_excerpt_length);
        if (excerpt.find(filter_substring) == std::string::npos) {
            return true;
        }
    }

    return false;
}

bool nodeboard::get_Node_card(const Node * node_ptr, std::string & rendered_cards) {
    if (!node_ptr) {
        return false;
    }

    std::string node_text;
    if (filtered_out(node_ptr, node_text)) {
        return true;
    }

    // For each node: Set up a map of content to template position IDs.
    template_varvalues nodevars;
    nodevars.emplace("node-id", node_ptr->get_id_str());
    nodevars.emplace("node-deps", node_ptr->get_id_str());
    nodevars.emplace("node-text", node_text);

    std::string include_filter_substr;
    if (!filter_substring.empty()) {
        include_filter_substr = "&F="+uri_encoded_filter_substring;
    }
    if (!filter_topics.empty()) {
        include_filter_substr += "&i="+uri_encoded_filter_topics;
    }
    if (show_completed) {
        include_filter_substr += "&I=true";
    }
    nodevars.emplace("filter-substr", include_filter_substr);

    // For each node: Create a Kanban card and add it to the output HTML.
    rendered_cards += env.render(templates[node_card_temp], nodevars);
    return true;
}

Node_render_result nodeboard::get_Node_alt_card(const Node * node_ptr, std::time_t tdate, std::string & rendered_cards, Node_Subtree * subtree_ptr) {
    if (!node_ptr) {
        return node_render_error;
    }

    std::string node_text;
    if (filtered_out(node_ptr, node_text)) {
        return node_not_rendered;
    }

    // For each node: Set up a map of content to template position IDs.
    template_varvalues nodevars;

    // Highlight Milestones and their valuation.
    std::string card_bg_highlight;
    if (highlight_topic_and_valuation) {
        if (node_ptr->in_topic(highlight_topic)) {
            if (node_ptr->get_valuation() > 3.0) { // elevated highlight
                card_bg_highlight = "w3-orange";
            } else { // highlight
                card_bg_highlight = "w3-green";
            }
        }
    }
    nodevars.emplace("nodebg-color", card_bg_highlight);

    // Show if a Node is inactive, active exact/fixed, or active VTD.
    std::string node_color;
    bool text_is_dark = true;
    bool highlight_targetdate = false;
    if (node_ptr->is_active()) {
        bool tderror = false;
        if (detect_tdfar) {
            if FAR_TD(tdate,t_now) { // Target dates more than a hundred years in the future are suspect.
                tderror = true;
            }
        }
        if (detect_tdbad) {
            if BAD_TD(tdate) {
                tderror = true;
            }
        }
        if (detect_tdorder) {
            if (const_cast<Node*>(node_ptr)->td_suspect_by_superiors()) {
                tderror = true;
            }
        }
        if (tderror) {
            node_color = "w3-red";
            text_is_dark = false;
        } else {
            if ((seconds_near_highlight > 0) && (tdate <= (t_now + seconds_near_highlight))) { // within near-term threshold
                highlight_targetdate = true;
            }
            if (node_ptr->td_fixed() || node_ptr->td_exact()) {
                node_color = "w3-aqua";
            } else {
                node_color = "w3-light-grey";
            }
        }
    } else {
        node_color = "w3-dark-grey";
    }
    nodevars.emplace("node-color", node_color);

    std::string node_id_str = node_ptr->get_id_str();
    nodevars.emplace("node-id", node_id_str);

    nodevars.emplace("fzserverpq", graph().get_server_full_address());

    // Show a hint that this Node may  have some history worth seeing.
    if (node_ptr->probably_has_Log_history()) {
        if (text_is_dark) {
            nodevars.emplace("nodelink-bg-color", "class=\"link-bg-color-yellow\"");
        } else {
            nodevars.emplace("nodelink-bg-color", "class=\"link-bg-color-blue\"");
        }
    } else {
        nodevars.emplace("nodelink-bg-color", "");
    }
    nodevars.emplace("node-id-history", node_id_str+"&alt=histfull");
    float completion = node_ptr->get_completion();
    float progress = completion*100.0;
    if (progress < 0.0) progress = 100.0;
    if (progress > 100.0) progress = 100.0;
    nodevars.emplace("node-progress", to_precision_string(progress, 1));

    if (highlight_targetdate) {
        nodevars.emplace("td-bg-color", "class=\"td-bg-color-green\"");
    } else {
        nodevars.emplace("td-bg-color", "");
    }
    nodevars.emplace("node-targetdate", " ("+DateStampYmd(tdate)+')');

    nodevars.emplace("node-deps", node_id_str);
    size_t depnum = node_ptr->num_dependencies();
    if (depnum<1) {
        nodevars.emplace("dep-num", "");
    } else {
        nodevars.emplace("dep-num", ",<b>"+std::to_string(node_ptr->num_active_dependencies())+'/'+std::to_string(depnum)+"</b>");
    }

    std::string include_filter_substr;
    if (!filter_substring.empty()) {
        include_filter_substr = "&F="+uri_encoded_filter_substring;
    }
    if (!filter_topics.empty()) {
        include_filter_substr += "&i="+uri_encoded_filter_topics;
    }
    if (show_completed) {
        include_filter_substr += "&I=true";
    }
    if (importance_threshold > 0.0) {
        include_filter_substr += "&w="+to_precision_string(importance_threshold, 2);
    }
    nodevars.emplace("filter-substr", include_filter_substr);

    std::string prereqs_str;
    std::string hours_applied_str;
    if (threads) {
        // Look for specified prerequisites and their state.
        auto prereqs = get_prerequisites(*node_ptr, true);
        if (!prereqs.empty()) {
            unsigned int num_unsolved = 0;
            unsigned int num_unfulfilled = 0;
            unsigned int num_fulfilled = 0;
            for (const auto & prereq : prereqs) {
                if (prereq.state()==unsolved) {
                    num_unsolved++;
                } else if (prereq.state()==unfulfilled) {
                    num_unfulfilled++;
                } else {
                    num_fulfilled++;
                }
            }
            if ((num_unsolved > 0) || (num_unfulfilled > 0)) {
                prereqs_str += "<p>";
                if (num_unsolved > 0) {
                    prereqs_str += "<span style=\"color:red;\">"+std::to_string(num_unsolved)+" unsolved</span> ";
                }
                if (num_unfulfilled > 0) {
                    prereqs_str += std::to_string(num_unfulfilled)+" unfulfilled ";
                }
                prereqs_str += "prerequisites</p>";
            }

            if (subtree_ptr) {
                subtree_ptr->prerequisites_with_solving_nodes += num_fulfilled + num_unfulfilled;
                subtree_ptr->unsolved_prerequisites += num_unsolved;
            }
        }

        if (progress_analysis && (!node_ptr->is_special_code())) {
            float hours_applied;
            float hours_required = node_ptr->get_required_hours();
            if (!node_ptr->get_repeats()) {
                // Determine actual time applied to this Node from its Log history.
                auto minutes_applied = get_Node_total_minutes_applied(node_ptr->get_id().key());
                hours_applied = float(minutes_applied)/60.0;
                hours_applied_str = "<p>hours applied: "+to_precision_string(hours_applied, 2);

                // If possible, propose a corrected required time.
                if (minutes_applied > 0) {
                    // Also, detect incorrect completion==0.
                    if (completion==0.0) {
                        hours_applied_str += " <span style=\"color:red;\"><b>Erroneous Zero Completion</b></span>";
                    } else {
                        long minutes_required = node_ptr->get_required_minutes();
                        if (completion > 1.0) {
                            minutes_required = round(float(minutes_required)*completion);
                            completion = 1.0;
                        }
                        long proposed_minutes_required = round(float(minutes_applied) / completion);
                        if (proposed_minutes_required != minutes_required) {
                            hours_required = float(proposed_minutes_required)/60.0;
                            hours_applied_str += " Proposed corrected required time: <b>"+to_precision_string(hours_required, 2)+" hrs</b>";
                        }
                    }
                }
            } else {
                hours_applied = hours_required*completion;
                hours_applied_str = "<p>hours applied (this instance): "+to_precision_string(hours_applied, 2);
            }
            hours_applied_str += "</p>";

            // Check if more granularity would be advised.
            float hours_remaining = (1.0-completion)*hours_required;
            if (hours_remaining > 2.0) {
                hours_applied_str += "<p style=\"color:red;\">Advise more granularity!</p>";
            }

            if (subtree_ptr) {
                // Gather combined data for subtree.
                subtree_ptr->hours_required += hours_required;
                subtree_ptr->hours_applied += hours_applied;
            }

            if (get_provides_capabilities(*node_ptr).empty()) {
                prereqs_str += "<p style=\"color:coral;\">No provides specified.</p>";
            }
        }

    }
    nodevars.emplace("node-prereqs", prereqs_str);
    nodevars.emplace("node-hrsapplied", hours_applied_str);

    nodevars.emplace("node-text", node_text);

    // Placement on timeline option
    std::string extra_style_str;
    if (timeline) {
        // position relative to the card itself, i.e. relative to min target date in line?
        // or absolute, which is relative to the line container?
        // or fixed, which is relative to the viewport?
        timeline_last_vh = float(tdate - timeline_min_td)/float(RTt_oneday);
        timeline_last_vh *= timeline_stretch;
        if (timeline_last_vh<0.0) timeline_last_vh = 0.0;
        extra_style_str = " style=\"top:"+to_precision_string(timeline_last_vh, 2)+"vh;position:absolute;\"";
    }
    nodevars.emplace("extra-style", extra_style_str);

    // For each node: Create a Kanban card and add it to the output HTML.
    if (progress_analysis) {
        rendered_cards += env.render(templates[node_analysis_card_temp], nodevars);
    } else {
        rendered_cards += env.render(templates[node_alt_card_temp], nodevars);
    }
    if (node_ptr->is_active()) {
        return node_rendered_active;
    }
    return node_rendered_inactive;
}

/**
 * vh = 1% of viewport height
 * Try to use 5/6 of height for the calendar, i.e. about 84vh.
 * A day contains 288 blocks of 5 minutes each, or 72 of 20 minutes each (each 84/72=1.17vh or about 1.2vh), or 48 (each 1.75vh) of 30 minutes each (each 2.8vh).
 * There are 1440 minutes in a day.
 * Finding the nearest 20 minute block: trunc((start_minute/1440)*72)
 * Finding the position of a 20 minute-chunked start: trunc( 1.17*(start_minute/1440)*72)
 */
float nodeboard::get_card_vertical_position(const std::string & start_time, float v_offset) {
    int hour = atoi(start_time.substr(0, 2).c_str());
    int minute = atoi(start_time.substr(3, 2).c_str());
    float start_minute = (hour*60) + minute;
    return 1.17*(start_minute/1440.0)*72.0*vertical_multiplier + v_offset;
}

/**
 * Each minute is 1/1440 of the day, thus, using the same vertical calculation as above,
 * each minute has a height of 84/1440=0.0583vh.
 */
float nodeboard::get_card_height(unsigned int minutes) {
    return (float(minutes))*0.0583*vertical_multiplier;
}

// *** TESTING: Explicit passed-time overlay.
bool nodeboard::get_Passed_Time_overlay(unsigned int passed_minutes, std::string & rendered_cards) {
    std::string vpos_daystart = to_precision_string(get_card_vertical_position("00:00"), 2);
    std::string vheight_passed = to_precision_string(get_card_height(passed_minutes));
    std::string overlay = "<div class=\"passed_overlay\" style=\"top:"+vpos_daystart+"vh;height:"+vheight_passed+"vh;\"></div>\n";
    rendered_cards += overlay;
    return true;
}

const std::map<char, std::string> schedule_entry_color = {
    {'e', "#ffc0cb"}, // pink
    {'E', "#dda0dd"}, // exact with repeats, plum
    {'f', "#98fb98"}, // pale-green
    {'F', "#66cdaa"}, // fixed with repeats, mediumacquamarine
    {'v', "#f0e68c"}, // khaki
    {'u', "#d3d3d3"}, // light-grey
    {'i', "#c4f0a2"}, // mixed khaki and pale-green using https://www.w3schools.com/colors/colors_mixer.asp
};

// *** NOTE: Hard-coded for the moment. Should come from a config file or command line argument.
const std::string sleepNNL = "group_sleep";

std::string nodeboard::get_schedule_card_color(const CSV_Data & entry_data) {
    if (sleepNNL_ptr->contains(entry_data.node_ptr->get_id().key())) return "#339afc";

    if (entry_data.node_ptr->get_repeats()) {
        if (entry_data.tdprop == 'e') {
            return schedule_entry_color.at('E');
        }
        return schedule_entry_color.at('F');
    }

    return schedule_entry_color.at(entry_data.tdprop);
}

bool nodeboard::get_Schedule_card(const CSV_Data & entry_data, std::string & rendered_cards) {
    if (!entry_data.node_ptr) {
        return false;
    }

    std::string node_text = entry_data.node_ptr->get_text();
    std::string excerpt = remove_html_tags(node_text).substr(0, 80);

    // For each node: Set up a map of content to template position IDs.
    template_varvalues nodevars;
    nodevars.emplace("node-id", entry_data.node_ptr->get_id_str());
    nodevars.emplace("node-color", get_schedule_card_color(entry_data));
    nodevars.emplace("node-text", excerpt);
    nodevars.emplace("start-time", entry_data.start_time);
    nodevars.emplace("minutes", std::to_string(entry_data.num_minutes));
    nodevars.emplace("vert-pos", to_precision_string(get_card_vertical_position(entry_data.start_time), 2));
    nodevars.emplace("node-height", to_precision_string(get_card_height(entry_data.num_minutes)));
    nodevars.emplace("fzserverpq", graph().get_server_full_address());

    // For each node: Create a Kanban card and add it to the output HTML.
    rendered_cards += env.render(templates[schedule_card_temp], nodevars);
    return true;
}

bool nodeboard::get_column(const std::string & column_header, const std::string & rendered_cards, std::string & rendered_columns, const std::string extra_header = "", template_id_enum column_template = kanban_column_temp) {
    // For each column: Set up a map of content to template position IDs.
    template_varvalues column;
    column.emplace("column-id", column_header);
    column.emplace("column-extra", extra_header);
    column.emplace("column-cards", rendered_cards);

    // For each node: Create a Kanban card and add it to the output HTML.
    rendered_columns += env.render(templates[column_template], column);
    num_columns++;
    return true;
}

// bool nodeboard::get_alt_column(const std::string & column_header, const std::string & rendered_cards, std::string & rendered_columns, const std::string extra_header = "") {
//     // For each column: Set up a map of content to template position IDs.
//     template_varvalues column;
//     column.emplace("column-id", column_header);
//     column.emplace("column-extra", extra_header);
//     column.emplace("column-cards", rendered_cards);

//     // For each node: Create a Kanban card and add it to the output HTML.
//     rendered_columns += env.render(templates[kanban_alt_column_temp], column);
//     num_columns++;
//     return true;
// }

bool nodeboard::get_dependencies_column(const std::string & column_header, const Node * column_node, std::string & rendered_columns, const std::string extra_header = "") {

    if (!column_node) {
        standard_error("Node not found, skipping", __func__);
        return false;
    }

    std::string rendered_cards;

    for (const auto & edge_ptr : column_node->dep_Edges()) {
        if (edge_ptr) {

            if (!get_Node_card(edge_ptr->get_dep(), rendered_cards)) {
                standard_error("Node "+edge_ptr->get_dep_str()+" not found in Graph, skipping", __func__);
            }

        }
    }

    return get_column(column_header, rendered_cards, rendered_columns, extra_header);
}

/*
When an NNL is converted to a board with dependencies, and when the Threads flag
is set, then requesting progress analysis causes the Log histories of the Nodes
in the Threads to be retrieved in order to obtain the actual time spent on each.

In a Node page, pressing the history button calls fzlink, and if it is the full
history, then that uses fzloghtml with the -t and -n options, so that only
Log chunks belonging to a Node are retrieved, and so that total time applied is
shown.

In fzloghtml, this is done by:

1. fzloghtml fzlh; // Has Log_filter called filter.
2. During constructor of fzlh: fzlh.ga(*fzlh, add_option_args, add_usage_top); // Graph_access
3. fzlh.filter.nkey = Node_ID_key(id_str);
4. fzlh.set_filter(); // Can skip! This does nothing, because nothing set implies full history for Node.
5. log_ptr = ga.request_Log_excerpt(filter);
6. total_minutes_applied = Chunks_total_minutes(edata.log_ptr->get_Chunks());
*/

unsigned long nodeboard::get_Node_total_minutes_applied(const Node_ID_key nkey) {
    filter.nkey = nkey;
    if (t_before > 0) {
        filter.t_to = t_before;
    }
    std::unique_ptr<Log> log_uptr = ga.request_Log_excerpt(filter);
    return Chunks_total_minutes(log_uptr->get_Chunks());
}

void nodeboard::progress_state_update() {
    int percentage;
    if (progress_node_count >= node_total) {
        percentage = 100;
    } else {
        percentage = int(100.0*(float(progress_node_count)/float(node_total)));
    }
    if (last_percentage_state < percentage) {
        last_percentage_state = percentage;
        if (!string_to_file(progress_state_file, std::to_string(percentage))) {
            standard_error("Unable to write progress to "+progress_state_file, __func__);
        }
    }
}

std::string nodeboard::tosup_todep_html_buttons(const Node_ID_key & column_key) {
    std::string column_key_str(column_key.str());
    return "<span><button class=\"tiny_button tiny_green\" onclick=\"window.open('http://"
        + graph().get_server_full_address()+"/fz/graph/namedlists/superiors?add="+column_key_str+"');\">2sup</button>"
           "<button class=\"tiny_button\" onclick=\"window.open('http://"
        + graph().get_server_full_address()+"/fz/graph/namedlists/dependencies?add="+column_key_str+"');\">2dep</button>"
           "<button class=\"tiny_button tiny_green\" onclick=\"window.open('/cgi-bin/fzgraphhtml-cgi.py?edit=new&tosup="
        + column_key_str+"');\">mkwsup</button></span>";
}

struct required_completed {
    float required_s;
    float completed_s;
};

required_completed get_required_completed(const Node& node) {
    required_completed reqcomp;
    float completion = node.get_completion();
    reqcomp.required_s = node.get_required();
    if (completion < 0.0) {
        completion = 1.0;
        reqcomp.required_s = 0.0;
    } else {
        if (completion > 1.0) {
            reqcomp.required_s = reqcomp.required_s*completion;
            completion = 1.0;
        }
    }
    reqcomp.completed_s = reqcomp.required_s*completion;   
    return reqcomp;
}

/**
 * This places the full dependencies tree into a single column according to
 * specified constraints and without duplicating Nodes across different
 * columns.
 * 
 * Notes:
 * 1. Which column a Node is allocated to depends on the specifications
 *    that were given to the collection process of Map_of_Subtrees.
 *    Here, this defaults to using the branch strength propagation method
 *    Node_Branch::minimum_importance and determines the strength of an
 *    Edge based on the 'importance' parameter.
 * 2. The full Map_of_Subtrees is collected for the NNL of header Nodes
 *    before entering this function, and additional constraints are applied
 *    here before actually placing Nodes into the column.
 * 3. This applies pruning if the 'norepeated' flag is on.
 */
bool nodeboard::get_fulldepth_dependencies_column(std::string & column_header, Node_ID_key column_key, Threads_Board_Data & board_data, const std::string extra_header) {
    if (!map_of_subtrees.is_subtree_head(column_key)) {
        standard_error("Node "+column_key.str()+" is not a head Node in NNL '"+map_of_subtrees.subtrees_list_name+"', skipping", __func__);
        return false;
    }

    std::string rendered_cards;

    float tot_required_s = 0;
    float tot_completed_s = 0;
    unsigned int active_rendered = 0;
    unsigned int column_length = 0;
    // Add all dependencies found to the column as cards.
    auto & subtree_set = map_of_subtrees.get_subtree_set(column_key);
    for (const auto & [tdate, depnode_ptr] : subtree_set.tdate_node_pointers) {
        if (!depnode_ptr) {
            standard_error("Dependency Node not found, skipping", __func__);
            continue;
        }
        
        if (norepeated) {
            if (depnode_ptr->get_repeats()) {
                continue;
            }
        }

        if (tdate <= nnl_deps_to_tdate) { // Apply possible target date constraint.
            if ((!nnl_deps_apply_maxrows) || (column_length < max_rows)) { // Apply max rows constraint.
                auto noderenderresult = get_Node_alt_card(depnode_ptr, tdate, rendered_cards, const_cast<Node_Subtree*>(&subtree_set));
                if (noderenderresult==node_render_error) {
                    standard_error("Dependency Node not found in Graph, skipping", __func__);
                } else if (noderenderresult==node_rendered_active) {
                    active_rendered++;
                }
                if (noderenderresult > node_not_rendered) {
                    column_length++;
                }
            }
        }

        // Calculations below are carried out whether the Node is shown or not.
        if (!progress_state_file.empty()) {
            progress_node_count++;
            if (progress_node_count < node_total) { // Do not signal 100% until output is completely provided.
                progress_state_update();
            }
        }

        // Obtain contributions to thread progress.
        auto reqcomp = get_required_completed(*depnode_ptr);
        tot_required_s += reqcomp.required_s;
        tot_completed_s += reqcomp.completed_s;
    }

    float thread_progress = 0;
    if (tot_required_s > 0.0) {
        thread_progress = 100.0 * tot_completed_s / tot_required_s;
    }
    std::string extra_header_with_progress = extra_header + "<br>Progress: " + to_precision_string(thread_progress, 1) + "&#37;";
    extra_header_with_progress += tosup_todep_html_buttons(column_key);

    if (progress_analysis) {
        if (subtree_set.hours_required > 0.0) {
            float corrected_thread_progress = 100.0 * subtree_set.hours_applied / subtree_set.hours_required;
            extra_header_with_progress += "<br>Corrected progress: " + to_precision_string(corrected_thread_progress, 1) + "&#37;";

            if ((subtree_set.unsolved_prerequisites > 0) && (subtree_set.prerequisites_with_solving_nodes > 0)) {
                float unsolved_ratio = float(subtree_set.unsolved_prerequisites)/float(subtree_set.prerequisites_with_solving_nodes);
                float hours_estimate_with_unsolved = (1.0 + unsolved_ratio)*subtree_set.hours_required;
                float estimated_progress_with_unsolved = 100.0 * subtree_set.hours_applied / hours_estimate_with_unsolved;
                extra_header_with_progress += "<br>Estimated progress with missing solutions: " + to_precision_string(estimated_progress_with_unsolved, 1) + "&#37;";
            }
        } else {
            extra_header_with_progress += "<br>(Unable to show corrected progress. Zero hours required.)";
        }
    }

    if (threads && (active_rendered==0)) {
        column_header = "<span style=\"color:red;\">"+column_header+"</span>";
    }

    board_data.invested_hrs.emplace_back(tot_completed_s/3600.0);
    board_data.remaining_hrs.emplace_back((tot_required_s - tot_completed_s)/3600.0);
    board_data.board_tot_required_s += tot_required_s;
    board_data.board_tot_completed_s += tot_completed_s;

    return get_column(column_header, rendered_cards, board_data.rendered_columns, extra_header_with_progress, kanban_alt_column_temp);
}

bool nodeboard::get_NNL_column(const std::string & nnl_str, std::string & rendered_columns) {
    std::string header, identifier;
    parse_header_identifier(nnl_str, header, identifier);
    Named_Node_List_ptr namedlist_ptr = graph().get_List(identifier);
    if (!namedlist_ptr) {
        standard_error("Named Node List "+identifier+" not found, skipping", __func__);
        return false;
    }

    std::string rendered_cards;

    for (const auto & nkey : namedlist_ptr->list) {

        Node * node_ptr = graph().Node_by_id(nkey);
        if (!get_Node_card(node_ptr, rendered_cards)) {
            standard_error("Node "+nkey.str()+" not found in Graph, skipping", __func__);
        }

    }

    return get_column(header, rendered_cards, rendered_columns);
}

bool nodeboard::get_Topic_column(const std::string & topic_str, std::string & rendered_columns) {
    std::string header, identifier;
    parse_header_identifier(topic_str, header, identifier);
    Topic * topic_ptr = graph().find_Topic_by_tag(identifier);
    if (!topic_ptr) {
        standard_error("Invalid Topic tag: "+identifier, __func__);
        return false;
    }
    topic_id = topic_ptr->get_id();
    targetdate_sorted_Nodes topic_nodes = Nodes_with_topic_by_targetdate(graph(), topic_id); // *** could grab a cache here

    std::string rendered_cards;

    for (const auto & [tdate, node_ptr] : topic_nodes) {

        if (!get_Node_card(node_ptr, rendered_cards)) {
            standard_error("Node not found in Graph, skipping", __func__);
        }

    }

    return get_column(header, rendered_cards, rendered_columns);
}

bool nodeboard::get_day_column(unsigned int day_idx, std::string & rendered_columns) {
    std::string header(csv_data_vec[day_idx].day[0].start_date);

    time_t t_day = ymd_stamp_time("20"+header.substr(6, 2)+header.substr(0, 2)+header.substr(3, 2));
    header = "<h4>"+WeekDay(t_day)+' '+DateStampYmd(t_day)+"</h4>";

    std::string rendered_cards;

    // *** NOTE: Here, can add optional explicit visualization of passed time with semi-transparent overlay.
    if (explicit_passed_time && (day_idx == 0)) {
        std::string & first_start = csv_data_vec[day_idx].day.front().start_time;
        int hours = atoi(first_start.substr(0,2).c_str());
        int mins = atoi(first_start.substr(3,2).c_str());
        unsigned int passed_minutes = (hours*60)+mins;
        get_Passed_Time_overlay(passed_minutes, rendered_cards);
    }

    for (const auto & entry_data : csv_data_vec[day_idx].day) {

        if (!get_Schedule_card(entry_data, rendered_cards)) {
            standard_error("Node not found in Graph, skipping", __func__);
        }

    }

    return get_column(header, rendered_cards, rendered_columns, "", schedule_column_temp);
}

bool nodeboard::make_simple_grid_board(const std::string & rendered_cards) {
    template_varvalues board;
    // Insert the HTML for all the cards into the Kanban board.
    board.emplace("board-header",board_title);
    board.emplace("the-nodes",rendered_cards);
    std::string rendered_board = env.render(templates[node_board_temp], board);

    return to_output(rendered_board);
}

std::string nodeboard::call_comment_string() {
    std::string call_comment;
    if (node_ptr != nullptr) {
        call_comment += " Node="+node_ptr->get_id_str();
    }
    if (!source_file.empty()) {
        call_comment += " source_file="+source_file;
    }
    if (!list_name.empty()) {
        call_comment += " list_name="+list_name;
    }
    if (!filter_substring.empty()) {
        call_comment += " filter_substring="+filter_substring;
    }
    if (show_completed) {
        call_comment += " show_completed=true";
    } else {
        call_comment += " show_completed=false";
    }
    return call_comment;
}

bool nodeboard::make_multi_column_board(const std::string & rendered_columns, template_id_enum board_template, bool specify_rows, const std::string & col_width, const std::string & container_width, const std::string & card_width, const std::string & card_height) {
    template_varvalues board;
    // Insert the HTML for all the cards into the Kanban board.
    std::string column_widths;
    for (unsigned int i=0; i<num_columns; i++) {
        column_widths += col_width;
    }
    board.emplace("board-header", board_title);
    board.emplace("board-extra", board_title_extra);
    board.emplace("column-widths", column_widths);
    if (timeline) {
        board.emplace("column-bgcolor", "rgba(0,0,0,0.0)");
    } else {
        board.emplace("column-bgcolor", "var(--grid-bgoverlay)");
    }
    if (!container_width.empty()) {
        board.emplace("container-width", container_width);
    }
    if (!card_width.empty()) {
        board.emplace("card-width", card_width);
    }
    if (!card_height.empty()) {
        board.emplace("card-height", card_height);
    }
    board.emplace("the-columns", rendered_columns);
    std::string specified_rows;
    if (specify_rows) {
        specified_rows = "grid-template-rows:";
        for (unsigned int i=0; i<num_rows; i++) {
            specified_rows += " 400px";
        }
        specified_rows += ";\n";
        //specified_rows = "grid-template-rows: 300px repeat("+std::to_string(num_rows)+");\n";
    }
    board.emplace("row-heights", specified_rows);
    board.emplace("call-comment", call_comment_string());
    board.emplace("post-extra", post_extra);
    std::string rendered_board;
    rendered_board = env.render(templates[board_template], board);

    if (to_output(rendered_board)) {
        if (!progress_state_file.empty()) {
            progress_node_count = node_total;
            progress_state_update();
        }
        return true;
    }
    return false;
}

const std::map<bool, std::string> alt_showcompleted_label = {
    { false, "Include Completed/Inactive" },
    { true, "Exclude Completed/Inactive" }
};

const std::map<bool, std::string> alt_shownonmilestone_label = {
    { false, "Include Non-Milestone Nodes" },
    { true, "Milestones Only" }
};

std::string nodeboard::with_and_without_inactive_Nodes_buttons() const {
    // Prepare options for the buttons.
    nodeboard_options refresh = get_nodeboard_options();
    nodeboard_options alt_showcompleted = refresh;
    alt_showcompleted._showcompleted = !refresh._showcompleted;
    nodeboard_options alt_shownonmilestone = refresh;
    alt_shownonmilestone._shownonmilestone = !refresh._shownonmilestone;
    // Prepare the CGI arguments for the buttons.
    std::string refresh_url = build_nodeboard_cgi_call(refresh);
    std::string alt_showcompleted_url = build_nodeboard_cgi_call(alt_showcompleted);
    std::string alt_shownonmilestone_url = build_nodeboard_cgi_call(alt_shownonmilestone);
    // Generate HTML for the buttons.
    std::string refresh_button = make_button(refresh_url, "Refresh", true); // On same page.
    std::string alt_showcompleted_button = make_button(alt_showcompleted_url, alt_showcompleted_label.at(show_completed), false); // On new page.
    std::string alt_shownonmilestone_button = make_button(alt_shownonmilestone_url, alt_shownonmilestone_label.at(refresh._shownonmilestone), false); // On new page.
    return refresh_button + alt_showcompleted_button + alt_shownonmilestone_button;
}

/**
 * Call this to generate the URL for a button that calls fzgraphsearch-cgi.py to
 * produce a list of near-term (1 week) active Nodes in the subtree of node_ptr.
 */
std::string nodeboard::get_list_nearterm_Nodes_url() const {
    if (!node_ptr) return "";

    time_t t_limit = t_now + (7*86400); // one week out
    std::string nearterm_url("/cgi-bin/fzgraphsearch-cgi.py?completion_lower=0.0&completion_upper=0.99&hours_lower=0.01&hours_upper=999999.0&TD_upper="+TimeStampYmdHM(t_limit)+"&sup_min=0&subtree="+node_ptr->get_id_str());
    return nearterm_url;
}

std::string nodeboard::into_grid(unsigned int row_idx, unsigned int col_idx, unsigned int span, const std::string & content) const {
    std::string to_grid = "<div style=\"grid-area: " // position: absolute; 
        + std::to_string(row_idx+1) + " / "
        + std::to_string(col_idx+1);
    if (span > 1) {
        to_grid += " / " + std::to_string(row_idx+1) + " / span "
            + std::to_string(span);
    }
    to_grid += ";\">" + content + "</div>\n";

    if (timeline) {
        // using total width of column-container in Kanban_board.template.html
        const unsigned long colpx = (250-5);
        unsigned long width = span * colpx;
        to_grid += "<div style=\"background-color:var(--grid-bgoverlay);position:absolute;top:"+to_precision_string(timeline_last_vh, 2);
        to_grid += "vh;left:"+std::to_string(col_idx*colpx);
        to_grid += "px;height:100px;width:"+std::to_string(width)+"px;\"></div>\n";
    }

    return to_grid;
}

bool node_board_render_random_test(nodeboard & nb) {
    if (!nb.render_init()) {
        return false;
    }

    // Creating a node index in order to be able to randomly select nodes
    // between 0 and N.
    Node_Index nodeindex = nb.graph().get_Indexed_Nodes();
    std::string rendered_cards;

    srand (time(NULL));
    FZOUT("Selecting 21 Nodes at random...");

    for (int i=0; i<21; ++i) {

        // Randomly pick 21 Nodes.
        int r = rand() % nb.graph().num_Nodes();

        Node * node_ptr = nodeindex[r];

        if (!nb.get_Node_card(node_ptr, rendered_cards)) {
            standard_error("Node not found in Graph, skipping", __func__);
            continue;
        }

        FZOUT(node_ptr->get_id_str()+'\n');

    }

    return nb.make_simple_grid_board(rendered_cards);
}

bool node_board_render_dependencies(nodeboard & nb) {
    if (!nb.node_ptr) {
        return false;
    }

    if (!nb.render_init()) {
        return false;
    }

    if (!nb.board_title_specified) {
        nb.board_title = nb.node_ptr->get_id_str();
        std::string idtext(nb.node_ptr->get_id_str());
        nb.board_title = "<a href=\"/cgi-bin/fzlink.py?id="+idtext+"\">"+idtext+"</a>";

        std::string htmltext(nb.node_ptr->get_text().c_str());
        nb.board_title_extra = remove_html_tags(htmltext).substr(0,nb.excerpt_length);
    }

    std::string rendered_columns;

    std::string include_filter_substr("&F="+nb.uri_encoded_filter_substring);
    if (!nb.filter_topics.empty()) {
        include_filter_substr += "&i="+nb.uri_encoded_filter_topics;
    }
    if (nb.show_completed) {
        include_filter_substr += "&I=true";
    }

    for (const auto & edge_ptr : nb.node_ptr->dep_Edges()) {
        if (edge_ptr) {

            Node * dep_ptr = edge_ptr->get_dep();

            if ((!nb.show_completed) && (!dep_ptr->is_active())) {
                continue;
            }

            std::string htmltext(dep_ptr->get_text().c_str());
            std::string tagless(remove_html_tags(htmltext));

            if (!nb.filter_substring.empty()) {
                std::string excerpt = tagless.substr(0, nb.filter_substring_excerpt_length);
                if (excerpt.find(nb.filter_substring)==std::string::npos) {
                    continue;
                }
            }

            std::string idtext(edge_ptr->get_dep_str());
            std::string column_header_with_link = "<a href=\"/cgi-bin/fzlink.py?id="+idtext+"\">"+idtext+"</a> (<a href=\"/cgi-bin/nodeboard-cgi.py?n="+idtext+include_filter_substr+"\">DEP</a>)";

            nb.get_dependencies_column(column_header_with_link, edge_ptr->get_dep(), rendered_columns, tagless.substr(0,nb.excerpt_length));

        }
    }

    nb.post_extra = nb.with_and_without_inactive_Nodes_buttons();

    return nb.make_multi_column_board(rendered_columns);
}

bool node_board_render_named_list(Named_Node_List_ptr namedlist_ptr, nodeboard & nb) {
    if (!namedlist_ptr) {
        return false;
    }

    if (!nb.render_init()) {
        return false;
    }

    std::string rendered_cards;

    for (const auto & nkey : namedlist_ptr->list) {

        Node * node_ptr = nb.graph().Node_by_id(nkey);
        if (!nb.get_Node_card(node_ptr, rendered_cards)) {
            standard_error("Node "+nkey.str()+" not found in Graph, skipping", __func__);
        }

    }

    return nb.make_simple_grid_board(rendered_cards);
}

bool node_board_render_list_of_named_lists(nodeboard & nb) {
    if (!nb.render_init()) {
        return false;
    }

    std::string rendered_columns;

    for (const auto & nnl_str : nb.list_names_vec) {

        nb.get_NNL_column(nnl_str, rendered_columns);

    }

    return nb.make_multi_column_board(rendered_columns);
}

bool node_board_render_list_of_topics(nodeboard & nb) {
    if (!nb.render_init()) {
        return false;
    }

    std::string rendered_columns;

    for (const auto & topic_str : nb.list_names_vec) {

        nb.get_Topic_column(topic_str, rendered_columns);

    }

    return nb.make_multi_column_board(rendered_columns);
}

bool node_board_render_list_of_topics_and_NNLs(nodeboard & nb) {
    if (!nb.render_init()) {
        return false;
    }

    std::string rendered_columns;

    for (const auto & topic_or_NNL_str : nb.list_names_vec) {

        if (topic_or_NNL_str.substr(0,4)=="NNL:") {
            nb.get_NNL_column(topic_or_NNL_str.substr(4), rendered_columns);
        } else {
            nb.get_Topic_column(topic_or_NNL_str, rendered_columns);
        }

    }

    return nb.make_multi_column_board(rendered_columns);
}

/**
 * This works with JSON files that are formatted according to the
 * standard used in sysmet-extract.
 */
bool node_board_render_sysmet_categories(nodeboard & nb) {
    std::string json_str;
    std::ifstream::iostate readstate;
    if (!file_to_string(nb.source_file, json_str, &readstate)) {
        return standard_exit_error(exit_file_error, "Unable to read file at "+nb.source_file, __func__);
    }
    JSON_data data(json_str);
    VERYVERBOSEOUT("JSON data:\n"+data.json_str());
    VERBOSEOUT("Number of blocks  : "+std::to_string(data.blocks())+'\n');
    VERBOSEOUT("Number of elements: "+std::to_string(data.size())+'\n');

    if (!nb.board_title_specified) {
        auto slashpos = nb.source_file.find_last_of('/');
        if (slashpos == std::string::npos) {
            nb.board_title = nb.source_file;
        } else {
            nb.board_title = nb.source_file.substr(slashpos+1);
        }
    }

    // Convert the content of the JSON file into a mixed Topics and NNLs string vector.
    for (auto & section : data.content_elements()) {
        // Is the value of this JSON key a map? 
        if (is_populated_JSON_block(section.get())) {
            std::string header_prefix = section->label;
            JSON_block * block_ptr = section->children.get();
            JSON_element_data_vec category_info; // register which variables to look for in the JSON string
            category_info.emplace_back("Topics");
            category_info.emplace_back("NNLs");
            if (block_ptr->find_many(category_info)>0) { // Contains "Topics" and/or "NNLs".
                std::string topics;
                std::string nnls;
                for (auto & category : category_info) {
                    if (category.label == "Topics") {
                        topics = category.text;
                    }
                    if (category.label == "NNLs") {
                        nnls = category.text;
                    }
                }
                auto topics_vec = split(topics, ';');
                auto nnls_vec = split(nnls, ';');
                if ((topics_vec.size()+nnls_vec.size())==1) {
                    for (unsigned int i=0; i<topics_vec.size(); i++) {
                        nb.list_names_vec.emplace_back(header_prefix+':'+topics_vec[i]);
                    }
                    for (unsigned int i=0; i<nnls_vec.size(); i++) {
                        nb.list_names_vec.emplace_back("NNL:"+header_prefix+':'+nnls_vec[i]);
                    }
                } else {
                    for (unsigned int i=0; i<topics_vec.size(); i++) {
                        nb.list_names_vec.emplace_back(header_prefix+' '+topics_vec[i]+':'+topics_vec[i]);
                    }
                    for (unsigned int i=0; i<nnls_vec.size(); i++) {
                        nb.list_names_vec.emplace_back("NNL:"+header_prefix+' '+nnls_vec[i]+':'+nnls_vec[i]);
                    }
                }
            }
        } // *** else - Do we want to be able to create empty columns with headers?
    }

    return node_board_render_list_of_topics_and_NNLs(nb);
}

std::string prepare_headnode_description(nodeboard & nb, const Node & node) {
    std::string headnode_description;

    std::string htmltext(node.get_text().c_str());

    if (nb.threads) {
        auto visoutput_start = htmltext.find("@VISOUTPUT:");
        if (visoutput_start != std::string::npos) {
            visoutput_start += 11;
            auto visoutput_end = htmltext.find('@', visoutput_start);
            if (visoutput_end != std::string::npos) {
                headnode_description = "<b>"+htmltext.substr(visoutput_start, visoutput_end - visoutput_start)+"</b>";
            }
        }
    }

    if (headnode_description.empty()) {
        headnode_description = remove_html_tags(htmltext).substr(0,nb.excerpt_length);
    }

    return headnode_description;
}

std::string prepare_threads_board_topdata(const Threads_Board_Data& board_data) {
    std::string boarddata_str;
    boarddata_str.reserve(1024);

    // std::string nodes_str;
    // for (size_t i = 0; i < board_data.topnodes.size(); i++) {
    //     if (i>0) {
    //         nodes_str += ',';
    //     }
    //     nodes_str += board_data.topnodes.at(i).str();
    // }

    std::string remaining_str;
    for (size_t i = 0; i < board_data.remaining_hrs.size(); i++) {
        if (i>0) {
            remaining_str += ',';
        }
        remaining_str += to_precision_string(board_data.remaining_hrs.at(i), 1);
    }

    std::string invested_str;
    for (size_t i = 0; i < board_data.invested_hrs.size(); i++) {
        if (i>0) {
            invested_str += ',';
        }
        invested_str += to_precision_string(board_data.invested_hrs.at(i), 1);
    }

    boarddata_str += "<br>Hours required: "+to_precision_string(board_data.board_tot_required_s/3600.0, 1);
    boarddata_str += " (Hours completed: "+to_precision_string(board_data.board_tot_completed_s/3600.0, 1);
    boarddata_str += ") Hours available: "+to_precision_string(float(board_data.board_seconds_to_targetdate)/3600.0, 1);
    boarddata_str += " (to "+TimeStampYmdHM(board_data.board_max_top_nodes_targetdate);
    boarddata_str += ") <button onclick=\"window.open('/cgi-bin/weekreview.py?nnl="+board_data.nnl+"&invested="+invested_str+"&remaining="+remaining_str+"','_blank');\">Plot</button>";

    return boarddata_str;
}

// This is used to generate a Threads Board (and similar boards).
bool node_board_render_NNL_dependencies(nodeboard & nb) {
    Named_Node_List_ptr namedlist_ptr = nb.graph().get_List(nb.list_name);
    if (!namedlist_ptr) {
        return false;
    }

    if (!nb.render_init()) {
        return false;
    }

    if (!nb.board_title_specified) {
        nb.board_title = "Dependencies of Nodes in NNL '"+nb.list_name+'\'';
    }

    std::string threads_option;
    if (nb.threads) threads_option = "&T=true";

    //nb.board_title_extra = with_and_without_inactive_Nodes_buttons("D="+nb.list_name, threads_option, nb.show_completed);
    nb.board_title_extra = nb.with_and_without_inactive_Nodes_buttons();

    if (nb.threads) {
        nodeboard_options with_progress_analysis = nb.get_nodeboard_options();
        with_progress_analysis._floption = flow_NNL_dependencies;
        //with_progress_analysis._showcompleted = true;
        with_progress_analysis._progressanalysis = true;
        nb.board_title_extra += make_button(nb.build_nodeboard_cgi_call(with_progress_analysis), "With Progress Analysis", false);
        nb.board_title_extra +=
            "<button class=\"button button1\" onclick=\"window.open('/cgi-bin/fzquerypq-cgi.py','_blank');\">Update Node histories</button>"
            "<b>Threads</b>:<br>"
            "Where @VISOUTPUT: ...@ is defined in Node content it is shown as the expected advantageous non-internal (visible) output of a thread.<br>"
            "Otherwise, an excerpt of Node content is shown as the thread header.<br>"
            "The Nodes in a thread should be a clear set of steps leading to the output.<br>"
            "A red header indicates that there are no active Nodes to work on in a thread.";
    }

    nb.map_of_subtrees.sort_by_targetdate = true;
    //nb.map_of_subtrees.norepeated = nb.norepeated; // *** Not using this right now, as it probably breaks Node_Branch connections! See how we do this below instead.
    nb.map_of_subtrees.collect(nb.graph(), nb.list_name);
    if (!nb.map_of_subtrees.has_subtrees) return false;

    if (!nb.progress_state_file.empty()) {
        nb.node_total = nb.map_of_subtrees.total_node_count();
    }

    Threads_Board_Data data;
    data.nnl = nb.list_name;

    for (const auto & nkey : namedlist_ptr->list) {

        // Get the next thread header Node.
        Node * node_ptr = nb.graph().Node_by_id(nkey);
        if (!node_ptr) {
            standard_error("Node "+nkey.str()+" not found in Graph, skipping", __func__);
            continue;
        }

        data.topnodes.emplace_back(nkey);
        time_t td_threadtop = node_ptr->effective_targetdate();
        if (td_threadtop > data.board_max_top_nodes_targetdate) {
            data.board_max_top_nodes_targetdate = td_threadtop;
        }

        // TODO: *** Should progress analysis also be applied to the header Node?

        std::string idtext(nkey.str());
        std::string headnode_data_and_links = "<a href=\"/cgi-bin/fzlink.py?id="+idtext+"\" target=\"_blank\">"+idtext+"</a>";
        headnode_data_and_links += "<button class=\"tiny_button tiny_wider\" onclick=\"window.open('/cgi-bin/nodeboard-cgi.py?n="+idtext+"&G=true&T=true&C=300&B=79&tdorder=true&tdbad=true&tdfar=true','_blank');\">DEP tree</button>";
        headnode_data_and_links += '('+node_ptr->get_effective_targetdate_str()+')';

        std::string headnode_description(prepare_headnode_description(nb, *node_ptr));

        nb.get_fulldepth_dependencies_column(headnode_description, nkey, data, headnode_data_and_links);

        // include data for top node
        auto topnode_reqcomp = get_required_completed(*node_ptr);
        data.board_tot_required_s += topnode_reqcomp.required_s;
        data.board_tot_completed_s += topnode_reqcomp.completed_s;
        data.invested_hrs.back() += (topnode_reqcomp.completed_s/3600.0);
        data.remaining_hrs.back() += ((topnode_reqcomp.required_s - topnode_reqcomp.completed_s)/3600.0);

    }

    data.board_seconds_to_targetdate = data.board_max_top_nodes_targetdate - nb.t_now;
    nb.board_title_extra += prepare_threads_board_topdata(data);

    return nb.make_multi_column_board(data.rendered_columns, kanban_alt_board_temp, false, nb.grid_column_width, nb.column_container_width, nb.card_width, nb.card_height);
}

// Show if the grid has been cropped in rows or columns.
// Possibly also show buttons that allow extending of the grid.
void show_grid_cropped_conditions(nodeboard & nb, Node_Grid & grid) {
    if (grid.columns_cropped) {
        nb.board_title_extra += "<b>Grid COLUMNS were cropped ("+std::to_string(nb.max_columns)+").</b> ";
        nodeboard_options extend_columns = nb.get_nodeboard_options();
        extend_columns.maxcols += 50;
        nb.board_title_extra += make_button(nb.build_nodeboard_cgi_call(extend_columns), "Extend Columns (50)", false);
    }
    if (grid.rows_cropped) {
        nb.board_title_extra += "<b>Grid ROWS were cropped ("+std::to_string(nb.max_rows)+").</b> ";
        nodeboard_options extend_rows = nb.get_nodeboard_options();
        extend_rows.maxrows += 50;
        nb.board_title_extra += make_button(nb.build_nodeboard_cgi_call(extend_rows), "Extend Rows (50)", false);
    }
}

struct Node_Tree_earliest_td: public Node_Tree_Op {
public:
    time_t earliest_td = RTt_maxtime;

public:
    Node_Tree_earliest_td() {}
    virtual void op(const Node& node) {
        time_t t = const_cast<Node&>(node).effective_targetdate();
        if ((t > 0) && (t < earliest_td)) {
            earliest_td = t;
        }
    }
};

bool node_board_render_dependencies_tree(nodeboard & nb) {
    if (!nb.node_ptr) {
        return false;
    }

    Node_Grid grid(nb);

    if (nb.timeline) {
        Node_Tree_earliest_td NTetd_op;
        grid.op(NTetd_op);
        nb.timeline_min_td = NTetd_op.earliest_td;
    }

    if (!nb.render_init()) {
        return false;
    }

    if (!nb.board_title_specified) {
        nb.board_title = "Dependencies tree of Node "+nb.node_ptr->get_id_str();
    }

    if (!nb.make_options_pane(nb.board_title_extra)) {
        nb.board_title_extra = "Warning: Failed to render options pane.<br>";
    }

    nb.board_title_extra += nb.with_and_without_inactive_Nodes_buttons();

    nb.board_title_extra += make_button(nb.get_list_nearterm_Nodes_url(), "List near-term Nodes", false);

    if (grid.number_of_proposed_td_changes()>0) {
        nb.board_title_extra += "\n<br>Number of proposed TD changes = "+std::to_string(grid.number_of_proposed_td_changes());
        nb.board_title_extra += "\n<br>Proposed TD changes:"+grid.list_of_proposed_td_changes_html();
        nb.board_title_extra += "\n<br>"+make_button(grid.get_td_changes_apply_url(),"Apply TD changes", false);
        nb.board_title_extra += make_button(grid.get_vtd_changes_only_apply_url(),"Apply only VTD changes", false);
    }

    if (grid.has_errors()) {
        nb.board_title_extra += "Errors: ";
        nb.board_title_extra += grid.errors_str();
    }

    show_grid_cropped_conditions(nb, grid);

    std::string rendered_grid;

    // Generate output row by row for a sensible output order of the
    // elements of the grid that were collected. Previously, this was
    // simply 'for (const auto & [nkey, element] : grid.nodes)', which
    // placed elements in the right positions, but made finding the
    // cause of rendering errors very difficult due to the nearly random
    // order of elements in the resulting HTML output.
    for (const auto & row : grid.rows) {
        if (row.is_legit_row()) {
            for (const auto & element_ptr : row.elements) {
                if (element_ptr) {

                    std::string rendered_card;
                    if (nb.get_Node_alt_card(element_ptr->node_ptr, element_ptr->node_ptr->get_targetdate(), rendered_card)==node_render_error) {
                        standard_error("Node not found in Graph, skipping", __func__);
                        continue;
                    }
                    if (!rendered_card.empty()) {
                        rendered_grid += nb.into_grid(element_ptr->row_pos, element_ptr->col_pos, element_ptr->span, rendered_card);
                    }
                }
            }
        }
    }

    nb.num_columns = grid.occupied.columns_used(); //grid.col_total;
    nb.num_rows = grid.occupied.rows_used(); //grid.row_total;

    return nb.make_multi_column_board(rendered_grid, kanban_alt_board_temp, true);
}

bool node_board_render_superiors_tree(nodeboard & nb) {
    if (!nb.node_ptr) {
        return false;
    }

    Node_Grid grid(nb, true);

    if (!nb.render_init()) {
        return false;
    }

    if (!nb.board_title_specified) {
        nb.board_title = "Superiors tree of Node "+nb.node_ptr->get_id_str();
    }

    if (!nb.make_options_pane(nb.board_title_extra)) {
        nb.board_title_extra = "Warning: Failed to render options pane.<br>";
    }

    nb.board_title_extra += nb.with_and_without_inactive_Nodes_buttons();

    nb.board_title_extra += make_button(nb.get_list_nearterm_Nodes_url(), "List near-term Nodes", false);

    if (grid.number_of_proposed_td_changes()>0) {
        nb.board_title_extra += "\n<br>Number of proposed TD changes = "+std::to_string(grid.number_of_proposed_td_changes());
        nb.board_title_extra += "\n<br>Proposed TD changes:"+grid.list_of_proposed_td_changes_html();
        nb.board_title_extra += make_button(grid.get_td_changes_apply_url(),"Apply TD changes", false);
        nb.board_title_extra += make_button(grid.get_vtd_changes_only_apply_url(),"Apply only VTD changes", false);
    }

    if (grid.has_errors()) {
        nb.board_title_extra += "Errors: ";
        nb.board_title_extra += grid.errors_str();
    }

    std::string rendered_grid;

    // Generate output row by row for a sensible output order of the
    // elements of the grid that were collected. Previously, this was
    // simply 'for (const auto & [nkey, element] : grid.nodes)', which
    // placed elements in the right positions, but made finding the
    // cause of rendering errors very difficult due to the nearly random
    // order of elements in the resulting HTML output.
    for (const auto & row : grid.rows) {
        if (row.is_legit_row()) {
            for (const auto & element_ptr : row.elements) {
                if (element_ptr) {

                    std::string rendered_card;
                    if (nb.get_Node_alt_card(element_ptr->node_ptr, element_ptr->node_ptr->get_targetdate(), rendered_card)==node_render_error) {
                        standard_error("Node not found in Graph, skipping", __func__);
                        continue;
                    }
                    if (!rendered_card.empty()) {
                        rendered_grid += nb.into_grid(element_ptr->row_pos, element_ptr->col_pos, element_ptr->span, rendered_card);
                    }
                }
            }
        }
    }

    nb.num_columns = grid.occupied.columns_used(); //grid.col_total;
    nb.num_rows = grid.occupied.rows_used(); //grid.row_total;

    return nb.make_multi_column_board(rendered_grid, kanban_alt_board_temp, true);
}

// Note: @TZADJUST@ is not applied here, because times are already provided
//       in the CSV data. To select @TZADJUST@ application, do so in the
//       program that generates the CSV data, e.g. schedule.
bool node_board_render_csv_schedule(nodeboard & nb) {
    std::string csv_str;
    if (!file_to_string(nb.source_file, csv_str)) {
        return standard_exit_error(exit_file_error, "Unable to read file at "+nb.source_file, __func__);
    }

    if (!nb.parse_csv(csv_str)) {
        return standard_exit_error(exit_file_error, "Unparsable CSV file "+nb.source_file, __func__);
    }

    nb.sleepNNL_ptr = nb.graph().get_List(sleepNNL);
    if (!nb.sleepNNL_ptr) return standard_error("Unable to retrieve NNL "+sleepNNL, __func__);

    if (!nb.render_init()) {
        return false;
    }

    std::string rendered_days;

    for (unsigned int day_idx = 0; day_idx < nb.csv_data_vec.size(); day_idx++) {

        nb.get_day_column(day_idx, rendered_days);

    }

    float col_width = 95.0/float(nb.csv_data_vec.size());
    float card_width = 0.95*col_width;

    nodeboard_options shorter = nb.get_nodeboard_options();
    nodeboard_options longer = shorter;
    shorter.multiplier = (shorter.multiplier > 1.0) ? shorter.multiplier-1.0 : shorter.multiplier/2.0;
    longer.multiplier = (longer.multiplier >= 1.0) ? longer.multiplier+1.0 : longer.multiplier*2.0;
    nb.post_extra = make_button(nb.build_nodeboard_cgi_call(shorter), "smaller", true)
        + make_button(nb.build_nodeboard_cgi_call(longer), "larger", true);

    std::string grid_column_width_str = ' '+to_precision_string(col_width, 2)+"vw";
    std::string card_width_str = to_precision_string(card_width, 2)+"vw";
    return nb.make_multi_column_board(rendered_days, schedule_board_temp, false, grid_column_width_str, card_width_str, card_width_str);
}
