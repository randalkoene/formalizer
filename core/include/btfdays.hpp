// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ header_description }}
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __BTFDAYS_HPP.
 */

#ifndef __BTFDAYS_HPP
#include "coreversion.hpp"
#define __BTFDAYS_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
#include <string>
#include <map>

// core
#include "Graphtypes.hpp"
//#include "error.hpp"

#define BTFDAYS_SAVEFILE "/var/www/webdata/formalizer/btf_days.txt"

namespace fz {

extern const std::map<std::string, int> weekday_shortstr;

/**
 * Try to retrieve specified BTF category week days.
 * 
 * If the savefile is not found then a default btf_days
 * string is returned.
 */
std::string get_btf_days();

/**
 * Accepts specifications such as:
 * "SELFWORK:WED,SAT_WORK:MON,TUE,THU,SUN"
 */
void set_btf_tag_days(
	const std::string& configstr, std::map<Boolean_Tag_Flags::boolean_flag,
	std::vector<int>>& tag_to_day_map,
	std::vector<Boolean_Tag_Flags::boolean_flag>& btf_tags);

} // namespace fz

#endif // __BTFDAYS_HPP
