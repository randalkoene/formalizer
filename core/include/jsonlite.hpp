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
#include <memory>
#include <vector>
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

enum JSON_element_type {
    json_string = 0,
    json_number = 1,
    json_flag = 2,
    json_block = 3,
    NUM_json_element_types
};

// forward declaration
struct JSON_block;

struct JSON_element {
    std::string label;
    JSON_element_type type = JSON_element_type::json_string;
    float number = 0.0;
    std::string text;
    bool flag = false;
    std::unique_ptr<JSON_block> children;
    JSON_element(std::string _label): label(_label) {}
    bool is_flag() { return type == JSON_element_type::json_flag; }
    bool is_number() { return type == JSON_element_type::json_number; }
    bool is_string() { return type == JSON_element_type::json_string; }
    bool is_block() { return type == JSON_element_type::json_block; }
    std::string json_str(size_t indent = 0);
};

struct JSON_block {
    std::vector<std::unique_ptr<JSON_element>> elements;
    std::string json_str(size_t indent = 0);
};

class JSON_data {
    size_t num_element = 0;
    size_t num_blocks = 0;
    bool get_number_value(const std::string & jsonstr, size_t & pos, JSON_element & element);
    JSON_element * get_element(const std::string & jsonstr, size_t & pos, JSON_block & parent);
    JSON_block * block_opening(const std::string & jsonstr, size_t & pos, JSON_element & parent);

public:
    JSON_element data;
    JSON_data(): data("") {}
    JSON_data(const std::string & jsonstr): data("") { parse_JSON(jsonstr, data); }
    bool parse_JSON(const std::string & jsonstr, JSON_element & data);
    size_t size() { return num_element; }
    size_t blocks() { return num_blocks; }
    std::string json_str();
};

} // namespace fz

#endif // __JSONLITE_HPP
