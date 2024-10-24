// Copyright 2024 Randal A. Koene
// License TBD

/**
 * This database access library needs to be able to:
 * - Create metrics tables used in DayWiz.
 * - Read data for a specific day from a metrics table.
 * - Write data for a specific day to a metrics table.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __METRICSPOSTGRES_HPP.
 */

#ifndef __METRICSPOSTGRES_HPP
#include "coreversion.hpp"
#define __METRICSPOSTGRES_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
#include <memory>
#include <vector>

// core
//#include "error.hpp"
#include "fzpostgres.hpp"

namespace fz {

/**
 * A generic data structure for Metrics data.
 */
struct Metrics_data {
    std::string tablename; ///< the name of the Guide table that this data belongs to
    std::string data; ///< a string that can contain metrics data (retrieved or to store)

    Metrics_data(std::string _tablename): tablename(_tablename) {}

    virtual std::string layout() const = 0; ///< inherit this base class and define this function

    virtual std::string idstr() const = 0; ///< inherit this base class and define this ID builder

    virtual std::string datastr() const = 0;

    virtual std::string all_values_pqstr() const = 0; ///< inherit this base class and define this function

    virtual bool nulldata() const = 0; ///< inherit this base class and define this function

    virtual bool multidata() const = 0; ///< inherit this base class and define this function

    virtual Metrics_data * clone() const = 0; ///< define these to enable emplace_back() to Metrics_data_ptr vector

    bool empty() const { return (tablename.empty() || nulldata()); }

};

typedef std::unique_ptr<Metrics_data> Metrics_data_ptr;

/**
 * A generic data structure for lists of Metrics data.
 */
struct Metrics_data_list {
    std::string tablename;
    std::vector<std::string> id_list;
    std::vector<std::string> data_list;

    Metrics_data_list(const std::string& _tablename): tablename(_tablename) {}

    virtual bool nulldata() const = 0;
    bool empty() const { return (tablename.empty() || nulldata()); }
};

bool create_Metrics_table(const active_pq& apq, const std::string& metricstable, const std::string& metricstablelayout);

bool store_Metrics_data_pq(const Metrics_data & data, Postgres_access & pa);

bool read_Metrics_data_pq(Metrics_data & data, Postgres_access & pa);

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
bool read_Metrics_IDs_pq(Metrics_data & data, Postgres_access & pa, std::vector<std::string> & ids);

/**
 * Read IDs and data from Metrics table in the database from a starting
 * ID to an ending ID (inclusive).
 * 
 * The `data` should specify `data.tablename`.
 * 
 * @param[in] datalist Data structure that clearly identifies the table in `datalist.tablename`.
 * @param[in] pa Access data with database name and schema name.
 * @param[in] id_start First ID in interval.
 * @param[in] id_end Last ID in interval.
 * @return True if successful.
 */
bool read_Metrics_IDs_and_data_interval_pq(Metrics_data_list & datalist, Postgres_access & pa, const std::string& id_start, const std::string& id_end);

bool delete_Metrics_data_pq(const Metrics_data & data, Postgres_access & pa);

bool delete_Metrics_table_pq(const std::string& tablename, Postgres_access & pa);

bool count_Metrics_table_pq(Metrics_data & data, Postgres_access & pa);

} // namespace fz

#endif // __METRICSPOSTGRES_HPP
