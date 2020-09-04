// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This file provides access to the minimal set of functions from dil2al/dil2al.cc needed for
 * initialization and utilization of DIL Hierarchy v1.x files.
 * 
 * The function declarations below are only for those functions that are not already
 * declared in dil2al.hh.
 */

#ifndef __DIL2AL_MINIMAL_HPP
#include "version.hpp"
#define __DIL2AL_MINIMAL_HPP (__VERSION_HPP)

#ifndef DEFAULTHOMEDIR
#define DEFAULTHOMEDIR "/home/randalk"
#endif

void initialize();

void exit_report();

void exit_postop();

#endif // __DIL2AL_MINIMAL_HPP
