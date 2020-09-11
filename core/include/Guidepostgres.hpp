// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ header_description }}
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __GUIDEPOSTGRES_HPP.
 */

#ifndef __GUIDEPOSTGRES_HPP
#include "coreversion.hpp"
#define __GUIDEPOSTGRES_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
//#include <>

// core
//#include "error.hpp"
#include "fzpostgres.hpp"


namespace fz {

/**
 * A generic data structure for Guide snippets.
 * 
 * Note: For an example, see fzguide.system.cpp.
 */
struct Guide_snippet {
    std::string tablename; ///< the name of the Guide table that this snippet belongs to

    Guide_snippet(std::string _tablename): tablename(_tablename) {}

    virtual std::string layout() const = 0; ///< inherit this base class and define this function

    virtual std::string all_values_pqstr() const = 0; ///< inherit this base class and define this function

    virtual bool nullsnippet() const = 0; ///< inherit this base class and define this function

    bool empty() const { return (tablename.empty() || nullsnippet()); }

};

bool create_Guide_table(PGconn* conn, const std::string schemaname, const std::string guidetable, const std::string guidetablelayout);

bool store_Guide_snippet_pq(const Guide_snippet & snippet, std::string dbname, std::string schemaname);

} // namespace fz

#endif // __GUIDEPOSTGRES_HPP
