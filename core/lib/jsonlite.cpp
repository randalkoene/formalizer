// Copyright 2020 Randal A. Koene
// License TBD

/**
 * General collection of functions for reading and writing a JSON subset format.
 */

// std
#include <memory>
#include <vector>
#include <cstdlib>

// core
#include "error.hpp"
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

std::string JSON_element::json_str(size_t indent) {
    std::string str;
    if (indent > 0) {
        str.resize(indent, '\t');
    }
    str += '"'+label+"\" : ";
    switch (type) {
        case json_string: {
            str += '"' + text + "\",\n";
            break;
        }
        case json_number: {
            str += to_precision_string(number, 3) + ",\n";
            break;
        }
        case json_flag: {
            if (flag) {
                str += "true,\n";
            } else {
                str += "false,\n";
            }
            break;
        }
        case json_block: {
            if (children) {
                str += "{\n";
                str += children->json_str(indent);
                std::string indent_str(indent, '\t');
                str += indent_str + "},\n";
            }
            break;
        }
        default: {
            // nothing to do here
        }
    }
    return str;
}

std::string JSON_block::json_str(size_t indent) {
    std::string str;
    ++indent;
    for (const auto & element : elements) {
        if (element) {
            str += element->json_str(indent);
        }
    }
    return str;
}

bool JSON_data::get_number_value(const std::string & jsonstr, size_t & pos, JSON_element & element) {
    size_t pos_after_number = jsonstr.find_first_of(", }\t\n", pos);
    if (pos_after_number == std::string::npos) {
        ERRRETURNFALSE(__func__, "Number syntax error at or near character "+std::to_string(pos)+'.');
    }
    element.number = std::atof(jsonstr.substr(pos, pos_after_number - pos).c_str());
    element.type = json_number;
    ++num_element;
    pos = pos_after_number;
    return true;
}

JSON_element * JSON_data::get_element(const std::string & jsonstr, size_t & pos, JSON_block & parent) {
    size_t next_json_ctrl = jsonstr.find('"', pos);
    if (next_json_ctrl == std::string::npos) {
        ERRRETURNNULL(__func__, "JSON element label syntax error, missing end-quote.");
    }
    std::string label(jsonstr.substr(pos, next_json_ctrl - pos));
    pos = next_json_ctrl + 1;

    std::unique_ptr<JSON_element> element = std::make_unique<JSON_element>(label);
    JSON_element * element_ptr = element.get();
    parent.elements.push_back(std::move(element));

    // *** not explicitly looking for separating colon (yet)
    next_json_ctrl = jsonstr.find_first_not_of(": \t\n", pos);
    if (next_json_ctrl == std::string::npos) {
        ERRRETURNNULL(__func__, "Broken JSON syntax and missing element value at character "+std::to_string(pos)+'.');
    }
    switch (jsonstr[next_json_ctrl]) {
        case '{' : {
            pos = next_json_ctrl + 1;
            if (!block_opening(jsonstr, pos, *element_ptr)) {
                ERRRETURNNULL(__func__, "Broken JSON nested blocks syntax forces early stop during parsing at character number "+std::to_string(pos)+'.');
            }
            return element_ptr;            
        }
        case '"': {
            pos = next_json_ctrl + 1;
            next_json_ctrl = jsonstr.find('"',pos);
            if (next_json_ctrl == std::string::npos) {
                ERRRETURNNULL(__func__, "Missing end-quote of JSON string value at or near character number "+std::to_string(pos)+'.');
            }
            element_ptr->text = jsonstr.substr(pos, next_json_ctrl - pos);
            element_ptr->type = json_string;
            pos = next_json_ctrl + 1;
            ++num_element;
            return element_ptr;
        }
        case 't': {
            if (jsonstr.substr(pos, 4) == "true") {
                element_ptr->flag = true;
                element_ptr->type = json_flag;
                pos = next_json_ctrl + 4;
                ++num_element;
                return element_ptr;
            } else {
                ERRRETURNNULL(__func__, "Unknown type of JSON element value at character "+std::to_string(pos)+'.');
            }
        }
        case 'f': {
            if (jsonstr.substr(pos, 5) == "false") {
                element_ptr->flag = false;
                element_ptr->type = json_flag;
                pos = next_json_ctrl + 5;
                ++num_element;
                return element_ptr;
            } else {
                ERRRETURNNULL(__func__, "Unknown type of JSON element value at character "+std::to_string(pos)+'.');
            }
        }
        default: {
            if ((jsonstr[next_json_ctrl] >= '0') && (jsonstr[next_json_ctrl] <= '9')) {
                if (!get_number_value(jsonstr, pos, *element_ptr)) {
                    ERRRETURNNULL(__func__, "Unknown type of JSON element value at character "+std::to_string(pos)+'.');
                }
                return element_ptr;
            } else {
                ERRRETURNNULL(__func__, "Unknown type of JSON element value at character "+std::to_string(pos)+'.');
            }
        }
    }
    // never gets here
}

JSON_block * JSON_data::block_opening(const std::string & jsonstr, size_t & pos, JSON_element & parent) {
    const size_t max_elements = 10000; // this is a safety in case of hazardous JSON string
    std::unique_ptr<JSON_block> block = std::make_unique<JSON_block>();
    JSON_block * block_ptr = block.get();
    parent.children = std::move(block);
    parent.type = json_block;
    ++num_blocks;

    for (size_t element = 0; element < max_elements; ++element) {

        size_t next_json_ctrl = jsonstr.find_first_of("{\"}", pos); // *** not yet explicitly looking for commas
        if (next_json_ctrl == std::string::npos) {
            ERRRETURNNULL(__func__, "Missing control character from JSON block opening\n");
        }

        switch (jsonstr[next_json_ctrl]) {
            case '"': {
                pos = next_json_ctrl + 1;
                JSON_element * element_ptr = get_element(jsonstr, pos, *block_ptr);
                if (!element_ptr) {
                    ADDWARNING(__func__, "Unable to extract labeled element from JSON string.");
                }
                //VERYVERBOSEOUT(element_ptr->json_str()+'\n');
                break;
            }
            case '}': {
                pos = next_json_ctrl + 1;
                return block_ptr;
            }
            default: {
                char errchar = jsonstr[next_json_ctrl];
                ERRRETURNNULL(__func__, "Syntax error in JSON string at character '"+(errchar+("' number "+std::to_string(pos)))+'.');
            }
        }

    }

    ADDWARNING(__func__, "JSON parsing reached maximum elements limit.");
    return block_ptr;
}

bool JSON_data::parse_JSON(const std::string & jsonstr, JSON_element & data) {
    size_t pos = jsonstr.find('{');
    if (pos == std::string::npos) {
        return false;
    }
    ++pos;
    return block_opening(jsonstr, pos, data) != nullptr;
}

std::string JSON_data::json_str() {
    std::string str(data.json_str().substr(5));
    str.pop_back();
    str.back() = '\n';
    return str;
}

} // namespace fz
