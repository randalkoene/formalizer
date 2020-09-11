// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ header_description }}
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

enum fgs_section {
    fgs_am = 0,
    fgs_pm = 1,
    fgs_NUMsections
};

enum fgs_subsection {
    fgs_wakeup = 0,
    // *** this needs a lot more
    fgs_NUMsubsections
};

struct fzguide_system: public formalizer_standard_program {

    fgs_section section;
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

    std::string section;
    std::string subsection;
    std::string idxstr;

    Guide_snippet_system(): Guide_snippet("guide_system") {}
    Guide_snippet_system(const fzguide_system & _fzgs): Guide_snippet("guide_system") { set_id(_fzgs); }

    void set_id(const fzguide_system & _fzgs);

    virtual std::string layout() const;

    virtual std::string idstr() const;

    virtual std::string all_values_pqstr() const;

    virtual bool nullsnippet() const { return (section.empty() || subsection.empty() || idxstr.empty() || snippet.empty()); }

};

#endif // __FZGUIDE_SYSTEM_HPP
