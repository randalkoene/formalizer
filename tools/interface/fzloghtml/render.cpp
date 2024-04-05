// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Template rendering functions.
 * 
 */

//#define USE_COMPILEDPING
#ifdef USE_COMPILEDPING
    #include <iostream>
#endif

// std
#include <locale>

// core
#include "error.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "templater.hpp"
#include "html.hpp"
#include "Graphinfo.hpp"

// local
#include "render.hpp"
#include "fzloghtml.hpp"


/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif


using namespace fz;


enum template_id_enum: unsigned int {
    LogHTML_head_temp,
    LogHTML_tail_temp,
    Log_chunk_RAW_temp,
    Log_chunk_TXT_temp,
    Log_chunk_HTML_temp,
    Log_entry_RAW_temp,
    Log_entry_TXT_temp,
    Log_entry_HTML_temp,
    Log_most_recent_HTML_temp,
    Log_most_recent_TXT_temp,
    Log_most_recent_RAW_temp,
    Log_review_RAW_temp,
    Log_review_TXT_temp,
    Log_review_HTML_temp,
    Log_review_JSON_temp,
    Log_review_topinfo_RAW_temp,
    Log_review_topinfo_TXT_temp,
    Log_review_topinfo_HTML_temp,
    Log_review_topinfo_JSON_temp,
    Log_review_chunk_RAW_temp,
    Log_review_chunk_TXT_temp,
    Log_review_chunk_HTML_temp,
    Log_review_chunk_JSON_temp,
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "LogHTML_head_template.html",
    "LogHTML_tail_template.html",
    "Log_chunk_template.raw",
    "Log_chunk_template.txt",
    "Log_chunk_template.html",
    "Log_entry_template.raw",
    "Log_entry_template.txt",
    "Log_entry_template.html",
    "Log_most_recent_template.html",
    "Log_most_recent_template.txt",
    "Log_most_recent_template.raw",
    "Log_review_template.raw",
    "Log_review_template.txt",
    "Log_review_template.html",
    "Log_review_template.html" // not used
    "Log_review_topinfo_template.raw",
    "Log_review_topinfo_template.txt",
    "Log_review_topinfo_template.html",
    "Log_review_topinfo_template.html", // not used
    "Log_review_chunk_template.raw",
    "Log_review_chunk_template.txt",
    "Log_review_chunk_template.html",
    "Log_review_chunk_template.html" // not used
};

render_environment env;

std::string template_path_from_id(template_id_enum template_id) {
    return template_dir+"/"+template_ids[template_id];
}

std::string template_path_from_map(const std::map<most_recent_format, template_id_enum> & tmap) {
    return template_dir+"/"+template_ids.at(static_cast<unsigned int>(tmap.at(fzlh.recent_format)));
}

const std::map<std::string, std::string> template_code_replacements = {
    {"\\n", "\n"}
};

void prepare_custom_template(std::string & customtemplate) {
    customtemplate = fzlh.custom_template.substr(4);
    for (const auto & [codestr, repstr] : template_code_replacements) {
        size_t codepos = 0;
        while (true) {
            codepos = customtemplate.find(codestr, codepos);
            if (codepos == std::string::npos) break;
            customtemplate.replace(codepos, codestr.size(), repstr);
            codepos += repstr.size();
        }
    }
}

std::string lowercase(const std::string & s, std::locale & loc) {
    std::string s_lower(s);
    for (size_t i = 0; i < s.size(); i++) {
        s_lower[i] = std::tolower(s[i], loc);
    }
    return s_lower;
}

void get_earliest_candidate(int & earliest_found, std::size_t & earliest_found_loc, int candidate_i, std::size_t candidate_loc) {
    if (earliest_found < 0) {
        earliest_found = candidate_i;
        earliest_found_loc = candidate_loc;
        return;
    }

    if (earliest_found_loc > candidate_loc) {
        earliest_found = candidate_i;
        earliest_found_loc = candidate_loc;
    }
}

std::size_t find_next_loc_with_a_search_term(const std::string & s, std::size_t start_loc, int & search_term_found) {
    std::size_t earliest_loc = std::string::npos;
    for (unsigned int i = 0; i < fzlh.search_strings.size(); i++) {
        std::size_t candidate_loc = s.find(fzlh.search_strings[i], start_loc);
        if (candidate_loc != std::string::npos) {
            get_earliest_candidate(search_term_found, earliest_loc, i, candidate_loc);
        }
    }
    return earliest_loc;
}

std::string add_search_highlighting(std::string & s, std::locale & loc) {
    std::string s_out;
    std::string * s_ptr = &s;
    std::string s_lowercase;
    if (fzlh.caseinsensitive) {
        s_lowercase = lowercase(s, loc);
        s_ptr = &s_lowercase;
    }
    std::size_t proc_loc = 0;
    while (true) {
        // 1. Find the next of the search terms in the string, from current processed location.
        int search_term_found = -1;
        std::size_t next_loc = find_next_loc_with_a_search_term(*s_ptr, proc_loc, search_term_found);
        // 2. Copy up to that location.
        if (search_term_found >= 0) {
            s_out += s.substr(proc_loc, next_loc - proc_loc);
            // 3. Highlight the search term found.
            s_out += "<span class=\"searched\">";
            s_out += s.substr(next_loc, fzlh.search_strings[search_term_found].size()); //fzlh.search_strings[search_term_found];
            s_out += "</span>";
            // 4. On to the next location with one of the search terms.
            proc_loc = next_loc + fzlh.search_strings[search_term_found].size();
        } else {
            break;
        }
    }
    if (proc_loc < s.size()) {
        s_out += s.substr(proc_loc, s.size() - proc_loc);
    }
    return s_out;
}

const std::vector<std::string> special_urls = {
    "@FZSERVER@"
};

std::string render_Log_entry(Log_entry & entry, std::locale & loc, const std::string & active_entry_template) {
    std::map<std::string, std::string> log_entry_data = {
        { "minor_id", std::to_string(entry.get_minor_id()) },
        { "entry_id", entry.get_id_str() },
    };

    if (fzlh.recent_format != most_recent_html) {
        log_entry_data["entry_text"] = entry.get_entrytext();
    } else {
        if (fzlh.search_strings.empty()) {
            log_entry_data["entry_text"] = make_embeddable_html(
                entry.get_entrytext(),
                fzlh.config.interpret_text,
                &special_urls,
                &fzlh.replacements
            );
        } else {
            std::string text_to_highlight = make_embeddable_html(
                entry.get_entrytext(),
                fzlh.config.interpret_text,
                &special_urls,
                &fzlh.replacements
            );
            log_entry_data["entry_text"] = add_search_highlighting(text_to_highlight, loc);
        }
    }

    if (entry.same_node_as_chunk()) {
        log_entry_data["node_id"] = ""; 
    } else {
        std::string nodestr(entry.get_nodeidkey().str());
        if (fzlh.recent_format == most_recent_html) {
            log_entry_data["node_id"] = "[<a href=\"/cgi-bin/fzlink.py?id="+nodestr+"\">"+nodestr+"</a>]";
        } else {
            log_entry_data["node_id"] = nodestr;
        }
    }

    std::string rendered_entry;
    if (!env.fill_preloaded_template_from_map(active_entry_template, log_entry_data, rendered_entry)) {
        return "";
    }
    return rendered_entry;
}

bool send_rendered_to_output(std::string & rendered_text) {
    if ((fzlh.config.dest.empty()) || (fzlh.config.dest == "STDOUT")) { // to STDOUT
        FZOUT(rendered_text);
        return true;
    }
    
    if (!string_to_file(fzlh.config.dest,rendered_text)) {
        ADDERROR(__func__,"unable to write to "+fzlh.config.dest);
        standard.exit(exit_file_error);
    }
    VERBOSEOUT("Rendered content written to "+fzlh.config.dest+".\n\n");
    return true;
}

/**
 * At a minimum, show the Node ID. Depending on configuration and available
 * memory-resident Graph, also show a short exerpt of Node description.
 */
std::string include_Node_info(const Node_ID & node_id) {
    std::string nodestr;
    if (fzlh.config.node_excerpt_len > 0) {
        Graph_ptr graphptr = fzlh.get_Graph_ptr(); // only attempts to fetch it the first time
        if (graphptr) {
            Node_ptr nodeptr = graphptr->Node_by_id(node_id.key());
            if (nodeptr) {
                std::string htmltext(nodeptr->get_text().c_str());
                nodestr += ' '+remove_html_tags(htmltext).substr(0,fzlh.config.node_excerpt_len);
            } else {
                nodestr += " (not found)";
            }
        }
    }
    return nodestr;
}

bool entry_has_each(const std::string & entrytextref) {
    for (const auto & search_text : fzlh.search_strings) {
        if (entrytextref.find(search_text)==std::string::npos) {
            return false;
        }
    }
    return true;
}

bool search_text_not_included(Log_chunk * chunkptr, std::locale & loc) {
    if (fzlh.caseinsensitive) {
        if (fzlh.mustcontainall) {
            for (const auto& entryptr : chunkptr->get_entries()) {
                if (entryptr) {
                    std::string entrytext = lowercase(entryptr->get_entrytext(), loc);
                    if (entry_has_each(entrytext)) {
                        return false;
                    }
                }
            }
            return true;
        }

        for (const auto& entryptr : chunkptr->get_entries()) {
            if (entryptr) {
                std::string entrytext = lowercase(entryptr->get_entrytext(), loc);
                for (const auto & search_text : fzlh.search_strings) {
                    if (entrytext.find(search_text)!=std::string::npos) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    if (fzlh.mustcontainall) {
        for (const auto& entryptr : chunkptr->get_entries()) {
            if (entryptr) {
                const auto & entrytextref = entryptr->get_entrytext();
                if (entry_has_each(entrytextref)) {
                    return false;
                }
            }
        }
        return true;
    }

    for (const auto& entryptr : chunkptr->get_entries()) {
        if (entryptr) {
            const auto & entrytextref = entryptr->get_entrytext();
            for (const auto & search_text : fzlh.search_strings) {
                if (entrytextref.find(search_text)!=std::string::npos) {
                    return false;
                }
            }
        }
    }
    return true;
}

std::string make_around_button(const std::string & logchunk_id) {
    return "<button class=\"button button1\" onclick=\"window.open('/cgi-bin/fzloghtml-cgi.py?around="+
            logchunk_id+"#"+logchunk_id+"','_blank');\">context</button>";
}

/**
 * Combine search strings and case insensitive options.
 */
void apply_search_strings_case_insensitive(std::locale & loc) {
    if (!fzlh.search_strings.empty()) {
        if (fzlh.caseinsensitive) {
            for (unsigned int i = 0; i < fzlh.search_strings.size(); i++) {
                fzlh.search_strings[i] = lowercase(fzlh.search_strings[i], loc);
            }
        }
    }
}

void report_interval() {
    if (fzlh.filter.nkey.isnullkey()) {
        VERYVERBOSEOUT("Finding Log chunks from "+TimeStampYmdHM(fzlh.filter.t_from)+" to "+TimeStampYmdHM(fzlh.filter.t_to)+'\n');
    } else {
        VERYVERBOSEOUT("Finding the Log history of Node "+fzlh.filter.nkey.str()+'\n');
    }
}

const std::map<most_recent_format, template_id_enum> log_interval_chunk_tmap = {
    { most_recent_raw, Log_chunk_RAW_temp },
    { most_recent_txt, Log_chunk_TXT_temp },
    { most_recent_html, Log_chunk_HTML_temp },
    { most_recent_json, Log_chunk_HTML_temp }, // not used
};

const std::map<most_recent_format, template_id_enum> log_interval_entry_tmap = {
    { most_recent_raw, Log_entry_RAW_temp },
    { most_recent_txt, Log_entry_TXT_temp },
    { most_recent_html, Log_entry_HTML_temp },
    { most_recent_json, Log_entry_HTML_temp }, // not used
};

/**
 * Convert Log content that was retrieved with filtering to HTML using
 * rending templates and send to designated output destination.
 */
bool render_Log_interval() {
    ERRTRACE;

    std::locale loc;
    apply_search_strings_case_insensitive(loc);

    std::string customtemplate;
    // if (fzlh.custom_template.empty() || (!fzlh.noframe)) {
    //     load_templates(templates); // *** wait? doesn't this segfault further down on the other templates if skipped???
    // }

    std::string active_chunk_template;
    if (!env.load_template(template_path_from_map(log_interval_chunk_tmap), active_chunk_template)) {
        return false;
    }
    std::string active_entry_template;
    if (!env.load_template(template_path_from_map(log_interval_entry_tmap), active_entry_template)) {
        return false;
    }

    if (!fzlh.custom_template.empty()) {
        if (fzlh.custom_template.substr(0,4) == "STR:") {
            prepare_custom_template(customtemplate);
        } else {
            standard_exit_error(exit_command_line_error, "Custom template from file has not yet been implemented.",__func__);
        }
    }

    report_interval();

    std::string rendered_logcontent;
    rendered_logcontent.reserve(128*1024);

    if (!fzlh.noframe) {
        std::string head_template;
        if (!env.load_template(template_path_from_id(LogHTML_head_temp), head_template)) {
            return false;
        }
        rendered_logcontent += head_template;
    }

    if (fzlh.show_total_time_applied) {
        std::string time_applied_str = std::to_string(fzlh.total_minutes_applied/60) + ':' + std::to_string(fzlh.total_minutes_applied % 60);
        rendered_logcontent += "<tr><td><p><b>Actual time applied to Log Chunks: "+time_applied_str+"<b></p></td></tr>\n";
    }

    COMPILEDPING(std::cout,"PING: got templates\n");

    std::string temporalcontextstr, t_open_str;
    for (const auto & [chunk_key, chunkptr] : fzlh.edata.log_ptr->get_Chunks()) {

        //COMPILEDPING(std::cout,"PING: commencing chunk idx#"+std::to_string(chunk_idx)+'\n');

        if (chunkptr) {
            if (!fzlh.search_strings.empty()) {
                if (search_text_not_included(chunkptr.get(), loc)) {
                    continue;
                }
            }
            std::string combined_entries;
            for (const auto& entryptr : chunkptr->get_entries()) {
                if (fzlh.get_log_entry) {
                    if (entryptr->get_minor_id() != fzlh.entry_id) {
                        continue;
                    }
                }
                if (entryptr) {
                    combined_entries += render_Log_entry(*entryptr, loc, active_entry_template);
                }
            }

            template_varvalues varvals;
            Node_ID node_id = chunkptr->get_NodeID();
            t_open_str = chunkptr->get_tbegin_str();
            time_t t_chunkclose = chunkptr->get_close_time();
            time_t t_chunkopen = chunkptr->get_open_time();
            varvals.emplace("chunk_id", t_open_str);
            varvals.emplace("node_id", node_id.str());
            if (fzlh.search_strings.empty()) {
                varvals.emplace("node_info", include_Node_info(node_id));
            } else {
                std::string around_button = make_around_button(t_open_str);
                varvals.emplace("node_info", include_Node_info(node_id)+around_button);
            }
            varvals.emplace("node_link", "/cgi-bin/fzlink.py?id="+node_id.str());
            //varvals.emplace("fzserverpq",graph.get_server_full_address()); *** so far, this is independent of whether the Graph is memory-resident
            std::string t_open_visible_str(t_open_str);
            if (fzlh.config.timezone_offset_hours != 0) {
                t_open_visible_str = TimeStampYmdHM(fzlh.time_zone_adjusted(t_chunkopen));
            }
            if (fzlh.filter.nkey.isnullkey()) {
                varvals.emplace("t_chunkopen", t_open_visible_str);
            } else { // In Node Histories, add links for temporal context.
                temporalcontextstr = "<a href=\"/cgi-bin/fzloghtml-cgi.py?around="+t_open_str+"&daysinterval=3#"+t_open_str+"\" target=\"_blank\">"+t_open_visible_str+"</a>";
                varvals.emplace("t_chunkopen", temporalcontextstr);
            }
            varvals.emplace("temp_context", temporalcontextstr);
            if (t_chunkclose < t_chunkopen) {
                varvals.emplace("t_chunkclose", "OPEN");
                varvals.emplace("t_diff", "");
                varvals.emplace("t_diff_mins", ""); // typically, only either t_diff or t_diff_mins appears in a template
            } else {
                time_t _tchunkclose = fzlh.time_zone_adjusted(t_chunkclose);
                varvals.emplace("t_chunkclose", TimeStampYmdHM(_tchunkclose));
                time_t t_diff = (t_chunkclose - t_chunkopen)/60; // mins
                varvals.emplace("t_diff_mins", std::to_string(t_diff)); // particularly useful for cutom templates
                if (t_diff >= 120) {
                    varvals.emplace("t_diff", to_precision_string(((double) t_diff)/60.0, 2, ' ', 5)+" hrs");
                } else {
                    varvals.emplace("t_diff", std::to_string(t_diff)+" mins");
                }
            }
            varvals.emplace("entries",combined_entries);
            if (fzlh.get_log_entry && fzlh.noframe && (fzlh.recent_format == most_recent_raw)) {
                rendered_logcontent = combined_entries; // very minimal output
            } else {
                if (customtemplate.empty()) {
                    rendered_logcontent += env.render(active_chunk_template, varvals);
                } else {
                    rendered_logcontent += env.render(customtemplate, varvals);
                }
            }
        }
    }

    if (!fzlh.noframe) {
        std::string tail_template;
        if (!env.load_template(template_path_from_id(LogHTML_tail_temp), tail_template)) {
            return false;
        }
        rendered_logcontent += tail_template;
    }

    return send_rendered_to_output(rendered_logcontent);
}

const std::map<most_recent_format, template_id_enum> log_most_recent_tmap = {
    { most_recent_raw, Log_most_recent_RAW_temp },
    { most_recent_txt, Log_most_recent_TXT_temp },
    { most_recent_html, Log_most_recent_HTML_temp },
    { most_recent_json, Log_most_recent_HTML_temp }, // not used
};

bool render_Log_most_recent() {
    ERRTRACE;

    VERYVERBOSEOUT("Finding most recent Log entry.\n");

    template_varvalues varvals;
    varvals.emplace("chunk_id",TimeStampYmdHM(fzlh.edata.newest_chunk_t));
    if (fzlh.edata.c_newest) {
        varvals.emplace("node_id",fzlh.edata.c_newest->get_NodeID().str());
    }
    if (fzlh.edata.is_open) {
        varvals.emplace("chunk_status","OPEN");
        varvals.emplace("t_chunkclose",std::to_string(FZ_TCHUNK_OPEN));
    } else {
        varvals.emplace("chunk_status","CLOSED");
        if (fzlh.edata.c_newest) {
            time_t t_chunkclose = fzlh.edata.c_newest->get_close_time();
            varvals.emplace("t_chunkclose",TimeStampYmdHM(t_chunkclose));
        } else {
            varvals.emplace("t_chunkclose",std::to_string(RTt_invalid_time_stamp)); // should not happen!
        }
    }
    varvals.emplace("entry_minor_id",std::to_string(fzlh.edata.newest_minor_id));
    if (fzlh.edata.e_newest) {
        varvals.emplace("entry_id",fzlh.edata.e_newest->get_id_str());
    } else {
        varvals.emplace("entry_id", LOG_NULLKEY_STR);
    }
    
    std::string render_template;
    if (!env.load_template(template_path_from_map(log_most_recent_tmap), render_template)) {
        return false;
    }
    std::string rendered_mostrecent = env.render(render_template, varvals);

    return send_rendered_to_output(rendered_mostrecent);
}

const std::map<Boolean_Tag_Flags::boolean_flag, char> category_character = {
    { Boolean_Tag_Flags::tzadjust, 't' },
    { Boolean_Tag_Flags::work, 'w' },
    { Boolean_Tag_Flags::self_work, 's' },
    { Boolean_Tag_Flags::system, 'S' },
    { Boolean_Tag_Flags::other, '?' },
    { Boolean_Tag_Flags::error,  'e' },
};

const std::map<char, std::string> category_letter_to_string = {
    { 't', "tzadjust" },
    { 'w', "work" },
    { 's', "selfwork" },
    { 'S', "system" },
    { 'e', "error" },
    { 'n', "nap" },
    { '?', "other" },
};

const std::map<std::string, Boolean_Tag_Flags::boolean_flag> log_override_tags = {
    { "@WORK@", Boolean_Tag_Flags::work },
    { "@SELFWORK@", Boolean_Tag_Flags::self_work },
    { "@SYSTEM@", Boolean_Tag_Flags::system },
    { "@OTHER@", Boolean_Tag_Flags::other },
};

const std::map<most_recent_format, template_id_enum> review_format_to_template_map = {
    { most_recent_raw, Log_review_RAW_temp },
    { most_recent_txt, Log_review_TXT_temp },
    { most_recent_html, Log_review_HTML_temp },
    { most_recent_json, Log_review_JSON_temp },
};

const std::map<most_recent_format, template_id_enum> review_format_to_topinfo_template_map = {
    { most_recent_raw, Log_review_topinfo_RAW_temp },
    { most_recent_txt, Log_review_topinfo_TXT_temp },
    { most_recent_html, Log_review_topinfo_HTML_temp },
    { most_recent_json, Log_review_topinfo_JSON_temp },
};

const std::map<most_recent_format, template_id_enum> review_format_to_chunk_template_map = {
    { most_recent_raw, Log_review_chunk_RAW_temp },
    { most_recent_txt, Log_review_chunk_TXT_temp },
    { most_recent_html, Log_review_chunk_HTML_temp },
    { most_recent_json, Log_review_chunk_JSON_temp },
};

struct review_element {
    time_t t_begin = 0;
    time_t seconds_applied = 0;
    char category = '?';
    std::string nodedesc;
    std::string logcontent;

    review_element(time_t start, time_t seconds, const Boolean_Tag_Flags & boolean_tag, const std::string & _nodedesc, const std::string & _content, bool is_nap = false):
        t_begin(start), seconds_applied(seconds), nodedesc(remove_html(_nodedesc).substr(0, 256)), logcontent(remove_html(_content).substr(0, 256)) {
        if (is_nap) {
            category = 'n';
        } else if (boolean_tag.None()) {
            category = category_character.at(Boolean_Tag_Flags::other);
        } else if (boolean_tag.Work()) {
            category = category_character.at(Boolean_Tag_Flags::work);
        } else if (boolean_tag.SelfWork()) {
            category = category_character.at(Boolean_Tag_Flags::self_work);
        } else if (boolean_tag.System()) {
            category = category_character.at(Boolean_Tag_Flags::system);
        } else if (boolean_tag.Other()) {
            category = category_character.at(Boolean_Tag_Flags::other);
        }
    }

    void json_safe(std::string & s) {
        for (size_t i = 0; i < s.size(); i++) {
            if ((int(s[i]) > 122) || (int(s[i]) < 32)) {
                s[i] = ' ';
            } else if (s[i]=='"') {
                s[i] = '`';
            }
        }
    }

    std::string json_str() {
        std::string jsonstr;
        jsonstr.reserve(3*1024);
        jsonstr += "\t\t{\n";

        jsonstr += "\t\t\"seconds\": "+std::to_string(seconds_applied)+",\n";
        jsonstr += (std::string("\t\t\"category\": \"")+category)+"\",\n";
        json_safe(nodedesc);
        jsonstr += "\t\t\"node\": \""+nodedesc+"\",\n";
        json_safe(logcontent);
        jsonstr += "\t\t\"log\": \""+logcontent+"\"\n";

        jsonstr += "\t\t}";
        return jsonstr;
    }

    const std::map<char, std::string> category_to_key = {
        { 'w', "work-checked" },
        { 's', "selfwork-checked" },
        { 'S', "system-checked" },
        { 'n', "nap-checked" },
        { '?', "other-checked" },
    };

    bool chunk_to_template(std::string & rendered_chunk) {
        std::string chunk_id(TimeStampYmdHM(t_begin));
        std::map<std::string, std::string> chunk_map = {
            { "chunk-id", TimeStampYmdHM(fzlh.time_zone_adjusted(t_begin)) },
            { "seconds-name", chunk_id+"seconds" },
            { "seconds", std::to_string(seconds_applied) },
            { "minutes", std::to_string(seconds_applied/60) },
            { "category-name", chunk_id+"category" },
            //{ "category", category_letter_to_string.at(category) },
            { "work-checked", "" },
            { "selfwork-checked", "" },
            { "system-checked", "" },
            { "nap-checked", "" },
            { "other-checked", "" },
            { "node", nodedesc },
            { "log", logcontent },
        };
        chunk_map[category_to_key.at(category)] = "checked";
        if (!env.fill_template_from_map(
                template_path_from_map(review_format_to_chunk_template_map),
                chunk_map,
                rendered_chunk)) {
            return false;
        }
        return true;
    }
};

struct review_data {
    time_t t_candidate_wakeup = 0;
    time_t t_wakeup = 0;
    time_t t_gosleep = 0;
    std::vector<review_element> elements;

    std::string json_str() {
        std::string jsonstr;
        jsonstr.reserve(128*1024);

        jsonstr += "{\n";

        jsonstr += "\t\"date\": \""+TimeStamp("%Y%m%d\",\n", fzlh.time_zone_adjusted(t_wakeup));
        jsonstr += "\t\"wakeup\": \""+TimeStamp("%H%M\",\n", fzlh.time_zone_adjusted(t_wakeup));
        jsonstr += "\t\"gosleep\": \""+TimeStamp("%H%M\",\n", fzlh.time_zone_adjusted(t_gosleep));

        jsonstr += "\t\"chunks\": [\n";

        // Only elements that occurred between t_wakeup and t_gosleep.
        for (size_t i = 0; i < elements.size(); i++) {

            if ((elements[i].t_begin >= t_wakeup) && (elements[i].t_begin <= t_gosleep)) {

                jsonstr += elements[i].json_str();

                if ((i+1) < elements.size()) {
                    jsonstr += ",\n";
                } else {
                    jsonstr += '\n';
                }
            }

        }

        jsonstr += "\t]\n";

        jsonstr += "}\n";

        return jsonstr;
    }

    bool topinfo_to_template(std::string & rendered_topinfo) {
        std::map<std::string, std::string> topinfo_map = {
            { "date", DateStampYmd(fzlh.time_zone_adjusted(t_wakeup)) },
            { "wakeup", TimeStamp("%H%M", fzlh.time_zone_adjusted(t_wakeup)) },
            { "gosleep", TimeStamp("%H%M", fzlh.time_zone_adjusted(t_gosleep)) },
        };
        if (!env.fill_template_from_map(
                template_path_from_map(review_format_to_topinfo_template_map),
                topinfo_map,
                rendered_topinfo)) {
            return false;
        }
        return true;
    }

    bool table_to_template(std::string & rendered_table) {
        for (size_t i = 0; i < elements.size(); i++) {
            if ((elements[i].t_begin >= t_wakeup) && (elements[i].t_begin <= t_gosleep)) {
                std::string rendered_chunk;
                if (!elements[i].chunk_to_template(rendered_chunk)) {
                    return false;
                }
                rendered_table += rendered_chunk;
            }
        }
        return true;
    }

};

/**
 * Search the indicated Log interval for the start and end of day, then parse Log
 * chunks to identify categories and time applied.
 * This is used by tools such as the System tool `dayreview`.
 */
bool render_Log_review() {
    ERRTRACE;

    Named_Node_List_ptr sleepNNL_ptr = nullptr;
    std::locale loc;
    Node_ID_key nap_id;

    auto prepare_sleep_node_identification = [&] () {
        if (fzlh.config.sleepNNL.empty()) return standard_error("Missing sleepNNL specification in config file.", __func__);
        sleepNNL_ptr = fzlh.get_Graph_ptr()->get_List(fzlh.config.sleepNNL);
        if (!sleepNNL_ptr) return standard_error("Unable to retrieve NNL "+fzlh.config.sleepNNL, __func__);
        for (const auto & node_key : sleepNNL_ptr->list) {
            Node_ptr node_ptr = fzlh.get_Graph_ptr()->Node_by_id(node_key);
            if (!node_ptr) {
                ADDERROR(__func__,fzlh.config.sleepNNL+" NNL node with ID "+node_key.str()+" not found");
            } else {
                if (lowercase(node_ptr->get_text(), loc).find(" nap") != std::string::npos) {
                    nap_id = node_key;
                }
            }
        }
        return true;
    };

    if (!fzlh.get_Graph_ptr()) {
        standard_exit_error(exit_resident_graph_missing, "Resident Graph missing.",__func__);
    }

    apply_search_strings_case_insensitive(loc);

    if (!prepare_sleep_node_identification()) {
        return false;
    }

    report_interval();

    review_data data;
    Map_of_Subtrees map_of_subtrees;
    map_of_subtrees.collect(*fzlh.get_Graph_ptr(), fzlh.config.subtrees_list_name);

    for (const auto & [chunk_key, chunkptr] : fzlh.edata.log_ptr->get_Chunks()) if (chunkptr) {

        time_t t_chunkclose = chunkptr->get_close_time();
        time_t t_chunkopen = chunkptr->get_open_time();
        if (t_chunkclose >= t_chunkopen) { // Only work with completed chunks.

            Node_ID_key node_id = chunkptr->get_NodeID().key();
            Node_ptr node_ptr = fzlh.get_Graph_ptr()->Node_by_id(node_id);
            if (!node_ptr) {
                ADDERROR(__func__,"node with ID "+node_id.str()+" not found");
            } else {

                Boolean_Tag_Flags boolean_tag;
                if (sleepNNL_ptr->contains(node_id)) {
                    if (node_id != nap_id) { // Does this chunk belong to a sleep Node?
                        data.t_wakeup = data.t_candidate_wakeup;
                        data.t_candidate_wakeup = t_chunkclose;
                        data.t_gosleep = t_chunkopen;
                    } else { // Chunk is a nap.
                        data.elements.emplace_back(t_chunkopen, t_chunkclose - t_chunkopen, boolean_tag, node_ptr->get_text(), chunkptr->get_combined_entries_text(), true);
                    }
                } else {
                    // Identify category.
                    //   Look for category override tags in chunk content.
                    // *** Perhaps make this a service function reached through Graphinfo.
                    bool override = false;
                    std::string combined_entries(chunkptr->get_combined_entries_text());
                    for (const auto & [ tag, flag ] : log_override_tags) {
                        if (combined_entries.find(tag) != std::string::npos) {
                            boolean_tag.copy_Boolean_Tag_flags(flag);
                            override = true;
                            break;
                        }
                    }
                    if (!override) {
                        //   Check if Node is in category subtree.
                        if (map_of_subtrees.has_subtrees) {
                            Boolean_Tag_Flags::boolean_flag booleanflag;
                            if (map_of_subtrees.node_in_heads_or_any_subtree(node_id, booleanflag)) {
                                boolean_tag.copy_Boolean_Tag_flags(booleanflag);
                            }
                        }
                    }
                    data.elements.emplace_back(t_chunkopen, t_chunkclose - t_chunkopen, boolean_tag, node_ptr->get_text(), combined_entries);
                }
            }
        }

    }

    VERYVERBOSEOUT("Data collected. Rendering.\n");

    std::string rendered_reviewcontent;
    rendered_reviewcontent.reserve(128*1024);

    if (fzlh.recent_format == most_recent_json) {
        rendered_reviewcontent = data.json_str();
    } else {
        std::string rendered_topinfo;
        if (!data.topinfo_to_template(rendered_topinfo)) {
            return false;
        }

        std::string rendered_table;
        rendered_table.reserve(128*1024);
        if (!data.table_to_template(rendered_table)) {
            return false;
        }

        std::map<std::string, std::string> overall_map = {
            { "top-info", rendered_topinfo },
            { "table-entries", rendered_table },
        };
        if (!env.fill_template_from_map(
                template_path_from_map(review_format_to_template_map),
                overall_map,
                rendered_reviewcontent)) {
            return false;
        }
    }

    return send_rendered_to_output(rendered_reviewcontent);
}
