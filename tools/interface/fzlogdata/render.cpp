// Copyright 20240418 Randal A. Koene
// License TBD

/**
 * Log data gathering, inspection, analysis tool
 * 
 * This tool is used to parse the Log to gather information within
 * it, to inspect its integrity, and to analyze data.
 * 
 * Template rendering functions.
 * 
 * For more about this, see ./README.md.
 */

// core
#include "LogtypesID.hpp"
#include "Logtypes.hpp"
#include "Loginfo.hpp"
#include "stringio.hpp"
#include "templater.hpp"

// local
#include "fzlogdata.hpp"

/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif

render_environment env;

enum template_id_enum: unsigned int {
    Log_data_RAW_temp,
    Log_data_TXT_temp,
    Log_data_HTML_temp,
    Log_data_JSON_temp,
    Log_data_summary_RAW_temp,
    Log_data_summary_TXT_temp,
    Log_data_summary_HTML_temp,
    Log_data_summary_JSON_temp,
    Log_data_chunk_RAW_temp,
    Log_data_chunk_TXT_temp,
    Log_data_chunk_HTML_temp,
    Log_data_chunk_JSON_temp,
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "Log_data_template.raw",
    "Log_data_template.txt",
    "Log_data_template.html",
    "Log_data_template.json",
    "Log_data_summary_template.raw",
    "Log_data_summary_template.txt",
    "Log_data_summary_template.html",
    "Log_data_summary_template.json",
    "Log_data_chunk_template.raw",
    "Log_data_chunk_template.txt",
    "Log_data_chunk_template.html",
    "Log_data_chunk_template.json",
};

const std::map<data_format, template_id_enum> integrity_format_to_template_map = {
    { data_format_raw, Log_data_RAW_temp },
    { data_format_txt, Log_data_TXT_temp },
    { data_format_html, Log_data_HTML_temp },
    { data_format_json, Log_data_JSON_temp },
};

const std::map<data_format, template_id_enum> summary_format_to_template_map = {
    { data_format_raw, Log_data_summary_RAW_temp },
    { data_format_txt, Log_data_summary_TXT_temp },
    { data_format_html, Log_data_summary_HTML_temp },
    { data_format_json, Log_data_summary_JSON_temp },
};

const std::map<data_format, template_id_enum> chunk_format_to_template_map = {
    { data_format_raw, Log_data_chunk_RAW_temp },
    { data_format_txt, Log_data_chunk_TXT_temp },
    { data_format_html, Log_data_chunk_HTML_temp },
    { data_format_json, Log_data_chunk_JSON_temp },
};

std::string template_path_from_id(template_id_enum template_id) {
    return template_dir+"/"+template_ids[template_id];
}

std::string template_path_from_map(const std::map<data_format, template_id_enum> & tmap) {
    return template_dir+"/"+template_ids.at(static_cast<unsigned int>(tmap.at(fzld.format)));
}

bool send_rendered_to_output(std::string & rendered_text) {
    if ((fzld.config.dest.empty()) || (fzld.config.dest == "STDOUT")) { // to STDOUT
        FZOUT(rendered_text);
        return true;
    }
    
    if (!string_to_file(fzld.config.dest, rendered_text)) {
        ADDERROR(__func__,"unable to write to "+fzld.config.dest);
        standard.exit(exit_file_error);
    }
    VERBOSEOUT("Rendered content written to "+fzld.config.dest+".\n\n");
    return true;
}

std::string render_issues_summary(LogIssues & logissues) {
    std::map<std::string, std::string> summary_map = {
        { "total-chunks", std::to_string(fzld.log->num_Chunks()) },
        { "total-entries", std::to_string(fzld.log->num_Entries()) },
        { "oldest-chunk", TimeStampYmdHM(fzld.log->oldest_chunk_t()) },
        { "newest-chunk", TimeStampYmdHM(fzld.log->newest_chunk_t()) },
        { "chunks-allocated", std::to_string(logissues.entries_allocated_to_chunks) },
        { "entries-chars", std::to_string(logissues.total_chars_in_entries_content) },
        { "entries-pages", std::to_string(logissues.total_chars_in_entries_content/3000) },
        { "open-chunks", std::to_string(logissues.unclosed_chunks.size()) },
        { "gaps", std::to_string(logissues.gaps.size()) },
        { "overlaps", std::to_string(logissues.overlaps.size()) },
        { "order-errors", std::to_string(logissues.order_errors.size()) },
        { "entry-enum-errors", std::to_string(logissues.entry_enumeration_errors.size()) },
        { "invalid-nodes", std::to_string(logissues.invalid_nodes.size()) },
        { "excessive-chunks", std::to_string(logissues.very_long_chunks.size()) },
        { "tiny-chunks", std::to_string(logissues.very_tiny_chunks.size()) },
        { "long-entries", std::to_string(logissues.chunks_with_long_entries.size()) },
    };
    std::string rendered_summary;
    rendered_summary.reserve(20*1024);
    if (!env.fill_template_from_map(
            template_path_from_map(summary_format_to_template_map),
            summary_map,
            rendered_summary)) {
        return "Error: Failed to render summary.";
    }
    return rendered_summary;
}

std::string render_chunks_table(const std::vector<time_t> & chunk_ids) {
    std::string rendered_table;
    rendered_table.reserve(100*1024);
    std::map<std::string, std::string> chunk_line_map = {
        { "chunk-id", ""},
    };
    for (time_t chunk_id : chunk_ids) {
        chunk_line_map["chunk-id"] = Log_chunk_ID_key(chunk_id).str();

        std::string rendered_row;
        if (!env.fill_template_from_map(
                template_path_from_map(chunk_format_to_template_map),
                chunk_line_map,
                rendered_row)) {
            rendered_row = "(failed to render)";
        }

        rendered_table += rendered_row;
    }
    return rendered_table;
}

bool render_integrity_issues(LogIssues & logissues) {
    VERYVERBOSEOUT("Data collected. Rendering.\n");

    std::string rendered_issues;
    rendered_issues.reserve(128*1024);

    if (fzld.format == data_format_json) {
        // rendered_reviewcontent = data.json_str();
    } else {
        std::map<std::string, std::string> overall_map = {
            { "top-info", render_issues_summary(logissues) },
            { "open-chunks", render_chunks_table(logissues.unclosed_chunks) },
            { "overlaps", render_chunks_table(logissues.overlaps) },
            { "gaps", render_chunks_table(logissues.gaps) },
            { "enum-errors", render_chunks_table(logissues.entry_enumeration_errors) },
            { "invalid-nodes", render_chunks_table(logissues.invalid_nodes) },
            { "long-chunks", render_chunks_table(logissues.very_long_chunks) },
            { "tiny-chunks", render_chunks_table(logissues.very_tiny_chunks) },
            { "long-entries", render_chunks_table(logissues.chunks_with_long_entries) },
        };
        if (!env.fill_template_from_map(
                template_path_from_map(integrity_format_to_template_map),
                overall_map,
                rendered_issues)) {
            return false;
        }
    }

    return send_rendered_to_output(rendered_issues);
}

bool render_Log_time_data() {
    for (auto& chunk_key : fzld.chunk_keys) {
        const Log_chunk * chunk = fzld.edata.log_ptr->get_chunk(chunk_key);
        if (!chunk) continue;

        FZOUT(chunk->get_tbegin_str()+": "+std::to_string(chunk->duration_minutes())+'\n');
    }
    return true;
}
