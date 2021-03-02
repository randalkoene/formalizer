// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Template rendering functions.
 * 
 */

// std
#include <memory>
#include <vector>
#include <cstdlib>

// core
#include "error.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "jsonlite.hpp"
#include "templater.hpp"

// local
#include "render.hpp"
#include "fzdashboard.hpp"


/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif


using namespace fz;


enum template_id_enum {
    index_temp,
    index_section_temp,
    index_button_here_temp,
    index_button_there_temp,
    NUM_temp
};

// const std::vector<std::string> dynamics_template_ids = {
//     "index_template.html",
//     "index_section_template.html",
//     "index_button_here_template.html",
//     "index_button_there_template.html",
// };

// const std::vector<std::string> static_template_ids = {
//     "index-static_template.html",
//     "index-static_section_template.html",
//     "index-static_button_here_template.html",
//     "index-static_button_there_template.html"  
// };

const std::vector<std::string> template_ids[2] = {
    {
        "index_template.html",
        "index_section_template.html",
        "index_button_here_template.html",
        "index_button_there_template.html",
    },
    {
        "index-static_template.html",
        "index-static_section_template.html",
        "index-static_button_here_template.html",
        "index-static_button_there_template.html"  
    }
};

typedef std::map<template_id_enum,std::string> fzdashboard_templates;

bool load_templates(fzdashboard_templates & templates, dynamic_or_static html_output = dynamic_html) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[html_output][i], templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[html_output][i]);
    }

    return true;
}

struct button_data {
    std::string url;
    std::string label;
};

struct section_data {
    std::string title;
    std::vector<button_data> buttons;
};

struct button_section_data {
    std::vector<section_data> sections;
};

std::string jsonstr = R"JSON(
{
    "NOTITLE" : {
        "Next Nodes" : {
            "url" : "/cgi-bin/fzgraphhtml-cgi.py",
            "window" : "_blank"
        },
        "Recent Log" : {
            "url" : "/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END",
            "window" : "_blank"
        },
        "Named Node Lists" : {
            "url" : "/cgi-bin/fzgraphhtml-cgi.py?srclist=?",
            "window" : "_blank"
        },
        "Topics" : {
            "url" : "/cgi-bin/fzgraphhtml-cgi.py?topics=?",
            "window" : "_blank"
        },
        "Calendar" : {
            "url" : "https://calendar.google.com/calendar/",
            "window" : "_blank"
        },
        "Morning Protocol" : {
            "url" : "/cgi-bin/fzguide.system-cgi.py?submit=Read",
            "window" : "_blank"
        },
        "Action Protocol" : {
            "url" : "/cgi-bin/fzguide.system-cgi.py?submit=Read&chapter=ACT&spec_chapter=on",
            "window" : "_blank"
        },
        "Integrity" : {
            "url" : "/integrity.html",
            "window" : "_blank"
        },
    },
    "NOTITLE" : {
        "Metrics" : {
            "url" : "https://docs.google.com/spreadsheets/d/1LeVD95TeBv_3QNby-Robe_BXS20g3WJEO3udy9mIb8I/edit#gid=0",
            "window" : "_blank"
        },
        "DAF &amp; Comm" : {
            "url" : "https://docs.google.com/spreadsheets/d/1p1lLpNOy6OeBQC_MNpzRqsLCPFEpnaF-ZfEAZ1KlRFc/edit#gid=1467656114",
            "window" : "_blank"
        },
        "Reward &amp; Positive" : {
            "url" : "https://docs.google.com/spreadsheets/d/1oDD4moypes9qII1olFaLWhydOPCWFa7f_u6hWXTZVP4/edit#gid=1180724961",
            "window" : "_blank"
        },
        "LVL4 Day Plan" : {
            "url" : "https://docs.google.com/spreadsheets/d/1mNAYcHlFjXHzSFHHli-_cOfeEjxwcRizJlwbWYPIZA8/edit#gid=0",
            "window" : "_blank"
        },
        "Notes on Change" : {
            "url" : "https://docs.google.com/document/d/1lBYknyQHhlhlqYZ3gku7OFTuM5FpFXV62jE2ETOnjv4/edit#heading=h.6algess7d8zh",
            "window" : "_blank"
        },
        "Accounting - Temp-New" : {
            "url" : "https://docs.google.com/spreadsheets/d/1-be-haSP3cqnwinYokcg0x91e8f-_LBT9HQOy9QHhxs/edit#gid=0",
            "window" : "_blank"
        },
    },
    "NOTITLE" : {
        "Server Status" : {
            "url" : "/cgi-bin/fzserver-info-cgi.py",
            "window" : "_blank"
        },
        "Port Status" : {
            "url" : "http://aether.local:8090/fz/status",
            "window" : ""
        },
        "ReqQ Status" : {
            "url" : "http://aether.local:8090/fz/ReqQ",
            "window" : ""
        },
        "ErrQ Status" : {
            "url" : "http://aether.local:8090/fz/ErrQ",
            "window" : ""
        },
        "Verbose" : {
            "url" : "http://aether.local:8090/fz/verbosity?set=normal",
            "window" : ""
        },
        "Very Verbose" : {
            "url" : "http://aether.local:8090/fz/verbosity?set=very",
            "window" : ""
        },
        "Quiet" : {
            "url" : "http://aether.local:8090/fz/verbosity?set=quiet",
            "window" : ""
        },
        "DB mode" : {
            "url" : "http://aether.local:8090/fz/db/mode",
            "window" : ""
        },
        "DB run" : {
            "url" : "http://aether.local:8090/fz/db/mode?set=run",
            "window" : ""
        },
        "DB log" : {
            "url" : "http://aether.local:8090/fz/db/mode?set=log",
            "window" : ""
        },
        "DB sim" : {
            "url" : "http://aether.local:8090/fz/db/mode?set=sim",
            "window" : ""
        },
        "DB Log Status" : {
            "url" : "http://aether.local:8090/fz/db/log",
            "window" : ""
        },
        "set NNL persistent" : {
            "url" : "http://aether.local:8090/fz/graph/namedlists/_set?persistent=true",
            "window" : ""
        },
        "set NNL non-persistent" : {
            "url" : "http://aether.local:8090/fz/graph/namedlists/_set?persistent=false",
            "window" : ""
        },
        "reload NNL" : {
            "url" : "http://aether.local:8090/fz/graph/namedlists/_reload",
            "window" : ""
        },
        "Stop Server" : {
            "url" : "http://aether.local:8090/fz/_stop",
            "window" : ""
        },
    },
    "System Boards: " : {
        "Challenges" : {
            "url" : "https://trello.com/b/mnu1VKIn",
            "window" : "_blank"
        },
        "Milestones" : {
            "url" : "https://trello.com/b/t2RmUmlN",
            "window" : "_blank"
        },
        "Roadmap" : {
            "url" : "https://trello.com/b/cOCvAYh4",
            "window" : "_blank"
        },
        "Metrics" : {
            "url" : "https://trello.com/b/e3sHVgMT",
            "window" : "_blank"
        },
        "Experiments" : {
            "url" : "https://trello.com/b/tlgXjZBm",
            "window" : "_blank"
        },
        "Concierge" : {
            "url" : "https://trello.com/b/ptGdOpzW",
            "window" : "_blank"
        },
        "Meditation, Mindstate & Flow" : {
            "url" : "https://trello.com/b/6ZfhOnuN",
            "window" : "_blank"
        },
    },
    "Current Tracks: " : {
        "VOXA" : {
            "url" : "https://trello.com/b/nyrFIMyd",
            "window" : "_blank"
        },
        "SFBAY-REGULAR-JOB" : {
            "url" : "https://trello.com/b/kdNO6dFD",
            "window" : "_blank"
        },
        "FORMALIZER-NEWAL-METHOD" : {
            "url" : "https://trello.com/b/PEfStDEN",
            "window" : "_blank"
        },
        "Formalizer Tools" : {
            "url" : "https://trello.com/b/uMW6fb4e",
            "window" : "_blank"
        },
        "CARBONCOPIES" : {
            "url" : "https://trello.com/b/ABTA2xEA",
            "window" : "_blank"
        },
    },
    "Track Collections: " : {
        "Financials, Income &amp; Capital" : {
            "url" : "https://trello.com/b/n8YN7jwd",
            "window" : "_blank"
        },
        "Solving Debt" : {
            "url" : "https://trello.com/b/gPUx2CON",
            "window" : "_blank"
        },
        "Health" : {
            "url" : "https://trello.com/b/uNGTL6aO",
            "window" : "_blank"
        },
        "Writing Projects" : {
            "url" : "https://trello.com/b/U81T3ycQ",
            "window" : "_blank"
        },
    },
    "Information: " : {
        "C++ / C" : {
            "url" : "https://trello.com/b/BIRsdsnE",
            "window" : "_blank"
        },
        "Software Engineering" : {
            "url" : "https://trello.com/b/Jjli7nWA",
            "window" : "_blank"
        },
        "Science" : {
            "url" : "https://trello.com/b/yPGMhTGw",
            "window" : "_blank"
        },
        "NMA Data Science" : {
            "url" : "https://trello.com/b/aE1VCkGj",
            "window" : "_blank"
        },
        "Ready Docs" : {
            "url" : "/formalizer/ready-docs.html",
            "window" : "_blank"
        },
        "Weather Service (94122)" : {
            "url" : "https://forecast.weather.gov/MapClick.php?lat=37.76012500000007&lon=-122.47004999999996#.X85MfGhKhhE",
            "window" : "_blank"
        },
        "Time and Date" : {
            "url" : "https://www.timeanddate.com/sun/usa/san-francisco",
            "window" : "_blank"
        },
    },
    "Action: " : {
        "Add Node" : {
            "url" : "/formalizer/add_node.html",
            "window" : "_blank"
        },
        "Add Log Entry" : {
            "url" : "",
            "window" : ""
        },
        "Search Graph" : {
            "url" : "/formalizer/fzgraphsearch-form.html",
            "window" : "_blank"
        },
    }
}
)JSON";

// *** We don't need this if we make JSON blocks use maps to search by label.
std::string value_by_label(JSON_element_data_vec & buffers, const std::string matchlabel) {
    for (auto & buffer : buffers) {
        if (buffer.label == matchlabel) {
            return buffer.text;
        }
    }
    return "";
}

std::string render_buttons(render_environment & env, fzdashboard_templates & templates, JSON_block * block_ptr, dynamic_or_static html_output = dynamic_html) {
    if (!block_ptr) {
        return "";
    }
    std::string buttons_str;
    JSON_element_data_vec button_info;
    button_info.emplace_back("url");
    button_info.emplace_back("window");
    std::string button_num_char("1");
    for (auto & button : block_ptr->elements) {
        if (is_populated_JSON_block(button.get())) {
            template_varvalues varvals;
            varvals.emplace("label", button->label);
            bool here = false;
            if (button->children->find_many(button_info) == 2) {
                here = (value_by_label(button_info, "window") != "_blank");
                varvals.emplace("url", value_by_label(button_info, "url"));
                if (html_output == dynamic_html) {
                    varvals.emplace("num", button_num_char);
                }
                if (here) {
                    buttons_str += env.render(templates[index_button_here_temp], varvals);
                } else {
                    buttons_str += env.render(templates[index_button_there_temp], varvals);
                }
                if (button_num_char == "1") {
                    button_num_char = "2";
                } else {
                    button_num_char = "1";
                }
            }
        }
    }
    return buttons_str;
}

bool send_rendered_to_output(std::string & filename_without_dir, std::string & rendered_text) {
    if ((fzdsh.config.top_path.empty()) || (fzdsh.config.top_path == "STDOUT")) { // to STDOUT
        //VERBOSEOUT("Log interval:\n\n");
        FZOUT(rendered_text);
        return true;
    }
    
    std::string output_path = fzdsh.config.top_path + filename_without_dir;
    VERBOSEOUT("Writing rendered output to "+output_path+".\n\n");
    if (!string_to_file(output_path,rendered_text)) {
        ERRRETURNFALSE(__func__,"unable to write to "+output_path);
    }
    return true;
}

bool render(dynamic_or_static html_output) {
    JSON_data data(jsonstr);
    VERYVERBOSEOUT("JSON data:\n"+data.json_str());
    FZOUT("Number of blocks  : "+std::to_string(data.blocks())+'\n');
    FZOUT("Number of elements: "+std::to_string(data.size())+'\n');

    render_environment env;
    fzdashboard_templates templates;
    load_templates(templates, html_output);

    std::string button_sections;

    for (auto & section : data.content_elements()) {
        if (is_populated_JSON_block(section.get())) {
            template_varvalues varvals;
            if (section->label == "NOTITLE") {
                varvals.emplace("section_heading", "");
            } else {
                varvals.emplace("section_heading", section->label);
            }
            varvals.emplace("buttons", render_buttons(env, templates, section->children.get()));
            button_sections += env.render(templates[index_section_temp], varvals);
        }
    }

    template_varvalues varvals;
    varvals.emplace("button-sections",button_sections);
    std::string rendered_str = env.render(templates[index_temp], varvals);

    std::string output_name;
    if (html_output == dynamic_html) {
        output_name += "/index.html";
    } else {
        output_name += "/index-static.html";
    }
    
    return send_rendered_to_output(output_name, rendered_str);
}
