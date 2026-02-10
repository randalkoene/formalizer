// Copyright 2024 Randal A. Koene
// License TBD

/**
 * Schedule rendering functions.
 * 
 * For more about this, see the Trello card at https://trello.com/c/w2XnEQcc
 */

#include "error.hpp"
#include "general.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "stringio.hpp"
#include "templater.hpp"
#include "schedule.hpp"

/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif

using namespace fz;

typedef std::map<template_id_enum,std::string> schedule_templates;

const std::vector<std::string> template_ids = {
    "html_entry",
};

bool load_templates(schedule_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i] + ".template.html", templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

bool schedule::render_init() {
    return load_templates(templates);
}

const std::string default_web_schedule_base("/var/www/webdata/formalizer");
const std::string default_html_schedule_base("/dev/shm");
const std::string default_htmlschedulefile("/fzschedule.html");
const std::string default_csv_schedule_base("/tmp");
const std::string default_csvschedulefile("/fzschedule.csv");

const char * map_html_head = R"HTMLHEAD(<html>
<head>
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fz-cards.css">
<link rel="stylesheet" href="/bluetable.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Formalizer: Proposed Schedule</title>
</head>
<body>
<h2>Formalizer: Proposed Schedule</h2>
<button id="darkmode" class="button button2" onclick="switch_light_or_dark();">Light / Dark</button>
<table class="blueTable">
<tbody>
)HTMLHEAD";

const char * map_html_tail = R"HTMLTAIL(</tbody>
</table>
<hr>
<p>[<a href="/index.html">fz: Top</a>]</p>

<script type="module" src="/fzuistate.js"></script>
</body>
</html>
)HTMLTAIL";

const char * map_csv_head = R"CSVHEAD(start,minutes,tdprop,id
)CSVHEAD";

const std::vector<std::string> dayquarter_color = {
    "000000",
    "007f00",
    "0000ff",
    "ff00ff",
};

struct Interval {
	time_t start;
	time_t end;
	unsigned long minutes;

	Interval(time_t t_today_start, unsigned long start_minute, unsigned long end_minute) {
		start = t_today_start + (start_minute*60);
		end = t_today_start + (end_minute*60);
		minutes = end_minute - start_minute;
	}
	unsigned long hrs() const { return minutes / 60; }
	unsigned long mins() const { return minutes % 60; }
};

unsigned long find_dayquarter(const std::string & hr_str) {
	unsigned int hr = atoi(hr_str.c_str());
	return hr / 6;
}

std::string render_entry_html(schedule & sch, unsigned long start_minute, unsigned long end_minute, Node_ID_key & nkey) {
	Node * node_ptr = sch.graph().Node_by_id(nkey);
	if (!node_ptr) {
		return "";
	}

	Interval interval(sch.t_today_start, start_minute, end_minute);

	unsigned long interval_hrs = interval.hrs();
	std::string interval_hrs_str;
    if (interval_hrs >= 1) {
    	interval_hrs_str = std::to_string(interval_hrs);
    	if (interval_hrs == 1) {
    		interval_hrs_str += " hr ";
    	} else {
    		interval_hrs_str += " hrs ";
    	}
    }
    unsigned long interval_mins = interval.mins();
    std::string interval_start_str = TimeStamp("%x %H:%M,", interval.start);
    unsigned long dayquarter = find_dayquarter(interval_start_str.substr(interval_start_str.size()-5, 2));
    std::string node_desc = replace_char(node_ptr->get_text().c_str(), '\n', ' ');

    template_varvalues entryvars;
    entryvars.emplace("dayquarter-color", dayquarter_color.at(dayquarter));
    entryvars.emplace("interval-start", interval_start_str);
    entryvars.emplace("interval-hours", interval_hrs_str);
    entryvars.emplace("interval-minutes", std::to_string(interval_mins));
    entryvars.emplace("node-id", nkey.str());
    entryvars.emplace("tdprop", node_ptr->get_tdproperty_str());
    entryvars.emplace("node-desc", node_desc);

    return sch.env.render(sch.templates[html_entry_temp], entryvars);
}

bool schedule_render_html(schedule & sch) {
	sch.graph().set_tzadjust_active(sch.use_tzadjust);
	if (!sch.generate_schedule()) {
		sch.graph().set_tzadjust_active(false);  // One should only activate it temporarily.
		return false;
	}
	sch.graph().set_tzadjust_active(false);  // One should only activate it temporarily.

	sch.render_init();

	Node_ID_key nkey;
	unsigned long start_minute = 0;
	std::string rows;
	for (unsigned long minute = sch.passed_minutes; minute < sch.daysmap.size(); minute++) {
		if (sch.daysmap.at(minute) != nkey) {
			if (!nkey.isnullkey()) {
				rows += render_entry_html(sch, start_minute, minute, nkey);
			}
			nkey = sch.daysmap.at(minute);
			start_minute = minute;
		}
	}
	if (!nkey.isnullkey()) {
		rows += render_entry_html(sch, start_minute, sch.daysmap.size(), nkey);
	}

	std::string map_html(map_html_head);
	map_html += rows + map_html_tail;

	if (sch.output_path.empty()) {
		if (sch.flowcontrol == flow_html) {
			sch.output_path = default_html_schedule_base + default_htmlschedulefile;
		} else {
			sch.output_path = default_web_schedule_base + default_htmlschedulefile;
		}
	}
	return sch.to_output(map_html);
}

std::string render_entry_csv(schedule & sch, unsigned long start_minute, unsigned long end_minute, Node_ID_key & nkey) {
	Node * node_ptr = sch.graph().Node_by_id(nkey);
	if (!node_ptr) {
		return "";
	}

	Interval interval(sch.t_today_start, start_minute, end_minute);

	std::string entry(TimeStamp("%x %H:%M,", interval.start));
	entry += std::to_string(interval.minutes) + ',';
	entry += node_ptr->get_tdproperty_str() + ',';
	entry += nkey.str() + '\n';
	return entry;
}

/**
 * Note about time zone adjustment:
 * The data generated for the schedule has had all the adjustments applied through
 * @TZADJUST@ and through offsets applied when variable target dates were updated.
 * The time distances between now and those times need to be preserved and used.
 * Hence, further adjustment of the output generated can only be about presentation
 * of the output, i.e. where days start and end and which minutes in a day are
 * assigned. E.g. the "current time" means a different minute in the day, as does
 * midnight.
 */
bool schedule_render_csv(schedule & sch) {
	sch.graph().set_tzadjust_active(sch.use_tzadjust);
	if (!sch.generate_schedule()) {
		sch.graph().set_tzadjust_active(false);  // One should only activate it temporarily.
		return false;
	}
	sch.graph().set_tzadjust_active(false);  // One should only activate it temporarily.

	//sch.t_today_start = today_start_time() + sch.timezone_offset_seconds;

	Node_ID_key nkey;
	unsigned long start_minute = 0;
	std::string rows;
	for (unsigned long minute = sch.passed_minutes; minute < sch.daysmap.size(); minute++) {
		if (sch.daysmap.at(minute) != nkey) {
			if (!nkey.isnullkey()) {
				rows += render_entry_csv(sch, start_minute, minute, nkey);
			}
			nkey = sch.daysmap.at(minute);
			start_minute = minute;
		}
	}
	if (!nkey.isnullkey()) {
		rows += render_entry_csv(sch, start_minute, sch.daysmap.size(), nkey);
	}

	std::string map_csv(map_csv_head);
	map_csv += rows;

	if (sch.output_path.empty()) {
		if (sch.flowcontrol == flow_csv) {
			sch.output_path = default_csv_schedule_base + default_csvschedulefile;
		} else {
			sch.output_path = default_web_schedule_base + default_csvschedulefile;
		}
	}
	return sch.to_output(map_csv);
}
