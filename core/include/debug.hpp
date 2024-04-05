// Copyright 2024 Randal A. Koene
// License TBD

/** @file debug.hpp
 * This header file declares debug handling classes and functions for use with core
 * Formalizer C++ code.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __GRAPHTYPES_HPP.
 */

#ifndef __DEBUG_HPP
#include "coreversion.hpp"
#define __DEBUG_HPP (__COREVERSION_HPP)

// std
#include <string>

// core

namespace fz {

/**
 * Instantly flushing debug logging to a file.
 * 
 * This is useful to track waypoint and catch issues even in programs
 * that work in the background and do not print to standard streams,
 * and even when the normal Formalizer logging process is disrupted.
 */
class Debug_LogFile {
protected:
    std::string logpath;
    bool valid_logfile = false;

    std::string make_entry_str(const std::string & message) const;

public:
    Debug_LogFile(const std::string & _logpath);

    bool valid() const { return valid_logfile; }

    /**
     * Create fresh log file and add initializing time stamp.
     */
    void initialize();

    /**
     * Append a message to the debug log file.
     * 
     * @param message A message string.
     * @return True if successfully logged to file.
     */
    bool log(const std::string & message);

};

/**
 * To easily include or exclude debug logging, use the macros below.
 * To enable the macros, define the directive FZACTIVATE_DEBUGLOG.
 * For example, do so through the Makefile. See how this is done in
 * fzserverpq.
 */

#ifdef FZACTIVATE_DEBUGLOG

    #define Set_Debug_LogFile(_logpath) Debug_LogFile dbglog(_logpath)

    #define To_Debug_LogFile(message) dbglog.log(message)

#else

    #define Set_Debug_LogFile(_logpath) char dbglog

    #define To_Debug_LogFile(message) ((void)0)

#endif

} // namespace fz

#endif // __DEBUG_HPP
