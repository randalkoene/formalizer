// Copyright 2020 Randal A. Koene
// License TBD

/** @file boilerplate.cpp
 * boilerplate is the authoritative source code stub generator for Formalizer development.
 * 
 * Source code stubs for multple programming languages can be generated and the process
 * can include everything from setting up a directory in the correct place within the
 * Formalizer source code hierarchy, to initializing the main source files for a
 * standardized Formalizer program, to auto-generating necessary additional files,
 * such as `version.hpp` and a Makefile.
 * 
 * For more about this, see the Trello card at https://trello.com/c/8czp28zx.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Development:AutoGeneration:Init"

// std
#include <iostream>
#include <filesystem>

// core
#include "version.hpp"
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"

// local
#include "boilerplate.hpp"
#include "cpp_boilerplate.hpp"

/// Typically provide this through -D in Makefile.
#ifndef FORMALIZER_ROOT
    #define FORMALIZER_ROOT "/home/randalk/src/formalizer"
#endif

using namespace fz;

boilerplate bp;

boilerplate::boilerplate() {
    add_option_args += "T:";
    add_usage_top += " [-T <target>]";
}

void boilerplate::usage_hook() {
    FZOUT("    -T target (language): cpp, python\n");        
}

bool boilerplate::options_hook(char c, std::string cargs) {

    switch (c) {

    case 'T':
        if (cargs == "cpp") {
            flowcontrol = flow_cpp;
            return true;
        }
        if (cargs == "python") {
            flowcontrol = flow_python;
            return true;
        }

    }

    return false;
}

/**
 * Initialize configuration parameters.
 * Call this at the top of main().
 * 
 * @param argc command line parameters count forwarded from main().
 * @param argv command line parameters array forwarded from main().
 */
void boilerplate::init_top(int argc, char *argv[]) {
    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);

    if (!std::filesystem::exists(FORMALIZER_ROOT)) {
        ADDERROR(__func__,"the compile-time Formalizer root definition FORMALIZER_ROOT does not exist at: " FORMALIZER_ROOT);
        exit(exit_file_error);
    }
}


// +----- begin: functions shared across targets -----+

/**
 * Interactively request a choice of two options.
 * 
 * @param description describes the choice and presents the two characters that choose one or the other.
 * @param truechar is the character that must be the first letter of the response to return true.
 * @param trueconfirm is an optional string to print when the choice is evaluated as true.
 * @param falseconfirm is an optional string to print when the choice is evaluated as false.
 * @return true if `truechar` is the response received.
 */
bool ask_boolean_choice(std::string description, char truechar, std::string trueconfirm, std::string falseconfirm) {
    FZOUT(description);
    std::string responsestr;
    std::cin >> responsestr;
    bool boolres = false;
    if (responsestr.size()>0)
        if (responsestr[0] == truechar) boolres = true;

    if (boolres) {
        if (!trueconfirm.empty())
            FZOUT(trueconfirm+'\n');
    } else {
        if (!falseconfirm.empty())
            FZOUT(falseconfirm+'\n');
    }
    return boolres;
}

/**
 * Interactively request a string of text.
 * 
 * @param description describes the text string requested.
 * @param allowempty allows an empty text string if true.
 * @return the string.
 */
std::string ask_string_input(std::string description, bool allowempty) {
    FZOUT(description);
    std::string inputstr;
    if (allowempty) {
        std::cin >> inputstr;
    } else {
        while (inputstr.empty()) {
            std::cin >> inputstr;
            if (inputstr.empty()) {
                FZOUT("Unable to accept empty text, please try again: ");
            }
        }
    }
    return inputstr;
}

/**
 * Generate the standardized path directory within the Formalizer source code tree
 * for a given component name.
 * 
 * @param compname name of the component (e.g. `boilerplate`).
 * @param category optional category name for `tools` (e.g. "system/metrics", "dev", "conversion").
 * @param iscore flag, set true if component belongs in `core`. See https://trello.com/c/Vz4ujwgb.
 * @param islibrary is true if the components adds library objects instead of a standalone program.
 * @param isheader is true if thie component is a .hpp instead of a .cpp (or other).
 * @return the absolute path of the directory where the component belongs.
 */
std::string formalizer_path_dir(std::string compname, std::string category, bool iscore, bool islibrary, bool isheader) {
    if (iscore) {
        if (islibrary) {
            if (isheader) {
                return FORMALIZER_ROOT "/core/include";
            } else {
                return FORMALIZER_ROOT "/core/lib";
            }
        } else {
            return FORMALIZER_ROOT "/core/" + compname;
        }
    } else {
        if (!category.empty()) {
            return FORMALIZER_ROOT "/tools/" + category + '/' + compname;
        } else {
            return FORMALIZER_ROOT "/tools/" + compname;
        }
    }
}

void careful_file_create(std::string filename, std::string & filecontent) {
    if (std::filesystem::exists(filename)) {
        ADDERROR(__func__,"the file "+filename+" already exists");
        bp.exit(exit_file_error);
    }

    if (!string_to_file(filename,filecontent)) {
        ADDERROR(__func__,"unable to write content to "+filename);
        bp.exit(exit_file_error);
    }
}

// +----- end  : functions shared across targets -----+

int main(int argc, char *argv[]) {
    bp.init_top(argc, argv);

    FZOUT("\nThis is the stub of the stub generator.\n\n");
    key_pause();

    switch (bp.flowcontrol) {

    case flow_cpp: {
        return make_cpp_boilerplate();
    }

    case flow_python: {

        break;
    }

    default: {
        bp.print_usage();
    }

    }

    return bp.completed_ok();
}

