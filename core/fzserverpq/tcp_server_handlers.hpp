// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the TCP port direct API server handler
 * functions of the fzserverpq program.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __TCP_SERVER_HANDLERS_HPP.
 */

#ifndef __TCP_SERVER_HANDLERS_HPP
#include "version.hpp"
#define __TCP_SERVER_HANDLERS_HPP (__VERSION_HPP)

// std
#include <string>

// core
#include "Graphmodify.hpp"

using namespace fz;

enum NNL_noarg_cmd {
    NNLnoargcmd_unknown = 0,
    NNLnoargcmd_reload = 1,
    NNLnoargcmd_shortlist = 2,
    NNLnoargcmd_NUM
};

enum NNL_underscore_cmd {
    NNLuscrcmd_unknown = 0,
    NNLuscrcmd_set = 1,
    NNLuscrcmd_select = 2,
    NNLuscrcmd_NUM  
};

enum NNL_list_cmd {
    NNLlistcmd_unknown = 0,
    NNLlistcmd_add = 1,
    NNLlistcmd_remove = 2,
    NNLlistcmd_copy = 3,
    NNLlistcmd_delete = 4,
    NNLlistcmd_NUM
};

#endif // __TCP_SERVER_HANDLERS_HPP