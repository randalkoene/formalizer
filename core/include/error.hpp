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

#include <string>
#include <deque>

namespace fz {

/// Global variable that can be updated to give a better hint about where exactly an error occurred.
extern std::string errhint;

/// A set of useful macros
#define ERRHINT(h) errhint = h
#define ERRHERE(idx) ERRHINT(std::string(__func__)+idx)

#define ADDERROR(f,e) ErrQ.push(f,e)
#define ERRRETURNFALSE(f,e) { ErrQ.push(f,e); return false; }
#define ERRRETURNNULL(f,e) { ErrQ.push(f,e); return NULL; }

#define ADDWARNING(f,e) WarnQ.push(f,e)

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
public:
    void push(std::string f, std::string e);
    std::string pretty_print() const;
    Error_Instance pop();
    int flush();
    int num() const { return errq.size(); }
};

void Clean_Exit(int ecode);

extern Errors ErrQ;
extern Errors WarnQ;

} // namespace fz

#endif // __GRAPHTTYPES_HPP
