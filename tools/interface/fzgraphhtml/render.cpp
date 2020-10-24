// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Template rendering functions.
 * 
 */

//#define USE_COMPILEDPING

// core
#include "error.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "html.hpp"
#include "templater.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"

// local
#include "render.hpp"
#include "fzgraphhtml.hpp"


/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif


using namespace fz;


enum template_id_enum {
    node_pars_in_list_html_temp,
    node_pars_in_list_head_html_temp,
    node_pars_in_list_tail_html_temp,
    node_txt_temp,
    node_html_temp,
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "node_pars_in_list_template.html",
    "node_pars_in_list_head_template.html",
    "node_pars_in_list_tail_template.html",
    "Node_template.txt",
    "Node_template.html"
};

typedef std::map<template_id_enum,std::string> fzgraphhtml_templates;

bool load_templates(fzgraphhtml_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i], templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

bool render_incomplete_nodes() {

    Graph * graph_ptr = graphmemman.find_Graph_in_shared_memory();
    if (!graph_ptr) {
        ADDERROR(__func__, "Memory resident Graph not found");
        FZERR("Memory resident Graph not found.\n");
        standard.exit(exit_general_error);
    }

    targetdate_sorted_Nodes incomplete_nodes = Nodes_incomplete_by_targetdate(*graph_ptr);
    unsigned int num_render = (fzgh.config.num_to_show > incomplete_nodes.size()) ? incomplete_nodes.size() : fzgh.config.num_to_show;

    render_environment env;
    fzgraphhtml_templates templates;
    load_templates(templates);

    std::string rendered_page;
    rendered_page.reserve(num_render * (2 * templates[node_pars_in_list_html_temp].size()) +
                          templates[node_pars_in_list_head_html_temp].size() +
                          templates[node_pars_in_list_tail_html_temp].size());
    rendered_page += templates[node_pars_in_list_head_html_temp];

    for (const auto & [tdate, node_ptr] : incomplete_nodes) {

        if (node_ptr) {
            template_varvalues varvals;
            varvals.emplace("node_id",node_ptr->get_id_str());
            Topic * topic_ptr = graph_ptr->main_Topic_of_Node(*node_ptr);
            if (topic_ptr) {
                varvals.emplace("topic",topic_ptr->get_tag());
            } else {
                varvals.emplace("topic","MISSING TOPIC!");
            }
            varvals.emplace("targetdate",TimeStampYmdHM(tdate));
            varvals.emplace("req_hrs",to_precision_string(((double) node_ptr->get_required())/3600.0));
            varvals.emplace("tdprop",td_property_str[node_ptr->get_tdproperty()]);
            std::string htmltext(node_ptr->get_text().c_str());
            varvals.emplace("excerpt",remove_html_tags(htmltext).substr(0,fzgh.config.excerpt_length));
            //varvals.emplace("excerpt",remove_html(htmltext).substr(0,fzgh.config.excerpt_length));

            rendered_page += env.render(templates[node_pars_in_list_html_temp], varvals);

        }

        if (--num_render == 0)
            break;
    }

    rendered_page += templates[node_pars_in_list_tail_html_temp];

    if (fzgh.config.rendered_out_path == "STDOUT") {
        FZOUT(rendered_page);
    } else {
        if (!string_to_file(fzgh.config.rendered_out_path,rendered_page))
            ERRRETURNFALSE(__func__,"unable to write rendered page to file");
    }
    
    return true;
}

/**
 * Individual Node data rendering.
 * 
 * Note: We will probably be unifying this with the Node
 * rendering code of `fzquerypq`, even though the data here
 * comes from the memory-resident Graph. See the proposal
 * at https://trello.com/c/jJamMykM. When unifying these, 
 * the output rendering format may be specified by an enum
 * as in the code in fzquerypq:servenodedata.cpp.
 * 
 * The rendering format is specified in `fzq.output_format`.
 * 
 * @param node A valid Node object.
 * @param render_format Specifies the rendering output format.
 * @return A string with rendered Node data according to the chosen format.
 */
std::string render_Node_data(Node & node, unsigned int render_format) {
    render_environment env;
    fzgraphhtml_templates templates;

    load_templates(templates);

    template_varvalues nodevars;
    nodevars.emplace("node-id", node.get_id_str());
    nodevars.emplace("node-text", node.get_text());

    switch (render_format) {

    case 1:
        return env.render(templates[node_html_temp], nodevars);

    default:
        return env.render(templates[node_txt_temp], nodevars);

    }

    return ""; // never gets here
}