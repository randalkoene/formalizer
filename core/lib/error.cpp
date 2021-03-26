// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <iostream>
#include <fstream>

// core
#include "ReferenceTime.hpp"
#include "TimeStamp.hpp"
#include "config.hpp"
#include "error.hpp"

namespace fz {

bool mute_error = false;

Errors ErrQ(DEFAULT_ERRLOGPATH);
Errors WarnQ(DEFAULT_WARNLOGPATH);

#ifdef NO_ERR_TRACE

std::string errhint;

#else

Stack_Tracer errtracer;

/// Compose the full stack trace into one string. (Brackets are added by Errors::pretty_print().)
std::string Stack_Tracer::print() {
    std::string tracestr;
    tracestr.reserve(errtracer.num_levels()*20);
    for (StackTrace::size_type i = 0; i < errtracer.num_levels(); ++i) {
        if (i!=0)
            tracestr += ':';
        tracestr += errtracer[i];
    }
    return tracestr;
}

#endif

err_configbase::err_configbase(): configbase("error") {
}

bool err_configbase::set_parameter(const std::string &parlabel, const std::string &parvalue) {
    CONFIG_TEST_AND_SET_PAR(ErrQ.errfilepath, "errlogpath", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(WarnQ.errfilepath, "warnlogpath", parlabel, parvalue);
    CONFIG_TEST_AND_SET_FLAG(ErrQ.enable_caching, ErrQ.disable_caching, "errcaching", parlabel, parvalue);
    CONFIG_TEST_AND_SET_FLAG(ErrQ.enable_caching, ErrQ.disable_caching, "errcaching", parlabel, parvalue);
    CONFIG_PAR_NOT_FOUND(parlabel);
}

Errors::Errors(std::string efp): errfilepath(efp), numflushed(0), caching(true), ping(false), trace_or_time(true) {
    timecode = ActualTime();
}

// See https://trello.com/c/4B7x2kif/102-configuration-files#comment-5f65941fc72f72665b0b52cf.
bool Errors::init() {
    if (!config.load()) {
        const std::string configerrstr("Errors during "+config.get_configfile()+" processing");
        VERBOSEERR(configerrstr+'\n');
        ERRRETURNFALSE(__func__,configerrstr);
    }

    return true;
}

/**
 * Pushes a new error instance to the back of the queue of errors.
 * 
 * @param f name of function in which error occurred.
 * @param e description of the error.
 */
void Errors::push(std::string f, std::string e) {
    if (mute_error)
        return;
    if (f.empty())
        f = "unidentified";
    if (e.empty())
        e = "unspecified";

#ifdef NO_ERR_TRACE

    if (trace_or_time) {
        // push the hint on with the function and error message
        errq.push_back(Error_Instance(errhint, f, e));
    } else {
        errq.push_back(TimeStampYmdHM(ActualTime()), f, e);
    }

#else

    if (trace_or_time) {
        // push the stack trace on with the function and error message
        errq.emplace_back(Error_Instance(errtracer.print(),f,e));
    } else {
        errq.emplace_back(Error_Instance(TimeStampYmdHM(ActualTime()), f, e));
    }

#endif

    if (!caching) {
        if (numflushed<1)
            output(ERRWARN_LOG_MODE);
        else
            output(std::ofstream::out | std::ofstream::app);
    }
}

/**
 * Pops the oldest error from the front of the queue of errors.
 * 
 * @return content of the oldest error instance or empty instance if the queue was empty.
 */
Error_Instance Errors::pop() {
    if (errq.size() < 1)
        return Error_Instance("", "", "");
    Error_Instance e(errq.front());
    errq.pop_front();
    ++numflushed;
    return e;
}

/**
 * Clears the queue of errors.
 * 
 * @return number of errors that were in the error queue when it was cleared.
 */
int Errors::flush() {
    int s = errq.size();
    numflushed += s;
    errq.clear();
    return s;
}

std::string Errors::print_first_timecode() {
    return "Time Code at START OF PROGRAM: "+TimeStampYmdHM(timecode)+'\n';
}

std::string Errors::print_updated_timecode() {
    return "Time Code UPDATE: "+TimeStampYmdHM(timecode)+'\n';
}

/**
 * Concatenate all errors in the error queue in a single string.
 * 
 * @return string of error content in order of occurrence.
 */
std::string Errors::pretty_print() {
    std::string estr;

    // If this is the first output, or if a day has passed since the last time code then include one.
    if (num() > 0) {
        if (numflushed < 1) {
            estr += print_first_timecode();
        }
        time_t t = ActualTime();
        if (t > (timecode+(24*60*60))) {
            timecode = t;
            estr += print_updated_timecode();
        }
    }

    for (auto it = errq.cbegin(); it != errq.cend(); ++it)
        estr += '[' + it->tracehint + "] " + it->func + ": " + it->err + '\n';

    return estr;
}

/**
 * Stream error messages to file and flush the queue.
 * 
 * Output modes are:
 *   (std::ofstream::out | std::ofstream::trunc)
 *   (std::ofstream::out | std::ofstream::app)
 * 
 * @param mode output mode.
 */
void Errors::output(std::ofstream::openmode mode) {
    if (num()>0) {

        if (get_errfilepath().empty()) set_errfilepath(DEFAULT_ERRLOGPATH);

        if (get_errfilepath() == "STDOUT") {
            std::cout << pretty_print();
            std::cout.flush();

        } else {
            std::ofstream errfile(get_errfilepath().c_str(),mode);
            errfile << pretty_print();
            errfile.close();
            flush();
        }
    }
}

/**
 * Clean up after yourself before you exit.
 * 
 * Call this to exit the program and log any remaining errors in the queue.
 * 
 * The ERRWARN_SUMMARY() macro can be used to print a summary report before
 * calling this function. After this function the queues will be empty.
 * 
 * Note: If you are using standard.hpp (which every Formalizer program
 *       should) then do not use this function. The correct exit processes
 *       will be set up by member functions and initialization of the
 *       `fz::standard` object instead. In your program, you can then
 *       simply call `exit(error_code)` or `standard.exit(error_code)`.
 *       Both will do the same thing.
 */
void Clean_Exit(int ecode) {
    ErrQ.output(ERRWARN_LOG_MODE);
    WarnQ.output(ERRWARN_LOG_MODE);
    exit(ecode);
}

/// A function that combines ADDERROR and VERBOSEERR without exit. Always returns false
bool standard_error(std::string error_message, const char * problem__func__) {
    ADDERROR(problem__func__, error_message);
    VERBOSEERR(error_message+'\n');
    return false;
}

/// A function that combines ADDWARNING and VERYVERBOSEOUT without exit, and without return value.
void standard_warning(std::string warn_message, const char * warning__func__) {
    ADDWARNING(warning__func__, warn_message);
    VERYVERBOSEOUT(warn_message+'\n');
}

// +----- begin: EXPERIMENTAL -----+
#ifdef INCLUDE_EXPERIMENTAL

/**
 * This experimental code is an attempt to try to (briefly) catch segfault,
 * just to flush error queues.
 * 
 * See the card at: https://trello.com/c/G6OPJKeq
 */

void segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
    ErrQ.output(ERRWARN_LOG_MODE);
    //printf("Caught segfault at address %p\n", si->si_addr); //*** using printf is actually dangerous
    exit(0); // should use a better code, or can we hand it back to the regular handler?
}

void setup_segfault_handler() {
    struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_sigaction;
    sa.sa_flags   = SA_SIGINFO;

    sigaction(SIGSEGV, &sa, NULL);

}

void test_segfault() {
    int * foo = NULL;
    *foo = 1;
}
#endif // INCLUDE_EXPERIMENTAL

// +----- end  : EXPERIMENTAL -----+


} // namespace fz
