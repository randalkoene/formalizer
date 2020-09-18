// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <filesystem>

// core
#include "error.hpp"
#include "general.hpp"
#include "config.hpp"

/// CONFIG_ROOT must be supplied by -D during make.
#ifndef CONFIG_ROOT
    #define CONFIT_ROOT this_breaks
#endif

namespace fz {

/**
 * Load the configuration file indicated by `config`, parse the
 * contents and set parameters through calls to `config::set_parameter()`.
 * 
 * Note: It is not an error if a configuration file does not exist. That
 * just means that no parameters have been specified and defaults are used.
 * Not finding a configuration file does generate a warning.
 * 
 * @param config A `configurable` with valid `configfile` specification.
 * @return True if the configuration file was successfully loaded and parsed.
 */
bool configure::load(configurable & config) {
    ERRTRACE;
    if (config.configfile.empty())
        ERRRETURNFALSE(__func__,"Empty string in config.configfile");

    std::string configpath(std::string(CONFIG_ROOT)+'/'+config.configfile);

    std::error_code ec;
    if (!std::filesystem::exists(configpath,ec)) {
        ADDWARNING(__func__,"No configuration file found at "+configpath+", default parameter values applied");
        return true; // this is not an error
    }

    std::string configcontentstr;
    if (!file_to_string(configpath,configcontentstr))
        ERRRETURNFALSE(__func__,"Unable to load configuration file "+configpath);

    if (!parse(configcontentstr, config))
        ERRRETURNFALSE(__func__,"Unable to parse the contents of configuration file "+configpath)

    return true;
}

/// Extract parameter label and parameter value.
std::pair<std::string, std::string> param_value(const std::string & par_value_pair) {
    std::string::size_type pos;
    if ((pos = par_value_pair.find('"')) == std::string::npos) {
        return std::make_pair("","");
    }
    auto start = pos+1;
    if ((pos = par_value_pair.find('"',start)) == std::string::npos) {
        return std::make_pair("","");
    }
    std::string parlabel(par_value_pair.substr(start,pos-start));
    if ((pos = par_value_pair.find(':',pos+1)) == std::string::npos) {
        return std::make_pair("","");
    }
    if ((pos = par_value_pair.find('"',pos+1)) == std::string::npos) {
        return std::make_pair("","");
    }
    start = pos+1;
    if ((pos = par_value_pair.find('"',start)) == std::string::npos) {
        return std::make_pair("","");
    }
    std::string parvalue(par_value_pair.substr(start,pos-start));
    return std::make_pair(parlabel,parvalue);
}

/**
 * Parse the contents of a configuration file.
 * 
 * Enclosing brackets are optional. Parameter-value assignments are assumed to
 * be separated by a colon and each surrounded by double quotes. For more
 * about this JSON subset format, see the README.md file.
 * 
 * @param configcontentstr A string containing the contents of the configuration file.
 * @param config A `configurable` with `set_parameter()` method.
 */
bool configure::parse(std::string & configcontentstr, configurable & config) {
    ERRTRACE;
    // trim away the opening bracket
    ltrim(configcontentstr);
    if (configcontentstr.front()=='{') { // not making a fuss if it's not there
        configcontentstr.erase(0);
    }
    // trim away the closing bracket
    rtrim(configcontentstr);
    if (configcontentstr.back()=='}') { // no big deal if it's missing
        configcontentstr.pop_back();
    }
    // split at the commas
    auto configlines = split(configcontentstr,',');

    ERRHERE(".params");
    for (const auto& it : configlines) {
        auto [parlabel, parvalue] = param_value(it);
        if (!parlabel.empty()) {
            config.set_parameter(parlabel, parvalue);
        }
    }
}

} // namespace fz
