// Copyright 2020 Randal A. Koene
// License TBD

/** @file Guidepostgres.hpp
 * {{ header_description }}
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __GUIDEPOSTGRES_HPP.
 */

#ifndef __GUIDEPOSTGRES_HPP
#include "coreversion.hpp"
#define __GUIDEPOSTGRES_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
#include <memory>

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
    std::string snippet; ///< a string that can contain snippet text (retrieved or to store)

    Guide_snippet(std::string _tablename): tablename(_tablename) {}

    virtual std::string layout() const = 0; ///< inherit this base class and define this function

    virtual std::string idstr() const = 0; ///< inherit this base class and define this ID builder

    virtual std::string all_values_pqstr() const = 0; ///< inherit this base class and define this function

    virtual bool nullsnippet() const = 0; ///< inherit this base class and define this function

    virtual Guide_snippet * clone() const = 0; ///< define these to enable emplace_back() to Guide_snippet_ptr vector

    bool empty() const { return (tablename.empty() || nullsnippet()); }

};

typedef std::unique_ptr<Guide_snippet> Guide_snippet_ptr;

bool create_Guide_table(const active_pq & apq, const std::string guidetable, const std::string guidetablelayout);

bool store_Guide_snippet_pq(const Guide_snippet & snippet, Postgres_access & pa);

/**
 * Store multiple Guide snippets in the PostgreSQL database. Creates the table if necessary.
 * 
 * At least the first element (index 0) of the vector must contain a valid `tablename` and
 * must produce valid `layout()` information.
 * 
 * @param snippets a vector guide snippets.
 * @param pa a standard database access stucture with database name and schema name.
 * @returns true if the snippet was successfully stored in the database.
 */
bool store_Guide_multi_snippet_pq(const std::vector<Guide_snippet_ptr> & snippets, Postgres_access & pa);

bool read_Guide_snippet_pq(Guide_snippet & snippet, Postgres_access & pa);

} // namespace fz

#endif // __GUIDEPOSTGRES_HPP
