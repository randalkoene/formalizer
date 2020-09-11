// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ brief_description }}
 * 
 * {{ long_description }}
 * 
 * For more about this, see {{ doc_reference }}.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Server:Guide:System"

// std
#include <cstdlib>
#include <iostream>
#include <map>

// core
#include "version.hpp"
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "fzpostgres.hpp"

// local
#include "fzguide.system.hpp"


using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzguide_system fgs;

const std::string pq_guide_system_layout(
    "id char(32) PRIMARY KEY," // e.g. section:subsection:index (e.g. "morning:wakeup:02.0")
    "snippet text"             // a descriptive text
);

std::map<fgs_section,const std::string> sectiontag = {
    { fgs_am, "am" },
    { fgs_pm, "pm" }
};

std::map<fgs_subsection,const std::string> subsectiontag = {
    { fgs_wakeup, "wakeup" }
};

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzguide_system::fzguide_system(): section(fgs_am), subsection(fgs_wakeup), decimalidx(1.0), flowcontrol(flow_unknown), pa(add_option_args, add_usage_top, true) {
    add_option_args += "SRAPU:x:i:";
    add_usage_top += " <-S|-R> [-A|-P] [-U <subsection>] [-x <idx>] [-i <inputfile>]";
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzguide_system::usage_hook() {
    pa.usage_hook();
    FZOUT("    -S Store new snippet in System guide\n");
    FZOUT("    -R Read snippet from System guide\n");
    FZOUT("    -A System guide section: AM\n");
    FZOUT("    -P System guide section: PM\n");
    FZOUT("    -U System guide <subsection>\n");
    FZOUT("    -x System guide decimal index number <idx>\n");
    FZOUT("    -i read snippet content from file (otherwise from STDIN)\n");
}

/**
 * Handler for command line options that are defined in the derived class
 * as options specific to the program.
 * 
 * Include case statements for each option. Typical handlers do things such
 * as collecting parameter values from `cargs` or setting `flowcontrol` choices.
 * 
 * @param c is the character that identifies a specific option.
 * @param cargs is the optional parameter value provided for the option.
 */
bool fzguide_system::options_hook(char c, std::string cargs) {
    if (pa.options_hook(c,cargs))
        return true;

    switch (c) {

    case 'S': {
        flowcontrol = flow_store;
        return true;
    }

    case 'R': {
        flowcontrol = flow_read;
        return true;
    }

    case 'A': {
        section = fgs_am;
        return true;
    }

    case 'P': {
        section = fgs_pm;
        return true;
    }

    case 'U': {
        if (cargs == "wakeup") {
            subsection = fgs_wakeup;
            return true;
        }
        return false;
    }

    case 'x': {
        float idx_float = atof(cargs.c_str());
        if (idx_float<=0.0)
            return false;
        decimalidx = idx_float;
        return true;
    }

    case 'i': {
        source = cargs;
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
void fzguide_system::init_top(int argc, char *argv[]) {
    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

std::string Guide_snippet_system::layout() const {
    return pq_guide_system_layout;
}

std::string Guide_snippet_system::all_values_pqstr() const {
    return "'"+section+':'+subsection+':'+idxstr+"', $txt$"+snippet+"$txt$";
}


int store_snippet() {
    VERBOSEOUT("Collecting snippet to store to the System Guide...\n\n");
    Guide_snippet_system snippet;
    if (fgs.source.empty()) { // from STDIN
        if (!stream_to_string(std::cin,snippet.snippet)) {
            ADDERROR(__func__,"unable to read snippet from STDIN");
            fgs.exit(exit_file_error);
        }
    } else {
        if (!file_to_string(fgs.source,snippet.snippet)) {
            ADDERROR(__func__,"unable to read snippet from "+fgs.source);
            fgs.exit(exit_file_error);
        }
    }

    snippet.section = sectiontag[fgs.section];
    snippet.subsection = subsectiontag[fgs.subsection];
    snippet.idxstr = to_precision_string(fgs.decimalidx,1);

    if (!store_Guide_snippet_pq(snippet, fgs.pa)) {
        ADDERROR(__func__,"unable to store snippet");
        fgs.exit(exit_database_error);
    }

    return fgs.completed_ok();
}

int main(int argc, char *argv[]) {
    fgs.init_top(argc, argv);

    switch (fgs.flowcontrol) {

    case flow_store: {
        return store_snippet();
    }

    default: {
        fgs.print_usage();
    }

    }

    return fgs.completed_ok();
}
