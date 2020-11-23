// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the shared memory server handler
 * functions of the fzserverpq program.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __SHM_SERVER_HANDLERS_HPP.
 */

#ifndef __SHM_SERVER_HANDLERS_HPP
#include "version.hpp"
#define __SHM_SERVER_HANDLERS_HPP (__VERSION_HPP)

// std
#include <string>

// core
#include "Graphmodify.hpp"

using namespace fz;

/**
 * Ensure that all of the requests contain valid data.
 * 
 * Otherwise reject the whole stack to avoid partial changes.
 * For more information, see https://trello.com/c/FQximby2/174-fzgraphedit-adding-new-nodes-to-the-graph-with-initial-edges#comment-5f8faf243d74b8364fac7739.
 * 
 * @param graphmod A requests stack with one or more requests.
 * @param segname Shared segment name where any error response data should be delivered.
 * @return True if everything is valid, false if the stack should be rejected.
 */
bool request_stack_valid(Graph_modifications & graphmod, std::string segname);

/**
 * Find the shared memory segment with the indicated request
 * stack, then process that stack by first carrying out
 * validity checks on all stack elements and then responding
 * to each request.
 * 
 * @param segname The shared memory segment name provided for the request stack.
 * @return 
 */
bool handle_request_stack(std::string segname);

#endif // SHM_SERVER_HANDLERS
