// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the nbrender part of the
 * nodeboard tool.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __NBRENDER_HPP.
 */

#ifndef __NBRENDER_HPP
#include "version.hpp"
#define __NBRENDER_HPP (__VERSION_HPP)

#include "Graphtypes.hpp"
#include "Graphaccess.hpp"
#include "Graphinfo.hpp"
#include "templater.hpp"

#define DEFAULTMAXCOLS 100
#define DEFAULTMAXROWS 100

using namespace fz;

enum flow_options {
    flow_unknown = 0,                 ///< no recognized request
    flow_node = 1,                    ///< request: show data for Node
    flow_named_list = 4,              ///< request: show Nodes in Named Node List
    flow_topics = 5,                  ///< request: show Topics
    flow_topic_nodes = 6,             ///< request: show Nodes with Topic
    flow_random_test = 8,
    flow_listof_NNL = 9,
    flow_listof_topics = 10,
    flow_listof_mixed = 11,
    flow_sysmet_categories = 12,
    flow_NNL_dependencies = 13,       ///< request: show dependencies of Nodes in Named Node List
    flow_dependencies_tree = 14,      ///< request: show tree of Node dependencies in a grid
    flow_superiors_tree = 15,         ///< request: show tree of Node dependencies in a grid
    flow_csv_schedule = 16,           ///< request: show schedule based on CSV file
    flow_NUMoptions
};

enum template_id_enum {
    node_board_temp,
    node_card_temp,
    kanban_board_temp,
    kanban_column_temp,
    kanban_alt_column_temp,
    kanban_alt_board_temp,
    node_alt_card_temp,
    schedule_card_temp,
    schedule_board_temp,
    schedule_column_temp,
    node_analysis_card_temp,
    NUM_temp
};

enum Node_render_result {
    node_render_error = -1,
    node_not_rendered = 0,
    node_rendered_inactive = 1,
    node_rendered_active = 2,
    NUM_Noderenderresult
};

typedef std::map<template_id_enum,std::string> nodeboard_templates;

struct CSV_Data {
    std::string start_date;
    std::string start_time;
    unsigned int num_minutes;
    char tdprop;
    Node * node_ptr;

    CSV_Data(Graph & graph, const std::string & csv_line);
};

struct CSV_Data_Day {
    std::vector<CSV_Data> day;

};

struct nodeboard: public formalizer_standard_program {
    Graph_access ga;

    flow_options flowcontrol;

    Node * node_ptr = nullptr;
    std::string source_file;
    std::string list_name;
    std::vector<std::string> list_names_vec;

    Map_of_Subtrees map_of_subtrees;

    int excerpt_length = 160;

    Topic_ID topic_id = 0;

    bool show_completed = false;
    bool show_zero_required = false;
    bool threads = false;
    bool progress_analysis = false;

    time_t t_now = 0;

    bool detect_tdorder = false;
    bool detect_tdfar = false;
    bool detect_tdbad = false;

    bool show_dependencies_tree = false;
    bool show_superiors_tree = false;

    time_t t_before = 0;

    std::string progress_state_file;
    unsigned long node_total = 0;
    unsigned long progress_node_count = 0;
    int last_percentage_state = -1;

    bool highlight_topic_and_valuation = false;
    Topic_ID highlight_topic = 0;

    std::vector<Topic_ID> filter_topics;
    std::string uri_encoded_filter_topics;

    std::string filter_substring;
    int filter_substring_excerpt_length = 80;
    std::string uri_encoded_filter_substring;

    bool board_title_specified = false;
    std::string board_title; // Set to -f name or "Kanban Board" if not provided.
    std::string board_title_extra;
    std::string post_extra;

    std::string grid_column_width = " 260px";
    std::string column_container_width = "250px";
    std::string card_width = "240px";
    std::string card_height = "240px";

    unsigned int max_columns = DEFAULTMAXCOLS;
    unsigned int max_rows = DEFAULTMAXROWS;

    bool minimize_grid = true;

    std::string output_path;

    std::vector<CSV_Data_Day> csv_data_vec;

    float vertical_multiplier = 1.0;

    Graph *graph_ptr;

    Log_filter filter;

    render_environment env;
    nodeboard_templates templates;

    unsigned int num_columns = 0;
    unsigned int num_rows = 0;

    nodeboard();

    virtual void usage_hook();

    bool set_grid_and_card_sizes(const std::string & cargs);

    bool parse_error_detection_list(const std::string & cargs);

    virtual bool options_hook(char c, std::string cargs);

    bool parse_list_names(const std::string & arg);

    bool parse_filter_topics(const std::string & arg);

    bool parse_header_identifier(const std::string & arg, std::string & header, std::string & identifier);

    bool parse_csv(const std::string & csv_data);

    Graph & graph();

    std::string build_nodeboard_cgi_call(flow_options _floption, bool _threads, bool _showcompleted, bool _progressanalysis, float multiplier = 1.0, unsigned int maxcols = DEFAULTMAXCOLS, unsigned int maxrows = DEFAULTMAXROWS);

    bool render_init();

    bool to_output(const std::string & rendered_board);

    bool filtered_out(const Node * node_ptr, std::string & node_text) const;

    bool get_Node_card(const Node * node_ptr, std::string & rendered_cards);

    Node_render_result get_Node_alt_card(const Node * node_ptr, std::time_t tdate, std::string & rendered_cards, Node_Subtree * subtree_ptr = nullptr);

    float get_card_vertical_position(const std::string & start_time, float v_offset = 4.0);

    float get_card_height(unsigned int minutes);

    bool get_Schedule_card(const CSV_Data & entry_data, std::string & rendered_cards);

    bool get_column(const std::string & column_header, const std::string & rendered_cards, std::string & rendered_columns, const std::string extra_header, template_id_enum column_template);

    //bool get_alt_column(const std::string & column_header, const std::string & rendered_cards, std::string & rendered_columns, const std::string extra_header);

    bool get_dependencies_column(const std::string & column_header, const Node * column_node, std::string & rendered_columns, const std::string extra_header);

    unsigned long get_Node_total_minutes_applied(const Node_ID_key nkey);

    void progress_state_update();

    std::string tosup_todep_html_buttons(const Node_ID_key & column_key);

    bool get_fulldepth_dependencies_column(std::string & column_header, Node_ID_key column_key, std::string & rendered_columns, const std::string extra_header);

    bool get_NNL_column(const std::string & nnl_str, std::string & rendered_columns);

    bool get_Topic_column(const std::string & topic_str, std::string & rendered_columns);

    bool get_day_column(unsigned int day_idx, std::string & rendered_columns);

    bool make_simple_grid_board(const std::string & rendered_cards);

    std::string call_comment_string();

    bool make_multi_column_board(const std::string & rendered_columns, template_id_enum board_template = kanban_board_temp, bool specify_rows = false, const std::string & col_width = " 240px", const std::string & container_width = "230px", const std::string & card_width = "220px", const std::string & card_height = "390px");

    std::string with_and_without_inactive_Nodes_buttons();

};

bool node_board_render_random_test(nodeboard & nb);

bool node_board_render_dependencies(nodeboard & nb);

bool node_board_render_named_list(Named_Node_List_ptr namedlist_ptr, nodeboard & nb);

bool node_board_render_list_of_named_lists(nodeboard & nb);

bool node_board_render_list_of_topics(nodeboard & nb);

bool node_board_render_list_of_topics_and_NNLs(nodeboard & nb);

bool node_board_render_sysmet_categories(nodeboard & nb);

bool node_board_render_NNL_dependencies(nodeboard & nb);

bool node_board_render_dependencies_tree(nodeboard & nb);

bool node_board_render_superiors_tree(nodeboard & nb);

bool node_board_render_csv_schedule(nodeboard & nb);

#endif // __NBRENDER_HPP
