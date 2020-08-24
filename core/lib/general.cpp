// Copyright 2020 Randal A. Koene
// License TBD

/**
 * General collection of functions used in various Core and Tool programs.
 */

#include <array>
#include <vector>
#include <memory>
#include <iomanip>
#include <sstream>
#include <fstream>

#include "error.hpp"
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
 * Note: Perhaps this should be moved to general utility functions.
 * 
 * @param d a real number.
 * @param p precision (default 2).
 * @return a string.
 */
std::string to_precision_string(double d, unsigned int p) {
    std::stringstream stream;
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
 * @param s a string.
 * @param delim a delimiter character.
 * @return a vector of strings.
 */
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

/**
 * Read the full contents of a (text) file into a string.
 * 
 * The contents of the receiving string are replaced.
 * For efficiency, this function finds the size of the file and reserves space
 * in the string before pulling in the content.
 * 
 * @param path of the file.
 * @param s reference to the receiving string.
 * @param readstate returns the iostate flags when provided (default: nullptr)
 * @return true if the read into string was successful.
 */
bool file_to_string(std::string path, std::string & s, std::ifstream::iostate * readstate) {
    std::ifstream ifs(path, std::ifstream::in);

    if (readstate) (*readstate) = ifs.rdstate();

    if (ifs.fail())
        ERRRETURNFALSE(__func__,"unable to open file "+path);

    ifs.seekg(0, std::ios::end);
    s.reserve(ifs.tellg());
    ifs.seekg(0, std::ios::beg);

    s.assign((std::istreambuf_iterator<char>(ifs)),std::istreambuf_iterator<char>());
    ifs.close();
    return true;
}

} // namespace fz
