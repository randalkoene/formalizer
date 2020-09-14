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
#include "{{ this }}.hpp"


/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif


using namespace fz;


enum template_id_enum {
    {{ a_template }}_temp,
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "{{ template_file }}_template.{{ ext }}",
};

typedef std::map<template_id_enum,std::string> {{ this }}_templates;

bool load_templates({{ this }}_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i], templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

bool render() {
    render_environment env;
    {{ this }}_templates templates;
    load_templates(templates);

    template_varvalues varvals;
    board.emplace("{{ a_variable }}",{{ a_string }});
    std::string rendered_str = env.render(templates[{{ a_template }}_temp], varvals);

    if (!string_to_file({{ a_path }},rendered_str))
            ERRRETURNFALSE(__func__,"unable to write rendered output to file");
    
    return true;
}
