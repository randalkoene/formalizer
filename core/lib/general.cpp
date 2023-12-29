// Copyright 2020 Randal A. Koene
// License TBD

/**
 * General collection of functions used in various Core and Tool programs.
 */

// std
#include <cstring>
#include <array>
#include <vector>
#include <memory>
#include <iomanip>
#include <sstream>

// core
#include "general.hpp"

namespace fz {

/**
 * Execute a shell command and retrieve the standard output as a string.
 * 
 * Note that this function will throw a runtime_error if the command could
 * not be executed.
 * 
 * @param cmd the shell command to execute.
 * @return a string containing the standard output of the executed command.
 */
std::string shellcmd2str(std::string cmd) {
    std::array<char, 2048> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed with command: "+cmd);
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

/**
 * Print double to string with specific precision.
 * 
 * @param d a real number.
 * @param p precision (default 2).
 * @param fillchar the fill character (default ' ', i.e. space).
 * @param w minimum width (default 0).
 * @return a string.
 */
std::string to_precision_string(double d, unsigned int p, char fillchar, unsigned int w) {
    std::stringstream stream;
    if (w>0) {
        stream.width(w);
        stream.fill(fillchar);
    }
    stream << std::fixed << std::setprecision(p) << d;
    return stream.str();
}

/**
 * Put pieces of a string into a pre-constructed vector.
 * 
 * See for example how this is used in the split() function below.
 * Borrowed from: https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string
 * Note that this solution does not skip empty tokens, so the following will
 * find 4 items, one of which is empty:
 * std::vector<std::string> x = split("one:two::three", ':');
 * 
 * @param s a string.
 * @param delim a delimiter character.
 * @param result a std::back_insert_iterator for the result container.
 **/
template <typename Out>
void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        *result++ = item;
    }
}

/**
 * Return a vector containing pieces of a string.
 * 
 * This uses the split() template above.
 * It lets you do things like pass the result directly to a function like this:
 * f(split(s, d, v)) while still having the benefit of a pre-allocated vector if you like.
 * 
 * @param s A string.
 * @param delim A delimiter character.
 * @return A vector of strings.
 */
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

/**
 * Efficient concatenation of strings from vector of strings with optional delimiter.
 * 
 * @param svec Vector of strings.
 * @param delim Optional delimiter string.
 * @return A string composed of all strings in the vector, each separated by the delimiter.
 */
std::string join(const std::vector<std::string> & svec, const std::string delim) {
    if (svec.empty())
        return "";

    if (delim.empty()) { // separating these for efficiency
        std::string s;
        for (const auto &element : svec)
            s += element;
        return s;

    } else {
        std::string s(svec.front());
        std::for_each(std::next(svec.begin()), svec.end(), [&](const std::string &element) {
            s += delim + element;
        });
        return s;
    }
}

bool find_in_vec_of_strings(const std::vector<std::string> & svec, const std::string & s) {
    for (const auto & candidate : svec) {
        if (candidate == s) return true;
    }
    return false;
}

std::string replace_char(const std::string & s, char c, char replacement) {
    std::string out_s(s);
    for (unsigned int i = 0; i<s.size(); i++) {
        if (s[i] == c)  out_s[i] = replacement;
    }
    return out_s;
}

/**
 * Find and return a substring enclosed by opening and closing
 * characters.
 * 
 * This is often used to find labels between brackets, e.g. [SOME-LABEL].
 * 
 * @param s The string to search in.
 * @param open_enclosure The opening character before the substring.
 * @param close_enclosure The closing character after the substring.
 * @param alt_return The string to return if an enclosed substring was not found.
 * @return The substring between enclosing characters or the alt_return string.
 */
std::string get_enclosed_substring(const std::string & s, char open_enclosure, char close_enclosure, const std::string & alt_return) {
    auto open_pos = s.find(open_enclosure);
    if (open_pos == std::string::npos) return alt_return;
    auto close_pos = s.find(close_enclosure, open_pos);
    if (close_pos == std::string::npos) return alt_return;
    open_pos++;
    return s.substr(open_pos, close_pos - open_pos);
}

/**
 * Safely copy from a string (not null-terminated) to a char buffer of limited
 * size. This can even include null-characters that were in the string. This
 * is safer than std::copy(), string::copy(), strcpy() or strncpy().
 * 
 * @param str A string.
 * @param buf Pointer to a limited-size character buffer.
 * @param bufsize Size of the character buffer (counting all chars, even if one is meant for '\0').
 */
void safecpy(std::string & str, char * buf, size_t bufsize) {
    if (bufsize<1)
        return;

    --bufsize; // counts from [0]
    if (str.size()<bufsize) // copy the string or up to buffer capacity
        bufsize = str.size();
    buf[bufsize] = 0; // make sure it is now null-terminated

    if (bufsize<1)
        return;

    memcpy(buf, str.data(), bufsize);
}

} // namespace fz
