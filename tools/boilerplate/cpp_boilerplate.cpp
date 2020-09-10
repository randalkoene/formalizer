// Copyright 2020 Randal A. Koene
// License TBD

/**
 * cpp_boilerplate is the authoritative C++ source code stub generator for Formalizer development.
 * 
 * For more about this, see the Trello card at https://trello.com/c/8czp28zx.
 */

// std
//#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "templater.hpp"

// local
#include "boilerplate.hpp"
#include "cpp_boilerplate.hpp"

/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif


using namespace fz;


enum template_id_enum {
    cpp_bp_cpp_temp,
    cpp_bp_hpp_temp,
    cpp_bp_Makefile_temp,
    cpp_bp_README_temp,
    cpp_bp_version_temp,
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "cpp_bp_cpp_template.cpp",
    "cpp_bp_hpp_template.hpp",
    "cpp_bp_Makefile_template",
    "cpp_bp_README_template",
    "cpp_bp_version_template.hpp"
};

typedef std::map<template_id_enum,std::string> cpp_bp_templates;

bool load_templates(cpp_bp_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i], templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

int make_cpp_boilerplate() {
    render_environment env;
    cpp_bp_templates templates;
    load_templates(templates);

    //std::string rendered_cards;

    template_varvalues cppvars;
    //cppvars.emplate(,);
    std::string rendered_cpp = env.render(templates[cpp_bp_cpp_temp], cppvars);

    template_varvalues hppvars;
    //hppvars.emplate(,);
    std::string rendered_hpp = env.render(templates[cpp_bp_hpp_temp], hppvars);

    template_varvalues Makefilevars;
    //Makefilevars.emplate(,);
    std::string rendered_Makefile = env.render(templates[cpp_bp_Makefile_temp], Makefilevars);

    template_varvalues READMEvars;
    //READMEvars.emplate(,);
    std::string rendered_README = env.render(templates[cpp_bp_README_temp], READMEvars);

    template_varvalues versionvars;
    //versionvars.emplate(,);
    std::string rendered_version = env.render(templates[cpp_bp_version_temp], versionvars);

    if (!string_to_file(" ",rendered_cpp)) {
        ADDERROR(__func__,"unable to write rendered content to cpp file");
        bp.exit(exit_file_error);
    }
    if (!string_to_file(" ",rendered_hpp)) {
        ADDERROR(__func__,"unable to write rendered content to hpp file");
        bp.exit(exit_file_error);
    }
    if (!string_to_file(" ",rendered_Makefile)) {
        ADDERROR(__func__,"unable to write rendered content to Makefile");
        bp.exit(exit_file_error);
    }
    if (!string_to_file(" ",rendered_README)) {
        ADDERROR(__func__,"unable to write rendered content to README file");
        bp.exit(exit_file_error);
    }
    if (!string_to_file(" ",rendered_version)) {
        ADDERROR(__func__,"unable to write rendered content to version file");
        bp.exit(exit_file_error);
    }

    return bp.completed_ok();
}
