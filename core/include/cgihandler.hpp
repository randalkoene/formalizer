// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Functions for CGI handlers.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __CGIHANDLER_HPP.
 */

#ifndef __CGIHANDLER_HPP
#include "coreversion.hpp"
#define __CGIHANDLER_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
#include <map>
#include <string>

// core
//#include "error.hpp"


namespace fz {

/// Convert URL %nn -codes into characters and '+' signs into spaces.
std::string URL_Decode(const std::string str);

typedef std::map<std::string, std::string> CGI_Token_Value_Map;
/**
 * Find GET or POST token-value pairs and make them available as maps.
 * 
 * See for example how this is used in fzlogtime.
 * 
 * Note that `has_query_string` and `has_post_length` report environment
 * variables found. The actual number of token-value pairs found is
 * reported by GET.size() and POST.size(). After merge() is called
 * POST.size() reports the merged number of unique token-value pairs.
 */
struct CGI_Token_Values {
    CGI_Token_Value_Map GET;
    CGI_Token_Value_Map POST;
    bool has_query_string = false;
    bool has_post_length = false;

    CGI_Token_Values() {}
    void merge() {
        POST.merge(GET);
        GET.clear();
    }
    void decode_GET_query_string();
    void decode_POST_data();
    size_t received_data() {
        decode_GET_query_string();
        decode_POST_data();
        merge();
        return POST.size();
    }
    bool called_as_cgi() { return has_query_string || has_post_length; }
};

} // namespace fz

#endif // __CGIHANDLER_HPP
