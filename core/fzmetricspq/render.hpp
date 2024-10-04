// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to template rendering part of
 * fzmetricspq.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __RENDER_HPP.
 */

#ifndef __RENDER_HPP
#include "version.hpp"
#define __RENDER_HPP (__VERSION_HPP)

#include "fzmetricspq.hpp"

using namespace fz;

bool render_data(Metrics_data& data, std::string& rendered_data);

#endif // __RENDER_HPP
