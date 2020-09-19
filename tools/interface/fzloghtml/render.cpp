// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Template rendering functions.
 * 
 */

// core
#include "error.hpp"
#include "general.hpp"
#include "templater.hpp"

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
    LogHTML_chunk_temp,
    LogHTML_entry_temp,
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "LogHTML_head_template.html",
    "LogHTML_tail_template.html",
    "LogHTML_chunk_template.html"
    "LogHTML_entry_template.html"
};

typedef std::map<template_id_enum,std::string> fzloghtml_templates;

render_environment env;
fzloghtml_templates templates;

bool load_templates(fzloghtml_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i], templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

std::string render_Log_entry(Log_entry & entry) {
    template_varvalues varvals;
    varvals.emplace("minor_id",std::to_string(entry.get_minor_id()));
    varvals.emplace("entry_text",entry.get_entrytext());
    if (entry.same_node_as_chunk()) {
        varvals.emplace("node_id","");
    } else {
        std::string nodestr(entry.get_nodeidkey().str());
        varvals.emplace("node_id","[<a href=\"/cgi-bin/fzlink.py?id="+nodestr+"\">"+nodestr+"</a>]");
    }
    return env.render(templates[LogHTML_entry_temp],varvals);
}

/**
 * Convert Log content between t_from and t_before to HTML using
 * rending templates and send to designated output destination.
 */
bool render() {
    ERRTRACE;
    load_templates(templates);

    auto [from_idx, to_idx] = fzlh.log->get_Chunks_index_t_interval(fzlh.t_from, fzlh.t_before);

    std::string rendered_logcontent(templates[LogHTML_head_temp]);
    rendered_logcontent.reserve(128*1024);

    for (auto chunk_idx = from_idx; chunk_idx <= to_idx; ++chunk_idx) {

        Log_chunk * chunkptr = fzlh.log->get_chunk(chunk_idx);
        if (chunkptr) {
            std::string combined_entries;
            for (const auto& entryptr : chunkptr->get_entries()) {
                if (entryptr) {
                    combined_entries += render_Log_entry(*entryptr);
                }
            }

            template_varvalues varvals;
            varvals.emplace("node_id",chunkptr->get_NodeID().str());
            varvals.emplace("t_chunkopen",chunkptr->get_tbegin_str());
            time_t t_chunkclose = chunkptr->get_close_time();
            if (t_chunkclose < chunkptr->get_open_time()) {
                varvals.emplace("t_chunkclose","OPEN");
            } else {
                varvals.emplace("t_chunkclose",TimeStampYmdHM(t_chunkclose));
            }
            varvals.emplace("entries",combined_entries);
            rendered_logcontent += env.render(templates[LogHTML_chunk_temp],varvals);
        }
    }

    rendered_logcontent += templates[LogHTML_tail_temp];

    // send to destination
    if (fzlh.dest.empty()) { // to STDOUT
        //VERBOSEOUT("Log interval:\n\n");
        FZOUT(rendered_logcontent);
    } else {
        VERBOSEOUT("Writing HTML Log content to "+fzlh.dest+".\n\n");
        if (!string_to_file(fzlh.dest,rendered_logcontent)) {
            ADDERROR(__func__,"unable to write snippet to "+fzlh.dest);
            standard.exit(exit_file_error);
        }
    }

    return true;
}
