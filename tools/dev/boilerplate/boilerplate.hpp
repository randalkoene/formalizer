// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the boilerplate tool.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __BOILERPLATE_HPP.
 */

#ifndef __BOILERPLATE_HPP
#include "version.hpp"
#define __BOILERPLATE_HPP (__VERSION_HPP)

// core
#include "standard.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0, ///< no recognized request
    flow_cpp = 1,     ///< request: make boilerplate for C++ program
    flow_python = 2,  ///< request: make boilerplate for Python program
    flow_NUMoptions
};

class bp_configurable : public configurable {
public:
    bp_configurable(formalizer_standard_program &fsp) : configurable("boilerplate", fsp) {}
    bool set_parameter(const std::string &parlabel, const std::string &parvalue);

    //std::string something = "default";        ///< Some default.
};

struct boilerplate: public formalizer_standard_program {

    bp_configurable config;

    flow_options flowcontrol = flow_unknown;

    boilerplate();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

};

extern boilerplate bp;

bool ask_boolean_choice(std::string description, char truechar, std::string trueconfirm, std::string falseconfirm);

std::string ask_string_input(std::string description, bool allowempty);

std::string formalizer_path_dir(std::string compname, std::string category, bool iscore, bool islibrary, bool isheader);

void careful_file_create(std::string filename, std::string & filecontent);

#endif // __BOILERPLATE_HPP
