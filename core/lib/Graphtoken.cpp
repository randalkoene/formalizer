// Copyright 2020 Randal A. Koene
// License TBD

// std
//#include <>

// core
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "Graphbase.hpp"
#include "Graphtoken.hpp"


namespace fz {

time_t interpret_config_targetdate(const std::string & parvalue) {
    if (parvalue.empty())
        return RTt_unspecified;

    if (parvalue == "TODAY") {
        //return today_end_time(); 
        return ymd_stamp_time(DateStampYmd(ActualTime())+"2330"); // this needs less adjustment when used to generate new Nodes
    }

    time_t t = time_stamp_time(parvalue);
    if (t == RTt_invalid_time_stamp)
        standard_exit_error(exit_bad_config_value, "Invalid time stamp for configured targetdate default: "+parvalue, __func__);

    return t;
}

std::vector<std::string> parse_config_topics(const std::string & parvalue) {
    std::vector<std::string> topics;
    if (parvalue.empty())
        return topics;

    topics = split(parvalue, ',');
    for (auto & topic_tag : topics) {
        trim(topic_tag);
    }

    return topics;
}

td_property interpret_config_tdproperty(const std::string & parvalue) {
    if (parvalue.empty())
        return variable;

    for (int i = 0; i < _tdprop_num; ++i) {
        if (parvalue == td_property_str[i]) {
            return (td_property) i;
        }
    }

    standard_exit_error(exit_bad_config_value, "Invalid configured td_property default: "+parvalue, __func__);
}

td_pattern interpret_config_tdpattern(const std::string & parvalue) {
    if (parvalue.empty())
        return patt_nonperiodic;

    std::string pval = "patt_"+parvalue;
    for (int i = 0; i <  _patt_num; ++i) {
        if (pval == td_pattern_str[i]) {
            return (td_pattern) i;
        }
    }

    standard_exit_error(exit_bad_config_value, "Invalid configured td_pattern default: "+parvalue, __func__);
    //return (td_pattern) 0; // never reaches this
}

} // namespace fz
