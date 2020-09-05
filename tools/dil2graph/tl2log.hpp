// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the tl2log part of the
 * dil2graph tool.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __TL2LOG_HPP.
 */

#ifndef __TL2LOG_HPP
#include "version.hpp"
#define __TL2LOG_HPP (__VERSION_HPP)

#include "Logtypes.hpp"

class Task_Log; // forward declaration instead of including dil2al.hh here

using namespace fz;

extern bool manual_decisions;

Task_Log * get_Task_Log(std::ostream * o = nullptr);

unsigned int convert_TL_Chunk_to_Log_entries(Log & log, std::string chunktext);

std::unique_ptr<Log> convert_TL_to_Log(Task_Log * tl);

std::pair<Task_Log *, std::unique_ptr<Log>> interactive_TL2Log_conversion();

void print_Log_metrics(Log & log, std::ostream & o, std::string indent);

#endif // __TL2LOG_HPP
