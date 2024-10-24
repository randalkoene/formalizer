// Copyright 20241003 Randal A. Koene
// License TBD

/**
 * This program provides access to read and write the metrics tables
 * in the Formalizer database.
 * 
 * {{ long_description }}
 * 
 * For more about this, see {{ doc_reference }}.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Formalizer:Server:Metrics:Postgres"

// std
//#include <iostream>
#include <string>
#include <vector>

// core
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "fzpostgres.hpp"
/* (uncomment to communicate with Graph server)
#include "tcpclient.hpp"
*/

// local
#include "version.hpp"
#include "fzmetricspq.hpp"
#include "render.hpp"


using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzmetricspq fzmet;

const std::string pq_metrics_wiztable_layout(
    "id char(12) PRIMARY KEY, " // e.g. "202410032234"
    "data text"                 // JSON data string
);

const std::vector<std::string> default_tables = {
    "wiztable",
    "nutrition",
    "exercise",
    "accounts",
    "milestones",
    "comms",
};

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzmetricspq::fzmetricspq() : formalizer_standard_program(false), config(*this), pa(*this, add_option_args, add_usage_top, true) {
    add_option_args += "RSDCi:w:n:e:a:m:c:F:o:";
    add_usage_top += " <-R|-S|-D|-C> [-i index] [-w JSON] [-n JSON] [-e JSON] [-a JSON] [-m JSON] [-c JSON] [-F format] [-o outfile]";
    //usage_head.push_back("Description at the head of usage information.\n");
    usage_tail.push_back(
        "If JSON data begins with 'file:' then the actual data is\n"
        "obtained from the indicated file instead.\n"
        "Reading data from a file is safer if the data might contain\n"
        "characters that are difficult to include on the command line.\n"
        "\n"
        "If -D is given the index '-i all' the specified tables are deleted.\n"
        "\n"
        "If an index is requested for which there is no data then an empty\n"
        "string is returned in -F raw mode or an empty dict in -F json mode.\n"
        "\n"
        "If an index contains the character '-' then it is treated as an\n"
        "interval in -R mode. Examples of valid intervals are:\n"
        "  202409010000-202410200000\n"
        "  -202410200000\n"
        "  202409010000-\n"
        "\n"
        "Read mode -R also recognizes the '-i all' interval.\n"
    );
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzmetricspq::usage_hook() {
    pa.usage_hook();
    FZOUT("    -i Index within table (e.g. date stamp)\n"
          "    -R Read from table\n"
          "       -w true: read wiztable data\n"
          "       -n true: read nutrition data\n"
          "       -e true: read exercise data\n"
          "       -a true: read accounts data\n"
          "       -m true: read milestones data\n"
          "       -c true: read comms data\n"
          "    -S Store in table\n"
          "       -w JSON data to store in wiztable\n"
          "       -n JSON data to store in nutrition\n"
          "       -e JSON data to store in exercise\n"
          "       -a JSON data to store in accounts\n"
          "       -m JSON data to store in milestones\n"
          "       -c JSON data to store in comms\n"
          "    -D Delete from table\n"
          "       -w true: delete from wiztable\n"
          "       -n true: delete from nutrition data\n"
          "       -e true: delete from exercise data\n"
          "       -a true: delete from accounts data\n"
          "       -a true: delete from milestones data\n"
          "       -a true: delete from comms data\n"
          "    -C Count rows in table\n"
          "       -w true: count rows in wiztable\n"
          "       -n true: count rows in nutrition data\n"
          "       -e true: count rows in exercise data\n"
          "       -a true: count rows in accounts data\n"
          "       -a true: count rows in milestones data\n"
          "       -a true: count rows in comms data\n"
          "    -F Output format: raw, json\n"
          "    -o Output file (default is STDOUT)\n"
    );
}

bool fzmetricspq::get_index_or_interval(const std::string& cargs) {
    if (cargs.empty()) return false;

    auto dashpos = cargs.find('-');
    if (dashpos == std::string::npos) {
        index = cargs;
        interval_of_indices = (index == "all");
        return true;
    }

    interval_of_indices = true;
    index = cargs.substr(0, dashpos);
    last_index = cargs.substr(dashpos+1);
    return true;
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
bool fzmetricspq::options_hook(char c, std::string cargs) {
    if (pa.options_hook(c,cargs))
        return true;

    switch (c) {

    case 'R': {
        flowcontrol = flow_read;
        return true;
    }

    case 'S': {
        flowcontrol = flow_store;
        return true;
    }

    case 'D': {
        flowcontrol = flow_delete;
        return true;
    }

    case 'C': {
        flowcontrol = flow_count;
        return true;
    }

    case 'i': {
        return get_index_or_interval(cargs);
    }

    case 'w': {
        datajson[0] = cargs;
        return true;
    }

    case 'n': {
        datajson[1] = cargs;
        return true;
    }

    case 'e': {
        datajson[2] = cargs;
        return true;
    }

    case 'a': {
        datajson[3] = cargs;
        return true;
    }

    case 'm': {
        datajson[4] = cargs;
        return true;
    }

    case 'c': {
        datajson[5] = cargs;
        return true;
    }

    case 'F': {
        format = cargs;
        return true;
    }

    case 'o': {
        outfile = cargs;
        return true;
    }

    }

    return false;
}

std::vector<std::string> parse_to_vector(const std::string & parvalue, char separator) {
    return split(parvalue, separator);
}

/// Configure configurable parameters.
bool fzmet_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(tables, "tables", parlabel, parse_to_vector(parvalue, ';'));
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
void fzmetricspq::init_top(int argc, char *argv[]) {
    ERRTRACE;

    datajson.resize(default_tables.size());

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class

    if (config.tables.empty()) {
        config.tables = default_tables;
    }
}

/* (uncomment to include access to memory-resident Graph)
Graph & fzgraphsearch::graph() {
    ERRTRACE;
    if (!graphmemman.get_Graph(graph_ptr)) {
        standard_exit_error(exit_resident_graph_missing, "Memory resident Graph not found.", __func__);
    }
    return *graph_ptr;
}
*/

Metrics_wiztable::Metrics_wiztable(const std::string& _table, const std::string & idstr, const std::string & datastr): Metrics_data(_table) {
    idxstr = idstr;
    data = datastr;
}

void Metrics_wiztable::set_id(const fzmetricspq & _fzmet) {
    idxstr = _fzmet.index;
}

std::string Metrics_wiztable::layout() const {
    return pq_metrics_wiztable_layout;
}

std::string Metrics_wiztable::idstr() const {
    return "'"+idxstr+"'";
}

std::string Metrics_wiztable::datastr() const {
    return "$txt$"+data+"$txt$";
}

std::string Metrics_wiztable::all_values_pqstr() const {
    return idstr()+", $txt$"+data+"$txt$";
}

Metrics_data * Metrics_wiztable::clone() const {
    return new Metrics_wiztable(*this);
}

// Translate to Postgres wildcards.
void Metrics_wiztable::set_pq_wildcards() {
    if (nulldata())
        return; // nothing to do, this is recognized as "everything"

    if (multidata()) {
        if (fzmet.index == "(any)")
            idxstr = "%";
    }
}

bool send_rendered_to_output(const std::string& rendered_text) {
    if ((fzmet.outfile.empty()) || (fzmet.outfile == "STDOUT")) { // to STDOUT
        FZOUT(rendered_text);
        return true;
    }
    
    if (!string_to_file(fzmet.outfile, rendered_text)) {
        ADDERROR(__func__,"unable to write to "+fzmet.outfile);
        standard.exit(exit_file_error);
    }
    VERBOSEOUT("Rendered content written to "+fzmet.outfile+".\n\n");
    return true;
}

/**
 * This can read an interval from one or more tables in one call.
 */
int read_interval_from_table() {
    ERRTRACE;

    if (fzmet.index == "all") {
        fzmet.last_index = "999912312359";
        fzmet.index = "000101010000";
    } else {
        if (fzmet.index.empty()) {
            fzmet.index = "000101010000";
        }
        if (fzmet.last_index.empty()) {
            fzmet.last_index = "999912312359";
        }
    }

    std::string rendered_data;
    if (fzmet.format == "json") {
        rendered_data = "{ ";
    }

    for (size_t i = 0; i < fzmet.datajson.size(); ++i) {
        if (!fzmet.datajson.at(i).empty()) {
            Metrics_wiztable_list datalist(fzmet.config.tables.at(i));

            if (!read_Metrics_IDs_and_data_interval_pq(datalist, fzmet.pa, fzmet.index, fzmet.last_index)) {
                return standard_error("Reading "+fzmet.config.tables.at(i)+" data failed.", __func__);
            }

            if (!render_data_list(datalist, rendered_data)) {
                return standard_error("Rendering "+fzmet.config.tables.at(i)+" data failed.", __func__);
            }

            VERYVERBOSEOUT("Read from "+fzmet.config.tables.at(i)+".\n");
        }
    }

    if (fzmet.format == "json") {
        rendered_data.back() = '}';
    }

    send_rendered_to_output(rendered_data);
    standard.completed_ok();
}

/**
 * This can read from one or more tables in one call.
 */
int read_from_table() {
    ERRTRACE;

    if (fzmet.interval_of_indices) return read_interval_from_table();

    std::string rendered_data;
    if (fzmet.format == "json") {
        rendered_data = "{ ";
    }

    for (size_t i = 0; i < fzmet.datajson.size(); ++i) {
        if (!fzmet.datajson.at(i).empty()) {
            Metrics_wiztable data(fzmet.config.tables.at(i), fzmet);

            if (!read_Metrics_data_pq(data, fzmet.pa)) {
                return standard_error("Reading "+fzmet.config.tables.at(i)+" data failed.", __func__);
            }

            if (!data.data.empty()) {
                if (!render_data(data, rendered_data)) {
                    return standard_error("Rendering "+fzmet.config.tables.at(i)+" data failed.", __func__);
                }
            }

            VERYVERBOSEOUT("Read from "+fzmet.config.tables.at(i)+".\n");
        }
    }

    if (fzmet.format == "json") {
        rendered_data.back() = '}';
    }

    send_rendered_to_output(rendered_data);
    standard.completed_ok();
}

std::string get_data_content(const std::string& str) {
    if (str.substr(0,5) == "file:") {
        return string_from_file(str.substr(5));
    } else return str;
}

/**
 * This can write to one or more tables in one call.
 */
int store_in_table() {
    ERRTRACE;

    for (size_t i = 0; i < fzmet.datajson.size(); ++i) {
        if (!fzmet.datajson.at(i).empty()) {
            Metrics_wiztable data(fzmet.config.tables.at(i), fzmet.index, get_data_content(fzmet.datajson.at(i)));

            if (!store_Metrics_data_pq(data, fzmet.pa)) {
                return standard_error("Storing "+fzmet.config.tables.at(i)+" data failed.", __func__);
            }

            VERYVERBOSEOUT("Stored to "+fzmet.config.tables.at(i)+".\n");
        }
    }

    standard.completed_ok();
}

/**
 * This can delete from one or more tables in one call.
 */
int delete_from_table() {
    ERRTRACE;

    for (size_t i = 0; i < fzmet.datajson.size(); ++i) {
        if (!fzmet.datajson.at(i).empty()) {

            if (fzmet.index == "all") {

                if (!delete_Metrics_table_pq(fzmet.config.tables.at(i), fzmet.pa)) {
                    return standard_error("Deleting table "+fzmet.config.tables.at(i)+" failed.", __func__);
                }

                VERYVERBOSEOUT("Deleted table "+fzmet.config.tables.at(i)+".\n");

            } else {

                Metrics_wiztable data(fzmet.config.tables.at(i), fzmet);

                if (!delete_Metrics_data_pq(data, fzmet.pa)) {
                    return standard_error("Deleting from "+fzmet.config.tables.at(i)+" failed.", __func__);
                }

                VERYVERBOSEOUT("Deleted from "+fzmet.config.tables.at(i)+".\n");

            }

        }
    }

    standard.completed_ok();
}

/**
 * This can count rows in one or more tables in one call.
 */
int count_rows_in_table() {
    ERRTRACE;

    std::string rendered_data;
    if (fzmet.format == "json") {
        rendered_data = "{";
    }

    for (size_t i = 0; i < fzmet.datajson.size(); ++i) {
        if (!fzmet.datajson.at(i).empty()) {

            Metrics_wiztable data(fzmet.config.tables.at(i), fzmet);

            if (!count_Metrics_table_pq(data, fzmet.pa)) {
                return standard_error("Counting rows in "+fzmet.config.tables.at(i)+" failed.", __func__);
            }

            if (!render_data(data, rendered_data)) {
                return standard_error("Rendering "+fzmet.config.tables.at(i)+" data failed.", __func__);
            }

            VERYVERBOSEOUT("Number of rows in "+fzmet.config.tables.at(i)+": "+data.data+'\n');

        }
    }

    if (fzmet.format == "json") {
        rendered_data.back() = '}';
    }
    send_rendered_to_output(rendered_data);

    standard.completed_ok();
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzmet.init_top(argc, argv);

    switch (fzmet.flowcontrol) {

    case flow_read: {
        return read_from_table();
    }

    case flow_store: {
        return store_in_table();
    }

    case flow_delete: {
        return delete_from_table();
    }

    case flow_count: {
        return count_rows_in_table();
    }

    default: {
        fzmet.print_usage();
    }

    }

    return standard.completed_ok();
}
