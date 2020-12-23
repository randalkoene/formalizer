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

constexpr time_t seconds_per_day = 24*60*60;
constexpr time_t five_minutes_in_seconds = 5*60;
constexpr unsigned int slots_per_day = 24*60/5;
constexpr unsigned int slots_per_line = 96;
constexpr unsigned int lines_per_day = slots_per_day / slots_per_line;
constexpr time_t seconds_per_line = 96*five_minutes_in_seconds;
constexpr time_t hours_per_line = 24 / lines_per_day;
constexpr unsigned int chars_per_hour = 2*60/5;

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

#ifndef USE_MAP_FOR_MAPPING

struct EPS_map_day {
    //Node_ptr fivemin_slot[slots_per_day]; // = { nullptr };
    std::vector<Node_ptr> fivemin_slot;
    time_t t_daystart;
    EPS_map_day(time_t t_start) : t_daystart(t_start) {
        for (unsigned int i=0; i<slots_per_day; ++i) {
            fivemin_slot.emplace_back(nullptr);
            //fivemin_slot[i] = nullptr;
        }
    }
    void init(time_t t_day) {
        t_daystart = t_day;
        //fivemin_slot = { nullptr }; // *** do we need this again?
    }

    std::vector<std::string> html(const Node_twochar_encoder & codebook) const;

    /**
     * Reserver 5 min slots immediately preceding exact target date.
     * @param n_ptr Pointer to Node for which to allocate slots.
     * @param slots Slots needed, returning any that did not fit into the day.
     * @param td Node target date (td<0 means used time from end of day).
     * @return True if any map slots were already occupied.
     */
    bool reserve_exact(Node_ptr n_ptr, int &slots, time_t td);
    /**
     * Reserve 5 min slots preceding the target date.
     * @param n_ptr Pointer to Node for which to allocate slots.
     * @param slots Slots needed, returning any that did not fit into the day.
     * @param td Node target date (td<0 means used time from end of day).
     * @return 0 if all goes well (even if slots remain to be reserved), 1 if there
     *         is a td value problem, 1 if reserved space from earlier iteration of
     *         the same Node was encountered (repeating Nodes scheduled too tightly).
     */
    int reserve_fixed(Node_ptr n_ptr, int & slots, time_t td);
    /**
     * Reserves 5 min slots advancing from the earliest available slot onward.
     * @param n_ptr Pointer to Node for which to allocate slots.
     * @param slots_req Slots needed, returning any that did not fit into the day.
     * @return Target date immediately following the last slot allocated or immediately
     *         following the end of the day.
     */
    time_t reserve(Node_ptr n_ptr, int & slots_req);
};

#else // USE_MAP_FOR_MAPPING

struct eps_data {
    chunks_t chunks_req = 0;
    EPS_flags epsflags;
    time_t t_eps = RTt_maxtime;
};
typedef std::vector<eps_data> eps_data_vec_t; // *** alternatively, could index by Node_ID_key and get rid of node_vector_index.
struct eps_data_vec: public eps_data_vec_t {
    eps_data_vec(const targetdate_sorted_Nodes & _incomplete_repeating);
    void updvar_chunks_required(const targetdate_sorted_Nodes & nodelist);
    size_t updvar_total_chunks_required_nonperiodic(const targetdate_sorted_Nodes & nodelist);
};

struct EPS_map {
    unsigned long num_days;
    time_t starttime;
    time_t firstdaystart;
    time_t first_slot_td; ///< Even the first usable 5 minute slot needs at least a 5 minute interval from starttime.
    //size_t firstday_slotspassed = 0;
    int slots_per_chunk;
    time_t previous_group_td = -1;

    targetdate_sorted_Nodes & nodelist;
    eps_data_vec_t & epsdata;
    //std::vector<chunks_t> & chunks_req;
    //std::vector<EPS_flags> & epsflags_vec;
    //std::vector<time_t> & t_eps;

    std::map<Node_ID_key, size_t> node_vector_index;

    std::map<time_t, Node_ptr> slots;

    std::string day_separator;

    EPS_map(time_t _t, unsigned long days_in_map, targetdate_sorted_Nodes & _incomplete_repeating, eps_data_vec_t & _epsdata);
        //std::vector<chunks_t> & _chunks_req, std::vector<EPS_flags> & _epsflags_vec, std::vector<time_t> & _t_eps);

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

    void group_and_place_movable();

    targetdate_sorted_Nodes get_eps_update_nodes();

};

#endif // USE_MAP_FOR_MAPPING

#ifndef USE_MAP_FOR_MAPPING

struct EPS_map {
    unsigned long num_days;
    time_t starttime;
    time_t firstdaystart;
    std::vector<EPS_map_day> days;
    int slots_per_chunk;
    time_t previous_group_td = -1;

    targetdate_sorted_Nodes & nodelist;
    std::vector<chunks_t> & chunks_req;
    std::vector<EPS_flags> & epsflags_vec;
    std::vector<time_t> & t_eps;

    EPS_map(time_t t, unsigned long days_in_map, targetdate_sorted_Nodes & _incomplete_repeating,
    std::vector<chunks_t> & _chunks_req, std::vector<EPS_flags> & _epsflags_vec, std::vector<time_t> & _t_eps);

    std::vector<std::string> html(const Node_twochar_encoder & codebook) const;
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

    void place_exact();

    /// Find the day that contains the target date.
    int find_dayTD(time_t td);

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
     * Note: In dil2al, this included collecting info about how a target date was obtained, and that info could be
     * fairly diverse. Now, in the Formalizer 2.x, target dates are set at the Node, so you only have two
     * cases: Either the targetdate was specified at the Node, or it was inherited (propagated). It is now
     * solidly the td_property that determines what is done, and the 'inherit' property forces inheritance
     * (much as 'unspecified+fixed' used to). For more about this, see the documentation of target date
     * properties in https://docs.google.com/document/d/1rYPFgzFgjkF1xGx3uABiXiaDR5sfmOzqYQRqSntcyyY/edit#heading=h.td3aptmw2f5.
     */
    void place_fixed();

    /**
     * Reserve `chunks_req` in five minute granularity from the earliest
     * available slot onward.
     * @param n_ptr Pointer to Node for which to allocate time.
     * @param chunks_req Number of chunks required.
     * @return Corresponding suggested target date taking into account TD preferences,
     *         or -1 if there are insufficient slots available.
     */
    time_t reserve(Node_ptr n_ptr, int chunks_req);

    void group_and_place_movable();

    targetdate_sorted_Nodes get_eps_update_nodes();
};

#endif // USE_MAP_FOR_MAPPING

#endif // __EPSMAP_HPP
