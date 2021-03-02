// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to template rendering part of
 * fzdashboard.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __RENDER_HPP.
 */

#ifndef __RENDER_HPP
#include "version.hpp"
#define __RENDER_HPP (__VERSION_HPP)


using namespace fz;

enum dynamic_or_static: unsigned int {
    dynamic_html = 0,
    static_html = 1
};

bool render(dynamic_or_static html_output = dynamic_html);

#endif // __RENDER_HPP
