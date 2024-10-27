// Copyright 20241003 Randal A. Koene
// License TBD

/**
 * This program provides access to read and write the metrics tables
 * in the Formalizer database.
 * 
 * Metrics needed for DayWiz and other purposes are likely to change.
 * For that reason, instead of specific columns for each, the data
 * within each row is presently stored as a JSON string.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZMETRICSPQ_HPP.
 */

#ifndef __FZMETRICSPQ_HPP
#include "version.hpp"
#define __FZMETRICSPQ_HPP (__VERSION_HPP)

// std
#include <string>
#include <vector>

// core
#include "config.hpp"
#include "standard.hpp"
#include "Metricspostgres.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0, /// no recognized request
    flow_read = 1,    /// request: read from table
    flow_store = 2,   /// request: store in table
    flow_delete = 3,  /// request: delete from table
    flow_count = 4,   /// request: count rows in table
    flow_NUMoptions
};


class fzmet_configurable: public configurable {
public:
    fzmet_configurable(formalizer_standard_program & fsp): configurable("fzmetricspq", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    std::vector<std::string> tables;
};

struct fzmetricspq: public formalizer_standard_program {

    fzmet_configurable config;

    flow_options flowcontrol;

    std::string index;
    std::string last_index;
    bool interval_of_indices = false;

    std::vector<std::string> datajson;

    std::string generic_table;

    std::string outfile;

    std::string format;

    Postgres_access pa;

    fzmetricspq();

    virtual void usage_hook();

    bool get_index_or_interval(const std::string& cargs);

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    /* (uncomment to include access to memory-resident Graph)
    Graph_ptr graph_ptr = nullptr;
    Graph & graph();
    */

};

extern fzmetricspq fzmet;

struct Metrics_wiztable: public Metrics_data {

    std::string idxstr;

    Metrics_wiztable(const std::string& _table): Metrics_data(_table) {}
    Metrics_wiztable(const std::string& _table, const std::string & idstr, const std::string & datastr);
    Metrics_wiztable(const std::string& _table, const fzmetricspq & _fzmet): Metrics_data(_table) { set_id(_fzmet); }

    void set_id(const fzmetricspq & _fzmet);

    virtual std::string layout() const;

    virtual std::string idstr() const;

    virtual std::string datastr() const;

    virtual std::string all_values_pqstr() const;

    virtual Metrics_data * clone() const;

    virtual bool nulldata() const { return idxstr.empty(); }

    virtual bool multidata() const { return (idxstr=="(any)"); }

    void set_pq_wildcards();
};

struct Metrics_wiztable_list: public Metrics_data_list {

    Metrics_wiztable_list(const std::string& _table): Metrics_data_list(_table) {}

    virtual bool nulldata() const { return false; }

};

#endif // __FZMETRICSPQ_HPP
