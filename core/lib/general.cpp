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
#include <filesystem>

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


/**
 * Write the full contents of a string to a file.
 * 
 * Note that this function does NOT create a backup of a previous file at the
 * given path.
 * 
 * @param path of the file.
 * @param s reference to the string.
 * @param writestate returns the iostate flags when provided (default: nullptr)
 * @return true if the write from string was successful.
 */
bool string_to_file(std::string path, std::string & s, std::ofstream::iostate * writestate) {
    std::ofstream ofs(path, std::ifstream::out);

    if (writestate) (*writestate) = ofs.rdstate();

    if (ofs.fail())
        ERRRETURNFALSE(__func__,"unable to create file "+path);

    ofs << s;
    ofs.close();
    return true;
}

/**
 * Write the full contents of a string to a file, but move an existing file
 * at the given path to a backup name first.
 * 
 * Note that TimeStamp:BackupStampYmd() and TimeStamp:BackupStampYmdHM() provide
 * valid Formalizer standardized backup extension tags.
 * 
 * @param path of the file.
 * @param s reference to the string.
 * @param backupext is the extension to use for a potential backup of existing.
 * @param backedup stores a flag to indicate is an existing file was renamed to a backup.
 * @param writestate returns the iostate flags when provided (default: nullptr)
 * @return true if the write from string was successful.
 */
bool string_to_file_with_backup(std::string path, std::string & s, std::string backupext, bool & backedup, std::ofstream::iostate * writestate) {
    if (!std::filesystem::exists(path))
        return string_to_file(path, s, writestate);

    if (backupext.empty())
        ERRRETURNFALSE(__func__,"missing backup extension tag");

    std::string attemptpath(path+".attempt");
    std::ofstream ofs(attemptpath, std::ifstream::out);

    if (writestate) (*writestate) = ofs.rdstate();

    if (ofs.fail())
        ERRRETURNFALSE(__func__,"unable to create file "+attemptpath);

    ofs << s;
    ofs.close();

    std::string backuppath(path+'.'+backupext);
    backedup = (rename(path.c_str(),backuppath.c_str()) == 0);
    if (!backedup)
        ERRRETURNFALSE(__func__,"unable to create "+backuppath+", retained "+attemptpath);

    if (rename(attemptpath.c_str(),path.c_str()) != 0)
        ERRRETURNFALSE(__func__,"unable to rename "+attemptpath);

    return true;
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

/**
 * Read the full contents of a (text) stream into a string.
 * 
 * This implementation was tested to be faster than the one-liner implementations shown
 * at https://stackoverflow.com/questions/3203452/how-to-read-entire-stream-into-a-stdstring,
 * at least as of Oct. 2011. It might be even faster to pre-allocate buffer space according
 * to stream size, as per the example inhttp://www.cplusplus.com/reference/istream/istream/read/,
 * but it isn't immediately clear to me if that also works with STDIN.
 * 
 * Note: This can be used with STDIN (`cin`) as well.
 * 
 * @param in is an open istream.
 * @param s reference to the receiving string.
 * @return true if the read into string was successful.
 */
bool stream_to_string(std::istream &in, std::string & s) {
    char buffer[4096];
    while (in.read(buffer, sizeof(buffer)))
        s.append(buffer, sizeof(buffer));
    s.append(buffer, in.gcount());
    return true; // *** might want to test the `badbit` mentioned in documentation
}

} // namespace fz
