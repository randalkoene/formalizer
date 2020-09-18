// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header provides classes and methods for configuration file loading and parsing.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __CONFIG_HPP.
 */

#ifndef __CONFIG_HPP
#include "coreversion.hpp"
#define __CONFIG_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
//#include <>

// core
//#include "error.hpp"


/// Use this handy macro within the `set_parameter()` function of classes inheriting `configurable`.
#define CONFIG_TEST_AND_SET_PAR(par, conflabel, confvalue) { \
    if (parname==conflabel) { \
        par = confvalue; \
        return true; \
    } }
/// Use this macro after the whole list of applicable parameter tests in `set_parameter()`.
#define CONFIG_PAR_NOT_FOUND(conflabel) ERRRETURNFALSE(__func__,"Configuration parameter ("+conflabel+") not recognized")

namespace fz {

/**
 * Formalizer standard programs that can use configuration files
 * should include a class that inherits `configurable` and that
 * provides method implementation for `set_parameter()`.
 * 
 * It makes sense to include an instance of that class as a
 * member variable of the local class derivation of
 * `formalizer_standard_program`.
 * 
 * This class is used to declare configurable parameters.
 */
class configurable {
public:
    std::string configfile;

    virtual bool set_parameter(const std::string & parlabel, const std::string & parvalue) = 0; // pure virtual!
};

/**
 * This class provides methods used on `configurable`, such as
 * loading and parsing configuration file contents.
 */
class configure {
public:
    bool load(configurable & config);
    bool parse(std::string & configcontentstr, configurable & config);
};


} // namespace fz

#endif // __CONFIG_HPP
