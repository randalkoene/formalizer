// Copyright 2020 Randal A. Koene
// License TBD

/**
 * The utf8.cpp and utf8.hpp files are core utility wrappers for the open source
 * cpputf library. See details in utf8.hpp.
 */

// required opensource
#include "utfcpp/source/utf8.h"

// core
#include "error.hpp"
#include "utf8.hpp"

namespace fz {

/**
 * Filter a text string to ensure that it contains valid UTF8 encoded content.
 * 
 * Attempts to assign content to the text parameter as provided, and
 * replaces any invalid UTF8 code points with a replacement
 * character. The default replacement character is the UTF8
 * 'REPLACEMENT CHARACTER' 0xfffd.
 * 
 * If invalid UTF8 code points were encountered and replaced then that is
 * optionally reported through ADDWARNING.
 * 
 * Note 1: ASCII text is valid UTF8 text and will be assigned
 *         unaltered.
 * Note 2: This does not test for valid HTML5 at this time.
 * 
 * @param utf8str A string that should contain UTF8 encoded text.
 * @param warn If true then warn about invalid UTF8 code replacements.
 * @return A guaranteed utf8 compliant string.
 */
std::string utf8_safe(const std::string & utf8str, bool warn) {
    utf8::reset_utf_fixes();
    if (!warn)
        return utf8::replace_invalid(utf8str);

    std::string utf8safe = utf8::replace_invalid(utf8str);
    if (utf8::check_utf_fixes()>0)
        ADDWARNING(__func__,"replaced "+std::to_string(utf8::check_utf_fixes())+" invalid UTF8 code points in string ("+utf8str.substr(0,20)+"...)");
    
    return utf8safe;
}

}
