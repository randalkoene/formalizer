// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <filesystem>

// core
#include "error.hpp"
#include "general.hpp"
#include "config.hpp"

/**
 * CONFIG_ROOT must be supplied by -D during make.
 * For example: CONFIGROOT=-DCONFIG_ROOT=\"$(HOME)/.formalizer/config\"
 */
#ifndef CONFIG_ROOT
    #define CONFIT_ROOT this_breaks
#endif

namespace fz {

configurable::configurable(std::string thisprogram, formalizer_standard_program & fsp): configbase(thisprogram), main_init_register(fsp) {}

bool configurable::init() {
    if (!load()) {
        const std::string configerrstr("Errors during configuration file processing");
        VERBOSEERR(configerrstr+'\n');
        ERRRETURNFALSE(__func__,configerrstr);
    }

    return true;
}

configbase::configbase(std::string thisprogram): processed(false) {
    if (!thisprogram.empty())
        configfile = std::string(CONFIG_ROOT)+'/'+thisprogram+"/config.json";
}

/**
 * Load the configuration file indicated by `configfile`, parse the
 * contents and set parameters through calls to `set_parameter()`.
 * 
 * Note: It is not an error if a configuration file does not exist. That
 * just means that no parameters have been specified and defaults are used.
 * Not finding a configuration file does generate a warning.
 * 
 * @return True if the configuration file was successfully loaded and parsed.
 */
bool configbase::load() {
    ERRTRACE;
    if (processed)
        return true; // not an error, calling multiple times is fine

    processed = true;
    if (configfile.empty())
        ERRRETURNFALSE(__func__,"Empty string in config.configfile");

    std::error_code ec;
    if (!std::filesystem::exists(configfile,ec)) {
        ADDWARNING(__func__,"No configuration file found at "+configfile+", default parameter values applied");
        return true; // this is not an error
    }

    std::string configcontentstr;
    if (!file_to_string(configfile,configcontentstr))
        ERRRETURNFALSE(__func__,"Unable to load configuration file "+configfile);

    if (!parse(configcontentstr))
        ERRRETURNFALSE(__func__,"Unable to parse some or all contents of configuration file "+configfile)

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
 * @return True if the configuration contents were successfully parsed.
 */
bool configbase::parse(std::string & configcontentstr) {
    ERRTRACE;
    // trim away the opening bracket
    ltrim(configcontentstr);
    if (configcontentstr.front()=='{') { // not making a fuss if it's not there
        configcontentstr.erase(0,1);
    }
    // trim away the closing bracket
    rtrim(configcontentstr);
    if (configcontentstr.back()=='}') { // no big deal if it's missing
        configcontentstr.pop_back();
    }
    // split at the commas
    auto configlines = split(configcontentstr,',');

    ERRHERE(".params");
    bool noerrors = true;
    for (const auto& it : configlines) {
        auto [parlabel, parvalue] = param_value(it);
        if (!parlabel.empty()) {
            if (!set_parameter(parlabel, parvalue)) {
                ADDERROR(__func__,"Unable to parse configuration parameter: "+parlabel);
                noerrors = false;
            }
        }
    }

    return noerrors;
}

} // namespace fz
