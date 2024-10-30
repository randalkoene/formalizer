// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares functions for working with HTML strings.
 * 
 * The corresponding source file is at core/lib/html.cpp.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __HTML_HPP.
 */

#ifndef __HTML_HPP
#include "coreversion.hpp"
#define __HTML_HPP (__COREVERSION_HPP)

// std

namespace fz {

enum text_interpretation: unsigned long {
    raw              = 0,
    detect_links     = 0b0000'0000'0000'0001,
    emptyline_is_par = 0b0000'0000'0000'0010,
    full_markdown    = 0b0000'0000'0000'0100,
    NUM_interpretations
};

/**
 * A useful function to convert a set of comma separated flag identifiers into
 * a `text_interpretation` flags bitmap.
 * 
 * @param csflags String with comma separated flag identifiers.
 * @return A bitmap with text_interpretation flags.
 */
text_interpretation config_parse_text_interpretation(std::string csflags);

/**
 * Compare a lower case match string with part of a content string
 * in any combination of lower and upper case characters. This is
 * handy when checking for HTML tags.
 * 
 * @param lowermatch A lower case string that must be matched.
 * @param content A string containing HTML text and tags.
 * @param start_at Location within content string at which to start matching.
 * @return True if there is a complete match.
 */
bool lowercase_equal(const std::string & lowermatch, const std::string & content, size_t start_at = 0);

/**
 * Compare a multi-part lower case match string with possible space or
 * tab separated components with part of a content string in any
 * combination of lower and upper case characters. For example, use
 * this to check for a 'a href="' tag specialization.
 * 
 * Note: If the multi-part match must contain a space between two parts
 *       then that space must be included in one of the specified parts,
 *       e.g. "a " in the example above.
 * 
 * @param match_vec Vector of lower case strings that must be matched.
 * @param content A string containing HTML text and tags.
 * @param start_at Location within content string at which to start matching.
 * @return True if there is a complete match.
 */
bool lowercase_match_skipping_spaces(const std::vector<std::string> & match_vec, const std::string & content, size_t start_at = 0, size_t * url_start_ptr = nullptr);

/**
 * Remove all HTML tags to create raw text.
 * 
 * Note: At this time, this function does not yet remove text
 * content between <script></script> tags and similar.
 * 
 * @param htmlstr A string in HTML format.
 * @return A string in raw text format.
 */
std::string remove_html(const std::string & htmlstr);

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
std::string remove_html_tags(const std::string & htmlstr);

/**
 * Efficiently and reliably filter snippets of HTML text such that they become
 * optimally embeddable within other HTML.
 * 
 * The `interpretation` parameter is a collection of flags defined in
 * the `text_interpretation` enum (see above). This can be used to request
 * additional transformations of the raw text. For example, you can detect
 * URL links that were not surrounded by HREF code using the `detect_links`
 * flag.
 * 
 * @param htmlstr A string in HTML format.
 * @param detect_links If true then convert recognized data into links.
 * @param special_urls An optional pointer to a list of special URL
 *                     indicators to look for (e.g. @FZSERVER@).
 * @param replacements Pointer to list of replacement URLs to insert where
 *                     a special URL occurs.
 * @return A string of embeddable HTML.
 */
std::string make_embeddable_html(const std::string & htmlstr, text_interpretation interpretation = text_interpretation::raw, const std::vector<std::string> * special_urls = nullptr, const std::vector<std::string> * replacements = nullptr);

/**
 * Convert a string into a URI-safe string.
 * 
 * @param s Unencoded string.
 * @return Encoded string.
 */
std::string uri_encode(std::string s);

/**
 * Make HTML code for a link button.
 * 
 * @param link A link to open when the button is pressed.
 * @param label The label to put on the button.
 * @param samepage If true then the link is opened in the same browser page.
 * @return HTML code string to embed.
 */
std::string make_button(const std::string & link, const std::string & label, bool samepage);

/**
 * Generates HTML for a button that will copy contents from the innerHTML of
 * a DOM object with a specific id to the clipboard.
 * Use this with copy_html_from_id_js().
 * 
 * @param id The id of the object from which to copy.
 * @param label The text to show on the button.
 * @param button_class Optional extra button class CSS identifiers.
 * @return The HTML to embed.
 */
std::string copy_from_id_button(const std::string & id, const std::string & label, const std::string & button_class = "");

/**
 * Like copy_from_id_button(), but copies the value of an input element.
 * Note that the input element must have an id, not just a name field.
 * Use this with copy_value_from_id_js().
 */
std::string copy_from_input_button(const std::string & id, const std::string & label, const std::string & button_class = "");

/**
 * Generates Javascript for a function top copy contents from the innerHTML
 * of a DOM object with a specific id to the clipboard.
 * Use this with copy_from_id_button().
 * 
 * @param id The id of the object from which to copy.
 * @return The Javascript function to include (between <script> tags or in a .js file).
 */
std::string copy_html_from_id_js(const std::string & id);

/**
 * Like copy_html_from_id_js(), but copies the value of an input element.
 * Use this with copy_from_input_button().
 */
std::string copy_value_from_id_js(const std::string & id);

/**
 * To use this, make sure the page includes:
 * <link rel="stylesheet" href="/tooltip.css">
 */
std::string make_tooltip(const std::string& text, const std::string& tooltip_text);

} // namespace fz
#endif // __HTML_HPP
