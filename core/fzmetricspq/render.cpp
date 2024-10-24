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
#include "templater.hpp"

// local
#include "render.hpp"
#include "fzmetricspq.hpp"


/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif


using namespace fz;


enum template_id_enum {
    example_template_temp, // replace with actual
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "example_templatefile_template.ext" // replace with actual
};

typedef std::map<template_id_enum,std::string> fzmetricspq_templates;

bool load_templates(fzmetricspq_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i], templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

bool render_data(Metrics_data& data, std::string& rendered_data) {
    if (fzmet.format == "json") {
        rendered_data += '"'+data.tablename+"\":"+data.data+',';
    } else { // raw is the default
        rendered_data += data.tablename + ":\n";
        rendered_data += data.data + '\n';
    }
    // render_environment env;
    // fzmetricspq_templates templates;
    // load_templates(templates);

    // template_varvalues varvals;
    // varvals.emplace("{{ a_variable }}","{{ a_string }}"); // replace with actual
    // std::string rendered_str = env.render(templates[example_template_temp], varvals);

    // if (!string_to_file("an/example/output/file/path",rendered_str)) // replace with actual
    //         ERRRETURNFALSE(__func__,"unable to write rendered output to file");
    
    return true;
}

bool render_data_list(Metrics_data_list& datalist, std::string& rendered_data) {

    if (fzmet.format == "json") {

        rendered_data += '"'+datalist.tablename+"\":" + '{';
        if (datalist.id_list.empty()) {
            rendered_data += "},";
        } else {
            for (size_t i = 0; i < datalist.id_list.size(); ++i) {
                if (!datalist.data_list[i].empty()) {
                    rendered_data += '"'+datalist.id_list[i]+"\":"+datalist.data_list[i]+',';
                }
            }
            if (rendered_data.back() == ',') {
                rendered_data.back() = '}';
                rendered_data += ',';
            } else {
                rendered_data += "},";
            }
        }

    } else {

        rendered_data += datalist.tablename + ":\n";
        for (size_t i = 0; i < datalist.id_list.size(); ++i) {
            if (!datalist.data_list[i].empty()) {
                rendered_data += datalist.id_list[i] + ':' + datalist.data_list[i] + '\n';
            }
        }

    }

    return true;
}
