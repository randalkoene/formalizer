// Copyright 2020 Randal A. Koene
// License TBD

/** @file fzguide.system.cpp
 * Authoritative server of System guide content.
 * 
 * This Formalizer environment server program provides a target independent authoritative source
 * for content in the System guide. The server program `fzguide.system` carries out interactions
 * with the underlying database to retrieve guide content or store new guide content.
 * 
 * For more about this, see the README.md file and cards at https://trello.com/c/6Bt1nyBz and
 * https://trello.com/c/TQ9lVjuH.
 * 
 * The guide table is composed of rows with 2 columnes (fields):
 * 
 * - id key : A composite ID that identifies uniquely where the entry belongs.
 * - snippet: The text content of the entry.
 * 
 * The id key is composed as follows:
 * 
 * {chapter_label}:{section_number}:{subsection_number}:{subsection_label}:{index_number}
 * 
 * Where:
 * 
 * - chapter_label is a text label (e.g. "AM").
 * - section_number is a decimal number > 0.0 (e.g. "01.1")
 * - subsection_number is a decimal number > 0.0 (e.g. "01.1")
 * - subsection_label is a text label (e.g. "wakeup")
 * - index_number if a decimal number > 0.0 (e.g. "01.1")
 * 
 * The special subsection label "SEC" is used to indicate that the entry contains a
 * section header instead of normal entry content. In that case, subsection_number and
 * index_number are both ignored and are presented as "01.1".
 * 
 */

#define FORMALIZER_MODULE_ID "Formalizer:Server:Guide:System"

//#define TESTING_FZGUIDE

// std
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>

// core
#include "version.hpp"
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "fzpostgres.hpp"
#include "templater.hpp"
#include "utf8.hpp"

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

/// See the description of the guide entries format at the top of thie file.
const std::string pq_guide_system_layout(
    "id char(64) PRIMARY KEY, " // e.g. section:subsection:index (e.g. "morning:wakeup:02.0")
    "snippet text"              // a descriptive text
);

std::map<fgs_chapter,const std::string> chaptertag = {
    { fgs_any, "(any)" },
    { fgs_am, "am" },
    { fgs_act, "act"},
    { fgs_pm, "pm" }
};

std::map<fgs_subsection,const std::string> subsectiontag = {
    { fgs_unspecified, "(any)" },
    { fgs_SEC, "SEC" },
    { fgs_NOTE, "NOTE" },
    { fgs_wakeup, "wakeup" },
    { fgs_catchup, "catchup" },
    { fgs_calendar, "calendar" },
    { fgs_AL_update, "AL_update" },
    { fgs_Track_LVL3, "Track LVL3" },
    { fgs_Challenges, "Challenges" },
    { fgs_Check_1, "Check #1" },
    { fgs_Time_Off, "Time Off" },
    { fgs_Score, "Score" },
    { fgs_Insight_Graphs, "Insight Graphs" },
    { fgs_Outcomes, "Outcomes" },
    { fgs_Communications, "Communications" },
    { fgs_Positive, "Positive" },
    { fgs_LVL3, "LVL3" },
    { fgs_Decisions_and_Challenges, "Decisions and Challenges" }
};

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzguide_system::fzguide_system() : formalizer_standard_program(false), chapter(fgs_any), sectionnum(0.0), subsectionnum(0.0),
                                   subsection(fgs_unspecified), decimalidx(0.0), format(format_none), flowcontrol(flow_unknown),
                                   pa(*this, add_option_args, add_usage_top, true), templatesloaded(false) {
    add_option_args += "SRLATPH:u:U:x:i:o:F:";
    add_usage_top += " <-S|-R|-L> [-A|-T|-P] [-H <section_num>] [-u <subsection_num>] [-U <subsection>] [-x <idx>] [-i <inputfile>] [-o <outputfile>] [-F <format>]";

    usage_tail.emplace_back("Note 1: When Reading (-R), identifiers left unspecified are interpreted\n"
                            "as wildcards. Specifying nothing therefore means requesting everything.\n");
    usage_tail.emplace_back("Note 2: When Storing (-S) from a file (-i), that file can contain many\n"
                            "snippets if it is formatted with specific codes. Importantly, the file\n"
                            "must begin with <!-- FZGUIDE -->. Only snippets between the markers\n"
                            "<!-- begin: content --> and <!-- end  : content --> are processed, and\n"
                            "each snippet must begin with an ID code, e.g:\n"
                            "  <!-- ID={am:01.1:01.1:SEC:01.1} -->\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzguide_system::usage_hook() {
    pa.usage_hook();
    FZOUT("    -S Store new snippet in System guide\n");
    FZOUT("    -R Read snippet from System guide\n");
    FZOUT("    -L List all snippet IDs from System guide\n");
    FZOUT("    -A AM - System guide chapter\n");
    FZOUT("    -T ACT - System guide chapter\n");
    FZOUT("    -P PM - System guide chapter\n");
    FZOUT("    -H System guide decimal <section_num> > 0.0 (e.g. '01.1')\n");
    FZOUT("    -u System guide decimal <subsection_num> > 0.0 (e.g. '01.1')\n");
    FZOUT("    -U System guide <subsection> label\n");
    FZOUT("    -x System guide decimal index number <idx> > 0.0\n");
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

    case 'L': {
        flowcontrol = flow_listIDs;
        return true;
    }

    case 'A': {
        chapter = fgs_am;
        return true;
    }

    case 'T': {
        chapter = fgs_act;
        return true;
    }

    case 'P': {
        chapter = fgs_pm;
        return true;
    }

    case 'H': {
        float idx_float = atof(cargs.c_str());
        if (idx_float<=0.0)
            return false;
        sectionnum = idx_float;
        return true;
    }

    case 'u': {
        float idx_float = atof(cargs.c_str());
        if (idx_float<=0.0)
            return false;
        subsectionnum = idx_float;
        return true;
    }

    case 'U': { // find the matching subsection, if it is defined
        for (const auto & [subseckey, subsecstr] : subsectiontag) {
            if (subsecstr == cargs) {
                subsection = subseckey;
                return true;
            }
        }

        std::string warnstr = "Unrecognized subsection title ("+cargs+"). The authoritative list is at the top of the fzguide.system source files.";
        ADDWARNING(__func__,warnstr);
        VERBOSEOUT(warnstr+"\n\n");
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

Guide_snippet_system::Guide_snippet_system(const std::string & idstr, const std::string & snippetstr): Guide_snippet("guide_system") {
    std::vector<std::string> id_components = split(idstr,':');
    if (id_components.size()!=5) {
        ADDERROR(__func__, "Invalid ID: "+idstr);
        standard.exit(exit_general_error);
    }

    chapter = id_components[0];
    sectionnum = id_components[1];
    subsectionnum = id_components[2];
    subsection = id_components[3];
    idxstr = id_components[4];

    snippet = snippetstr;
}

void Guide_snippet_system::set_id(const fzguide_system & _fzgs) {
    if (!_fzgs.nullsnippet()) {
        chapter = chaptertag[_fzgs.chapter];
        sectionnum = to_precision_string(_fzgs.sectionnum,1,'0',4);
        subsectionnum = to_precision_string(_fzgs.subsectionnum,1,'0',4);
        subsection = subsectiontag[_fzgs.subsection];
        idxstr = to_precision_string(_fzgs.decimalidx,1,'0',4);
    }
}

std::string Guide_snippet_system::layout() const {
    return pq_guide_system_layout;
}

std::string Guide_snippet_system::idstr() const {
    return "'"+chapter+':'+sectionnum+':'+subsectionnum+':'+subsection+':'+idxstr+"'";
}

std::string Guide_snippet_system::all_values_pqstr() const {
    return idstr()+", $txt$"+snippet+"$txt$";
}

Guide_snippet * Guide_snippet_system::clone() const {
    return new Guide_snippet_system(*this);
}

// Translate to Postgres wildcards.
void Guide_snippet_system::set_pq_wildcards() {
    if (nullsnippet())
        return; // nothing to do, this is recognized as "everything"

    if (multisnippet()) {
        if (chapter == "(any)")
            chapter = "%";
        if (fgs.sectionnum<=0.0)
            sectionnum = "%";
        if (fgs.subsectionnum<=0.0)
            subsectionnum = "%";
        if (subsection == "(any)")
            subsection = "%";
        if (fgs.decimalidx<=0.0)
            idxstr = "%";
    }
}

/*
The whole guide: SELECT snippet FROM randalk.guide_system
The whole AM chapter: SELECT snippet FROM randalk.guide_system WHERE id LIKE 'am%'
The whole AM chapter section 2: SELECT snippet FROM randalk.guide_system WHERE id LIKE 'am:02%'
The whole AM chapter, AL_update subsection: SELECT snippet FROM randalk.guide_system WHERE id LIKE 'am:02%:AL_update:%'
All AL_update subsections in all chapters: SELECT snippet FROM randalk.guide_system WHERE id LIKE '%:AL_update:%'
*/

int store_multi_snippet(const std::string & utf8safestr) {
    // skip preamble
    size_t pos = utf8safestr.find("<!-- begin: content -->\n");
    if (pos == std::string::npos) {
        ADDERROR(__func__,"Missing begin content marker in multi-snippet data");
        standard.exit(exit_general_error);
    }

    pos += 24;
    // build vector of snippets
    std::vector<Guide_snippet_ptr> snippets;
    while (true) {
        pos = utf8safestr.find("<!-- ID={",pos);
        if (pos == std::string::npos)
            break;

        pos += 9;
        size_t id_endpos = utf8safestr.find('}',pos);
        if (id_endpos == std::string::npos)
            break;

        std::string idstr = utf8safestr.substr(pos,id_endpos-pos);
        pos = utf8safestr.find('\n',id_endpos);
        if (pos == std::string::npos)
            break;
        
        pos += 1;
        size_t snippet_endpos = utf8safestr.find("<!-- ID={",pos);
        if (snippet_endpos == std::string::npos) {
            snippet_endpos = utf8safestr.find("<!-- end  : content -->",pos);
            if (snippet_endpos == std::string::npos)
                break;
        }

        std::string snippetstr = utf8safestr.substr(pos,snippet_endpos-pos);
        pos = snippet_endpos;

        Guide_snippet_ptr guidesnippet = std::make_unique<Guide_snippet_system>(idstr,snippetstr);
        snippets.emplace_back(guidesnippet->clone());
    }

    if (snippets.empty()) {
        ADDERROR(__func__,"No valid snippets found in multi-snippet data");
        standard.exit(exit_general_error);
    }

    VERBOSEOUT("Storing multi-snippets to "+snippets[0]->tablename+"\n\n");

    if (!store_Guide_multi_snippet_pq(snippets, fgs.pa)) {
        ADDERROR(__func__,"unable to store multi-snippets");
        standard.exit(exit_database_error);
    }

#ifdef TESTING_FZGUIDE
    FZOUT("Postgres operations were simulated. Here are the Postgres commands:\n\n"+SimPQ.GetLog());
#endif

    return standard.completed_ok();
}

int store_snippet() {
    ERRTRACE;
    Guide_snippet_system snippet;
    std::string utf8_unsafe;
    if (fgs.source.empty()) { // from STDIN
        VERBOSEOUT("Collecting snippet to store to the System Guide from STDIN until EOF (CTRL+D)...\n\n");
        if (!stream_to_string(std::cin,utf8_unsafe)) {
            ADDERROR(__func__,"unable to read snippet from STDIN");
            standard.exit(exit_file_error);
        }
    } else {
        VERBOSEOUT("Collecting snippet to store to the System Guide from "+fgs.source+"...\n\n");
        if (!file_to_string(fgs.source,utf8_unsafe)) {
            ADDERROR(__func__,"unable to read snippet from "+fgs.source);
            standard.exit(exit_file_error);
        }
    }
    snippet.snippet = utf8_safe(utf8_unsafe,true);
    if (snippet.snippet.substr(0,16)=="<!-- FZGUIDE -->") {
        return store_multi_snippet(snippet.snippet);

    } else {
        snippet.set_id(fgs);
        VERBOSEOUT("Storing snippet to "+snippet.tablename+" with ID="+snippet.idstr()+"\n\n");

        if (!store_Guide_snippet_pq(snippet, fgs.pa)) {
            ADDERROR(__func__,"unable to store snippet");
            standard.exit(exit_database_error);
        }
    }

    return standard.completed_ok();
}

const std::vector<std::string> template_ids = {
    "format_txt_template.txt",
    "format_html_template.html",
    "format_fullhtml_template.html"
};

bool load_templates(format_templates & templates) {
    if (fgs.templatesloaded) // previously loaded
        return true;

    ERRTRACE;
    templates.clear();

    VERBOSEOUT("Using template directory: "+template_dir+'\n');

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i], templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    fgs.templatesloaded = true;
    return true;
}

std::string format_snippet(const std::string & snippet) {
    ERRTRACE;
    if (!load_templates(fgs.templates))
        standard.exit(exit_file_error); // Don't continue if you don't have the templates.

    template_varvalues formatvars;        
    switch (fgs.format) {

        case format_txt: {
            formatvars.emplace("chapter",chaptertag[fgs.chapter]);
            formatvars.emplace("sectionnum",to_precision_string(fgs.sectionnum,1,'0',4));
            formatvars.emplace("subsectionnum",to_precision_string(fgs.subsectionnum,1,'0',4));
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
            formatvars.emplace("chapter",chaptertag[fgs.chapter]);
            formatvars.emplace("sectionnum",to_precision_string(fgs.sectionnum,1,'0',4));
            formatvars.emplace("subsectionnum",to_precision_string(fgs.subsectionnum,1,'0',4));
            formatvars.emplace("subsection",subsectiontag[fgs.subsection]);
            formatvars.emplace("index",to_precision_string(fgs.decimalidx,1,'0',4));
            formatvars.emplace("snippet",snippet);
            break;
        }

        default:
            break;

    }
    return fgs.env.render(fgs.templates[(template_id_enum) fgs.format],formatvars);
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

int list_IDs() {
    ERRTRACE;
    Guide_snippet_system snippet(fgs);
    VERBOSEOUT("Listing snippet IDs from "+snippet.tablename+"\n\n");

    std::vector<std::string> ids;
    if (!read_Guide_IDs_pq(snippet, fgs.pa, ids)) {
        ADDERROR(__func__,"unable to read IDs");
        standard.exit(exit_database_error);
    }

    std::string rendered_IDs;
    if ((fgs.format == format_none) || (fgs.format == format_txt)) {
        rendered_IDs = join(ids,"\n")+'\n';
    } else { // either of the html formats
        rendered_IDs = join(ids,"<br />\n")+"<br />\n";
    }

    if (fgs.dest.empty()) { // to STDOUT
        VERBOSEOUT("List of IDs:\n\n");
        FZOUT(rendered_IDs);
    } else {
        VERBOSEOUT("Writing List of IDs to "+fgs.dest+".\n\n");
        if (!string_to_file(fgs.dest,rendered_IDs)) {
            ADDERROR(__func__,"unable to write List of IDs to "+fgs.dest);
            standard.exit(exit_file_error);
        }
    }

    return standard.completed_ok();
}

std::string multi_format_snippet(const std::string & id, const std::string & snippet) {
    ERRTRACE;
    if (!load_templates(fgs.templates))
        standard.exit(exit_file_error); // Don't continue if you don't have the templates.

    std::vector<std::string> id_components = split(id,':');

    template_varvalues formatvars;        
    switch (fgs.format) {

        case format_txt: {
            formatvars.emplace("chapter",id_components[0]);
            formatvars.emplace("sectionnum",id_components[1]);
            formatvars.emplace("subsectionnum",id_components[2]);
            formatvars.emplace("subsection",id_components[3]);
            formatvars.emplace("index",id_components[4]);
            formatvars.emplace("snippet",snippet);
            break;
        }

        case format_html: {
            formatvars.emplace("snippet",snippet);
            break;
        }

        case format_fullhtml: {
            formatvars.emplace("chapter",id_components[0]);
            formatvars.emplace("sectionnum",id_components[1]);
            formatvars.emplace("subsectionnum",id_components[2]);
            formatvars.emplace("subsection",id_components[3]);
            formatvars.emplace("index",id_components[4]);
            formatvars.emplace("snippet",snippet);
            if (id_components[3]=="SEC") {
                formatvars.emplace("checkbox_space","");
            } else {
                formatvars.emplace("checkbox_space","<input type=\"checkbox\" name=\""+id+"\">");
            }
            break;
        }

        default:
            break;

    }
    return fgs.env.render(fgs.templates[(template_id_enum) fgs.format],formatvars);
}

int read_multi_snippet() {
    ERRTRACE;
    Guide_snippet_system snippet(fgs);
    VERBOSEOUT("Reading multiple snippets from "+snippet.tablename+" with ID="+snippet.idstr()+"\n\n");

    // Translate System Guide specific ID components with wildcards to a version that
    // idstr() can properly deliver to a Postgres command.
    snippet.set_pq_wildcards();

    std::vector<std::string> ids;
    std::vector<std::string> snippets;
    if (!read_Guide_multi_snippets_pq(snippet, fgs.pa, ids, snippets)) {
        ADDERROR(__func__,"unable to read snippets");
        standard.exit(exit_database_error);
    }

    std::string rendered_snippets;
    if ((fgs.format == format_none) || (fgs.format == format_txt)) {
        rendered_snippets = join(snippets,"\n\n")+"\n\n";
    } else { // either of the html formats
        for (size_t i = 0; i < snippets.size(); ++i) {
            rendered_snippets += multi_format_snippet(ids[i],snippets[i]);
        }
        //rendered_snippets = join(snippets,"<br />\n")+"<br />\n";
    }

    if (fgs.dest.empty()) { // to STDOUT
        VERBOSEOUT("Multi-snippet content:\n\n");
        FZOUT(rendered_snippets);
    } else {
        VERBOSEOUT("Writing multi-snippet content to "+fgs.dest+".\n\n");
        if (!string_to_file(fgs.dest,rendered_snippets)) {
            ADDERROR(__func__,"unable to write multi-snippets to "+fgs.dest);
            standard.exit(exit_file_error);
        }
    }

    return standard.completed_ok();
}

int main(int argc, char *argv[]) {
    ERRTRACE;
    fgs.init_top(argc, argv);

#ifdef TESTING_FZGUIDE
    SimPQ.SimulateChanges();
#endif

    switch (fgs.flowcontrol) {

    case flow_store: {
        return store_snippet();
    }

    case flow_read: {
        if (fgs.multisnippet()) {
            return read_multi_snippet();
        } else {
            return read_snippet();
        }
    }

    case flow_listIDs: {
        return list_IDs();
    }

    default: {
        fgs.print_usage();
    }

    }

    return standard.completed_ok();
}
