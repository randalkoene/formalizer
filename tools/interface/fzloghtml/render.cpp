// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Template rendering functions.
 * 
 */

//#define USE_COMPILEDPING
#ifdef USE_COMPILEDPING
    #include <iostream>
#endif

// core
#include "error.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "templater.hpp"
#include "html.hpp"

// local
#include "render.hpp"
#include "fzloghtml.hpp"


/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif


using namespace fz;


enum template_id_enum {
    LogHTML_head_temp,
    LogHTML_tail_temp,
    Log_chunk_RAW_temp,
    Log_chunk_TXT_temp,
    Log_chunk_HTML_temp,
    Log_entry_RAW_temp,
    Log_entry_TXT_temp,
    Log_entry_HTML_temp,
    Log_most_recent_HTML_temp,
    Log_most_recent_TXT_temp,
    Log_most_recent_RAW_temp,
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "LogHTML_head_template.html",
    "LogHTML_tail_template.html",
    "Log_chunk_template.raw",
    "Log_chunk_template.txt",
    "Log_chunk_template.html",
    "Log_entry_template.raw",
    "Log_entry_template.txt",
    "Log_entry_template.html",
    "Log_most_recent_template.html",
    "Log_most_recent_template.txt",
    "Log_most_recent_template.raw"
};

typedef std::map<template_id_enum,std::string> fzloghtml_templates;

render_environment env;
fzloghtml_templates templates;
std::string * active_entry_template = nullptr;

bool load_templates(fzloghtml_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i], templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

const std::map<std::string, std::string> template_code_replacements = {
    {"\\n", "\n"}
};

void prepare_custom_template(std::string & customtemplate) {
    customtemplate = fzlh.custom_template.substr(4);
    for (const auto [codestr, repstr] : template_code_replacements) {
        size_t codepos = 0;
        while (true) {
            codepos = customtemplate.find(codestr, codepos);
            if (codepos == std::string::npos) break;
            customtemplate.replace(codepos, codestr.size(), repstr);
            codepos += repstr.size();
        }
    }
}

std::string render_Log_entry(Log_entry & entry) {
    template_varvalues varvals;
    varvals.emplace("minor_id",std::to_string(entry.get_minor_id()));
    varvals.emplace("entry_id",entry.get_id_str());
    varvals.emplace("entry_text",make_embeddable_html(entry.get_entrytext(),fzlh.config.interpret_text));
    if (entry.same_node_as_chunk()) {
        varvals.emplace("node_id","");
    } else {
        std::string nodestr(entry.get_nodeidkey().str());
        if (fzlh.recent_format == most_recent_html) {
            varvals.emplace("node_id","[<a href=\"/cgi-bin/fzlink.py?id="+nodestr+"\">"+nodestr+"</a>]");
        } else {
            varvals.emplace("node_id",nodestr);
        }
    }
    return env.render(*active_entry_template,varvals);
}

bool send_rendered_to_output(std::string & rendered_text) {
    if ((fzlh.config.dest.empty()) || (fzlh.config.dest == "STDOUT")) { // to STDOUT
        //VERBOSEOUT("Log interval:\n\n");
        FZOUT(rendered_text);
        return true;
    }
    
    VERBOSEOUT("Writing rendered content to "+fzlh.config.dest+".\n\n");
    if (!string_to_file(fzlh.config.dest,rendered_text)) {
        ADDERROR(__func__,"unable to write to "+fzlh.config.dest);
        standard.exit(exit_file_error);
    }
    return true;
}

/**
 * At a minimum, show the Node ID. Depending on configuration and available
 * memory-resident Graph, also show a short exerpt of Node description.
 */
std::string include_Node_info(const Node_ID & node_id) {
    std::string nodestr = node_id.str();
    if (fzlh.config.node_excerpt_len > 0) {
        Graph_ptr graphptr = fzlh.get_Graph_ptr(); // only attempts to fetch it the first time
        if (graphptr) {
            Node_ptr nodeptr = graphptr->Node_by_id(node_id.key());
            if (nodeptr) {
                std::string htmltext(nodeptr->get_text().c_str());
                nodestr += ' '+remove_html_tags(htmltext).substr(0,fzlh.config.node_excerpt_len);
            } else {
                nodestr += " (not found)";
            }
        }
    }
    return nodestr;
}

/**
 * Convert Log content that was retrieved with filtering to HTML using
 * rending templates and send to designated output destination.
 */
bool render_Log_interval() {
    ERRTRACE;
    std::string customtemplate;
    if (fzlh.custom_template.empty() || (!fzlh.noframe)) {
        load_templates(templates); // *** wait? doesn't this segfault further down on the other templates if skipped???
    }

    std::string * active_chunk_template;
    switch (fzlh.recent_format) {
        case most_recent_raw: {
            active_chunk_template = &templates[Log_chunk_RAW_temp];
            active_entry_template = &templates[Log_entry_RAW_temp];
            break;
        }

        case most_recent_txt: {
            active_chunk_template = &templates[Log_chunk_TXT_temp];
            active_entry_template = &templates[Log_entry_TXT_temp];
            break;
        }

        default: {
            active_chunk_template = &templates[Log_chunk_HTML_temp];
            active_entry_template = &templates[Log_entry_HTML_temp];
        }
    }

    if (!fzlh.custom_template.empty()) {
        if (fzlh.custom_template.substr(0,4) == "STR:") {
            prepare_custom_template(customtemplate);
        } else {
            standard_exit_error(exit_command_line_error, "Custom template from file has not yet been implemented.",__func__);
        }
    }

    if (fzlh.filter.nkey.isnullkey()) {
        VERYVERBOSEOUT("Finding Log chunks from "+TimeStampYmdHM(fzlh.filter.t_from)+" to "+TimeStampYmdHM(fzlh.filter.t_to)+'\n');
    } else {
        VERYVERBOSEOUT("Finding the Log history of Node "+fzlh.filter.nkey.str()+'\n');
    }

    std::string rendered_logcontent;
    rendered_logcontent.reserve(128*1024);

    if (!fzlh.noframe) {
        rendered_logcontent += templates[LogHTML_head_temp];
    }

    COMPILEDPING(std::cout,"PING: got templates\n");

    for (const auto & [chunk_key, chunkptr] : fzlh.edata.log_ptr->get_Chunks()) {

        //COMPILEDPING(std::cout,"PING: commencing chunk idx#"+std::to_string(chunk_idx)+'\n');

        if (chunkptr) {
            std::string combined_entries;
            for (const auto& entryptr : chunkptr->get_entries()) {
                if (entryptr) {
                    combined_entries += render_Log_entry(*entryptr);
                }
            }

            template_varvalues varvals;
            Node_ID node_id = chunkptr->get_NodeID();
            varvals.emplace("node_id",include_Node_info(node_id));
            varvals.emplace("node_link","/cgi-bin/fzlink.py?id="+node_id.str());
            //varvals.emplace("fzserverpq",graph.get_server_full_address()); *** so far, this is independent of whether the Graph is memory-resident
            varvals.emplace("t_chunkopen",chunkptr->get_tbegin_str());
            time_t t_chunkclose = chunkptr->get_close_time();
            time_t t_chunkopen = chunkptr->get_open_time();
            if (t_chunkclose < chunkptr->get_open_time()) {
                varvals.emplace("t_chunkclose","OPEN");
                varvals.emplace("t_diff","");
                varvals.emplace("t_diff_mins",""); // typically, only either t_diff or t_diff_mins appears in a template
            } else {
                varvals.emplace("t_chunkclose",TimeStampYmdHM(t_chunkclose));
                time_t t_diff = (t_chunkclose - t_chunkopen)/60; // mins
                varvals.emplace("t_diff_mins", std::to_string(t_diff)); // particularly useful for cutom templates
                if (t_diff >= 120) {
                    varvals.emplace("t_diff", to_precision_string(((double) t_diff)/60.0, 2, ' ', 5)+" hrs");
                } else {
                    varvals.emplace("t_diff", std::to_string(t_diff)+" mins");
                }
            }
            varvals.emplace("entries",combined_entries);
            if (customtemplate.empty()) {
                rendered_logcontent += env.render(*active_chunk_template, varvals);
            } else {
                rendered_logcontent += env.render(customtemplate, varvals);
            }
        }
    }

    if (!fzlh.noframe) {
        rendered_logcontent += templates[LogHTML_tail_temp];
    }

    return send_rendered_to_output(rendered_logcontent);
}

bool render_Log_most_recent() {
    ERRTRACE;
    load_templates(templates);

    VERYVERBOSEOUT("Finding most recent Log entry.\n");

    template_varvalues varvals;
    varvals.emplace("chunk_id",TimeStampYmdHM(fzlh.edata.newest_chunk_t));
    if (fzlh.edata.c_newest) {
        varvals.emplace("node_id",fzlh.edata.c_newest->get_NodeID().str());
    }
    if (fzlh.edata.is_open) {
        varvals.emplace("chunk_status","OPEN");
        varvals.emplace("t_chunkclose",std::to_string(FZ_TCHUNK_OPEN));
    } else {
        varvals.emplace("chunk_status","CLOSED");
        if (fzlh.edata.c_newest) {
            time_t t_chunkclose = fzlh.edata.c_newest->get_close_time();
            varvals.emplace("t_chunkclose",TimeStampYmdHM(t_chunkclose));
        } else {
            varvals.emplace("t_chunkclose",std::to_string(RTt_invalid_time_stamp)); // should not happen!
        }
    }
    varvals.emplace("entry_minor_id",std::to_string(fzlh.edata.newest_minor_id));
    if (fzlh.edata.e_newest) {
        varvals.emplace("entry_id",fzlh.edata.e_newest->get_id_str());
    } else {
        varvals.emplace("entry_id", LOG_NULLKEY_STR);
    }
    
    std::string * render_template = nullptr;
    switch (fzlh.recent_format) {
        case most_recent_raw: {
            render_template = &templates[Log_most_recent_RAW_temp];
            break;
        }

        case most_recent_txt: {
            render_template = &templates[Log_most_recent_TXT_temp];
            break;
        }

        default: {
            render_template = &templates[Log_most_recent_HTML_temp];
        }
    }
    std::string rendered_mostrecent = env.render(*render_template,varvals);

    return send_rendered_to_output(rendered_mostrecent);
}
