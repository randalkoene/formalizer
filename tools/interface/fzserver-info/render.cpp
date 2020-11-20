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
    graph_server_status_txt_temp = 0,
    graph_server_status_html_temp = 1,
    graph_server_status_json_temp = 2,
    graph_server_status_csv_temp = 3,
    graph_server_status_raw_temp = 4,
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "graph_server_status_template.txt",
    "graph_server_status_template.html",
    "graph_server_status_template.json",
    "graph_server_status_template.csv",
    "graph_server_status_template.raw"
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

bool output_response(std::string & rendered_str) {
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

bool render_graph_server_status(const template_varvalues & statusinfo) {
    render_environment env;
    fzserver_info_templates templates;
    load_templates(templates);

    std::string rendered_str;
    rendered_str = env.render(templates[(template_id_enum) fzsi.output_format], statusinfo);

    return output_response(rendered_str);
}

std::string render_shared_memory_blocks(const POSIX_shm_data_vec & shmblocksvec) {
    std::string rendered_str;
    for (const auto & shmblock : shmblocksvec) {
        switch (fzsi.output_format) {

            case output_txt: {
                rendered_str += shmblock.name + " : " + std::to_string(shmblock.size) + '\n';
                break;
            }

            case output_html: {
                rendered_str += "<tr><td>"+shmblock.name + "</td><td>" + std::to_string(shmblock.size) + "</td></tr>\n";
                break;
            }

            case output_json: {
                rendered_str += '"'+shmblock.name + "\" : \"" + std::to_string(shmblock.size) + "\",\n";
                break;
            }

            case output_csv: {
                rendered_str += shmblock.name + ',' + std::to_string(shmblock.size) + '\n';
                break;
            }

            case output_raw: {
                rendered_str += shmblock.name + ' ' + std::to_string(shmblock.size) + '\n';
                break;
            }

            default: {
                VERYVERBOSEOUT("Unrecognized output format.\n");
            }
        }
    }
    if ((!rendered_str.empty()) && (fzsi.output_format == output_json)) {
        rendered_str.insert(0,"{\n");
        rendered_str[rendered_str.size()-2] = '\n';
        rendered_str.back() = '}';
        rendered_str += '\n';
    }
    return rendered_str;
}
