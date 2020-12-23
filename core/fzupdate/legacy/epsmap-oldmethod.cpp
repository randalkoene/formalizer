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
#include <algorithm>

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

void Node_twochar_encoder::build_codebook(const targetdate_sorted_Nodes & nodelist) {
    unsigned int i = 0;
    for (const auto & [t, node_ptr] : nodelist) {
        if (node_ptr) {
            emplace(node_ptr->get_id().key(), i);
            ++i;
        }
    }
}

std::string Node_twochar_encoder::html_link_str(const Node & node, const EPS_map & epsmap) const {
    std::string htmlstr("<a href=\"/cgi-bin/fzlink.py?id=");
    htmlstr += node.get_id_str()+"\">";
    bool movable = epsmap.epsdata[epsmap.node_vector_index.at(node.get_id().key())].t_eps != RTt_maxtime;
    if (movable) {
        htmlstr += "<b>";
    } 
    htmlstr += at(node.get_id().key());
    if (movable) {
        htmlstr += "</b>";
    }
    htmlstr += "</a>";
    return htmlstr;
}

std::string Node_twochar_encoder::html_link_str(const Node_ptr node_ptr, const EPS_map & epsmap) const {
    if (node_ptr == nullptr) {
        return "::";
    }
    return html_link_str(*node_ptr, epsmap);
}

void eps_data_vec::updvar_chunks_required(const targetdate_sorted_Nodes & nodelist) {
    chunks_t suspicious_req = (36*60)/fzu.config.chunk_minutes;
    size_t i = 0;
    for (const auto & [t, node_ptr] : nodelist) {
        at(i).chunks_req = seconds_to_chunks(node_ptr->seconds_to_complete());
        if (at(i).chunks_req > suspicious_req) {
            ADDWARNING(__func__, "Suspiciously large number of chunks ("+std::to_string(at(i).chunks_req)+") needed to complete Node "+node_ptr->get_id_str());
        }
        ++i;
    }
}

size_t eps_data_vec::updvar_total_chunks_required_nonperiodic(const targetdate_sorted_Nodes & nodelist) {
    // *** Note: The chunks_req[i] test may be unnecessary, because we're working with a list of incomplete Nodes.
    size_t total = 0;
    size_t i = 0;
    for (const auto & [t, node_ptr] : nodelist) {
        if (node_ptr->effective_targetdate()>fzu.t_limit) {
            break;
        }
        if ((at(i).chunks_req > 0) && (!node_ptr->get_repeats())) { // *** It's actually not clear to me why we don't want to count the repeating Node chunks.
            total += at(i).chunks_req;
        }
        ++i;
    }
    return total;
}

eps_data_vec::eps_data_vec(const targetdate_sorted_Nodes & _incomplete_repeating): eps_data_vec_t(_incomplete_repeating.size()) {
    updvar_chunks_required(_incomplete_repeating);
}

std::string EPS_map::show() {
    // *** So far, this version does not place old and new side by side. It only shows new.
    Node_twochar_encoder codebook(nodelist);
    std::string showstr;
    std::vector<std::string> htmlvec = html(codebook);
    for (const auto & line : htmlvec) {
        if (line.empty()) {
            showstr += '\n';
        } else {
            showstr += line + '\n';
        }
    }
    return showstr;
}

#ifdef USE_MAP_FOR_MAPPING

void EPS_map::prepare_day_separator() {
    constexpr unsigned int space_for_date = 8;
    day_separator.assign((slots_per_line*2) - space_for_date,' ');
    for (unsigned int hour = 1; hour < hours_per_line; ++hour) {
        unsigned int i = hour*chars_per_hour - space_for_date;
        for (unsigned int j = 0; j < lines_per_day; ++j) {
            unsigned int h = hour + (j*hours_per_line);
            if (j>0) {
                day_separator[i++] = ',';
            }
            day_separator[i++] = '0' + (h / 10);
            day_separator[i++] = '0' + (h % 10);
        }
    }
}

void EPS_map::add_map_code(time_t t, const char * code_cstr, std::vector<std::string> & maphtmlvec) const {
    time_t t_diff = t - firstdaystart;
    bool is_linestart = (t_diff % seconds_per_line) == 0;
    if (!is_linestart) {
        maphtmlvec.back() += code_cstr;
    } else {
        bool is_daystart = (t_diff % seconds_per_day) == 0; // *** this breaks on daylight savings change!
        if (is_daystart) {
            maphtmlvec.emplace_back(DateStampYmd(t)+day_separator);
        }
        maphtmlvec.emplace_back(code_cstr);
        maphtmlvec.back().reserve(slots_per_line*2);
    }
}

std::vector<std::string> EPS_map::html(const Node_twochar_encoder & codebook) {
    std::vector<std::string> maphtmlvec;
    prepare_day_separator();

    for (time_t t = firstdaystart; t < first_slot_td; t += five_minutes_in_seconds) {
        add_map_code(t, "##", maphtmlvec);
    }

    for (const auto & [t, node_ptr] : slots) {
        add_map_code(t, codebook.html_link_str(node_ptr, *this).c_str(), maphtmlvec);
    }

    return maphtmlvec;
}

EPS_map::EPS_map(time_t _t, unsigned long days_in_map, targetdate_sorted_Nodes & _incomplete_repeating, eps_data_vec_t & _epsdata) :
    //std::vector<chunks_t> & _chunks_req, std::vector<EPS_flags> & _epsflags_vec, std::vector<time_t> & _t_eps) : 
    num_days(days_in_map), starttime(_t), slots_per_chunk(fzu.config.chunk_minutes / 5), nodelist(_incomplete_repeating),
    epsdata(_epsdata) {
    //chunks_req(_chunks_req), epsflags_vec(_epsflags_vec), t_eps(_t_eps) {
    if (days_in_map<1) {
        days_in_map = 1;
        num_days = 1;
    }
    firstdaystart = day_start_time(_t);
    time_t epochtime_aftermap = firstdaystart + (days_in_map * seconds_per_day);

    time_t t_diff = starttime - firstdaystart;
    size_t firstday_slotspassed = t_diff / five_minutes_in_seconds;
    firstday_slotspassed += ((t_diff % five_minutes_in_seconds) != 0) ? 1 : 0;
    first_slot_td = firstdaystart + (firstday_slotspassed+1)*five_minutes_in_seconds;

    for (time_t t = first_slot_td; t < epochtime_aftermap; t += five_minutes_in_seconds) {
        slots.emplace_hint(slots.end(), t, nullptr);
    }

    size_t vector_index = 0;
    for (const auto & [t, node_ptr] : nodelist) {
        if (node_ptr) {
            node_vector_index.emplace(node_ptr->get_id().key(), vector_index);
        }
        ++vector_index;
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
    bool overlap = false;
    auto slot_it = slots.lower_bound(td); // at td or earlier
    if (slot_it == slots.end()) {
        return false;
    }

    for (size_t slots_req = chunks_req * slots_per_chunk; slots_req > 0; --slots_req) {

        if (slot_it->second) {
            overlap = true;
        } else {
            slot_it->second = n_ptr;
        }

        if (slot_it == slots.begin()) {
            return overlap;
        }
        --slot_it;
    }

    return overlap;
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
    size_t slots_req = chunks_req * slots_per_chunk;
    auto slot_it = slots.lower_bound(td); // at td or earlier
    if (slot_it == slots.end()) {
        return false;
    }

    do {
        if (!slot_it->second) {
            slot_it->second = n_ptr;
            --slots_req;
        }

        if (slot_it == slots.begin()) {
            return (slots_req > 0);
        }
        --slot_it;            
    } while (slots_req > 0);

    return false;            
}

/**
 * Adjust the new target date to take into account regular end of day
 * target times for prioritized activities.
 * 
 * @param td_raw The new target date that was obtained, without adjustments.
 * @return A target date that may be shifted to align with a time of day
 *         specified for activities with a particular priority type.
 */
time_t EPS_map::end_of_day_adjusted(time_t td_raw) {
    // which end-of-day to use is indicated by the urgency parameter
    time_t priorityendofday = fzu.config.dolater_endofday;
    /* *** There is no equivalent of this urgency parameter in the Node (yet)!
    if (n_ptr->Urgency() >= 1.0)
        priorityendofday = fzu.config.doearlier_endofday;
    */
    if (priorityendofday > (23 * 3600 + 1800))
        priorityendofday -= 1800; // insure space for minute offsets to avoid auto-merging

    // compare that with the proposed target date time of day
    time_t td_new = day_start_time(td_raw) + priorityendofday;
    if (td_raw > td_new) {
        td_new = time_add_day(td_new);
    }

    // check if a previous group already has the target date, if so offset slightly to preserve grouping and group order
    if (td_new <= previous_group_td)
        td_new = previous_group_td + (fzu.config.eps_group_offset_mins*60); // one or a few minutes offset

    return td_new;
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
    size_t slots_req = chunks_req * slots_per_chunk;
    time_t new_targetdate = RTt_unspecified;
    for (auto & slot : slots) {
        if (!slot.second) {
            slot.second = n_ptr;
            --slots_req;
            if (slots_req == 0) {
                new_targetdate = slot.first;
                break;
            }
        }
    }
    if (new_targetdate == RTt_unspecified) {
        return new_targetdate; // slots needed do not fit into map
    }

    if (fzu.config.endofday_priorities) {
        return end_of_day_adjusted(new_targetdate);
    }

    return new_targetdate;
}

#else // USE_MAP_FOR_MAPPING

std::vector<std::string> EPS_map_day::html(const Node_twochar_encoder & codebook) const {
    std::vector<std::string> dayhtmlvec;
    dayhtmlvec.emplace_back(DateStampYmdHM(t_daystart));
    std::string fourhourshtml;
    for (unsigned int i = 0; i < slots_per_day; ++i) {
        Node_ptr node_ptr = fivemin_slot[i];
        if (node_ptr) {
            if (node_ptr == UNAVAILABLE_SLOT) {
                fourhourshtml += "##";
            } else {
                fourhourshtml += codebook.html_link_str(*node_ptr);
            }
        } else {
            fourhourshtml += "::";
        }
        if (((i+1) % slots_per_line) == 0) {
            dayhtmlvec.emplace_back(fourhourshtml);
            fourhourshtml.clear();
        }
    }
    return dayhtmlvec;
}

std::vector<std::string> EPS_map::html(const Node_twochar_encoder & codebook) const {
    constexpr const char * emptyline = "";
    std::vector<std::string> maphtmlvec;
    for (const auto & day : days) {
        std::vector<std::string> dayhtmlvec = day.html(codebook);
        for (const auto & line : dayhtmlvec) {
            maphtmlvec.emplace_back(line);
        }
        //std::move(dayhtmlvec.begin(), dayhtmlvec.end(), maphtmlvec.end()); // merge, see https://trello.com/c/IQ5h71Xj
        maphtmlvec.emplace_back(emptyline);
    }
    return maphtmlvec;
}

/**
 * Reserve 5 min slots immediately preceding exact target date.
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
        if ((td < 0) || (td > seconds_per_day)) {
            return false;
        }
    }
    bool overlap = false;
    int before_i = td / five_minutes_in_seconds;
    for (int i = (before_i - 1); ((i >= 0) && (slots > 0)); i--) {
        if (fivemin_slot[i] > (UNAVAILABLE_SLOT)) {
            overlap = true;
        } else {
            fivemin_slot[i] = n_ptr;
            slots--;
        }
        //slots--;
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
    int before_i = slots_per_day;
    if (td >= 0) {
        td -= t_daystart; // seconds into the day
        if ((td < 0) || (td > seconds_per_day))
            return 1;
        before_i = td / five_minutes_in_seconds;
    }
    for (int i = (before_i - 1); ((i >= 0) && (slots > 0)); i--) {
        if (fivemin_slot[i] == nullptr) {
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
 * @param slots_req Slots needed, returning any that did not fit into the day.
 * @return Target date immediately following the last slot allocated or immediately
 *         following the end of the day, or -1 of no slots were available this day.
 */
time_t EPS_map_day::reserve(Node_ptr n_ptr, int & slots_req) {
    long slots_reserved = -1;
    for (size_t slot = 0; ((slot < slots_per_day) && (slots_req > 0)); slot++) {
        if (fivemin_slot[slot] == nullptr) {
            fivemin_slot[slot] = n_ptr;
            slots_req--;
            slots_reserved = slot + 1;
        }
    }

    return (slots_reserved < 0) ? slots_reserved : t_daystart + (five_minutes_in_seconds * slots_reserved);
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
    VERYVERBOSEOUT("Slots passed on the first day: "+std::to_string(slots_passed)+'\n');
    for (int i=0; i < slots_passed; i++) {
        days[0].fivemin_slot[i] = UNAVAILABLE_SLOT; // special code, slots unavailable on first day in map
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
    if ((td_day < 0) || (td_day >= (int)num_days))
        return false;
    while ((slots > 0) && (td_day >= 0)) {
        if (days[td_day].reserve_exact(n_ptr, slots, td))
            overlap = true;
        td_day--;
        td = -1; // then from end of day
    }
    return overlap;
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
    if ((td_day < 0) || (td_day >= (int)num_days))
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
            epstd = previous_group_td + (fzu.config.eps_group_offset_mins*60); // one or a few minutes offset
    }
    return epstd;
}

#endif // USE_MAP_FOR_MAPPINGs

void EPS_map::place_exact() {
    size_t i = 0;
    for (const auto & [t, node_ptr] : nodelist) {
        if (node_ptr->effective_targetdate()>fzu.t_limit) {
            break;
        }
        eps_data & epsdataref = epsdata[i];
        if ((epsdataref.chunks_req>0) && (node_ptr->get_tdproperty() == exact)) {
            epsdataref.epsflags.set_exact();
            if (reserve_exact(node_ptr, epsdataref.chunks_req, t)) {
                epsdataref.epsflags.set_overlap();
            }
            if ((node_ptr->get_tdpattern() < patt_yearly) && (node_ptr->get_tdspan() == 0)) {
                epsdataref.epsflags.set_periodiclessthanyear();
            }
        /*if ((chunks_req[i]>0) && (node_ptr->get_tdproperty() == exact)) {
            epsflags_vec[i].set_exact();
            if (reserve_exact(node_ptr, chunks_req[i], t)) {
                epsflags_vec[i].set_overlap();
            }
            if ((node_ptr->get_tdpattern() < patt_yearly) && (node_ptr->get_tdspan() == 0)) {
                epsflags_vec[i].set_periodiclessthanyear();
            }*/
        }
        ++i;
    }
    if (fzu.config.showmaps) {
        VERYVERBOSEOUT("\nMap with exact targetdate placements:\n");
        VERYVERBOSEOUT(show());
    }
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
        eps_data & epsdataref = epsdata[i];
        bool effectively_fixed = (origin->get_tdproperty() == fixed);
        if ((epsdataref.chunks_req>0) && effectively_fixed) {
            if (node_ptr == origin) {
                epsdataref.epsflags.set_fixed();
                if (reserve_fixed(node_ptr, epsdataref.chunks_req, t)) {
                    epsdataref.epsflags.set_insufficient();
                }
            } else {
                epsdataref.epsflags.set_treatgroupable();
            }
            if ((node_ptr->get_tdpattern() < patt_yearly) && (node_ptr->get_tdspan() == 0)) {
                epsdataref.epsflags.set_periodiclessthanyear();
            }                
        }
        ++i;
    }
    if (fzu.config.showmaps) {
        VERYVERBOSEOUT("\nMap with fixed targetdate placements:\n");
        VERYVERBOSEOUT(show());
    }
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
        eps_data & epsdataref = epsdata[i];
        if ((epsdataref.chunks_req > 0) && ((node_ptr->get_tdproperty() == variable) || (node_ptr->get_tdproperty() == unspecified) || epsdataref.epsflags.EPS_treatgroupable())) {
            if (t != group_td) { // process EPS group and start new one
                if (group_first.index >= 0) {
                    time_t epsgroup_td = group_td;
                    auto j_it = group_first.it;
                    for (auto j = group_first.index; j <= group_last.index; j++) {
                        eps_data & epsj = epsdata[j];
                        if (epsj.epsflags.EPS_epsgroupmember()) {
                            epsgroup_td = reserve(j_it->second, epsj.chunks_req);
                        }
                        ++j_it;
                    }
                    if (epsgroup_td < 0) { // mostly, for tasks with MAXTIME target dates that don't fit into the map
                        for (auto j = group_first.index; j <= group_last.index; j++) {
                            epsdata[j].epsflags.set_insufficient();
                        }
                    } else {
                        for (auto j = group_first.index; j <= group_last.index; j++) {
                            epsdata[j].t_eps = epsgroup_td; // THIS is the new proposed target date
                            //VERYVERBOSEOUT("t_eps["+std::to_string(j)+"]="+TimeStampYmdHM(epsgroup_td)+'\n');
                        }
                        previous_group_td = epsgroup_td; // referenced in reserve()
                    }
                }

                group_first.set(i, it);
                group_td = t;
            }

            group_last.set(i, it); // mark in case there are non-group entries between firsts in this group and the next
            epsdataref.epsflags.set_epsgroupmember();
        }

        ++i;
    }
    if (fzu.config.showmaps) {
        VERYVERBOSEOUT("\nMap with all placements:\n");
        VERYVERBOSEOUT(show());
    }
}

targetdate_sorted_Nodes EPS_map::get_eps_update_nodes() {
    targetdate_sorted_Nodes update_nodes;
    time_t group_td = RTt_unspecified;
    size_t i = 0;
    VERYVERBOSEOUT("\nThe set of target date updates that will be requested:\n");
    for (const auto & [t, node_ptr] : nodelist) {
        if (!node_ptr) {
            standard_exit_error(exit_general_error, "Unexpected node_ptr == nullptr", __func__);
        }
        if (node_ptr->effective_targetdate() > fzu.t_limit) {
            break;
        }
        eps_data & epsdataref = epsdata[i];

        if (epsdataref.chunks_req > 0) {
            if (t != group_td) {
                group_td = t;
            }
            // FXD if !epsflags_vec[i].EPS_epsgroupmember() (in the dil2al version)
            // the group_td is now the new target date if unspecified or variable
            if ((node_ptr->get_tdproperty() == variable) || (node_ptr->get_tdproperty() == unspecified)) {
                if (fzu.config.update_to_earlier_allowed || (epsdataref.t_eps > group_td)) { 
                    // THIS is a Node for which the target date should be updated to t_eps[i]
                    VERYVERBOSEOUT('\t'+node_ptr->get_id_str()+": from "+TimeStampYmdHM(node_ptr->effective_targetdate())+" to t_eps["+std::to_string(i)+"]="+TimeStampYmdHM(epsdataref.t_eps)+'\n');
                    update_nodes.emplace(epsdataref.t_eps, node_ptr);
                }
            }
        }

        ++i;
    }
    VERYVERBOSEOUT("GOT TO THE END OF THE SET TO UPDATE\n");
    return update_nodes;
}
