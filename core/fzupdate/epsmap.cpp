// Copyright 20201126 Randal A. Koene
// License TBD

/**
 * EPS Map structures and methods used to update the Node Schedule.
 * 
 * {{ long_description }}
 * 
 * For more about this, see {{ doc_reference }}.
 */

// std

// core
#include "error.hpp"
#include "standard.hpp"
#include "ReferenceTime.hpp"
#include "Graphinfo.hpp"
#include "Graphmodify.hpp"
#include "tcpclient.hpp"

// local
#include "fzupdate.hpp"
#include "epsmap.hpp"


using namespace fz;

constexpr time_t seconds_per_day = 24*60*60;
constexpr time_t five_minutes_in_seconds = 5*60;

/**
 * Reserver 5 min slots immediately preceding exact target date.
 * @param n_ptr Pointer to Node for which to allocate slots.
 * @param slots Slots needed, returning any that did not fit into the day.
 * @param td Node target date (td<0 means used time from end of day).
 * @return True if any map slots were already occupied.
 */
bool EPS_map_day::reserve_exact(Node_ptr n_ptr, int &slots, time_t td) {
    if (td < 0) {
        td = seconds_per_day;
    } else {
        td -= t_daystart; // seconds into the day
        if ((td < 0) || (td > seconds_per_day))
            return false;
    }
    bool overlap = false;
    int before_i = td / five_minutes_in_seconds;
    for (int i = (before_i - 1); ((i >= 0) && (slots > 0)); i--) {
        if (fivemin_slot[i] > ((Node_ptr)1))
            overlap = true;
        else
            fivemin_slot[i] = n_ptr;
        slots--;
    }
    return overlap;
}

/**
 * Reserve 5 min slots preceding the target date.
 * @param n_ptr Pointer to Node for which to allocate slots.
 * @param slots Slots needed, returning any that did not fit into the day.
 * @param td Node target date (td<0 means used time from end of day).
 * @return 0 if all goes well (even if slots remain to be reserved), 1 if there
 *         is a td value problem, 1 if reserved space from earlier iteration of
 *         the same Node was encountered (repeating Nodes scheduled too tightly).
 */
int EPS_map_day::reserve_fixed(Node_ptr n_ptr, int & slots, time_t td) {
    int before_i = 288;
    if (td >= 0) {
        td -= t_daystart; // seconds into the day
        if ((td < 0) || (td > seconds_per_day))
            return 1;
        before_i = td / five_minutes_in_seconds;
    }
    for (int i = (before_i - 1); ((i >= 0) && (slots > 0)); i--) {
        if (!fivemin_slot[i]) {
            fivemin_slot[i] = n_ptr;
            slots--;
        } else if (fivemin_slot[i] == n_ptr)
            return 2;
    }
    return 0;
}

/**
 * Reserves 5 min slots advancing from the earliest available slot onward.
 * @param n_ptr Pointer to Node for which to allocate slots.
 * @param slots Slots needed, returning any that did not fit into the day.
 * @return Target date immediately following the last slot allocated or immediately
 *         following the end of the day.
 */
time_t EPS_map_day::reserve(Node_ptr n_ptr, int & slots) {
    time_t td = -1;
    for (int i = 0; ((i < 288) && (slots > 0)); i++) {
        if (!fivemin_slot[i]) {
            fivemin_slot[i] = n_ptr;
            slots--;
            td = i + 1;
        }
    }
    if (td < 0)
        return td;
    td *= five_minutes_in_seconds;
    return t_daystart + td;
}

EPS_map::EPS_map(time_t t, unsigned long days_in_map, targetdate_sorted_Nodes &_incomplete_repeating,
                 std::vector<chunks_t> &_chunks_req, std::vector<EPS_flags> &_epsflags_vec, std::vector<time_t> &_t_eps) : num_days(days_in_map), starttime(t), slots_per_chunk(fzu.config.chunk_minutes / 5),
                                                                                                                           nodelist(_incomplete_repeating), chunks_req(_chunks_req), epsflags_vec(_epsflags_vec), t_eps(_t_eps) {
    if (days_in_map<1) {
        days_in_map = 1;
        num_days = 1;
    }
    firstdaystart = day_start_time(t);
    days.assign(days_in_map, firstdaystart);
    time_t daystart = firstdaystart;
    for (auto & day : days) {
        day.init(daystart);
        daystart = time_add_day(daystart,1);
    }
    time_t seconds_passed = t - firstdaystart;
    int slots_passed = (seconds_passed % five_minutes_in_seconds) ? (seconds_passed/five_minutes_in_seconds) + 1 : seconds_passed/five_minutes_in_seconds;
    if (slots_passed > 288) {
        standard_exit_error(exit_general_error, "More than 288 five minute slots passed in day. If the clock was moved back from daylight savings time today and this function was used with a time greater than 23:00, then more than 288 five minute chunks of the day can indeed have been used. There is no special case for this, since it is a problem that can only occur for a maximum of 60 minutes per year. If this is the reason for the problem then please wait until 00:00 to try again.", __func__);
    }
    for (int i=0; i < slots_passed; i++) {
        days[0].fivemin_slot[i] = ((Node_ptr) 1); // special code, slots unavailable on first day in map
    }
}

/**
 * Reserve `chunks_req` in five minute granularity, immediately preceding
 * the exact target date.
 * @param n_ptr Pointer to Node for which to allocate time.
 * @param chunks_req Number of chunks required.
 * @param td Node target date.
 * @return True if any of the slots are already occupied (exact target date
 *         Nodes with overlapping intervals).
 */
bool EPS_map::reserve_exact(Node_ptr n_ptr, int chunks_req, time_t td) {
    int slots = chunks_req * slots_per_chunk;
    bool overlap = false;
    int td_day = find_dayTD(td);
    if (td_day < 0)
        return false;
    while ((slots > 0) && (td_day >= 0)) {
        if (days[td_day].reserve_exact(n_ptr, slots, td))
            overlap = true;
        td_day--;
        td = -1; // then from end of day
    }
    return overlap;
}

void EPS_map::place_exact() {
    size_t i = 0;
    for (const auto & [t, node_ptr] : nodelist) {
        if (node_ptr->effective_targetdate()>fzu.t_limit) {
            break;
        }
        if ((chunks_req[i]>0) && (node_ptr->get_tdproperty() == exact)) {
            epsflags_vec[i].set_exact();
            if (reserve_exact(node_ptr, chunks_req[i], t)) {
                epsflags_vec[i].set_overlap();
            }
            if ((node_ptr->get_tdpattern() < patt_yearly) && (node_ptr->get_tdspan() == 0)) {
                epsflags_vec[i].set_periodiclessthanyear();
            }
        }
        ++i;
    }      
}

/// Find the day that contains the target date.
int EPS_map::find_dayTD(time_t td) {
    if ((td < days[0].t_daystart) || (td >= (days[num_days - 1].t_daystart + seconds_per_day)))
        return -1;
    int bottom = 0, top = num_days, middle = num_days / 2;
    while ((bottom < middle) && (top > middle)) {
        if (td < days[middle].t_daystart) {
            top = middle;
            middle = bottom + ((middle - bottom) / 2);
        } else {
            bottom = middle;
            middle = middle + ((top - middle) / 2);
        }
    }
    return bottom;
}

/**
 * Reserve `chunks_req` in five minute granularity, immediately preceding
 * the fixed target date. Reserves nearest available slots from the target
 * date on down.
 * @param n_ptr Pointer to Node for which to allocate time.
 * @param chunks_req Number of chunks required.
 * @param td Node target date.
 * @return True if there are insufficient slots available.
 */
bool EPS_map::reserve_fixed(Node_ptr n_ptr, int chunks_req, time_t td) {
    int slots = chunks_req * slots_per_chunk;
    bool insufficient = false;
    int td_day = find_dayTD(td);
    if (td_day < 0)
        return false;
    time_t td_topflag = td;
    while ((slots > 0) && (td_day >= 0)) {
        switch (days[td_day].reserve_fixed(n_ptr, slots, td_topflag)) {
        case 2: {
            insufficient = true;
            td_day = -1;
            // *** The following should have a link that can be followed, or
            //     something similar, even when using UI_Info_Yad and such.
            if (fzu.config.warn_repeating_too_tight) {
                ADDWARNING("Repeating Node "+n_ptr->get_id_str()+" may repeat too frequently before "+TimeStampYmdHM(td), __func__);
            }
            break;
        }
        case 1: {
            insufficient = true;
        }
        }
        td_day--;
        td_topflag = -1; // set to indicate start next at end of preceding day
    }
    if (slots > 0)
        insufficient = true;
    return insufficient;
}

/**
 * Note: In dil2al, this included collecting info about how a target date was obtained, and that info could be
 * fairly diverse. Now, in the Formalizer 2.x, target dates are set at the Node, so you only have two
 * cases: Either the targetdate was specified at the Node, or it was inherited (propagated). It is now
 * solidly the td_property that determines what is done, and the 'inherit' property forces inheritance
 * (much as 'unspecified+fixed' used to). For more about this, see the documentation of target date
 * properties in https://docs.google.com/document/d/1rYPFgzFgjkF1xGx3uABiXiaDR5sfmOzqYQRqSntcyyY/edit#heading=h.td3aptmw2f5.
 */
void EPS_map::place_fixed() {
    size_t i = 0;
    for (const auto & [t, node_ptr] : nodelist) {
        //time_t td_eff = node_ptr->effective_targetdate();
        Node_ptr origin; // defaults to node_ptr in effective_targetdate()
        if (node_ptr->effective_targetdate(&origin)>fzu.t_limit) {
            break;
        }
        bool effectively_fixed = (origin->get_tdproperty() == fixed);
        if ((chunks_req[i]>0) && effectively_fixed) {
            if (node_ptr == origin) {
                epsflags_vec[i].set_fixed();
                if (reserve_fixed(node_ptr, chunks_req[i], t)) {
                    epsflags_vec[i].set_insufficient();
                }
            } else {
                epsflags_vec[i].set_treatgroupable();
            }
            if ((node_ptr->get_tdpattern() < patt_yearly) && (node_ptr->get_tdspan() == 0)) {
                epsflags_vec[i].set_periodiclessthanyear();
            }                
        }
        ++i;
    }
}

/**
 * Reserve `chunks_req` in five minute granularity from the earliest
 * available slot onward.
 * @param n_ptr Pointer to Node for which to allocate time.
 * @param chunks_req Number of chunks required.
 * @return Corresponding suggested target date taking into account TD preferences,
 *         or -1 if there are insufficient slots available.
 */
time_t EPS_map::reserve(Node_ptr n_ptr, int chunks_req) {
    int slots = chunks_req * slots_per_chunk;
    unsigned long td_day = 0;
    time_t epstd = starttime;
    while ((slots > 0) && (td_day < num_days)) {
        epstd = days[td_day].reserve(n_ptr, slots);
        td_day++;
    }
    if (slots > 0)
        return -1;

    // Adapt epstd suggestion to TD preferences
    if (fzu.config.endofday_priorities) {

        // which end-of-day to use is indicated by the urgency parameter
        time_t priorityendofday = fzu.config.dolater_endofday;
        /* *** There is no equivalent of this urgency parameter in the Node (yet)!
        if (n_ptr->Urgency() >= 1.0)
            priorityendofday = fzu.config.doearlier_endofday;
        */
        if (priorityendofday > (23 * 3600 + 1800))
            priorityendofday -= 1800; // insure space for minute offsets to avoid auto-merging

        // compare that with the proposed target date time of day
        if (td_day > 0) {
            td_day--; // back to the last day accessed
        }
        time_t timeofday = epstd - days[td_day].t_daystart;
        if (timeofday < priorityendofday) { // set to priorityendofday on this day
            epstd = days[td_day].t_daystart + priorityendofday;
        } else { // set to priorityendofday on next day
            epstd = time_add_day(days[td_day].t_daystart, 1) + priorityendofday;
        }

        // check if a previous group already has the target date, if so offset slightly to preserve grouping and group order
        if (epstd <= previous_group_td)
            epstd = previous_group_td + fzu.config.eps_group_offset_mins; // one or a few minutes offset
    }
    return epstd;
}

void EPS_map::group_and_place_movable() {
    struct nodelist_itidx {
        ssize_t index;
        targetdate_sorted_Nodes::iterator it;
        nodelist_itidx(ssize_t _i, targetdate_sorted_Nodes::iterator _it) : index(_i), it(_it) {}
        void set(ssize_t _i, targetdate_sorted_Nodes::iterator _it) {
            index = _i;
            it = _it;
        }
    };
    nodelist_itidx group_first((ssize_t)-1, nodelist.end());
    nodelist_itidx group_last((ssize_t)-1, nodelist.end());
    time_t group_td = RTt_unspecified;
    ssize_t i = 0;
    for (auto it = nodelist.begin(); it != nodelist.end(); ++it) {
        time_t t = it->first;
        Node_ptr node_ptr = it->second;
        //time_t td_eff = node_ptr->effective_targetdate();
        //Node_ptr origin; // defaults to node_ptr in effective_targetdate()
        if (node_ptr->effective_targetdate()>fzu.t_limit) {
            break;
        }
        if ((chunks_req[i] > 0) && ((node_ptr->get_tdproperty() == variable) || (node_ptr->get_tdproperty() == unspecified) || epsflags_vec[i].EPS_treatgroupable())) {
            if (t != group_td) { // process EPS group and start new one
                if (group_first.index >= 0) {
                    time_t epsgroup_td = group_td;
                    auto j_it = group_first.it;
                    for (auto j = group_first.index; j <= group_last.index; j++) {
                        if (epsflags_vec[j].EPS_epsgroupmember()) {
                            epsgroup_td = reserve(j_it->second, chunks_req[j]);
                        }
                        ++j_it;
                    }
                    if (epsgroup_td < 0) { // mostly, for tasks with MAXTIME target dates that don't fit into the map
                        for (auto j = group_first.index; j <= group_last.index; j++) {
                            epsflags_vec[j].set_insufficient();
                        }
                    } else {
                        for (auto j = group_first.index; j <= group_last.index; j++) {
                            t_eps[j] = epsgroup_td; // THIS is the new proposed target date
                        }
                        previous_group_td = epsgroup_td; // referenced in reserve()
                    }
                }

                group_first.set(i, it);
                group_td = t;
            }

            group_last.set(i, it); // mark in case there are non-group entries between firsts in this group and the next
            epsflags_vec[i].set_epsgroupmember();
        }

        ++i;
    }
}

targetdate_sorted_Nodes EPS_map::get_eps_update_nodes() {
    targetdate_sorted_Nodes update_nodes;
    time_t group_td = RTt_unspecified;
    size_t i = 0;
    for (const auto & [t, node_ptr] : nodelist) {
        if (node_ptr->effective_targetdate() > fzu.t_limit) {
            break;
        }

        if (chunks_req[i] > 0) {
            if (t != group_td) {
                group_td = t;
            }
            // FXD if !epsflags_vec[i].EPS_epsgroupmember() (in the dil2al version)
            // the group_td is now the new target date if unspecified or variable
            if ((node_ptr->get_tdproperty() == variable) || (node_ptr->get_tdproperty() == unspecified)) {
                if (fzu.config.update_to_earlier_allowed || (t_eps[i] > group_td)) { 
                    // THIS is a Node for which the target date should be updated to t_eps[i]
                    update_nodes.emplace(t_eps[i], node_ptr);
                }
            }
        }

        ++i;
    }
    return update_nodes;
}
