// Copyright 2020 Randal A. Koene
// License TBD

/**
 * General collection of functions for working with HTML strings.
 */

// std
#include <map>
#include <vector>
#include <cctype>

// core
#include "html.hpp"
#include "general.hpp"

namespace fz {

const std::map<std::string, text_interpretation> text_interpretation_flags_map = {
    {"raw", text_interpretation::raw},
    {"detect_links", text_interpretation::detect_links},
    {"emptyline_is_par", text_interpretation::emptyline_is_par},
    {"full_markdown", text_interpretation::full_markdown}
};

/**
 * A useful function to convert a set of comma separated flag identifiers into
 * a `text_interpretation` flags bitmap.
 * 
 * @param csflags String with comma separated flag identifiers.
 * @return A bitmap with text_interpretation flags.
 */
text_interpretation config_parse_text_interpretation(std::string csflags) {
    unsigned long bitflags = text_interpretation::raw;
    auto flagtokens = split(csflags,',');
    for (auto & token : flagtokens) {
        if (!token.empty()) {
            trim(token);
            auto it = text_interpretation_flags_map.find(token);
            if (it != text_interpretation_flags_map.end()) {
                bitflags |= it->second;
            }
        }
    }
    return (text_interpretation) bitflags;
}

bool lowercase_equal(const std::string & lowermatch, const std::string & content, size_t start_at) {
    size_t content_size = content.size() - start_at;
    if (content_size < lowermatch.size()) return false;

    for (size_t i = 0; i < lowermatch.size(); i++) {
        if (tolower(content[i+start_at]) != lowermatch[i]) return false;
    }
    return true;
}

bool lowercase_match_skipping_spaces(const std::vector<std::string> & match_vec, const std::string & content, size_t start_at, size_t * url_start_ptr) {
    for (size_t i = 0; i < match_vec.size(); i++) {
        if (!lowercase_equal(match_vec.at(i), content, start_at)) return false;
        // Note: This also skips spaces and tabs after the last match-part. We
        //       could add an option not to do this where start_at would be set
        //       to start_at + match_vec.at(i).size() instead if it's the last
        //       part.
        start_at = content.find_first_not_of(" \t", start_at + match_vec.at(i).size());
        if (start_at == std::string::npos) return false;
    }
    if (url_start_ptr) {
        (*url_start_ptr) = start_at; 
    }
    return true;
}

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
 * 
 * At present, this identifies and converts to links URLs, Log chunk IDs,
 * Log entry IDs and Node IDs.
 */
std::string convert_special_data_word_html(const std::string & wstr, size_t from, size_t bef) {

    // 1. recognise HTTP(S)
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

    // 2. recognize Log chunk ID
    if ((bef-from)==12) {
        bool islogid = true;
        for (int i = 0; i<12; i++) {
            if ((wstr[from+i]<'0') || (wstr[from+i]>'9')) {
                islogid = false;
                break;
            }
        }
        if (islogid) {
            std::string urlstr("<a href=\"/cgi-bin/fzlink.py?id="+wstr.substr(from, bef-from)+"\">"+wstr.substr(from, bef-from)+"</a>");
            return urlstr;
        }
    }

    // 3. recognize Log entry ID or Node ID
    if ((bef-from)>=14) {

        // 3.1 recognize Log entry ID
        if (wstr[from+12]=='.') {
            bool islogid = true;
            for (size_t i = 0; i<12; i++) {
                if ((wstr[from+i]<'0') || (wstr[from+i]>'9')) {
                    islogid = false;
                    break;
                }
            }
            if (islogid) {
                for (size_t i = from+13; i<bef; i++) {
                    if ((wstr[i]<'0') || (wstr[i]>'9')) {
                        islogid = false;
                        break;
                    }
                }
                if (islogid) {
                    std::string urlstr("<a href=\"/cgi-bin/fzlink.py?id="+wstr.substr(from, bef-from)+"\">"+wstr.substr(from, bef-from)+"</a>");
                    return urlstr;
                }
            }
        }

        // 3.2 recognize Node ID
        if ((bef-from)>=16) {
            if (wstr[from+14]=='.') {
                bool isnodeid = true;
                for (size_t i = 0; i<14; i++) {
                    if ((wstr[from+i]<'0') || (wstr[from+i]>'9')) {
                        isnodeid = false;
                        break;
                    }
                }
                if (isnodeid) {
                    for (size_t i = from+15; i<bef; i++) {
                        if ((wstr[i]<'0') || (wstr[i]>'9')) {
                            isnodeid = false;
                            break;
                        }
                    }
                    if (isnodeid) {
                        std::string urlstr("<a href=\"/cgi-bin/fzlink.py?id="+wstr.substr(from, bef-from)+"\">"+wstr.substr(from, bef-from)+"</a>");
                        return urlstr;
                    }
                }
            }
        }
    }

    // ***recognize other things?

    return wstr.substr(from, bef-from);
}

/**
 * This tokenizes, i.e. finds words separated by white space, in order to
 * convert special data, which we assume is identifiable within a token.
 * This function is applied only to parts of a string that were NOT part
 * of an HTML tag, as found in make_embeddable_html().
 * 
 * Note: According to RFC 3986, a valid URI may contain any of the following
 *       characters:
 * 
 *       ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~:/?#[]@!$&'()*+,;=
 * 
 *       Because of this, auto-detection of a URL can fail if the URL is
 *       followed immediately by a sentence divider such as ".,;:!?". If
 *       you want auto-detection to work, add a space.
 * 
 *       If you wish to detect simplified URLs between square brackets then
 *       you can add '[]' to the default set used in token_sep.
 * 
 *       Instead, you can use the extra-space-after-URL convention and
 *       have the extra space automatically removed by setting the
 *       'remove_extra_space' flag. This will detect if a URI is followed
 *       by ' ' and one of '.,:])' and remove the extra space.
 * 
 * ***TODO:
 *    - Possibly apply the tokenize-first approach explained in make_embeddable_html().
 *    - Ideally, only a '.' that is followed by a token_sep character, or that is the
 *      last character in the string should be treated as a '.' when processing a URI,
 *      otherwise it should be treated as part of the URI.
 * 
 * @param htmlstr The string in which to convert identified special data.
 * @param frompos Process string from this position.
 * @param beforepos Process string up to but not including this position.
 * @param remove_extra_space Remove space that was added just to identify links.
 * @param token_sep A set of characters to recognize as token separators.
 * @return The updated string with conversions applied.
 */
std::string convert_special_data_html(
    const std::string & htmlstr,
    size_t frompos,
    size_t beforepos,
    bool remove_extra_space = false,
    const std::string & token_sep = " \t\n\r\f\v") {

    if (beforepos == std::string::npos)
        beforepos = htmlstr.size();
    if (frompos >= beforepos)
        return "";

    std::string converted;
    converted.reserve((beforepos-frompos)+512);

    while (true) {
        size_t wordstart = htmlstr.find_first_not_of(token_sep, frompos);

        if ((wordstart == std::string::npos) || (wordstart >= beforepos)) { // only whitespace from here
            converted += htmlstr.substr(frompos,beforepos-frompos);
            return converted;
        }

        converted += htmlstr.substr(frompos,wordstart-frompos); // we don't convert whitespace

        size_t nextwhite = htmlstr.find_first_of(token_sep, wordstart);

        if ((nextwhite == std::string::npos) || (nextwhite >= beforepos)) { // word runs to end of chunk
            converted += convert_special_data_word_html(htmlstr, wordstart, beforepos);
            return converted;
        }

        converted += convert_special_data_word_html(htmlstr, wordstart, nextwhite);

        frompos = nextwhite;
        if (remove_extra_space) {
            if ((frompos+1) < beforepos) {
                char possible_sentence_delimiter = htmlstr.at(frompos+1);
                if ((possible_sentence_delimiter == '.')
                    || (possible_sentence_delimiter == ',')
                    || (possible_sentence_delimiter == ':')
                    || (possible_sentence_delimiter == ']')
                    || (possible_sentence_delimiter == ')')
                    ) {

                    frompos++; // Remove the extra whitespace character for aesthetic improvement.
                }
            }
        }
    }

    return converted; // it never actually gets here
}

const std::vector<std::string> href_match_vec = {
    "a ",
    "href",
    "=",
    "\""
};

/**
 * Efficiently and reliably filter snippets of HTML text such that they become
 * optimally embeddable within other HTML.
 * 
 * With the `detect_links` option, this filter also detects interesting
 * information that would be improved if presented as a link and adds such a
 * link.
 * 
 * See the comment above the convert_special_data_html() function for more
 * details and options.
 * 
 * WARNING: The 'full_markdown' text_interpretation option is not yet supported!
 * 
 * ***TODO: It might be ideal to replace this function with one that starts by
 *          fully tokenizing the htmlstr, by filling a vector with pieces, each
 *          found by using a set of delimiters "\t\n\r\f\v.:/[]!(),;", saving
 *          the delimited content and the delimiters, then walking through the
 *          vector item by item to construct a new string, while entering
 *          conversion modes as prescribed by each item or delimiter. This
 *          could be the most versatile, intelligent, and yet fast method.
 *          It may also be fine to apply this tokenization-first method just
 *          to the non-html-token content that is sent to convert_special_data_html().
 * 
 * @param htmlstr A string in HTML format.
 * @param interpretation A bit-pattern of flags used in text interpretation with
 *                       options: detect_links, emptyline_is_par, full_markdown.
 * @param special_urls Optional list of special URL components, such as @FZSERVER@
 *                     that are automatically replaced.
 * @param replacements Optional list of replacements to use for special URL components.
 * @return A string of embeddable HTML.
 */
std::string make_embeddable_html(
    const std::string & htmlstr,
    text_interpretation interpretation,
    const std::vector<std::string> * special_urls,
    const std::vector<std::string> * replacements) {

    std::string txtstr;
    txtstr.reserve(htmlstr.size()+512);
    size_t pos = 0;
    while (true) {
        // Is there still a possible HTML tag coming up?
        size_t next_ltpos = htmlstr.find('<',pos);
        if (next_ltpos == std::string::npos) {
            // Process remaining content.
            if (interpretation & text_interpretation::detect_links) {
                txtstr += convert_special_data_html(htmlstr, pos, next_ltpos, true);
            } else {
                txtstr += htmlstr.substr(pos);
            }
            return txtstr;
        }

        // Process content up to possible HTML tag.
        if (interpretation & text_interpretation::detect_links) {
            txtstr += convert_special_data_html(htmlstr, pos, next_ltpos, true);
        } else {
            txtstr += htmlstr.substr(pos,next_ltpos-pos);
        }
        // Does the possible HTML tag close?
        size_t next_gtpos = htmlstr.find('>',next_ltpos);
        if (next_gtpos == std::string::npos) { // tag doesn't close by end of string
            // *** skipping that tag to prevent broken HTML, could try to close it instead
            return txtstr;
        }
        // Identify significant HTML tags.
        std::string tagcontent(htmlstr.substr(next_ltpos, (next_gtpos-next_ltpos)+1));
        if (special_urls && replacements) {
            if (special_urls->size() == replacements->size()) {
                size_t url_start;
                if (lowercase_match_skipping_spaces(href_match_vec, tagcontent, 1, &url_start)) {
                    for (size_t i = 0; i < special_urls->size(); i++) {
                        int cmpres = tagcontent.compare(url_start, special_urls->at(i).size(), special_urls->at(i));
                        if (cmpres == 0) {
                            tagcontent = tagcontent.substr(0, url_start) + "http://" + replacements->at(i) + tagcontent.substr(url_start+special_urls->at(i).size());
                            break;
                        }
                    }
                }
            }
        }
        // *** if there are problematic tags, this would be where to check for those
        // *** if you need to skip it then do not do the next line
        // Process embeddable HTML tag.
        txtstr += tagcontent;
        //txtstr += htmlstr.substr(next_ltpos, (next_gtpos-next_ltpos)+1);

        pos = next_gtpos + 1;
        if (pos>htmlstr.size())
            break;
    }
    return txtstr;
}

void hexchar(unsigned char c, unsigned char &hex1, unsigned char &hex2) {
    hex1 = c / 16;
    hex2 = c % 16;
    hex1 += hex1 <= 9 ? '0' : 'a' - 10;
    hex2 += hex2 <= 9 ? '0' : 'a' - 10;
}

std::string uri_encode(std::string s) {
    const char *str = s.c_str();
    std::vector<char> v(s.size());
    v.clear();
    for (size_t i = 0, l = s.size(); i < l; i++) {
        char c = str[i];
        if ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            c == '-' || c == '_' || c == '.' || c == '!' || c == '~' ||
            c == '*' || c == '\'' || c == '(' || c == ')') {
            v.push_back(c);
        } else if (c == ' ') {
            v.push_back('+');
        } else {
            v.push_back('%');
            unsigned char d1, d2;
            hexchar(c, d1, d2);
            v.push_back(d1);
            v.push_back(d2);
        }
    }

    return std::string(v.cbegin(), v.cend());
}

std::string make_button(const std::string & link, const std::string & label, bool samepage) {
    std::string button_str("<button class=\"button button1\" onclick=\"window.open('"+link+'\'');
    if (samepage) {
        button_str += ",'_self'";
    }
    button_str += ");\">" + label + "</button>";
    return button_str;
}

} // namespace fz
