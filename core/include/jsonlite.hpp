// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares functions for reading and writing a JSON subset format.
 * 
 * The corresponding source file is at core/lib/jsonlite.cpp.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __JSONLITE_HPP.
 */

#ifndef __JSONLITE_HPP
#include "coreversion.hpp"
#define __JSONLITE_HPP (__COREVERSION_HPP)

// std
#include <map>
#include <fstream>

namespace fz {

typedef std::string jsonlite_parlabel;
typedef std::string jsonlite_parvalue;
typedef std::vector<std::string> jsonlite_lines;
typedef std::pair<std::string, std::string> jsonlite_label_value_pair;
typedef std::map<jsonlite_parlabel, jsonlite_parvalue> jsonlite_label_value_pairs;

/// Identify json key labels that are actually comments.
bool is_json_comment(const std::string & keylabel);

/**
 * Extract parameter label and parameter value from a content line.
 * 
 * A line typically has this format:
 *   '"somelabel" : "somevalue"'
 * 
 * @param par_value_pair A string in the expected format.
 * @return A pair of strings representing the parameter and the value.
 */
jsonlite_label_value_pair json_param_value(const std::string & par_value_pair);

/**
 * Convert the content of a JSON subset string into a vector of
 * '"parameter" : "value"' statement strings.
 * 
 * @param configcontentstr A string in the JSON subset format.
 * @return A vector of strings, each of which contains a parameter-value statement.
 */
jsonlite_lines json_get_param_value_lines(std::string & configcontentstr);

/**
 * Convert a string containing JSON subset content into a map of
 * parameter label-value pairs.
 * 
 * @param jsoncontentstr A string in JSON subset format.
 * @return A map in jsonlite_label_value_pairs format.
 */
jsonlite_label_value_pairs json_get_label_value_pairs_from_string(std::string & jsoncontentstr);

/**
 * Convert JSON subset label-value pairs into a JSON content string.
 * 
 * @param labelvaluepairs A map in jsonlite_label_value_pairs format.
 * @return A string containing the JSON content.
 */
std::string json_label_value_pairs_to_string(const jsonlite_label_value_pairs & labelvaluepairs);

} // namespace fz

#endif // __JSONLITE_HPP
