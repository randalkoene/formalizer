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
#define CONFIG_TEST_AND_SET_PAR(par, parname, conflabel, confvalue) { \
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
protected:
    std::string configfile; ///< This is set in the constructor.

    bool processed; ///< Flag used to ensure a configuration file is normally processed only once.

public:
    configurable(std::string thisprogram);

    /**
     * This method must be defined in configurable Formalizer components and
     * programs that define a class inheriting `configurable`.
     * 
     * It is called by `parse()` and does not need to be called directly.
     * It is provided as a public method here, because it can also be used
     * as a way to configure a specific parameter by label. That may be
     * useful for web interfaces and other means by which parameters can be
     * specified interactively.
     * 
     * @return True if a parameter was set or no legible parlabel or parvalue was given,
     *         False if a proper parlabel string was extracted, but the corresponding
     *         parameter was not recognized (possibly a syntax error in the parlabel).
     */
    virtual bool set_parameter(const std::string & parlabel, const std::string & parvalue) = 0; // pure virtual!

    /**
     * Call this function from a place where it is guaranteed to be run before
     * the configurable parameters are used. Logically, it should be called
     * before command line parameters are parsed (see https://trello.com/c/eJPHqQdC).
     * 
     * In `formalizer_standard_program` code that defines an `init_top()`
     * function (which calls the standard `init()`), a natural fit is to make
     * this call at the top of that function, before the call to `init()`.
     * 
     * In `formalizer_standard_program` code and in core components that provide
     * command line configuration hooks (e.g. fzpostgres) AND that require at least
     * one mandatory command line argument, a natural fit could be to call this
     * function at the top of `options_hook()`, because that guarantees that it
     * will be run just like command line parsing will be run, and it will occur
     * before corresponding parameters are set on the command line.
     * 
     * Another natural location could be in the constructor of the class that
     * inherits configurable - if the state of the program is ready for this
     * at the time when the constructor is called.
     * 
     * Don't worry about multiple calls (as can happen within options_hook()).
     * This function checks and sets a flag to ensure it processes the configuration
     * file only once (unless the flag is explicitly reset).
     * 
     * It can be useful to embed the `load()` call in some error reporting code
     * (see for example `fzloghtml`).
     * 
     * @return True if configuration processing was successful or if the `processed` flag was already set.
     */
    bool load();

    /**
     * Provides explicit reset of the `processed` flag. This can be useful
     * if a program provides an interface for modifying a configuration file
     * and if it wishes to call `load()` again to activate the modified
     * parameter settings.
     */
    void reset() { processed = false; }

    /**
     * The configuration file contents parsing function is called by `load()`
     * and does not need to be called directly. It is made available as a
     * public method, because it can also be used as a way to configure
     * parameters by label. That might be useful for web interfaces and
     * other means by which parameters can be specified interactively.
     * 
     * @param configcontentstr A string containing parameter-value pairs in
     *        a format that is a JSON subset without nesting.
     * @return True if configuration parsing was successfully completed.
     */
    bool parse(std::string & configcontentstr);
};


} // namespace fz

#endif // __CONFIG_HPP
