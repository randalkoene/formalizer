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

/**
 * Test the Log for logical inconsistencies and propose or apply fixes.
 * 
 * Examples of logical inconsistencies are:
 * 
 * - Missing Log chunk close times when the chunk is not the last one in the Log.
 * - Log chunk IDs that appear to be out of temporal order.
 * - Duplicate Log chunk IDs.
 * 
 * Fixes are applied automatically unless `manual_decisions` is true.
 * 
 * @param log A reference to a Log object containing Log chunks and entries.
 */
void Log_Integrity_Tests(Log & log);

std::unique_ptr<Log> convert_TL_to_Log(Task_Log * tl);

std::pair<Task_Log *, std::unique_ptr<Log>> interactive_TL2Log_conversion();

void interactive_TL2Log_validation(Task_Log * tl, Log * log, Graph * graph);

void print_Log_metrics(Log & log, std::ostream & o, std::string indent);

#endif // __TL2LOG_HPP
