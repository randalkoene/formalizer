// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Node Board rendering functions.
 * 
 * For more about this, see the Trello card at https://trello.com/c/w2XnEQcc
 */

#include "error.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "Graphtypes.hpp"
#include "templater.hpp"


/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif


using namespace fz;


enum template_id_enum {
    node_board_temp,
    node_card_temp,
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "Node_board",
    "Node_card"
};

typedef std::map<template_id_enum,std::string> nodeboard_templates;

bool load_templates(nodeboard_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i] + ".template.html", templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

bool node_board_render(Graph & graph) {
    render_environment env;
    nodeboard_templates templates;
    load_templates(templates);

    Node_Index nodeindex = graph.get_Indexed_Nodes();
    std::string rendered_cards;

    for (int i=0; i<21; ++i) {

        int r = rand() % graph.num_Nodes();

        Node &node = *(nodeindex[r]);

        template_varvalues nodevars;
        nodevars.emplace("node-id", node.get_id_str());
        nodevars.emplace("node-text", node.get_text());
        rendered_cards += env.render(templates[node_card_temp], nodevars);
    }

    template_varvalues board;
    board.emplace("the-nodes",rendered_cards);
    std::string rendered_board = env.render(templates[node_board_temp], board);

    if (!string_to_file("/var/www/html/formalizer/test_node_card.html",rendered_board))
            ERRRETURNFALSE(__func__,"unable to write rendered card to file");
    
    return true;
}
