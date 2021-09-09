// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to template rendering part of
 * fzserver-info.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __RENDER_HPP.
 */

#ifndef __RENDER_HPP
#include "version.hpp"
#define __RENDER_HPP (__VERSION_HPP)

// core
#include "templater.hpp"

// local
#include "fzserver-info.hpp"

using namespace fz;

bool output_response(std::string & rendered_str);

bool render_graph_server_status(const template_varvalues & statusinfo);

std::string render_shared_memory_blocks(const POSIX_shm_data_vec & shmblocksvec);

#endif // __RENDER_HPP
