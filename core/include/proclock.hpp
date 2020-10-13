// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares functions for process detection and lock files.
 * 
 * The corresponding source file is at core/lib/proclock.cpp.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __PROCLOCK_HPP.
 */

#ifndef __PROCLOCK_HPP
#include "coreversion.hpp"
#define __PROCLOCK_HPP (__COREVERSION_HPP)

// std
#include <sys/types.h>

namespace fz {

/**
 * Obtain the PID of this program.
 */
pid_t this_program_process_ID();

/**
 * Test if a process with a specific PID is running.
 * 
 * This uses the kill() function with signal 0, which sends no
 * actual signal but still performs error checking.
 */
bool test_process_running(pid_t pid);

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
int check_and_make_lockfile(const std::string lockfilepath, std::string extrainfo);

/**
 * Remove lockfile and report if the lockfile existed.
 * 
 * @param lockfilepath The path of a lockfile.
 * @return 0 if the lockfile existed and was removed, 1 if the lockfile did not exist, -1 on error.
 */
int remove_lockfile(const std::string lockfilepath);

/**
 * Test to see if a lockfile exists and read it.
 * 
 * @param[in] lockfilepath The path of a lockfile.
 * @param[out] lockfilecontent A string variable to receive the lockfile contents.
 * @return 0 if the lockfile does not exist, 1 if it does exist, -1 on error.
 */
int check_and_read_lockfile(const std::string lockfilepath, std::string & lockfilecontent);

} // namespace fz

#endif // __PROCLOCK_HPP
