// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares a simple set of templating functions. They are not as
 * feature rich as something like `inja`, but will often suffice and are small and fast.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __TEMPLATER_HPP.
 */

#ifndef __TEMPLATER_HPP
#include "coreversion.hpp"
#define __TEMPLATER_HPP (__COREVERSION_HPP)

// std
#include <string>
#include <vector>
#include <map>

namespace fz {

struct template_variable_values {
    std::string varvalue;
    unsigned int n_applied;
    template_variable_values() {}
    template_variable_values(std::string v): varvalue(v), n_applied(0) {}
};

//typedef std::map<std::string,template_variable_values> template_varvalues;
struct template_varvalues: public std::map<std::string,template_variable_values> {
    template_varvalues() {}
    template_varvalues(const std::map<std::string,std::string> & init) {
        for (const auto& [k, v] : init) {
            emplace(k,v);
        }
    }
};

struct render_varpos {
    std::string::size_type pos;
    std::string::size_type len;
    std::string varlabel;
    render_varpos(std::string vlabel, std::string::size_type p, std::string::size_type l): pos(p), len(l), varlabel(vlabel) {}
};

typedef std::vector<render_varpos> template_varpos;

enum render_error_t {
    rerr_ok = 0,
    rerr_missing_value = 1,
    rerr_empty_template = 2,
    rerr_no_variable_values = 3,
    rerr_empty_variable_label = 4,
    rerr_unused_variable_value = 5
};

/**
 * Configuration for a template rendering environment.
 * 
 * @param varopen is the string that opens a variable label in the template.
 * @param varclose is the string that closes a variable label in the template.
 * @param skipmissing selects skipping missing variables with a warning rather
 *                    that failing a render attempt.
 */
struct render_environment {
    std::string varopen = "{{";
    std::string varclose = "}}";
    bool skipmissing = true;
    render_error_t render_error = rerr_ok;
    std::string render(const std::string temp, template_varvalues vars, bool proceed_if_emptyvars = true);

    /**
     * Load a template.
     * 
     * Example: See how this is used in fzloghtml.
     * 
     * @param template_path The path to the template file.
     * @param render_template Reference to a string to hold template content.
     * @return True if successfully loaded.
     */
    bool load_template(const std::string & template_path, std::string & render_template);

    /**
     * Use a prepared map of template tags and template data to fill a template.
     * 
     * Example: See how this is used in fzloghtml.
     * 
     * @param template_path The path to the template file.
     * @param tag_data_map A string:string map of tags and data.
     * @param rendered Reference to a string to hold the filled template.
     * @return True if successfully filled.
     */
    bool fill_template_from_map(const std::string & template_path, const std::map<std::string, std::string> & tag_data_map, std::string & rendered);

    /**
     * Use a prepared map of template tags and template data to fill a preloaded template.
     * 
     * Example: See how this is used in fzloghtml.
     * 
     * @param template The preloaded template.
     * @param tag_data_map A string:string map of tags and data.
     * @param rendered Reference to a string to hold the filled template.
     * @return True if successfully filled.
     */
    bool fill_preloaded_template_from_map(const std::string & preloaded_template, const std::map<std::string, std::string> & tag_data_map, std::string & rendered);

};

} // namespace fz

#endif // __TEMPLATER_HPP
