// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to handling refresh requests made with
 * the fzquerypq server.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __REFRESH_HPP.
 */

#ifndef __REFRESH_HPP
#include "version.hpp"
#define ___REFRESH_HPP (__VERSION_HPP)

void refresh_Node_histories_cache_table();

void refresh_Named_Node_Lists_cache_table();

#endif // __REFRESH_HPP
