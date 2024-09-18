// Copyright 20201126 Randal A. Koene
// License TBD

/**
 * EPS Map structures and methods used to update the Node Schedule.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __EPSMAP_HPP.
 */

#ifndef __EPSMAP_HPP
#include "version.hpp"
#define __EPSMAP_HPP (__VERSION_HPP)

#include <memory>

// core
#include "config.hpp"
#include "standard.hpp"
#include "ReferenceTime.hpp"
#include "Graphtypes.hpp"
// #include "Graphaccess.hpp"

// local
#include "fzupdate.hpp"

#define USE_MAP_FOR_MAPPING

using namespace fz;

#define UNAVAILABLE_SLOT (Node_ptr)1

constexpr unsigned int minutes_per_slot = 5;
constexpr time_t seconds_per_day = 24*60*60;
constexpr time_t seconds_per_week = 7*24*60*60;
constexpr time_t five_minutes_in_seconds = minutes_per_slot*60;
constexpr unsigned int slots_per_day = 24*60/minutes_per_slot;
constexpr unsigned int slots_per_line = 96;
constexpr unsigned int lines_per_day = slots_per_day / slots_per_line;
constexpr time_t seconds_per_line = 96*five_minutes_in_seconds;
constexpr time_t hours_per_line = 24 / lines_per_day;
constexpr unsigned int chars_per_hour = 2*60/minutes_per_slot;

constexpr unsigned int max_twochar_num = 26*26;

struct twochar_code {
    char code[3];
    twochar_code(unsigned int counter) {
        counter = counter % max_twochar_num;
        code[0] = 'A' + (counter / 26);
        code[1] = 'a' + (counter % 26);
        code[2] = '\0';
    }
    operator const char *() const {
        return code;
    }
};

// forward declaration
struct EPS_map;
class Placer;

typedef std::map<Node_ID_key, twochar_code> Node_twochar_map;

struct Node_twochar_encoder: public Node_twochar_map {
    Node_twochar_encoder() {}
    Node_twochar_encoder(const targetdate_sorted_Nodes & nodelist) { build_codebook(nodelist); }
    Node_twochar_map & map() { return *this; }
    void build_codebook(const targetdate_sorted_Nodes & nodelist);
    std::string html_link_str(const Node & node, const EPS_map & epsmap) const;
    std::string html_link_str(const Node_ptr node_ptr, const EPS_map & epsmap) const;
};

typedef std::uint8_t eps_flags_type;
struct EPS_flags {
    enum eps_mask : eps_flags_type {
        overlap              = 0b0000'0001,
        insufficient         = 0b0000'0010,
        treatgroupable       = 0b0000'0100,
        exact                = 0b0000'1000,
        fixed                = 0b0001'0000,
        epsgroupmember       = 0b0010'0000,
        periodiclessthanyear = 0b0100'0000
    };
    eps_flags_type epsflags;
    EPS_flags() : epsflags(0) {}
    eps_flags_type get_EPS_flags() { return epsflags; }
    void clear() { epsflags = 0; }
    void set_EPS_flags(eps_flags_type _epsflags) { epsflags = _epsflags; }
    bool None() const { return epsflags == 0; }
    void set_overlap() { epsflags |= eps_mask::overlap; }
    void set_insufficient() { epsflags |= eps_mask::insufficient; }
    void set_treatgroupable() { epsflags |= eps_mask::treatgroupable; }
    void set_exact() { epsflags |= eps_mask::exact; }
    void set_fixed() { epsflags |= eps_mask::fixed; }
    void set_epsgroupmember() { epsflags |= eps_mask::epsgroupmember; }
    void set_periodiclessthanyear() { epsflags |= eps_mask::periodiclessthanyear; }
    bool EPS_overlap() const { return epsflags & eps_mask::overlap; }
    bool EPS_insufficient() const { return epsflags & eps_mask::insufficient; }
    bool EPS_treatgroupable() const { return epsflags & eps_mask::treatgroupable; }
    bool EPS_exact() const { return epsflags & eps_mask::exact; }
    bool EPS_fixed() const { return epsflags & eps_mask::fixed; }
    bool EPS_epsgroupmember() const { return epsflags & eps_mask::epsgroupmember; }
    bool EPS_periodiclessthanyear() const { return epsflags & eps_mask::periodiclessthanyear; }
};

struct eps_data {
    chunks_t chunks_req = 0;
    EPS_flags epsflags;
    time_t t_eps = RTt_maxtime;
};

typedef std::vector<eps_data> eps_data_vec_t; // *** alternatively, could index by Node_ID_key and get rid of node_vector_index.

/**
 * Initialized to one entry for each Node in the incomplete_repeating map.
 * Initialized with the number of chunks required for each and RTt_maxtime.
 */
struct eps_data_vec: public eps_data_vec_t {
    eps_data_vec(const targetdate_sorted_Nodes & _incomplete_repeating);
    void updvar_chunks_required(const targetdate_sorted_Nodes & nodelist);
    size_t updvar_total_chunks_required_nonperiodic(const targetdate_sorted_Nodes & nodelist);
};

typedef std::map<time_t, Node_ptr> eps_slots_map_t;

struct EPS_map {
    unsigned long num_days;
    time_t starttime;
    time_t firstdaystart;
    time_t first_slot_td; ///< Even the first usable 5 minute slot needs at least a 5 minute interval from starttime.
    //size_t firstday_slotspassed = 0;
    int slots_per_chunk;
    time_t previous_group_td = -1;
    time_t t_beyond; ///< Will be initialized to the time right after the last slot, then incremented by pack_interval_beyond if in back_moveable mode.

    bool usechain = false;
    std::vector<std::unique_ptr<Placer>> placer_chain;
    bool test_fail_full = false;
    bool utd_all_placed = false;

    targetdate_sorted_Nodes & nodelist;
    eps_data_vec_t & epsdata;

    std::map<Node_ID_key, size_t> node_vector_index;

    eps_slots_map_t slots;
    eps_slots_map_t::iterator next_slot;

    std::string day_separator;

    EPS_map(time_t _t, unsigned long days_in_map, targetdate_sorted_Nodes & _incomplete_repeating, eps_data_vec_t & _epsdata, bool _testfailfull);

    size_t bytes_estimate();

    void init_next_slot() { next_slot = slots.begin(); }

    void process_chain(std::string& chain);
    void prepare_day_separator();
    void add_map_code(time_t t, const char * code_cstr, std::vector<std::string> & maphtmlvec) const;
    std::vector<std::string> html(const Node_twochar_encoder & codebook);
    std::string show();

    /**
     * Reserve `chunks_req` in five minute granularity, immediately preceding
     * the exact target date.
     * @param n_ptr Pointer to Node for which to allocate time.
     * @param chunks_req Number of chunks required.
     * @param td Node target date.
     * @return True if any of the slots are already occupied (exact target date
     *         Nodes with overlapping intervals).
     */
    bool reserve_exact(Node_ptr n_ptr, int chunks_req, time_t td);

    /**
     * Reserve `chunks_req` in five minute granularity, immediately preceding
     * the fixed target date. Reserves nearest available slots from the target
     * date on down.
     * @param n_ptr Pointer to Node for which to allocate time.
     * @param chunks_req Number of chunks required.
     * @param td Node target date.
     * @return True if there are insufficient slots available.
     */
    bool reserve_fixed(Node_ptr n_ptr, int chunks_req, time_t td);

    /**
     * Adjust the new target date to take into account regular end of day
     * target times for prioritized activities.
     * 
     * @param td_raw The new target date that was obtained, without adjustments.
     * @return A target date that may be shifted to align with a time of day
     *         specified for activities with a particular priority type.
     */
    time_t end_of_day_adjusted(time_t td_raw);

    /**
     * Reserve `chunks_req` in five minute granularity from the earliest
     * available slot onward.
     * @param n_ptr Pointer to Node for which to allocate time.
     * @param chunks_req Number of chunks required.
     * @return Corresponding suggested target date taking into account TD preferences,
     *         or -1 if there are insufficient slots available.
     */
    time_t reserve(Node_ptr n_ptr, int chunks_req);

    void place_exact();

    /**
     * Note: In dil2al, this included collecting info about how a target date was obtained, and that info could be
     * fairly diverse. Now, in the Formalizer 2.x, target dates are set at the Node, so you only have two
     * cases: Either the targetdate was specified at the Node, or it was inherited (propagated). It is now
     * solidly the td_property that determines what is done, and the 'inherit' property forces inheritance
     * (much as 'unspecified+fixed' used to). For more about this, see the documentation of target date
     * properties in https://docs.google.com/document/d/1rYPFgzFgjkF1xGx3uABiXiaDR5sfmOzqYQRqSntcyyY/edit#heading=h.td3aptmw2f5.
     */
    void place_fixed();

    void group_and_place_movable(bool include_UTD = true);

    targetdate_sorted_Nodes get_eps_update_nodes();
    targetdate_sorted_Nodes get_epsvtd_and_utd_update_nodes();

};

/**
 * Chainable objects defining Node placement rules.
 */
class Placer {
protected:
    EPS_map& updvar_map;
public:
    Placer(EPS_map& _updvar_map): updvar_map(_updvar_map) {}
    virtual void place() = 0;
};

class VTD_Placer: public Placer {
public:
    VTD_Placer(EPS_map& _updvar_map): Placer(_updvar_map) {}
    virtual void place();
};

class Uncategorized_UTD_Placer: public Placer {
public:
    Uncategorized_UTD_Placer(EPS_map& _updvar_map): Placer(_updvar_map) {}
    virtual void place();
};

#endif // __EPSMAP_HPP
