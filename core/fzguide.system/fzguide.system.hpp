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
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZGUIDE_SYSTEM_HPP.
 */

#ifndef __FZGUIDE_SYSTEM_HPP
#include "version.hpp"
#define __FZGUIDE_SYSTEM_HPP (__VERSION_HPP)

// core
#include "standard.hpp"
#include "templater.hpp"
#include "Guidepostgres.hpp"

using namespace fz;

enum format_options {
    format_txt = 0,
    format_html = 1,
    format_fullhtml = 2,
    format_none = 3,
    format_NUMoptions  
};

enum flow_options {
    flow_unknown = 0, /// no recognized request
    flow_store = 1,   /// store new snippet
    flow_read = 2,    /// read snippet
    flow_listIDs = 3, /// list all snippet IDs
    flow_NUMoptions
};

enum fgs_chapter {
    fgs_any = 0,
    fgs_am = 1,
    fgs_act = 2,
    fgs_pm = 3,
    fgs_NUMsections
};

enum fgs_subsection {
    fgs_unspecified = 0,
    fgs_SEC = 1,
    fgs_NOTE = 2,
    fgs_wakeup = 3,
    fgs_catchup = 4,
    fgs_calendar = 5,
    fgs_AL_update = 6,
    fgs_Track_LVL3 = 7,
    fgs_Challenges = 8,
    fgs_Check_1 = 9,
    fgs_Time_Off = 10,
    fgs_Score = 11,
    fgs_Insight_Graphs = 12,
    fgs_Outcomes = 13,
    fgs_Communications = 14,
    fgs_Positive = 15,
    fgs_LVL3 = 16,
    fgs_Decisions_and_Challenges = 17,
    fgs_NUMsubsections
};

enum template_id_enum {
    format_txt_temp = format_txt,
    format_html_temp = format_html,
    format_fullhtml_temp = format_fullhtml,
    NUM_temp
};

typedef std::map<template_id_enum,std::string> format_templates;

struct fzguide_system: public formalizer_standard_program {

    fgs_chapter chapter;
    float sectionnum;
    float subsectionnum;
    fgs_subsection subsection;
    float decimalidx;
    std::string source; ///< where to get snippet content (empty means STDIN)
    std::string dest;   ///< where to put snippet content (empty means STDOUT)
    format_options format;

    flow_options flowcontrol;

    Postgres_access pa;

    render_environment env;
    format_templates templates;
    bool templatesloaded;

    fzguide_system();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    bool nullsnippet() const { return (chapter == fgs_any && sectionnum <= 0.0 && subsectionnum <= 0.0 && subsection == fgs_unspecified && decimalidx <= 0.0); }
    bool multisnippet() const { return (chapter == fgs_any || sectionnum <= 0.0 || subsectionnum <= 0.0 || subsection == fgs_unspecified || decimalidx <= 0.0); }

};

extern fzguide_system fgs;

struct Guide_snippet_system: public Guide_snippet {

    std::string chapter;
    std::string sectionnum;
    std::string subsectionnum;
    std::string subsection;
    std::string idxstr;

    Guide_snippet_system(): Guide_snippet("guide_system") {}
    Guide_snippet_system(const std::string & idstr, const std::string & snippetstr);
    Guide_snippet_system(const fzguide_system & _fzgs): Guide_snippet("guide_system") { set_id(_fzgs); }

    void set_id(const fzguide_system & _fzgs);

    virtual std::string layout() const;

    virtual std::string idstr() const;

    virtual std::string all_values_pqstr() const;

    virtual Guide_snippet * clone() const;

    virtual bool nullsnippet() const { return (chapter.empty() && sectionnum.empty() && subsectionnum.empty() && subsection.empty() && idxstr.empty()); }

    virtual bool multisnippet() const { return (chapter=="(any)" || sectionnum=="00.0" || subsectionnum=="00.0" || subsection=="(any)" || idxstr=="00.0"); }

    void set_pq_wildcards();
};

#endif // __FZGUIDE_SYSTEM_HPP
