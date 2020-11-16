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
    named_node_list_in_list_html_temp,
    node_txt_temp,
    node_html_temp,
    node_pars_in_list_card_temp,
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "node_pars_in_list_template.html",
    "node_pars_in_list_head_template.html",
    "node_pars_in_list_tail_template.html",
    "named_node_list_in_list_template.html",
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

struct line_render_parameters {
    Graph &graph;                    ///< Reference to the Graph in which the Node resides.
    const std::string srclist;       ///< The Named Node List being rendered (or "" when that is not the case).
    render_environment env;          ///< Rendering environment in use.
    fzgraphhtml_templates templates; ///< Loaded rendering templates in use.
    std::string rendered_page;       ///< String to which the rendered line is appended.

    line_render_parameters(const std::string _srclist, const char * problem__func__) : srclist(_srclist) {
        Graph * graph_ptr = graphmemman.find_Graph_in_shared_memory();
        if (!graph_ptr) {
            standard_exit_error(exit_general_error, "Memory resident Graph not found.", problem__func__);
        }
        graph = *graph_ptr;
        if (!load_templates(templates)) {
            standard_exit_error(exit_file_error, "Missing template file.", problem__func__);
        }
    }

    void prep(unsigned int num_render) {
        if (fzgh.config.embeddable) {
            rendered_page.reserve(num_render * (2 * templates[node_pars_in_list_html_temp].size()));
        } else {
            rendered_page.reserve(num_render * (2 * templates[node_pars_in_list_html_temp].size()) +
                            templates[node_pars_in_list_head_html_temp].size() +
                            templates[node_pars_in_list_tail_html_temp].size());
            rendered_page += templates[node_pars_in_list_head_html_temp];
        }
    }

    /**
     * Call this to render parameters of a Node on a single line of a list of Nodes.
     * For example, this selection of data is shown when Nodes are listed in a schedule.
     * 
     * @param node Reference to the Node to render.
     * @param tdate Target date to show (e.g. effective target date or locally specified target date).
     */
    void render_Node(const Node & node, time_t tdate) {
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
        varvals.emplace("fzserverpq","aether.local:8090"); // *** get this elsewhere!
        varvals.emplace("srclist",srclist);

        if (fzgh.test_cards) {
            rendered_page += env.render(templates[node_pars_in_list_card_temp], varvals);
        } else {
            rendered_page += env.render(templates[node_pars_in_list_html_temp], varvals);
        }
    }

    void render_List(const std::string & list_name) {
        template_varvalues varvals;
        varvals.emplace("list_name",list_name);
        rendered_page += env.render(templates[named_node_list_in_list_html_temp], varvals);
    }

    bool present() {
        if (!fzgh.config.embeddable) {
            rendered_page += templates[node_pars_in_list_tail_html_temp];
        }

        if (fzgh.config.rendered_out_path == "STDOUT") {
            FZOUT(rendered_page);
        } else {
            if (!string_to_file(fzgh.config.rendered_out_path,rendered_page))
                ERRRETURNFALSE(__func__,"unable to write rendered page to file");
        }
        return true;
    }

};

bool render_incomplete_nodes() {

    line_render_parameters lrp("",__func__);

    targetdate_sorted_Nodes incomplete_nodes = Nodes_incomplete_by_targetdate(lrp.graph);
    unsigned int num_render = (fzgh.config.num_to_show > incomplete_nodes.size()) ? incomplete_nodes.size() : fzgh.config.num_to_show;

    lrp.prep(num_render);

    for (const auto & [tdate, node_ptr] : incomplete_nodes) {

        if (node_ptr) {
            lrp.render_Node(*node_ptr, tdate);
        }

        if (--num_render == 0)
            break;
    }

    return lrp.present();
}

bool render_named_node_list_names(line_render_parameters & lrp) {
    std::vector<std::string> list_names_vec = lrp.graph.get_List_names();

    unsigned int num_render = (fzgh.config.num_to_show > list_names_vec.size()) ? list_names_vec.size() : fzgh.config.num_to_show;

    lrp.prep(num_render);

    for (const auto & name : list_names_vec) {

        lrp.render_List(name);

    }

    return lrp.present();
}

bool render_named_node_list() {

    line_render_parameters lrp(fzgh.list_name,__func__);

    if (fzgh.list_name == "?") {
        return render_named_node_list_names(lrp);
    }

    Named_Node_List_ptr namedlist_ptr = lrp.graph.get_List(fzgh.list_name);
    if (!namedlist_ptr) {
        standard_exit_error(exit_general_error, "Named Node List "+fzgh.list_name+" not found.", __func__);
    }

    unsigned int num_render = (fzgh.config.num_to_show > namedlist_ptr->list.size()) ? namedlist_ptr->list.size() : fzgh.config.num_to_show;

    lrp.prep(num_render);

    for (const auto & nkey : namedlist_ptr->list) {

        Node * node_ptr = lrp.graph.Node_by_id(nkey);
        if (node_ptr) {
            lrp.render_Node(*node_ptr, node_ptr->effective_targetdate());
        } else {
            standard_error("Node "+nkey.str()+" not found in Graph, skipping", __func__);
        }

        if (--num_render == 0)
            break;
    }

    return lrp.present();
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