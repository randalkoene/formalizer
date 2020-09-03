// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares standard structures and functions that should be used with
 * any standardized Formalizer C++ program.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define ___STANDARD_HPP.
 */

#ifndef __STANDARD_HPP
#include "coreversion.hpp"
#define __STANDARD_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

//*** consider removing the next one and setting base.out and base.err to nullptr first
#include <iostream>
#include <stdlib.h>
#include <cstdlib>
#include <ostream>
#include <memory>

#include "error.hpp"

// *** The following will be removed once the fzserverpq is ready.
#define TEMPORARY_DIRECT_GRAPH_LOAD_IN_USE

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

/// Utility macros
#define FZOUT(s) { if (base.out) (*base.out) << s; }
#define FZERR(s) { if (base.err) (*base.err) << s; }

namespace fz {

enum exit_status_code {
    exit_ok,
    exit_general_error,
    exit_database_error,
    exit_unknown_option,
    exit_cancel,
    exit_conversion_error,
    exit_DIL_error,
    exit_unable_to_stack_clean_exit,
    exit_command_line_error,
    exit_NUMENUMS // this one simplifies corresponding array definition, e.g. char[exit_NUMENUMS]
    };

void error_summary_wrapper();

void clean_exit_wrapper();

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
struct formalizer_standard_program {
    std::string server_long_id; /// standardized module string

    std::string add_option_args; // more option argument characters, e.g. "n:F:"
    std::string add_usage_top; // more options, e.g. " [-d <dbname>] [-m]"

    bool quiet; // report less

    /**
     * Base initialization of standard Formalizer programs.
     */
    formalizer_standard_program();

    /**
     * You can call this instead of std::exit() if you like, same thing, same stack.
     * There is one minor difference that might matter when you want to:
     * a) run a program quietly, and,
     * b) leave base.out unaltered (no need for a special handler or nullptr).
     * This version of exit copies `quiet` explicitly to `standard.quiet`, so
     * that `error_summary_wrapper` can see it.
     */
    void exit(int exit_code);

    /**
     * Add additional steps to the exit() stack.
     */
    void add_to_exit_stack(void (*func) (void)) {
        // *** could add a step here to remember a list of exit steps that can be reported
        if (atexit(func)!=0) {
            ADDERROR(__func__,"unable to add clean exit function to exit stack");
            error_summary_wrapper();
            clean_exit_wrapper();
            exit(exit_unable_to_stack_clean_exit);
        }
    }

protected:
    bool initialized = false; // This is used to test if standard.init() was called before permitting other things.

    std::string runnablename;

    void commandline(int argc, char *argv[]);

public:
    bool was_initialized() { return initialized; }

    /**
     * The standard initialization procedure. This should typically be called in
     * the first line (or so) of `main()`. Whether that was done is tested by the
     * `initialized` flag.
     */
    void init(int argc, char *argv[], std::string version, std::string module, std::ostream * o = nullptr, std::ostream * e = nullptr);

    /**
     * An all-is-well exit call.
     */
    int completed_ok();

    std::string const & name() { return runnablename; }

    void print_version();

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


//*** It is a little bit weird to have this here. Probably move it to Graphpostgres.hpp or to
//    a special Graphaccess.hpp set.
class Graph; // forward declaration
/**
 * A standardized way to access the Graph database.
 * 
 * Note: While TEMPORARY_DIRECT_GRAPH_LOAD_IN_USE is defined this includes
 *       code to provide direct access to the Postgres database, which is
 *       not advisable and will be replaced.
 */
struct Graph_access {
    std::string dbname;

    bool is_server; /// authoritative server programs should set this flag

    /**
     * Carry out initializations needed to enable access to the Graph data structure.
     * 
     * @param add_topion_args_here receiving string where "d:" is appended to extend
     *                             command line options recognized.
     * @param add_usage_top_here receiving string where the option format is appended
     *                           to extend the top line of usage output.
     */
    Graph_access(std::string & add_option_args_here, std::string & add_usage_top_here, bool _isserver = false): is_server(_isserver) {
        COMPILEDPING(std::cout, "PING-Graph_access().1\n");
        // This is better called just before a Graph request: graph_access_initialize();
        add_option_args_here += "d:";
        add_usage_top_here += " [-d <dbname>]";
    }

    void usage_hook();

    bool options_hook(char c, std::string cargs);

protected:
    void dbname_error();
    void graph_access_initialize();

#ifdef TEMPORARY_DIRECT_GRAPH_LOAD_IN_USE
public:
    std::unique_ptr<Graph> request_Graph_copy();
#endif

};

std::pair<int, std::string> safe_cmdline_options(int argc, char *argv[], std::string options, int &optindcopy);

//*** It is not entirely clear if key_pause() should be here or in some stream utility set.
void key_pause();

extern formalizer_standard_program standard;

} // namespace fz

#endif // __STANDARD_HPP
