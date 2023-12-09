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
#include "templater.hpp"

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
    flow_NUMoptions
};

enum template_id_enum {
    node_board_temp,
    node_card_temp,
    kanban_board_temp,
    kanban_column_temp,
    NUM_temp
};

typedef std::map<template_id_enum,std::string> nodeboard_templates;

struct nodeboard: public formalizer_standard_program {
    Graph_access ga;

    flow_options flowcontrol;

    Node * node_ptr = nullptr;
    std::string source_file;
    std::string list_name;
    std::vector<std::string> list_names_vec;

    int excerpt_length = 160;

    Topic_ID topic_id = 0;

    bool show_completed = false;

    bool board_title_specified = false;
    std::string board_title; // Set to -f name or "Kanban Board" if not provided.
    std::string board_title_extra;

    std::string output_path;

    Graph *graph_ptr;

    render_environment env;
    nodeboard_templates templates;

    unsigned int num_columns = 0;

    nodeboard();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    bool parse_list_names(const std::string & arg);

    bool parse_header_identifier(const std::string & arg, std::string & header, std::string & identifier);

    Graph & graph();

    bool render_init();

    bool to_output(const std::string & rendered_board);

    bool get_Node_card(const Node * node_ptr, std::string & rendered_cards);

    bool get_column(const std::string & column_header, const std::string & rendered_cards, std::string & rendered_columns, const std::string extra_header);

    bool get_dependencies_column(const std::string & column_header, const Node * column_node, std::string & rendered_columns, const std::string extra_header);

    bool get_NNL_column(const std::string & nnl_str, std::string & rendered_columns);

    bool get_Topic_column(const std::string & topic_str, std::string & rendered_columns);

    bool make_simple_grid_board(const std::string & rendered_cards);

    bool make_multi_column_board(const std::string & rendered_cards);

};

bool node_board_render_random_test(nodeboard & nb);

bool node_board_render_dependencies(nodeboard & nb);

bool node_board_render_named_list(Named_Node_List_ptr namedlist_ptr, nodeboard & nb);

bool node_board_render_list_of_named_lists(nodeboard & nb);

bool node_board_render_list_of_topics(nodeboard & nb);

bool node_board_render_list_of_topics_and_NNLs(nodeboard & nb);

bool node_board_render_sysmet_categories(nodeboard & nb);

#endif // __NBRENDER_HPP
