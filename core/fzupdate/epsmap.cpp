// Copyright 20201126 Randal A. Koene
// License TBD

/**
 * EPS Map structures and methods used to update the Node Schedule.
 * 
 * {{ long_description }}
 * 
 * For more about this, see {{ doc_reference }}.
 */

//#define USE_COMPILEDPING
//#define DEBUG_SKIP

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
        if (!node_ptr) {
            ADDERROR(__func__, "Received a null-node");
        } else {
            at(i).chunks_req = seconds_to_chunks(node_ptr->seconds_to_complete());
            if (at(i).chunks_req > suspicious_req) {
                ADDWARNING(__func__, "Suspiciously large number of chunks ("+std::to_string(at(i).chunks_req)+") needed to complete Node "+node_ptr->get_id_str());
            }
        }
        ++i;
    }
}

size_t eps_data_vec::updvar_total_chunks_required_nonperiodic(const targetdate_sorted_Nodes & nodelist) {
    // *** Note: The chunks_req[i] test may be unnecessary, because we're working with a list of incomplete Nodes.
    size_t total = 0;
    size_t i = 0;
    for (const auto & [t, node_ptr] : nodelist) {
        if (!node_ptr) {
            ADDERROR(__func__, "Received a null-node");
        } else {
            if (node_ptr->effective_targetdate()>fzu.t_limit) {
                break;
            }
            if ((at(i).chunks_req > 0) && (!node_ptr->get_repeats())) { // *** It's actually not clear to me why we don't want to count the repeating Node chunks.
                total += at(i).chunks_req;
            }
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
    num_days(days_in_map), starttime(_t), slots_per_chunk(fzu.config.chunk_minutes / minutes_per_slot), nodelist(_incomplete_repeating),
    epsdata(_epsdata) {
    if (days_in_map<1) {
        days_in_map = 1;
        num_days = 1;
    }
    firstdaystart = day_start_time(_t);
    time_t epochtime_aftermap = firstdaystart + (days_in_map * seconds_per_day);
    t_beyond = epochtime_aftermap;

    time_t t_diff = starttime - firstdaystart;
    size_t firstday_slotspassed = t_diff / five_minutes_in_seconds;
    firstday_slotspassed += ((t_diff % five_minutes_in_seconds) != 0) ? 1 : 0;
    first_slot_td = firstdaystart + (firstday_slotspassed+1)*five_minutes_in_seconds;

    for (time_t t = first_slot_td; t < epochtime_aftermap; t += five_minutes_in_seconds) {
        slots.emplace_hint(slots.end(), t, nullptr);
    }
    next_slot = slots.begin();

    size_t vector_index = 0;
    for (const auto & [t, node_ptr] : nodelist) {
        if (node_ptr) {
            node_vector_index.emplace(node_ptr->get_id().key(), vector_index);
        }
        ++vector_index;
    }

    VERYVERBOSEOUT("\nInitialized map with "+std::to_string(slots.size())+"slots with room for "+std::to_string(slots.size()/slots_per_chunk)+" Node chunks.\n");
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
    // *** TODO: What to do if the offset is such that it shifts before the start of day?
    if (fzu.config.timezone_offset_hours != 0) {
        priorityendofday -= (3600*fzu.config.timezone_offset_hours);
    }
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
 * 
 * Note that this function uses the `EPS_map::next_slot` parameter to avoid
 * checking slots more than once.
 * 
 * @param n_ptr Pointer to Node for which to allocate time.
 * @param chunks_req Number of chunks required.
 * @return Corresponding suggested target date taking into account TD preferences,
 *         or -1 if there are insufficient slots available.
 */
time_t EPS_map::reserve(Node_ptr n_ptr, int chunks_req) {
    size_t slots_req = chunks_req * slots_per_chunk;
    time_t new_targetdate = RTt_unspecified;
    for (; next_slot != slots.end(); ++next_slot) {
        if (!next_slot->second) {
            next_slot->second = n_ptr;
            --slots_req;
            if (slots_req == 0) {
                new_targetdate = next_slot->first;
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

void EPS_map::place_exact() {
    size_t i = 0;
    for (const auto & [t, node_ptr] : nodelist) {
        if (!node_ptr) {
            ADDERROR(__func__, "Received a null-node");
        } else {
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
            }
        }
        ++i;
    }

    VERYVERBOSEOUT("\nNodes with exact target dates mapped.\n");
    if (fzu.config.showmaps) {
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
        if (!node_ptr) {
            ADDERROR(__func__, "Received a null-node");
        } else {
            //time_t td_eff = node_ptr->effective_targetdate();
            Node_ptr origin; // defaults to node_ptr in effective_targetdate()
            if (node_ptr->effective_targetdate(&origin)>fzu.t_limit) {
                break;
            }
            eps_data & epsdataref = epsdata[i];
            bool effectively_fixed = false;
            if (origin) {
                effectively_fixed = (origin->get_tdproperty() == fixed);
            }
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
        }
        ++i;
    }

    VERYVERBOSEOUT("\nNodes with fixed target dates mapped.\n");
    if (fzu.config.showmaps) {
        VERYVERBOSEOUT(show());
    }
}

/**
 * Setting updated times for movable target date Nodes. The EPS method uses grouping where Nodes
 * had the same target dates. The `group_first` iterator and index cache is updated whenever a
 * movable Node has a different (later) target date. The `group_last` iterator and index cache
 * is updated whenever another Node comes along that is movable and that still has the same
 * target date as the target date of the Node at index `group_first`.
 * 
 * Note that, because `group_first` is changed only when another movable Node with a later
 * target date is encountered, and only when a new group is create the Nodes in the current
 * group receive their updated target dates, the most recent group will not have been
 * updated when `fzu.t_limit` is passed or when `nodelist.end()` is reached. This is why
 * a final update call has to be made after the for-loop unless `group_td` remained `RT_unspecified`.
 */
void EPS_map::group_and_place_movable() {

    struct nodelist_itidx {
        ssize_t index;
        targetdate_sorted_Nodes::iterator it;
        nodelist_itidx(ssize_t _i, targetdate_sorted_Nodes::iterator _it) : index(_i), it(_it) {}
        void set(ssize_t _i, targetdate_sorted_Nodes::iterator _it) {
            index = _i;
            it = _it;
        }
        void advance() {
            ++index;
            ++it;
        }
        time_t t() const { return it->first; }
        Node_ptr node_ptr() const { return it->second; }
        std::pair<time_t, Node_ptr> data() const { return std::make_pair(it->first, it->second); }
        bool operator<=(const nodelist_itidx & nl_itidx) const { return index <= nl_itidx.index; }
    };

    struct node_group {
        nodelist_itidx first;
        nodelist_itidx last;
        time_t td = RTt_unspecified;
        node_group(targetdate_sorted_Nodes::iterator end_iterator) : first((ssize_t)-1, end_iterator), last((ssize_t)-1, end_iterator) {}
        void next(time_t next_td, const nodelist_itidx & next_it) { td = next_td; first = next_it; }
        time_t reserve_map_slots(time_t new_td, EPS_map & epsmap) {
            for (auto j = first; j <= last; j.advance()) {
                eps_data & epsj = epsmap.epsdata[j.index];
                if (epsj.epsflags.EPS_epsgroupmember()) {
                    new_td = epsmap.reserve(j.node_ptr(), epsj.chunks_req);
                }
            }
            return new_td;
        }
        void mark_insufficient_map_slots(EPS_map & epsmap) {
            for (auto j = first; j <= last; j.advance()) {
                epsmap.epsdata[j.index].epsflags.set_insufficient();
            }
        }
        void propose_updated_targetdates(time_t t, EPS_map & epsmap) {
            for (auto j = first; j <= last; j.advance()) {
                epsmap.epsdata[j.index].t_eps = t;
            }
        }
        void process_and_start_next(time_t t, const nodelist_itidx & it, EPS_map & epsmap) {
            if (first.index >= 0) {

                time_t epsgroup_newtd = reserve_map_slots(td, epsmap);

                if (epsgroup_newtd < 0) { // mostly, for tasks with MAXTIME target dates that don't fit into the map
                    mark_insufficient_map_slots(epsmap);
                    if (fzu.config.pack_moveable) {
                        // Keep going and update variable target date Nodes beyond the map at intervals.
                        epsmap.t_beyond += fzu.config.pack_interval_beyond;
                        propose_updated_targetdates(epsmap.t_beyond, epsmap);
                    }

                } else {
                    propose_updated_targetdates(epsgroup_newtd, epsmap);
                    epsmap.previous_group_td = epsgroup_newtd; // used in end_of_day_adjusted() to maintain spacing between groups
                }
            }

            next(t, it);
        }
        void process_last(EPS_map & epsmap) { process_and_start_next(RTt_maxtime, last, epsmap); }
        void add(const nodelist_itidx & it) { last = it; }
    };

    node_group group(nodelist.end());
    for (nodelist_itidx it(0,nodelist.begin()); it.it != nodelist.end(); it.advance()) { 
        auto [t, node_ptr] = it.data();
        if (!node_ptr) {
            ADDERROR(__func__, "Received a null-node");
        } else {
            // With the `pack_moveable` option, keep trying to pack more moveables into the map.
            if ((!fzu.config.pack_moveable) &&(node_ptr->effective_targetdate() > fzu.t_limit)) {
                break;
            }
            eps_data & epsdataref = epsdata[it.index];
            if ((epsdataref.chunks_req > 0)
                && ((node_ptr->get_tdproperty() == variable) || (node_ptr->get_tdproperty() == unspecified) || epsdataref.epsflags.EPS_treatgroupable())) {

                if (t != group.td) { // process EPS group and start new one
                    group.process_and_start_next(t, it, *this);
                }

                group.add(it);
                epsdataref.epsflags.set_epsgroupmember();
            }
        }
    }
    group.process_last(*this);

    VERYVERBOSEOUT("\nNodes with movable target dates grouped and mapped.\n");
    if (fzu.config.showmaps) {
        VERYVERBOSEOUT(show());
    }
}

targetdate_sorted_Nodes EPS_map::get_eps_update_nodes() {
    targetdate_sorted_Nodes update_nodes;

    time_t group_td = RTt_unspecified;
    size_t i = 0;
    VERYVERBOSEOUT("\nThe set of target date updates that will be requested (out of "+std::to_string(nodelist.size())+"):\n");
    for (const auto & [t, node_ptr] : nodelist) {
        if (!node_ptr) {
            standard_exit_error(exit_general_error, "Unexpected node_ptr == nullptr", __func__);
        }
        if ((!fzu.config.pack_moveable) && (node_ptr->effective_targetdate() > fzu.t_limit)) {
            break;
        }

        eps_data & epsdataref = epsdata.at(i); // *** Beware! I assumed that i cannot be too large here.

        if (epsdataref.chunks_req > 0) {
            if (t != group_td) {
                group_td = t;
            }
            // FXD if !epsflags_vec[i].EPS_epsgroupmember() (in the dil2al version)
            // the group_td is now the new target date if unspecified or variable
            if ((node_ptr->get_tdproperty() == variable) || (node_ptr->get_tdproperty() == unspecified)) {
                if (fzu.config.update_to_earlier_allowed || (epsdataref.t_eps > group_td)) {
                    // THIS is a Node for which the target date should be updated to t_eps[i]
                    VERYVERBOSEOUT('\t'+node_ptr->get_id_str()+": from "+TimeStampYmdHM(node_ptr->effective_targetdate())+
                               " to t_eps["+std::to_string(i)+"]="+TimeStampYmdHM(epsdataref.t_eps)+'\n');
                    update_nodes.emplace(epsdataref.t_eps, node_ptr);
                }
            }
        }

        ++i;
    }
    VERBOSEOUT("\nNumber of Node target date updates requested: "+std::to_string(update_nodes.size())+'\n');

    return update_nodes;
}
