// Copyright 2020 Randal A. Koene
// License TBD

#include "error.hpp"
#include <iostream>

namespace fz {

std::string errhint;

Errors ErrQ, WarnQ;


/**
 * Pushes a new error instance to the back of the queue of errors.
 * 
 * @param f name of function in which error occurred.
 * @param e description of the error.
 */
void Errors::push(std::string f, std::string e) {
    if (f.empty())
        f = "unidentified";
    if (e.empty())
        e = "unspecified";
    errq.push_back(Error_Instance(errhint, f, e));
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
    return e;
}

/**
 * Clears the queue of errors.
 * 
 * @return number of errors that were in the error queue when it was cleared.
 */
int Errors::flush() {
    int s = errq.size();
    errq.clear();
    return s;
}

/**
 * Concatenate all errors in the error queue in a single string.
 * 
 * @return string of error content in order of occurrence.
 */
std::string Errors::pretty_print() const {
    std::string estr;
    for (auto it = errq.cbegin(); it != errq.cend(); ++it) estr += '[' + it->hint + "] " + it->func + ": " + it->err + '\n';
    return estr;
}

/**
 * Clean up after yourself before you exit.
 * 
 * Call this to exit the program and show any remaining errors in the queue.
 * 
 * ***NOTE: We may want to move this elsewhere, since a clean exit will need
 * ***      more than just a list of errors, and we don't even know where to
 * ***      send the output.
 */
void Clean_Exit(int ecode) {
    if (ErrQ.num()>0) {
        std::cerr << "Errors:\n";
        std::cerr << ErrQ.pretty_print();
        ErrQ.flush();
    }
    if (WarnQ.num()>0) {
        std::cerr << "Warnings:\n";
        std::cerr << WarnQ.pretty_print();
        WarnQ.flush();
    }
    exit(ecode);
}

} // namespace fz
