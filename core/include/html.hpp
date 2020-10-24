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
 * With the `detect_links` option, this filter also detects interesting
 * information that would be improved if presented as a link and adds such a
 * link.
 * 
 * @param htmlstr A string in HTML format.
 * @param detect_links If true then convert recognized data into links.
 * @return A string of embeddable HTML.
 */
std::string make_embeddable_html(const std::string & htmlstr, bool detect_links = true);

} // namespace fz

#endif // __HTML_HPP
