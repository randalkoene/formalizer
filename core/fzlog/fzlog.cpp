// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ brief_description }}
 * 
 * {{ long_description }}
 * 
 * For more about this, see {{ doc_reference }}.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Log:MakeEntry"

// std
#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"
#include "stringio.hpp"
#include "Graphtypes.hpp"
#include "Logtypes.hpp"
#include "Logpostgres.hpp"
#include "utf8.hpp"

// local
#include "version.hpp"
#include "fzlog.hpp"



using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzlog fzl;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzlog::fzlog() : formalizer_standard_program(false), config(*this), ga(*this, add_option_args, add_usage_top),
                 reftime(add_option_args, add_usage_top) {
    add_option_args += "en:T:Cc:f:";
    add_usage_top += " [-e] [-n <node-ID>] [-T <text>] [-C] [-c <node-ID>] [-f <content-file>]";
    //usage_head.push_back("Description at the head of usage information.\n");
    usage_tail.push_back(
        "If [-c] is called when a Log chunk is still open then the Log chunk\n"
        "is first closed, then a new Log chunk is opened.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzlog::usage_hook() {
    ga.usage_hook();
    reftime.usage_hook();
    FZOUT("    -e make Log entry\n");
    FZOUT("    -n entry belongs to Node with <node-ID>\n");
    FZOUT("    -T entry <text> from the command line\n");
    FZOUT("    -f entry text from <content-file> (\"STDIN\" for stdin until eof, CTRL+D)\n");
    FZOUT("    -C close Log chunk (if open)\n");
    FZOUT("    -c open new Log chunk for Node <node-ID>\n");
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

    case 'T': {
        edata.utf8_text = utf8_safe(cargs);
        return true;
    }

    case 'C': {
        flowcontrol = flow_close_chunk;
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

    }

    return false;
}


/// Configure configurable parameters.
bool fzl_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(content_file, "content_file", parlabel, parvalue);
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

bool make_entry(entry_data & edata) {
    ERRTRACE;
    if (edata.utf8_text.empty()) {
        if (fzl.config.content_file.empty()) {
            standard_exit_error(exit_general_error, "Missing Log entry content", __func__);
        }
        if (fzl.config.content_file == "STDIN") {
            if (!stream_to_string(std::cin, edata.utf8_text)) {
                standard_exit_error(exit_file_error, "Unable to obtain entry content from standard input", __func__);
            }
        } else {
            if (!file_to_string(fzl.config.content_file, edata.utf8_text)) {
                standard_exit_error(exit_file_error, "Unable to obtain entry content from file at "+fzl.config.content_file, __func__);
            }
        }
        if (edata.utf8_text.empty()) {
            standard_exit_error(exit_general_error, "Missing Log entry content", __func__);
        }
    }

    check_specific_node(edata);
    get_newest_Log_data(fzl.ga ,edata);

    //verbose_test_output(edata);

    // *** maybe add a try-catch here
    Log_TimeStamp log_stamp(edata.newest_chunk_t,true,edata.newest_minor_id+1);
    Log_entry * new_entry;
    if (edata.node_ptr) {
        new_entry = new Log_entry(log_stamp, edata.utf8_text, edata.node_ptr->get_id().key(), edata.c_newest);
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
 * Note that the closing_time must be greater or equal to the opening
 * time of the Log chunk.
 */
bool close_chunk(time_t closing_time) {
    ERRTRACE;
    get_newest_Log_data(fzl.ga ,fzl.edata);
    if (!fzl.edata.is_open)
        return true;

    if (closing_time < fzl.edata.newest_chunk_t) {
        standard_exit_error(exit_general_error, "Chunk closing time cannot be earlier than its opening time ("+fzl.edata.c_newest->get_tbegin_str()+')', __func__);
    }

    fzl.edata.c_newest->set_close_time(closing_time);
    if (!close_Log_chunk_pq(*fzl.edata.c_newest, fzl.ga)) {
        standard_exit_error(exit_database_error, "Unable to close Log chunk "+fzl.edata.c_newest->get_tbegin_str(), __func__);
    }
    VERBOSEOUT("Log chunk "+fzl.edata.c_newest->get_tbegin_str()+" closed.\n");   

    return true;
}

/**
 * Note that the new Log chunk ID has to be unique. Therefore, if the
 * previous Log chunk spans less than a minute then the only options
 * are to either a) refuse to create the new Log chunk, or b) advance
 * the opening time of the new Log chunk by 1 minute.
 */
bool open_chunk() {
    // Ensure a valid Node for the requested new Log chunk.
    Node * newchunk_node_ptr;
    std::tie(newchunk_node_ptr, fzl.edata.graph_ptr) = find_Node_by_idstr(fzl.newchunk_node_id, nullptr);
    if (!newchunk_node_ptr) {
        standard.exit(exit_general_error); // error messages were already sent
    }
    
    // *** NOTE: I'll just use actual time here for the moment, but we'll want to update
    //     this to enable emulated time (as long as that time is greater than recent and
    //     smaller than future).
    time_t t = fzl.reftime.Time();

    // Determine the state of the most recent Log chunk and close it if it was open.
    if (!close_chunk(t)) {
        standard_exit_error(exit_database_error, "Unable to close most recent Log chunk", __func__);
    }

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

    case flow_close_chunk: {
        close_chunk(fzl.reftime.Time());
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
