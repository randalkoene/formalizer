// Copyright 2020 Randal A. Koene
// License TBD

/**
 * General collection of functions for process detection and lock files.
 */

// std
#include <unistd.h>
#include <signal.h>
#include <filesystem>

// core
#include "error.hpp"
#include "TimeStamp.hpp"
#include "jsonlite.hpp"
#include "stringio.hpp"
#include "proclock.hpp"

namespace fz {

/**
 * Obtain the PID of this program.
 * 
 * To use, #include <sys/types.h>, #include <unistd.h>.
 */
pid_t this_program_process_ID() {
    pid_t pid = getpid();
    return pid;
}

/**
 * Test if a process with a specific PID is running.
 * 
 * This uses the kill() function with signal 0, which sends no
 * actual signal but still performs error checking.
 * 
 * To use, #include <sys/types.h>, #include <signal.h>.
 */
bool test_process_running(pid_t pid) {
    int ret = kill(pid, 0);
    if (ret==0)
        return true;
    
    if (errno != ESRCH)
        ADDERROR(__func__, "EINVAL or EPERM received, we may not have permission to signal the target process");
    return false;
}

/**
 * Create a lockfile that contains a PID and time stamp, plus
 * optional extra info.
 * 
 * The extra info is assumed to be a simple string unless it
 * begins with '{', in which case it is assumed to be in
 * JSON format.
 * 
 * The file is created in a simple pseudo-JSON format.
 * 
 * @param lockfilepath The path of the lockfile to create.
 * @param extrainfo Any additional data to include.
 * @return 0 if the lockfile was made, 1 if it already existed, or -1 on error.
 */
int check_and_make_lockfile(const std::string lockfilepath, std::string extrainfo) {
    // Test if the lockfile already exists and return if it does.
    if (std::filesystem::exists(lockfilepath))
        return 1;

    // Create the basic information for the lockfile.
    pid_t pid = this_program_process_ID();
    std::string pid_str = std::to_string(pid);
    std::string tstamp_str = TimeStampYmdHM(ActualTime());

    // Create the pseudo-JSON string.
    jsonlite_label_value_pairs labelvaluepairs;
    labelvaluepairs["pid"] = pid_str;
    labelvaluepairs["time"] = tstamp_str;
    if (!extrainfo.empty()) {
        if (extrainfo.front()=='{') { // JSON parameter-value pairs to add
            jsonlite_label_value_pairs extralabelvaluepairs = json_get_label_value_pairs_from_string(extrainfo);
            labelvaluepairs.merge(extralabelvaluepairs);
        } else { // simple extra info string to add
            labelvaluepairs["info"] = extrainfo;
        }
    }
    std::string jsonstr = json_label_value_pairs_to_string(labelvaluepairs);

    // Write the lockfile
    if (!string_to_file(lockfilepath, jsonstr)) {
        ADDERROR(__func__, "Unable to write lockfile to "+lockfilepath);
        return -1;
    }

    return 0;
}

/**
 * Remove lockfile and report if the lockfile existed.
 * 
 * @param lockfilepath The path of a lockfile.
 * @return 0 if the lockfile existed and was removed, 1 if the lockfile did not exist, -1 on error.
 */
int remove_lockfile(const std::string lockfilepath) {
    if (!std::filesystem::exists(lockfilepath))
        return 1;

    std::filesystem::path p = lockfilepath;
    std::error_code ec;
    if (std::filesystem::remove(p, ec)) {
        return 0;
    }

    ADDERROR(__func__, "Unable to remove lockfile at "+lockfilepath);
    return -1;
}

/**
 * Test to see if a lockfile exists and read it.
 * 
 * @param[in] lockfilepath The path of a lockfile.
 * @param[out] lockfilecontent A string variable to receive the lockfile contents.
 * @return 0 if the lockfile does not exist, 1 if it does exist, -1 on error.
 */
int check_and_read_lockfile(const std::string lockfilepath, std::string & lockfilecontent) {
    if (!std::filesystem::exists(lockfilepath))
        return 0;
    
    if (!file_to_string(lockfilepath, lockfilecontent)) {
        ADDERROR(__func__, "Unable to read contents of lockfile at "+lockfilepath);
        return -1;
    }

    return 1;
}

} // namespace fz
