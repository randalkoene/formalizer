// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Inspect memory resident servers.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZSERVER_INFO_HPP.
 */

#ifndef __FZSERVER_INFO_HPP
#include "version.hpp"
#define __FZSERVER_INFO_HPP (__VERSION_HPP)

// core
#include "config.hpp"
#include "standard.hpp"
#include "Graphtypes.hpp"
// #include "Graphaccess.hpp"

/**
 * FORMALIZER_ROOT must be supplied by -D during make.
 * For example: FORMALIZERROOT=-DFORMALIZER_ROOT=\"$(HOME)/.formalizer\"
 */
#ifndef FORMALIZER_ROOT
    #define FORMALIZER_ROOT this_breaks
#endif

using namespace fz;

enum flow_options {
    flow_unknown = 0,       /// no recognized request
    flow_graph_server = 1,  /// request: graph server status
    flow_ping_server = 2,   /// request: ping the server
    flow_shared_memory = 3, /// request: POSIX named shared memory blocks
    flow_NUMoptions
};

enum output_format_specifier {
    output_txt = 0,
    output_html = 1,
    output_json = 2,
    output_csv = 3,
    output_raw = 4,
    output_NUMENUMS
};

struct POSIX_shm_data {
    std::string name;
    std::uintmax_t size;
    POSIX_shm_data(const std::string _name, std::uintmax_t _size) : name(_name), size(_size) {}
};
typedef std::vector<POSIX_shm_data> POSIX_shm_data_vec;

class fzsi_configurable : public configurable {
public:
    fzsi_configurable(formalizer_standard_program &fsp) : configurable("fzserver-info", fsp) {}
    bool set_parameter(const std::string &parlabel, const std::string &parvalue);

    std::string info_out_path = "STDOUT"; ///< path to send info output to (STDOUT for standard output)
    uint16_t port_number = 8090;          ///< the expected Graph server port number
};

struct fzserver_info: public formalizer_standard_program {

    fzsi_configurable config;

    flow_options flowcontrol;

    output_format_specifier output_format; /// the format used to deliver query results

    // Graph_access ga; // to include Graph or Log access support

    static constexpr const char * lockfilepath = FORMALIZER_ROOT "/.fzserverpq.lock";

    Graph * graph_ptr = nullptr;

    fzserver_info();

    virtual void usage_hook();

    bool set_output_format(const std::string & cargs);

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    Graph & graph();

};

extern fzserver_info fzsi;

POSIX_shm_data_vec named_POSIX_shared_memory_blocks();

#endif // __FZSERVER_INFO_HPP
