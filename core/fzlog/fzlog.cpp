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
fzlog::fzlog() : formalizer_standard_program(false), config(*this), ga(*this, add_option_args, add_usage_top) {
    add_option_args += "en:T:";
    add_usage_top += " [-e] [-n <node-ID>] [-T <text>]";
    //usage_head.push_back("Description at the head of usage information.\n");
    //usage_tail.push_back("Extra usage information.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzlog::usage_hook() {
    ga.usage_hook();
    FZOUT("    -e make Log entry\n");
    FZOUT("    -n entry belongs to Node with <node-ID>\n");
    FZOUT("    -T entry <text> from the command line\n");
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
    if (ga.options_hook(c,cargs))
        return true;

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

    }

    return false;
}


/// Configure configurable parameters.
bool fzl_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    //CONFIG_TEST_AND_SET_PAR(example_par, "examplepar", parlabel, parvalue);
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

void get_newest_Log_data(entry_data & edata) {
    if (!load_last_chunk_and_entry_pq(edata.log, fzl.ga)) {
        std::string errstr("Unable to read the newest Log chunk");
        ADDERROR(__func__, errstr);
        VERBOSEERR(errstr+'\n');
        standard.exit(exit_database_error);
    }
    
    edata.c_newest = edata.log.get_newest_Chunk();
    if (edata.c_newest) {
        edata.newest_chunk_t = edata.log.newest_chunk_t();
        edata.is_open = edata.c_newest->is_open();
    }

    edata.e_newest = edata.log.get_newest_Entry();
    if (edata.e_newest) {
        edata.newest_minor_id = edata.e_newest->get_minor_id();
    }
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

int make_entry(entry_data & edata) {
    ERRTRACE;
    check_specific_node(edata);
    get_newest_Log_data(edata);

    verbose_test_output(edata);

    // *** maybe add a try-catch here
    Log_TimeStamp log_stamp(edata.newest_chunk_t,true,edata.newest_minor_id+1);
    Log_entry * new_entry;
    if (edata.node_ptr) {
        new_entry = new Log_entry(log_stamp, edata.utf8_text, edata.node_ptr->get_id().key(), edata.c_newest);
    } else {
        new_entry = new Log_entry(log_stamp, edata.utf8_text, edata.c_newest);
    }

    if (!append_Log_entry_pq(*new_entry, fzl.ga)) {
        ADDERROR(__func__, "Unable to append Log entry");
        VERBOSEERR("Unable to append Log entry.\n");
        standard.exit(exit_database_error);
    }
    VERBOSEOUT("Log entry "+new_entry->get_id_str()+" appended.\n");

    return standard.completed_ok();
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzl.init_top(argc, argv);

    switch (fzl.flowcontrol) {

    case flow_make_entry: {
        return make_entry(fzl.edata);
    }

    default: {
        fzl.print_usage();
    }

    }

    return standard.completed_ok();
}
