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
//#include "Graphinfo.hpp"
//#include "Graphpostgres.hpp"
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

const serialized_func_map_t serialized_data_functions = {
    {"NNLlen", NNL_len}
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
