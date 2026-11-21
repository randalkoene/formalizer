// Copyright 2026 Randal A. Koene
// License TBD

/**
 * Log calendar rendering functions (nodeboard -k).
 *
 * Shows the Log as a calendar of day columns, with a card for each Log chunk at
 * the time where it is. Clicking a card opens a pop-up that shows the whole Log
 * chunk as it is shown in the Log page (fetched from fzloghtml-cgi.py?chunk=<id>).
 *
 * The CSV data is made by fzloghtml with a custom template, e.g. by
 * nodeboard-cgi.py?logcal=<YYYYmmdd>&days=<n>. See Node 20260223154446.1.
 */

#include <algorithm>

#include "error.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "html.hpp"
#include "TimeStamp.hpp"
#include "Graphtypes.hpp"
#include "templater.hpp"
#include "nbrender.hpp"

using namespace fz;

#ifdef DEFAULT_TEMPLATE_DIR
    const std::string logcal_template_dir(DEFAULT_TEMPLATE_DIR "/templates/html/");
#else
    const std::string logcal_template_dir("./templates/html/");
#endif

const std::string logcal_sleepNNL("group_sleep");
const unsigned int logcal_max_days = 31;
const unsigned int logcal_excerpt_length = 160;

bool all_digits(const std::string & s) {
    if (s.empty()) return false;
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}

/// Start of the day after the day that starts at t_day (safe across DST changes).
time_t next_day_start(time_t t_day) {
    return day_start_time(t_day + 30*3600);
}

/// Start of the day before the day that starts at t_day (safe across DST changes).
time_t previous_day_start(time_t t_day) {
    return day_start_time(t_day - 12*3600);
}

time_t add_days(time_t t_day, int days) {
    for (; days > 0; --days) t_day = next_day_start(t_day);
    for (; days < 0; ++days) t_day = previous_day_start(t_day);
    return t_day;
}

/// Text is placed in HTML. remove_html_tags() already removed tags, keep entities as they are.
std::string logcal_html_safe(const std::string & s) {
    std::string res;
    res.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '<': res += "&lt;"; break;
            case '>': res += "&gt;"; break;
            case '"': res += "&quot;"; break;
            default: res += c;
        }
    }
    return res;
}

/// Cut a UTF8 string to at most max_bytes without splitting a multi-byte character.
std::string logcal_utf8_cut(const std::string & s, size_t max_bytes) {
    if (s.size() <= max_bytes) return s;
    size_t cut = max_bytes;
    while ((cut > 0) && ((static_cast<unsigned char>(s[cut]) & 0xC0) == 0x80)) cut--;
    return s.substr(0, cut);
}

bool Logcal_chunk::parse(const std::string & csv_line) {
    // The first five fields contain no commas, the excerpt (last) may.
    std::vector<std::string> fields;
    size_t pos = 0;
    for (int i = 0; i < 5; ++i) {
        size_t comma = csv_line.find(',', pos);
        if (comma == std::string::npos) return false;
        fields.emplace_back(csv_line.substr(pos, comma - pos));
        pos = comma + 1;
    }
    excerpt = csv_line.substr(pos);
    while ((!excerpt.empty()) && ((excerpt.back() == '\r') || (excerpt.back() == ' '))) excerpt.pop_back();

    chunk_id = fields[0];
    if ((chunk_id.size() != 12) || (!all_digits(chunk_id))) return false;
    if ((fields[1].size() != 12) || (!all_digits(fields[1]))) return false;
    t_open = time_stamp_time(fields[1], true);
    if (t_open == RTt_invalid_time_stamp) return false;
    if (!all_digits(fields[2])) return false;
    minutes = std::atoi(fields[2].c_str());
    is_open = (fields[3] == "1");
    node_id = fields[4];
    return true;
}

/**
 * Parse '-a <YYYYmmdd>[,<days>]'. Instead of a date, 'today' may be used.
 */
bool nodeboard::parse_logcal_range(const std::string & cargs) {
    auto comma = cargs.find(',');
    std::string datestr = cargs.substr(0, comma);
    if (datestr == "today") {
        logcal_first_day = 0;
    } else {
        if ((datestr.size() != 8) || (!all_digits(datestr))) {
            return standard_error("Invalid Log calendar date: "+datestr, __func__);
        }
        logcal_first_day = ymd_stamp_time(datestr, true);
        if (logcal_first_day == RTt_invalid_time_stamp) {
            return standard_error("Invalid Log calendar date: "+datestr, __func__);
        }
    }
    if (comma != std::string::npos) {
        std::string daysstr = cargs.substr(comma + 1);
        if (!all_digits(daysstr)) {
            return standard_error("Invalid number of days: "+daysstr, __func__);
        }
        logcal_days = std::atoi(daysstr.c_str());
    } else {
        logcal_days = 1;
    }
    if (logcal_days < 1) logcal_days = 1;
    if (logcal_days > logcal_max_days) logcal_days = logcal_max_days;
    return true;
}

bool nodeboard::parse_logcal_csv(const std::string & csv_data) {
    std::vector<std::string> csv_lines = split(csv_data, '\n');
    for (const auto & line : csv_lines) {
        Logcal_chunk chunk;
        if (chunk.parse(line)) { // Lines that are not chunk data are skipped.
            logcal_chunks.emplace_back(chunk);
        }
    }
    std::sort(logcal_chunks.begin(), logcal_chunks.end(), [](const Logcal_chunk & a, const Logcal_chunk & b) { return a.t_open < b.t_open; });
    return true;
}

std::string nodeboard::logcal_cgi_call(time_t first_day, unsigned int days, float multiplier) const {
    return "/cgi-bin/nodeboard-cgi.py?logcal="+DateStampYmd(first_day)+"&days="+std::to_string(days)+"&M="+to_precision_string(multiplier, 1);
}

const std::string logcal_open_color("#ffa07a");     // light salmon: the open chunk
const std::string logcal_sleep_color("#339afc");    // as sleep in the calendar Schedule
const std::vector<std::string> logcal_colors = {    // alternate, so that neighbours can be told apart
    "#e0f0ff",
    "#fafad2",
};

bool nodeboard::get_logcal_day_column(time_t t_day, const std::string & card_template, const std::string & column_template, std::string & rendered_columns) {
    time_t t_day_end = next_day_start(t_day);

    std::string header = "<h4><a href=\""+logcal_cgi_call(t_day, 1, vertical_multiplier)+"\">"+WeekDay(t_day)+' '+DateStampYmd(t_day)+"</a></h4>";

    std::string rendered_cards;

    // Hour lines
    for (int hr = 0; hr < 24; ++hr) {
        std::string vpos = to_precision_string(get_minute_vertical_position(hr*60), 2);
        std::string hrstr = (hr < 10) ? '0'+std::to_string(hr) : std::to_string(hr);
        rendered_cards += "<div class=\"logcal-hour\" style=\"top:"+vpos+"vh;\">"+hrstr+":00</div>\n";
    }

    unsigned int color_idx = 0;
    for (const auto & chunk : logcal_chunks) {
        // Show chunks that overlap the day, including 0 minute chunks that open in it.
        if (chunk.t_open >= t_day_end) continue;
        if ((chunk.t_open < t_day) && (chunk.t_close() <= t_day)) continue;

        time_t seg_start = std::max(chunk.t_open, t_day);
        time_t seg_end = std::min(chunk.t_close(), t_day_end);
        float start_minute = float(seg_start - t_day)/60.0;
        unsigned int seg_minutes = (seg_end > seg_start) ? (seg_end - seg_start)/60 : 0;
        bool continued = chunk.t_open < t_day;
        bool continues = chunk.t_close() > t_day_end;

        std::string card_text;
        if (!chunk.excerpt.empty()) {
            card_text = logcal_html_safe(chunk.excerpt);
        } else if (chunk.node_ptr) {
            std::string node_text(chunk.node_ptr->get_text().c_str());
            card_text = "<i>"+logcal_html_safe(logcal_utf8_cut(remove_html_tags(node_text), logcal_excerpt_length))+"</i>";
        }

        std::string color;
        if (chunk.is_open) {
            color = logcal_open_color;
        } else if (sleepNNL_ptr && chunk.node_ptr && sleepNNL_ptr->contains(chunk.node_ptr->get_id().key())) {
            color = logcal_sleep_color;
        } else {
            color = logcal_colors[color_idx % logcal_colors.size()];
        }
        color_idx++;

        std::string start_time = TimeStamp("%H:%M", chunk.t_open);
        if (continued) start_time = "(from "+TimeStamp("%a %H:%M", chunk.t_open)+")";
        std::string minutes_str = std::to_string(chunk.minutes)+" mins";
        if (chunk.is_open) minutes_str += " so far (open)";
        if (continues) minutes_str += " (continues)";

        template_varvalues vars;
        vars.emplace("chunk-id", chunk.chunk_id);
        vars.emplace("node-id", chunk.node_id);
        vars.emplace("card-color", color);
        vars.emplace("card-text", card_text);
        vars.emplace("start-time", start_time);
        vars.emplace("minutes", minutes_str);
        vars.emplace("vert-pos", to_precision_string(get_minute_vertical_position(start_minute), 2));
        // Height from the same scale as the position, so that neighbouring chunks meet exactly.
        float card_height = get_minute_vertical_position(start_minute + float(seg_minutes)) - get_minute_vertical_position(start_minute);
        vars.emplace("card-height", to_precision_string(card_height, 2));
        rendered_cards += env.render(card_template, vars);
    }

    template_varvalues column;
    column.emplace("column-header", header);
    column.emplace("column-cards", rendered_cards);
    rendered_columns += env.render(column_template, column);
    num_columns++;
    return true;
}

/**
 * Render the Log calendar page from nb.logcal_chunks. This uses the Graph only
 * through the node_ptr of chunks and nb.sleepNNL_ptr, which may be nullptr.
 */
bool render_log_calendar_page(nodeboard & nb, std::string & rendered_page) {
    std::string board_template, column_template, card_template;
    if (!file_to_string(logcal_template_dir+"Logcal_board.template.html", board_template)
        || !file_to_string(logcal_template_dir+"Logcal_column.template.html", column_template)
        || !file_to_string(logcal_template_dir+"Logcal_card.template.html", card_template)) {
        return standard_exit_error(exit_file_error, "Unable to load Log calendar templates from "+logcal_template_dir, __func__);
    }

    time_t today = today_start_time();
    time_t first_day = (nb.logcal_first_day == 0) ? today : nb.logcal_first_day;
    unsigned int days = nb.logcal_days;

    std::string rendered_days;
    time_t t_day = first_day;
    for (unsigned int i = 0; i < days; ++i) {
        nb.get_logcal_day_column(t_day, card_template, column_template, rendered_days);
        t_day = next_day_start(t_day);
    }
    time_t last_day = previous_day_start(t_day);

    float col_width = std::min(95.0/float(days), 60.0);
    float mult = nb.vertical_multiplier;

    // The Day button shows today if today is shown, else the first day shown.
    time_t day_view = ((today >= first_day) && (today < t_day)) ? today : first_day;
    // The Week button shows the week (from Monday) that contains the day above.
    int weekday_from_monday = (std::atoi(TimeStamp("%u", day_view).c_str()) + 6) % 7;
    time_t week_view = add_days(day_view, -weekday_from_monday);

    std::string header = nb.board_title_specified ? nb.board_title : "Log Calendar";
    std::string range = DateStampYmd(first_day);
    if (days > 1) range += " to "+DateStampYmd(last_day);

    template_varvalues board;
    board.emplace("board-header", header);
    board.emplace("range", range);
    board.emplace("column-widths", " repeat("+std::to_string(days)+", "+to_precision_string(col_width, 2)+"vw)");
    board.emplace("column-height", to_precision_string(nb.get_minute_vertical_position(1440.0, 8.0), 1)+"vh");
    board.emplace("prev-url", nb.logcal_cgi_call(add_days(first_day, -int(days)), days, mult));
    board.emplace("next-url", nb.logcal_cgi_call(add_days(first_day, days), days, mult));
    board.emplace("today-url", nb.logcal_cgi_call((days == 1) ? today : add_days(today, -int(days)+1), days, mult));
    board.emplace("day-url", nb.logcal_cgi_call(day_view, 1, mult));
    board.emplace("week-url", nb.logcal_cgi_call(week_view, 7, mult));
    board.emplace("smaller-url", nb.logcal_cgi_call(first_day, days, (mult > 1.0) ? mult - 1.0 : 1.0));
    board.emplace("larger-url", nb.logcal_cgi_call(first_day, days, mult + 1.0));
    board.emplace("first-day", TimeStamp("%Y-%m-%d", first_day));
    board.emplace("days", std::to_string(days));
    board.emplace("multiplier", to_precision_string(mult, 1));
    board.emplace("num-chunks", std::to_string(nb.logcal_chunks.size()));
    board.emplace("the-columns", rendered_days);

    rendered_page = nb.env.render(board_template, board);
    return true;
}

// Note: The times in the CSV data are already time zone adjusted by fzloghtml.
bool node_board_render_log_calendar(nodeboard & nb) {
    std::string csv_str;
    if (!file_to_string(nb.source_file, csv_str)) {
        return standard_exit_error(exit_file_error, "Unable to read file at "+nb.source_file, __func__);
    }

    if (!nb.parse_logcal_csv(csv_str)) {
        return standard_exit_error(exit_file_error, "Unparsable CSV file "+nb.source_file, __func__);
    }

    for (auto & chunk : nb.logcal_chunks) {
        chunk.node_ptr = nb.graph().Node_by_idstr(chunk.node_id);
    }
    nb.sleepNNL_ptr = nb.graph().get_List(logcal_sleepNNL); // optional, only used for color

    std::string rendered_page;
    if (!render_log_calendar_page(nb, rendered_page)) {
        return false;
    }
    return nb.to_output(rendered_page);
}
