// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Client functions to make requests via the port-API of fzserverpq.
 * 
 * These functions do not include the principal Graph modification requests that
 * utilize shared memory and are provided through Graphmodify.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __APICLIENT_HPP.
 */

#ifndef __APICLIENT_HPP
#include "coreversion.hpp"
#define __APICLIENT_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
//#include <>

// core
//#include "error.hpp"


namespace fz {

bool ping_server(const std::string server_IPaddr_str, uint16_t port_number);

bool http_GET(const std::string server_IPaddr_str, uint16_t port_number, const std::string url, std::string & response_str);

bool NNLreq_update_shortlist(const std::string server_IPaddr_str, uint16_t port_number);

} // namespace fz

#endif // __APICLIENT_HPP
