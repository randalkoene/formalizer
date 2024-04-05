// Copyright 2020 Randal A. Koene
// License TBD

// std

// core
#include "stringio.hpp"
#include "TimeStamp.hpp"
#include "debug.hpp"

namespace fz {

Debug_LogFile::Debug_LogFile(const std::string & _logpath): logpath(_logpath) {
	initialize();
}

std::string Debug_LogFile::make_entry_str(const std::string & message) const {
	return TimeStampYmdHM(ActualTime())+": "+message+'\n';
}

/**
 * Create fresh log file and add initializing time stamp.
 */
void Debug_LogFile::initialize() {
	valid_logfile = string_to_file(logpath, make_entry_str("Debug log started."));
}

/**
 * Append a message to the debug log file.
 * 
 * @param message A message string.
 * @return True if successfully logged to file.
 */
bool Debug_LogFile::log(const std::string & message) {
	valid_logfile &= append_string_to_file(logpath, make_entry_str(message));
	return valid_logfile;
}

} // namespace fz
