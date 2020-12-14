// Copyright 20201126 Randal A. Koene
// License TBD

/**
 * Update the Node Schedule.
 * 
 * {{ long_description }}
 * 
 * For more about this, see {{ doc_reference }}.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Graph:Update"

// std
#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"
#include "ReferenceTime.hpp"
#include "Graphinfo.hpp"
#include "Graphmodify.hpp"

// local
#include "version.hpp"
#include "fzupdate.hpp"



using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzupdate fzu;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzupdate::fzupdate() : formalizer_standard_program(false), config(*this),
                 reftime(add_option_args, add_usage_top) { //ga(*this, add_option_args, add_usage_top)
    add_option_args += "ruT:";
    add_usage_top += " [-r] [-u] [-T <t_max|full>]";
    //usage_head.push_back("Description at the head of usage information.\n");
    usage_tail.push_back("The -T limit overrides the 'map_days' configuration or default parameter.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzupdate::usage_hook() {
    //ga.usage_hook();
    reftime.usage_hook();
    FZOUT("    -r update repeating Nodes\n"
          "    -u update variable target date Nodes\n"
          "    -T update up to and including <t_max> or 'full' update\n");
}

/**
 * Handler for command line options that are defined in the derived class
 * as options specific to the program.
 * 
 * Include case statements for each option. Typical handlers do things such
 * as collecting parameter values from `cargs` or setting `flowcontrol` choices.
 * 
 * @param c is the character that identifies a specific option.
 * @param cargs is the optional parameter value provided for the option.
 */
bool fzupdate::options_hook(char c, std::string cargs) {
    //if (ga.options_hook(c,cargs))
    //        return true;
    if (reftime.options_hook(c, cargs)) {
        if (reftime.Time() == RTt_invalid_time_stamp) {
            standard_exit_error(exit_general_error, "Invalid emulated time specification ("+cargs+')', __func__);
        }
        return true;
    }

    switch (c) {

    case 'r': {
        flowcontrol = flow_update_repeating;
        break;
    }

    case 'u': {
        flowcontrol = flow_update_variable;
        break;
    }

    case 'T': {
        if (cargs=="full") {
            t_limit = RTt_maxtime;
        } else {
            t_limit = time_stamp_time(cargs);
        }
        break;
    }

    }

    return false;
}

unsigned int get_chunk_minutes(const std::string & parvalue) {
    int mins = std::atoi(parvalue.c_str());
    if (mins<=0) {
        standard_exit_error(exit_bad_config_value, "Invalid chunk_minutes in configuration file", __func__);
    }
    fzu.config.chunk_seconds = (unsigned)mins*60;
    return (unsigned)mins;
}

unsigned int get_positive_integer(const std::string & parvalue, std::string errmsg) {
    int v = std::atoi(parvalue.c_str());
    if (v<=0) {
        standard_exit_error(exit_bad_config_value, "Invalid "+errmsg+" in configuration file", __func__);
    }
    return (unsigned)v;
}

/// Configure configurable parameters.
bool fzu_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(chunk_minutes, "chunk_minutes", parlabel, get_chunk_minutes(parvalue));
    CONFIG_TEST_AND_SET_PAR(map_multiplier, "map_multiplier", parlabel, get_positive_integer(parvalue, "map_multiplier"));
    CONFIG_TEST_AND_SET_PAR(map_days, "map_days", parlabel, get_positive_integer(parvalue, "map_days"));
    CONFIG_TEST_AND_SET_PAR(warn_repeating_too_tight,"warn_repeating_too_tight", parlabel, (parvalue != "false"));
    CONFIG_TEST_AND_SET_PAR(endofday_priorities,"endofday_priorities", parlabel, (parvalue != "false"));
    CONFIG_TEST_AND_SET_PAR(dolater_endofday,"dolater_endofday", parlabel, time_stamp_time(parvalue));
    CONFIG_TEST_AND_SET_PAR(doearlier_endofday,"doearlier_endofday", parlabel, time_stamp_time(parvalue));
    CONFIG_TEST_AND_SET_PAR(eps_group_offset_mins,"eps_group_offset_mins", parlabel, get_positive_integer(parvalue, "eps_group_offset_mins"));
    CONFIG_TEST_AND_SET_PAR(update_to_earlier_allowed,"update_to_earlier_allowed", parlabel, (parvalue != "false"));
    //CONFIG_TEST_AND_SET_FLAG(example_flagenablefunc, example_flagdisablefunc, "exampleflag", parlabel, parvalue);
    CONFIG_PAR_NOT_FOUND(parlabel);
}

/**
 * Initialize configuration parameters.
 * Call this at the top of main().
 * 
 * @param argc command line parameters count forwarded from main().
 * @param argv command line parameters array forwarded from main().
 */
void fzupdate::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

Graph & fzupdate::graph() {
    ERRTRACE;
    if (!graphmemman.get_Graph(graph_ptr)) {
        standard_exit_error(exit_resident_graph_missing, "Memory resident Graph not found.", __func__);
    }
    return *graph_ptr;
}

int update_repeating(time_t t_pass) {
    targetdate_sorted_Nodes incomplete_repeating = Nodes_incomplete_and_repeating_by_targetdate(fzu.graph());
    Edit_flags editflags;
    // *** BEWARE: Correct this to match the protocol decision about who gets to do what as per https://trello.com/c/mXHq1fLh.
    targetdate_sorted_Nodes updated_repeating = Update_repeating_Nodes(incomplete_repeating, t_pass, editflags);
    if (!editflags.None()) {
        // *** here, call the Graphpostgres function that can update multiple Nodes
    }
    return standard_exit_success("Update repeating done.");
}

void updvar_set_t_limit(time_t t_pass) {
    if (fzu.t_limit > 0) {
        if (fzu.t_limit <= t_pass) {
            standard_exit_error(exit_command_line_error, "Specified T limit "+TimeStampYmdHM(fzu.t_limit)+" is smaller than (emulated) current time "+TimeStampYmdHM(t_pass), __func__);
        }
        return; // command line limit overrides map_days
    }
    fzu.t_limit = time_add_day(t_pass,fzu.config.map_days);
}

std::vector<chunks_t> updvar_chunks_required(const targetdate_sorted_Nodes & nodelist) {
    std::vector<chunks_t> chunks_req(nodelist.size(), 0);
    chunks_t suspicious_req = (36*60)/fzu.config.chunk_minutes;
    size_t i = 0;
    for (const auto & [t, node_ptr] : nodelist) {
        chunks_req[i] = seconds_to_chunks(node_ptr->seconds_to_complete());
        if (chunks_req[i]>suspicious_req) {
            ADDWARNING(__func__, "Suspiciously large number of chunks ("+std::to_string(chunks_req[i])+") needed to complete Node "+node_ptr->get_id_str());
        }
        ++i;
    }
    return chunks_req;
}

unsigned long updvar_total_chunks_required_nonperiodic(const targetdate_sorted_Nodes & nodelist, const std::vector<chunks_t> & chunks_req) {
    // *** Note: The chunks_req[i] test may be unnecessary, because we're working with a list of incomplete Nodes.
    unsigned long total = 0;
    size_t i = 0;
    for (const auto & [t, node_ptr] : nodelist) {
        if (node_ptr->effective_targetdate()>fzu.t_limit) {
            break;
        }
        if ((chunks_req[i]>0) && (!node_ptr->get_repeats())) {
            total += chunks_req[i];
        }
        ++i;
    }
}

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
    void set_overlap() { eps_flags |= eps_mask::overlap; }
    void set_insufficient() { eps_flags |= eps_mask::insufficient; }
    void set_treatgroupable() { eps_flags |= eps_mask::treatgroupable; }
    void set_exact() { eps_flags |= eps_mask::exact; }
    void set_fixed() { eps_flags |= eps_mask::fixed; }
    void set_epsgroupmember() { eps_flags |= eps_mask::epsgroupmember; }
    void set_periodiclessthanyear() { eps_flags |= eps_mask::periodiclessthanyear; }
    bool EPS_overlap() const { return eps_flags & eps_mask::overlap; }
    bool EPS_insufficient() const { return eps_flags & eps_mask::insufficient; }
    bool EPS_treatgroupable() const { return eps_flags & eps_mask::treatgroupable; }
    bool EPS_exact() const { return eps_flags & eps_mask::exact; }
    bool EPS_fixed() const { return eps_flags & eps_mask::fixed; }
    bool EPS_epsgroupmember() const { return eps_flags & eps_mask::epsgroupmember; }
    bool EPS_periodiclessthanyear() const { return eps_flags & eps_mask::periodiclessthanyear; }
};

constexpr time_t seconds_per_day = 24*60*60;
constexpr time_t five_minutes_in_seconds = 5*60;

struct EPS_map_day {
    Node_ptr fivemin_slot[288] = { nullptr };
    time_t t_daystart;
    EPS_map_day(time_t t_start) : t_daystart(t_start) {}
    void init(time_t t_day) {
        t_daystart = t_day;
        fivemin_slot = { nullptr }; // *** do we need this again?
    }
    /**
     * Reserver 5 min slots immediately preceding exact target date.
     * @param n_ptr Pointer to Node for which to allocate slots.
     * @param slots Slots needed, returning any that did not fit into the day.
     * @param td Node target date (td<0 means used time from end of day).
     * @return True if any map slots were already occupied.
     */
    bool reserve_exact(Node_ptr n_ptr, int &slots, time_t td) {
        if (td < 0) {
            td = seconds_per_day;
        } else {
            td -= daytd; // seconds into the day
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
    int reserve_fixed(Node_ptr n_ptr, int & slots, time_t td) {
        int before_i = 288;
        if (td >= 0) {
            td -= daytd; // seconds into the day
            if ((td < 0) || (td > seconds_per_day))
                return 1;
            before_i = td / five_minutes_in_seconds;
        }
        for (int i = (before_i - 1); ((i >= 0) && (slots > 0)); i--) {
            if (!fivemin_slot[i]) {
                fivemin_slot[i] = n_ptr;
                slots--;
            } else if (fiveminchunk[i] == n_ptr)
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
    time_t reserve(Node_ptr n_ptr, int & slots) {
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
};

struct EPS_map {
    unsigned long num_days;
    time_t starttime;
    time_t firstdaystart;
    std::vector<EPS_map_day> days;
    int slots_per_chunk;
    time_t previous_group_td = -1;
    EPS_map(time_t t, unsigned long days_in_map) : num_days(days_in_map), starttime(t), slots_per_chunk(fzu.config.chunk_minutes/5) {
        if (days_in_map<1) {
            days_in_map = 1;
            num_days = 1;
        }
        firstdaystart = day_start_time(t);
        days.assign(days_in_map, firstdaystart);
        time_t daystart = firstdaystart;
        for (const auto & day : days) {
            day.init(daystart);
            daystart = time_add_day(daystart,1);
        }
        time_t seconds_passed = t - firstdaysstart;
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
    bool reserve_exact(Node_ptr n_ptr, int chunks_req, time_t td) {
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
    void place_exact(const targetdate_sorted_Nodes & nodelist, const std::vector<chunks_t> & chunks_req, std::vector<EPS_flags> & epsflags_vec) {
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
    int find_dayTD(time_t td) {
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
    bool reserve_fixed(Node_ptr n_ptr, int chunks_req, time_t td) {
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
    void place_fixed(const targetdate_sorted_Nodes & nodelist, const std::vector<chunks_t> & chunks_req, std::vector<EPS_flags> & epsflags_vec) {
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
                    epsmarks[i] |= INSUFFICIENTFLAG;
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
    time_t reserve(Node_ptr n_ptr, int chunks_req) {
        int slots = chunks_req * slots_per_chunk;
        int td_day = 0;
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
            td_day--; // back to the last day accessed
            time_t timeofday = epstd - days[td_day].t_daystart;
            if (timeofday < priorityendofday) { // set to priorityendofday on this day
                epstd = days[td_day].t_daystart + priorityendofday;
            } else { // set to priorityendofday on next day
                epstd = time_add_day(days[td_day].t_daystart, 1) + priorityendofday;
            }

            // check if a previous group already has the target date, if so offset slightly to preserve grouping and group order
            if (epstd <= previous_group_td)
                epstd = previous_group_td + eps_group_offset_mins; // one or a few minutes offset
        }
        return epstd;
    }
    void group_and_place_movable(const targetdate_sorted_Nodes & nodelist, const std::vector<chunks_t> & chunks_req, std::vector<EPS_flags> & epsflags_vec, std::vector<time_t> & t_eps) {
        struct nodelist_itidx {
            ssize_t index;
            targetdate_sorted_Nodes::iterator it;
            nodelist_ididx(ssize_t _i, targetdate_sorted_Nodes::iterator _it) : index(_i), it(_it) {}
        };
        nodelist_itidx group_first(-1, nodelist.end());
        nodelist_ididx group_last(-1, nodelist.end());
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
                    if (group_first >= 0) {
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

                    group_first = i; // *** maybe this needs to be an iterator or a pair of index and iterator
                    group_td = t;
                }

                group_last = i; // mark in case there are non-group entries between firsts in this group and the next
                epsflags_vec[i].set_epsgroupmember();
            }

            ++i;
        }
    }

    targetdate_sorted_Nodes get_eps_update_nodes(const targetdate_sorted_Nodes & nodelist, const std::vector<chunks_t> & chunks_req, std::vector<EPS_flags> & epsflags_vec, std::vector<time_t> & t_eps) {
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
                    if (fze.config.update_to_earlier_allowed || (t_eps[i] > group_td)) { 
                        // THIS is a Node for which the target date should be updated to t_eps[i]
                        update_nodes.emplace(t_eps[i], node_ptr);
                    }
                }
            }

            ++i;
        }
        return update_nodes;
    }
};

int update_variable(time_t t_pass) {
    constexpr time_t seconds_per_week = 7*24*60*60;
    targetdate_sorted_Nodes incomplete_repeating = Nodes_incomplete_and_repeating_by_targetdate(fzu.graph());
    Edit_flags editflags;

    updvar_set_t_limit(t_pass);

    std::vector<time_t> t_eps(incomplete_repeating.size(), RTt_maxtime);
    std::vector<EPS_flags> epsflags_vec;

    std::vector<chunks_t> chunks_req = updvar_chunks_required(incomplete_repeating);
    unsigned long chunks_req_total = updvar_total_chunks_required_nonperiodic(incomplete_repeating, chunks_req);
    unsigned long chunks_per_week = seconds_per_week / ((unsigned long)fzu.chunk_minutes * 60);
    unsigned long weeks_for_nonperiodic = t_req_total / chunks_per_week;
    unsigned long days_in_map = weeks_for_nonperiodic * 7 * fzu.map_multiplier;
    if (fzu.map_days > days_in_map) {
        days_in_map = fzu.map_days;
    }

    EPS_map updvar_map(t_pass, days_in_map); // *** putting all the call parameters below into the updvar_map structure can clean things up a bit
    updvar_map.place_exact(incomplete_repeating, chunks_req, epsflags_vec);
    updvar_map.place_fixed(incomplete_repeating, chunks_req, epsflags_vec);
    updvar_map.group_and_place_movable(incomplete_repeating, chunks_req, epsflags_vec, t_eps);

    targetdate_sorted_Nodes eps_update_nodes = updvar_map.get_eps_update_nodes(incomplete_repeating, chunks_req, epsflags_vec, t_eps);

    // *** Now build the Graph modification stack expected using eps_upate_nodes with editflags set to targetdate
    // *** BEWARE: Correct this to match the protocol decision about who gets to do what as per https://trello.com/c/mXHq1fLh.

    // *** You can probably copy the method used in dil2al, because placing all of the immovables in a fine-grained
    //     map and then putting the variable target date Nodes where they have to be was already a good strategy.
    //     The dil2al function alcomp.cc:generate_AL_CRT() applies a method, including options such as EPS grouping
    //     and determines which modified target dates to propose for variable target date tasks.
    FZOUT("NOT YET IMPLEMENTED!\n");

    if (!editflags.None()) { // *** probably need a different test here
        // *** here, call the Graphpostgres function that can update multiple Nodes
    }

    return standard_exit_success("Update variable target date Nodes done.");
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzu.init_top(argc, argv);

    FZOUT("\nThis is a stub.\n\n");
    key_pause();

    switch (fzu.flowcontrol) {

    case flow_update_repeating: {
        return update_repeating(fzu.reftime.Time());
    }

    case flow_update_variable: {
        return update_variable(fzu.reftime.Time());
    }

    // *** NOTE: If we combine and carry out both repeating and variable updates then there is no need to
    //     write changes to database until all targetdates have been updated by both methods and the full
    //     list of Nodes with set Edit_flags is known.
    //     We also don't need to collect the incomplete_repeating list twice.

    default: {
        fzu.print_usage();
    }

    }

    return standard.completed_ok();
}
