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

using namespace fz;

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

struct EPS_map_day {
    Node_ptr fivemin_slot[288] = { nullptr };
    time_t t_daystart;
    EPS_map_day(time_t t_start) : t_daystart(t_start) {}
    void init(time_t t_day) {
        t_daystart = t_day;
        //fivemin_slot = { nullptr }; // *** do we need this again?
    }
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
     * @param slots Slots needed, returning any that did not fit into the day.
     * @return Target date immediately following the last slot allocated or immediately
     *         following the end of the day.
     */
    time_t reserve(Node_ptr n_ptr, int & slots);
};

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

#endif // __EPSMAP_HPP
