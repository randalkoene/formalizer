// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Node Board rendering functions.
 * 
 * For more about this, see the Trello card at https://trello.com/c/w2XnEQcc
 */

#include <iostream>
#include <stdexcept>

#include "error.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "html.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "templater.hpp"
#include "nbrender.hpp"
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
    "Node_alt_card"
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

    add_option_args += "RGgn:L:D:l:t:m:f:IF:H:To:";
    add_usage_top += " [-R] [-G|-g] [-n <node-ID>] [-L <name>] [-D <name>] [-l {<name>,...}] [-t {<topic>,...}]"
        " [-m {<topic>,NNL:<name>,...}] [-f <json-path>] [-I] [-F <substring>]"
        " [-H <board-header>] [-T] [-o <output-file|STDOUT>]";
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
        "    -m List of mixed Topics and NNLs (prepend with 'NNL:').\n"
        "    -f Use categories from sysmet-style JSON file.\n"
        "\n"
        "       If <name> or <topic> contain a ':' then text preceding it is\n"
        "       used as a custom header.\n"
        "\n"
        "    -I Include completed Nodes.\n"
        "       This also includes Nodes with completion values < 0.\n"
        "    -F Filter to show only Nodes where the first 80 characters contain the\n"
        "       substring.\n"
        "    -H Board header.\n"
        "    -T Threads.\n"
        "    -o Output to file (or STDOUT).\n"
        "       Default: /var/www/html/formalizer/test_node_card.html\n"
        "\n"
    );
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

        case 'I': {
            show_completed = true;
            return true;
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

bool nodeboard::get_Node_card(const Node * node_ptr, std::string & rendered_cards) {
    if (!node_ptr) {
        return false;
    }

    if ((!show_completed) && (!node_ptr->is_active())) {
        return true;
    }

    std::string node_text = node_ptr->get_text();
    if (!filter_substring.empty()) {
        std::string excerpt = node_text.substr(0, filter_substring_excerpt_length);
        if (excerpt.find(filter_substring)==std::string::npos) {
            return true;
        }
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
    if (show_completed) {
        include_filter_substr += "&I=true";
    }
    nodevars.emplace("filter-substr", include_filter_substr);

    // For each node: Create a Kanban card and add it to the output HTML.
    rendered_cards += env.render(templates[node_card_temp], nodevars);
    return true;
}

bool nodeboard::get_Node_alt_card(const Node * node_ptr, std::time_t tdate, std::string & rendered_cards) {
    if (!node_ptr) {
        return false;
    }

    if ((!show_completed) && (!node_ptr->is_active())) {
        return true;
    }

    std::string node_text = node_ptr->get_text();
    if (!filter_substring.empty()) {
        std::string excerpt = node_text.substr(0, filter_substring_excerpt_length);
        if (excerpt.find(filter_substring)==std::string::npos) {
            return true;
        }
    }

    // For each node: Set up a map of content to template position IDs.
    template_varvalues nodevars;
    nodevars.emplace("node-id", node_ptr->get_id_str());
    nodevars.emplace("node-deps", node_ptr->get_id_str());
    nodevars.emplace("node-text", node_text);
    float progress = node_ptr->get_completion()*100.0;
    if (progress < 0.0) progress = 100.0;
    if (progress > 100.0) progress = 100.0;
    nodevars.emplace("node-progress", to_precision_string(progress, 1));
    nodevars.emplace("node-targetdate", " ("+DateStampYmd(tdate)+')');

    std::string prereqs_str;
    if (threads) {
        // Look for specified prerequisites and their state.
        auto prereqs = get_prerequisites(*node_ptr, true);
        if (!prereqs.empty()) {
            unsigned int num_unsolved = 0;
            unsigned int num_unfulfilled = 0;
            for (const auto & prereq : prereqs) {
                if (prereq.state()==unsolved) {
                    num_unsolved++;
                } else if (prereq.state()==unfulfilled) {
                    num_unfulfilled++;
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
        }
    }
    nodevars.emplace("node-prereqs", prereqs_str);

    std::string node_color;
    if (node_ptr->is_active()) {
        node_color = "w3-light-grey";
    } else {
        node_color = "w3-dark-grey";
    }
    nodevars.emplace("node-color", node_color);

    std::string include_filter_substr;
    if (!filter_substring.empty()) {
        include_filter_substr = "&F="+uri_encoded_filter_substring;
    }
    if (show_completed) {
        include_filter_substr += "&I=true";
    }
    nodevars.emplace("filter-substr", include_filter_substr);

    // For each node: Create a Kanban card and add it to the output HTML.
    rendered_cards += env.render(templates[node_alt_card_temp], nodevars);
    return true;
}

bool nodeboard::get_column(const std::string & column_header, const std::string & rendered_cards, std::string & rendered_columns, const std::string extra_header = "") {
    // For each column: Set up a map of content to template position IDs.
    template_varvalues column;
    column.emplace("column-id", column_header);
    column.emplace("column-extra", extra_header);
    column.emplace("column-cards", rendered_cards);

    // For each node: Create a Kanban card and add it to the output HTML.
    rendered_columns += env.render(templates[kanban_column_temp], column);
    num_columns++;
    return true;
}

bool nodeboard::get_alt_column(const std::string & column_header, const std::string & rendered_cards, std::string & rendered_columns, const std::string extra_header = "") {
    // For each column: Set up a map of content to template position IDs.
    template_varvalues column;
    column.emplace("column-id", column_header);
    column.emplace("column-extra", extra_header);
    column.emplace("column-cards", rendered_cards);

    // For each node: Create a Kanban card and add it to the output HTML.
    rendered_columns += env.render(templates[kanban_alt_column_temp], column);
    num_columns++;
    return true;
}

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

//bool nodeboard::get_fulldepth_dependencies_column(const std::string & column_header, const base_Node_Set & column_set, std::string & rendered_columns, const std::string extra_header = "") {
bool nodeboard::get_fulldepth_dependencies_column(const std::string & column_header, Node_ID_key column_key, std::string & rendered_columns, const std::string extra_header = "") {
    if (!map_of_subtrees.is_subtree_head(column_key)) {
        standard_error("Node "+column_key.str()+" is not a head Node in NNL '"+map_of_subtrees.subtrees_list_name+"', skipping", __func__);
        return false;
    }

    std::string rendered_cards;

    float tot_required_hrs = 0;
    float tot_completed_hrs = 0;
    // Add all dependencies found to the column as cards.
    //for (const auto & depkey : map_of_subtrees.get_subtree_set(column_key)) {
    for (const auto & [tdate, depnode_ptr] : map_of_subtrees.get_subtree_set(column_key).tdate_node_pointers) {
        //Node_ptr depnode_ptr = graph().Node_by_id(depkey);
        if (!depnode_ptr) {
            standard_error("Dependency Node not found, skipping", __func__);
            continue;
        }
        if (!get_Node_alt_card(depnode_ptr, tdate, rendered_cards)) {
            standard_error("Dependency Node not found in Graph, skipping", __func__);
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

    return get_alt_column(column_header, rendered_cards, rendered_columns, extra_header_with_progress);
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

bool nodeboard::make_multi_column_board(const std::string & rendered_columns, bool use_alt_board, bool specify_rows) {
    template_varvalues board;
    // Insert the HTML for all the cards into the Kanban board.
    std::string column_widths;
    for (unsigned int i=0; i<num_columns; i++) {
        column_widths += " 240px";
    }
    //column_widths = " 270px repeat("+std::to_string(num_columns)+")";
    board.emplace("board-header", board_title);
    board.emplace("board-extra", board_title_extra);
    board.emplace("column-widths", column_widths);
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
    if (use_alt_board) {
        rendered_board = env.render(templates[kanban_alt_board_temp], board);
    } else {
        rendered_board = env.render(templates[kanban_board_temp], board);
    }

    return to_output(rendered_board);
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

std::string with_and_without_inactive_Nodes_buttons(const std::string & flow_request, const std::string & extra_options, bool current_page_shows_inactive) {
    std::string with_inactive(
        "<button class=\"button button1\" onclick=\"window.open('/cgi-bin/nodeboard-cgi.py?"
        + flow_request
        + extra_options
        + "&I=true'");
    std::string without_inactive(
        "<button class=\"button button1\" onclick=\"window.open('/cgi-bin/nodeboard-cgi.py?"
        + flow_request
        + extra_options
        + "'");
    if (current_page_shows_inactive) {
        with_inactive += ",'_self');\">Refresh</button>";
        without_inactive += ");\">Exclude Completed/Inactive</button>";
        with_inactive += without_inactive;
        return with_inactive;
    } else {
        with_inactive += ");\">Include Completed/Inactive</button>";
        without_inactive += ",'_self');\">Refresh</button>";
        without_inactive += with_inactive;
        return without_inactive;
    }
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

    nb.post_extra = with_and_without_inactive_Nodes_buttons("n="+nb.node_ptr->get_id_str(), include_filter_substr, nb.show_completed);

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

//bool node_board_render_NNL_dependencies(Named_Node_List_ptr namedlist_ptr, nodeboard & nb) {
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

    nb.board_title_extra = with_and_without_inactive_Nodes_buttons("D="+nb.list_name, threads_option, nb.show_completed);

    if (nb.threads) {
        nb.board_title_extra +=
            "<b>Threads</b>:<br>"
            "Where @VISOUTPUT: ...@ is defined in Node content it is shown as the expected advantageous non-internal (visible) output of a thread.<br>"
            "Otherwise, an excerpt of Node content is shown as the thread header.<br>"
            "The Nodes in a thread should be a clear set of steps leading to the output.";
    }

    nb.map_of_subtrees.sort_by_targetdate = true;
    nb.map_of_subtrees.collect(nb.graph(), nb.list_name);
    if (!nb.map_of_subtrees.has_subtrees) return false;

    std::string rendered_columns;

    for (const auto & nkey : namedlist_ptr->list) {

        // Get the next thread header Node.
        Node * node_ptr = nb.graph().Node_by_id(nkey);

        std::string idtext(nkey.str());
        std::string headnode_id_link = "<a href=\"/cgi-bin/fzlink.py?id="+idtext+"\">"+idtext+"</a>";

        std::string htmltext(node_ptr->get_text().c_str());

        std::string headnode_description;
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

        nb.get_fulldepth_dependencies_column(headnode_description, nkey, rendered_columns, headnode_id_link);

    }

    return nb.make_multi_column_board(rendered_columns, true);
}

struct Node_Grid_Element {
    unsigned int col_pos = 0;
    unsigned int row_pos = 0;
    unsigned int span = 1;
    const Node * node_ptr;
    const Node * above;

    Node_Grid_Element(const Node & node, const Node * _above, unsigned int _colpos, unsigned int _rowpos): col_pos(_colpos), row_pos(_rowpos), node_ptr(&node), above(_above) {}
};

struct Node_Grid_Row {
    unsigned int legit_row;
    unsigned int row_number;

    Node_Grid_Row(unsigned int _rownumber): legit_row(12345), row_number(_rownumber) {}
    bool is_legit_row() const { return legit_row == 12345; }
};

struct Node_Grid {
    unsigned int col_total = 0;
    unsigned int row_total = 0;
    std::map<Node_ID_key, Node_Grid_Element> nodes;
    std::vector<Node_Grid_Row> rows;

    Node_Grid(const Node & top_node, bool superiors = false) {
        extend();

        add(top_node, nullptr, 0, 0); //add(row, top_node, nullptr, 0, 0);

        if (superiors) {
            add_superiors(top_node, 0, 0);
        } else {
            add_dependencies(top_node, 0, 0);
        }
        find_node_spans(rows.size()-1);
    }

    unsigned int num_elements() { return nodes.size(); }

    bool is_in_grid(const Node & node) const {
        return nodes.find(node.get_id().key()) != nodes.end();
    }

    Node_Grid_Element & add(const Node & node, const Node * above, unsigned int _colpos, unsigned int _rowpos) {
        Node_Grid_Element element(node, above, _colpos, _rowpos);
        auto it = nodes.emplace(node.get_id().key(), element).first;
        Node_Grid_Element & placed_element = (*it).second;
        return placed_element;
    }

    Node_Grid_Element & add_to_row(unsigned int row_idx, unsigned int col_idx, const Node & node, const Node & above) {
        return add(node, &above, col_idx, row_idx);
    }

    Node_Grid_Row & extend() {
        Node_Grid_Row row(rows.size());
        rows.emplace_back(row);
        row_total = rows.size();
        return rows.back();
    }

    unsigned int add_dependencies(const Node & node, unsigned int node_row, unsigned int node_col) {
        unsigned int dep_row = node_row + 1;
        if (dep_row >= rows.size()) {
            extend();
        }

        // Collect node dependencies as elements in the row.
        unsigned int rel_dep_idx = 0;
        const Edges_Set & dep_edges = node.dep_Edges();
        for (const auto & edge_ptr : dep_edges) {
            if (edge_ptr) {
                Node * dep_ptr = edge_ptr->get_dep();
                if (dep_ptr) {
                    if (!is_in_grid(*dep_ptr)) { // Unique.
                        // Add to row.
                        add_to_row(dep_row, node_col, *dep_ptr, node);
                        if (node_col > col_total) {
                            col_total = node_col;
                        }
                        // Depth first search.
                        node_col = add_dependencies(*dep_ptr, dep_row, node_col);
                    }
                    rel_dep_idx++;
                    if (rel_dep_idx < dep_edges.size()) {
                        node_col++;
                    }
                }
            }
        }
        return node_col;
    }

    unsigned int add_superiors(const Node & node, unsigned int node_row, unsigned int node_col) {
        unsigned int sup_row = node_row + 1;
        if (sup_row >= rows.size()) {
            extend();
        }

        // Collect node superiors as elements in the row.
        unsigned int rel_sup_idx = 0;
        const Edges_Set & sup_edges = node.sup_Edges();
        for (const auto & edge_ptr : sup_edges) {
            if (edge_ptr) {
                Node * sup_ptr = edge_ptr->get_sup();
                if (sup_ptr) {
                    if (!is_in_grid(*sup_ptr)) { // Unique.
                        // Add to row.
                        add_to_row(sup_row, node_col, *sup_ptr, node);
                        if (node_col > col_total) {
                            col_total = node_col;
                        }
                        // Depth first search.
                        node_col = add_superiors(*sup_ptr, sup_row, node_col);
                    }
                    rel_sup_idx++;
                    if (rel_sup_idx < sup_edges.size()) {
                        node_col++;
                    }
                }
            }
        }
        return node_col;
    }

    void find_node_spans(unsigned int row_idx) {
        for (auto & [key, element] : nodes) {
            if (element.above) {
                auto it_above = nodes.find(element.above->get_id().key()); // Find by key, because the map gets rebuilt when more space is needed during element adding.
                if (it_above != nodes.end()) {
                    Node_Grid_Element & above = it_above->second;
                    if ((above.col_pos+(above.span - 1)) < element.col_pos) {
                        above.span = (element.col_pos - above.col_pos) + 1;
                        //FZOUT("col_pos="+std::to_string(above.col_pos)+" span="+std::to_string(above.span)+'\n'); std::cout.flush();
                    }
                }
            }
        }
    }

};

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

    Node_Grid grid(*nb.node_ptr);

    if (!nb.render_init()) {
        return false;
    }

    if (!nb.board_title_specified) {
        nb.board_title = "Dependencies tree of Node "+nb.node_ptr->get_id_str();
    }

    std::string threads_option;
    if (nb.threads) threads_option = "&T=true";

    nb.board_title_extra = with_and_without_inactive_Nodes_buttons("G=true&n="+nb.node_ptr->get_id_str(), threads_option, nb.show_completed);

    // if (nb.threads) {
    //     nb.board_title_extra +=
    //         "<b>Tree</b>:<br>"
    //         "Where @VISOUTPUT: ...@ is defined in Node content it is shown as the expected advantageous non-internal (visible) output of a thread.<br>"
    //         "Otherwise, an excerpt of Node content is shown as the thread header.<br>"
    //         "The Nodes in a thread should be a clear set of steps leading to the output.";
    // }

    std::string rendered_grid;

    for (const auto & [nkey, element] : grid.nodes) {

        std::string rendered_card;
        if (!nb.get_Node_alt_card(element.node_ptr, element.node_ptr->get_targetdate(), rendered_card)) {
            standard_error("Node not found in Graph, skipping", __func__);
            continue;
        }
        if (!rendered_card.empty()) {
            rendered_grid += into_grid(element.row_pos, element.col_pos, element.span, rendered_card);
        }

    }

    nb.num_columns = grid.col_total;
    nb.num_rows = grid.row_total;

    // FZOUT("Number of rows: "+std::to_string(grid.row_total)+'\n');
    // FZOUT("Number of columns: "+std::to_string(grid.col_total)+'\n');
    // FZOUT("Number of elements: "+std::to_string(grid.num_elements())+'\n');

    // FZOUT("Drawing board...\n"); std::cout.flush();

    return nb.make_multi_column_board(rendered_grid, true, true);
}

bool node_board_render_superiors_tree(nodeboard & nb) {
    if (!nb.node_ptr) {
        return false;
    }

    Node_Grid grid(*nb.node_ptr, true);

    if (!nb.render_init()) {
        return false;
    }

    if (!nb.board_title_specified) {
        nb.board_title = "Superiors tree of Node "+nb.node_ptr->get_id_str();
    }

    std::string threads_option;
    if (nb.threads) threads_option = "&T=true";

    nb.board_title_extra = with_and_without_inactive_Nodes_buttons("g=true&n="+nb.node_ptr->get_id_str(), threads_option, nb.show_completed);

    // if (nb.threads) {
    //     nb.board_title_extra +=
    //         "<b>Tree</b>:<br>"
    //         "Where @VISOUTPUT: ...@ is defined in Node content it is shown as the expected advantageous non-internal (visible) output of a thread.<br>"
    //         "Otherwise, an excerpt of Node content is shown as the thread header.<br>"
    //         "The Nodes in a thread should be a clear set of steps leading to the output.";
    // }

    std::string rendered_grid;

    for (const auto & [nkey, element] : grid.nodes) {

        std::string rendered_card;
        if (!nb.get_Node_alt_card(element.node_ptr, element.node_ptr->get_targetdate(), rendered_card)) {
            standard_error("Node not found in Graph, skipping", __func__);
            continue;
        }
        if (!rendered_card.empty()) {
            rendered_grid += into_grid(element.row_pos, element.col_pos, element.span, rendered_card);
        }

    }

    nb.num_columns = grid.col_total;
    nb.num_rows = grid.row_total;

    // FZOUT("Number of rows: "+std::to_string(grid.row_total)+'\n');
    // FZOUT("Number of columns: "+std::to_string(grid.col_total)+'\n');
    // FZOUT("Number of elements: "+std::to_string(grid.num_elements())+'\n');

    // FZOUT("Drawing board...\n"); std::cout.flush();

    return nb.make_multi_column_board(rendered_grid, true, true);
}
