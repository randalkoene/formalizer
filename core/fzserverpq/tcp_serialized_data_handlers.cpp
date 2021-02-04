// Copyright 2020 Randal A. Koene
// License TBD

/**
 * TCP port serialized data API handler functions for fzserverpq.
 * 
 * For more about this, see https://trello.com/c/S7SZUyeU.
 */

// std
#include <sys/socket.h>
//#include <filesystem>

// core
#include "error.hpp"
#include "standard.hpp"
//#include "general.hpp"
//#include "stringio.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "Graphpostgres.hpp"
//#include "stringio.hpp"
//#include "binaryio.hpp"

// local
#include "tcp_serialized_data_handlers.hpp"
#include "fzserverpq.hpp"

using namespace fz;

enum serialized_response_code: unsigned int {
    serialized_ok = 200,
    serialized_bad_request = 400,
    serialized_missing_data = 404
};

const std::map<serialized_response_code, std::string> serialized_response_code_map = {
    {serialized_ok, "FZ 200 "},
    {serialized_bad_request, "FZ 400 "},
    {serialized_missing_data, "FZ 404 "}
};

struct FZ_request_args {
    std::string request;
    std::string args;
    FZ_request_args(const std::string _request, const std::string _args) : request(_request), args(_args) {}
};
typedef std::vector<FZ_request_args> FZ_request_args_vec;

struct serialized_data {
    std::string data_str;
    serialized_response_code code = serialized_bad_request;
    serialized_data(serialized_response_code _code): code(_code) {}
    serialized_data(serialized_response_code _code, const std::string & _data_str): code(_code) {
        data_str = code_str()+_data_str+'\n';
    }
    size_t len() {
            return data_str.size();
    }
    const std::string & str() {
        return data_str;
    }
    const std::string & code_str() {
        static const std::string nullcodestr = "       ";
        auto it = serialized_response_code_map.find(code);
        if (it != serialized_response_code_map.end()) {
            return it->second;
        } else {
            return nullcodestr;
        }
    }
    ssize_t respond(int socket) {
        if ((code != serialized_ok) || (data_str.empty()) || (code_str()[0]==' ')) {
            if (code == serialized_ok) {
                code = serialized_missing_data;
            }
            VERYVERBOSEOUT("Responding with: "+code_str()+".\n");
            ssize_t sent_len = send(socket, code_str().c_str(), code_str().size(), 0);
            return sent_len;
        } else {
            ssize_t sent_len = send(socket, str().c_str(), len(), 0);
            return sent_len;
        }
    }
};

bool handle_serialized_data_request_response(int socket, const std::string & serstr, std::string msg) {
    serialized_data serdata(serialized_ok, serstr);
    fzs.log("TCP", msg);
    VERYVERBOSEOUT(msg+'\n');
    return (serdata.respond(socket) >= 0);
}

void handle_serialized_data_request_error(int socket, std::string error_msg) {
    serialized_data serdata(serialized_bad_request);
    serdata.respond(socket); // a VERYVERBOSEOUT is in the respond() function
    fzs.log("TCP", error_msg);
}

typedef bool serialized_func_t(int, const std::string&);
typedef std::map<std::string, serialized_func_t*> serialized_func_map_t;

bool NNL_len(int socket, const std::string &argstr) {
    if (argstr.empty()) {
        handle_serialized_data_request_error(socket, "Missing list name.");
        return false;
    }
    VERYVERBOSEOUT("Fetching length of "+argstr+'\n');
    auto list_ptr = fzs.graph_ptr->get_List(argstr);
    size_t listsize = 0;
    if (list_ptr) {
        listsize = list_ptr->size();
    }
    return handle_serialized_data_request_response(socket, std::to_string(listsize), "Serializing size of NNL.");
}

typedef bool match_condition_func_t(Node_Filter &, const std::string&);
typedef std::map<std::string, match_condition_func_t*> match_condition_func_map_t;

const std::map<std::string, td_property> td_property_map = {
    {"unspecified", td_property::unspecified},
    {"inherit", td_property::inherit},
    {"variable", td_property::variable},
    {"fixed", td_property::fixed},
    {"exact", td_property::exact}
};

// Split a '[min-max]' range into a vector of argument strings.
std::vector<std::string> get_range(const std::string & argstr) {
    std::vector<std::string> argrange = split(argstr,'-');
    if (!argrange.empty()) {
        if (!argrange[0].empty()) {
            if (argrange[0].front() == '[') {
                argrange[0].erase(0,1);
            } 
        }
        if (!argrange[1].empty()) {
            if (argrange[1].back() == ']') {
                argrange[1].pop_back();
            } 
        }
    }
    return argrange;
}

/**
 * Set a completion condition.
 * 
 * - If the argument is a single value then both lower and upper bounds
 *   are set to that value.
 * - If the argument is a `[c_min-c_max]` range then the lower bound is set to
 *   `c_min` and the upper bound is set to `c_max`.
 * 
 * @param[out] nodefilter A Node Filter object reference that receives the completion
 *                        filter data. The completion flag of the `filtermask` is set.
 * @param[in] completionstr The argument string specifying a completion value or range.
 * @return True if the argument string could be correctly interpreted.
 */
bool Match_Condition_completion(Node_Filter & nodefilter, const std::string & completionstr) {
    if (completionstr.find('-') != std::string::npos) {
        auto c_range = get_range(completionstr);
        if (c_range.size() < 2) {
            return false;
        }
        nodefilter.lowerbound.completion = std::atof(c_range[0].c_str());
        nodefilter.upperbound.completion = std::atof(c_range[1].c_str());
    } else {
        float c = std::atof(completionstr.c_str());
        nodefilter.lowerbound.completion = c;
        nodefilter.upperbound.completion = c;
    }
    nodefilter.filtermask.set_Edit_completion();
    return true;
}

// Similar to `completion` but sets only the lower bound.
bool Match_Condition_lower_completion(Node_Filter & nodefilter, const std::string & lower_completionstr) {
    nodefilter.lowerbound.completion = std::atof(lower_completionstr.c_str());
    nodefilter.filtermask.set_Edit_completion();
    return true;
}

// Similar to `completion` but sets only the upper bound.
bool Match_Condition_upper_completion(Node_Filter & nodefilter, const std::string & upper_completionstr) {
    nodefilter.upperbound.completion = std::atof(upper_completionstr.c_str());
    nodefilter.filtermask.set_Edit_completion();
    return true;
}

/**
 * Set a hours required condition.
 * 
 * - If the argument is a single value then both lower and upper bounds
 *   are set to that value.
 * - If the argument is a `[h_min-h_max]` range then the lower bound is set to
 *   `h_min` and the upper bound is set to `h_max`.
 * 
 * @param[out] nodefilter A Node Filter object reference that receives the hours
 *                        filter data. The required hours flag of the `filtermask` is set.
 * @param[in] hoursstr The argument string specifying a hours value or range.
 * @return True if the argument string could be correctly interpreted.
 */
bool Match_Condition_hours(Node_Filter & nodefilter, const std::string & hoursstr) {
    if (hoursstr.find('-') != std::string::npos) {
        auto h_range = get_range(hoursstr);
        if (h_range.size() < 2) {
            return false;
        }
        nodefilter.lowerbound.hours = std::atof(h_range[0].c_str());
        nodefilter.upperbound.hours = std::atof(h_range[1].c_str());
    } else {
        float h = std::atof(hoursstr.c_str());
        nodefilter.lowerbound.hours = h;
        nodefilter.upperbound.hours = h;
    }
    nodefilter.filtermask.set_Edit_required();
    return true;
}

// Similar to `hours` but sets only the lower bound.
bool Match_Condition_lower_hours(Node_Filter & nodefilter, const std::string & lower_hoursstr) {
    nodefilter.lowerbound.hours = std::atof(lower_hoursstr.c_str());
    nodefilter.filtermask.set_Edit_required();
    return true;
}

// Similar to `hours` but sets only the upper bound.
bool Match_Condition_upper_hours(Node_Filter & nodefilter, const std::string & upper_hoursstr) {
    nodefilter.upperbound.hours = std::atof(upper_hoursstr.c_str());
    nodefilter.filtermask.set_Edit_required();
    return true;
}

/**
 * Converts an argument string to a td_property enumerated value.
 * 
 * @param tdpropertystr Valid Formalizer target date property type label.
 * @return Value of corresponding `td_property` enum or -1 if unrecognized.
 */
int interpret_tdproperty(const std::string & tdpropertystr) {
    auto it = td_property_map.find(tdpropertystr);
    if (it == td_property_map.end()) {
        return -1;
    }
    return it->second;
}

/**
 * Set a td_property condition.
 * 
 * - If the argument is a single value then both lower and upper bounds
 *   are set to that value.
 * - If the argument is a `[tdprop_A-tdprop_B]` pair then the lower bound is set to
 *   `tdprop_A` and the upper bound is set to `tdprop_B`.
 * 
 * @param[out] nodefilter A Node Filter object reference that receives the td_property
 *                        filter data. The tdproperty flag of the `filtermask` is set.
 * @param[in] tdpropertystr The argument string specifying a tdproperty value or pair.
 * @return True if the argument string could be correctly interpreted.
 */
bool Match_Condition_tdproperty(Node_Filter & nodefilter, const std::string & tdpropertystr) {
    if (tdpropertystr.find('-') != std::string::npos) {
        auto tdprop_pair = get_range(tdpropertystr);
        if (tdprop_pair.size() < 2) {
            return false;
        }
        int tdprop_A = interpret_tdproperty(tdprop_pair[0]);
        if (tdprop_A < 0) {
            return false;
        }
        int tdprop_B = interpret_tdproperty(tdprop_pair[1]);
        if (tdprop_B < 0) {
            return false;
        }
        nodefilter.lowerbound.tdproperty = (td_property) tdprop_A;
        nodefilter.upperbound.tdproperty = (td_property) tdprop_B;
    } else {
        int tdprop = interpret_tdproperty(tdpropertystr);
        if (tdprop < 0) {
            return false;
        }
        nodefilter.lowerbound.tdproperty = (td_property) tdprop;
        nodefilter.upperbound.tdproperty = (td_property) tdprop;
    }
    nodefilter.filtermask.set_Edit_tdproperty();
    return true;
}

// Similar to `tdproperty` but sets only tdproperty A.
bool Match_Condition_tdproperty_A(Node_Filter & nodefilter, const std::string & lower_tdpropertystr) {
    int tdprop = interpret_tdproperty(lower_tdpropertystr);
    if (tdprop < 0) {
        return false;
    }
    nodefilter.lowerbound.tdproperty = (td_property) tdprop;
    nodefilter.filtermask.set_Edit_tdproperty();
    return true;
}

// Similar to `tdproperty` but sets only tdproperty B.
bool Match_Condition_tdproperty_B(Node_Filter & nodefilter, const std::string & upper_tdpropertystr) {
    int tdprop = interpret_tdproperty(upper_tdpropertystr);
    if (tdprop < 0) {
        return false;
    }
    nodefilter.upperbound.tdproperty = (td_property) tdprop;
    nodefilter.filtermask.set_Edit_tdproperty();
    return true;
}


/**
 * Convert Formalizer time stamp string to Unix epoch time, and
 * recognize seveal special codes:
 *   NOW = current actual time
 *   MIN = smallest valid time (Unix epoch start)
 *   MAX = largest valid time
 * 
 * @param targetdatestr Valid Formalizer time stamp or special code.
 * @return Unix epoch time or `RTf_invalid_time_stamp`.
 */
time_t interpret_targetdate(const std::string & targetdatestr) {
    if (targetdatestr == "NOW") {
        return ActualTime();
    }
    if (targetdatestr == "MIN") {
        return RTt_unix_epoch_start;
    }
    if (targetdatestr == "MAX") {
        return RTt_maxtime;
    }
    return time_stamp_time(targetdatestr);
}

/**
 * Set a target date filter condition.
 * 
 * - If the target date is a single time stamp then both lower and upper bounds
 *   are set to that value.
 * - If the argument is a `[t_min-t_max]` range then the lower bound is set to
 *   `t_min` and the upper bound is set to `t_max`.
 * 
 * Special codes are also recognized. See `interpret_targetdate()`.
 * 
 * @param[out] nodefilter A Node Filter object reference that receives the target date
 *                        filter data. The target date flag of the `filtermask` is set.
 * @param[in] targetdatestr The argument string specifying a date-time or range of date-times.
 * @return True if the argument string could be correctly interpreted.
 */
bool Match_Condition_targetdate(Node_Filter & nodefilter, const std::string & targetdatestr) {
    if (targetdatestr.find('-') != std::string::npos) {
        auto td_range = get_range(targetdatestr);
        if (td_range.size() < 2) {
            return false;
        }
        nodefilter.lowerbound.targetdate = interpret_targetdate(td_range[0]);
        nodefilter.upperbound.targetdate = interpret_targetdate(td_range[1]);
        if ((nodefilter.lowerbound.targetdate == RTt_invalid_time_stamp) || (nodefilter.upperbound.targetdate == RTt_invalid_time_stamp)) {
            return false;
        }
    } else {
        time_t t = interpret_targetdate(targetdatestr);
        if (t == RTt_invalid_time_stamp) {
            return false;
        }
        nodefilter.lowerbound.targetdate = t;
        nodefilter.upperbound.targetdate = t;
    }
    nodefilter.filtermask.set_Edit_targetdate();
    return true;
}

// Similar to `targetdate` but sets only the lower bound.
bool Match_Condition_lower_targetdate(Node_Filter & nodefilter, const std::string & lower_targetdatestr) {
    time_t t = interpret_targetdate(lower_targetdatestr);
    if (t == RTt_invalid_time_stamp) {
        return false;
    }
    nodefilter.lowerbound.targetdate = t;
    nodefilter.filtermask.set_Edit_targetdate();
    return true;
}

// Similar to `targetdate` but sets only the upper bound.
bool Match_Condition_upper_targetdate(Node_Filter & nodefilter, const std::string & upper_targetdatestr) {
    time_t t = interpret_targetdate(upper_targetdatestr);
    if (t == RTt_invalid_time_stamp) {
        return false;
    }    
    nodefilter.upperbound.targetdate = t;
    nodefilter.filtermask.set_Edit_targetdate();
    return true;
}

bool Match_Condition_repeats(Node_Filter & nodefilter, const std::string & repeatsstr) {
    bool repeats = false;
    if (repeatsstr == "true") {
        repeats = true;
    } else {
        if (repeatsstr != "false") {
            return false;
        }
    } 
    nodefilter.lowerbound.repeats = repeats;
    nodefilter.upperbound.repeats = repeats;
    nodefilter.filtermask.set_Edit_repeats();
    return true;
}

const match_condition_func_map_t match_condition_functions = {
    {"completion", Match_Condition_completion},
    {"lower_completion", Match_Condition_lower_completion},
    {"upper_completion", Match_Condition_upper_completion},
    {"hours", Match_Condition_hours},
    {"lower_hours", Match_Condition_lower_hours},
    {"upper_hours", Match_Condition_upper_hours},
    {"tdproperty", Match_Condition_tdproperty},
    {"tdproperty_A", Match_Condition_tdproperty_A},
    {"tdproperty_B", Match_Condition_tdproperty_B},
    {"targetdate", Match_Condition_targetdate},
    {"lower_targetdate", Match_Condition_lower_targetdate},
    {"upper_targetdate", Match_Condition_upper_targetdate},
    {"repeats", Match_Condition_repeats}
};

bool build_filter(int socket, Node_Filter & nodefilter, const std::string & argstr) {
    if (argstr.empty()) {
        handle_serialized_data_request_error(socket, "Missing match conditions.");
        return false;
    }
    auto matchconditions_vec = GET_token_values(argstr, ',');

    for (const auto & matchcondition : matchconditions_vec) {
        auto it = match_condition_functions.find(matchcondition.token);
        if (it == match_condition_functions.end()) {
            handle_serialized_data_request_error(socket, "Unrecognized match condition: '" + matchcondition.token + '\'');
            return false;
        }
        if (!it->second(nodefilter, matchcondition.value)) {
            handle_serialized_data_request_error(socket, "Unrecognized match value: '" + matchcondition.value + '\'');
            return false;
        }
    }

    return true;
}

bool Nodes_match(int socket, const std::string & argstr) {
    Node_Filter nodefilter;
    if (!build_filter(socket, nodefilter, argstr)) {
        return false;
    }

    VERYVERBOSEOUT("Matching Nodes to "+argstr+'\n');
    VERYVERBOSEOUT(nodefilter.str());

    targetdate_sorted_Nodes matching_nodes = Nodes_subset(*fzs.graph_ptr, nodefilter);

    std::string matching_nodes_str;
    tdsorted_Nodes_to_csv(matching_nodes, matching_nodes_str);

    return handle_serialized_data_request_response(socket, matching_nodes_str, "Serializing matching Nodes.");
}

bool NNL_add_match(int socket, const std::string & argstr) {
    auto commapos = argstr.find(',');
    if ((commapos == std::string::npos) || (commapos == 0)) {
        handle_serialized_data_request_error(socket, "Missing list name or filter specification: '" + argstr + '\'');
        return false;
    }
    std::string list_name(argstr.substr(0,commapos));

    Node_Filter nodefilter;
    if (!build_filter(socket, nodefilter, argstr.substr(commapos+1))) {
        return false;
    }

    VERYVERBOSEOUT("Adding to list "+list_name+" Nodes that match "+argstr.substr(commapos+1)+'\n');
    targetdate_sorted_Nodes matching_nodes = Nodes_subset(*fzs.graph_ptr, nodefilter);

    if (!matching_nodes.empty()) {
        for (const auto & [t, n_ptr] : matching_nodes) {
            if (!fzs.graph_ptr->add_to_List(list_name, *n_ptr)) {
                std::string errsmg = "Unable to add Node "+n_ptr->get_id_str()+" to Named Node List "+list_name;
                handle_serialized_data_request_error(socket, errsmg);
                return standard_error(errsmg, __func__);
            }
        }
        // synchronize with stored List
        if (fzs.graph_ptr->persistent_Lists()) {
            if (!Update_Named_Node_List_pq(fzs.ga.dbname(), fzs.ga.pq_schemaname(), list_name, *fzs.graph_ptr)) {
                handle_serialized_data_request_error(socket, "Synchronizing Named Node List update to database failed");
                return standard_error("Synchronizing Named Node List update to database failed", __func__);
            }
        }        
    }

    return handle_serialized_data_request_response(socket, std::to_string(matching_nodes.size()), "Serializing number of matched Nodes added to NNL.");
}

const serialized_func_map_t serialized_data_functions = {
    {"NNLlen", NNL_len},
    {"nodes_match", Nodes_match},
    {"NNLadd_match", NNL_add_match}
};

bool handle_request_args(int socket, const FZ_request_args & fra) {
    ERRTRACE;
    auto it = serialized_data_functions.find(fra.request);
    if (it == serialized_data_functions.end()) {
        return standard_error("Unexpected request: '" + fra.request + '\'', __func__);
    }
    return it->second(socket, fra.args);
}

FZ_request_args_vec FZ_request_tokenize(const std::string requeststr) {
    auto requests_vec = split(requeststr,';');
    FZ_request_args_vec fravec;
    for (auto & frastr : requests_vec) {
        if (!frastr.empty()) {
            frastr.pop_back();
            auto bracketpos = frastr.find('(');
            if ((bracketpos != std::string::npos) && (bracketpos != 0)) {
                fravec.emplace_back(frastr.substr(0,bracketpos), frastr.substr(bracketpos+1));
            }
        }
    }
    return fravec;
}

void handle_serialized_data_request(int new_socket, const std::string & request_str) {

    auto requests_vec = FZ_request_tokenize(request_str.substr(3));

    if (requests_vec.empty()) {
        handle_serialized_data_request_error(new_socket, "Missing request.");
        return;
    }

    for (const auto & fra : requests_vec) {
        if (!handle_request_args(new_socket, fra)) {
            standard_error("Unable to carry out FZ request: "+fra.request, __func__);
        }
    }
}
