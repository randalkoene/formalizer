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
    node_pars_in_list_card_temp,
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "node_pars_in_list_template.html",
    "node_pars_in_list_head_template.html",
    "node_pars_in_list_tail_template.html",
    "Node_template.txt",
    "Node_template.html",
    "node_pars_in_list_card_template.html"
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

/**
 * Call this to render parameters of a Node on a single line of a list of Nodes.
 * For example, this selection of data is shown when Nodes are listed in a schedule.
 * 
 * @param rendered_page String to which the rendered line is appended.
 * @param graph Reference to the Graph in which the Node resides.
 * @param node Reference to the Node to render.
 * @param tdate Target date to show (e.g. effective target date or locally specified target date).
 * @param env Rendering environment in use.
 * @param templates Loaded rendering templates in use.
 */
void render_Node_pars_on_list_line(std::string & rendered_page, const Graph & graph, const Node & node, time_t tdate, render_environment & env, fzgraphhtml_templates & templates) {
    // *** this can be more efficient if rendered_page, graph, env and templates are all in fzgh.
    template_varvalues varvals;
    varvals.emplace("node_id",node.get_id_str());
    Topic * topic_ptr = graph.main_Topic_of_Node(node);
    if (topic_ptr) {
        varvals.emplace("topic",topic_ptr->get_tag());
    } else {
        varvals.emplace("topic","MISSING TOPIC!");
    }
    varvals.emplace("targetdate",TimeStampYmdHM(tdate));
    varvals.emplace("req_hrs",to_precision_string(((double) node.get_required())/3600.0));
    varvals.emplace("tdprop",td_property_str[node.get_tdproperty()]);
    std::string htmltext(node.get_text().c_str());
    varvals.emplace("excerpt",remove_html_tags(htmltext).substr(0,fzgh.config.excerpt_length));
    //varvals.emplace("excerpt",remove_html(htmltext).substr(0,fzgh.config.excerpt_length));

    if (fzgh.test_cards) {
        rendered_page += env.render(templates[node_pars_in_list_card_temp], varvals);
    } else {
        rendered_page += env.render(templates[node_pars_in_list_html_temp], varvals);
    }
}

void prepare_to_render_Nodes_in_list(unsigned int num_render, fzgraphhtml_templates & templates, std::string & rendered_page) {
    // *** this can be more efficient if rendered_page, graph, env and templates are all in fzgh.
    load_templates(templates);
    rendered_page.reserve(num_render * (2 * templates[node_pars_in_list_html_temp].size()) +
                          templates[node_pars_in_list_head_html_temp].size() +
                          templates[node_pars_in_list_tail_html_temp].size());
    // *** this is where an option to include or not include HTML head and tail frame could be added
    rendered_page += templates[node_pars_in_list_head_html_temp];
}

bool present_rendered_Nodes_in_list(std::string & rendered_page, fzgraphhtml_templates & templates) {
    // *** this can be more efficient if rendered_page, graph, env and templates are all in fzgh.
    // *** this is where an option to include or not include HTML head and tail frame could be added
    rendered_page += templates[node_pars_in_list_tail_html_temp];

    if (fzgh.config.rendered_out_path == "STDOUT") {
        FZOUT(rendered_page);
    } else {
        if (!string_to_file(fzgh.config.rendered_out_path,rendered_page))
            ERRRETURNFALSE(__func__,"unable to write rendered page to file");
    }
    return true;
}

bool render_incomplete_nodes() {

    Graph * graph_ptr = graphmemman.find_Graph_in_shared_memory();
    if (!graph_ptr) {
        standard_exit_error(exit_general_error, "Memory resident Graph not found.", __func__);
    }

    targetdate_sorted_Nodes incomplete_nodes = Nodes_incomplete_by_targetdate(*graph_ptr);
    unsigned int num_render = (fzgh.config.num_to_show > incomplete_nodes.size()) ? incomplete_nodes.size() : fzgh.config.num_to_show;

    render_environment env;
    fzgraphhtml_templates templates;
    std::string rendered_page;
    prepare_to_render_Nodes_in_list(num_render, templates, rendered_page);

    for (const auto & [tdate, node_ptr] : incomplete_nodes) {

        if (node_ptr) {
            render_Node_pars_on_list_line(rendered_page, *graph_ptr, *node_ptr, tdate, env, templates);
        }

        if (--num_render == 0)
            break;
    }

    return present_rendered_Nodes_in_list(rendered_page, templates);
}

bool render_named_node_list_names(const Graph & graph) {
    std::vector<std::string> list_names_vec = graph.get_List_names();

    unsigned int num_render = (fzgh.config.num_to_show > list_names_vec.size()) ? list_names_vec.size() : fzgh.config.num_to_show;

    render_environment env;
    fzgraphhtml_templates templates;
    std::string rendered_page;
    prepare_to_render_Nodes_in_list(num_render, templates, rendered_page);

    for (const auto & name : list_names_vec) {

        rendered_page += "\n<tr><td>" + name + "</td></tr>\n";

    }

    return present_rendered_Nodes_in_list(rendered_page, templates);
}

bool render_named_node_list() {

    Graph * graph_ptr = graphmemman.find_Graph_in_shared_memory();
    if (!graph_ptr) {
        standard_exit_error(exit_general_error, "Memory resident Graph not found.", __func__);
    }

    if (fzgh.list_name == "?") {
        return render_named_node_list_names(*graph_ptr);
    }

    Named_Node_List_ptr namedlist_ptr = graph_ptr->get_List(fzgh.list_name);
    if (!namedlist_ptr) {
        standard_exit_error(exit_general_error, "Named Node List "+fzgh.list_name+" not found.", __func__);
    }

    unsigned int num_render = (fzgh.config.num_to_show > namedlist_ptr->list.size()) ? namedlist_ptr->list.size() : fzgh.config.num_to_show;

    render_environment env;
    fzgraphhtml_templates templates;
    std::string rendered_page;
    prepare_to_render_Nodes_in_list(num_render, templates, rendered_page);

    for (const auto & nkey : namedlist_ptr->list) {

        Node * node_ptr = graph_ptr->Node_by_id(nkey);
        if (node_ptr) {
            render_Node_pars_on_list_line(rendered_page, *graph_ptr, *node_ptr, node_ptr->effective_targetdate(), env, templates);
        } else {
            standard_error("Node "+nkey.str()+" not found in Graph, skipping", __func__);
        }

        if (--num_render == 0)
            break;
    }

    return present_rendered_Nodes_in_list(rendered_page, templates);
}

std::string render_Node_topics(Graph & graph, Node & node) {
    auto topictagsvec = Topic_tags_of_Node(graph, node);
    std::string topics_str;
    int i = 0;
    for (const auto & [tagstr, tagrel] : topictagsvec) {
        if (i>0) {
            topics_str += ", ";
        }
        topics_str += tagstr + " (" + to_precision_string(tagrel,1) + ')';
        ++i;
    }
    return topics_str;
}

std::string render_Node_superiors(Graph & graph, Node & node) {
    std::string sups_str;
    for (const auto & edge_ptr : node.sup_Edges()) {
        if (edge_ptr) {
            sups_str += "<li><a href=\"/cgi-bin/fzlink.py?id="+edge_ptr->get_sup_str()+"\">" + edge_ptr->get_sup_str() + "</a>: ";
            Node * sup_ptr = edge_ptr->get_sup();
            if (!sup_ptr) {
                ADDERROR(__func__, "Node "+node.get_id_str()+" has missing superior at Edge "+edge_ptr->get_id_str());
            } else {
                std::string htmltext(sup_ptr->get_text().c_str());
                sups_str += remove_html_tags(htmltext).substr(0,fzgh.config.excerpt_length);
            }
            sups_str += "</li>\n";
        }
    }
    return sups_str;
}

std::string render_Node_dependencies(Graph & graph, Node & node) {
    std::string deps_str;
    for (const auto & edge_ptr : node.dep_Edges()) {
        if (edge_ptr) {
            deps_str += "<li><a href=\"/cgi-bin/fzlink.py?id="+edge_ptr->get_dep_str()+"\">" + edge_ptr->get_dep_str() + "</a>: ";
            Node * dep_ptr = edge_ptr->get_dep();
            if (!dep_ptr) {
                ADDERROR(__func__, "Node "+node.get_id_str()+" has missing dependency at Edge "+edge_ptr->get_id_str());
            } else {
                std::string htmltext(dep_ptr->get_text().c_str());
                deps_str += remove_html_tags(htmltext).substr(0,fzgh.config.excerpt_length);
            }
            deps_str += "</li>\n";
        }
    }
    return deps_str;
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
 * @param graph A valid Graph.
 * @param node A valid Node object.
 * @param render_format Specifies the rendering output format.
 * @return A string with rendered Node data according to the chosen format.
 */
std::string render_Node_data(Graph & graph, Node & node, unsigned int render_format) {
    render_environment env;
    fzgraphhtml_templates templates;

    load_templates(templates);

    template_varvalues nodevars;
    long required_mins = node.get_required() / 60;
    double required_hrs = (double) required_mins / 60.0;
    td_property tdprop = node.get_tdproperty();
    td_pattern tdpatt = node.get_tdpattern();

    nodevars.emplace("node-id", node.get_id_str());
    nodevars.emplace("node-text", node.get_text());
    nodevars.emplace("comp", to_precision_string(node.get_completion()));
    nodevars.emplace("req_hrs", to_precision_string(required_hrs));
    nodevars.emplace("req_mins", std::to_string(required_mins));
    nodevars.emplace("val", to_precision_string(node.get_valuation()));
    nodevars.emplace("eff_td", TimeStampYmdHM(node.effective_targetdate()));
    if (tdprop<_tdprop_num) {
        nodevars.emplace("td_prop", td_property_str[tdprop]);
    } else {
        nodevars.emplace("td_prop", "(unrecognized)");
        ADDERROR(__func__, "Node "+node.get_id_str()+" has unrecognized TD property "+std::to_string((int) tdprop));
    }
    if (tdpatt<_patt_num) {
        nodevars.emplace("td_patt", td_pattern_str[tdpatt]);
    } else {
        nodevars.emplace("td_patt", "(unrecognized)");
        ADDERROR(__func__, "Node "+node.get_id_str()+" has unrecognized TD pattern "+std::to_string((int) tdpatt));
    }
    nodevars.emplace("td_every", std::to_string(node.get_tdevery()));
    nodevars.emplace("td_span", std::to_string(node.get_tdspan()));
    nodevars.emplace("topics", render_Node_topics(graph, node));
    nodevars.emplace("superiors", render_Node_superiors(graph, node));
    nodevars.emplace("dependencies", render_Node_dependencies(graph, node));

    switch (render_format) {

    case 1:
        return env.render(templates[node_html_temp], nodevars);

    default:
        return env.render(templates[node_txt_temp], nodevars);

    }

    return ""; // never gets here
}