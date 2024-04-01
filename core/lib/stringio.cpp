// Copyright 2020 Randal A. Koene
// License TBD

/**
 * General collection of functions used for string I/O with files and streams.
 */

// std
#include <filesystem>

// core
//#include "error.hpp"
#include "stringio.hpp"

namespace fz {

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
bool string_to_file(const std::string path, const std::string & s, std::ofstream::iostate * writestate) {
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
bool file_to_string(const std::string & path, std::string & s, std::ifstream::iostate * readstate) {
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

/**
 * Try to fill a string with text content if it was empty.
 * 
 * E.g. see how this is used in fzaddnode.cpp.
 * 
 * @param utf8_text A string reference to receive the content (if not already present).
 * @param content_path A possible path to a file containing text content.
 * @param errmsg_target A short string to be used in error messages.
 * @return A pair with a proposed exit code (exit_ok if no problem) and possible error message (empty if no problem).
 */
std::pair<exit_status_code, std::string> get_content(std::string & utf8_text, const std::string content_path, const std::string errmsg_target) {
    ERRTRACE;
    if (utf8_text.empty()) {
        if (content_path.empty()) {
            return std::make_pair(exit_missing_parameter, "Missing "+errmsg_target+" content");
        }
        if (content_path == "STDIN") {
            if (!stream_to_string(std::cin, utf8_text)) {
                return std::make_pair(exit_file_error, "Unable to obtain "+errmsg_target+" content from standard input");
            }
        } else {
            if (!file_to_string(content_path, utf8_text)) {
                return std::make_pair(exit_file_error, "Unable to obtain "+errmsg_target+" content from file at "+content_path);
            }
        }
        if (utf8_text.empty()) {
            return std::make_pair(exit_missing_data, "Missing "+errmsg_target+" content");
        }
    }
    return std::make_pair(exit_ok, "");
}

} // namespace fz
