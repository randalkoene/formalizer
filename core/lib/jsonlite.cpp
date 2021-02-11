// Copyright 2020 Randal A. Koene
// License TBD

/**
 * General collection of functions for reading and writing a JSON subset format.
 */

// std
#include <vector>

// core
#include "general.hpp"
#include "jsonlite.hpp"

namespace fz {

/**
 * Identify json key labels that are actually comments.
 * 
 * E.g. see how this is used in config.cpp.
 */
bool is_json_comment(const std::string & keylabel) {
    if (keylabel.empty())
        return false;

    if ((keylabel.front() == '_') && (keylabel.back() == '_'))
        return true;

    return false;
}

/**
 * Extract parameter label and parameter value from a content line.
 * 
 * A line typically has this format:
 *   '"somelabel" : "somevalue"'
 * 
 * Note that JSON does not support comments as such. To add comments
 * simply add some data such as '"__comment1__" : "This is a comment."'.
 * 
 * And to deactivate a configuration option, just turn the variable
 * key into a comment.
 * 
 * @param par_value_pair A string in the expected format.
 * @return A pair of strings representing the parameter and the value.
 */
jsonlite_label_value_pair json_param_value(const std::string & par_value_pair) {
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
 * Convert the content of a JSON subset string into a vector of
 * '"parameter" : "value"' statement strings.
 * 
 * @param configcontentstr A string in the JSON subset format.
 * @return A vector of strings, each of which contains a parameter-value statement.
 */
jsonlite_lines json_get_param_value_lines(std::string & configcontentstr) {
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
    //rtrim(configcontentstr);
    // split at the commas
    return split(configcontentstr,',');
}

/**
 * Convert a string containing JSON subset content into a map of
 * parameter label-value pairs.
 * 
 * @param jsoncontentstr A string in JSON subset format.
 * @return A map in jsonlite_label_value_pairs format.
 */
jsonlite_label_value_pairs json_get_label_value_pairs_from_string(std::string & jsoncontentstr) {
    jsonlite_lines jsonlines = json_get_param_value_lines(jsoncontentstr);
    jsonlite_label_value_pairs jsonparpairs;
    for (const auto & line : jsonlines) {
        jsonparpairs.emplace(json_param_value(line));
    }
    return jsonparpairs;
}

/**
 * Convert JSON subset label-value pairs into a JSON content string.
 * 
 * This conversion automatically skips any empty labels.
 * 
 * @param labelvaluepairs A map in jsonlite_label_value_pairs format.
 * @return A string containing the JSON content.
 */
std::string json_label_value_pairs_to_string(const jsonlite_label_value_pairs & labelvaluepairs) {
    if (labelvaluepairs.empty())
        return "{\n}\n";
    
    std::string jsoncontentstr("{");
    jsoncontentstr.reserve(80*labelvaluepairs.size());

    for (const auto & [label, value] : labelvaluepairs) {
        if (!label.empty()) {
            jsoncontentstr += "\n\t\"" + label + "\" : \"" + value + "\",";
        }
    }

    jsoncontentstr.back() = '\n';
    jsoncontentstr += "}\n";
    return jsoncontentstr;
}

} // namespace fz
