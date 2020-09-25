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
    flow_NUMoptions
};

enum fgs_chapter {
    fgs_am = 0,
    fgs_pm = 1,
    fgs_NUMsections
};

enum fgs_subsection {
    fgs_SEC = 0,
    fgs_wakeup = 1,
    fgs_catchup = 2,
    // *** this needs a lot more
    fgs_NUMsubsections
};

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

    fzguide_system();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

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

    virtual bool nullsnippet() const { return (chapter.empty() || sectionnum.empty() || subsectionnum.empty() || subsection.empty() || idxstr.empty() || snippet.empty()); }

};

#endif // __FZGUIDE_SYSTEM_HPP
