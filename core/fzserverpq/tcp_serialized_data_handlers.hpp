// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the TCP port serialized data API
 * handler functions of the fzserverpq program.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __TCP_SERIALIZED_DATA_HANDLERS_HPP.
 */

#ifndef __TCP_SERIALIZED_DATA_HANDLERS_HPP
#include "version.hpp"
#define __TCP_SERIALIZED_DATA_HANDLERS_HPP (__VERSION_HPP)

// std
#include <string>

// core
//#include "Graphmodify.hpp"

//using namespace fz;

/**
 * Entry point for the interpretation of serialized data protocol API requests.
 * 
 * @param new_socket Unix domain socket (IPC socket) number.
 * @param request_str String containing one or more valid serial data protocol API requests.
 */
void handle_serialized_data_request(int new_socket, const std::string & request_str);

#endif // __TCP_SERIALIZED_DATA_HANDLERS_HPP
