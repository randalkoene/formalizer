// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Node Board rendering functions.
 * 
 * For more about this, see the Trello card at https://trello.com/c/w2XnEQcc
 */

#define FORMALIZER_MODULE_ID "Formalizer:Visualization:Nodes:schedule"

#include "error.hpp"
#include "general.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "stringio.hpp"
#include "schedule.hpp"

using namespace fz;

schedule fzsch;

const char * strategy1 = R"STRATEGY(
Strategy 1:
1. Exact target date Nodes are scheduled into their appointed time-interval.
   Where they overlap, Nodes processed later (in temporal order) override
   Nodes processed earlier.
2. Fixed target date Nodes are scheduled into the latest minutes available
   before their target date.
3. Remaining available time is filled from earliest to latest minute by
   allocating minutes to variable target date Nodes. The variable target
   date Nodes are fully allocated. There is no Node-switching while a
   Node has not been completed. There is no shuffling of Nodes. And
   prioritization is done entirely based on target date and Node ID order,
   i.e. the order in which the variable target date Nodes appear in the
   Schedule.

Problems / to fix:
- Inherited target date Nodes are not properly dealt with.
)STRATEGY";

schedule::schedule():
    formalizer_standard_program(false),
    ga(*this, add_option_args, add_usage_top),
    flowcontrol(flow_unknown),
    graph_ptr(nullptr) {

    add_option_args += "HwcWS:D:b:efxo:";
    add_usage_top += " <-H|-w|-c|-W> [-S <strategy>] [-D <num_days>] [-b <min_minutes>] [-e] [-f] [-x] [-o <output-file|STDOUT>]";

    thisdatetime = ActualTime();
    thisdate = DateStampYmd(thisdatetime);
    thistimeofday = time_of_day(thisdatetime);
    thisminutes = thistimeofday.num_minutes();
    t_today_start = today_start_time();
}

void schedule::usage_hook() {
    ga.usage_hook();
    FZOUT(
        "    -H HTML output.\n"
        "    -w HTML output for web page.\n"
        "    -c CSV output.\n"
        "    -W CSV output for use in web app.\n"
        "    -S Use strategy:\n"
        "       f_late_v_early (default), f_late_v_early_2\n"
        "    -D Number of days to schedule (default: 1).\n"
        "    -b Minimum block size in minutes (default: 1).\n"
        "    -e Exclude exact target date Nodes.\n"
        "    -f Exclude fixed target date Nodes.\n"
        "    -x Exclude variable target date Nodes.\n"
        "    -o Output to file (or STDOUT).\n"
        "       (Default depends on output target.)\n"
        "\n"
    );
}

bool schedule::options_hook(char c, std::string cargs) {

    if (ga.options_hook(c,cargs))
        return true;

    
    switch (c) {
    
        case 'H': {
            flowcontrol = flow_html;
            return true;
        }

        case 'w': {
            flowcontrol = flow_html_for_web;
            return true;
        }

        case 'c': {
            flowcontrol = flow_csv;
            return true;
        }

        case 'W': {
            flowcontrol = flow_csv_for_web;
            return true;
        }

        case 'S': {
            if (cargs == "f_late_v_early") {
                strategy = fixed_late_variable_early_strategy;
            } else if (cargs == "f_late_v_early_2") {
                strategy = fixed_late_variable_early_from_sorted_strategy;
            }
            return true;
        }

        case 'D': {
            num_days = atoi(cargs.c_str());
            if (num_days < 1) {
                num_days = 1;
            }
            return true;
        }

        case 'b': {
            min_block_size = atoi(cargs.c_str());
            if (min_block_size < 1) {
                min_block_size = 1;
            }
            return true;
        }

        case 'e': {
            inc_exact = false;
            return true;
        }

        case 'f': {
            inc_fixed = false;
            return true;
        }

        case 'x': {
            inc_variable = false;
            return true;
        }

        case 'o': {
            output_path = cargs;
            return true;
        }

    }

    return false;
}

Graph & schedule::graph() { 
    if (graph_ptr) {
        return *graph_ptr;
    }
    graph_ptr = graphmemman.find_Graph_in_shared_memory();
    if (!graph_ptr) {
        standard_exit_error(exit_general_error, "Memory resident Graph not found.", __func__);
    }
    return *graph_ptr;
}

bool schedule::to_output(const std::string & rendered_schedule) {
    if (output_path=="STDOUT") {
        FZOUT(rendered_schedule);
        return true;
    }

    if (!string_to_file(output_path, rendered_schedule)) {
        ERRRETURNFALSE(__func__,"unable to write rendered schedule to "+output_path);
    }
    FZOUT("Schedule rendered to "+output_path+".\n");
    return true;
}

// In schedule.py, this uses a call to fzgraphhtml.
bool schedule::get_schedule_data(unsigned int get_num_days) {
    std::string num_days_str = std::to_string(get_num_days);
    VERBOSEOUT("Fetching Schedule data for "+num_days_str+" days.\n");

    if (incomplete_nodes_cache.empty()) {
        incomplete_nodes_cache = Nodes_incomplete_by_targetdate(graph());
    }
    size_t num_to_show = std::numeric_limits<unsigned int>::max(); // Disabling this limit.
    time_t t_max = ActualTime() + get_num_days*24*60*60;
    incnodes_with_repeats = Nodes_with_repeats_by_targetdate(incomplete_nodes_cache, t_max, num_to_show);
    schedule_size = (num_to_show > incnodes_with_repeats.size()) ? incnodes_with_repeats.size() : num_to_show;

    return true;
}

bool schedule::convert_to_data_by_day() {
    // Start to collect a list of entries for day 0.
    unsigned long day = 0;
    day_entries_uptr day_entries = std::make_unique<Day_Entries>();

    // Go through the sorted Nodes and their repeats in targetdate order.
    unsigned int num_schedule = schedule_size;
    VERBOSEOUT("Number of Nodes and repeats to assign to days: "+std::to_string(num_schedule)+'\n');
    for (const auto & [tdate, node_ptr] : incnodes_with_repeats) {

        if (node_ptr) {
            // Does the next entry still belong to the same day (use tdate, not node_ptr->get_targetdate())?
            unsigned long entry_day = date_as_ulong(tdate);
            if (entry_day != day) {

                // Move this completed day list to the days collection.
                if (day_entries->size() > 0) {
                    days[day].reset(day_entries.release());
                }

                // Start a new day list and switch to the next day.
                day_entries = std::make_unique<Day_Entries>();
                day = entry_day;

            }

            // Add the next entry Node to the active day list.
            day_entries->append(node_ptr);
        }

        // Continue until the specified limit is reached.
        if (--num_schedule == 0)
            break;

        t_last_scheduled = tdate; // *** Could remove this.
    }

    // Move the final day list to the days collection.
    if (day_entries->size() > 0) {
        days[day].reset(day_entries.release());
    }

    VERBOSEOUT("Found "+std::to_string(days.size())+" days.\n");
    for (const auto & [ day_date, day_entries ] : days) {
        VERBOSEOUT("Date: "+std::to_string(day_date)+" - "+std::to_string(day_entries->size())+" entries.\n");
    }
    return true;
}

bool schedule::initialize_daymap() {
    if (strategy == fixed_late_variable_early_strategy) {
        total_minutes = days.size()*24*60;
    } else {
        total_minutes = (get_estimated_offset_day(incnodes_with_repeats.rbegin()->first)+1)*24*60;
    }
    
    VERBOSEOUT("Number of minutes in daysmap: "+std::to_string(total_minutes)+'\n');
    daysmap.resize(total_minutes);
    /*
        thisday_int = int(thisdate)
        firstday_int = int(list(days.keys())[0])
        passed_days = firstday_int - thisday_int
        passed_minutes = 0
        if passed_days > 0: passed_minutes = 60*24*passed_days
    */
    passed_minutes += thisminutes;
    if (passed_minutes > 0) {
        VERBOSEOUT("Passed minutes: "+std::to_string(passed_minutes)+'\n');
    }
    return true;
}

/**
 * Note: When an exact target date Node has been partially completed, i.e.
 *       minutes_to_complete() is less than get_required_minutes(), then,
 *       if the current time is later than the start time of the exact
 *       time interval, then it makes sense that the remaining time belongs
 *       at the end of the interval. Otherwise, we can map it to the start
 *       of the interval (e.g. to complete a meeting early).
 */
bool schedule::map_exact_target_date_entries() {
    exact_consumed = 0;
    unsigned int mapped_day_count = 0;
    for (const auto & [ day, day_entries ] : days) {
        for (const auto & node_ptr : day_entries->entries) {
            if (node_ptr) {
                // VERBOSEOUT(node_ptr->get_id_str()+'\n');
                if (node_ptr->td_exact()) {
                    // Tag indexed map entries with Node key.
                    auto nkey = node_ptr->get_id().key();
                    unsigned int original_req_minutes = node_ptr->get_required_minutes();
                    unsigned int num_minutes = node_ptr->minutes_to_complete();
                    time_of_day_t timeofday = node_ptr->get_targetdate_timeofday(); // *** May not work for unspecified/inherited.
                    unsigned long td_minute_index = (mapped_day_count*60*24) + (timeofday.hour*60) + timeofday.minute;
                    unsigned long td_start_index = td_minute_index - num_minutes; // From end of exact time interval.
                    if (num_minutes < original_req_minutes) {
                        if (thisdatetime < (node_ptr->effective_targetdate()-(60*original_req_minutes))) {
                            td_start_index = td_minute_index - original_req_minutes; // From start of exact time interval.
                            td_minute_index = td_start_index + num_minutes;
                        }
                    }
                    if (td_start_index < passed_minutes) {
                        td_start_index = passed_minutes;
                    }
                    // VERBOSE("EXACT "+node_ptr->get_id_str()+": "+std::to_string(td_start_index)+" - "+std::to_string(td_minute_index)+'\n');
                    exact_consumed += daysmap.fill(td_start_index, td_minute_index, nkey);
                }
            }
        }
        mapped_day_count++;
    }
    VERBOSEOUT("Minutes consumed by exact target date Nodes: "+std::to_string(exact_consumed)+'\n');
    return true;
}

// *** TODO: This does may not work entirely correctly around daylight savings changes.
unsigned int schedule::get_estimated_offset_day(time_t t) {
    unsigned long seconds = t - t_today_start;
    return seconds / 86400;
}

// This uses the same strategy as above, but obtains data directly from
// the incnode_with_repeats list.
bool schedule::map_exact_target_date_entries_from_sorted() {
    exact_consumed = 0;

    unsigned int num_schedule = schedule_size;
    for (const auto & [tdate, node_ptr] : incnodes_with_repeats) {
        if (node_ptr->td_exact() && (tdate >= thisdatetime)) {

            // Tag indexed map entries with Node key.
            auto nkey = node_ptr->get_id().key();
            unsigned int original_req_minutes = node_ptr->get_required_minutes();
            unsigned int num_minutes = node_ptr->minutes_to_complete(tdate); // This takes into account if this is the first instance or a repeat.
            time_of_day_t timeofday = node_ptr->get_targetdate_timeofday(); // *** May not work for unspecified/inherited.
            unsigned long td_minute_index = (get_estimated_offset_day(tdate)*60*24) + (timeofday.hour*60) + timeofday.minute;
            unsigned long td_start_index = td_minute_index - num_minutes;
            if (num_minutes < original_req_minutes) {
                if (thisdatetime < (node_ptr->effective_targetdate()-(60*original_req_minutes))) {
                    td_start_index = td_minute_index - original_req_minutes; // From start of exact time interval.
                    td_minute_index = td_start_index + num_minutes;
                }
            }
            if (td_start_index < passed_minutes) {
                td_start_index = passed_minutes;
            }
            exact_consumed += daysmap.fill(td_start_index, td_minute_index, nkey);

        }

        // Continue until the specified limit is reached.
        if (--num_schedule == 0)
            break;
    }
    VERBOSEOUT("Minutes consumed by exact target date Nodes: "+std::to_string(exact_consumed)+'\n');
    return true;
}


bool schedule::min_block_available_backwards(unsigned long idx, int next_grab) {
    while (idx >= passed_minutes) {
        if (!daysmap.at(idx).isnullkey()) {
            return false;
        }
        next_grab--;
        if (next_grab < 1) {
            return true;
        }
        idx--;
    }
    return false;
}

unsigned long schedule::set_block_to_node_backwards(unsigned long idx, int next_grab, Node_ID_key nkey) {
    while (next_grab > 0) {
        daysmap.at(idx) = nkey;
        idx--;
        next_grab--;
    }
    return idx;
}

// See README.md.
bool include_in_fixed_td_step(Node & node) {
    if (node.td_fixed()) return true;
    if (node.td_unspecified()) return false;
    if (!node.td_inherit()) return false; // takes care of td_variable() and td_exact()

    // Find the origin Node for the targetdate used.
    Node_ptr origin = nullptr;
    node.effective_targetdate(&origin);
    if ((origin==nullptr) || (origin==&node)) return false;
    if (origin->td_exact() || origin->td_fixed()) return true;
    return false;
}

// For each fixed target date entry, find the latest available minute
// and start filling back from there.
bool schedule::map_fixed_target_date_entries_late() {
    fixed_consumed = 0;
    unsigned int mapped_day_count = 0;
    for (const auto & [ day, day_entries ] : days) {
        for (const auto & node_ptr : day_entries->entries) {
            if (node_ptr) {
                if (include_in_fixed_td_step(*node_ptr)) {
                    auto nkey = node_ptr->get_id().key();
                    int num_minutes = node_ptr->minutes_to_complete(); // *** This does not seem to be able to discern first instance or repeat (as tdate is not available).
                    time_of_day_t timeofday = node_ptr->get_targetdate_timeofday();
                    unsigned long latest_td_minute_index = (mapped_day_count*60*24) + (timeofday.hour*60) + timeofday.minute;
                    // Find blocks starting at the latest possible index.
                    unsigned long idx = latest_td_minute_index;
                    while ((idx >= passed_minutes) && (num_minutes > 0)) {
                        int next_grab = (num_minutes < min_block_size) ? num_minutes : min_block_size;
                        if (min_block_available_backwards(idx, next_grab)) {
                            idx = set_block_to_node_backwards(idx, next_grab, nkey);
                            num_minutes -= next_grab;
                            fixed_consumed += next_grab;
                        } else {
                            idx--;
                        }
                    }
                }
            }
        }
        mapped_day_count++;
    }
    VERBOSEOUT("Minutes consumed by fixed target date Nodes: "+std::to_string(fixed_consumed)+'\n');
    return true;
}

bool schedule::map_fixed_target_date_entries_late_from_sorted() {
    fixed_consumed = 0;

    unsigned int num_schedule = schedule_size;
    for (const auto & [tdate, node_ptr] : incnodes_with_repeats) {
        if (include_in_fixed_td_step(*node_ptr) && (tdate >= thisdatetime)) {

            auto nkey = node_ptr->get_id().key();
            int num_minutes = node_ptr->minutes_to_complete(tdate); // This takes into account if this is the first instance or a repeat.
            time_of_day_t timeofday = node_ptr->get_targetdate_timeofday();
            unsigned long latest_td_minute_index = (get_estimated_offset_day(tdate)*60*24) + (timeofday.hour*60) + timeofday.minute;
            // Find blocks starting at the latest possible index.
            if (latest_td_minute_index < daysmap.size()) {
                unsigned long idx = latest_td_minute_index;
                while ((idx >= passed_minutes) && (num_minutes > 0)) {
                    int next_grab = (num_minutes < min_block_size) ? num_minutes : min_block_size;
                    if (min_block_available_backwards(idx, next_grab)) {
                        idx = set_block_to_node_backwards(idx, next_grab, nkey);
                        num_minutes -= next_grab;
                        fixed_consumed += next_grab;
                    } else {
                        idx--;
                    }
                }
            }
        }

        // Continue until the specified limit is reached.
        if (--num_schedule == 0)
            break;
    }
    VERBOSEOUT("Minutes consumed by fixed target date Nodes: "+std::to_string(fixed_consumed)+'\n');
    return true;
}

bool schedule::min_block_available_forwards(unsigned long idx, int next_grab) {
    while (idx < daysmap.size()) {
        if (!daysmap.at(idx).isnullkey()) {
            return false;
        }
        next_grab--;
        if (next_grab < 1) {
            return true;
        }
        idx++;
    }
    return false;
}

unsigned long schedule::set_block_to_node_forwards(unsigned long idx, int next_grab, Node_ID_key nkey) {
    while (next_grab > 0) {
        daysmap.at(idx) = nkey;
        idx++;
        next_grab--;
    }
    return idx;
}

// See README.md.
bool include_in_variable_td_step(Node & node) {
    if (node.td_variable()) return true;
    if (node.td_unspecified()) return true;
    if (!node.td_inherit()) return false; // takes care of td_fixed() and td_exact()

    // Find the origin Node for the targetdate used.
    Node_ptr origin = nullptr;
    node.effective_targetdate(&origin);
    if ((origin==nullptr) || (origin==&node)) return true;
    if (origin->td_exact() || origin->td_fixed()) return false;
    return true;
}

bool schedule::map_variable_target_date_entries_early(unsigned int start_at) {
    variable_consumed = 0;
    unsigned int mapped_day_count = 0;
    for (const auto & [ day, day_entries ] : days) {
        if (start_at > 0) {
            start_at--;
            continue;
        }
        for (const auto & node_ptr : day_entries->entries) {
            if (node_ptr) {
                if (include_in_variable_td_step(*node_ptr)) {
                    auto nkey = node_ptr->get_id().key();
                    int num_minutes = node_ptr->minutes_to_complete();
                    //time_of_day_t timeofday = node_ptr->get_targetdate_timeofday();
                    //unsigned long latest_td_minute_index = (mapped_day_count*60*24) + (timeofday.hour*60) + timeofday.minute;
                    // Find blocks starting at the earliest possible index.
                    unsigned long idx = passed_minutes;
                    while ((idx < daysmap.size()) && (num_minutes > 0)) {
                        int next_grab = (num_minutes < min_block_size) ? num_minutes : min_block_size;
                        if (min_block_available_forwards(idx, next_grab)) {
                            idx = set_block_to_node_forwards(idx, next_grab, nkey);
                            num_minutes -= next_grab;
                            variable_consumed += next_grab;
                        } else {
                            idx++;
                        }
                    }
                }
            }
        }
        mapped_day_count++;
    }
    VERBOSEOUT("Minutes consumed by variable target date Nodes: "+std::to_string(variable_consumed)+'\n');
    return true;
}

bool schedule::get_and_map_more_variable_target_date_entries(unsigned long remaining_minutes) {
    unsigned int num_mapped_days = days.size();
    unsigned long initial_num_entries = incnodes_with_repeats.size();
    // Find more variable target date Nodes in chunks of 15 days.
    unsigned int local_num_days = 15;
    while (true) {
        VERBOSEOUT("Getting more variable target date Nodes in "+std::to_string(local_num_days)+" days.\n");
        if (!get_schedule_data(local_num_days)) {
            ERRRETURNFALSE(__func__, "failed to get "+std::to_string(local_num_days)+" days of schedule data.");
        }
        // *** TODO: Could be more efficient if we can safely skip what was
        //           already converted.
        if (!convert_to_data_by_day()) {
            ERRRETURNFALSE(__func__, "failed to convert additional data to an arrangement by days.");
        }
        // Skip entries already processed.
        auto it = incnodes_with_repeats.begin();
        for (unsigned int i = 0; i < initial_num_entries; ++i) {
            it++;
        }
        // Find more variable target date Node minutes in additional entries.
        unsigned long more_minutes = 0;
        while (true) {
            Node * node_ptr = it->second;
            if (node_ptr) {
                if (include_in_variable_td_step(*node_ptr)) {
                    more_minutes += node_ptr->minutes_to_complete();
                    if (more_minutes >= remaining_minutes) {
                        VERBOSEOUT("Additional variable target date Node minutes found: "+std::to_string(more_minutes)+'\n');
                        return map_variable_target_date_entries_early(num_mapped_days);
                    }
                }
            }
            it++;
            if (it == incnodes_with_repeats.end()) {
                break; // Try even more days to find sufficient extra minutes.
            }
        }
        local_num_days += 15;
        if (local_num_days > 150) {
            VERBOSEOUT("WARNING: Unable to fill all remaining minutes with variable target date Nodes within 150 days."
                       "Additional variable target date Node minutes found: "+std::to_string(more_minutes)+'\n');
            return map_variable_target_date_entries_early(num_mapped_days);
        }
    }
    return true;
}

bool schedule::map_variable_target_date_entries_early_from_sorted() {
    variable_consumed = 0;

    unsigned int num_schedule = schedule_size;
    for (const auto & [tdate, node_ptr] : incnodes_with_repeats) {
        if (include_in_variable_td_step(*node_ptr)) {

            auto nkey = node_ptr->get_id().key();
            int num_minutes = node_ptr->minutes_to_complete();
            unsigned long idx = passed_minutes;
            while ((idx < daysmap.size()) && (num_minutes > 0)) {
                int next_grab = (num_minutes < min_block_size) ? num_minutes : min_block_size;
                if (min_block_available_forwards(idx, next_grab)) {
                    idx = set_block_to_node_forwards(idx, next_grab, nkey);
                    num_minutes -= next_grab;
                    variable_consumed += next_grab;
                } else {
                    idx++;
                }
            }

        }

        // Continue until the specified limit is reached.
        if (--num_schedule == 0)
            break;
    }
    VERBOSEOUT("Minutes consumed by variable target date Nodes: "+std::to_string(variable_consumed)+'\n');
    return true;
}

bool schedule::generate_schedule() {
    switch (strategy) {
        case fixed_late_variable_early_strategy: {
            VERBOSEOUT(strategy1);
            if (!get_schedule_data(num_days)) {
                return false;
            }
            if (!convert_to_data_by_day()) {
                return false;
            }
            if (!initialize_daymap()) {
                return false;
            }
            if (inc_exact) {
                if (!map_exact_target_date_entries()) {
                    return false;
                }
            }
            if (inc_fixed) {
                if (!map_fixed_target_date_entries_late()) {
                    return false;
                }
            }
            if (inc_variable) {
                if (!map_variable_target_date_entries_early()) {
                    return false;
                }
                unsigned long remaining_minutes = total_minutes - exact_consumed - fixed_consumed - variable_consumed - passed_minutes;
                VERBOSEOUT("Remaining minutes to fill with variable target date entries: "+std::to_string(remaining_minutes)+'\n');
                if (remaining_minutes > 0) {
                    if (!get_and_map_more_variable_target_date_entries(remaining_minutes)) {
                        return false;
                    }
                }
            }
            return true;
        }
        case fixed_late_variable_early_from_sorted_strategy: {
            VERBOSEOUT(strategy1);
            if (!get_schedule_data(num_days)) {
                return false;
            }
            if (!initialize_daymap()) {
                return false;
            }
            if (inc_exact) {
                if (!map_exact_target_date_entries_from_sorted()) {
                    return false;
                }
            }
            if (inc_fixed) {
                if (!map_fixed_target_date_entries_late_from_sorted()) {
                    return false;
                }
            }
            if (inc_variable) {
                if (!map_variable_target_date_entries_early_from_sorted()) {
                    return false;
                }
            }
            return true;
        }
        default: {
            ERRRETURNFALSE(__func__, "unknown scheduling strategy.");
        }
    }
}

bool schedule_html() {
    return standard_exit(schedule_render_html(fzsch), "HTML schedule created.\n", exit_general_error, "Unable to create HTML schedule.", __func__);
}

bool schedule_html_for_web() {
    return standard_exit(schedule_render_html(fzsch), "HTML for web schedule created.\n", exit_general_error, "Unable to create HTML schedule for web.", __func__);
}

bool schedule_csv() {
    return standard_exit(schedule_render_csv(fzsch), "CSV schedule created.\n", exit_general_error, "Unable to create CSV schedule.", __func__);
}

bool schedule_csv_for_web() {
    return standard_exit(schedule_render_csv(fzsch), "CSV for web schedule created.\n", exit_general_error, "Unable to create CSV schedule for web.", __func__);
}

int main(int argc, char *argv[]) {
    fzsch.init(argc,argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);

    switch (fzsch.flowcontrol) {

        case flow_html: {
            schedule_html();
            break;
        }

        case flow_html_for_web: {
            schedule_html_for_web();
            break;
        }

        case flow_csv: {
            schedule_csv();
            break;
        }

        case flow_csv_for_web: {
            schedule_csv_for_web();
            break;
        }

        default: {
            fzsch.print_usage();
        }

    }

    return standard.completed_ok();
}
