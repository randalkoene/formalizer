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

render_environment env;
template_varvalues inner_varvals;

enum template_id_enum {
    index_temp,
    index_section_temp,
    index_button_here_temp,
    index_button_there_temp,
    NUM_temp
};

const std::vector<std::string> template_ids[2] = {
    {
        "template.html",
        "section_template.html",
        "button_here_template.html",
        "button_there_template.html",
    },
    {
        "static_template.html",
        "static_section_template.html",
        "static_button_here_template.html",
        "static_button_there_template.html"  
    }
};

typedef std::map<template_id_enum,std::string> fzdashboard_templates;

bool load_templates(std::string dashboardlabel, fzdashboard_templates & templates, dynamic_or_static html_output = dynamic_html) {
    templates.clear();

    std::string template_directory;
    if (dashboardlabel.substr(0,7) == "custom:") {
        template_directory = dashboardlabel.substr(7);
    } else {
        template_directory = template_dir + "/" + dashboardlabel + "/";
    }
    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_directory + template_ids[html_output][i], templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_directory + template_ids[html_output][i]);
    }

    return true;
}

/* *** The following are not presently in use. ***
struct button_data {
    std::string url;
    std::string label;
    bool needs_SPA_info = false; // by default, assume that using '?SPA=no' (single page application) specification for the static HTML version is not necessary
};

struct section_data {
    std::string title;
    std::vector<button_data> buttons;
};

struct button_section_data {
    std::vector<section_data> sections;
};
*/

// *** We don't need this if we make JSON blocks use maps to search by label.
std::string text_by_label(JSON_element_data_vec & buffers, const std::string matchlabel) {
    for (auto & buffer : buffers) {
        if (buffer.label == matchlabel) {
            return env.render(buffer.text, inner_varvals); // The JSON string value can contain placeholders that are filled in according to inner_varvals.
        }
    }
    return "";
}
bool flag_by_label(JSON_element_data_vec & buffers, const std::string matchlabel) {
    for (auto & buffer : buffers) {
        if (buffer.label == matchlabel) {
            return buffer.flag;
        }
    }
    return false;
}
void set_text_by_label(JSON_element_data_vec & buffers, const std::string matchlabel, std::string text) {
    for (auto & buffer : buffers) {
        if (buffer.label == matchlabel) {
            buffer.text = text;
            return;
        }
    }
}

/* We might even want to do this if we want to use more complicated strings in the URL:

string url_encode(const string &value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << uppercase;
        escaped << '%' << setw(2) << int((unsigned char) c);
        escaped << nouppercase;
    }

    return escaped.str();
}

*/

// We need to do this, because double quotes are not allowed in URL strings.
std::string escape_double_quotes(const std::string & unescaped_str) {
    std::string escaped_str(unescaped_str.size()*3, '_');
    size_t j = 0;
    for (size_t i = 0; i < unescaped_str.size(); ++i) {
        if (unescaped_str[i] == '"') {
            escaped_str[j++] = '%';
            escaped_str[j++] = '2';
            escaped_str[j++] = '2';
        } else {
            escaped_str[j++] = unescaped_str[i];
        }
    }
    escaped_str.resize(j);
    return escaped_str;
}

std::string render_buttons(render_environment & env, fzdashboard_templates & templates, JSON_block * block_ptr, dynamic_or_static html_output = dynamic_html) {
    if (!block_ptr) {
        return "";
    }
    std::string buttons_str;
    JSON_element_data_vec button_info; // register which variables to look for in the JSON string
    button_info.emplace_back("url");
    button_info.emplace_back("window");
    button_info.emplace_back("SPAinfo");
    std::string button_num_char("1");
    for (auto & button : block_ptr->elements) {
        if (is_populated_JSON_block(button.get())) {
            template_varvalues varvals;
            varvals.emplace("label", button->label);
            bool here = false;
            // grab data for the labels in `button_info`, only 2 are mandatory, "SPAinfo" is optional
            if (button->children->find_many(button_info) >= 2) {
                here = (text_by_label(button_info, "window") != "_blank");
                if (html_output == dynamic_html) {
                    varvals.emplace("url", escape_double_quotes(text_by_label(button_info, "url")));
                    varvals.emplace("num", button_num_char);
                } else {
                    varvals.emplace("url", escape_double_quotes(text_by_label(button_info, "url"))+text_by_label(button_info, "SPAinfo")); // using a string and not a flag here, because some URLs may already have arguments (use "&SPA=no") and others not (use "?SPA=no")
                    set_text_by_label(button_info, "SPAinfo", ""); // explicitly clear, since it doesn't always get replaced
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

    fzdashboard_templates templates;
    load_templates(fzdsh.target, templates, html_output);

    // Prepare a few globally applicable template variable replacements
    inner_varvals.emplace("fzserverpq",fzdsh.graph().get_server_full_address());

    std::string button_sections;

    for (auto & section : data.content_elements()) {
        if (is_populated_JSON_block(section.get())) {
            template_varvalues varvals;
            if (section->label == "NOTITLE") {
                varvals.emplace("section_heading", "");
            } else {
                varvals.emplace("section_heading", section->label);
            }
            varvals.emplace("buttons", render_buttons(env, templates, section->children.get(), html_output));
            button_sections += env.render(templates[index_section_temp], varvals);
        }
    }

    template_varvalues varvals;
    varvals.emplace("button-sections",button_sections);
    std::string rendered_str = env.render(templates[index_temp], varvals);

    std::string embed_html_name;
    if (fzdsh.target.substr(0,7) == "custom:") {
        embed_html_name = "custom";
    } else {
        embed_html_name = fzdsh.target;
    }
    std::string output_name;
    if (html_output == dynamic_html) {
        output_name += '/' + embed_html_name + ".html";
    } else {
        output_name += '/' + embed_html_name + "-static.html";
    }
    
    return send_rendered_to_output(output_name, rendered_str);
}
