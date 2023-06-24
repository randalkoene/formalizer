// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Template rendering functions.
 * 
 */

// core
#include "error.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "templater.hpp"
#include "ReferenceTime.hpp"

// local
#include "render.hpp"
#include "fzlogtime.hpp"


/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif


using namespace fz;

int hours_offset = 0;

enum template_id_enum {
    logtime_head_temp,
    logtime_tail_temp,
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "logtime-head-template.html",
    "logtime-tail-template.html"
};

typedef std::map<template_id_enum,std::string> fzlogtime_templates;

bool load_templates(fzlogtime_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i], templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

void append_rendered_hour(time_t t, std::string & rendered_str) {
    constexpr const time_t five_minutes_in_seconds = 5*60;
    time_t visible_t = t;
    if (hours_offset != 0) {
        visible_t += (hours_offset * 60 * 60);
    }
    rendered_str += TimeStamp(" <b>%H:</b>", visible_t); // actually generate this in case of Daylight Savings time change
    for (int i = 0; i < 12; i++) {
        if (i > 0)
            rendered_str += ' ';
        if (t > fzlt.edata.newest_chunk_t) {
            rendered_str += "<a href=\"/cgi-bin/fztask-cgi.py?T=" + TimeStamp("%Y%m%d%H%M", t);
            if (fzlt.nonlocal) {
                rendered_str += "&n=on\">"; // add link
            } else {
                rendered_str += "\">"; // add link
            }
        }
        if (t >= fzlt.T_page_build)
            rendered_str += "<b>"; // add bold
        rendered_str += ('0'+(i/2));
        rendered_str += (i & 1) ? '5' : '0';
        if (t >= fzlt.T_page_build)
            rendered_str += "</b>"; // close bold
        if (t > fzlt.edata.newest_chunk_t)
            rendered_str += "</a>"; // close link
        t += five_minutes_in_seconds;
    }
}

std::string cgi_variants_args() {
    std::string cvargs;
    if (fzlt.config.wrap_cgi_script) {
        cvargs += "wrap";
    }
    if (fzlt.config.skip_content_type) {
        if (!cvargs.empty()) {
            cvargs += ',';
        }
        cvargs += "skip";
    }
    if (!cvargs.empty()) {
        return "cgivar=" + cvargs + '&';
    }
    return cvargs;
}

std::string get_logtime_call_format() {
    if (fzlt.config.wrap_cgi_script) {
        if (fzlt.nonlocal) {
            return "fzlogtime.cgi?source=nonlocal&";
        } else {
            return "fzlogtime.cgi?";
        }
    }

    return "fzlogtime?";
}

/**
 * Generate a list of the last 14 days before the page date as links that
 * jump to specific previous day fzlogtime pages.
 * Each of these requires a link much like the previous day link, combining:
 * "/cgi-bin/{{ fzlogtime_call }}{{ cgi_variants }}D={{ day_Ymd }}"
 */
std::string make_recent_days_links_list() {
    std::string recent_days_links;
    constexpr const time_t day_in_seconds = 24*60*60;
    time_t T_day = fzlt.T_page_date;
    for (int i = 0; i < 14; i++) {
        T_day -= day_in_seconds;
        std::string T_day_str = DateStampYmd(T_day);
        std::string day_link = "/cgi-bin/"+get_logtime_call_format()+cgi_variants_args()+"D="+T_day_str;
        recent_days_links += "<a href=\""+day_link+"\">"+T_day_str+"</a> ";
        if (i==5) {
            recent_days_links += "<br>";
        }
    }
    return recent_days_links;
}

/**
 * Print an HTML page with time picking links for a given day.
 * 
 * Note that we are not actually using emulated time. We need to compare
 * times shown with actual time, but we do allow printing pages for
 * earlier days.
 * 
 * Note that the central portion of the page building code was copied from
 * dil2al/controller.cc:second_call_page().
 */
bool render_logtime_page() {
    render_environment env;
    fzlogtime_templates templates;
    load_templates(templates);

    fzlt.T_page_build = ActualTime();
    get_newest_Log_data(fzlt.ga, fzlt.edata);
    if (fzlt.T_page_date == RTt_unspecified) {
        fzlt.T_page_date = today_start_time();
    }

    std::string rendered_str;
    rendered_str.reserve(20480);

    if (!fzlt.embeddable) {
        template_varvalues varvals;
        varvals.emplace("T_page_date", DateStampYmd(fzlt.T_page_date));
        if (fzlt.cgi.called_as_cgi()) {
            varvals.emplace("cmd_or_cgi", "cgi");
        } else {
            varvals.emplace("cmd_or_cgi", "cmd");
        }
        if (fzlt.nonlocal) {
            varvals.emplace("hidden_nonlocal","&n=on");
        } else {
            varvals.emplace("hidden_nonlocal","");
        }
        rendered_str += env.render(templates[logtime_head_temp], varvals);
    }

    // build the day's time table
    constexpr const time_t sixty_minutes_in_seconds = 60*60;
    constexpr const time_t half_day_in_seconds = 12*60*60;
    rendered_str += "<pre>\n";
    time_t T_plus_12hrs = fzlt.T_page_date + half_day_in_seconds;
    for (time_t hour_date = fzlt.T_page_date; hour_date < T_plus_12hrs; hour_date += sixty_minutes_in_seconds) {
        append_rendered_hour(hour_date, rendered_str); // AM
        append_rendered_hour(hour_date + half_day_in_seconds, rendered_str); // PM
        rendered_str += "\n";
    }
    rendered_str += "</pre>\n";

    if (!fzlt.embeddable) {
        template_varvalues varvals;
        varvals.emplace("prevday", DateStampYmd(fzlt.T_page_date - 1));
        varvals.emplace("T_last_log_chunk", TimeStampYmdHM(fzlt.edata.newest_chunk_t));
        varvals.emplace("T_page_build", TimeStampYmdHM(fzlt.T_page_build));
        varvals.emplace("fzlogtime_call", get_logtime_call_format());
        varvals.emplace("cgi_variants", cgi_variants_args());
        if (fzlt.nonlocal) {
            varvals.emplace("hidden_nonlocal","<input type=\"hidden\" name=\"n\" value=\"on\">");
        } else {
            varvals.emplace("hidden_nonlocal","");
        }
        varvals.emplace("recent_days", make_recent_days_links_list() );
        rendered_str += env.render(templates[logtime_tail_temp], varvals);
    }

    if (fzlt.config.rendered_out_path == "STDOUT") {
    
        if (fzlt.cgi.called_as_cgi()) {
            if (!fzlt.config.skip_content_type) {
                rendered_str.insert(0,"Content-Type: text/html\n\n");
            }
            // *** This part only if responding directly through TCP port: FZOUT("HTTP/1.1 200 OK\nServer: aether\nContent-type: text/html\nContent-Length: "+std::to_string(rendered_str.size())+"\n");
        }
        FZOUT(rendered_str);
    } else {
        if (!string_to_file(fzlt.config.rendered_out_path,rendered_str))
            return standard_error("unable to write rendered page to file", __func__);
    }
    
    return true;
}
