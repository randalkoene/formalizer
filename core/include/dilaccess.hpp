// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Access methods for the dil2al HTML data files.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __DILACCESS_HPP.
 */

#ifndef __DILACCESS_HPP
#include "coreversion.hpp"
#define __DILACCESS_HPP (__COREVERSION_HPP)

#include <vector>

// We are trying to avoid requiring the whole dil2al.hh header in formalizer/core.
// (Alternatively, we could move dilaccess.hpp/cpp out of core.)
//#include "dil2al.hh"

class DIL_entry;
class Detailed_Items_List;

namespace fz {

/**
 * To USE and COMPILE :
 * 
 * 1. The up-to-date dil2al.hh header file must be on the include path, e.g. by
 *    adding -I$(HOME)/src/dil2al.
 * 2. The necessary functions that are declared there must be compiled in
 *    up-to-date object files made accessible, e.g. by adding them in
 *    dependencies and in the build command.
 * 
 * For an example, see the Makefile of formalizer/tools/dil2graph.
 */

Detailed_Items_List *get_DIL_Graph();

std::vector<std::string> get_DIL_Topics_File_List();

int get_DIL_entry_num_superiors(DIL_entry *e);

int get_DIL_hierarchy_num_connections(Detailed_Items_List * dil);

} // namespace fz

#endif // __DILACCESS_HPP
