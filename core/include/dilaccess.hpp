// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Access methods for the dil2al HTML data files.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __DILACCESS_HPP.
 */

#ifndef __DILACCESS_HPP
#include "coreversion.hpp"
#define __DILACCESS_HPP (__VERSION_HPP)

#include "dil2al.hh"

#include "error.hpp"
#include "general.hpp"

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

/**
 * Load the whole Detailed_Items_List and return a pointer to its data structure.
 * 
 * @return pointer to Detailed_Items_List object, or NULL if unsuccessful.
 */
Detailed_Items_List *get_DIL_Graph() {
    Detailed_Items_List *dil = new Detailed_Items_List;
    if (!dil)
        ERRRETURNNULL(__func__, "unable to initialize Detailed_Items_List");

    if (dil->Get_All_DIL_ID_File_Parameters() < 0) {
        delete dil;
        ERRRETURNNULL(__func__, "unable to obtain parameters from DIL-by-ID file");
    }

    if (!dil->list.head()) {
        delete dil;
        ERRRETURNNULL(__func__, "no DIL entries found");
    }

    if (dil->Get_All_Topical_DIL_Parameters(true) < 0) {
        delete dil;
        ERRRETURNNULL(__func__, "unable to obtain DIL entry parameters from topical DIL files");
    }

    return dil;
}

/**
 * Find the actual set of Topical DIL Files that exist in the dil2al base directory.
 * 
 * This function requires that `basedir` is valid, otherwise the shellcmd2str()
 * call will throw a runtime_error. This function does not distinguish between
 * actual files and symlinks (see detect_DIL_Topics_Symlinks()).
 * 
 * Note: Perhaps this function belongs in the utilities.cc library of dil2al.
 * 
 * @return a vector containing the absolute file name strings of each Topical DIL File.
 */
std::vector<std::string> get_DIL_Topics_File_List() {
  std::string diltopicfiles = shellcmd2str("grep -l '^<!-- dil2al: DIL begin -->' " + std::string(basedir.chars()) + RELLISTSDIR "*.html");
  return split(diltopicfiles,'\n');
}

/**
 * The number of connections from a DIL entry to Superiors.
 * 
 * Note: Perhaps this function belongs in the utilities.cc library of dil2al.
 * 
 * @param e pointer to DIL_entry.
 * @return count of number of connections to Superiors.
 */
int get_DIL_entry_num_superiors(DIL_entry *e) {
    if (!e)
        return 0;
    if (!e->parameters)
        return 0;
    return e->parameters->projects.length();
}

/**
 * The number of connections in a DIL hierarchy.
 * 
 * Note: Perhaps this function belongs in the utilties.cc library of dil2al.
 * 
 * @param dil pointer to DIL hierarchy (Detailed_Items_List).
 * @return count of total number of connections in the hierarchy.
 */
int get_DIL_hierarchy_num_connections(Detailed_Items_List * dil) {
  if (!dil) return 0;
  int num = 0;
  PLL_LOOP_FORWARD(DIL_entry, dil->list.head(), 1) {
    num += get_DIL_entry_num_superiors(e);
  }
  return num;
}

} // namespace fz

#endif // __DILACCESS_HPP
