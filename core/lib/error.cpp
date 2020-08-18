// Copyright 2020 Randal A. Koene
// License TBD

#include <fstream>

#include "error.hpp"

namespace fz {

std::string errhint;

Errors ErrQ(DEFAULT_ERRLOGPATH);
Errors WarnQ(DEFAULT_WARNLOGPATH);


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
 * Call this to exit the program and log any remaining errors in the queue.
 * 
 * The ERRWARN_SUMMARY() macro can be used to print a summary report before
 * calling this function. After this function the queues will be empty.
 * 
 * ***NOTE: We may want to move this elsewhere, since a clean exit will need
 * ***      more than just a list of errors, and we don't even know where to
 * ***      send the output.
 */
void Clean_Exit(int ecode) {
    if (ErrQ.num()>0) {
        if (ErrQ.get_errfilepath().empty()) ErrQ.set_errfilepath(DEFAULT_ERRLOGPATH);
        std::ofstream errfile(ErrQ.get_errfilepath().c_str(),ERRWARN_LOG_MODE);
        errfile << ErrQ.pretty_print();
        errfile.close();
        ErrQ.flush();
    }
    if (WarnQ.num()>0) {
        if (WarnQ.get_errfilepath().empty()) WarnQ.set_errfilepath(DEFAULT_WARNLOGPATH);
        std::ofstream warnfile(WarnQ.get_errfilepath().c_str(),ERRWARN_LOG_MODE);
        warnfile << WarnQ.pretty_print();
        warnfile.close();
        WarnQ.flush();
    }
    exit(ecode);
}

} // namespace fz
