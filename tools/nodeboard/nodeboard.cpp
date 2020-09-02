// Copyright 2020 Randal A. Koene
// License TBD

/**
 * nodeboard is a simple prototype for casting Nodes onto dashboard-like web pages
 * in the form of cards.
 * 
 * For more about this, see the Trello card at https://trello.com/c/w2XnEQcc
 */

#define FORMALIZER_MODULE_ID "Formalizer:Visualization:Nodes:HTMLcards"

#include <iostream>

#include "version.hpp"
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "Graphtypes.hpp"
#include "Graphpostgres.hpp"
#include "templater.hpp"

/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif

using namespace fz;

struct nodeboard: public formalizer_standard_program {
    Graph_access ga;

    nodeboard() {
         add_usage_top += ga.usage_top;
    }

    virtual void usage_hook() {
        ga.usage_hook();
    }

    virtual void options_hook(char c, std::string cargs) {
        switch (c) {

        case 'd':
            ga.dbname = cargs;
            break;
        }
    }
} nb;

enum template_id_enum {
    node_card_temp,
    NUM_temp
};

const std::vector<std::string> template_ids = {
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


int main(int argc, char *argv[]) {
    nb.init(argc,argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);

    FZOUT("\nThis is a test. Let's go get the Graph, so that we have some Nodes to work with.\n\n");
    key_pause();

    std::unique_ptr<Graph> graph = nb.ga.request_Graph_copy();

    FZOUT("\nThe Graph has "+std::to_string(graph->num_Nodes())+" Nodes.\n\n");

    render_environment env;
    nodeboard_templates templates;
    load_templates(templates);

    Node & node = *((--(graph->end_Nodes()))->second);

    template_varvalues nodevars;
    nodevars.emplace("node-id",node.get_id_str());
    nodevars.emplace("node-text",node.get_text());
    std::string rendered_card = env.render(templates[node_card_temp],nodevars);

    if (!string_to_file("/var/www/html/formalizer/test_node_card.html",rendered_card))
            ERRRETURNFALSE(__func__,"unable to write rendered card to file");

    return nb.completed_ok();
}
