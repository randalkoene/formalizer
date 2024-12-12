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
#include "btfdays.hpp"

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

#define INCLUDE_SKIP_BUTTON

using namespace fz;

std::vector<std::string> template_ids = {
    "node_pars_in_list_template",
    "node_pars_in_list_nojs_template",
    "node_pars_in_list_head_template",
    "node_pars_in_list_tail_template",
    "named_node_list_in_list_template",
    "Node_template",
    "node_pars_in_list_card_template",
    "topic_pars_in_list_template",
    "Node_edit_template",
    "Node_new_template",
    "node_pars_in_list_with_remove_template",
    "Node_BTF_template"
};

typedef std::map<template_id_enum,std::string> fzgraphhtml_templates;

/**
 * This templates loading function recognizes special cases for custom
 * templates:
 * - "/something" is interpreted as a custom template path.
 * - "STRING:something" is interpreted as a custom template directly in the string.
 */
bool load_templates(fzgraphhtml_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (template_ids[i].front() == '/') { // we need this in case a custom template was specified
            if (!file_to_string(template_ids[i], templates[static_cast<template_id_enum>(i)])) {
                ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
            }
        } else {
            if ((template_ids[i].size()>6) && (template_ids[i][6]==':')) { // expecting a custom template in the string itself
                if (template_ids[i].substr(0,7) != "STRING:") {
                    ERRRETURNFALSE(__func__, "incorrect 'STRING:' based custome template syntax: " + template_ids[i]);
                }
                templates[static_cast<template_id_enum>(i)] = template_ids[i].substr(7);
            } else {
                std::string format_subdir, format_ext;
                switch (fzgh.config.outputformat) {

                    case output_txt: {
                        format_subdir = "/txt/";
                        format_ext = ".txt";
                        break;
                    }

                    case output_json: {
                        format_subdir = "/json/";
                        format_ext = ".json";
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
                if (!file_to_string(template_path, templates[static_cast<template_id_enum>(i)])) {
                    ERRRETURNFALSE(__func__, "unable to load " + template_path);
                }
            }
        }
    }

    return true;
}

const std::map<Boolean_Tag_Flags::boolean_flag, std::string> category_tag_str = {
    { Boolean_Tag_Flags::none, "" },
    { Boolean_Tag_Flags::work, "<span class=\"bold-blue\">W</span>" },
    { Boolean_Tag_Flags::self_work, "<span class=\"bold-green\">S</span>" },
    { Boolean_Tag_Flags::system, "<span class=\"bold-green\">C</span>" },
};

const std::map<td_property, std::string> td_property_class_map = {
    { unspecified, " tdp_U" },
    { inherit, " tdp_I" },
    { variable, " tdp_V" },
    { fixed, " tdp_F" },
    { exact, " tdp_E" },
};

/**
 * Notes:
 * 1. Rendering of the head of the page is done after collecting the rendered
 *    Nodes, because some of the information in the head may require information
 *    that is collected along the way, such as 'days_rendered'.
 */
struct line_render_parameters {
    Graph *graph_ptr;                ///< Pointer to the Graph in which the Node resides.
    const std::string srclist;       ///< The Named Node List being rendered (or "" when that is not the case).
    render_environment env;          ///< Rendering environment in use.
    fzgraphhtml_templates templates; ///< Loaded rendering templates in use.
    std::string rendered_page;       ///< String to which the rendered line is appended.
    std::string datestamp;
    std::string tdstamp;
    size_t actual_num_render = 0;
    size_t days_rendered = 0;
    time_t t_render = 0; ///< The time when page rendering commenced.
    const Node* prev_ETD_node = nullptr;
    time_t prev_ETD_node_t_endbefore = RTt_unspecified;
    float day_total_hrs = 0.0;
    std::map<Boolean_Tag_Flags::boolean_flag, float> day_category_hrs = {
        {Boolean_Tag_Flags::work, 0.0},
        {Boolean_Tag_Flags::self_work, 0.0},
        {Boolean_Tag_Flags::system, 0.0},
    };
    Map_of_Subtrees map_of_subtrees;
    std::string subtrees_list_tag;

    line_render_parameters(const std::string _srclist, const char * problem__func__) : srclist(_srclist) {
        graph_ptr = graphmemman.find_Graph_in_shared_memory();
        if (!graph_ptr) {
            standard_exit_error(exit_general_error, "Memory resident Graph not found.", problem__func__);
        }
        if (!load_templates(templates)) {
            standard_exit_error(exit_file_error, "Missing template file.", problem__func__);
        }
        t_render = ActualTime();
    }

    Graph & graph() { return *graph_ptr; }

    std::string rendered_head() {
        template_varvalues varvals;
        if (fzgh.config.num_to_show==std::numeric_limits<unsigned int>::max()) {
            varvals.emplace("num_to_show","");
            varvals.emplace("all_checked","checked");
        } else {
            varvals.emplace("num_to_show",std::to_string(fzgh.config.num_to_show));
            varvals.emplace("all_checked","");
        }
        varvals.emplace("actual_num_shown", std::to_string(actual_num_render));
        varvals.emplace("days_shown", std::to_string(days_rendered));
        if (fzgh.num_days > 0) {
            varvals.emplace("num_days",std::to_string(fzgh.num_days));
            varvals.emplace("t_max","");
        } else {
            varvals.emplace("num_days","");
            varvals.emplace("t_max",TimeStampYmdHM(fzgh.config.t_max));            
        }

        varvals.emplace("btf_days", get_btf_days());

        return env.render(templates[node_pars_in_list_head_temp], varvals);
    }

    std::string rendered_tail() {
        template_varvalues varvals;
        time_t t_max_next = fzgh.t_last_rendered + 30*seconds_per_day;
        varvals.emplace("thirty_days_more",TimeStampYmdHM(t_max_next));
        return env.render(templates[node_pars_in_list_tail_temp], varvals);    
    }

    void prep(unsigned int num_render) {
        actual_num_render = num_render;
        if (fzgh.config.embeddable) {
            rendered_page.reserve(num_render * (2 * templates[node_pars_in_list_temp].size()));
        } else {
            rendered_page.reserve(num_render * (2 * templates[node_pars_in_list_temp].size()) +
                            templates[node_pars_in_list_head_temp].size() +
                            templates[node_pars_in_list_tail_temp].size());
        }
        map_of_subtrees.collect(graph(), fzgh.subtrees_list_name);
        if (map_of_subtrees.has_subtrees) {
            subtrees_list_tag = "<b>["+fzgh.subtrees_list_name+"]</b>";
        }
    }

    void insert_previous_day_summary() {
        // Note that datestamp was already TZ adjusted.
        template_varvalues varvals;
        time_t t_nextday = ymd_stamp_time(datestamp); // have to do this, because a Node's tdate is not indicative of the day start
        time_t t_daystart = t_nextday - seconds_per_day; // *** breaks slightly when switching to daylight savings time
        time_t seconds_available = seconds_per_day;
        time_t t_render_tzadjusted = t_render;
        if ((fzgh.config.timezone_offset_hours > 0) && fzgh.config.tzadjust_day_separators) {
            t_render_tzadjusted += (fzgh.config.timezone_offset_hours*3600);
        }
        if ((t_daystart < t_render_tzadjusted) && (t_nextday > t_render_tzadjusted)) {
            seconds_available = t_nextday - t_render_tzadjusted;
        }
        float hours_available = ((float)seconds_available / 3600.0);
        if ((t_nextday < t_render_tzadjusted) || (day_total_hrs > hours_available)) {
            varvals.emplace("alertstyle", " class=\"high_req\"");
        } else {
            varvals.emplace("alertstyle", ""); // " class=\"fit_req\"");
        }
        varvals.emplace("req_hrs", to_precision_string(day_total_hrs,2));
        varvals.emplace("targetdate", to_precision_string(hours_available,2));
        varvals.emplace("node_id","");
        varvals.emplace("topic","");
        varvals.emplace("tdprop","");
        if (map_of_subtrees.has_subtrees) {
            std::string categories_hrs_str;
            for (auto & [ btflag, hrs ]: day_category_hrs) {
                categories_hrs_str += category_tag_str.at(btflag)+": "+to_precision_string(day_category_hrs.at(btflag),2)+' ';
            }
            varvals.emplace("excerpt", categories_hrs_str);
        } else {
            varvals.emplace("excerpt","");
        }
        varvals.emplace("fzserverpq","");
        varvals.emplace("srclist","");
        varvals.emplace("do_link", "");
        rendered_page += env.render(templates[node_pars_in_list_temp], varvals);
        day_total_hrs = 0.0;
        for (auto & [ flag, hours ] : day_category_hrs) {
            day_category_hrs[flag] = 0.0;
        }
    }

    void insert_day_start(time_t t) {
        // Note that datestamp was already TZ adjusted.
        days_rendered += 1;
        if ((fzgh.config.outputformat == output_txt) || (fzgh.config.outputformat == output_html)) {
            if ((fzgh.config.include_daysummary) && (day_total_hrs > 0.0)) {
                insert_previous_day_summary();
            }
            template_varvalues varvals;
            varvals.emplace("node_id","<b>ID</b>");
            varvals.emplace("topic","<b>main topic</b>");
            if ((fzgh.config.timezone_offset_hours > 0) && fzgh.config.tzadjust_day_separators) {
                varvals.emplace("targetdate","<b>"+datestamp+"</b> (tzadj)");
            } else {
                varvals.emplace("targetdate","<b>"+datestamp+"</b>");
            }
            varvals.emplace("alertstyle","");
            varvals.emplace("req_hrs","<b>hrs</b>");
            varvals.emplace("tdprop","");
            varvals.emplace("excerpt","<b>"+WeekDay(t)+"</b>");
            varvals.emplace("fzserverpq","");
            varvals.emplace("srclist","");
            varvals.emplace("do_link", "");
            rendered_page += env.render(templates[node_pars_in_list_temp], varvals);
        }
    }

#ifndef INCLUDE_SKIP_BUTTON
    std::string render_tdproperty(const Node & node) {
#else
    std::string render_tdproperty(const Node & node, bool with_skip_button = false) {
#endif
        std::string tdpropstr;
        tdpropstr.reserve(20);
        td_property tdprop = node.get_tdproperty();
        bool boldit = (tdprop==fixed) || (tdprop==exact);
        if (boldit) {
            tdpropstr += "<b>";
        }
        tdpropstr += td_property_str[tdprop];
        if (boldit) {
            if (node.get_repeats()) {
#ifndef INCLUDE_SKIP_BUTTON
                tdpropstr += '*';
#else
                if (with_skip_button) {
                    tdpropstr += " <button class=\"tiny_green\" onclick=\"window.open('/cgi-bin/fzedit-cgi.py?action=skip&num_skip=1&id="+node.get_id_str()+"','_blank');\">skip</button>";
                } else {
                    tdpropstr += '*';
                }
#endif
            }
            tdpropstr += "</b>";
        }
        return tdpropstr;
    }

    bool visible_time_and_date_with_tz_adjustments(const time_t tdate, time_t & tdate_tzadjusted, std::string & vis_tdstamp_str) {
        // Prepare time and date stamp.
        tdstamp = TimeStampYmdHM(tdate);
        tdate_tzadjusted = tdate;
        if (fzgh.config.timezone_offset_hours==0) {
            vis_tdstamp_str = tdstamp;
        } else {
            // Time zone adjust.
            tdate_tzadjusted += (fzgh.config.timezone_offset_hours*3600);
            // Visible time zone offset.
            if (fzgh.config.timezone_offset_hours > 0) {
                vis_tdstamp_str = TimeStampYmdHM(tdate_tzadjusted)+'-'+std::to_string(fzgh.config.timezone_offset_hours);
            } else {
                vis_tdstamp_str = TimeStampYmdHM(tdate_tzadjusted)+'+'+std::to_string(-fzgh.config.timezone_offset_hours);
            }
        }
        // Return dayseparator status.
        if (fzgh.config.tzadjust_day_separators) {
            return (vis_tdstamp_str.substr(0,8) != datestamp);
        }
        return (tdstamp.substr(0,8) != datestamp);
    }

    // This is rendered into the top line of the list when included.
    void render_counts(size_t num_rendered, size_t num_in_list, bool include_remove_button) {
        template_varvalues varvals;
        if (!fzgh.test_cards) {
            // -- Node ID
            varvals.emplace("node_id", "");
            // -- Optional checkbox
            if (fzgh.config.include_checkboxes) {
                varvals.emplace("checkbox","<td> </td>");
            } else {
                varvals.emplace("checkbox","");
            }
            varvals.emplace("checkbox","");
            // -- Topic
            varvals.emplace("topic","");
            // -- Target date and time zone
            varvals.emplace("targetdate", "");
            varvals.emplace("rawtd", "");
            varvals.emplace("alertstyle", "");
            // -- Required time
            varvals.emplace("req_hrs", "");
            // -- Target date property
            varvals.emplace("tdprop", "");
            // -- Content excerpt
            varvals.emplace("excerpt", "<b>Rendered "+std::to_string(num_rendered)+" of "+std::to_string(num_in_list)+" in list.</b>");
            // -- Server address
            varvals.emplace("fzserverpq", "");
            // -- Do-link
            varvals.emplace("do_link", "");
            // -- Possible source NNL
            varvals.emplace("srclist", "");
            // -- Possible position in NNL
            varvals.emplace("list_pos", "");
            // -- Render
            if (fzgh.no_javascript) {
                rendered_page += env.render(templates[node_pars_in_list_nojs_temp], varvals);
            } else {
                if (include_remove_button) {
                    rendered_page += env.render(templates[node_pars_in_list_with_remove_temp], varvals);
                } else {
                    rendered_page += env.render(templates[node_pars_in_list_temp], varvals);
                }
            }
        }
    }

    // std::string td_property_styling(const std::string& excerpt, const Node& node) {
    //     if (node.td_exact()) {
    //         return "<b>"+excerpt+"</b>";
    //     }
    //     if (node.get_repeats()) {
    //         return "<i>"+excerpt+"</i>";
    //     }
    //     return excerpt;
    // }

    std::string td_property_classes(const Node& node) {
        std::string tdpropclasses;
        if (node.get_repeats()) {
            tdpropclasses += " tdp_R";
        } else {
            if (node.td_fixed() || node.td_exact()) {
                tdpropclasses += " tdp_rEF";
            }
        }
        tdpropclasses += td_property_class_map.at(node.get_tdproperty());
        return tdpropclasses;
    }

    void add_class_to_alertstyle(const std::string& class_str, std::string& alertstyle) {
        if (alertstyle.empty()) {
            alertstyle = " class=\""+class_str+'"';
        } else {
            alertstyle.pop_back();
            alertstyle += ' '+class_str+'"';
        }
    }

    /**
     * Call this to render parameters of a Node on a single line of a list of Nodes.
     * For example, this selection of data is shown when Nodes are listed in a schedule.
     * 
     * Some aspects of rending are controlled by configuration parameters.
     * 
     * @param node Reference to the Node to render.
     * @param tdate Target date to show (e.g. effective target date or locally specified target date).
     * @param showdate Insert date and day of week if true.
     */
    void render_Node(const Node & node, time_t tdate, bool showdate = true, int list_pos = -1, bool remove_button = false) {
        template_varvalues varvals;
        // -- Node ID
        std::string nodestr(node.get_id_str());
        varvals.emplace("node_id",nodestr);
        // -- Optional checkbox
        if (fzgh.config.include_checkboxes) {
            varvals.emplace("checkbox","<td><input type=\"checkbox\" id=\"chkbx_"+nodestr+"\" name=\"chkbx_"+nodestr+"\"></td>");
        } else {
            varvals.emplace("checkbox","");
        }
        // -- Topic
        Topic * topic_ptr = graph_ptr->main_Topic_of_Node(node);
        if (topic_ptr) {
            varvals.emplace("topic",topic_ptr->get_tag());
        } else {
            varvals.emplace("topic","MISSING TOPIC!");
        }
        // -- Prepare target date and time zone
        time_t tdate_tzadjusted;
        std::string vis_tdstamp_str;
        bool day_separator = visible_time_and_date_with_tz_adjustments(tdate, tdate_tzadjusted, vis_tdstamp_str);
        // -- Is this the first Node on the next day, and do we show that?
        if (showdate && day_separator) {
            // *** BEWARE: For very extensive Node time spans, tdate may more than a day out, thereby skipping days!
            //     You should probably actually just keep track of day starts from one day to the next and place
            //     an insert even if there was no targetdate of a Node on a particular day (if a full calendar is
            //     being created rather than a temporally ordered list of Nodes).
            if (fzgh.config.tzadjust_day_separators) {
                datestamp = vis_tdstamp_str.substr(0,8);
                insert_day_start(tdate_tzadjusted);
            } else {
                datestamp = tdstamp.substr(0,8);
                insert_day_start(tdate);
            }
        }
        // -- Detect overlap between ETD Nodes
        std::string alertstyle;
        if (node.td_exact()) {
            if (prev_ETD_node) {
                if ((prev_ETD_node_t_endbefore+node.get_required()) >= tdate) {
                    //add_class_to_alertstyle("textbg-cyan", alertstyle);
                    vis_tdstamp_str = "<span class=\"high_req\">"+vis_tdstamp_str+"</span>";
                }
            }
            prev_ETD_node = &node;
            prev_ETD_node_t_endbefore = tdate;
        }
        // -- Target date (adjusted and raw) and alert style of required time
        if (fzgh.config.show_current_time) {
            if (tdate <= t_render) { // *** This might still need TZADJUST attention.
                //alertstyle = " class=\"passed_td\"";
                add_class_to_alertstyle("passed_td", alertstyle);
                std::string tdstr = "<a href=\"/cgi-bin/fzlink.py?id="+nodestr+"\">"+vis_tdstamp_str+"</a>";
                varvals.emplace("targetdate",tdstr);
            } else {
                if (node.is_special_code()) {
                    //alertstyle = " class=\"inactive_td\"";
                    add_class_to_alertstyle("inactive_td", alertstyle);
                }
                varvals.emplace("targetdate",vis_tdstamp_str);
            }
        } else {
            varvals.emplace("targetdate",vis_tdstamp_str);
        }
        varvals.emplace("rawtd",tdstamp);
        varvals.emplace("alertstyle",alertstyle);
        // -- Required time
        float hours_to_show;
        if (fzgh.config.show_still_required) {
            hours_to_show = node.hours_to_complete(tdate); // This checks if it is a repeat or first instance.
        } else {
            hours_to_show = node.get_required_hours();
        }
        varvals.emplace("req_hrs",to_precision_string(hours_to_show));
        day_total_hrs += hours_to_show;
        // -- Target date property
#ifdef INCLUDE_SKIP_BUTTON
        bool not_a_repeat = (!node.get_repeats()) || (tdate == const_cast<Node*>(&node)->effective_targetdate());
        if (node.get_repeats() && not_a_repeat) { // render tdproperty with skip button
            varvals.emplace("tdprop",render_tdproperty(node, true));
        } else {
            varvals.emplace("tdprop",render_tdproperty(node));
        }
#else
        varvals.emplace("tdprop",render_tdproperty(node));
#endif
        // -- Content excerpt
        Boolean_Tag_Flags::boolean_flag boolean_tag;
        if (map_of_subtrees.node_in_heads_or_any_subtree(node.get_id().key(), boolean_tag)) {
            if (category_tag_str.find(boolean_tag) != category_tag_str.end()) {
                varvals.emplace("excerpt", category_tag_str.at(boolean_tag)+subtrees_list_tag+node.get_excerpt(fzgh.config.excerpt_length));
            } else {
                varvals.emplace("excerpt", node.get_excerpt(fzgh.config.excerpt_length));
            }
            if (day_category_hrs.find(boolean_tag) != day_category_hrs.end()) {
                day_category_hrs.at(boolean_tag) += hours_to_show;
            }
        } else {
            varvals.emplace("excerpt", node.get_excerpt(fzgh.config.excerpt_length));
        }
        varvals.emplace("tdprop-style", td_property_classes(node));
        // -- Server address
        varvals.emplace("fzserverpq", fzgh.replacements[fzserverpq_address]); // graph_ptr->get_server_full_address()
        // -- Do-link
        if (not_a_repeat && ((fzgh.config.max_do_links==0) || (fzgh.do_links_rendered < fzgh.config.max_do_links))) {
            varvals.emplace("do_link", " <a class=\"nnl\" href=\"/cgi-bin/fzlog-cgi.py?action=open&node="+nodestr+"\" target=\"_blank\">[do]</a>");
            fzgh.do_links_rendered++;
        } else {
            varvals.emplace("do_link", "");
        }
        // -- (Possible) source NNL
        varvals.emplace("srclist",srclist);
        // -- (Possible) position in NNL
        if (list_pos<0) {
            varvals.emplace("list_pos", "");
        } else {
            varvals.emplace("list_pos","&list_pos="+std::to_string(list_pos));
        }

        // -- Render
        if (fzgh.test_cards) {
            rendered_page += env.render(templates[node_pars_in_list_card_temp], varvals);
        } else {
            if (fzgh.no_javascript) {
                rendered_page += env.render(templates[node_pars_in_list_nojs_temp], varvals);
            } else {
                if (remove_button) {
                    rendered_page += env.render(templates[node_pars_in_list_with_remove_temp], varvals);
                } else {
                    rendered_page += env.render(templates[node_pars_in_list_temp], varvals);
                }
            }
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
            if (fzgh.node_idstr=="NEW") {
                varvals.emplace("add_to_node","<td>[<a href=\"/cgi-bin/fzgraphhtml-cgi.py?edit=new&topics="+std::string(topic.get_tag().c_str())+"\">add to NEW</a>]</td>");
            } else {
                // fzgh.graph().get_server_full_address()
                varvals.emplace("add_to_node","<td>[<a href=\"http://"+fzgh.replacements[fzserverpq_address]+"/fz/graph/nodes/"+fzgh.node_idstr+"/topics/add?"+topic.get_tag().c_str()+"=1.0\">add to "+fzgh.node_idstr+"</a>]</td>");
            }
        } else {
            varvals.emplace("add_to_node","");
        }

        rendered_page += env.render(templates[topic_pars_in_list_temp], varvals);
    }

    void render_List(const std::string & list_name) {
        template_varvalues varvals;
        varvals.emplace("list_name",list_name);
        varvals.emplace("fzserverpq", fzgh.replacements[fzserverpq_address]); // graph_ptr->get_server_full_address()
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

    bool send_to_output(const std::string& rendered_output) {
        if (fzgh.config.rendered_out_path == "STDOUT") {
            FZOUT(rendered_output);
        } else {
            if (!string_to_file(fzgh.config.rendered_out_path, rendered_output))
                ERRRETURNFALSE(__func__,"unable to write rendered page to file");
        }
        return true;
    }

    /**
     * See Note 1 for the reason why the head is only generated here.
     */
    bool present() {
        if (!fzgh.config.embeddable) {
            std::string rendered_page_with_head_and_tail;
            rendered_page_with_head_and_tail.reserve(3000 + rendered_page.size());
            rendered_page_with_head_and_tail = rendered_head() + rendered_page + rendered_tail();
            return send_to_output(rendered_page_with_head_and_tail);
        } else {
            return send_to_output(rendered_page);
        }
    }

};

bool render_incomplete_nodes() {

    line_render_parameters lrp("",__func__);

    lrp.graph().set_tzadjust_active(fzgh.config.show_tzadjust);

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

    lrp.graph().set_tzadjust_active(false); // One should only activate it temporarily.

    return lrp.present();
}

bool render_incomplete_nodes_with_repeats() {

    line_render_parameters lrp("",__func__);

    lrp.graph().set_tzadjust_active(fzgh.config.show_tzadjust);

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

        fzgh.t_last_rendered = tdate;
    }

    lrp.graph().set_tzadjust_active(false); // One should only activate it temporarily.

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

/**
 * Turn a Named Node List into rendered HTML. Optionally, included HTML
 * page head and tail from templates.
 * Results are sent to one of three targets:
 * 1. STDOUT, if indicated in fzgh.config.rendered_out_path
 * 2. A file, as per fzgh.config.rendered_out_path
 * 3. The internal fzgh.cache_str container for further used by another
 *    function, if indicated by fzgh.cache_it.
 */
bool render_named_node_list() {

    line_render_parameters lrp(fzgh.list_name,__func__);

    if (fzgh.list_name == "?") {
        return render_named_node_list_names(lrp);
    }

    Named_Node_List_ptr namedlist_ptr = lrp.graph().get_List(fzgh.list_name);
    if (!namedlist_ptr) {
        return standard_error("Named Node List "+fzgh.list_name+" not found.", __func__);
    }

    unsigned int num_render = (fzgh.config.num_to_show > namedlist_ptr->list.size()) ? namedlist_ptr->list.size() : fzgh.config.num_to_show;

    lrp.prep(num_render);

    if (fzgh.nnl_with_counter) {
        lrp.render_counts(num_render, namedlist_ptr->list.size(), true);
    }

    if (fzgh.config.sort_by_targetdate) {

        targetdate_sorted_Nodes list_nodes = Nodes_in_list_by_targetdate(lrp.graph(), namedlist_ptr);

        int list_pos = 0;
        for (const auto & [tdate, node_ptr] : list_nodes) {

            if (node_ptr) {
                lrp.render_Node(*node_ptr, tdate, false, list_pos, true);
                list_pos++;
            }

            if (--num_render == 0)
                break;
        }

    } else {

        int list_pos = 0;
        for (const auto & nkey : namedlist_ptr->list) {

            Node * node_ptr = lrp.graph().Node_by_id(nkey);
            if (node_ptr) {
                lrp.render_Node(*node_ptr, node_ptr->effective_targetdate(), false, list_pos, true);
                list_pos++;
            } else {
                standard_error("Node "+nkey.str()+" not found in Graph, skipping", __func__);
            }

            if (--num_render == 0)
                break;
        }

    }

    if (fzgh.cache_it) {
        fzgh.cache_str = std::move(lrp.rendered_page);
        return true;
    } else {
        return lrp.present();
    }
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

std::string render_Node_NNLs(Graph & graph, Node & node) {
    auto nnlsvec = graph.find_all_NNLs_Node_is_in(node);
    std::string nnls_str;
    int i = 0;
    for (const auto & list_name : nnlsvec) {
        if (i>0) {
            nnls_str += ", ";
        }
        nnls_str += list_name;
        ++i;
    }
    return nnls_str;
}

const std::map<bool, std::string> supdep_active_highlight = {
    { false, "inactive-node"},
    { true, "active-node"},
};

const std::map<bool, std::string> supdep_td_highlight = {
    { false, "passed_td"}, // used here to indicate incorrect TD order
    { true, "active-node"},
};

const std::string importance_tooltip =
R"(<div style="width:500px;">The importance parameter of a Node on an Edge in a hierarchy is used when displaying the hierarchy in a Nodes board.
Minimum importance is propagated along branches to calculate the relative strength of an Edge
and can be used to determine which column or branch a Node appears in.</div>)";

std::string render_Node_superiors(Graph & graph, Node & node, bool remove_button = false, bool edit_edges = false) {
    std::string sups_str;
    std::string graphserveraddr = fzgh.replacements[fzserverpq_address]; // graph.get_server_full_address();
    time_t node_td = node.get_targetdate();
    for (const auto & edge_ptr : node.sup_Edges()) {
        if (edge_ptr) {
            if (fzgh.config.outputformat == output_node) {
                sups_str += edge_ptr->get_sup_str() + ' ';
            } else {
                // Link to superior.
                sups_str += "<li><a href=\"/cgi-bin/fzlink.py?id="+edge_ptr->get_sup_str()+"\">" + edge_ptr->get_sup_str() + "</a>: ";

                // Excerpt of superior node description.
                Node * sup_ptr = edge_ptr->get_sup();
                if (!sup_ptr) {
                    ADDERROR(__func__, "Node "+node.get_id_str()+" has missing superior at Edge "+edge_ptr->get_id_str());
                } else {
                    std::string htmltext(sup_ptr->get_text().c_str());
                    bool is_active = sup_ptr->is_active();
                    if (is_active) {
                        sups_str += "[<span class=\""+supdep_td_highlight.at(sup_ptr->get_targetdate()>=node_td)+"\">"+sup_ptr->get_targetdate_str()+"</span>] ";
                    }
                    sups_str += "<span class=\""+supdep_active_highlight.at(is_active)+"\">"+remove_html_tags(htmltext).substr(0,fzgh.config.excerpt_length)+"</span>";
                }

                // Add a link to remove the superior.
                if (remove_button) {
                    sups_str += "[<a href=\"http://"+graphserveraddr+"/fz/graph/nodes/" + node.get_id_str() + "/superiors/remove?" + edge_ptr->get_sup_str() + "=\">remove</a>]";
                }

                // Add Edge parameter data.
                if (edit_edges) {
                    std::string edgeidstr(edge_ptr->get_id_str());
                    sups_str += "<br>dependency: <input onchange=\"edge_update(event);\" type=\"number\" size=5 id=\"dep_"+edgeidstr+"\" name=\"dep_"+edgeidstr+"\" value="+to_precision_string(edge_ptr->get_dependency(), 2)+'>';
                    sups_str += " significance: <input onchange=\"edge_update(event);\" type=\"number\" size=5 id=\"sig_"+edgeidstr+"\" name=\"sig_"+edgeidstr+"\" value="+to_precision_string(edge_ptr->get_significance(), 2)+'>';
                    sups_str += " "+make_tooltip("importance", importance_tooltip)+": <input onchange=\"edge_update(event);\" type=\"number\" size=5 id=\"imp_"+edgeidstr+"\" name=\"imp_"+edgeidstr+"\" value="+to_precision_string(edge_ptr->get_importance(), 2)+'>';
                    sups_str += " urgency: <input onchange=\"edge_update(event);\" type=\"number\" size=5 id=\"urg_"+edgeidstr+"\" name=\"urg_"+edgeidstr+"\" value="+to_precision_string(edge_ptr->get_urgency(), 2)+'>';
                    sups_str += " priority: <input onchange=\"edge_update(event);\" type=\"number\" size=5 id=\"pri_"+edgeidstr+"\" name=\"pri_"+edgeidstr+"\" value="+to_precision_string(edge_ptr->get_priority(), 2)+'>';
                } else {
                    sups_str += "<br>dependency: ";
                    sups_str += to_precision_string(edge_ptr->get_dependency(), 2);
                    sups_str += " significance: ";
                    sups_str += to_precision_string(edge_ptr->get_significance(), 2);
                    sups_str += " "+make_tooltip("importance", importance_tooltip)+": ";
                    sups_str += to_precision_string(edge_ptr->get_importance(), 2);
                    sups_str += " urgency: ";
                    sups_str += to_precision_string(edge_ptr->get_urgency(), 2);
                    sups_str += " priority: ";
                    sups_str += to_precision_string(edge_ptr->get_priority(), 2);
                }

                sups_str += "</li>\n";
            }
        }
    }
    return sups_str;
}

std::string render_Node_dependencies(Graph & graph, Node & node, bool remove_button = false, bool edit_edges = false) {
    std::string deps_str;
    std::string graphserveraddr = fzgh.replacements[fzserverpq_address]; //graph.get_server_full_address();
    time_t node_td = node.get_targetdate();
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
                    bool is_active = dep_ptr->is_active();
                    if (is_active) {
                        deps_str += "[<span class=\""+supdep_td_highlight.at(dep_ptr->get_targetdate()<=node_td)+"\">"+dep_ptr->get_targetdate_str()+"</span>] ";
                    }
                    deps_str += "<span class=\""+supdep_active_highlight.at(is_active)+"\">"+remove_html_tags(htmltext).substr(0,fzgh.config.excerpt_length)+"</span>";
                }
                if (remove_button) {
                    deps_str += "[<a href=\"http://"+graphserveraddr+"/fz/graph/nodes/" + node.get_id_str() + "/superiors/remove?" + edge_ptr->get_dep_str() + "=\">remove</a>]";
                }

                // Add Edge parameter data.
                if (edit_edges) {
                    std::string edgeidstr(edge_ptr->get_id_str());
                    deps_str += "<br>dependency: <input onchange=\"edge_update(event);\" type=\"number\" size=5 id=\"dep_"+edgeidstr+"\" name=\"dep_"+edgeidstr+"\" value="+to_precision_string(edge_ptr->get_dependency(), 2)+'>';
                    deps_str += " significance: <input onchange=\"edge_update(event);\" type=\"number\" size=5 id=\"sig_"+edgeidstr+"\" name=\"sig_"+edgeidstr+"\" value="+to_precision_string(edge_ptr->get_significance(), 2)+'>';
                    deps_str += " "+make_tooltip("importance", importance_tooltip)+": <input onchange=\"edge_update(event);\" type=\"number\" size=5 id=\"imp_"+edgeidstr+"\" name=\"imp_"+edgeidstr+"\" value="+to_precision_string(edge_ptr->get_importance(), 2)+'>';
                    deps_str += " urgency: <input onchange=\"edge_update(event);\" type=\"number\" size=5 id=\"urg_"+edgeidstr+"\" name=\"urg_"+edgeidstr+"\" value="+to_precision_string(edge_ptr->get_urgency(), 2)+'>';
                    deps_str += " priority: <input onchange=\"edge_update(event);\" type=\"number\" size=5 id=\"pri_"+edgeidstr+"\" name=\"pri_"+edgeidstr+"\" value="+to_precision_string(edge_ptr->get_priority(), 2)+'>';
                } else {
                    deps_str += "<br>dependency: ";
                    deps_str += to_precision_string(edge_ptr->get_dependency(), 2);
                    deps_str += " significance: ";
                    deps_str += to_precision_string(edge_ptr->get_significance(), 2);
                    deps_str += " "+make_tooltip("importance", importance_tooltip)+": ";
                    deps_str += to_precision_string(edge_ptr->get_importance(), 2);
                    deps_str += " urgency: ";
                    deps_str += to_precision_string(edge_ptr->get_urgency(), 2);
                    deps_str += " priority: ";
                    deps_str += to_precision_string(edge_ptr->get_priority(), 2);
                }

                deps_str += "</li>\n";
            }
        }
    }
    return deps_str;
}

typedef std::string string_attributes_func_t(const std::string&);
typedef std::map<Prerequisite_States, string_attributes_func_t*> string_attributes_map_t;

std::string unsolved_prerequisite_attributes(const std::string & prerequisite) {
    return "<span style=\"color:red;\">"+prerequisite+"</span>";
}

std::string unfulfilled_prerequisite_attributes(const std::string & prerequisite) {
    return prerequisite;
}

std::string fulfilled_prerequisite_attributes(const std::string & prerequisite) {
    return "<b>"+prerequisite+"</b>";
}

const string_attributes_map_t string_attributes = {
    {unsolved, unsolved_prerequisite_attributes},
    {unfulfilled, unfulfilled_prerequisite_attributes},
    {fulfilled, fulfilled_prerequisite_attributes},
};

std::string render_Node_prerequisites_and_provides_capabilities(Node & node) {
    std::string render_str;
    auto prereqs = get_prerequisites(node, true);

    if (!prereqs.empty()) {
        render_str += "<br>Prerequisites:\n<ul>\n";
        //int prereq_num = 0;
        for (const auto & prereq : prereqs) {
            // if (prereq_num>0) {
            //     render_str += ", ";
            // }
            render_str += "<li>"+string_attributes.at(prereq.state())(prereq.str())+'\n';
            //prepreq_num++;
        }
        render_str += "</ul>\n";
    }

    auto provides = get_provides_capabilities(node);
    if (!provides.empty()) {
        render_str += "<br>Provides capabilities:\n<ul>\n";
        //int provides_num = 0;
        for (const auto & capability : provides) {
            // if (provides_num>0) {
            //     render_str += ", ";
            // }
            render_str += "<li>"+capability+'\n';
            //provides_num++;
        }
        render_str += "</ul>\n";
    }

    return render_str;
}

const std::vector<std::string> special_urls = {
    "@FZSERVER@"
};

const std::map<bool, std::string> flag_set_map = {
    { false, "none" },
    { true, "SET" },
};

/**
 * Render the Boolean Tag Flag data found for the Node,
 * either as directly and explicitly specified, or as
 * inferred.
 * 
 * To use this, call fzgraphhtml with the -n, -B and -S arguments.
 * 
 * @param graph A valid Graph.
 * @param node A valid Node object.
 * @return A string with rendered Node data according to the chosen format.
 */
std::string render_Node_BTF(Graph & graph, Node & node) {
    Map_of_Subtrees map_of_subtrees;
    map_of_subtrees.collect(graph, fzgh.subtrees_list_name);

    if (!map_of_subtrees.has_subtrees) {
        standard_exit_error(exit_bad_request_data, "Missing subtrees list: "+fzgh.subtrees_list_name, __func__);
    }

    Boolean_Tag_Flags::boolean_flag boolean_tag;
    if (!map_of_subtrees.node_in_heads_or_any_subtree(node.get_id().key(), boolean_tag, true)) { // includes searching of superiors as needed
        boolean_tag = Boolean_Tag_Flags::none;
    }

    render_environment env;
    fzgraphhtml_templates templates;
    load_templates(templates);
    template_varvalues nodevars;

    nodevars.emplace("node-id", node.get_id_str());
    if (boolean_tag != Boolean_Tag_Flags::none) {
        nodevars.emplace("btf-category", boolean_flag_str_map.at(boolean_tag));
    } else {
        nodevars.emplace("btf-category", "none");
    }
    
    nodevars.emplace("btf-tzadjust", flag_set_map.at(node.get_bflags().TZadjust()));
    nodevars.emplace("btf-error", flag_set_map.at(node.get_bflags().Error()));

    return env.render(templates[node_BTF_temp], nodevars);
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
    nodevars.emplace("fzserverpq", fzgh.replacements[fzserverpq_address]);
    nodevars.emplace("T_context", TimeStampYmdHM(node.t_created() - RTt_oneday));
    if (fzgh.config.excerpt_requested) {
        nodevars.emplace("node-text", remove_html_tags(node.get_text()).substr(0,fzgh.config.excerpt_length)); // *** must this also have c_str()?
    } else {
        nodevars.emplace("node-text",
            make_embeddable_html(
                node.get_text().c_str(),
                fzgh.config.interpret_text,
                &special_urls,
                &fzgh.replacements
            ) ); //node.get_text());
    }
    
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
    if (node.get_repeats()) {
        nodevars.emplace("repeats", "Yes");
    } else {
        nodevars.emplace("repeats", "No");
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
    nodevars.emplace("bflags", join(node.get_bflags().get_Boolean_Tag_flags_strvec(), ", "));
    nodevars.emplace("NNLs", render_Node_NNLs(graph, node));
    nodevars.emplace("superiors", render_Node_superiors(graph, node));
    nodevars.emplace("dependencies", render_Node_dependencies(graph, node));
    nodevars.emplace("prereqs-provides", render_Node_prerequisites_and_provides_capabilities(node));

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

static const char update_skip_template_A[] = R"USTEMP( | <input type="submit" name="action" value="update"> to <input type="datetime-local" id="tpass" name="tpass" min="1990-01-01T00:00:00" value=")USTEMP";
static const char update_skip_template_B[] = R"USTEMP("> <input type="submit" name="action" value="skip"> <input type="number" name="num_skip" min="1" max="100" step="1" value=1>)USTEMP";

std::string prop_argument(td_property tdprop) {
    if (tdprop == _tdprop_num) return "";

    return "&prop="+td_property_str[tdprop];
}

bool render_new_node_page() {
    ERRTRACE;

    //Graph & graph = fzgh.graph();
    
    // this is where you do the same as in render_Node_data(), with a few extra bits
    render_environment env;
    fzgraphhtml_templates templates;
    load_templates(templates);
    template_varvalues nodevars;

    // Let's use a Node_data object for standardized defaults.
    Node_data ndata;
    long required_mins = (ndata.hours * 60.0);
    double required_hrs = ndata.hours;
    if (fzgh.req_suggested != 0.0) {
        required_hrs = fzgh.req_suggested;
    }
    td_property tdprop = ndata.tdproperty;
    if (fzgh.init_tdprop != _tdprop_num) {
        tdprop = fzgh.init_tdprop;
    }
    td_pattern tdpatt = ndata.tdpattern;

    nodevars.emplace("node-id", "NEW");
    if (fzgh.list_name.empty()) {
        nodevars.emplace("is_disabled", " disabled");
        nodevars.emplace("notice_1", " <b>add a Topic to enable 'create'</b> ");
        nodevars.emplace("select-template", "");
        nodevars.emplace("node-text", " (add a Topic to enable description entry) ");
    } else {
        nodevars.emplace("is_disabled", "");
        nodevars.emplace("notice_1", "<input type=\"hidden\" name=\"topics\" value=\""+fzgh.list_name+"\">");
        nodevars.emplace("select-template", "<tr><td><button class=\"button button1\" onclick=\"window.open('/cgi-bin/addnode-template.py?topics="+fzgh.list_name+prop_argument(fzgh.init_tdprop)+"','_self');\">Select Template</button></td></tr>");
        nodevars.emplace("node-text", ndata.utf8_text);
    }

    nodevars.emplace("comp", to_precision_string(0.0));
    nodevars.emplace("hrs_to_complete", to_precision_string(required_hrs));
    if (fzgh.req_suggested != 0.0) {
        nodevars.emplace("req_hrs", to_precision_string(required_hrs));
    } else {
        nodevars.emplace("req_hrs", "");
    }
    nodevars.emplace("req_mins", std::to_string(required_mins));
    nodevars.emplace("val", to_precision_string(ndata.valuation));

    time_t t_eff = ndata.targetdate;
    if (t_eff == RTt_unspecified) {
        //t_eff = today_end_time();
        t_eff = ymd_stamp_time(DateStampYmd(ActualTime())+"2330"); // Giving ourselves a little extra half hour at the end of the day by default.
    }
    nodevars.emplace("eff_td", TimeStampYmdHM(t_eff));
    nodevars.emplace("eff_td_date",TimeStamp("%Y-%m-%d", t_eff));
    nodevars.emplace("eff_td_time",TimeStamp("%H:%M", t_eff));

    std::string prop_value[_tdprop_num] = {"", "", "", "", ""};
    if (tdprop<_tdprop_num) {
        prop_value[tdprop] = "checked";
    } else {
        ADDERROR(__func__, "New Node has unrecognized TD property "+std::to_string((int) tdprop));
    }
    for (int i = 0; i < _tdprop_num; ++i) {
        nodevars.emplace(prop_token[i],prop_value[i]);
    }

    std::string patt_value[_patt_num] = {"", "", "", "", "", "", "", "", ""};
    if (tdpatt<_patt_num) {
        patt_value[tdpatt] = "checked";
    } else {
        ADDERROR(__func__, "New Node has unrecognized TD pattern "+std::to_string((int) tdpatt));
    }
    for (int i = 0; i < _patt_num; ++i) {
        if (i != OLD_patt_span) {
            nodevars.emplace(patt_token[i],patt_value[i]);
        }
    }

    nodevars.emplace("td_every", std::to_string(ndata.tdevery));
    nodevars.emplace("td_span", std::to_string(ndata.tdspan));
    nodevars.emplace("fzserverpq", fzgh.replacements[fzserverpq_address]); // graph.get_server_full_address()
    nodevars.emplace("topics", fzgh.list_name);

    fzgh.config.embeddable = true;
    fzgh.cache_it = true;
    standard_mute();
    fzgh.list_name = "superiors";
    if (render_named_node_list()) {
        nodevars.emplace("superiors", fzgh.cache_str);
    } else {
        nodevars.emplace("superiors", "Please add at least one Superior.");
    }
    fzgh.list_name = "dependencies";
    if (render_named_node_list()) {
        nodevars.emplace("dependencies", fzgh.cache_str);
    } else {
        nodevars.emplace("dependencies", "");
    }
    standard_unmute();

    std::string rendered_node_data = env.render(templates[node_new_temp], nodevars);

    if (fzgh.config.rendered_out_path == "STDOUT") {
        FZOUT(rendered_node_data);
    } else {
        if (!string_to_file(fzgh.config.rendered_out_path, rendered_node_data))
            standard_exit_error(exit_file_error, "Unable to write rendered page to file.", __func__);
    }

    return true;
}

/**
 * Note: `fzgh.node_idstr == "new"` means generate edit page to collect information for a new Node.
 */
bool render_node_edit() {
    ERRTRACE;

    if (fzgh.node_idstr == "new") {
        return render_new_node_page();
    }

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

    if (node.get_repeats()) {
        nodevars.emplace("td_update_skip", update_skip_template_A+TimeStamp("%Y-%m-%dT%H:%M", ActualTime())+update_skip_template_B);
    } else {
        nodevars.emplace("td_update_skip", "");
    }

    //nodevars.emplace("select-template", "<tr><td><button class=\"button button1\" onclick=\"window.open('/cgi-bin/addnode-template.py?topics="+fzgh.list_name+prop_argument(fzgh.init_tdprop)+"','_self');\">Overwrite with Template</button></td></tr>");

    nodevars.emplace("node-text", node.get_text());
    float completion = node.get_completion();
    nodevars.emplace("comp", to_precision_string(completion));
    if ((completion >= 0.0) && (completion < 1.0)) {
        nodevars.emplace("hrs_to_complete", to_precision_string(required_hrs*(1.0-completion)));
    } else {
        nodevars.emplace("hrs_to_complete", "0.00");
    }
    if (completion >= 0.0) {
        float comp_hrs = required_hrs*completion;
        nodevars.emplace("comp_hrs", to_precision_string(comp_hrs));
    } else {
        nodevars.emplace("comp_hrs", "inactive");
    }
    nodevars.emplace("req_hrs", to_precision_string(required_hrs));
    nodevars.emplace("req_mins", std::to_string(required_mins));
    nodevars.emplace("val", to_precision_string(node.get_valuation()));

    time_t t_eff = node.effective_targetdate();
    nodevars.emplace("eff_td", TimeStampYmdHM(t_eff));
    nodevars.emplace("eff_td_date",TimeStamp("%Y-%m-%d", t_eff));
    nodevars.emplace("eff_td_time",TimeStamp("%H:%M", t_eff));

    time_t t_earliest_superior = node.earliest_active_superior();
    if (t_earliest_superior == RTt_maxtime) {
        nodevars.emplace("earliest-superior", "");
    } else {
        nodevars.emplace("earliest-superior", "(earliest superior: "+TimeStampYmdHM(t_earliest_superior)+')');
    }

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
    nodevars.emplace("fzserverpq", fzgh.replacements[fzserverpq_address]); // graph.get_server_full_address()
    nodevars.emplace("topics", render_Node_topics(graph, node, true));
    nodevars.emplace("bflags", join(node.get_bflags().get_Boolean_Tag_flags_strvec(), ", "));
    nodevars.emplace("NNLs", render_Node_NNLs(graph, node));

    // Check potential superiors to add from the superiors NNL.
    std::string nnl_superiors;
    auto nlist = graph_ptr->get_List_Nodes("superiors");
    for (const auto& nptr : nlist) {
        nnl_superiors += nptr->get_id_str()+": "+nptr->get_excerpt(60)+"<br>\n";
    }
    nodevars.emplace("nnl_superiors", nnl_superiors);

    nodevars.emplace("superiors", render_Node_superiors(graph, node, true, true));
    nodevars.emplace("dependencies", render_Node_dependencies(graph, node, true, true));

    std::string rendered_node_data = env.render(templates[node_edit_temp], nodevars);

    if (fzgh.config.rendered_out_path == "STDOUT") {
        FZOUT(rendered_node_data);
    } else {
        if (!string_to_file(fzgh.config.rendered_out_path, rendered_node_data))
            standard_exit_error(exit_file_error, "Unable to write rendered page to file.", __func__);
    }

    return true;
}
