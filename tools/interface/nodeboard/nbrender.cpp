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
};

bool load_templates(nodeboard_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i] + ".template.html", templates[static_cast<template_id_enum>(i)]))
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

    add_option_args += "RGgn:L:D:l:t:m:f:c:Ii:F:H:TPe:b:M:Kp:B:C:S:Xo:";
    add_usage_top += " [-R] [-G|-g] [-n <node-ID>] [-L <name>] [-D <name>] [-l {<name>,...}] [-t {<topic>,...}]"
        " [-m {<topic>,NNL:<name>,...}] [-f <json-path>] [-c <csv-path>] [-I] [-Z] [-i <topic_id>,...] [-F <substring>]"
        " [-H <board-header>] [-T] [-P] [-e <errors-list>] [-b <before>] [-M <multiplier>] [-p <progress-state-file>]"
        " [-K] [-S <size-list>] [-B <topic-id>] [-C <max-columns>] [-r <max-rows>] [-X] [-o <output-file|STDOUT>]";
}

void nodeboard::usage_hook() {
    ga.usage_hook();
    FZOUT(
        "    -R Test with random selection of Nodes.\n"
        "    -G Use grid to show dependencies tree.\n"
        "    -g Use grid to show superiors tree.\n"
        "    -n Node dependencies.\n"
        "    -L Use Nodes data in Named Node List.\n"
        "    -D Dependencies of Nodes in Named Node List.\n"
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
        "    -i Filter to show only Nodes in one of the listed topics (by ID)\n"
        "    -F Filter to show only Nodes where the first 80 characters contain the\n"
        "       substring.\n"
        "    -H Board header.\n"
        "    -T Threads.\n"
        "    -P Progress analaysis.\n"
        "    -b Before time stamp.\n"
        "    -M Vertical length multiplier.\n"
        "    -p Progress state file\n"
        "    -K Sort by subtree times."
        "    -e Detect and visualize errors specified in comma separated list.\n"
        "       List can contain: tdorder,  tdfar, tdbad\n"
        "    -B Background highlight Nodes in <topic-id> with elevated valuation\n"
        "       highlight colors.\n"
        "    -S List of grid and card sizes:\n"
        "       '<grid-column-width>,<column-container-width>,<card-width>,<card-height>'\n"
        "       E.g. '260px,250px,240px,240px'\n"
        "    -C Max number of columns (default: 100)\n"
        "    -r Max number of rows (default: 100)\n"
        "    -X Fully expanded grid (do not minimize rows and columns)\n"
        "    -o Output to file (or STDOUT).\n"
        "       Default: /var/www/html/formalizer/test_node_card.html\n"
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
            flowcontrol = flow_random_test;
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
            flowcontrol = flow_NNL_dependencies;
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

        case 'i': {
            uri_encoded_filter_topics = uri_encode(cargs);
            return parse_filter_topics(cargs);
        }

        case 'F': {
            filter_substring = cargs;
            uri_encoded_filter_substring = uri_encode(filter_substring);
            return true;
        }

        case 'H': {
            board_title_specified = true;
            board_title = cargs;
            return true;
        }

        case 'T': {
            threads = true;
            return true;
        }

        case 'e': {
            return parse_error_detection_list(cargs);
        }

        case 'P': {
            progress_analysis = true;
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

        case 'C': {
            max_columns = atoi(cargs.c_str());
            return true;
        }

        case 'r': {
            max_rows = atoi(cargs.c_str());
            return true;
        }

        case 'X': {
            minimize_grid = false;
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

std::string nodeboard::build_nodeboard_cgi_call(flow_options _floption, bool _threads, bool _showcompleted, bool _progressanalysis, float multiplier, unsigned int maxcols, unsigned int maxrows) {
    std::string cgi_cmd("/cgi-bin/nodeboard-cgi.py?");
    switch (_floption) {
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
            if (multiplier > 1.0) {
                multiplier_arg = "&M="+to_precision_string(multiplier, 1);
            }
            cgi_cmd += "c="+source_file+"&H=Proposed_Schedule"+multiplier_arg;
            return cgi_cmd;
        }
        default: {
            return "";
        }
    }
    if (_showcompleted) {
        cgi_cmd += "&I=true";
    }
    if (_threads) {
        cgi_cmd += "&T=true";
    }
    if (_progressanalysis) {
        cgi_cmd += "&P=true";
    }
    if (highlight_topic_and_valuation) {
        cgi_cmd += "&B="+std::to_string(highlight_topic);
    }
    if (maxrows != DEFAULTMAXROWS) {
        cgi_cmd += "&r="+std::to_string(maxrows);
    }
    if (maxcols != DEFAULTMAXCOLS) {
        cgi_cmd += "&C="+std::to_string(maxcols);
    }
    if (!filter_substring.empty()) {
        cgi_cmd += "&F="+uri_encoded_filter_substring;
    }
    if (!filter_topics.empty()) {
        cgi_cmd += "&i="+uri_encoded_filter_topics;
    }
    return cgi_cmd;
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
    std::string node_id_str = node_ptr->get_id_str();
    nodevars.emplace("node-id", node_id_str);
    // Show a hint that this Node may  have some history worth seeing.
    if (node_ptr->probably_has_Log_history()) {
        nodevars.emplace("nodelink-bg-color", "class=\"link-bg-color-yellow\"");
    } else {
        nodevars.emplace("nodelink-bg-color", "");
    }
    nodevars.emplace("node-id-history", node_id_str+"&alt=histfull");
    nodevars.emplace("node-deps", node_id_str);
    nodevars.emplace("node-text", node_text);
    float completion = node_ptr->get_completion();
    float progress = completion*100.0;
    if (progress < 0.0) progress = 100.0;
    if (progress > 100.0) progress = 100.0;
    nodevars.emplace("node-progress", to_precision_string(progress, 1));
    nodevars.emplace("node-targetdate", " ("+DateStampYmd(tdate)+')');
    nodevars.emplace("fzserverpq", graph().get_server_full_address());

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

    // Show if a Node is inactive, active exact/fixed, or active VTD.
    std::string node_color;
    if (node_ptr->is_active()) {
        bool tderror = false;
        if (detect_tdfar) {
            if (tdate > (t_now + (100L*365L*86400L))) { // Target dates more than a hundred years in the future are suspect.
                tderror = true;
            }
        }
        if (detect_tdbad) {
            if (tdate <= 0) {
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
        } else {
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

const std::map<char, std::string> schedule_entry_color = {
    {'e', "#ffc0cb"}, // pink
    {'E', "#dda0dd"}, // exact with repeats, plum
    {'f', "#98fb98"}, // pale-green
    {'F', "#66cdaa"}, // fixed with repeats, mediumacquamarine
    {'v', "#f0e68c"}, // khaki
    {'u', "#d3d3d3"}, // light-grey
    {'i', "#c4f0a2"}, // mixed khaki and pale-green using https://www.w3schools.com/colors/colors_mixer.asp
};

std::string get_schedule_card_color(const CSV_Data & entry_data) {
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

//bool nodeboard::get_fulldepth_dependencies_column(const std::string & column_header, const base_Node_Set & column_set, std::string & rendered_columns, const std::string extra_header = "") {
bool nodeboard::get_fulldepth_dependencies_column(std::string & column_header, Node_ID_key column_key, std::string & rendered_columns, const std::string extra_header = "") {
    if (!map_of_subtrees.is_subtree_head(column_key)) {
        standard_error("Node "+column_key.str()+" is not a head Node in NNL '"+map_of_subtrees.subtrees_list_name+"', skipping", __func__);
        return false;
    }

    std::string rendered_cards;

    float tot_required_hrs = 0;
    float tot_completed_hrs = 0;
    unsigned int active_rendered = 0;
    // Add all dependencies found to the column as cards.
    //for (const auto & depkey : map_of_subtrees.get_subtree_set(column_key)) {
    auto & subtree_set = map_of_subtrees.get_subtree_set(column_key);
    for (const auto & [tdate, depnode_ptr] : subtree_set.tdate_node_pointers) {
        //Node_ptr depnode_ptr = graph().Node_by_id(depkey);
        if (!depnode_ptr) {
            standard_error("Dependency Node not found, skipping", __func__);
            continue;
        }
        auto noderenderresult = get_Node_alt_card(depnode_ptr, tdate, rendered_cards, const_cast<Node_Subtree*>(&subtree_set));
        if (noderenderresult==node_render_error) {
            standard_error("Dependency Node not found in Graph, skipping", __func__);
        } else if (noderenderresult==node_rendered_active) {
            active_rendered++;
        }

        if (!progress_state_file.empty()) {
            progress_node_count++;
            if (progress_node_count < node_total) { // Do not signal 100% until output is completely provided.
                progress_state_update();
            }
        }

        // Obtain contributions to thread progress.
        float completion = depnode_ptr->get_completion();
        float required = depnode_ptr->get_required();
        if (completion < 0.0) {
            completion = 1.0;
            required = 0.0;
        } else {
            if (completion > 1.0) {
                required = required*completion;
                completion = 1.0;
            }
        }
        tot_required_hrs += required;
        tot_completed_hrs += (required*completion);
    }

    float thread_progress = 0;
    if (tot_required_hrs > 0.0) {
        thread_progress = 100.0 * tot_completed_hrs / tot_required_hrs;
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

    return get_column(column_header, rendered_cards, rendered_columns, extra_header_with_progress, kanban_alt_column_temp);
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

std::string nodeboard::with_and_without_inactive_Nodes_buttons() {
    std::string no_inactive_url = build_nodeboard_cgi_call(flowcontrol, threads, false, progress_analysis, vertical_multiplier, max_columns, max_rows);
    std::string with_inactive_url = build_nodeboard_cgi_call(flowcontrol, threads, true, progress_analysis, vertical_multiplier, max_columns, max_rows);
    std::string refresh_button;
    std::string alt_button;
    if (show_completed) {
        refresh_button = make_button(with_inactive_url, "Refresh", true);
        alt_button = make_button(no_inactive_url, "Exclude Completed/Inactive", false);
    } else {
        refresh_button = make_button(no_inactive_url, "Refresh", true);
        alt_button = make_button(with_inactive_url, "Include Completed/Inactive", false);
    }
    return refresh_button+alt_button;
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
        nb.board_title_extra += make_button(nb.build_nodeboard_cgi_call(flow_NNL_dependencies, nb.threads, true, true), "With Progress Analysis", false);
        nb.board_title_extra +=
            "<button class=\"button button1\" onclick=\"window.open('/cgi-bin/fzquerypq-cgi.py','_blank');\">Update Node histories</button>"
            "<b>Threads</b>:<br>"
            "Where @VISOUTPUT: ...@ is defined in Node content it is shown as the expected advantageous non-internal (visible) output of a thread.<br>"
            "Otherwise, an excerpt of Node content is shown as the thread header.<br>"
            "The Nodes in a thread should be a clear set of steps leading to the output.<br>"
            "A red header indicates that there are no active Nodes to work on in a thread.";
    }

    nb.map_of_subtrees.sort_by_targetdate = true;
    nb.map_of_subtrees.collect(nb.graph(), nb.list_name);
    if (!nb.map_of_subtrees.has_subtrees) return false;

    if (!nb.progress_state_file.empty()) {
        nb.node_total = nb.map_of_subtrees.total_node_count();
    }

    std::string rendered_columns;

    for (const auto & nkey : namedlist_ptr->list) {

        // Get the next thread header Node.
        Node * node_ptr = nb.graph().Node_by_id(nkey);
        if (!node_ptr) {
            standard_error("Node "+nkey.str()+" not found in Graph, skipping", __func__);
            continue;
        }

        // TODO: *** Should progress analysis also be applied to the header Node?

        std::string idtext(nkey.str());
        std::string headnode_id_link = "<a href=\"/cgi-bin/fzlink.py?id="+idtext+"\">"+idtext+"</a>";

        std::string headnode_description(prepare_headnode_description(nb, *node_ptr));

        nb.get_fulldepth_dependencies_column(headnode_description, nkey, rendered_columns, headnode_id_link);

    }

    return nb.make_multi_column_board(rendered_columns, kanban_alt_board_temp, false, nb.grid_column_width, nb.column_container_width, nb.card_width, nb.card_height);
}

// Show if the grid has been cropped in rows or columns.
// Possibly also show buttons that allow extending of the grid.
void show_grid_cropped_conditions(nodeboard & nb, Node_Grid & grid) {
    if (grid.columns_cropped) {
        nb.board_title_extra += "<b>Grid COLUMNS were cropped ("+std::to_string(nb.max_columns)+").</b> ";
        std::string link = nb.build_nodeboard_cgi_call(
            nb.flowcontrol, nb.threads, nb.show_completed,
            nb.progress_analysis, nb.vertical_multiplier, nb.max_columns+50, nb.max_rows);
        nb.board_title_extra += make_button(link, "Extend Columns (50)", false);
    }
    if (grid.rows_cropped) {
        nb.board_title_extra += "<b>Grid ROWS were cropped ("+std::to_string(nb.max_rows)+").</b> ";
        std::string link = nb.build_nodeboard_cgi_call(
            nb.flowcontrol, nb.threads, nb.show_completed,
            nb.progress_analysis, nb.vertical_multiplier, nb.max_columns, nb.max_rows+50);
        nb.board_title_extra += make_button(link, "Extend Rows (50)", false);
    }
}

std::string into_grid(unsigned int row_idx, unsigned int col_idx, unsigned int span, const std::string & content) {
    std::string to_grid = "<div style=\"grid-area: " // position: absolute; 
        + std::to_string(row_idx+1) + " / "
        + std::to_string(col_idx+1);
    if (span > 1) {
        to_grid += " / " + std::to_string(row_idx+1) + " / span "
            + std::to_string(span);
    }
    to_grid += ";\">" + content + "</div>\n";
    return to_grid;
}

bool node_board_render_dependencies_tree(nodeboard & nb) {
    if (!nb.node_ptr) {
        return false;
    }

    Node_Grid grid(nb);

    if (!nb.render_init()) {
        return false;
    }

    if (!nb.board_title_specified) {
        nb.board_title = "Dependencies tree of Node "+nb.node_ptr->get_id_str();
    }

    //std::string threads_option;
    //if (nb.threads) threads_option = "&T=true";

    //nb.board_title_extra = with_and_without_inactive_Nodes_buttons("G=true&n="+nb.node_ptr->get_id_str(), threads_option, nb.show_completed);
    nb.board_title_extra = nb.with_and_without_inactive_Nodes_buttons();

    // if (nb.threads) {
    //     nb.board_title_extra +=
    //         "<b>Tree</b>:<br>"
    //         "Where @VISOUTPUT: ...@ is defined in Node content it is shown as the expected advantageous non-internal (visible) output of a thread.<br>"
    //         "Otherwise, an excerpt of Node content is shown as the thread header.<br>"
    //         "The Nodes in a thread should be a clear set of steps leading to the output.";
    // }

    if (!grid.errors.empty()) {
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
                        rendered_grid += into_grid(element_ptr->row_pos, element_ptr->col_pos, element_ptr->span, rendered_card);
                    }
                }
            }
        }
    }

    nb.num_columns = grid.occupied.columns_used(); //grid.col_total;
    nb.num_rows = grid.occupied.rows_used(); //grid.row_total;

    // FZOUT("Number of rows: "+std::to_string(grid.row_total)+'\n');
    // FZOUT("Number of columns: "+std::to_string(grid.col_total)+'\n');
    // FZOUT("Number of elements: "+std::to_string(grid.num_elements())+'\n');

    // FZOUT("Drawing board...\n"); std::cout.flush();

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

    //std::string threads_option;
    //if (nb.threads) threads_option = "&T=true";

    //nb.board_title_extra = with_and_without_inactive_Nodes_buttons("g=true&n="+nb.node_ptr->get_id_str(), threads_option, nb.show_completed);
    nb.board_title_extra = nb.with_and_without_inactive_Nodes_buttons();

    // if (nb.threads) {
    //     nb.board_title_extra +=
    //         "<b>Tree</b>:<br>"
    //         "Where @VISOUTPUT: ...@ is defined in Node content it is shown as the expected advantageous non-internal (visible) output of a thread.<br>"
    //         "Otherwise, an excerpt of Node content is shown as the thread header.<br>"
    //         "The Nodes in a thread should be a clear set of steps leading to the output.";
    // }

    if (!grid.errors.empty()) {
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
                        rendered_grid += into_grid(element_ptr->row_pos, element_ptr->col_pos, element_ptr->span, rendered_card);
                    }
                }
            }
        }
    }

    nb.num_columns = grid.occupied.columns_used(); //grid.col_total;
    nb.num_rows = grid.occupied.rows_used(); //grid.row_total;

    // FZOUT("Number of rows: "+std::to_string(grid.row_total)+'\n');
    // FZOUT("Number of columns: "+std::to_string(grid.col_total)+'\n');
    // FZOUT("Number of elements: "+std::to_string(grid.num_elements())+'\n');

    // FZOUT("Drawing board...\n"); std::cout.flush();

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

    if (!nb.render_init()) {
        return false;
    }

    std::string rendered_days;

    for (unsigned int day_idx = 0; day_idx < nb.csv_data_vec.size(); day_idx++) {

        nb.get_day_column(day_idx, rendered_days);

    }

    float col_width = 95.0/float(nb.csv_data_vec.size());
    float card_width = 0.95*col_width;

    nb.post_extra = make_button(nb.build_nodeboard_cgi_call(flow_csv_schedule, false, false, false, nb.vertical_multiplier-1.0), "smaller", true)
        + make_button(nb.build_nodeboard_cgi_call(flow_csv_schedule, false, false, false, nb.vertical_multiplier+1.0), "larger", true);

    std::string grid_column_width_str = ' '+to_precision_string(col_width, 2)+"vw";
    std::string card_width_str = to_precision_string(card_width, 2)+"vw";
    return nb.make_multi_column_board(rendered_days, schedule_board_temp, false, grid_column_width_str, card_width_str, card_width_str);
}
