// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Functions that serve Node data for `fzquerypq`.
 * 
 */

// corelib
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "templater.hpp"

// local
#include "fzquerypq.hpp"
#include "servenodedata.hpp"

/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif

using namespace fz;


enum template_id_enum {
    node_txt_temp,
    node_html_temp,
    node_NUM_temp
};

const std::vector<std::string> template_ids = {
    "Node_txt",
    "Node_html"
};

typedef std::map<template_id_enum,std::string> nodeboard_templates;

bool load_templates(nodeboard_templates & templates) {
    templates.clear();

    for (int i = 0; i < node_NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i] + ".template.html", templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

std::string render_Node_data(Node & node) {
    render_environment env;
    nodeboard_templates templates;

    load_templates(templates);

    template_varvalues nodevars;
    nodevars.emplace("node-id", node.get_id_str());
    nodevars.emplace("node-text", node.get_text());

    switch (fzq.output_format) {

    case output_html:
        return env.render(templates[node_html_temp], nodevars);

    default:
        return env.render(templates[node_txt_temp], nodevars);

    }

    return ""; // never gets here
}

void serve_request_Node_data() {
    if (fzq.node_idstr.empty()) {
        ADDERROR(__func__,"empty Node ID string");
        FZERR("empty Node ID string");
        exit(exit_command_line_error);
    }

    ERRHERE(".loadGraph");
    //std::unique_ptr<Graph> graph = fzq.ga.request_Graph_copy();
    Graph * graph = fzq.ga.request_Graph_copy(true, false);
    if (!graph) {
        ADDERROR(__func__,"unable to load Graph");
        return;
    }

    ERRHERE(".findNode");
    Node * nodeptr = graph->Node_by_idstr(fzq.node_idstr);
    ERRHERE(".render");
    if (nodeptr) {
        FZOUT(render_Node_data(*nodeptr));

    } else {
        FZERR("invalid Node ID: "+fzq.node_idstr);
    }

    VERYVERBOSEOUT(graphmemman.info_str());
    VERYVERBOSEOUT(Graph_Info_str(*graph));
}
