// Copyright 2020 Randal A. Koene
// License TBD

/**
 * A simmple set of templating functions.
 * 
 * In many cases, this can be a small fast replacement for a feature-rich
 * templating library such as `inja`.
 */

#include <string>

#include "error.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "templater.hpp"

namespace fz {

/**
 * Finds the variables in the template and replaces them with the
 * values of those variables.
 * 
 * If an error is encountered then an empty string is returned and the
 * `render_environment::render_error` variable is set. Some potential
 * errors are reduced to warnings if `render_environment::skipmissing`
 * is true.
 * 
 * When the `proceed_if_emptyvars` flag is true then rendering will be
 * attempted eevn if `vars` contains no variable values. This is useful
 * when a template may contain no variable labels to substitute.
 * 
 * @param temp a template containing {{ varname }} to be replaced.
 * @param vars a map of variable names and variable values.
 * @param proceed_if_emptyvars if true attempts to render even if `vars` is empty.
 * @return the rendered string composing template and variables (or empty if error).
 */
std::string render_environment::render(const std::string temp, template_varvalues vars, bool proceed_if_emptyvars) {
    render_error = rerr_ok;

    if (temp.empty()) {
        ADDWARNING(__func__,"empty template, unable to render "+std::to_string(vars.size())+" variables");
        render_error = rerr_empty_template;
        return temp;
    }

    if ((!proceed_if_emptyvars) && (vars.empty())) {
        ADDWARNING(__func__,"no variables to render into template ("+temp.substr(0,20)+") [`proceed_if_emptyvars` can relax this requirement]");
        render_error = rerr_no_variable_values;
        return temp;
    }

    std::string rendered;
    // Find all of the variable to replace in the template
    //template_varpos replacethese;
    std::string::size_type p = 0, br_open, br_close, vstart, vlen;
    while (p<temp.size()) {

        br_open = temp.find(varopen,p);
        if (br_open == std::string::npos)
            break;

        vstart = br_open+varopen.size();
        br_close = temp.find(varclose,vstart);
        if (br_close == std::string::npos)
            break;

        vlen = br_close - vstart;
        std::string vlabel = temp.substr(vstart,vlen);
        trim(vlabel);

        rendered += temp.substr(p, br_open - p);
        p = br_open;

        if (vlabel.empty()) {
            ADDWARNING(__func__,"empty variable label at position "+std::to_string(br_open)+" in template ("+temp.substr(0,20)+')');
            rendered += temp.substr(p, br_close - p + varclose.size());
            p = br_close + varclose.size();
            render_error = rerr_empty_variable_label;

        } else {
            auto var_it = vars.find(vlabel);
            if (var_it==vars.end()) {
                if (skipmissing) {
                    ADDWARNING(__func__,"no value specified for variable label {{ "+vlabel+" }} in template ("+temp.substr(0,20)+')');
                    rendered += temp.substr(p, br_close - p + varclose.size());
                    p = br_close + varclose.size();
                    render_error = rerr_missing_value;

                } else {
                    ADDERROR(__func__,"no value specified for variable label {{ "+vlabel+" }} in template ("+temp.substr(0,20)+')');                    
                    render_error = rerr_missing_value;
                    return "";

                }
            } else {
                rendered += var_it->second.varvalue;
                var_it->second.n_applied++;
                p = br_close + varclose.size();
            }
        }
    }
    rendered += temp.substr(p);

    for (auto v : vars) {
        if (v.second.n_applied == 0) {
            ADDWARNING(__func__,"unused variable value ("+v.first+','+v.second.varvalue+") with template ("+temp.substr(0,20)+')');
            render_error = rerr_unused_variable_value;
        }
    }

    return rendered;
}

/**
 * Load a template.
 * 
 * Example: See how this is used in fzloghtml.
 * 
 * @param template_path The path to the template file.
 * @param render_template Reference to a string to hold template content.
 * @return True if successfully loaded.
 */
bool render_environment::load_template(const std::string & template_path, std::string & render_template) {
    if (!file_to_string(template_path, render_template)) {
        ERRRETURNFALSE(__func__, "unable to load " + template_path);
    }
    return true;
}

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
bool render_environment::fill_template_from_map(const std::string & template_path, const std::map<std::string, std::string> & tag_data_map, std::string & rendered) {
    // Collect variables.
    template_varvalues varvals;
    for (const auto & [ tag, data ] : tag_data_map) varvals.emplace(tag, data);

    // Fill in template.
    std::string render_template;
    if (!load_template(template_path, render_template)) {
        return false;
    }
    rendered = render(render_template, varvals);
    return true;
}

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
bool render_environment::fill_preloaded_template_from_map(const std::string & preloaded_template, const std::map<std::string, std::string> & tag_data_map, std::string & rendered) {
    // Collect variables.
    template_varvalues varvals;
    for (const auto & [ tag, data ] : tag_data_map) varvals.emplace(tag, data);

    // Fill in template.
    rendered = render(preloaded_template, varvals);
    return true;
}

} // namespace fz
