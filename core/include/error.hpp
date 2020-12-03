// Copyright 2020 Randal A. Koene
// License TBD

/** @file error.hpp
 * This header file declares error handling classes and functions for use with core
 * Formalizer C++ code.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __GRAPHTYPES_HPP.
 */

#ifndef __ERROR_HPP
#include "coreversion.hpp"
#define __ERROR_HPP (__COREVERSION_HPP)

// std
#include <fstream>
#include <string>
#include <deque>
#include <vector>

// core
#include "config.hpp"

/// +----- begin: A set of useful macros -----+

/**
 * You can specify NO_ERR_TRACE if you are confident that you don't need that and want to save a few cycles.
 * You can also specify NO_ERR_HINTS if you just want no string copying in the way of maximum performance.
 */
#ifdef NO_ERR_TRACE
    #ifdef NO_ERR_HINTS
        #define ERRENTER(f) { }
        #define ERRTRACE { }
        #define ERRHINT(h) { }
        #define ERRHERE(idx) { }
    #else
        /// Treat this just like ERRHINT.
        #define ERRENTER(f) errhint = f

        /// Treat this just like ERRHERE.
        #define ERRTRACE ERRHINT(__func__)

        /// Specify this at any point for more fine-grained ADDERROR/ADDWARNING info.
        #define ERRHINT(h) errhint = h

        /// A version of ERRHINT that automatically includes the function name.
        #define ERRHERE(idx) ERRHINT(std::string(__func__)+idx)
    #endif
#else
    /// Put this at the top of any function you wish to include in the trace.
    #define ERRENTER(f) Trace_This tracethis(f)

    /// Simplified version that always puts __func__ on the trace.
    #define ERRTRACE Trace_This tracethis(__func__)

    /// Specify this at milestones within a fucntion for fine-grained ADDERROR/ADDWARNING info.
    #define ERRHINT(f_milestone) errtracer.update_milestone(f_milestone)

    /// A version of ERRHINT that automatically includes the function name.
    #define ERRHERE(f_milestone) errtracer.update_milestone(std::string(__func__)+f_milestone)
#endif


#define ADDERROR(f,e) fz::ErrQ.push(f,e)
#define ERRRETURNFALSE(f,e) { fz::ErrQ.push(f,e); return false; }
#define ERRRETURNNULL(f,e) { fz::ErrQ.push(f,e); return NULL; }
#define ADDERRPING(p) if (fz::ErrQ.pinging()) { fz::ErrQ.push(__func__,std::string("PING-")+p); }
/// Declare USE_COMPILEDPING at the top of the main program source file or in Makefile via -D
/// These are particularly useful for detecting issues in constructors, before main() begins.
#ifdef USE_COMPILEDPING
    #define COMPILEDPING(tostream,p) { tostream << p; tostream.flush(); }
#else
    #define COMPILEDPING(tostream,p) { }
#endif

#define ADDWARNING(f,e) fz::WarnQ.push(f,e)

#define ERRWARN_SUMMARY(estream) { \
    if ((fz::ErrQ.total_num()+fz::WarnQ.total_num())<1) { \
        estream << "No errors or warnings.\n"; \
    } else { \
        estream << "Logging " << fz::ErrQ.total_num() << " errors in " << fz::ErrQ.get_errfilepath() << '\n'; \
        estream << "Logging " << fz::WarnQ.total_num() << " warnings in " << fz::WarnQ.get_errfilepath() << '\n'; \
    } \
}

#define DEFAULT_ERRLOGPATH "/tmp/formalizer.core.error.ErrQ.log"
#define DEFAULT_WARNLOGPATH  "/tmp/formalizer.core.error.WarnQ.log"
#define LOG_TRUNC_MODE
#ifdef LOG_TRUNC_MODE
    #define ERRWARN_LOG_MODE (std::ofstream::out | std::ofstream::trunc)
#else
    #define ERRWARN_LOG_MODE (std::ofstream::out | std::ofstream::app)
#endif

// +----- end  : A set of useful macros -----+

namespace fz {

/**
 * Standardized exit codes for the Formalizer environment.
 * 
 * Note: If you modify this, please also modify error.py.
 */
enum exit_status_code {
    exit_ok = 0,
    exit_general_error = 1,
    exit_database_error = 2,
    exit_unknown_option = 3,
    exit_cancel = 4,
    exit_conversion_error = 5,
    exit_DIL_error = 6,
    exit_unable_to_stack_clean_exit = 7,
    exit_command_line_error = 8,
    exit_file_error = 9,
    exit_missing_parameter = 10,
    exit_missing_data = 11,
    exit_bad_config_value = 12,
    exit_resident_graph_missing = 13,
    exit_bad_request_data = 14,
    exit_communication_error = 15,
    exit_NUMENUMS ///< this one simplifies corresponding array definition, e.g. char[exit_NUMENUMS]
    };

#ifdef NO_ERR_TRACE
    /// Global variable that can be updated to give a better hint about where exactly an error occurred.
    extern std::string errhint;
#endif

struct Error_Instance {
    std::string tracehint;
    std::string func;
    std::string err;
    Error_Instance() {}
    Error_Instance(const std::string h, const std::string f, const std::string e): tracehint(h), func(f), err(e) {}
};

#ifndef NO_ERR_TRACE

/// Defining this type for easy swap-in/out of vector, deque, or other container types.
typedef std::vector<std::string> StackTrace;

/**
 * This structure is used to collaboratively maintain a stack trace for use in both
 * logged errors and exception handling.
 * 
 * Functions that wish to expose their place on the stack for easy tracing should
 * use the defined ERRSTACK_ macros to collaboratively manage this stack.
 * 
 * For more backtground information, read the card at https://trello.com/c/lNwxlrbT.
 */
struct Stack_Tracer: public StackTrace {

    /// The number of levels presently in the trace.
    StackTrace::size_type num_levels() const { return size(); }

    /// Replace the current deepest level descriptor to identify a new milestone within the function.
    void update_milestone(const std::string milestone) { if (!empty()) back() = milestone; }

    /// Return a string that combines the full current state of the stack trace.
    std::string print();

};

/// The global stack tracer variable.
extern Stack_Tracer errtracer;

class Trace_This {
protected:
    StackTrace::size_type tracelevel;

public:
    /**
     * Construct this as local variable when you enter a function. Remember
     * the level of the stack trace upon entering.
     * 
     * @param extend Label to add for the extension of the trace.
     */
    Trace_This(const std::string extend): tracelevel(errtracer.num_levels()) {
        errtracer.push_back(extend);
    }

    /**
     * Upon exiting the function, this local variable destructor is called.
     * The stack trace is returned to its level at entry.
     */
    ~Trace_This() {
        errtracer.resize(tracelevel,"trace_error"); ///< A "trace_error" is shown for any levels that had been removed too early.
    }
};

#endif // NO_ERR_TRACE

class err_configbase: public configbase {
public:
    err_configbase();

    virtual bool set_parameter(const std::string & parlabel, const std::string & parvalue);
};

/**
 * Maintain a queue of recent errors encountered in ErrQ and provide them on demand.
 * 
 * A separate queue of warnings is also maintained in WarnQ.
 * 
 * This allows you to store and later process a sequence of non-fatal errors in
 * whatever way you wish.
 */
class Errors {
    friend err_configbase;
protected:
    std::deque<Error_Instance> errq;
    std::string errfilepath;
    int numflushed;
    bool caching;
    bool ping;
    bool trace_or_time;
    time_t timecode;

    std::string print_first_timecode();
    std::string print_updated_timecode();

public:
    err_configbase config; // Notice that this is purposely configbase derived (not configurable derived).

    Errors(std::string efp);

    bool init();

    void set_errfilepath(std::string efp) { errfilepath = efp; }
    std::string get_errfilepath() const { return errfilepath; }
    void push(std::string f, std::string e);
    std::string pretty_print();
    Error_Instance pop();
    int flush();
    int num() const { return errq.size(); }
    int total_num() const { return numflushed+num(); }
    void disable_caching() { caching=false; }
    void enable_caching() { caching=true; }
    void enable_pinging() { ping=true; }
    void disable_pinging() { ping=false; }
    bool pinging() { return ping; }
    void enable_tracing() { trace_or_time = true; }
    void enable_timestamping() { trace_or_time = false; }
    void output(std::ofstream::openmode mode);
};

void Clean_Exit(int ecode);

extern Errors ErrQ;
extern Errors WarnQ;

} // namespace fz

#endif // __GRAPHTTYPES_HPP
