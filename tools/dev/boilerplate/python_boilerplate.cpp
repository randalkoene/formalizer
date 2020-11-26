// Copyright 2020 Randal A. Koene
// License TBD

/**
 * python_boilerplate is the authoritative Python source code stub generator for Formalizer development.
 * 
 * The options provided by this boilerplate are:
 * - core / tools component
 * - core:library component / core:standalone component
 * - tools component with filesystem category detailing
 * - include template rendering
 * - standard program with standard program derived class, version, module id.
 * - standardized config parameters from config tree
 * 
 * - TEST all the different variants, make sure they even compile, delete the tests when done
 * 
 * For more about this, see the Trello card at https://trello.com/c/8czp28zx.
 */

// std
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <string>

// core
#include "error.hpp"
#include "stringio.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "templater.hpp"
#include "TimeStamp.hpp"

// local
#include "boilerplate.hpp"
#include "python_boilerplate.hpp"


using namespace fz;

enum python_template_id_enum {
    python_bp_py_temp,
    python_bp_libpy_temp,
    python_bp_README_temp,
    NUM_temp
};

typedef std::map<python_template_id_enum,std::string> python_bp_templates;

bool load_templates(python_bp_templates & templates) {
    static const std::vector<std::string> template_ids = {
        "python_bp_py_template.py",
        "python_bp_libpy_template.py",
        "python_bp_README_template",
    };

    templates.clear();

/// The Makefile attempts to provide this at compile time based on the source file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif

    VERBOSEOUT("Using template directory: "+template_dir+'\n');

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i], templates[static_cast<python_template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

int make_python_boilerplate() {
    ERRHERE(".top");
    render_environment env;
    python_bp_templates templates;
    if (!load_templates(templates))
        standard.exit(exit_file_error); // Don't continue if you don't have the templates.

    std::string thisname = ask_string_input("Component name (e.g. boilerplate): ", false);
    std::string CAPSthis = thisname;
    std::transform(CAPSthis.begin(), CAPSthis.end(),CAPSthis.begin(), ::toupper);

    std::string datestr = DateStampYmd(ActualTime());

    bool iscore = ask_boolean_choice(
        "A Formalizer component belongs in `core` if:\n\n"
        "  1. It is an essential component. There is no other lower-level\n"
        "     program that supplies the essential content.\n"
        "  2. A program performs a task that is clearly categorized as belonging\n"
        "     to one of the first three abstraction layers: the **Data Layer**,\n"
        "     the **Database Layer**, or the **Data Server Layer**.\n\n"
        "Is "+thisname+" a [c]ore or a [t]ools component? "
        ,'c',"CORE","TOOLS");

    bool islibrary = false;
    std::string category;
    if (iscore) {
        islibrary = ask_boolean_choice("Is this adding [l]ibrary objects or a [s]tandalone program? ",'l',"LIBRARY","STANDALONE");
    } else {
        category = ask_string_input("Optional category (e.g. \"system/metrics\", \"dev\", \"conversion\"): ",true);
    }
    std::string sourcedir = formalizer_path_dir(thisname,category,iscore,islibrary,false);

    bool uses_config = ask_boolean_choice("Uses standardized config file? [y]/[N] ",'y',"INCLUDE CONFIG METHOD","NO CONFIG PARAMETERS");

    std::string abbreviation, moduleid, versionstr;
    if (!islibrary) {
        abbreviation = ask_string_input("Abbreviation (e.g. \"bp\" for \"boilerplate\"): ",false);
        moduleid = ask_string_input("Module id sans 'Formalizer:' (e.g. \"Development:AutoGeneration:Init\"): ",false);
        versionstr = ask_string_input("Version (e.g. \"0.1.0-0.1\"): ",false);
    }

    ERRHERE(".py");
    template_varvalues pyvars;
    std::string rendered_py;
    pyvars.emplace("this",thisname);
    pyvars.emplace("thedate", datestr);
    pyvars.emplace("version", versionstr);
    pyvars.emplace("module_id", moduleid);

    if (uses_config) {
        //pyvars.emplace("", "");
    } else {
        //pyvars.emplace("", "");
    }
    if (!islibrary) {
        //pyvars.emplace("", "");
        rendered_py = env.render(templates[python_bp_py_temp], pyvars);
    } else {
        rendered_py = env.render(templates[python_bp_libpy_temp], pyvars);
    }

    std::string rendered_README;
    if (!islibrary) {

        ERRHERE(".README");
        template_varvalues READMEvars;
        READMEvars.emplace("this",thisname);
        READMEvars.emplace("thedate", datestr);
        rendered_README = env.render(templates[python_bp_README_temp], READMEvars);

    }

    ERRHERE(".makedir");
    if (!islibrary) {
        std::filesystem::create_directories(sourcedir);
    }

    ERRHERE(".files");
    careful_file_create(sourcedir+'/'+thisname+".py",rendered_py);

    if (!islibrary) {

        careful_file_create(sourcedir+"/README.md",rendered_README);

        if (iscore) {
            FZOUT("\nPlease remember to add:\n\tEXECUTABLES += $(COREPATH)/"+thisname+'/'+thisname+".py\nto the root Makefile.\n");
        } else {
            FZOUT("\nPlease remember to add:\n\tEXECUTABLES += $(TOOLSPATH)/"+thisname+'/'+thisname+".py\nto the root Makefile.\n");
        }
        FZOUT("\nPlease check for any necessary updates of:\n\tSYMBIN, CGIEXE or WEBINTERFACES\nin the root Makefile.\n");
    }
    if (uses_config) {
        FZOUT("\nPlease add to:\n\texecutables.py, or,\n\tcoreconfigurable.py,\nso that `fzsetup.py -1 config` prepares the configuration file location.\n");
    }
    FZOUT("\nPlease check `fzinfo` in case information about this new component needs to be added.\n");

    return standard.completed_ok();
}
