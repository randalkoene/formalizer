// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares error handling classes and functions for use with core
 * Formalizer C++ code.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __GRAPHTYPES_HPP.
 */

#ifndef __ERROR_HPP
#include "coreversion.hpp"
#define __ERROR_HPP (__COREVERSION_HPP)

#include <fstream>
#include <string>
#include <deque>

/// A set of useful macros
#define ERRHINT(h) errhint = h
#define ERRHERE(idx) ERRHINT(std::string(__func__)+idx)

#define ADDERROR(f,e) fz::ErrQ.push(f,e)
#define ERRRETURNFALSE(f,e) { fz::ErrQ.push(f,e); return false; }
#define ERRRETURNNULL(f,e) { fz::ErrQ.push(f,e); return NULL; }
#define ADDERRPING(p) if (fz::ErrQ.pinging()) { fz::ErrQ.push(__func__,std::string("PING-")+p); }

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

namespace fz {

/// Global variable that can be updated to give a better hint about where exactly an error occurred.
extern std::string errhint;

struct Error_Instance {
    std::string hint;
    std::string func;
    std::string err;
    Error_Instance(std::string h, std::string f, std::string e): hint(h), func(f), err(e) {}
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
protected:
    std::deque<Error_Instance> errq;
    std::string errfilepath;
    int numflushed;
    bool caching;
    bool ping;
public:
    Errors(std::string efp): errfilepath(efp), numflushed(0), caching(true), ping(false) {}
    void set_errfilepath(std::string efp) { errfilepath = efp; }
    std::string get_errfilepath() const { return errfilepath; }
    void push(std::string f, std::string e);
    std::string pretty_print() const;
    Error_Instance pop();
    int flush();
    int num() const { return errq.size(); }
    int total_num() const { return numflushed+num(); }
    void disable_caching() { caching=false; }
    void enable_caching() { caching=true; }
    void enable_pinging() { ping=true; }
    void disable_pinging() { ping=false; }
    bool pinging() { return ping; }
    void output(std::ofstream::openmode mode);
};

void Clean_Exit(int ecode);

extern Errors ErrQ;
extern Errors WarnQ;

} // namespace fz

#endif // __GRAPHTTYPES_HPP
