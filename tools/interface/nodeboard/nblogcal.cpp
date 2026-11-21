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
    std::string rest = csv_line.substr(pos);
    while ((!rest.empty()) && ((rest.back() == '\r') || (rest.back() == ' '))) rest.pop_back();

    // Optional category field, e.g. @WORK@ (only the known tags, so an excerpt that
    // starts with something else is never taken for a category).
    category.clear();
    excerpt = rest;
    for (const std::string cat : { "WORK", "SELFWORK", "SYSTEM", "OTHER", "NONE" }) {
        std::string tag = '@'+cat+'@';
        if (rest.compare(0, tag.size(), tag) == 0) {
            if (rest.size() == tag.size()) {
                category = cat;
                excerpt.clear();
            } else if (rest[tag.size()] == ',') {
                category = cat;
                excerpt = rest.substr(tag.size() + 1);
            }
            break;
        }
    }

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
 * Parse '-a <YYYYmmdd>[,<days>]' or '-a <YYYYmm>' (month grid).
 * Instead of a date, 'today' may be used.
 */
bool nodeboard::parse_logcal_range(const std::string & cargs) {
    auto comma = cargs.find(',');
    std::string datestr = cargs.substr(0, comma);
    logcal_month = false;
    if (datestr == "today") {
        logcal_first_day = 0;
    } else if ((datestr.size() == 6) && all_digits(datestr)) {
        logcal_first_day = ymd_stamp_time(datestr+"01", true);
        if (logcal_first_day == RTt_invalid_time_stamp) {
            return standard_error("Invalid Log calendar month: "+datestr, __func__);
        }
        logcal_month = true;
        logcal_days = 0;
        std::string month = TimeStamp("%m", logcal_first_day);
        for (time_t t = logcal_first_day; TimeStamp("%m", t) == month; t = next_day_start(t)) logcal_days++;
        return true;
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

std::string nodeboard::logcal_month_cgi_call(time_t t_in_month) const {
    return "/cgi-bin/nodeboard-cgi.py?logcal="+TimeStamp("%Y%m", t_in_month)+"&M="+to_precision_string(vertical_multiplier, 1);
}

// Colors by category, as also shown in the legend.
const std::string logcal_sleep_color("#339afc");    // as sleep in the calendar Schedule
const std::vector<std::pair<std::string, std::string>> logcal_category_colors = {
    { "WORK", "#7dcf91" },
    { "SELFWORK", "#c3a6ec" },
    { "SYSTEM", "#f5c26b" },
    { "OTHER", "#bdbdbd" },
    { "NONE", "#f0f0f0" },
};
const std::string logcal_unknown_color("#e0f0ff");  // CSV data without category

const std::map<std::string, std::string> logcal_category_labels = {
    { "WORK", "work" },
    { "SELFWORK", "self-work" },
    { "SYSTEM", "system/care" },
    { "OTHER", "other" },
    { "NONE", "none" },
    { "", "" },
};

std::string logcal_chunk_color(const Logcal_chunk & chunk) {
    if (chunk.is_sleep) return logcal_sleep_color;
    for (const auto & [cat, color] : logcal_category_colors) {
        if (chunk.category == cat) return color;
    }
    return logcal_unknown_color;
}

std::string logcal_chunk_label(const Logcal_chunk & chunk) {
    if (chunk.is_sleep) return "sleep";
    auto it = logcal_category_labels.find(chunk.category);
    return (it != logcal_category_labels.end()) ? it->second : "";
}

std::string logcal_legend() {
    auto swatch = [](const std::string & color, const std::string & label) {
        return "<span class=\"logcal-swatch\" style=\"background-color:"+color+"\"></span>"+label+' ';
    };
    std::string legend("<span class=\"w3-small\">");
    for (const auto & [cat, color] : logcal_category_colors) {
        legend += swatch(color, logcal_category_labels.at(cat));
    }
    legend += swatch(logcal_sleep_color, "sleep");
    legend += "<span class=\"logcal-swatch logcal-open\"></span>open chunk</span>";
    return legend;
}

std::string logcal_card_text(const Logcal_chunk & chunk) {
    if (!chunk.excerpt.empty()) {
        return logcal_html_safe(chunk.excerpt);
    }
    if (chunk.node_ptr) {
        std::string node_text(chunk.node_ptr->get_text().c_str());
        return "<i>"+logcal_html_safe(logcal_utf8_cut(remove_html_tags(node_text), logcal_excerpt_length))+"</i>";
    }
    return "";
}

/// The part of a Log chunk that is within one day.
struct Logcal_segment {
    const Logcal_chunk * chunk;
    float start_minute;      ///< Minutes after the start of the day.
    unsigned int minutes;    ///< Minutes within the day.
    bool continued;          ///< The chunk began on an earlier day.
    bool continues;          ///< The chunk continues on a later day.
};

std::vector<Logcal_segment> logcal_day_segments(const std::vector<Logcal_chunk> & chunks, time_t t_day) {
    time_t t_day_end = next_day_start(t_day);
    std::vector<Logcal_segment> segments;
    for (const auto & chunk : chunks) {
        // Chunks that overlap the day, including 0 minute chunks that open in it.
        if (chunk.t_open >= t_day_end) continue;
        if ((chunk.t_open < t_day) && (chunk.t_close() <= t_day)) continue;

        time_t seg_start = std::max(chunk.t_open, t_day);
        time_t seg_end = std::min(chunk.t_close(), t_day_end);
        segments.push_back({
            &chunk,
            float(seg_start - t_day)/60.0f,
            (seg_end > seg_start) ? static_cast<unsigned int>((seg_end - seg_start)/60) : 0,
            chunk.t_open < t_day,
            chunk.t_close() > t_day_end
        });
    }
    return segments;
}

bool nodeboard::get_logcal_day_column(time_t t_day, const std::string & card_template, const std::string & column_template, std::string & rendered_columns) {
    std::string header = "<h4><a href=\""+logcal_cgi_call(t_day, 1, vertical_multiplier)+"\">"+WeekDay(t_day)+' '+DateStampYmd(t_day)+"</a></h4>";

    std::string rendered_cards;

    // Hour lines
    for (int hr = 0; hr < 24; ++hr) {
        std::string vpos = to_precision_string(get_minute_vertical_position(hr*60), 2);
        std::string hrstr = (hr < 10) ? '0'+std::to_string(hr) : std::to_string(hr);
        rendered_cards += "<div class=\"logcal-hour\" style=\"top:"+vpos+"vh;\">"+hrstr+":00</div>\n";
    }

    for (const auto & seg : logcal_day_segments(logcal_chunks, t_day)) {
        const Logcal_chunk & chunk = *seg.chunk;

        std::string start_time = TimeStamp("%H:%M", chunk.t_open);
        if (seg.continued) start_time = "(from "+TimeStamp("%a %H:%M", chunk.t_open)+")";
        std::string minutes_str = std::to_string(chunk.minutes)+" mins";
        if (chunk.is_open) minutes_str += " so far (open)";
        if (seg.continues) minutes_str += " (continues)";
        std::string label = logcal_chunk_label(chunk);
        if (!label.empty()) minutes_str += ", "+label;

        template_varvalues vars;
        vars.emplace("chunk-id", chunk.chunk_id);
        vars.emplace("node-id", chunk.node_id);
        vars.emplace("card-class", chunk.is_open ? " logcal-open" : "");
        vars.emplace("card-color", logcal_chunk_color(chunk));
        vars.emplace("card-text", logcal_card_text(chunk));
        vars.emplace("start-time", start_time);
        vars.emplace("minutes", minutes_str);
        vars.emplace("vert-pos", to_precision_string(get_minute_vertical_position(seg.start_minute), 2));
        // Height from the same scale as the position, so that neighbouring chunks meet exactly.
        float card_height = get_minute_vertical_position(seg.start_minute + float(seg.minutes)) - get_minute_vertical_position(seg.start_minute);
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

std::string logcal_hours(unsigned int minutes) {
    return to_precision_string(float(minutes)/60.0, 1)+'h';
}

/**
 * A day in the month grid: four rows of 6 hours, each with the chunk segments
 * as colored blocks. Hovering a block shows its time and excerpt, clicking it
 * opens the chunk pop-up. The date links to the Day view and its tooltip has
 * the hours per category.
 */
bool nodeboard::get_logcal_month_cell(time_t t_day, bool is_today, const std::string & cell_template, std::string & rendered_cells) {
    const float row_minutes = 360.0;
    std::vector<std::string> rows(4);
    std::map<std::string, unsigned int> totals; // label -> minutes
    unsigned int total_logged = 0;

    for (const auto & seg : logcal_day_segments(logcal_chunks, t_day)) {
        if (seg.minutes == 0) continue;
        const Logcal_chunk & chunk = *seg.chunk;
        std::string label = logcal_chunk_label(chunk);
        totals[label.empty() ? "unknown" : label] += seg.minutes;
        total_logged += seg.minutes;

        std::string title = TimeStamp("%H:%M", chunk.t_open)+" ("+std::to_string(chunk.minutes)+" mins";
        if (chunk.is_open) title += " so far";
        title += ")";
        if (!label.empty()) title += ' '+label;
        std::string text = chunk.excerpt;
        if (text.empty() && chunk.node_ptr) {
            std::string node_text(chunk.node_ptr->get_text().c_str());
            text = logcal_utf8_cut(remove_html_tags(node_text), logcal_excerpt_length);
        }
        title += ": "+text;
        title = logcal_html_safe(title);

        float seg_end = seg.start_minute + float(seg.minutes);
        for (int r = 0; r < 4; ++r) {
            float row_start = r*row_minutes;
            float row_end = row_start + row_minutes;
            float from = std::max(seg.start_minute, row_start);
            float to = std::min(seg_end, row_end);
            if (to <= from) continue;
            std::string left = to_precision_string(100.0*(from - row_start)/row_minutes, 2);
            std::string width = to_precision_string(100.0*(to - from)/row_minutes, 2);
            rows[r] += "<div class=\"logcal-seg"+std::string(chunk.is_open ? " logcal-open" : "")+"\" data-chunk=\""+chunk.chunk_id
                +"\" style=\"left:"+left+"%;width:"+width+"%;background-color:"+logcal_chunk_color(chunk)+"\" title=\""+title+"\"></div>";
        }
    }

    std::string day_rows;
    const std::vector<std::string> row_labels = { "00", "06", "12", "18" };
    for (int r = 0; r < 4; ++r) {
        day_rows += "<div class=\"logcal-sixh\"><span class=\"logcal-sixh-label\">"+row_labels[r]+"</span><div class=\"logcal-sixh-bar\">"+rows[r]+"</div></div>\n";
    }

    std::string totals_str = "Logged "+logcal_hours(total_logged);
    for (const auto & [cat, color] : logcal_category_colors) {
        const std::string & label = logcal_category_labels.at(cat);
        if (totals.count(label)) totals_str += ", "+label+' '+logcal_hours(totals[label]);
    }
    for (const std::string label : { "sleep", "unknown" }) {
        if (totals.count(label)) totals_str += ", "+label+' '+logcal_hours(totals[label]);
    }

    template_varvalues vars;
    vars.emplace("cell-class", is_today ? " logcal-today" : "");
    vars.emplace("day-url", logcal_cgi_call(t_day, 1, vertical_multiplier));
    vars.emplace("day-label", TimeStamp("%e", t_day));
    vars.emplace("day-totals", totals_str);
    vars.emplace("day-rows", day_rows);
    rendered_cells += env.render(cell_template, vars);
    return true;
}

std::string render_log_calendar_month(nodeboard & nb, time_t first_day, time_t today, const std::string & cell_template) {
    std::string cells;
    for (const std::string wd : { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" }) {
        cells += "<div class=\"logcal-weekday\">"+wd+"</div>\n";
    }
    int offset = (std::atoi(TimeStamp("%u", first_day).c_str()) + 6) % 7; // Monday = 0
    for (int i = 0; i < offset; ++i) {
        cells += "<div class=\"logcal-daycell logcal-outside\"></div>\n";
    }
    time_t t_day = first_day;
    for (unsigned int i = 0; i < nb.logcal_days; ++i) {
        nb.get_logcal_month_cell(t_day, t_day == today, cell_template, cells);
        t_day = next_day_start(t_day);
    }
    unsigned int used = offset + nb.logcal_days;
    for (; (used % 7) != 0; ++used) {
        cells += "<div class=\"logcal-daycell logcal-outside\"></div>\n";
    }
    return cells;
}

/**
 * Render the Log calendar page from nb.logcal_chunks. This uses the Graph only
 * through the node_ptr of chunks, which may be nullptr.
 */
bool render_log_calendar_page(nodeboard & nb, std::string & rendered_page) {
    std::string board_template, column_template, card_template, cell_template;
    if (!file_to_string(logcal_template_dir+"Logcal_board.template.html", board_template)
        || !file_to_string(logcal_template_dir+"Logcal_column.template.html", column_template)
        || !file_to_string(logcal_template_dir+"Logcal_card.template.html", card_template)
        || !file_to_string(logcal_template_dir+"Logcal_daycell.template.html", cell_template)) {
        return standard_exit_error(exit_file_error, "Unable to load Log calendar templates from "+logcal_template_dir, __func__);
    }

    time_t today = today_start_time();
    time_t first_day = (nb.logcal_first_day == 0) ? today : nb.logcal_first_day;
    unsigned int days = nb.logcal_days;
    float mult = nb.vertical_multiplier;

    std::string rendered_days;
    time_t t_day = first_day;
    if (nb.logcal_month) {
        rendered_days = render_log_calendar_month(nb, first_day, today, cell_template);
        t_day = add_days(first_day, days);
    } else {
        for (unsigned int i = 0; i < days; ++i) {
            nb.get_logcal_day_column(t_day, card_template, column_template, rendered_days);
            t_day = next_day_start(t_day);
        }
    }
    time_t last_day = previous_day_start(t_day);

    // The Day button shows today if today is shown, else the first day shown.
    time_t day_view = ((today >= first_day) && (today < t_day)) ? today : first_day;
    // The Week button shows the week (from Monday) that contains the day above.
    int weekday_from_monday = (std::atoi(TimeStamp("%u", day_view).c_str()) + 6) % 7;
    time_t week_view = add_days(day_view, -weekday_from_monday);

    std::string header = nb.board_title_specified ? nb.board_title : "Log Calendar";
    std::string range;
    if (nb.logcal_month) {
        range = TimeStamp("%B %Y", first_day);
    } else {
        range = DateStampYmd(first_day);
        if (days > 1) range += " to "+DateStampYmd(last_day);
    }

    template_varvalues board;
    board.emplace("board-header", header);
    board.emplace("range", range);
    board.emplace("legend", logcal_legend());
    if (nb.logcal_month) {
        board.emplace("column-widths", " repeat(7, 13.2vw)");
        board.emplace("prev-url", nb.logcal_month_cgi_call(previous_day_start(first_day)));
        board.emplace("next-url", nb.logcal_month_cgi_call(t_day));
        board.emplace("today-url", nb.logcal_month_cgi_call(today));
        board.emplace("size-style", "display:none;");
        board.emplace("is-month", "1");
        board.emplace("hint", "Hover over a block to see it, click it to open the chunk. The date opens the Day view, its tooltip has hours per category.");
    } else {
        float col_width = std::min(95.0/float(days), 60.0);
        board.emplace("column-widths", " repeat("+std::to_string(days)+", "+to_precision_string(col_width, 2)+"vw)");
        board.emplace("prev-url", nb.logcal_cgi_call(add_days(first_day, -int(days)), days, mult));
        board.emplace("next-url", nb.logcal_cgi_call(add_days(first_day, days), days, mult));
        board.emplace("today-url", nb.logcal_cgi_call((days == 1) ? today : add_days(today, -int(days)+1), days, mult));
        board.emplace("size-style", "");
        board.emplace("is-month", "0");
        board.emplace("hint", "Click a chunk to see it.");
    }
    board.emplace("column-height", to_precision_string(nb.get_minute_vertical_position(1440.0, 8.0), 1)+"vh");
    board.emplace("day-url", nb.logcal_cgi_call(day_view, 1, mult));
    board.emplace("week-url", nb.logcal_cgi_call(week_view, 7, mult));
    board.emplace("month-url", nb.logcal_month_cgi_call(day_view));
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

    nb.sleepNNL_ptr = nb.graph().get_List(logcal_sleepNNL); // optional, only used for color
    for (auto & chunk : nb.logcal_chunks) {
        chunk.node_ptr = nb.graph().Node_by_idstr(chunk.node_id);
        chunk.is_sleep = nb.sleepNNL_ptr && chunk.node_ptr && nb.sleepNNL_ptr->contains(chunk.node_ptr->get_id().key());
    }

    std::string rendered_page;
    if (!render_log_calendar_page(nb, rendered_page)) {
        return false;
    }
    return nb.to_output(rendered_page);
}
