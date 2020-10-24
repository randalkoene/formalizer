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

/**
 * This analyzes a token to identify special data, which is converted
 * by adding a useful link.
 */
std::string convert_special_data_word_html(const std::string & wstr, size_t from, size_t bef) {
    // recognise HTTP(S)
    bool isfullurl = false;
    if ((bef-from)>=9) {
        if (((wstr[from]=='h') || (wstr[from]=='H')) && ((wstr[from+1]=='t') || (wstr[from+1]=='T')) && ((wstr[from+2]=='t') || (wstr[from+2]=='T')) & ((wstr[from+3]=='p') || (wstr[from+3]=='P'))) {
            size_t upos = from+4;
            if (((wstr[upos]=='s') || (wstr[upos]=='S'))) {
                ++upos;
            }
            if ((wstr[upos]==':') && (wstr[upos+1]=='/') && (wstr[upos+2]=='/')) {
                isfullurl = true;
            }
        }
    }
    // convert the convertible
    if (isfullurl) {
        size_t urlbef = bef;
        if ((wstr[bef - 1] <= 34) || (wstr[bef - 1] >= 123) || ((wstr[bef - 1] >= 91) && (wstr[bef - 1] <= 96)) ||
            (wstr[bef - 1] == ';') || (wstr[bef - 1] == ':') || (wstr[bef - 1] == '.') || (wstr[bef - 1] == ',') || (wstr[bef - 1] == '\'')) {
            --urlbef;
        }
        std::string urlstr("<a href=\""+wstr.substr(from, urlbef-from)+"\">"+wstr.substr(from, urlbef-from)+"</a>");
        if (urlbef<bef)
            urlstr += wstr[bef-1];
        return urlstr;
    }

    // ***recognize other things?

    return wstr.substr(from, bef-from);
}

/**
 * This tokenizes, i.e. finds words separated by white space, in order to
 * convert special data, which we assume is identifiable within a token.
 */
std::string convert_special_data_html(const std::string & htmlstr, size_t frompos, size_t beforepos) {
    if (beforepos == std::string::npos)
        beforepos = htmlstr.size();
    if (frompos >= beforepos)
        return "";

    std::string converted;
    converted.reserve((beforepos-frompos)+512);

    while (true) {
        size_t wordstart = htmlstr.find_first_not_of(" \t\n\r\f\v", frompos);
        if ((wordstart == std::string::npos) || (wordstart >= beforepos)) { // only whitespace from here
            converted += htmlstr.substr(frompos,beforepos-frompos);
            return converted;
        }
        converted += htmlstr.substr(frompos,wordstart-frompos); // we don't convert whitespace
        size_t nextwhite = htmlstr.find_first_of(" \t\n\r\f\v", wordstart);
        if ((nextwhite == std::string::npos) || (nextwhite >= beforepos)) { // word runs to end of chunk
            converted += convert_special_data_word_html(htmlstr, wordstart, beforepos);
            return converted;
        }
        converted += convert_special_data_word_html(htmlstr, wordstart, nextwhite);
        frompos = nextwhite;
    }

    return converted; // it never actually gets here
}

/**
 * Efficiently and reliably filter snippets of HTML text such that they become
 * optimally embeddable within other HTML.
 * 
 * With the `detect_links` option, this filter also detects interesting
 * information that would be improved if presented as a link and adds such a
 * link.
 * 
 * @param htmlstr A string in HTML format.
 * @param detect_links If true then convert recognized data into links.
 * @return A string of embeddable HTML.
 */
std::string make_embeddable_html(const std::string & htmlstr, bool detect_links) {
    std::string txtstr;
    txtstr.reserve(htmlstr.size()+512);
    size_t pos = 0;
    while (true) {
        size_t next_ltpos = htmlstr.find('<',pos);
        if (next_ltpos == std::string::npos) {
            if (!detect_links) {
                txtstr += htmlstr.substr(pos);
            } else {
                txtstr += convert_special_data_html(htmlstr, pos, next_ltpos);
            }
            return txtstr;
        }

        if (!detect_links) {
            txtstr += htmlstr.substr(pos,next_ltpos-pos);
        } else {
            txtstr += convert_special_data_html(htmlstr, pos, next_ltpos);
        }
        size_t next_gtpos = htmlstr.find('>',next_ltpos);
        if (next_gtpos == std::string::npos) { // tag doesn't close by end of string
            // *** skipping that tag to prevent broken HTML, could try to close it instead
            return txtstr;
        }
        // *** if there are problematic tags, this would be where to check for those
        // *** if you need to skip it then do not do the next line
        txtstr += htmlstr.substr(next_ltpos, (next_gtpos-next_ltpos)+1);
        pos = next_gtpos + 1;

        if (pos>htmlstr.size())
            break;
    }
    return txtstr;
}

} // namespace fz
