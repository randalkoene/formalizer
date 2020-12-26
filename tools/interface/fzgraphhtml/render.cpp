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

std::vector<std::string> template_ids = {
    "node_pars_in_list_template",
    "node_pars_in_list_head_template",
    "node_pars_in_list_tail_template",
    "named_node_list_in_list_template",
    "Node_template",
    "node_pars_in_list_card_template",
    "topic_pars_in_list_template",
    "Node_edit_template"
};

typedef std::map<template_id_enum,std::string> fzgraphhtml_templates;

bool load_templates(fzgraphhtml_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (template_ids[i].front() == '/') { // we need this in case a custom template was specified
            if (!file_to_string(template_ids[i], templates[static_cast<template_id_enum>(i)]))
                ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
        } else {
            std::string format_subdir, format_ext;
            switch (fzgh.config.outputformat) {

                case output_txt: {
                    format_subdir = "/txt/";
                    format_ext = ".txt";
                    break;
                }

                case output_node: {
                    format_subdir = "/node/";
                    format_ext = ".node";
                    break;
                }

                case output_desc: {
                    format_subdir = "/desc/";
                    format_ext = ".desc";
                    break;
                }

                default: { // html
                    format_subdir = "/html/";
                    format_ext = ".html";
                }
            }
            std::string template_path(template_dir + format_subdir + template_ids[i] + format_ext);
            if (!file_to_string(template_path, templates[static_cast<template_id_enum>(i)]))
                ERRRETURNFALSE(__func__, "unable to load " + template_path);
        }
    }

    return true;
}

struct line_render_parameters {
    Graph * graph_ptr;               ///< Pointer to the Graph in which the Node resides.
    const std::string srclist;       ///< The Named Node List being rendered (or "" when that is not the case).
    render_environment env;          ///< Rendering environment in use.
    fzgraphhtml_templates templates; ///< Loaded rendering templates in use.
    std::string rendered_page;       ///< String to which the rendered line is appended.
    std::string datestamp;

    line_render_parameters(const std::string _srclist, const char * problem__func__) : srclist(_srclist) {
        graph_ptr = graphmemman.find_Graph_in_shared_memory();
        if (!graph_ptr) {
            standard_exit_error(exit_general_error, "Memory resident Graph not found.", problem__func__);
        }
        if (!load_templates(templates)) {
            standard_exit_error(exit_file_error, "Missing template file.", problem__func__);
        }
    }

    Graph & graph() { return *graph_ptr; }

    void prep(unsigned int num_render) {
        if (fzgh.config.embeddable) {
            rendered_page.reserve(num_render * (2 * templates[node_pars_in_list_temp].size()));
        } else {
            rendered_page.reserve(num_render * (2 * templates[node_pars_in_list_temp].size()) +
                            templates[node_pars_in_list_head_temp].size() +
                            templates[node_pars_in_list_tail_temp].size());
            rendered_page += templates[node_pars_in_list_head_temp];
        }
    }

    void insert_day_start(time_t t) {
        if ((fzgh.config.outputformat == output_txt) || (fzgh.config.outputformat == output_html)) {
            template_varvalues varvals;
            varvals.emplace("node_id","");
            varvals.emplace("topic","");
            varvals.emplace("targetdate","<b>"+datestamp+"</b>");
            varvals.emplace("req_hrs","");
            varvals.emplace("tdprop","");
            varvals.emplace("excerpt","<b>"+WeekDay(t)+"</b>");
            varvals.emplace("fzserverpq","");
            varvals.emplace("srclist","");
            rendered_page += env.render(templates[node_pars_in_list_temp], varvals);
        }
    }

    std::string render_tdproperty(td_property tdprop) {
        std::string tdpropstr;
        tdpropstr.reserve(20);
        bool boldit = (tdprop==fixed) || (tdprop==exact);
        if (boldit) {
            tdpropstr += "<b>";
        }
        tdpropstr += td_property_str[tdprop];
        if (boldit) {
            tdpropstr += "</b>";
        }
        return tdpropstr;
    }

    /**
     * Call this to render parameters of a Node on a single line of a list of Nodes.
     * For example, this selection of data is shown when Nodes are listed in a schedule.
     * 
     * @param node Reference to the Node to render.
     * @param tdate Target date to show (e.g. effective target date or locally specified target date).
     * @param showdate Insert date and day of week if true.
     */
    void render_Node(const Node & node, time_t tdate, bool showdate = true) {
        template_varvalues varvals;
        varvals.emplace("node_id",node.get_id_str());
        Topic * topic_ptr = graph_ptr->main_Topic_of_Node(node);
        if (topic_ptr) {
            varvals.emplace("topic",topic_ptr->get_tag());
        } else {
            varvals.emplace("topic","MISSING TOPIC!");
        }
        std::string tdstamp(TimeStampYmdHM(tdate));
        if (showdate && (tdstamp.substr(0,8) != datestamp)) {
            datestamp = tdstamp.substr(0,8);
            insert_day_start(tdate);
        }
        varvals.emplace("targetdate",tdstamp);
        if (fzgh.config.show_still_required) {
            varvals.emplace("req_hrs",to_precision_string(node.hours_to_complete()));
        } else {
            varvals.emplace("req_hrs",to_precision_string(node.get_required_hours()));
        }

        varvals.emplace("tdprop",render_tdproperty(node.get_tdproperty()));
        std::string htmltext(node.get_text().c_str());
        varvals.emplace("excerpt",remove_html_tags(htmltext).substr(0,fzgh.config.excerpt_length));
        //varvals.emplace("excerpt",remove_html(htmltext).substr(0,fzgh.config.excerpt_length));
        varvals.emplace("fzserverpq",graph_ptr->get_server_full_address());
        varvals.emplace("srclist",srclist);

        if (fzgh.test_cards) {
            rendered_page += env.render(templates[node_pars_in_list_card_temp], varvals);
        } else {
            rendered_page += env.render(templates[node_pars_in_list_temp], varvals);
        }
    }

    /**
     * Call this to render parameters of a Topic on a single line of a list of Topics.
     * 
     * @param topic Reference to the Topic to render.
     */
    void render_Topic(const Topic & topic) {
        template_varvalues varvals;
        varvals.emplace("topic_id",std::to_string(topic.get_id()));
        varvals.emplace("sup_id",std::to_string(topic.get_supid()));
        varvals.emplace("tag",topic.get_tag().c_str());
        varvals.emplace("title",topic.get_title().c_str());
        varvals.emplace("keyrel",topic.keyrel_str());
        if (fzgh.add_to_node) {
            varvals.emplace("add_to_node","<td>[<a href=\"http://"+fzgh.graph().get_server_full_address()+"/fz/graph/nodes/"+fzgh.node_idstr+"/topics/add?"+topic.get_tag().c_str()+"=1.0\">add to "+fzgh.node_idstr+"</a>]</td>");
        } else {
            varvals.emplace("add_to_node","");
        }

        rendered_page += env.render(templates[topic_pars_in_list_temp], varvals);
    }

    void render_List(const std::string & list_name) {
        template_varvalues varvals;
        varvals.emplace("list_name",list_name);
        varvals.emplace("fzserverpq",graph_ptr->get_server_full_address());
        Named_Node_List_ptr nnl_ptr = graph_ptr->get_List(list_name);
        if (nnl_ptr) {
            varvals.emplace("size",std::to_string(nnl_ptr->size()));
            if (!nnl_ptr->get_maxsize()) {
                varvals.emplace("maxsize", "unlimited");
            } else {
                varvals.emplace("maxsize", std::to_string(nnl_ptr->get_maxsize()));
            }
            std::string feature_str;
            if (nnl_ptr->prepend())
                feature_str += "prepend,";
            if (nnl_ptr->unique())
                feature_str += "unique,";
            if (nnl_ptr->fifo())
                feature_str += "fifo,";
            if (!feature_str.empty()) {
                feature_str.back() = ')';
                feature_str.insert(0,"(");
            }
            varvals.emplace("features", feature_str);
        }
        rendered_page += env.render(templates[named_node_list_in_list_temp], varvals);
    }

    bool present() {
        if (!fzgh.config.embeddable) {
            rendered_page += templates[node_pars_in_list_tail_temp];
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

    targetdate_sorted_Nodes incomplete_nodes = Nodes_incomplete_by_targetdate(lrp.graph()); // *** could grab a cache here
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

bool render_incomplete_nodes_with_repeats() {

    line_render_parameters lrp("",__func__);

    targetdate_sorted_Nodes incomplete_nodes = Nodes_incomplete_by_targetdate(lrp.graph()); // *** could grab a cache here
    targetdate_sorted_Nodes incnodes_with_repeats = Nodes_with_repeats_by_targetdate(incomplete_nodes, fzgh.config.t_max, fzgh.config.num_to_show);
    unsigned int num_render = (fzgh.config.num_to_show > incnodes_with_repeats.size()) ? incnodes_with_repeats.size() : fzgh.config.num_to_show;

    lrp.prep(num_render);

    for (const auto & [tdate, node_ptr] : incnodes_with_repeats) {

        if (node_ptr) {
            lrp.render_Node(*node_ptr, tdate);
        }

        if (--num_render == 0)
            break;
    }

    return lrp.present();
}

bool render_named_node_list_names(line_render_parameters & lrp) {
    std::vector<std::string> list_names_vec = lrp.graph().get_List_names();

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

    Named_Node_List_ptr namedlist_ptr = lrp.graph().get_List(fzgh.list_name);
    if (!namedlist_ptr) {
        standard_exit_error(exit_general_error, "Named Node List "+fzgh.list_name+" not found.", __func__);
    }

    unsigned int num_render = (fzgh.config.num_to_show > namedlist_ptr->list.size()) ? namedlist_ptr->list.size() : fzgh.config.num_to_show;

    lrp.prep(num_render);

    for (const auto & nkey : namedlist_ptr->list) {

        Node * node_ptr = lrp.graph().Node_by_id(nkey);
        if (node_ptr) {
            lrp.render_Node(*node_ptr, node_ptr->effective_targetdate(), false);
        } else {
            standard_error("Node "+nkey.str()+" not found in Graph, skipping", __func__);
        }

        if (--num_render == 0)
            break;
    }

    return lrp.present();
}

bool render_topics() {

    line_render_parameters lrp("",__func__);

    auto topics = lrp.graph().get_topics();

    //unsigned int num_render = (fzgh.config.num_to_show > topics.num_Topics()) ? topics.num_Topics() : fzgh.config.num_to_show;
    unsigned int num_render = topics.num_Topics();

    lrp.prep(num_render);

    for (const auto & [t_str, t_ptr] : topics.get_topicbytag()) {

        Topic * topic_ptr = t_ptr.get();
        if (topic_ptr) {
            lrp.render_Topic(*topic_ptr);
        } else {
            standard_error(("Topic "+t_str+" not found in Graph, skipping").c_str(), __func__);
        }

        if (--num_render == 0)
            break;
    }

    return lrp.present();
}

bool render_topic_nodes() {

    line_render_parameters lrp("",__func__);

    targetdate_sorted_Nodes topic_nodes = Nodes_with_topic_by_targetdate(lrp.graph(), fzgh.topic_id); // *** could grab a cache here
    //unsigned int num_render = (fzgh.config.num_to_show > topic_nodes.size()) ? topic_nodes.size() : fzgh.config.num_to_show;
    unsigned int num_render = topic_nodes.size();

    lrp.prep(num_render);

    for (const auto & [tdate, node_ptr] : topic_nodes) {

        if (node_ptr) {
            lrp.render_Node(*node_ptr, tdate, false);
        }

        if (--num_render == 0)
            break;
    }

    return lrp.present();
}

std::string render_Node_topics(Graph & graph, Node & node, bool remove_button = false) {
    auto topictagsvec = Topic_tags_of_Node(graph, node);
    std::string topics_str;
    int i = 0;
    for (const auto & [tagstr, tagrel] : topictagsvec) {
        if (i>0) {
            topics_str += ", ";
        }
        if (remove_button) {
            topics_str += tagstr + " [<a href=\"/fz/graph/nodes/" + node.get_id_str() + "/topics/remove?" + tagstr + "=\">remove</a>] (" + to_precision_string(tagrel,1) + ')';
        } else {
            topics_str += tagstr + " (" + to_precision_string(tagrel,1) + ')';
        }
        ++i;
    }
    return topics_str;
}

std::string render_Node_superiors(Graph & graph, Node & node, bool remove_button = false) {
    std::string sups_str;
    for (const auto & edge_ptr : node.sup_Edges()) {
        if (edge_ptr) {
            if (fzgh.config.outputformat == output_node) {
                sups_str += edge_ptr->get_sup_str() + ' ';
            } else {
                sups_str += "<li><a href=\"/cgi-bin/fzlink.py?id="+edge_ptr->get_sup_str()+"\">" + edge_ptr->get_sup_str() + "</a>: ";
                Node * sup_ptr = edge_ptr->get_sup();
                if (!sup_ptr) {
                    ADDERROR(__func__, "Node "+node.get_id_str()+" has missing superior at Edge "+edge_ptr->get_id_str());
                } else {
                    std::string htmltext(sup_ptr->get_text().c_str());
                    sups_str += remove_html_tags(htmltext).substr(0,fzgh.config.excerpt_length);
                }
                if (remove_button) {
                    sups_str += "[<a href=\"/fz/graph/nodes/" + node.get_id_str() + "/superiors/remove?" + edge_ptr->get_sup_str() + "=\">remove</a>]";
                }
                sups_str += "</li>\n";
            }
        }
    }
    return sups_str;
}

std::string render_Node_dependencies(Graph & graph, Node & node, bool remove_button = false) {
    std::string deps_str;
    for (const auto & edge_ptr : node.dep_Edges()) {
        if (edge_ptr) {
            if (fzgh.config.outputformat == output_node) {
                deps_str += edge_ptr->get_dep_str() + ' ';
            } else {
                deps_str += "<li><a href=\"/cgi-bin/fzlink.py?id="+edge_ptr->get_dep_str()+"\">" + edge_ptr->get_dep_str() + "</a>: ";
                Node * dep_ptr = edge_ptr->get_dep();
                if (!dep_ptr) {
                    ADDERROR(__func__, "Node "+node.get_id_str()+" has missing dependency at Edge "+edge_ptr->get_id_str());
                } else {
                    std::string htmltext(dep_ptr->get_text().c_str());
                    deps_str += remove_html_tags(htmltext).substr(0,fzgh.config.excerpt_length);
                }
                if (remove_button) {
                    deps_str += "[<a href=\"/fz/graph/nodes/" + node.get_id_str() + "/superiors/remove?" + edge_ptr->get_sup_str() + "=\">remove</a>]";
                }
                deps_str += "</li>\n";
            }
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
 * @param graph A valid Graph.
 * @param node A valid Node object.
 * @return A string with rendered Node data according to the chosen format.
 */
std::string render_Node_data(Graph & graph, Node & node) {
    render_environment env;
    fzgraphhtml_templates templates;

    load_templates(templates);

    template_varvalues nodevars;
    long required_mins = node.get_required() / 60;
    double required_hrs = (double) required_mins / 60.0;
    td_property tdprop = node.get_tdproperty();
    td_pattern tdpatt = node.get_tdpattern();

    nodevars.emplace("node-id", node.get_id_str());
    nodevars.emplace("fzserverpq",graph.get_server_full_address());
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

    return env.render(templates[node_temp], nodevars);
}

const std::string prop_token[_tdprop_num] = {
    "prop_unspecified",
    "prop_inherit",
    "prop_variable",
    "prop_fixed",
    "prop_exact"
};

const std::string patt_token[_patt_num] = {
    "patt_daily",
    "patt_workdays",
    "patt_weekly",
    "patt_biweekly",
    "patt_monthly",
    "patt_endofmonthoffset",
    "patt_yearly",
    "OLD_patt_span",
    "patt_nonperiodic"
};

bool render_node_edit() {
    ERRTRACE;

    auto [node_ptr, graph_ptr] = find_Node_by_idstr(fzgh.node_idstr, fzgh.graph_ptr);
    if (!graph_ptr) {
        standard_exit_error(exit_resident_graph_missing, "Unable to access memory-resident Graph.", __func__);
    }
    if (!node_ptr) {
        standard_exit_error(exit_bad_request_data, "Invalid Node ID: "+fzgh.node_idstr, __func__);
    }
    Graph & graph = *graph_ptr;
    Node & node = *node_ptr;
    
    // this is where you do the same as in render_Node_data(), with a few extra bits
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

    time_t t_eff = node.effective_targetdate();
    nodevars.emplace("eff_td", TimeStampYmdHM(t_eff));
    nodevars.emplace("eff_td_date",TimeStamp("%Y-%m-%d", t_eff));
    nodevars.emplace("eff_td_time",TimeStamp("%H:%M", t_eff));

    std::string prop_value[_tdprop_num] = {"", "", "", "", ""};
    if (tdprop<_tdprop_num) {
        prop_value[tdprop] = "checked";
    } else {
        ADDERROR(__func__, "Node "+node.get_id_str()+" has unrecognized TD property "+std::to_string((int) tdprop));
    }
    for (int i = 0; i < _tdprop_num; ++i) {
        nodevars.emplace(prop_token[i],prop_value[i]);
    }

    std::string patt_value[_patt_num] = {"", "", "", "", "", "", "", "", ""};
    if (tdpatt<_patt_num) {
        patt_value[tdpatt] = "checked";
    } else {
        ADDERROR(__func__, "Node "+node.get_id_str()+" has unrecognized TD pattern "+std::to_string((int) tdpatt));
    }
    for (int i = 0; i < _patt_num; ++i) {
        if (i != OLD_patt_span) {
            nodevars.emplace(patt_token[i],patt_value[i]);
        }
    }

    nodevars.emplace("td_every", std::to_string(node.get_tdevery()));
    nodevars.emplace("td_span", std::to_string(node.get_tdspan()));
    nodevars.emplace("fzserverpq", graph.get_server_full_address());
    nodevars.emplace("topics", render_Node_topics(graph, node, true));
    nodevars.emplace("superiors", render_Node_superiors(graph, node, true));
    nodevars.emplace("dependencies", render_Node_dependencies(graph, node, true));

    std::string rendered_node_data = env.render(templates[node_edit_temp], nodevars);

    if (fzgh.config.rendered_out_path == "STDOUT") {
        FZOUT(rendered_node_data);
    } else {
        if (!string_to_file(fzgh.config.rendered_out_path, rendered_node_data))
            standard_exit_error(exit_file_error, "Unable to write rendered page to file.", __func__);
    }

    return true;
}
