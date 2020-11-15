// Copyright 2020 Randal A. Koene
// License TBD

/** @file standard.hpp
 * This header file declares standard structures and functions that should be used with
 * any standardized Formalizer C++ program.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define ___STANDARD_HPP.
 */

#ifndef __STANDARD_HPP
#include "coreversion.hpp"
#define __STANDARD_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
//*** consider removing the next one and setting base.out and base.err to nullptr first
#include <iostream>
#include <stdlib.h>
#include <cstdlib>
#include <ostream>
#include <memory>
#include <deque>

// core
// Note: Purposely keep error.hpp out of here (it can be in standard.cpp).
//       This is helpful in case standard.hpp needs to be included in
//       config.hpp, and we wish to avoid mutual inclusions.

/// Provide the following through -D in Makefile
#ifndef FORMALIZER_MODULE_ID
    #define FORMALIZER_MODULE_ID "Formalizer:DUMMYMODULE-REPLACE-THIS!"
#endif
#ifndef FORMALIZER_BASE_OUT_OSTREAM_PTR
    #define FORMALIZER_BASE_OUT_OSTREAM_PTR (nullptr)
#endif
#ifndef FORMALIZER_BASE_ERR_OSTREAM_PTR
    #define FORMALIZER_BASE_ERR_OSTREAM_PTR (nullptr)
#endif

/**
 * Utility macros for regular and verbose out and err streams.
 * 
 * - Use `FZOUT(s)` to stream out to `base.out`.
 * - Use `FZERR(s)` to stream out to `base.err`.
 * - Use `VERBOSEOUT(s)` to stream out to `base.out` when `standard.quiet==false`.
 * - Use `VERBOSEERR(s)` to stream out to `base.err` when `standard.quiet==false`.
 */
#define FZOUT(s) { if (fz::base.out) (*fz::base.out) << s; }
#define FZERR(s) { if (fz::base.err) (*fz::base.err) << s; }
#define VERBOSEOUT(s) { if ((fz::base.out) && (!fz::standard.quiet)) (*fz::base.out) << s; }
#define VERBOSEERR(s) { if ((fz::base.err) && (!fz::standard.quiet)) (*fz::base.err) << s; }
#define VERYVERBOSEOUT(s) { if ((fz::base.out) && (fz::standard.veryverbose)) { (*fz::base.out) << s; fz::base.out->flush(); } }

namespace fz {

// Forward declaration
class main_init_register;

void error_summary_wrapper();

void clean_exit_wrapper();

/**
 * The standardized structure for standard stream redirection.
 */
struct formalizer_base_streams {
    std::ostream *out = &std::cout;//nullptr;      /// for example *cout
    std::ostream *err = &std::cerr;//nullptr;      /// for example *cerr
};

extern formalizer_base_streams base;

/**
 * This structure registers a `standard` object with a set of expected
 * parameters and functions for any standardized Formalizer program.
 * 
 * About:
 *   `exit`: After the standard object has been registered, you can do
 *           a clean exit from the program by calling either `exit()`
 *           or `standard::exit()`. You can add additional clean-up
 *           steps to the exit stack by calling add_to_exit_stack().
 */
struct the_standard_object {
    std::string server_long_id;         ///< standardized module string (see documentation)
    bool quiet;                         ///< report less
    bool veryverbose;                   ///< report much more

    std::string runnablename;           ///< set this in formalizer_standard_program::init()

    the_standard_object(): server_long_id(FORMALIZER_MODULE_ID), quiet(false), veryverbose(false) {}

    /// Typically call set_name() from formalizer_standard_program::init().
    void set_name(std::string argv0);

    /// Typically call set_id() from formalizer_standard_program::init().
    void set_id(std::string _severlongid);

    /**
     * You can call this instead of std::exit() if you like, same thing, same stack.
     * There is one minor difference that might matter when you want to:
     * a) run a program quietly, and,
     * b) leave base.out unaltered (no need for a special handler or nullptr).
     * This version of exit copies `quiet` explicitly to `standard.quiet`, so
     * that `error_summary_wrapper` can see it.
     */
    [[noreturn]] void exit(int exit_code);

    /**
     * Add additional steps to the exit() stack.
     * 
     * This affects all paths to exit. The additional exit hooks will be called if
     * the regular `exit()` function or a `formalizer_standard_program::exit()` derived
     * function is called. It's a stack, so the order of the calls to the hooks is the
     * reverse of the order in which they were added.
     * 
     * The exit hook function must be a regular function, not a method (member function)
     * of a class, but you could always make a wrapper around a call to a method.
     * 
     * @param func Any void function that takes no parameters.
     * @param label A string that names or describes the exit hook being added.
     */
    void add_to_exit_stack(void (*func) (void), std::string label);

    /**
     * An all-is-well exit call.
     */
    [[noreturn]] int completed_ok();

    void print_version();
};

/**
 * This object projects one unique and clear interface with settings
 * for standardized Formalizer programs, including exit hooks and
 * other important requirements.
 */
extern the_standard_object standard;

/**
 * This class provides a framework for standardized Formalizer programs.
 * It should be inherited by a local struct or class declared and defined
 * in such a program.
 * 
 * Note: This does not replace variables or methods of the standard object
 * `standard`, of which there should be just one, and which guarantees one
 * clear interface with settings.
 */
class formalizer_standard_program {
protected:
    bool uses_config = true;  ///< This standard program uses standard configuration methods.
    bool initialized = false; ///< This is used to test if standard.init() was called before permitting other things.

    //std::deque<bool (*) (void)> init_register_stack; ///< Here we store registered early init calls (see `main_init_register`).
    std::deque<main_init_register *> init_register_stack;

    void commandline(int argc, char *argv[]);

public:
    std::string add_option_args;        ///< more option argument characters, e.g. "n:F:"
    std::string add_usage_top;          ///< more options, e.g. " [-d <dbname>] [-m]"
    std::deque<std::string> usage_head; ///< strings to print at the head of usage information
    std::deque<std::string> usage_tail; ///< strings to print at the tail of usage information

    /**
     * Base initialization of standard Formalizer programs.
     * 
     * @param _usesconfig Does this standardized component make use of the standard configuration method.
     */
    formalizer_standard_program(bool _usesconfig);

    bool was_initialized() { return initialized; }

    void add_registered_init(main_init_register * mir);//bool (*initfunc) (void)); ///< Called by the constructor of `main_init_register`.

    /**
     * The standard initialization procedure. This should typically be called in the first
     * line (or so) of `main()`. Whether that was done is tested by the `initialized` flag.
     */
    void init(int argc, char *argv[], std::string version, std::string module, std::ostream * o = nullptr, std::ostream * e = nullptr);

    std::string const & name() const { return standard.runnablename; }

    std::string const & id() const { return standard.server_long_id; }

    void print_usage();

    // *** instead of these hooks and add_usage_top there could be a usage_stack
    //     that you can register strings with.
    /**
     * Replace this usage hook by inheriting the class and specifying a derived
     * version of this virtual member function.
     */
    virtual void usage_hook() {}

    /**
     * Replace this options processing hook by inheriting the class and specifying a
     * derived version of this virtual member function.
     * 
     * @param c is the command line option character.
     * @param cargs points to the option argument value if `c` has one (otherwise it is
     *              undefined - so DO NOT assume that you can use it to initialize a string).
     */
    virtual bool options_hook(char c, std::string cargs) { return false; }

};

/**
 * Classes that need an initialization function to be called first-thing upon
 * entering `main()` (but not before that in the hazardous global variables
 * constructor calls phase) should inherit this class.
 * 
 * Their initialization function will then be added to the `init_register_stack`
 * of the `formalizer_standard_program` of which they are a part.
 */
class main_init_register {
public:
    main_init_register(formalizer_standard_program & fsp); //, bool (*initfunc) (void));

    virtual bool init() = 0; ///< Define this, at least as a wrapper around any other function.
};

std::pair<int, std::string> safe_cmdline_options(int argc, char *argv[], std::string options, int &optindcopy);

//*** It is not entirely clear if key_pause() should be here or in some stream utility set.
void key_pause();
bool default_choice(const std::string question, char not_default);

/// Another set of exit options, these with potential messages.
int standard_exit_success(std::string veryverbose_message);
[[noreturn]] int standard_exit_error(int exit_code, std::string error_message, const char * problem__func__);
int standard_exit(bool success, std::string veryverbose_message, int exit_code, std::string error_message, const char * problem__func__);

/// A function that combines ADDERROR and VERBOSEERR without exit. Always returns false.
bool standard_error(std::string error_message, const char * problem__func__);

} // namespace fz

#endif // __STANDARD_HPP
