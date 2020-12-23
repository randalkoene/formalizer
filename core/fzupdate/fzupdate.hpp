// Copyright 20201126 Randal A. Koene
// License TBD

/**
 * Update the Node Schedule.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZUPDATE_HPP.
 */

#ifndef __FZUPDATE_HPP
#include "version.hpp"
#define __FZUPDATE_HPP (__VERSION_HPP)

// core
#include "config.hpp"
#include "standard.hpp"
#include "ReferenceTime.hpp"
#include "Graphtypes.hpp"
// #include "Graphaccess.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0,          /// no recognized request
    flow_update_repeating = 1, /// request: update repeating Nodes
    flow_update_variable = 2,  /// request: update variable target date Nodes
    flow_NUMoptions
};

class fzu_configurable: public configurable {
public:
    fzu_configurable(formalizer_standard_program & fsp): configurable("fzupdate", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    unsigned int chunk_minutes = 20;
    time_t chunk_seconds = 20*60;
    unsigned int map_multiplier = 3;
    unsigned long map_days = 14;
    bool warn_repeating_too_tight = true;
    bool endofday_priorities = true;
    time_t dolater_endofday = 73800;
    time_t doearlier_endofday = 68400;
    unsigned int eps_group_offset_mins = 2;
    bool update_to_earlier_allowed = true;
    unsigned long fetch_days_beyond_t_limit = 30;
    bool showmaps = true;
};


struct fzupdate: public formalizer_standard_program {
protected:
    std::string segname;
    unsigned long segsize = 0;
    Graph_modifications * graphmod_ptr = nullptr;

public:

    fzu_configurable config;

    flow_options flowcontrol;

    Graph * graph_ptr = nullptr;

    // Graph_access ga; // to include Graph or Log access support

    ReferenceTime reftime;

    time_t t_limit = 0; ///< Time to update to (inclusively), 0 means unspecified (use config.map_days).

    fzupdate();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    Graph & graph();

    void prepare_Graphmod_shared_memory(unsigned long _segsize);
    
    Graph_modifications & graphmod();

    std::string get_segname() { return segname; }

};

extern fzupdate fzu;

//typedef time_t chunks_t;
typedef size_t chunks_t;

inline chunks_t seconds_to_chunks(time_t seconds) {
    return (seconds % fzu.config.chunk_seconds) ? (seconds/fzu.config.chunk_seconds) + 1 : seconds/fzu.config.chunk_seconds;
}

#endif // __FZUPDATE_HPP
