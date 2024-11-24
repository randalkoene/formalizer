// Copyright 2024 Randal A. Koene
// License TBD

// std

// core
//#include "error.hpp"
#include "stringio.hpp"
#include "btfdays.hpp"


namespace fz {

const std::string btf_days_savefile(BTFDAYS_SAVEFILE);

const std::map<std::string, int> weekday_shortstr = {
    { "SUN", 0 },
    { "MON", 1 },
    { "TUE", 2 },
    { "WED", 3 },
    { "THU", 4 },
    { "FRI", 5 },
    { "SAT", 6 },
};

/**
 * Try to retrieve specified BTF category week days.
 * 
 * If the savefile is not found then a default btf_days
 * string is returned.
 */
std::string get_btf_days() {
    std::string btf_days_retrieved;
    if (!file_to_string(btf_days_savefile, btf_days_retrieved)) {
        btf_days_retrieved = "WORK:MON,TUE,THU,SUN_SELFWORK:WED,SAT";
    }
    return btf_days_retrieved;
}

/**
 * Accepts specifications such as:
 * "SELFWORK:WED,SAT_WORK:MON,TUE,THU,SUN"
 */
void set_btf_tag_days(
	const std::string& configstr, std::map<Boolean_Tag_Flags::boolean_flag,
	std::vector<int>>& tag_to_day_map,
	std::vector<Boolean_Tag_Flags::boolean_flag>& btf_tags) {

    auto btf_configstr_vec = split(configstr, '_');
    for (const auto& btf_configstr : btf_configstr_vec) {
        auto btf_configstr_pair = split(btf_configstr, ':');
        if (btf_configstr_pair.size() == 2) {
            auto btf_it = boolean_flag_map.find(btf_configstr_pair.at(0).c_str());
            if (btf_it != boolean_flag_map.end()) {
                Boolean_Tag_Flags::boolean_flag btf = btf_it->second;
                unsigned int btf_num_days = 0;
                auto weekdaystr_vec = split(btf_configstr_pair.at(1), ',');
                for (const auto& weekdaystr : weekdaystr_vec) {
                    auto weekday_it = weekday_shortstr.find(weekdaystr);
                    if (weekday_it != weekday_shortstr.end()) {
                        tag_to_day_map[btf].emplace_back(weekday_it->second);
                        btf_num_days++;
                    }
                }
                if (btf_num_days > 0) {
                    btf_tags.emplace_back(btf);
                }
            }
        }
    }
}

} // namespace fz
