// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Template rendering functions.
 * 
 */

// core
#include "error.hpp"
#include "general.hpp"
#include "stringio.hpp"

// local
#include "render.hpp"
#include "fzserver-info.hpp"


/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif


using namespace fz;


enum template_id_enum {
    graph_server_status_txt_temp,
    graph_server_status_html_temp,
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "graph_server_status_template.txt",
    "graph_server_status_template.html"
};

typedef std::map<template_id_enum,std::string> fzserver_info_templates;

bool load_templates(fzserver_info_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i], templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

bool render_graph_server_status(const template_varvalues & statusinfo) {
    render_environment env;
    fzserver_info_templates templates;
    load_templates(templates);

    std::string rendered_str;
    if (fzsi.output_format == output_html) {
        rendered_str = env.render(templates[graph_server_status_html_temp], statusinfo);
    } else { // output_txt is the default
        rendered_str = env.render(templates[graph_server_status_txt_temp], statusinfo);
    }

    if (fzsi.config.info_out_path == "STDOUT") {
        FZOUT(rendered_str);
    } else {
        if (!string_to_file(fzsi.config.info_out_path,rendered_str)) {
            ERRRETURNFALSE(__func__,"unable to write rendered output to "+fzsi.config.info_out_path);
        } else {
            VERBOSEOUT("Graph server status info sent to file at "+fzsi.config.info_out_path+'\n');
        }
    }

    return true;
}
