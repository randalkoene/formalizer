// Copyright 2020 Randal A. Koene
// License TBD

/**
 * cpp_boilerplate is the authoritative C++ source code stub generator for Formalizer development.
 * 
 * The options provided by this boilerplate are:
 * - core / tools component
 * - core:library component / core:standalone component
 * - tools component with filesystem category detailing
 * - include template rendering
 * - standard program with standard program derived class, version, module id.
 * - standardized config parameters from config tree
 * - configuration via init register configurable or via configbase
 * - Graph or Log access is presently included just through hints in comments
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
#include "cpp_boilerplate.hpp"


using namespace fz;


enum cpp_template_id_enum {
    cpp_bp_cpp_temp,
    cpp_bp_libcpp_temp,
    cpp_bp_hpp_temp,
    cpp_bp_libhpp_temp,
    cpp_bp_Makefile_temp,
    cpp_bp_README_temp,
    cpp_bp_version_temp,
    cpp_bp_rendercpp_temp,
    cpp_bp_renderhpp_temp,
    cpp_bp_hpp_configurable_temp,
    cpp_bp_cpp_configurable_temp,
    cpp_bp_hpp_configbase_temp,
    cpp_bp_cpp_configbase_temp,
    NUM_temp
};

typedef std::map<cpp_template_id_enum,std::string> cpp_bp_templates;

bool load_templates(cpp_bp_templates & templates) {
    static const std::vector<std::string> template_ids = {
        "cpp_bp_cpp_template.cpp",
        "cpp_bp_libcpp_template.cpp",
        "cpp_bp_hpp_template.hpp",
        "cpp_bp_libhpp_template.hpp",
        "cpp_bp_Makefile_template",
        "cpp_bp_README_template",
        "cpp_bp_version_template.hpp",
        "cpp_bp_render_template.cpp",
        "cpp_bp_render_template.hpp",
        "cpp_bp_hpp_configurable_template.hpp",
        "cpp_bp_cpp_configurable_template.cpp",
        "cpp_bp_hpp_configbase_template.hpp",
        "cpp_bp_cpp_configbase_template.cpp"
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
        if (!file_to_string(template_dir + "/" + template_ids[i], templates[static_cast<cpp_template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

int make_cpp_boilerplate() {
    ERRHERE(".top");
    render_environment env;
    cpp_bp_templates templates;
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
    std::string headerdir = formalizer_path_dir(thisname,category,iscore,islibrary,true);
    std::string sourcedir = formalizer_path_dir(thisname,category,iscore,islibrary,false);

    bool uses_config = ask_boolean_choice("Uses standardized config file? [y]/[N] ",'y',"INCLUDE CONFIG METHOD","NO CONFIG PARAMETERS");
    bool configurable = false;
    if (uses_config) {
        configurable = !ask_boolean_choice("Configurable at program start via init register? [Y]/[n]",'n',"USES CONFIGBASE","USES CONFIGURABLE");
    }

    bool includetemplater = false;
    if (!islibrary) {
        includetemplater = ask_boolean_choice("Include template rendering file? [y]/[N] ",'y',"INCLUDE TEMPLATE RENDERING","NO TEMPLATE RENDERING");
    }

    std::string abbreviation, moduleid, versionstr;
    if (!islibrary) {
        abbreviation = ask_string_input("Abbreviation (e.g. \"bp\" for \"boilerplate\"): ",false);
        moduleid = ask_string_input("Module id sans 'Formalizer:' (e.g. \"Development:AutoGeneration:Init\"): ",false);
        versionstr = ask_string_input("Version (e.g. \"0.1.0-0.1\"): ",false);
    }

    ERRHERE(".cpp");
    template_varvalues cppvars;
    std::string rendered_cpp;
    cppvars.emplace("this",thisname);
    cppvars.emplace("thedate",datestr);
    if (includetemplater) {
        cppvars.emplace("render_hpp","#include \"render.hpp\"");
    } else {
        cppvars.emplace("render_hpp","");
    }
    if (uses_config) {
        if (configurable) {
            cppvars.emplace("construct_inherited_config_access_etc", ": formalizer_standard_program(false), config(*this)");
            template_varvalues cppconfvars;
            std::string rendered_cppconfig;
            cppconfvars.emplace("th",abbreviation);
            rendered_cppconfig = env.render(templates[cpp_bp_cpp_configurable_temp], cppconfvars);
            cppvars.emplace("config_set_parameter_implementation", rendered_cppconfig);
        } else {
            cppvars.emplace("construct_inherited_config_access_etc", ": formalizer_standard_program(false)");
            template_varvalues cppconfvars;
            std::string rendered_cppconfig;
            cppconfvars.emplace("th",abbreviation);
            cppconfvars.emplace("this",thisname);
            rendered_cppconfig = env.render(templates[cpp_bp_cpp_configbase_temp], cppconfvars);
            cppvars.emplace("config_set_parameter_implementation", rendered_cppconfig);
        }
    } else {
        cppvars.emplace("construct_inherited_config_access_etc", ": formalizer_standard_program(false)");
        cppvars.emplace("config_set_parameter_implementation", "");
    }
    if (!islibrary) {
        cppvars.emplace("th", abbreviation);
        cppvars.emplace("module_id", moduleid);
        rendered_cpp = env.render(templates[cpp_bp_cpp_temp], cppvars);
    } else {
        rendered_cpp = env.render(templates[cpp_bp_libcpp_temp], cppvars);
    }

    ERRHERE(".hpp");
    template_varvalues hppvars;
    std::string rendered_hpp;
    hppvars.emplace("this",thisname);
    hppvars.emplace("thedate",datestr);
    hppvars.emplace("CAPSthis",CAPSthis);
    if (uses_config) {
        hppvars.emplace("config_include", "#include \"config.hpp\"");
        if (configurable) {
            template_varvalues hppconfvars;
            std::string rendered_hppconfig;
            hppconfvars.emplace("this",thisname);
            hppconfvars.emplace("th",abbreviation);
            rendered_hppconfig = env.render(templates[cpp_bp_hpp_configurable_temp], hppconfvars);
            hppvars.emplace("th_configurable_or_configbase",rendered_hppconfig);
            hppvars.emplace("config_support",abbreviation+"_configurable config;");
        } else {
            template_varvalues hppconfvars;
            std::string rendered_hppconfig;
            hppconfvars.emplace("th",abbreviation);
            rendered_hppconfig = env.render(templates[cpp_bp_hpp_configbase_temp], hppconfvars);
            hppvars.emplace("th_configurable_or_configbase",rendered_hppconfig);
            hppvars.emplace("config_support","friend "+abbreviation+"_configbase;\n\t"+abbreviation+"_configbase config;");
        }
    } else {
        hppvars.emplace("config_include", "");
        hppvars.emplace("th_configurable_or_configbase", "");
        hppvars.emplace("config_support", "");
    }
    
    if (!islibrary) {
        hppvars.emplace("th", abbreviation);
        rendered_hpp = env.render(templates[cpp_bp_hpp_temp], hppvars);
    } else {
        rendered_hpp = env.render(templates[cpp_bp_libhpp_temp], hppvars);
    }

    std::string rendered_Makefile, rendered_README, rendered_version;
    if (!islibrary) {

        ERRHERE(".Makefile");
        template_varvalues Makefilevars;
        Makefilevars.emplace("this",thisname);
        if (iscore) {
            Makefilevars.emplace("tools_or_core","core");
        } else {
            std::string toolsorcorestr("tools");
            if (!category.empty()) {
                toolsorcorestr += '/' + category;
            }
            Makefilevars.emplace("tools_or_core",toolsorcorestr);
        }
        if (includetemplater) {
            Makefilevars.emplace("templater_target","$(OBJ)/render.o: render.cpp render.hpp\n"
                "	$(CCPP) $(CPPFLAGS) $(COLORS) $(STANDARD_STREAMS) $(FORMALIZERROOT) $(TEMPLATEDIR) -c render.cpp -o $(OBJ)/render.o\n"
                "\n");
            Makefilevars.emplace("templater_macro","$(TEMPLATEDIR) ");
            Makefilevars.emplace("templater_obj","$(OBJ)/render.o ");
        } else {
            Makefilevars.emplace("templater_target","");
            Makefilevars.emplace("templater_macro","");
            Makefilevars.emplace("templater_obj","");
        }
        rendered_Makefile = env.render(templates[cpp_bp_Makefile_temp], Makefilevars);

        ERRHERE(".README");
        template_varvalues READMEvars;
        READMEvars.emplace("this",thisname);
        READMEvars.emplace("thedate",datestr);
        rendered_README = env.render(templates[cpp_bp_README_temp], READMEvars);

        ERRHERE(".version");
        template_varvalues versionvars;
        versionvars.emplace("version",versionstr);
        rendered_version = env.render(templates[cpp_bp_version_temp], versionvars);

    }

    std::string rendered_rcpp, rendered_rhpp;
    if (includetemplater) {
        ERRHERE(".render.cpp");
        template_varvalues rcppvars;
        rcppvars.emplace("this", thisname);
        rendered_rcpp = env.render(templates[cpp_bp_rendercpp_temp], rcppvars);

        ERRHERE(".render.hpp");
        template_varvalues rhppvars;
        rhppvars.emplace("this", thisname);
        rendered_rhpp = env.render(templates[cpp_bp_renderhpp_temp], rhppvars);
    }

    ERRHERE(".makedir");
    if (!islibrary) {
        std::filesystem::create_directories(sourcedir);
    }
    if (includetemplater) {
        std::filesystem::create_directories(sourcedir+"/templates");
    }

    ERRHERE(".files");
    careful_file_create(sourcedir+'/'+thisname+".cpp",rendered_cpp);
    careful_file_create(headerdir+'/'+thisname+".hpp",rendered_hpp);

    if (includetemplater) {
        careful_file_create(sourcedir+"/render.cpp",rendered_rcpp);
        careful_file_create(sourcedir+"/render.hpp",rendered_rhpp);
    }

    if (!islibrary) {

        std::filesystem::create_directories(sourcedir+"/obj");

        careful_file_create(sourcedir+"/Makefile",rendered_Makefile);
        careful_file_create(sourcedir+"/README.md",rendered_README);
        careful_file_create(sourcedir+"/version.hpp",rendered_version);

        if (iscore) {
            FZOUT("\nPlease remember to add:\n\tEXECUTABLES += $(COREPATH)/"+thisname+'/'+thisname+"\nto the root Makefile.\n");
        } else {
            FZOUT("\nPlease remember to add:\n\tEXECUTABLES += $(TOOLSPATH)/"+thisname+'/'+thisname+"\nto the root Makefile.\n");
        }
        FZOUT("\nPlease check for any necessary updates of:\n\tSYMBIN, CGIEXE or WEBINTERFACES\nin the root Makefile.\n");
    }
    if (uses_config) {
        FZOUT("\nPlease add to:\n\texecutables.py, or,\n\tcoreconfigurable.py,\nso that `fzsetup.py -1 config` prepares the configuration file location.\n");
    }
    FZOUT("\nPlease check `fzinfo` in case information about this new component needs to be added.\n");
    FZOUT("\nPlease consider adding a build rule for this component to `fzbuild.py`.\n");

    return standard.completed_ok();
}
