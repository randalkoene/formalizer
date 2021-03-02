// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Template rendering functions.
 * 
 */

// std
#include <memory>
#include <vector>
#include <cstdlib>

// core
#include "error.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "jsonlite.hpp"
#include "templater.hpp"

// local
#include "render.hpp"
#include "fzdashboard.hpp"


/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif


using namespace fz;


enum template_id_enum {
    index_temp,
    index_section_temp,
    index_button_here_temp,
    index_button_there_temp,
    NUM_temp
};

const std::vector<std::string> template_ids[2] = {
    {
        "index_template.html",
        "index_section_template.html",
        "index_button_here_template.html",
        "index_button_there_template.html",
    },
    {
        "index-static_template.html",
        "index-static_section_template.html",
        "index-static_button_here_template.html",
        "index-static_button_there_template.html"  
    }
};

typedef std::map<template_id_enum,std::string> fzdashboard_templates;

bool load_templates(fzdashboard_templates & templates, dynamic_or_static html_output = dynamic_html) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[html_output][i], templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[html_output][i]);
    }

    return true;
}

struct button_data {
    std::string url;
    std::string label;
};

struct section_data {
    std::string title;
    std::vector<button_data> buttons;
};

struct button_section_data {
    std::vector<section_data> sections;
};

// *** We don't need this if we make JSON blocks use maps to search by label.
std::string value_by_label(JSON_element_data_vec & buffers, const std::string matchlabel) {
    for (auto & buffer : buffers) {
        if (buffer.label == matchlabel) {
            return buffer.text;
        }
    }
    return "";
}

std::string render_buttons(render_environment & env, fzdashboard_templates & templates, JSON_block * block_ptr, dynamic_or_static html_output = dynamic_html) {
    if (!block_ptr) {
        return "";
    }
    std::string buttons_str;
    JSON_element_data_vec button_info;
    button_info.emplace_back("url");
    button_info.emplace_back("window");
    std::string button_num_char("1");
    for (auto & button : block_ptr->elements) {
        if (is_populated_JSON_block(button.get())) {
            template_varvalues varvals;
            varvals.emplace("label", button->label);
            bool here = false;
            if (button->children->find_many(button_info) == 2) {
                here = (value_by_label(button_info, "window") != "_blank");
                varvals.emplace("url", value_by_label(button_info, "url"));
                if (html_output == dynamic_html) {
                    varvals.emplace("num", button_num_char);
                }
                if (here) {
                    buttons_str += env.render(templates[index_button_here_temp], varvals);
                } else {
                    buttons_str += env.render(templates[index_button_there_temp], varvals);
                }
                if (button_num_char == "1") {
                    button_num_char = "2";
                } else {
                    button_num_char = "1";
                }
            }
        }
    }
    return buttons_str;
}

bool send_rendered_to_output(std::string & filename_without_dir, std::string & rendered_text) {
    if ((fzdsh.config.top_path.empty()) || (fzdsh.config.top_path == "STDOUT")) { // to STDOUT
        //VERBOSEOUT("Log interval:\n\n");
        FZOUT(rendered_text);
        return true;
    }
    
    std::string output_path = fzdsh.config.top_path + filename_without_dir;
    VERBOSEOUT("Writing rendered output to "+output_path+".\n\n");
    if (!string_to_file(output_path,rendered_text)) {
        ERRRETURNFALSE(__func__,"unable to write to "+output_path);
    }
    return true;
}

bool render(std::string & json_str, dynamic_or_static html_output) {
    JSON_data data(json_str);
    VERYVERBOSEOUT("JSON data:\n"+data.json_str());
    VERBOSEOUT("Number of blocks  : "+std::to_string(data.blocks())+'\n');
    VERBOSEOUT("Number of elements: "+std::to_string(data.size())+'\n');

    render_environment env;
    fzdashboard_templates templates;
    load_templates(templates, html_output);

    std::string button_sections;

    for (auto & section : data.content_elements()) {
        if (is_populated_JSON_block(section.get())) {
            template_varvalues varvals;
            if (section->label == "NOTITLE") {
                varvals.emplace("section_heading", "");
            } else {
                varvals.emplace("section_heading", section->label);
            }
            varvals.emplace("buttons", render_buttons(env, templates, section->children.get()));
            button_sections += env.render(templates[index_section_temp], varvals);
        }
    }

    template_varvalues varvals;
    varvals.emplace("button-sections",button_sections);
    std::string rendered_str = env.render(templates[index_temp], varvals);

    std::string output_name;
    if (html_output == dynamic_html) {
        output_name += "/index.html";
    } else {
        output_name += "/index-static.html";
    }
    
    return send_rendered_to_output(output_name, rendered_str);
}
