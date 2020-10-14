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

} // namespace fz

#endif // __HTML_HPP
