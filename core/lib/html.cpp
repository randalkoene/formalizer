// Copyright 2020 Randal A. Koene
// License TBD

/**
 * General collection of functions for working with HTML strings.
 */

// std
#include <map>

// core
#include "html.hpp"

namespace fz {

/**
 * This is only a partial list. See https://www.rapidtables.com/web/html/html-codes.html
 * for more information about HTML character codes, special codes, etc.
 * To convert them all you would need to take a different approach to handling "&...;" codes.
 */
const std::map<std::string, std::string> html_special_codes = {
    {"&lt;", "<"},
    {"&gt;", ">"},
    {"&nbsp;", " "},
    {"&quot;", "\""},
    {"&amp;", "&"},
    {"&apos;", "'"}
};


/**
 * Remove all HTML tags to create raw text.
 * 
 * Note: At this time, this function does not yet remove text
 * content between <script></script> tags and similar.
 * 
 * @param htmlstr A string in HTML format.
 * @return A string in raw text format.
 */
std::string remove_html(const std::string & htmlstr) {
    std::string txtstr;
    txtstr.reserve(htmlstr.size());
    size_t pos = 0;
    while (true) {
        size_t next_ltpos = htmlstr.find_first_of("<&",pos);
        if (next_ltpos == std::string::npos) {
            txtstr += htmlstr.substr(pos);
            return txtstr;
        }
        if (htmlstr[next_ltpos]=='<') {
            txtstr += htmlstr.substr(pos,next_ltpos-pos);
            size_t next_gtpos = htmlstr.find('>',next_ltpos);
            if (next_gtpos == std::string::npos) { // tag doesn't close by end of string
                return txtstr;
            }
            pos = next_gtpos + 1;
        } else {
            txtstr += htmlstr.substr(pos,next_ltpos-pos);
            size_t next_gtpos = htmlstr.find_first_of(";<",next_ltpos);
            if (next_gtpos == std::string::npos) { // it was a spurious '&'
                txtstr += '&';
                pos = next_ltpos + 1;
            } else if (htmlstr[next_gtpos]=='<') { // it was a spurious '&'
                txtstr += '&';
                pos = next_ltpos + 1;
            } else { // check for special codes I know
                std::string special_code = htmlstr.substr(next_ltpos, next_gtpos-next_ltpos+1);
                if (html_special_codes.find(special_code) == html_special_codes.end()) { // unknown special code
                    txtstr += special_code;
                    pos = next_gtpos + 1;
                } else {
                    txtstr += html_special_codes.at(special_code);
                    pos = next_gtpos + 1;
                }
            }
        }
        if (pos>htmlstr.size())
            break;
    }
    return txtstr;
}

/**
 * Remove all HTML tags (but not special characters) to produce text that
 * can be safely excerpted into HTML tables, etc, but that can still show
 * ampersands and such properly within an HTML context.
 * 
 * Note: At this time, this function does not yet remove text
 * content between <script></script> tags and similar.
 * 
 * @param htmlstr A string in HTML format.
 * @return A string in HTML-lite format.
 */
std::string remove_html_tags(const std::string & htmlstr) {
    std::string txtstr;
    txtstr.reserve(htmlstr.size());
    size_t pos = 0;
    while (true) {
        size_t next_ltpos = htmlstr.find('<',pos);
        if (next_ltpos == std::string::npos) {
            txtstr += htmlstr.substr(pos);
            return txtstr;
        }

        txtstr += htmlstr.substr(pos,next_ltpos-pos);
        size_t next_gtpos = htmlstr.find('>',next_ltpos);
        if (next_gtpos == std::string::npos) { // tag doesn't close by end of string
            return txtstr;
        }
        pos = next_gtpos + 1;

        if (pos>htmlstr.size())
            break;
    }
    return txtstr;
}

} // namespace fz
