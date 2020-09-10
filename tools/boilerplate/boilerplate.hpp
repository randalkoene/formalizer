// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the boilerplate tool.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __BOILERPLATE_HPP.
 */

#ifndef __BOILERPLATE_HPP
#include "version.hpp"
#define __BOILERPLATE_HPP (__VERSION_HPP)

// core
#include "standard.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0, /// no recognized request
    flow_cpp = 1,     /// request: make boilerplate for C++ program
    flow_python = 2,  /// request: make boilerplate for Python program
    flow_NUMoptions
};

struct boilerplate: public formalizer_standard_program {

    flow_options flowcontrol;

    boilerplate();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

};

extern boilerplate bp;

#endif // __BOILERPLATE_HPP
