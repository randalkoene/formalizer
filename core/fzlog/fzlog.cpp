// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Formalizer core program to handle Log requests.
 * 
 * Provides the authoritative command line interface with which to
 * open and close Log chunks, and to make Log entries.
 * 
 * For more about this, see the README.md file.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Log:MakeEntry"

// std
#include <iostream>
#include <chrono> // FOR PROFILING AND DEBUGGING (remove this)

// core
#include "debug.hpp"
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "Graphtypes.hpp"
#include "Logtypes.hpp"
#include "Logpostgres.hpp"
#include "utf8.hpp"
#include "tcpclient.hpp"
#include "apiclient.hpp"

// local
#include "version.hpp"
#include "fzlog.hpp"

Set_Debug_LogFile("/var/www/webdata/formalizer/fzlog.debug");

using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzlog fzl;

const std::string onopen_onclose_log = "/var/www/webdata/formalizer/fzlog-onopen-onclose.log";

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzlog::fzlog() : formalizer_standard_program(false), config(*this), ga(*this, add_option_args, add_usage_top),
                 reftime(add_option_args, add_usage_top) {
    add_option_args += "ei:r:D:n:T:CRm:I:2:1:c:f:O";
    add_usage_top += " -e|-i <chunk-ID>|-r <entry-ID>|-D <entry-ID>|-C|-R|-c <node-ID>|-m <chunk-ID>|-I <chunk-ID>|-h [-n <node-ID>] [-T <text>]"
                     " [-2 <close-time>] [-1 <open-time>] [-f <content-file>] [-O]";
    //usage_head.push_back("Description at the head of usage information.\n");
    usage_tail.push_back(
        "If [-c] is called when a Log chunk is still open then the Log chunk\n"
        "is first closed, then a new Log chunk is opened.\n"
        "The [-m] option requires a [-n], [-2] or [-1] specification.\n"
        "Changing the open-time changes the ID of a Log chunk, which may\n"
        "affect any links that use the Chunk ID as a target.\n"
        "Chunk open and close time changes are constrained by preceding and\n"
        "following Log chunk close and open times respectively, and by the\n"
        "current time.\n"
        "To insert a Log chunk (-I) a gap must first be ensured.\n"
        );
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzlog::usage_hook() {
    ga.usage_hook();
    reftime.usage_hook();
    FZOUT("    -e make Log entry (appended to end of Log)\n"
          "    -i insert Log entry at end of <chunk-ID>\n"
          "    -r replace Log entry <entry-ID>\n"
          "    -D delete Log entry <entry-ID>\n"
          "    -m modify Log chunk <chunk-ID>, Node (-n <node_ID>),\n"
          "       close time (-2 <close-time>), or open time (-1 <open-time>)\n"
          "    -I insert Log chunk <chunk-ID>, requires Node (-n <node ID),\n"
          "       (recommended) with optional -2 <close-time>\n"
          "    -n entry belongs to Node with <node-ID>\n"
          "    -1 modify to <open-time> (expressed as YYYYmmddHHMM)\n"
          "    -2 modify to <close-time> (expressed as YYYYmmddHHMM)\n"
          "    -T entry <text> from the command line\n"
          "    -f entry text from <content-file> (\"STDIN\" for stdin until eof, CTRL+D)\n"
          "    -C close Log chunk (if open)\n"
          "    -R reopen Log chunk (if closed)\n"
          "    -c open new Log chunk for Node <node-ID>\n"
          "    -O override safety precautions\n");
}

void get_entry_ID_and_chunk_ID(std::string & cargs) {
    std::string formerror;
    Log_TimeStamp logstamp;
    if (!valid_Log_entry_ID(cargs, formerror, &logstamp)) {
        standard_exit_error(exit_command_line_error, "Invalid Log Entry ID specification: "+formerror, __func__);
    }
    fzl.edata.newest_minor_id = logstamp.minor_id;
    fzl.newchunk_node_id = cargs.substr(0,12);
}

void get_chunk_ID(std::string & cargs) {
    std::string formerror;
    Log_TimeStamp logstamp;
    if (!valid_Log_chunk_ID(cargs, formerror, &logstamp)) {
        standard_exit_error(exit_command_line_error, "Invalid Log Chunk ID specification: "+formerror, __func__);
    }
    fzl.newchunk_node_id = cargs.substr(0,12);
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
bool fzlog::options_hook(char c, std::string cargs) {
    if (ga.options_hook(c, cargs))
        return true;
    if (reftime.options_hook(c, cargs)) {
        if (reftime.Time() == RTt_invalid_time_stamp) {
            standard_exit_error(exit_general_error, "Invalid emulated time specification ("+cargs+')', __func__);
        }
        return true;
    }

    switch (c) {

    case 'n': {
        edata.specific_node_id = cargs;
        return true;
    }

    case 'e': {
        flowcontrol = flow_make_entry;
        return true;
    }

    case 'i': {
        flowcontrol = flow_insert_entry;
        get_chunk_ID(cargs);
        return true;
    }

    case 'r': {
        flowcontrol = flow_replace_entry;
        get_entry_ID_and_chunk_ID(cargs);
        return true;
    }

    case 'D': {
        flowcontrol = flow_delete_entry;
        get_entry_ID_and_chunk_ID(cargs);
        return true;
    }

    case 'T': {
        edata.utf8_text = utf8_safe(cargs);
        return true;
    }

    case 'C': {
        flowcontrol = flow_close_chunk;
        return true;
    }

    case 'm': {
        if ((flowcontrol != flow_replace_chunk_close) && (flowcontrol != flow_replace_chunk_open)) {
            flowcontrol = flow_replace_chunk_node; // combine with '-n <node-id>'
        }
        chunk_id_str = cargs;
        return true;
    }

    case '2': {
        if (flowcontrol != flow_insert_chunk) {
            flowcontrol = flow_replace_chunk_close;
        }
        t_modify = time_stamp_time(cargs); // combine with '-m <chunk-id>'
        return (t_modify != RTt_invalid_time_stamp);
    }

    case '1': {
        flowcontrol = flow_replace_chunk_open;
        t_modify = time_stamp_time(cargs); // combine with '-m <chunk-id>'
        return (t_modify != RTt_invalid_time_stamp);
    }

    case 'R': {
        flowcontrol = flow_reopen_chunk;
        return true;
    }

    case 'I': {
        flowcontrol = flow_insert_chunk;
        newchunk_node_id = cargs;
        return true;
    }

    case 'c': {
        flowcontrol = flow_open_chunk;
        newchunk_node_id = cargs;
        return true;
    }

    case 'f': {
        config.content_file = cargs;
        return true;
    }

    case 'O': {
        override_precautions = true;
        return true;
    }

    }

    return false;
}


std::vector<std::string> parse_onopen_onclose(const std::string & parvalue) {
    ERRTRACE;

    std::vector<std::string> permitted;
    if (parvalue.empty())
        return permitted;

    permitted = split(parvalue, '&');

    return permitted;
}

/// Configure configurable parameters.
bool fzl_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(content_file, "content_file", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(permitted_onopen_onclose, "onopen_onclose", parlabel, parse_onopen_onclose(parvalue)); // E.g. from "indicators.py,fzinfo"
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
void fzlog::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class

}

// Parse an ONOPEN or ONCLOSE tag string to extract program and arguments.
// Format is:
//   <tag>:<program>,<arg>=<value>,<arg>=<value>,...
onopen_onclose::onopen_onclose(const std::string tagstr) {
    // Find start of data.
    auto colonpos = tagstr.find(':');
    if (colonpos == std::string::npos) return;

    // Find program.
    colonpos++;
    auto comma = tagstr.find(',', colonpos);
    if (comma == std::string::npos) { // Only a program name.
        program = tagstr.substr(colonpos);
        if (program.empty()) return;
        valid = true;
        return;
    }
    if (colonpos == comma) return; // Missing program name.
    program = tagstr.substr(colonpos, comma - colonpos);

    // Find arguments.
    comma++;
    auto argpairs = split(tagstr.substr(comma), ',');
    for (auto&& argpair : argpairs) {
        if (argpair.empty()) continue; // no arg-value data
        auto equalpos = argpair.find('=');
        if (equalpos == std::string::npos) {
            args.emplace(argpair, ""); // omitting '=' is permitted
        } else {
            if (equalpos==0) continue; // missing arg
            args.emplace(argpair.substr(0, equalpos), argpair.substr(equalpos+1));
        }
    }

    valid = true;
}

// Note: This assumes that the program is reachable through /usr/lib/cgi-bin.
void onopen_onclose::try_call() {
    // Check if program is in list of permitted programs.
    if (!find_in_vec_of_strings(fzl.config.permitted_onopen_onclose, program)) {
        return;
    }

    // Build and make the call.
    std::string cmdstr = "/usr/lib/cgi-bin/"+program;
    for (const auto& [arg, value] : args) {
        cmdstr += ' ' + arg;
        if (!value.empty()) {
            cmdstr += ' ' + value;
        }
    }
    std::string response = shellcmd2str(cmdstr);
    if (!response.empty()) {
        string_to_file(onopen_onclose_log, cmdstr+'\n'+response+'\n');
    }
}

void check_specific_node(entry_data & edata) {
    if (edata.specific_node_id.empty()) {
        edata.node_ptr = nullptr;
        VERYVERBOSEOUT("Entry will extend history of the same Node as the Log chunk.\n");
        return;
    }

    std::tie(edata.node_ptr, edata.graph_ptr) = find_Node_by_idstr(edata.specific_node_id, nullptr);
    if (!edata.node_ptr) {
        standard.exit(exit_general_error); // error messages were already sent
    }
    VERYVERBOSEOUT("Entry will extend history of Node ["+edata.specific_node_id+"] (found in Graph).\n");
}

void verbose_test_output(const entry_data & edata) {
    FZOUT("TESTING: Newest Log chunk has time stamp "+TimeStampYmdHM(edata.newest_chunk_t)+'\n');
    if (!edata.c_newest) {
        FZOUT("TESTING: There are no chunks\n");
    } else {
        if (edata.is_open) {
            FZOUT("TESTING: The newest Log chunk is still OPEN\n");
        } else {
            FZOUT("TESTING: The newest Log chunk is CLOSED\n");
        }
    }
    if (!edata.e_newest) {
        FZOUT("TESTING: That chunk has no entries\n");
    } else {
        FZOUT("TESTING: The last entry in that chunk has minor id "+std::to_string(edata.newest_minor_id)+'\n');
    }
}

/**
 * This is used to replace the content of an existing Log entry. It is modeled directly
 * after the make_entry() function and can be used as a way to edit Log content.
 * 
 * This expects the Log entry text content to be:
 * a) Already in `edata.utf8_text`.
 * b) In a file found at the path specified by `fzl.config.content_file`.
 * 
 * If the `edata.specific_node_id` string is empty then the Log entry is
 * (re)assigned to the surrounding Log chunk's Node history. If it is not
 * empty then this function confirms that the specified Node exists and
 * (re)assigns the Log entry to the history of that Node.
 * 
 * Where `make_entry()` uses `get_newest_Log_data()` to prepare to append
 * a new Log entry, this function tries to get the Log data for an existing
 * chunk, as specified by the base of the Log Entry ID to be replaced.
 * Similarly, where `make_entry()` calls `append_Log_entry_pq()`, this
 * calls `update_Log_entry_pq()`.
 * 
 * The string variable `fzl.newchunk_node_id` should already contain a
 * reference to an existing Log chunk.
 * The parameter `edata.newest_minor_id` should contain the enumerator of
 * and existing Log entry within that chunk.
 * 
 * @param edata Structure that contains necessary Log entry data.
 */
bool replace_entry(entry_data & edata) {
    ERRTRACE;
    auto [exit_code, errstr] = get_content(edata.utf8_text, fzl.config.content_file, "Log entry");
    if ((exit_code != exit_ok) && (exit_code != exit_missing_data)) // Here, empty is allowed as a way to clear the entry.
        standard_exit_error(exit_code, errstr, __func__);

    check_specific_node(edata);
    get_Log_data(fzl.ga, fzl.newchunk_node_id, edata);

    //verbose_test_output(edata);

    // *** maybe add a try-catch here
    Log_TimeStamp log_stamp(edata.newest_chunk_t, true, edata.newest_minor_id); // do not add 1 here!
    Log_entry * new_entry;
    if (edata.node_ptr) {
        new_entry = new Log_entry(log_stamp, edata.utf8_text, edata.node_ptr->get_id().key(), edata.c_newest);
        //*** This should probably be able to cause an update of the Node history chain and the histories cache!
    } else {
        new_entry = new Log_entry(log_stamp, edata.utf8_text, edata.c_newest);
    }

    if (!update_Log_entry_pq(*new_entry, fzl.ga)) {
        standard_exit_error(exit_database_error, "Unable to update Log entry", __func__);
    }
    VERBOSEOUT("Log entry "+new_entry->get_id_str()+" modified.\n");

    return true;
}

/**
 * Make a Log entry that is added to the end of the Log.
 * The minor ID of the last entry (if there is one) in the last Log chunk
 * is used to determine the minor ID of the new Log entry.
 * 
 * If 'edata.specific_node_id' contains a specified Node ID a check is
 * performed to ensure that the Node exists. The 'edata.node_ptr' will then
 * contain a pointer to the Node object.
 * 
 * The 'edata.log_ptr' receives a pointer to a valid Log object.
 * The 'edata.newest_c' receives a pointer to the newest (last)
 * Log chunk object in the retrieved Log (interval).
 * The 'edata.newest_chunk_t' and 'edata.is_open' variables are set to the
 * open-time (ID) and open or closed state of of that chunk.
 * The 'edata.newest_c' receives a pointer to the newest (last) Log entry
 * in that chunk (if there is an entry), and 'edata.newest_minor_id' receives
 * the minor ID of that entry (or 0 if there is no entry).
 * 
 * Note that if there are no log chunks in the Log, this function will cause
 * an ID exception.
 * 
 * The new Log entry object that is created is not explicitly cleaned up. It
 * is expected that clean-up takes place when fzlog exits.
 * 
 * @param edata Structure that contains necessary Log entry data. It is also
 *              used to hold the entry content that will be fetched. Only the
 *              'specific_node_id' variable of edata needs to be prepared
 *              before calling this function.
 * @return True if successfully created and added to the database.
 */
bool make_entry(entry_data & edata) {
    ERRTRACE;
    auto [exit_code, errstr] = get_content(edata.utf8_text, fzl.config.content_file, "Log entry");
    if (exit_code != exit_ok)
        standard_exit_error(exit_code, errstr, __func__);

    check_specific_node(edata);
    get_newest_Log_data(fzl.ga, edata);

    //verbose_test_output(edata);

    // *** maybe add a try-catch here
    Log_TimeStamp log_stamp(edata.newest_chunk_t,true,edata.newest_minor_id+1);
    Log_entry * new_entry;
    if (edata.node_ptr) {
        new_entry = new Log_entry(log_stamp, edata.utf8_text, edata.node_ptr->get_id().key(), edata.c_newest);
        //*** This should probably be able to cause an update of the Node history chain and the histories cache!
    } else {
        new_entry = new Log_entry(log_stamp, edata.utf8_text, edata.c_newest);
    }

    if (!append_Log_entry_pq(*new_entry, fzl.ga)) {
        standard_exit_error(exit_database_error, "Unable to append Log entry", __func__);
    }
    VERBOSEOUT("Log entry "+new_entry->get_id_str()+" appended.\n");

    return true;
}

/**
 * Insert a Log entry as the new last entry of an existing Log chunk.
 * The Log chunk can be anywhere in the Log.
 * 
 * The string variable `fzl.newchunk_node_id` should already contain a
 * reference to an existing Log chunk.
 *
 * Note that if the specified Log chunk is not in the Log then this function
 * will cause an ID exception.
 * 
 * As explained in the description of 'get_Log_data()', the 'edata' object
 * will be loaded with: log_ptr (to Log object), c_newest (to Log chunk),
 * c_newest_chunk_t (with chunk ID), is_open (state of chunk),
 * e_newest (to last Log entry in chunk), newest_minor_id (with minor ID
 * of e_newest, or 0).
 * 
 * The new Log entry object that is created is not explicitly cleaned up. It
 * is expected that clean-up takes place when fzlog exits.
 * 
 * @param edata Structure that contains necessary Log entry data. It is also
 *              used to hold the entry content that will be fetched. Only the
 *              'specific_node_id' variable of edata needs to be prepared
 *              before calling this function.
 * @return True if successfully created and inserted into the database.
 */
bool insert_entry(entry_data & edata) {
    ERRTRACE;

    // Get content of new Log entry.
    auto [exit_code, errstr] = get_content(edata.utf8_text, fzl.config.content_file, "Log entry");
    if (exit_code != exit_ok)
        standard_exit_error(exit_code, errstr, __func__);

    // Get Log chunk and its entries data.
    check_specific_node(edata);
    get_Log_data(fzl.ga, fzl.newchunk_node_id, edata);

    // Create the new Log entry object.
    // *** maybe add a try-catch here
    Log_TimeStamp log_stamp(edata.newest_chunk_t, true, edata.newest_minor_id+1);
    Log_entry * new_entry;
    if (edata.node_ptr) {
        new_entry = new Log_entry(log_stamp, edata.utf8_text, edata.node_ptr->get_id().key(), edata.c_newest);
        //*** This should probably be able to cause an update of the Node history chain and the histories cache!
    } else {
        new_entry = new Log_entry(log_stamp, edata.utf8_text, edata.c_newest);
    }

    // Update the database.
    if (!insert_Log_entry_pq(*new_entry, fzl.ga)) {
        standard_exit_error(exit_database_error, "Unable to insert Log entry", __func__);
    }
    VERBOSEOUT("Log entry "+new_entry->get_id_str()+" inserted.\n");

    return true;
}

/**
 * Delete an empty Log entry from an existing Log chunk.
 * 
 * Rule: The Log entry must have empty text content to be deletable.
 *       This is a safety precaution to ensure that deletion of that
 *       specific entry is fully intentional.
 *       The 'replace_entry()' function must be used to clear entry
 *       content before this function can be called successfully.
 *       (Node ownership of the empty Log entry is irrelevant.)
 * 
 * The string variable `fzl.newchunk_node_id` should already contain a
 * reference to an existing Log chunk.
 * The parameter `edata.newest_minor_id` should contain the enumerator of
 * and existing Log entry within that chunk.
 *
 * Note that if the specified Log chunk is not in the Log then this function
 * will cause an ID exception.
 * 
 * As explained in the description of 'get_Log_data()', the 'edata' object
 * will be loaded with: log_ptr (to Log object), c_newest (to Log chunk),
 * c_newest_chunk_t (with chunk ID), is_open (state of chunk),
 * e_newest (to specified Log entry in chunk).
 * 
 * @param edata Structure that contains necessary Log entry data.
 * @return True if successfully deleted from the database.
 */
bool delete_entry(entry_data & edata) {
    ERRTRACE;

    // Check the Log chunk, that the entry exists, and ensure that the entry is empty.
    get_Log_data(fzl.ga, fzl.newchunk_node_id, edata);
    if (edata.e_newest->get_entrytext().find_first_not_of(" \t\n\r")!=std::string::npos) {
        std::string s = edata.e_newest->get_entrytext();
        standard_exit_error(exit_general_error, "Entry must be empty to be deletable", __func__);
    }

    if (!delete_Log_entry_pq(*edata.e_newest, fzl.ga)) {
        standard_exit_error(exit_database_error, "Unable to delete Log entry", __func__);
    }
    VERBOSEOUT("Log entry "+edata.e_newest->get_id_str()+" deleted.\n");

    return true;
}

bool port_API_request(const std::string api_url) {
    VERYVERBOSEOUT("Sending Logged time update request to Server API port.\n");
    std::string response_str;
    Graph_ptr graph_ptr = nullptr;
    if (!graphmemman.get_Graph(graph_ptr)) {
        return false;
    }
    //To_Debug_LogFile("port_API_request(): "+graph_ptr->get_server_IPaddr()+':'+std::to_string(graph_ptr->get_server_port())+api_url);
    if (!http_GET(graph_ptr->get_server_IPaddr(), graph_ptr->get_server_port(), api_url, response_str)) {
        return standard_exit_error(exit_communication_error, "API request to Server port failed: "+api_url, __func__);
    }

    VERYVERBOSEOUT("Server response:\n\n"+response_str);

    return true;
}

bool update_Node_completion(const std::string & node_idstr, time_t add_seconds) {
    if (add_seconds <= 0) {
        return true; // nothing to add
    }

    // *** could change the api_url to the even shorter version with "/fz/graph/logtime?"
    std::string api_url("/fz/graph/nodes/logtime?"+node_idstr+'=');
    api_url += std::to_string(add_seconds / 60);

    if (fzl.reftime.is_emulated()) {
        api_url += "&T=" + TimeStampYmdHM(fzl.reftime.Time());
    }
    
    return port_API_request(api_url);
}

/**
 * Note that the closing_time must be greater or equal to the opening
 * time of the Log chunk.
 */
bool close_chunk(time_t closing_time) {
    ERRTRACE;
    get_newest_Log_data(fzl.ga, fzl.edata);

    // Detect times suspiciously far in the future.
    if (!fzl.override_precautions) {
        if ((closing_time - fzl.edata.newest_chunk_t) > RTt_oneday) {
            standard_exit_error(exit_bad_request_data, "Canceled request. Time interval is greater than a day. To force anyway, rerun with the -O override precautions option.", __func__);
        }
    }

    if (!fzl.edata.is_open)
        return true;

    if (closing_time < fzl.edata.newest_chunk_t) {
        standard_exit_error(exit_general_error, "Chunk closing time cannot be earlier than its opening time ("+fzl.edata.c_newest->get_tbegin_str()+')', __func__);
    }

    fzl.edata.c_newest->set_close_time(closing_time);
    if (!close_Log_chunk_pq(*fzl.edata.c_newest, fzl.ga)) {
        standard_exit_error(exit_database_error, "Unable to close Log chunk "+fzl.edata.c_newest->get_tbegin_str(), __func__);
    }

    if (!fzl.edata.c_newest) {
        standard_exit_error(exit_missing_data, "Unable to obtain Node of closed Log chunk, because Log chunk pointer is null pointer.", __func__);
    }
    std::string node_idstr(fzl.edata.c_newest->get_NodeID().str());
    To_Debug_LogFile("close_chunk(): calling update_Node_completion("+node_idstr+", ...)");
    if (!update_Node_completion(node_idstr, closing_time - fzl.edata.newest_chunk_t)) {
        standard_exit_error(exit_communication_error, "Server request to update completion ratio of Node "+node_idstr+" failed.", __func__);
    }
    To_Debug_LogFile("close_chunk(): chunk closed");

    // Possible permitted program with arguments to run ONCLOSE.
    Node * prevchunk_node_ptr;
    std::tie(prevchunk_node_ptr, fzl.edata.graph_ptr) = find_Node_by_idstr(node_idstr, fzl.edata.graph_ptr);
    if (prevchunk_node_ptr) {
        std::string onclose_tagstr = prevchunk_node_ptr->find_tag_in_text("ONCLOSE");
        if (!onclose_tagstr.empty()) {
            onopen_onclose onclose(onclose_tagstr);
            onclose.try_call();
        }
    }

    VERBOSEOUT("Log chunk "+fzl.edata.c_newest->get_tbegin_str()+" closed.\n");   

    return true;
}

bool revert_Node_completion(const std::string & node_idstr, time_t revert_seconds) {
    if (revert_seconds < 60) {
        return true; // nothing to revert
    }

    std::string api_url("/fz/graph/nodes/"+node_idstr+"/completion?add=-");
    api_url += std::to_string(revert_seconds / 60) + 'm';

    /*
    if (fzl.reftime.is_emulated()) {
        api_url += "&T=" + TimeStampYmdHM(fzl.reftime.Time());
    }
    */
    
    return port_API_request(api_url);
}

/**
 * Undo close_chunk().
 * Has no effect if the most recent Log chunk is still open.
 * 
 * Note: If you have to do this manually for some reason, open the database with
 *       `psql`. Then find the last row with:
 *         select * from randalk.logchunks order by id desc limit 1;
 *       Then update its tclose column with appropriately, e.g:
 *         update randalk.logchunks set tclose = 'infinity' where id = '2021-02-03 19:25:00';
 */
bool reopen_chunk() {
    ERRTRACE;
    get_newest_Log_data(fzl.ga, fzl.edata);
    if (fzl.edata.is_open)
        return true;
    
    time_t closing_time = fzl.edata.c_newest->get_close_time();
    fzl.edata.c_newest->set_close_time(FZ_TCHUNK_OPEN);
    if (!close_Log_chunk_pq(*fzl.edata.c_newest, fzl.ga)) {
        standard_exit_error(exit_database_error, "Unable to reopen Log chunk "+fzl.edata.c_newest->get_tbegin_str(), __func__);
    }

    if (!fzl.edata.c_newest) {
        standard_exit_error(exit_missing_data, "Unable to obtain Node of reopened Log chunk, because Log chunk pointer is null pointer.", __func__);
    }
    std::string node_idstr(fzl.edata.c_newest->get_NodeID().str());
    if (!revert_Node_completion(node_idstr, closing_time - fzl.edata.newest_chunk_t)) {
        standard_exit_error(exit_communication_error, "Server request to revert completion ratio of Node "+node_idstr+" failed.", __func__);
    }

    VERBOSEOUT("Log chunk "+fzl.edata.c_newest->get_tbegin_str()+" reopened and Node completion reverted.\n");

    return true;
}

const std::map<bool, std::string> passfailstr = {
    { false, " [fail]\n" },
    { true, " [pass]\n" },
};

/**
 * Change Log chunk close time.
 * 
 * Rules:
 * - Close time can be moved earlier, but no earlier than a chunk's start time.
 * - Close time can be moved forwards, but no later than the start time of the next Log chunk.
 * - Close time can not be moved forwards further than current time. 
 */
bool replace_chunk_close(time_t t_close_new) {
    ERRTRACE;

    // We want to read the Log chunk and the one after it.
    Log_chunk_ID_key key(fzl.chunk_id_str);
    if (!fzl.edata.log_ptr) { // make an empty one if it does not exist yet
        fzl.edata.log_ptr = std::make_unique<Log>();
    }
    Log & log = *(fzl.edata.log_ptr.get());
    Log_filter filter;
    filter.get_n_from(key.get_epoch_time(), 2);
    if (!load_partial_Log_pq(log, fzl.ga, filter)) {
        standard_exit_error(exit_database_error, "Unable to read the specified Log chunk", __func__);
    }

    // Get chunk and check time of chunk after and current time.
    //auto [it_from, it_before] = flz.edata.log_ptr->get_Chunks_ID_n_interval(filter.t_from, 2);
    auto it = log.find_chunk_by_key(key);
    auto chunk_ptr = log.get_chunk(it);
    if (!chunk_ptr) {
        standard_exit_error(exit_bad_request_data, "Unable to find Log chunk with ID "+fzl.chunk_id_str, __func__);
    }
    auto chunk_after_ptr = log.get_chunk(std::next(it)); // if this is nullptr then it's the end of the Log
    VERYVERBOSEOUT("\nModifying Log chunk close-time. Note these rules and data:\n");
    VERYVERBOSEOUT("  Rule 1: New close-time must be >= open-time.\n");
    VERYVERBOSEOUT("  Rule 2: New close-time must be <= next chunk open-time.\n");
    VERYVERBOSEOUT("  Rule 3: New close-time must be <= current time.\n");
    VERYVERBOSEOUT("All times are expressed in the Formalizer database time zone, i.e. no time-zone adjustment.\n\n");

    time_t chunk_t_open = chunk_ptr->get_open_time();
    time_t after_t_open = RTt_maxtime;
    if (chunk_after_ptr) {
        after_t_open = chunk_after_ptr->get_open_time();
    }
    time_t t_now = ActualTime();
    bool atorafter_topen = t_close_new >= chunk_t_open;
    bool notafter_tnextopen = t_close_new <= after_t_open;
    bool notafter_tnow = t_close_new <= t_now;

    VERYVERBOSEOUT("Chunk open-time              : "+TimeStampYmdHM(chunk_t_open)+passfailstr.at(atorafter_topen));
    VERYVERBOSEOUT("Chunk close-time (unmodified): "+TimeStampYmdHM(chunk_ptr->get_close_time())+'\n');
    if (chunk_after_ptr) {
        VERYVERBOSEOUT("Next chunk open-time         : "+TimeStampYmdHM(after_t_open)+passfailstr.at(notafter_tnextopen));
    } else {
        VERYVERBOSEOUT("Next chunk open-time         : end of Log"+passfailstr.at(notafter_tnextopen));
    }
    VERYVERBOSEOUT("Current time                 : "+TimeStampYmdHM(t_now)+passfailstr.at(notafter_tnow));
    VERYVERBOSEOUT("New close-time (specified)   : "+TimeStampYmdHM(t_close_new)+'\n');

    if (!atorafter_topen) {
        standard_exit_error(exit_bad_request_data, "Failed to modify. Specified close-time preceeds chunk open-time.", __func__);
    }
    if (!notafter_tnextopen) {
        standard_exit_error(exit_bad_request_data, "Failed to modify. Specified close-time exceeds next chunk open-time.", __func__);
    }
    if (!notafter_tnow) {
        standard_exit_error(exit_bad_request_data, "Failed to modify. Specified close-time exceeds current time.", __func__);
    }

    // Carry out modification in database, creating a new Log chunk container for the update.
    Log_chunk chunk_container(chunk_ptr->get_tbegin_idT(), chunk_ptr->get_NodeID(), t_close_new);
    if (!close_Log_chunk_pq(chunk_container, fzl.ga)) {
        standard_exit_error(exit_database_error, "Unable to modify Log chunk "+fzl.chunk_id_str+" in database", __func__);
    }

    VERBOSEOUT("Log chunk "+fzl.chunk_id_str+" close-time modified to "+TimeStampYmdHM(t_close_new)+".\n");

    return true;
}

/**
 * Change Log chunk open time and therefore its ID.
 * 
 * Rules:
 * - Start time can be moved forwards, but not further than the close time.
 * - Start time can be moved forwards, but not further than the current time.
 * - Start time can be moved earlier, but no earlier than the close time of the preceding chunk. 
 */
bool replace_chunk_open(time_t t_open_new) {
    ERRTRACE;

    FZOUT("WARNING! This is not yet properly implemented. It does not yet change the IDs of associated Entries! The Log Chunk must be empty of Entries.\n");

    // We want to read the Log chunk and the one before it.
    Log_chunk_ID_key key(fzl.chunk_id_str);
    if (!fzl.edata.log_ptr) { // make an empty one if it does not exist yet
        fzl.edata.log_ptr = std::make_unique<Log>();
    }
    Log & log = *(fzl.edata.log_ptr.get());
    Log_filter filter;
    filter.get_n_earlier_to(key.get_epoch_time(), 2);
    if (!load_partial_Log_pq(log, fzl.ga, filter)) {
        standard_exit_error(exit_database_error, "Unable to read the specified Log chunk", __func__);
    }

    // FZOUT("DEBUG ==> Chunks loaded:");
    // for (const auto & [key, chunk_ptr] : log.get_Chunks()) {
    //     FZOUT("DEBUG ==> "+key.str()+'\n');
    // }

    // Get chunk and check time of chunk before and current time.
    auto it = log.find_chunk_by_key(key);
    auto chunk_ptr = log.get_chunk(it);
    if (!chunk_ptr) {
        standard_exit_error(exit_bad_request_data, "Unable to find Log chunk with ID "+fzl.chunk_id_str, __func__);
    }
    auto chunk_before_ptr = log.get_chunk(std::prev(it)); // if this is nullptr then it's the beginning of the Log
    VERYVERBOSEOUT("\nModifying Log chunk open-time. Note these rules and data:\n");
    VERYVERBOSEOUT("  Rule 1: New open-time must be <= close-time.\n");
    VERYVERBOSEOUT("  Rule 2: New open-time must be >= previous chunk close-time.\n");
    VERYVERBOSEOUT("  Rule 3: New open-time must be <= current time.\n");
    VERYVERBOSEOUT("All times are expressed in the Formalizer database time zone, i.e. no time-zone adjustment.\n\n");

    // --- *** Temporary extra test:
    if (const_cast<Log_chunk*>(chunk_ptr)->get_entries().size() > 0) {
        standard_exit_error(exit_bad_request_data, "Sorry! For now, a Log chunk must be emptied to change its ID.", __func__);
    }
    // ---

    time_t chunk_t_close = chunk_ptr->get_close_time();
    time_t before_t_close = 0;
    if (chunk_before_ptr) {
        before_t_close = chunk_before_ptr->get_close_time();
    }
    time_t t_now = ActualTime();
    bool atorbefore_tclose = chunk_ptr->is_open() || (t_open_new <= chunk_t_close);
    bool notbefore_tprevclose = t_open_new >= before_t_close;
    bool notafter_tnow = t_open_new <= t_now;

    VERYVERBOSEOUT("Chunk open-time (unmodified) : "+TimeStampYmdHM(chunk_ptr->get_open_time())+'\n');
    VERYVERBOSEOUT("Chunk close-time             : "+TimeStampYmdHM(chunk_t_close)+passfailstr.at(atorbefore_tclose));
    if (chunk_before_ptr) {
        VERYVERBOSEOUT("Previous chunk close-time    : "+TimeStampYmdHM(before_t_close)+passfailstr.at(notbefore_tprevclose));
    } else {
        VERYVERBOSEOUT("Previous chunk close-time    : beginning of Log"+passfailstr.at(notbefore_tprevclose));
    }
    VERYVERBOSEOUT("Current time                 : "+TimeStampYmdHM(t_now)+passfailstr.at(notafter_tnow));
    VERYVERBOSEOUT("New open-time (specified)    : "+TimeStampYmdHM(t_open_new)+'\n');

    if (!atorbefore_tclose) {
        standard_exit_error(exit_bad_request_data, "Failed to modify. Specified open-time exceeds chunk close-time.", __func__);
    }
    if (!notbefore_tprevclose) {
        standard_exit_error(exit_bad_request_data, "Failed to modify. Specified open-time preceeds previous chunk close-time.", __func__);
    }
    if (!notafter_tnow) {
        standard_exit_error(exit_bad_request_data, "Failed to modify. Specified open-time exceeds current time.", __func__);
    }

    // Carry out modification in database.
    if (!modify_Log_chunk_id_pq(*chunk_ptr, t_open_new, fzl.ga)) {
        standard_exit_error(exit_database_error, "Unable to modify Log chunk "+fzl.chunk_id_str+" in database", __func__);
    }

    VERBOSEOUT("Log chunk "+fzl.chunk_id_str+" open-time (ID) modified to "+TimeStampYmdHM(t_open_new)+".\n");

    return true;
}

/**
 * Replace ownership of a Log chunk to the specified Node.
 * 
 * Note: After doing this, Node histories should be refreshed.
 */
bool replace_chunk_node(const std::string & chunk_id_str) {
    ERRTRACE;

    std::string replacement_node_id_str(fzl.edata.specific_node_id); // Cache this, as get_Log_data will replace it.

    VERYVERBOSEOUT("\nReplacing Node of Log chunk "+chunk_id_str+" with Node "+replacement_node_id_str+".\n");
    std::cout.flush();

    get_Log_data(fzl.ga, chunk_id_str, fzl.edata);

    // Ensure that the Log chunk was found.
    if (!fzl.edata.c_newest) {
        standard_exit_error(exit_missing_data, "Unable to modify Node that owns Log chunk, because the Log chunk was not found.", __func__);
    }

    VERYVERBOSEOUT("Found Log chunk.\n");
    std::cout.flush();

    // Ensure that the Node ID is valid.
    Node * node_ptr;
    std::tie(node_ptr, fzl.edata.graph_ptr) = find_Node_by_idstr(replacement_node_id_str, nullptr);
    if (!node_ptr) {
        standard.exit(exit_general_error); // error messages were already sent
    }

    VERYVERBOSEOUT("Found Node.\n");
    std::cout.flush();

    // Create a new Log chunk container for the update.
    Log_chunk chunk(fzl.edata.c_newest->get_tbegin_idT(), *node_ptr, fzl.edata.c_newest->get_close_time());

    if (!modify_Log_chunk_nid_pq(chunk, fzl.ga)) {
        standard_exit_error(exit_database_error, "Unable to modify Log chunk "+chunk.get_tbegin_str(), __func__);
    }

    VERBOSEOUT("Log chunk "+chunk.get_tbegin_str()+" modified.\n");
    std::cout.flush();

    return true;
}

/**
 * Insert a Log chunk anywhere in the Log. A time interval should have been
 * created first by changing closing and opening times of adjacent Log
 * chunks as needed.
 * 
 * Rules:
 * 1. The Log chunk ID cannot be a duplicate.
 * 2. The close time of the preceding Log chunk must be <= the new Log chunk ID.
 * 3. If the new Log chunk is closed then its close time must be <= the
 *    open-time of the following Log chunk.
 *    Note that not closing the Log chunk is considered a Log issue and
 *    is reported by 'fzlogdata'.
 * 
 * The string variable `fzl.newchunk_node_id` should contain the proposed
 * Log chunk ID.
 * The variable fzl.t_modify should contain the proposed close-time or -1.
 * 
 * As explained in the description of 'get_Log_data()', the 'edata' object
 * will be loaded with: log_ptr (to Log object), c_newest (to preceding Log chunk),
 * c_newest_chunk_t (with that chunk ID), is_open (state of preceding chunk),
 * e_newest (to last Log entry in chunk), newest_minor_id (with minor ID
 * of e_newest, or 0).
 * 
 * The new Log chunk object that is created is not explicitly cleaned up. It
 * is expected that clean-up takes place when fzlog exits.
 * 
 * @param edata Structure that is used for Log chunk data. Only the
 *              'specific_node_id' variable of edata needs to be prepared
 *              before calling this function.
 * @return True if successfully created and inserted into the database.
 */
bool insert_chunk(entry_data & edata) {
    ERRTRACE;

    // We want to read the Log chunk (if it already exists) and the one before it.
    Log_chunk_ID_key key(fzl.newchunk_node_id);
    if (!fzl.edata.log_ptr) { // make an empty one if it does not exist yet
        fzl.edata.log_ptr = std::make_unique<Log>();
    }
    Log & log = *(fzl.edata.log_ptr.get());
    Log_filter filter;
    filter.get_n_earlier_to(key.get_epoch_time(), 2);
    if (!load_partial_Log_pq(log, fzl.ga, filter)) {
        standard_exit_error(exit_database_error, "Unable to read the preceding Log chunk", __func__);
    }

    // Get chunk and check time of chunk before and current time.
    auto it = log.find_chunk_by_key(key);
    auto chunk_ptr = log.get_chunk(it);
    if (chunk_ptr) { // Rule 1
        standard_exit_error(exit_bad_request_data, "Cannot create Log chunk with ID "+fzl.chunk_id_str+" that already exists", __func__);
    }
    edata.c_newest = log.get_newest_Chunk(); // a pointer to the Log chunk preceding the proposed chunk
    if (!edata.c_newest) {
        standard_exit_error(exit_database_error, "No Log chunk found before the proposed chunk", __func__);
    }
    if (!edata.c_newest->is_open()) {
        if (edata.c_newest->get_close_time() > key.get_epoch_time()) { // Rule 2
            standard_exit_error(exit_bad_request_data, "Preceding Log chunk close time must leave room for the proposed Log chunk ID", __func__);
        }
    }

    // Check Node of the new Log chunk.
    if (edata.specific_node_id.empty()) {
        standard_exit_error(exit_bad_request_data, "A Node must be specified for the proposed Log chunk", __func__);
    }
    check_specific_node(edata);
    if (!edata.node_ptr) {
        standard_exit_error(exit_database_error, "Specified Node for Log chunk not found", __func__);
    }

    // Check close-time.
    if (fzl.t_modify >= 0) { // if closed
        if (fzl.t_modify < key.get_epoch_time()) {
            standard_exit_error(exit_bad_request_data, "Proposed chunk close-time must be >= its ID (open-time)", __func__);
        }
        fzl.edata.log_ptr = std::make_unique<Log>();
        Log & log = *(fzl.edata.log_ptr.get());
        Log_filter filter;
        filter.get_n_from(key.get_epoch_time(), 1);
        if (!load_partial_Log_pq(log, fzl.ga, filter)) {
            standard_exit_error(exit_database_error, "Unable to read the following Log chunk", __func__);
        }
        auto logchunk_ptr = log.get_oldest_Chunk();
        if (!logchunk_ptr) {
             standard_exit_error(exit_database_error, "No Log chunk found after the proposed chunk", __func__);
        }
        if (fzl.t_modify > logchunk_ptr->get_open_time()) { // Rule 3
            standard_exit_error(exit_bad_request_data, "Proposed chunk close-time must be <= the following chunk ID (open-time)", __func__);
        }
    }

    // Create the new Log chunk object.
    // *** maybe add a try-catch here   
    Log_chunk * new_chunk = new Log_chunk(key.idT, *edata.node_ptr, fzl.t_modify);
    // *** Perhaps this should cause an automatic update of Node histories.

    // Update the database.
    if (!insert_Log_chunk_pq(*new_chunk, fzl.ga)) {
        standard_exit_error(exit_database_error, "Unable to insert Log chunk", __func__);
    }
    VERBOSEOUT("Log chunk "+new_chunk->get_tbegin_str()+" inserted.\n");

    return true;
}

/**
 * This pushes the new Node into the fifo 'recent' Named Node List.
 * 
 * The 'recent' List also has the 'unique' feature and 'maxsize=5'.
 * 
 * For more, see https://trello.com/c/I2f2kvmc/73-transition-mvp#comment-5fc2681c317bf8510c97f651.
 * 
 * This could be done either via shared-memory modifications API or through
 * the direct TCP-port API. In this version, we choose the port API,
 * because tehre is very little data transmit.
 * (*** But that could be checked by profiling each approach and comparing.)
 * 
 * @param node The Node for which a Log chunk was just opened.
 * @return True if the Node operation was successful.
 */
bool add_to_recent_Nodes_FIFO(const Node & node) {
    std::string api_url("/fz/graph/namedlists/_recent?id="+node.get_id_str());
    return port_API_request(api_url);
}

/**
 * Note that the new Log chunk ID has to be unique. Therefore, if the
 * previous Log chunk spans less than a minute then the only options
 * are to either a) refuse to create the new Log chunk, or b) advance
 * the opening time of the new Log chunk by 1 minute.
 */
bool open_chunk() {
    // std::vector<long> profiling_us; // PROFILING (remove this)
    // auto t1 = std::chrono::high_resolution_clock::now(); // PROFILING (remove this)
    // Ensure a valid Node for the requested new Log chunk.
    Node * newchunk_node_ptr;
    std::tie(newchunk_node_ptr, fzl.edata.graph_ptr) = find_Node_by_idstr(fzl.newchunk_node_id, nullptr);
    if (!newchunk_node_ptr) {
        standard.exit(exit_general_error); // error messages were already sent
    }
    // auto t2 = std::chrono::high_resolution_clock::now(); // PROFILING (remove this)
    // profiling_us.emplace_back(std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()); // PROFILING (remove this)

    time_t t = fzl.reftime.Time(); // Emulated times are included via -t options_hook() of ReferenceTime.

    // Determine the state of the most recent Log chunk and close it if it was open.
    if (!close_chunk(t)) {
        standard_exit_error(exit_database_error, "Unable to close most recent Log chunk", __func__);
    }
    // auto t3 = std::chrono::high_resolution_clock::now(); // PROFILING (remove this)
    // profiling_us.emplace_back(std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count()); // PROFILING (remove this)

    // Create a new Log chunk and appended it to the Log.
    // *** maybe add a try-catch here
    if (t <= fzl.edata.newest_chunk_t) {
        standard_exit_error(exit_general_error, "Unable to create Log chunk with ID earlier or equal to most recent", __func__);
    }
    Log_TimeStamp log_stamp(t, true);
    Log_chunk new_chunk(log_stamp, *newchunk_node_ptr, FZ_TCHUNK_OPEN);

    if (!append_Log_chunk_pq(new_chunk, fzl.ga)) {
        standard_exit_error(exit_database_error, "Unable to append Log chunk", __func__);
    }
    VERBOSEOUT("Log chunk "+new_chunk.get_tbegin_str()+" appended.\n");
    // auto t4 = std::chrono::high_resolution_clock::now(); // PROFILING (remove this)
    // profiling_us.emplace_back(std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count()); // PROFILING (remove this)

    // Possible permitted program with arguments to run ONOPEN.
    std::string onopen_tagstr = newchunk_node_ptr->find_tag_in_text("ONOPEN");
    if (!onopen_tagstr.empty()) {
        onopen_onclose onopen(onopen_tagstr);
        onopen.try_call();
    }

    if (!add_to_recent_Nodes_FIFO(*newchunk_node_ptr)) {
        standard_warning("Unable to push new Log chunk Node to fifo 'recent' Named Node List.", __func__);
    }
    // auto t5 = std::chrono::high_resolution_clock::now(); // PROFILING (remove this)
    // profiling_us.emplace_back(std::chrono::duration_cast<std::chrono::microseconds>(t5 - t4).count()); // PROFILING (remove this)
    // std::string profiling_str; // PROFILING (remove this)
    // for (auto& us : profiling_us) { // PROFILING (remove this)
    //     profiling_str += std::to_string(us)+' '; // PROFILING (remove this)
    // } // PROFILING (remove this)
    // string_to_file("/var/www/webdata/formalizer/fzlog.profiling", profiling_str); // PROFILING (remove this)

    //*** This should probably be able to cause an update of the Node history chain and the histories cache!

    return true;
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzl.init_top(argc, argv);

    switch (fzl.flowcontrol) {

    case flow_make_entry: {
        make_entry(fzl.edata);
        break;
    }

    case flow_insert_entry: {
        insert_entry(fzl.edata);
        break;
    }

    case flow_replace_entry: {
        replace_entry(fzl.edata);
        break;
    }

    case flow_delete_entry: {
        delete_entry(fzl.edata);
        break;
    }

    case flow_close_chunk: {
        close_chunk(fzl.reftime.Time());
        break;
    }

    case flow_reopen_chunk: {
        reopen_chunk();
        break;
    }

    case flow_replace_chunk_node: {
        Node_ID node_id(fzl.edata.specific_node_id);
        replace_chunk_node(fzl.chunk_id_str);
        break;
    }

    case flow_replace_chunk_close: {
        replace_chunk_close(fzl.t_modify);
        break;
    }

    case flow_replace_chunk_open: {
        replace_chunk_open(fzl.t_modify);
        break;
    }

    case flow_insert_chunk: {
        insert_chunk(fzl.edata);
        break;
    }

    case flow_open_chunk: {
        open_chunk();
        break;
    }

    default: {
        fzl.print_usage();
    }

    }

    return standard.completed_ok();
}
