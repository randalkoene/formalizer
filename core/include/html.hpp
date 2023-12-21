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
 * @return A string of embeddable HTML.
 */
std::string make_embeddable_html(const std::string & htmlstr, text_interpretation interpretation = text_interpretation::raw);

/**
 * Convert a string into a URI-safe string.
 * 
 * @param s Unencoded string.
 * @return Encoded string.
 */
std::string uri_encode(std::string s);

} // namespace fz

#endif // __HTML_HPP
