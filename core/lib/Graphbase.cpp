// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <iomanip>

// core
//#include "general.hpp"
//#include "utf8.hpp"
#include "Graphbase.hpp"
#include "Graphtypes.hpp"

// Boost libraries need the following.
#pragma GCC diagnostic warning "-Wuninitialized"

namespace fz {

const std::string td_property_str[_tdprop_num] = {
    "unspecified",
    "inherit",
    "variable",
    "fixed",
    "exact"
};
const std::map<std::string, td_property> td_property_map = {
    {"unspecified", td_property::unspecified},
    {"inherit", td_property::inherit},
    {"variable", td_property::variable},
    {"fixed", td_property::fixed},
    {"exact", td_property::exact}
};

const std::string td_pattern_str[_patt_num] =  {
    "patt_daily",
    "patt_workdays",
    "patt_weekly",
    "patt_biweekly",
    "patt_monthly",
    "patt_endofmonthoffset",
    "patt_yearly",
    "OLD_patt_span",
    "patt_nonperiodic"
};
const std::map<std::string, td_pattern> td_pattern_map = {
    {"daily", td_pattern::patt_daily},
    {"workdays", td_pattern::patt_workdays},
    {"weekly", td_pattern::patt_weekly},
    {"biweekly", td_pattern::patt_biweekly},
    {"monthly", td_pattern::patt_monthly},
    {"endofmonthoffset", td_pattern::patt_endofmonthoffset},
    {"yearly", td_pattern::patt_yearly},
    {"OLD_span", td_pattern::OLD_patt_span},
    {"nonperiodic", td_pattern::patt_nonperiodic},
};

/**
 * Convert a target date sorted multimap of Node pointers into a string of
 * Node ID strings as comma separated values.
 * 
 * @param[in] sorted_nodes The multimap of Node pointers sorted by a target date key.
 * @param[out] csv_str The string object reference for the resulting comma separated values.
 */
void tdsorted_Nodes_to_csv(const targetdate_sorted_Nodes & sorted_nodes, std::string & csv_str) {
    for (const auto & [t, n_ptr]: sorted_nodes) {
        if (!csv_str.empty()) {
            csv_str += ',';
        }
        csv_str += n_ptr->get_id_str();
    }
}

#define VALID_NODE_ID_FAIL(f) \
    {                         \
        formerror = f;        \
        return false;         \
    }

/**
 * Test if a ID_TimeStamp can be used as a valid Node_ID.
 * 
 * Note that years before 1999 are disqualified,
 * since the Formalizer did not exist before then.
 * 
 * @param idT reference to an ID_TimeStamp object.
 * @param formerror a string that collects specific error information if there is any.
 * @return true if valid.
 */
bool valid_Node_ID(const ID_TimeStamp &idT, std::string &formerror) {
    if (idT.year < 1999)
        VALID_NODE_ID_FAIL("year");
    if ((idT.month < 1) || (idT.month > 12))
        VALID_NODE_ID_FAIL("month");
    if ((idT.day < 1) || (idT.day > 31))
        VALID_NODE_ID_FAIL("day");
    if (idT.hour > 23)
        VALID_NODE_ID_FAIL("hour");
    if (idT.minute > 59)
        VALID_NODE_ID_FAIL("minute");
    if (idT.second > 59)
        VALID_NODE_ID_FAIL("second");
    if (idT.minor_id < 1)
        VALID_NODE_ID_FAIL("minor_id");
    return true;
}

/**
 * Test if a string can be used to form a valid Node_ID.
 * 
 * Checks string length, period separating time stamp from minor ID,
 * all digits in time stamp and minor ID, and time stamp components
 * within valid ranges. Note that years before 1999 are disqualified,
 * since the Formalizer did not exist before then.
 * 
 * @param id_str a string of the format YYYYmmddHHMMSS.num.
 * @param formerror a string that collects specific error information if there is any.
 * @param id_timestamp if not NULL, receives valid components.
 * @return true if valid.
 */
bool valid_Node_ID(std::string id_str, std::string &formerror, ID_TimeStamp *id_timestamp) {

    if (id_str.length() < NODE_ID_STR_NUMCHARS)
        VALID_NODE_ID_FAIL("string size: "+id_str);
    if (id_str[14] != '.')
        VALID_NODE_ID_FAIL("format: "+id_str);
    for (int i = 0; i < 14; i++)
        if (!isdigit(id_str[i]))
            VALID_NODE_ID_FAIL("digits: "+id_str);

    ID_TimeStamp idT;
    idT.year = stoi(id_str.substr(0, 4));
    idT.month = stoi(id_str.substr(4, 2));
    idT.day = stoi(id_str.substr(6, 2));
    idT.hour = stoi(id_str.substr(8, 2));
    idT.minute = stoi(id_str.substr(10, 2));
    idT.second = stoi(id_str.substr(12, 2));
    idT.minor_id = stoi(id_str.substr(15));
    if (!valid_Node_ID(idT, formerror))
        return false;

    if (id_timestamp)
        *id_timestamp = idT;
    return true;
}

std::string Node_ID_TimeStamp_to_string(const ID_TimeStamp idT) {
    std::stringstream ss;
    ss << std::setfill('0')
       << std::setw(4) << (int) idT.year
       << std::setw(2) << (int) idT.month
       << std::setw(2) << (int) idT.day
       << std::setw(2) << (int) idT.hour
       << std::setw(2) << (int) idT.minute
       << std::setw(2) << (int) idT.second
       << '.' << std::setw(1) << (int) idT.minor_id;
    return ss.str();
}

/**
 * Create a usable Node ID TimeStamp from an epoch time and a
 * minor-ID.
 * 
 * This can also be used to produce valid Node ID TimeStamps without
 * the node-ID extension by specifying `minor_id = 0`. (See for
 * example how that is used in `fzaddnode` to generate a base for
 * a Node ID before choosing the minor-ID.)
 * 
 * Valid epoch times must be greater than 1999-01-01 00:00:00.
 * If `throw_if_invalid` is true then an invalid epoch time
 * causes ID_exception to be thrown.
 * 
 * @param t A valid epoch time for a Node ID.
 * @param minor_id A single-digit minor-ID (0 means time stamp only).
 * @param throw_if_invalid If true then throw ID_exception for invalid specifications.
 * @return A string with a valid Node ID TimeStamp, including minor-ID if given (or empty if invalid).
 */
std::string Node_ID_TimeStamp_from_epochtime(time_t t, uint8_t minor_id, bool throw_if_invalid) {
    if ((t < NODE_ID_FIRST_VALID_TEPOCH) || (minor_id > 9)) {
        if (throw_if_invalid) {
            std::string formerror(" with epoch time ("+std::to_string(t)+") that converts to invalid Node ID\n");
            throw(ID_exception(formerror));
        }
        return "";
    }

    std::string nodeid_str = TimeStamp("%Y%m%d%H%M%S", t);
    if (minor_id > 0) {
        nodeid_str += '.' + std::to_string(minor_id);
    }
    return nodeid_str;
}

// Add to a date-time in accordance with a repeating pattern.
time_t Add_to_Date(time_t t, td_pattern pattern, int every) {
    if (every < 1) {
        every = 1;
    }

    switch (pattern) {
        case patt_daily: {
            return time_add_day(t, every);
        }

        case patt_workdays: {
            for ( ; every>0; --every) {
                if (time_day_of_week(t)<5) {
                    t = time_add_day(t);
                } else {
                    t = time_add_day(t, 3);
                }
            }
            return t;
        }

        case patt_weekly: {
            return time_add_day(t, 7*every);
        }

        case patt_biweekly: {
            return time_add_day(t, 14*every);
        }

        case patt_monthly: {
            return time_add_month(t, every);
        }

        case patt_endofmonthoffset: {
            for ( ; every>0; --every) {
                t = time_add_month_EOMoffset(t);
            }
            return t;
        }

        case patt_yearly: {
            return time_add_month(t, 12*every);
        }

        default: {
            ADDERROR(__func__, "Unknown repeating pattern ("+std::to_string((int)pattern)+')');
        }
    }

    return t;
}

/**
 * Convert standardized Formalizer Node ID time stamp into local time
 * data object.
 * 
 * Note: This function ignores the `minor_id` value.
 * 
 * @return local time data objet with converted Node ID time stamp.
 */
std::tm ID_TimeStamp::get_local_time() {
    std::tm tm = { 0 };
    if (isnullstamp())
        return tm;
        
    tm.tm_year = year-1900;
    tm.tm_mon = month-1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = -1; // See https://trello.com/c/ANI2Bxei.
    return tm;
}

Node_ID_key::Node_ID_key(const ID_TimeStamp& _idT) { //}: idC( { .id_major = 0, .id_minor = 0 } ) {
    idT=_idT;
    std::string formerror;
    if (!valid_Node_ID(_idT,formerror)) throw(ID_exception(formerror));
}

Node_ID_key::Node_ID_key(std::string _idS) { //}: idC( { .id_major = 0, .id_minor = 0 } ) {
    std::string formerror;
    if (!valid_Node_ID(_idS,formerror,&idT)) throw(ID_exception(formerror));
}

Node_ID_key::Node_ID_key(time_t t, uint8_t minor_id) {
    std::string formerror;
    if ((minor_id<1) || (minor_id>9)) throw(ID_exception(formerror));
    std::string node_idstr = Node_ID_TimeStamp_from_epochtime(t, minor_id, true);
    if (!valid_Node_ID(node_idstr,formerror,&idT)) throw(ID_exception(formerror));
}

Edge_ID_key::Edge_ID_key(std::string _idS) {
    std::string formerror;
    size_t arrowpos = _idS.find('>');
    if (arrowpos==std::string::npos) {
        formerror = "arrow";
        throw(ID_exception(formerror));
    }
    if (!valid_Node_ID(_idS.substr(0,arrowpos),formerror,&dep.idT)) throw(ID_exception(formerror));
    if (!valid_Node_ID(_idS.substr(arrowpos+1),formerror,&sup.idT)) throw(ID_exception(formerror));
}

const std::map<std::string, Edit_flags_type> flagbylabel = {
    {"topics", Edit_flags::topics},
    {"valuation", Edit_flags::valuation},
    {"completion", Edit_flags::completion},
    {"required", Edit_flags::required},
    {"text", Edit_flags::text},
    {"targetdate", Edit_flags::targetdate},
    {"tdproperty", Edit_flags::tdproperty},
    {"repeats", Edit_flags::repeats},
    {"tdpattern", Edit_flags::tdpattern},
    {"tdevery", Edit_flags::tdevery},
    {"tdspan", Edit_flags::tdspan},
    {"topicrels", Edit_flags::topicrels},
    {"superiors", Edit_flags::supdep},
    {"dependencies", Edit_flags::supdep},
    {"in_NNLs", Edit_flags::nnl},
};

bool Edit_flags::set_Edit_flag_by_label(const std::string flaglabel) {
    auto it = flagbylabel.find(flaglabel);
    if (it == flagbylabel.end()) {
        return false;
    }
    editflags |= it->second;
    return true;
}

const std::map<std::string, Edit_flags_type> tdpropbylabel = {
    {"unspecified", tdproperty_binary_pattern::unspecified},
    {"inherit", tdproperty_binary_pattern::inherit},
    {"variable", tdproperty_binary_pattern::variable},
    {"fixed", tdproperty_binary_pattern::fixed},
    {"exact", tdproperty_binary_pattern::exact},
};

bool tdproperty_binary_pattern::set_tdpropbinpat_flag_by_label(const std::string tdproplabel) {
    auto it = tdpropbylabel.find(tdproplabel);
    if (it == tdpropbylabel.end()) {
        return false;
    }
    tdpropbinpattern |= it->second;
    return true;
}

bool tdproperty_binary_pattern::in_pattern(td_property _tdprop) const {
    if ((_tdprop == td_property::unspecified) && (unspecified_is_set())) {
        return true;
    }
    if ((_tdprop == td_property::inherit) && (inherit_is_set())) {
        return true;
    }
    if ((_tdprop == td_property::variable) && (variable_is_set())) {
        return true;
    }
    if ((_tdprop == td_property::fixed) && (fixed_is_set())) {
        return true;
    }
    if ((_tdprop == td_property::exact) && (exact_is_set())) {
        return true;
    }
    return false;
}

/** 
 * Copies a complete set of Node data from a buffer on heap to shared memory Node object.
 * 
 * @param graph Valid Graph object.
 * @param node Valid Node object (this may be in a different shared memory buffer, not part of graph).
 */
void Node_data::copy(Graph & graph, Node & node) {
    node.set_text(utf8_text);
    node.set_completion(completion);
    node.set_required((unsigned int) round(hours*3600.0));
    node.set_valuation(valuation);
    node.set_targetdate(targetdate);
    node.set_tdproperty(tdproperty);
    node.set_tdpattern(tdpattern);
    node.set_tdevery(tdevery);
    node.set_tdspan(tdspan);
    node.set_repeats((tdpattern != patt_nonperiodic) && (tdproperty != variable) && (tdproperty != unspecified));

    for (const auto & tag_str : topics) {
        VERYVERBOSEOUT("\n  adding Topic tag: "+tag_str+'\n');
        Topic * topic_ptr = graph.find_Topic_by_tag(tag_str);
        if (!topic_ptr) {
            standard_exit_error(exit_general_error, "Unknown Topic: "+tag_str, __func__);
        }
        Topic_Tags & topictags = *(const_cast<Topic_Tags *>(&graph.get_topics())); // We need the list of Topics from the memory-resident Graph.
        node.add_topic(topictags, topic_ptr->get_id(), 1.0);
    }
}

/**
 * Set parameter value from string by Edit_flag.
 * 
 * @param param_id An enumerated Edit_flags::editmask parameter identifier.
 * @param valstr A string containing the parameter value.
 * @return True if successfully interpreted and set.
 */
bool Node_data::parse_value(Edit_flags_type param_id, const std::string valstr) {
    switch (param_id) {
        case Edit_flags::text: {
            utf8_text = valstr;
            return true;
        }
        case Edit_flags::completion: {
            completion = std::atof(valstr.c_str());
            if (completion < 0.0) {
                return false;
            }
            return true;
        }
        case Edit_flags::required: { // in hours
            hours = std::atof(valstr.c_str());
            return true;
        }
        case Edit_flags::valuation: {
            valuation = std::atof(valstr.c_str());
            return true;
        }
        case Edit_flags::targetdate: {
            targetdate = time_stamp_time(valstr); // *** should this be noerror=true?
            return true;
        }
        case Edit_flags::repeats: {
            repeats = (valstr == "true");
            return true;
        }
        case Edit_flags::tdproperty: {
            auto it = td_property_map.find(valstr);
            if (it == td_property_map.end()) {
                return false;
            }
            tdproperty = it->second;
            return true;
        }
        case Edit_flags::tdpattern: {
            auto it = td_pattern_map.find(valstr);
            if (it == td_pattern_map.end()) {
                return false;
            }
            tdpattern = it->second;
            return true;
        }
        case Edit_flags::tdevery: {
            tdevery = std::atoi(valstr.c_str());
            if (tdevery < 0) {
                return false;
            }
            return true;
        }
        case Edit_flags::tdspan: {
            tdspan = std::atoi(valstr.c_str());
            if (tdspan < 0) {
                return false;
            }
            return true;
        }
        default: {
            return false;
        }
    }
    return true;
}

/** 
 * Copies a complete set of Edge data from a buffer on heap to shared memory Edge object.
 * 
 * @param edge Valid Edge object (in a shared memory buffer).
 */
void Edge_data::copy(Edge & edge) {
    edge.set_dependency(dependency);
    edge.set_significance(significance);
    edge.set_importance(importance);
    edge.set_urgency(urgency);
    edge.set_priority(priority);
}

// +----- begin: friend functions -----+

// +----- end  : friend functions -----+

} // namespace fz
