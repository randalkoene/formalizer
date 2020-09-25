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

    virtual bool multisnippet() const = 0; ///< inherit this base class and define this function

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

/**
 * Read all IDs from Guide table in the database.
 * 
 * The `snippet` should specify `snippet.tablename`.
 * 
 * @param[in] snippet Data structure that clearly identifies the table in `snippet.tablename`.
 * @param[in] pa Access data with database name and schema name.
 * @param[out] ids Vector of ID strings, each of which can be parsed for its components.
 * @return True if successful.
 */
bool read_Guide_IDs_pq(Guide_snippet & snippet, Postgres_access & pa, std::vector<std::string> & ids);

/**
 * Read multiple snippets from Guide table in the database.
 * 
 * The `snippet` should specify the filtered subset by specifying those parts
 * of the ID that should be matched and leaving other parts unspecified. This
 * function is shared for various guide tables and does not know the actual
 * class of `snippet`. Therefore, the translation from Guide-specific ID
 * components and wildcards to a Postgres key wildcards needs to be done
 * before calling this function, so that `snippet.idstr()` returns the
 * right filter for "WHERE id LIKE '<something>'".
 * 
 * @param[in] snippet Data structure that clearly specifies the filter as described.
 * @param[in] pa Access data with database name and schema name.
 * @param[out] snippets Vector of snippet strings.
 * @return True if successful.
 */
bool read_Guide_multi_snippets_pq(Guide_snippet & snippet, Postgres_access & pa, std::vector<std::string> & ids, std::vector<std::string> & snippets);

} // namespace fz

#endif // __GUIDEPOSTGRES_HPP
