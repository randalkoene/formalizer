// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Authoritative server of System guide content.
 * 
 * This Formalizer environment server program provides a target independent authoritative source
 * for content in the System guide. The server program `fzguide.system` carries out interactions
 * with the underlying database to retrieve guide content or store new guide content.
 * 
 * For more about this, see the README.md file and cards at https://trello.com/c/6Bt1nyBz and
 * https://trello.com/c/TQ9lVjuH.
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
#include "templater.hpp"

// local
#include "fzguide.system.hpp"

/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif

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
fzguide_system::fzguide_system() : formalizer_standard_program(false), section(fgs_am), subsection(fgs_wakeup),
                                   decimalidx(1.0), format(format_none), flowcontrol(flow_unknown),
                                   pa(*this, add_option_args, add_usage_top, true) {
    add_option_args += "SRAPU:x:i:o:F:";
    add_usage_top += " <-S|-R> [-A|-P] [-U <subsection>] [-x <idx>] [-i <inputfile>] [-o <outputfile>] [-F <format>]";
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
    FZOUT("    -i read snippet content from <inputfile> (otherwise from STDIN)\n");
    FZOUT("    -o write snippet content to <outputfile> (otherwise to STDOUT)\n");
    FZOUT("    -F format result as: txt, html, fullhtml (default=none)\n");
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
    ERRTRACE;
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

    case 'o': {
        dest = cargs;
        return true;
    }

    case 'F': {
        if (cargs=="txt") {
            format = format_txt;
            return true;
        }
        if (cargs=="html") {
            format = format_html;
            return true;
        }
        if (cargs=="fullhtml") {
            format = format_fullhtml;
            return true;
        }
        return false;
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
    ERRTRACE;
    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

void Guide_snippet_system::set_id(const fzguide_system & _fzgs) {
    section = sectiontag[_fzgs.section];
    subsection = subsectiontag[_fzgs.subsection];
    idxstr = to_precision_string(_fzgs.decimalidx,1,'0',4);
}

std::string Guide_snippet_system::layout() const {
    return pq_guide_system_layout;
}

std::string Guide_snippet_system::idstr() const {
    return "'"+section+':'+subsection+':'+idxstr+"'";
}

std::string Guide_snippet_system::all_values_pqstr() const {
    return idstr()+", $txt$"+snippet+"$txt$";
}


int store_snippet() {
    ERRTRACE;
    Guide_snippet_system snippet;
    if (fgs.source.empty()) { // from STDIN
        VERBOSEOUT("Collecting snippet to store to the System Guide from STDIN until EOF (CTRL+D)...\n\n");
        if (!stream_to_string(std::cin,snippet.snippet)) {
            ADDERROR(__func__,"unable to read snippet from STDIN");
            standard.exit(exit_file_error);
        }
    } else {
        VERBOSEOUT("Collecting snippet to store to the System Guide from "+fgs.source+"...\n\n");
        if (!file_to_string(fgs.source,snippet.snippet)) {
            ADDERROR(__func__,"unable to read snippet from "+fgs.source);
            standard.exit(exit_file_error);
        }
    }

    snippet.set_id(fgs);
    VERBOSEOUT("Storing snippet to "+snippet.tablename+" with ID="+snippet.idstr()+"\n\n");

    if (!store_Guide_snippet_pq(snippet, fgs.pa)) {
        ADDERROR(__func__,"unable to store snippet");
        standard.exit(exit_database_error);
    }

    return standard.completed_ok();
}

enum template_id_enum {
    format_txt_temp = format_txt,
    format_html_temp = format_html,
    format_fullhtml_temp = format_fullhtml,
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "format_txt_template.txt",
    "format_html_template.html",
    "format_fullhtml_template.html"
};

typedef std::map<template_id_enum,std::string> format_templates;

bool load_templates(format_templates & templates) {
    ERRTRACE;
    templates.clear();

    VERBOSEOUT("Using template directory: "+template_dir+'\n');

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i], templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

std::string format_snippet(const std::string & snippet) {
    ERRTRACE;
    render_environment env;
    format_templates templates;
    if (!load_templates(templates))
        standard.exit(exit_file_error); // Don't continue if you don't have the templates.

    template_varvalues formatvars;        
    switch (fgs.format) {

        case format_txt: {
            formatvars.emplace("section",sectiontag[fgs.section]);
            formatvars.emplace("subsection",subsectiontag[fgs.subsection]);
            formatvars.emplace("index",to_precision_string(fgs.decimalidx,1,'0',4));
            formatvars.emplace("snippet",snippet);
            break;
        }

        case format_html: {
            formatvars.emplace("snippet",snippet);
            break;
        }

        case format_fullhtml: {
            formatvars.emplace("section",sectiontag[fgs.section]);
            formatvars.emplace("subsection",subsectiontag[fgs.subsection]);
            formatvars.emplace("index",to_precision_string(fgs.decimalidx,1,'0',4));
            formatvars.emplace("snippet",snippet);
            break;
        }

        default:
            break;

    }
    return env.render(templates[(template_id_enum) fgs.format],formatvars);
}

int read_snippet() {
    ERRTRACE;
    Guide_snippet_system snippet(fgs);
    VERBOSEOUT("Reading snippet from "+snippet.tablename+" with ID="+snippet.idstr()+"\n\n");

    if (!read_Guide_snippet_pq(snippet,fgs.pa)) {
        ADDERROR(__func__,"unable to read snippet");
        standard.exit(exit_database_error);
    }

    std::string rendered_snippet;
    if (fgs.format == format_none) {
        rendered_snippet = snippet.snippet;
    } else {
        rendered_snippet = format_snippet(snippet.snippet);
    }

    if (fgs.dest.empty()) { // to STDOUT
        VERBOSEOUT("Snippet content:\n\n");
        FZOUT(rendered_snippet);
    } else {
        VERBOSEOUT("Writing snippet content to "+fgs.dest+".\n\n");
        if (!string_to_file(fgs.dest,rendered_snippet)) {
            ADDERROR(__func__,"unable to write snippet to "+fgs.dest);
            standard.exit(exit_file_error);
        }
    }

    return standard.completed_ok();
}

int main(int argc, char *argv[]) {
    ERRTRACE;
    fgs.init_top(argc, argv);

    switch (fgs.flowcontrol) {

    case flow_store: {
        return store_snippet();
    }

    case flow_read: {
        return read_snippet();
    }

    default: {
        fgs.print_usage();
    }

    }

    return standard.completed_ok();
}
