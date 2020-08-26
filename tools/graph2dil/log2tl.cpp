// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This contains functions for validation tets for the converted Log.
 * 
 * This file has been separated from tl2log.cpp in order to isolate the use of the
 * inja.hpp library, which takes a considerable amount of time to compile.
 * 
 * For more development details, see the Trello card at https://trello.com/c/NNSWVRFf.
 */

// needs these for the HTML output test samples of converted Log data
#include <nlohmann/json.hpp>
#include <inja/inja.hpp>

#include "error.hpp"
#include "general.hpp"
#include "Logtypes.hpp"
#include "Graphtypes.hpp"

using namespace fz;

/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string test_template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string test_template_dir("./templates");
#endif

enum section_status { section_ok, section_out_of_bounds, section_fist_chunk_not_found };

/**
 * This structure contains all the necessary data and rendering functions
 * to convert the content from a Breakpoint start in a Log to its end
 * (as defined by the next Breakpoint if there is one).
 * 
 * The rendering functions make use of templates that are defined in
 * template files 'TL_*.template.html' in the 'templates/' directory
 * of the source code. The templates were made by mapping components
 * of actual dil2al Task Log files.
 * 
 * section_Converter constructor:
 * @param _graph a complete in-memory Graph.
 * @param _log a complete in-memory Log.
 * @param _bridx index to the Breakpoint for which to convert a section. 
 */
struct section_Converter {
protected:
    section_status status;
public:
    inja::Environment env;

    Graph & graph;
    Log & log;
    Log_chunk_ID_key_deque::size_type bridx;

    std::string yyyymmdd;
    Log_chunk_ptr_deque::size_type chunk_begin_idx; // index of first chunk in section
    Log_chunk_ptr_deque::size_type chunk_end_limit; // one past index of last chunk in section

    std::string rendered_section; /// Holds the result of section rendering process.

    section_Converter() = delete; // explicitly forbid the default constructor, just in case
    section_Converter(Graph & _graph, Log & _log, Log_chunks_Deque::size_type _bridx): status(section_ok), graph(_graph), log(_log), bridx(_bridx) {
        if (bridx>=log.num_Breakpoints()) {
            status = section_out_of_bounds;
            ADDERROR(__func__,"section out of bounds at breakpoint index "+std::to_string(bridx));

        } else {
            yyyymmdd = log.get_Breakpoint_Ymd_str(bridx);
            chunk_begin_idx = log.get_chunk_first_at_Breakpoint(bridx);

            if (chunk_begin_idx>=log.num_Chunks()) {
                status = section_fist_chunk_not_found;
                ADDERROR(__func__,"section first chunk not found "+log.get_Breakpoint_first_chunk_id_str(bridx));

            } else {
                auto next_idx = bridx+1;
                if (next_idx>=log.num_Breakpoints()) {
                    chunk_end_limit = log.num_Chunks();
                } else {
                    chunk_end_limit = log.get_chunk_first_at_Breakpoint(next_idx);
                }
            }
        }
    }

    enum template_id_enum {
        section_temp,
        section_next_temp,
        section_nonext_temp,
        section_prev_temp,
        section_noprev_temp,
        chunk_temp,
        chunk_ALnext_temp,
        chunk_ALnonext_temp,
        chunk_ALprev_temp,
        chunk_ALnoprev_temp,
        chunk_nodenext_temp,
        chunk_nodenonext_temp,
        chunk_nodeprev_temp,
        chunk_nodenoprev_temp,
        entry_temp,
        entry_withnode_temp,
        NUM_temp        
    };

    const std::vector<std::string> template_ids = {
        "section",
        "section_next",
        "section_nonext",
        "section_prev",
        "section_noprev",
        "chunk",
        "chunk_ALnext",
        "chunk_ALnonext",
        "chunk_ALprev",
        "chunk_ALnoprev",
        "chunk_nodenext",
        "chunk_nodenonext",
        "chunk_nodeprev",
        "chunk_nodenoprev",
        "entry",
        "entry_withnode"
    };

    std::map<template_id_enum,std::string> templates;

    bool load_templates() {
        templates.clear();

        for (int i = 0; i < NUM_temp; ++i) {
            if (!file_to_string(test_template_dir + "/TL_"+template_ids[i]+".template.html", templates[static_cast<template_id_enum>(i)]))
                ERRRETURNFALSE(__func__, "unable to load "+template_ids[i]);
        }

        return true;
    }

    std::string prevsection;
    std::string nextsection;
    bool have_looked_for_chunks = false; // empty chunks is possible
    std::string chunks;
    std::string section;

    /**
     * Before calling this function:
     * 1. Make sure that `yyyymmdd` has been derived from Breakpoint.
     * 2. Make sure that `prevsection` has been generated with `render_prevsection()`.
     * 3. Make sure that `nextsection` has been generated with `render_nextsection()`.
     * 4. Make sure that `chunks` has been generated.
     */
    bool render_section() {
        if (status!=section_ok)
            ERRRETURNFALSE(__func__,"section status does not permit rendering");

        if (yyyymmdd.empty())
            ERRRETURNFALSE(__func__,"the section needs an identified Log breakpoint");

        if (templates.empty()) {
            if (!load_templates())
                ERRRETURNFALSE(__func__,"unable to load templates");
        }

        nlohmann::json sectiondata;
        sectiondata["yyyymmdd"] = yyyymmdd;

        if (prevsection.empty()) {
            if (!render_prevsection())
                ERRRETURNFALSE(__func__,"unable to generate prevsection");
        }
        sectiondata["prevsection"] = prevsection;

        if (nextsection.empty()) {
            if (!render_nextsection())
                ERRRETURNFALSE(__func__,"unable to generate nextsection");
        }
        sectiondata["nextsection"] = nextsection;

        if (!have_looked_for_chunks) {
            if (!render_chunks())
                ERRRETURNFALSE(__func__,"unable to generate chunks");
        }
        sectiondata["chunks"] = chunks;

        section = env.render(templates[section_temp],sectiondata);
        return true;
    }

    bool has_prev_section;
    std::string prev_yyyymmdd;

    /**
     * Before calling this function:
     * 1. Determine if the section `has_prev_section` from the list of Breakpoints.
     * 2. Make sure that `prev_yyyymmdd` has been derived from prev Breakpoint (if there is one).
     */
    bool render_prevsection() {
        nlohmann::json prevsectiondata;
        prevsectiondata["prev_yyyymmdd"] = prev_yyyymmdd; //*** find this!
        if (has_prev_section) { // *** find this!
            prevsection = env.render(templates[section_prev_temp],prevsectiondata);
        } else {
            prevsection = env.render(templates[section_noprev_temp],prevsectiondata);
        }
        return true;
    }

    bool has_next_section;
    std::string next_yyyymmdd;

    /**
     * Before calling this function:
     * 1. Determine if the section `has_next_section` from the list of Breakpoints.
     * 2. Make sure that `next_yyyymmdd` has been derived from next Breakpoint (if there is one).
     */
    bool render_nextsection() {
        nlohmann::json nextsectiondata;
        nextsectiondata["next_yyyymmdd"] = next_yyyymmdd; //*** find this!
        if (has_next_section) { // *** find this!
            nextsection = env.render(templates[section_next_temp],nextsectiondata);
        } else {
            nextsection = env.render(templates[section_nonext_temp],nextsectiondata);
        }
        return true;
    }

    std::string chunkbeginyyyymmddhhmm;
    std::string chunkendIyyyymmddhhmm; // wrap in <I></I>
    std::string entry;

    /**
     * Render a Log chunk's references to preceding and following Log chunks.
     */
    std::string render_chunk_ALprev(unsigned long c) {
        if (c==0) {
            return templates[chunk_ALnoprev_temp]; // no need to render
        }

        nlohmann::json ALprevdata;
        if ((c - 1) < chunk_begin_idx) {
            ALprevdata["ALprevsectionyyyymmdd"] = log.get_Breakpoint_Ymd_str(bridx - 1);
        } else {
            ALprevdata["ALprevsectionyyyymmdd"] = yyyymmdd;
        }
        ALprevdata["ALprevchunkstartyyyymmddhhmm"] = log.get_chunk(c - 1)->get_tbegin_str();
        return env.render(templates[chunk_ALprev_temp], ALprevdata);
    }

    std::string render_chunk_ALnext(unsigned long c) {
        if ((c + 1) >= log.num_Chunks()) {
            return templates[chunk_ALnonext_temp]; // no need to render
        }

        nlohmann::json ALnextdata;
        if ((c + 1) >= chunk_end_limit) {
            ALnextdata["ALnextsectionyyyymmdd"] = "task-log."+log.get_Breakpoint_Ymd_str(bridx + 1)+".html#";
        } else {
            ALnextdata["ALnextsectionyyyymmdd"] = "#"; // All but the last in section are relative!
        }
        ALnextdata["ALnextchunkstartyyyymmddhhmm"] = log.get_chunk(c + 1)->get_tbegin_str();
        return env.render(templates[chunk_ALnext_temp], ALnextdata);
    }

    std::string render_chunk_nodeprev(Log_chunk & chunk) {
        if (!chunk.get_node_prev_chunk()) {
            return templates[chunk_nodenoprev_temp]; // no need to render
        }

        nlohmann::json nodeprevdata;
        // find the section of the previous chunk belonging to the same node
        auto node_prev_bridx = log.find_Breakpoint_index_before_chunk(chunk.get_node_prev_chunk()->get_tbegin_key());
        if (node_prev_bridx != bridx) {
            nodeprevdata["nodeprevsectionyyyymmdd"] = log.get_Breakpoint_Ymd_str(node_prev_bridx);
        } else {
            nodeprevdata["nodeprevsectionyyyymmdd"] = yyyymmdd;
        }
        nodeprevdata["nodeprevchunkstartyyyymmddhhmm"] = chunk.get_node_prev_chunk()->get_tbegin_str();
        return env.render(templates[chunk_nodeprev_temp], nodeprevdata);
    }

    std::string render_chunk_nodenext(Log_chunk & chunk) {
        if (!chunk.get_node_next_chunk()) {
            return templates[chunk_nodenonext_temp]; // no need to render
        }

        nlohmann::json nodenextdata;
        // find the section of the next chunk belonging to the same node
        auto node_next_bridx = log.find_Breakpoint_index_before_chunk(chunk.get_node_next_chunk()->get_tbegin_key());
        if (node_next_bridx != bridx) {
            nodenextdata["nodenextsectionyyyymmdd"] = "task-log."+log.get_Breakpoint_Ymd_str(node_next_bridx)+".html#";
        } else {
            nodenextdata["nodenextsectionyyyymmdd"] = "#"; // all that are in the same section!
        }
        nodenextdata["nodenextchunkstartyyyymmddhhmm"] = chunk.get_node_next_chunk()->get_tbegin_str();
        return env.render(templates[chunk_nodenext_temp], nodenextdata);
    }

    /**
     * Render all Log chunks within the section into the `chunks` string variable.
     */
    bool render_chunks() {
        chunks.clear();

        for (unsigned long c = chunk_begin_idx; c < chunk_end_limit; ++c) {

            Log_chunk & chunk = *(log.get_chunk(c));
            nlohmann::json chunkdata;

            chunkdata["chunkbeginyyyymmddhhmm"] = chunk.get_tbegin_str();

            chunkdata["ALprev"] = render_chunk_ALprev(c);

            chunkdata["ALnext"] = render_chunk_ALnext(c);

            if (graph.num_Topics()>0) {
                Topic * maintopic = main_topic(graph,chunk);
                chunkdata["nodetopicid"] = maintopic->get_tag();
                chunkdata["nodetopictitle"] =maintopic->get_title();
            } else {
                chunkdata["nodetopicid"] = "{missing-topic}";
                chunkdata["nodetopictitle"] = "{missing-title}";
            }

            chunkdata["nodeid"] = chunk.get_NodeID().str();

            chunkdata["nodeprev"] = render_chunk_nodeprev(chunk);

            chunkdata["nodenext"] = render_chunk_nodenext(chunk);

            time_t chunkclose = chunk.get_close_time();
            if (chunkclose<0) {
                chunkdata["chunkendIyyyymmddhhmm"] = "";
            } else {
                chunkdata["chunkendIyyyymmddhhmm"] = "<I>"+TimeStampYmdHM(chunkclose)+"</I>";
            }

            // Let's hope for or enforce valid rapid-access `entries` vector.
            entry.clear();
            auto entries = chunk.get_entries();
            for (unsigned long e = 0; e < entries.size(); ++e) {
                // *** BEGIN DUMMY CODE
                entry = "{missing-entries}";
                break;
                // *** END DUMMY CODE
            }
            chunkdata["entries"] = entry;

            chunks += env.render(templates[chunk_temp],chunks);
        }
    }

    bool render_entry() {
        return true;
    }

    bool render_entry_withnode() {
        return true;
    }

};

/**
 * Convert an entire Log into a dil2al backwards compatible set of
 * Task Log files.
 * 
 * The Graph and Log must already be fully in memory.
 * 
 * @param graph the Graph.
 * @param log the fully in-memory Log.
 * @param TLdirectory is the directory path where TL files should be created.
 * @return true if successfully converted.
 */
bool interactive_Log2TL_conversion(Graph & graph, Log & log, std::string TLdirectory) {
    ERRHERE(".top");

    for (Log_chunk_ID_key_deque::size_type bridx = 0; bridx < log.num_Breakpoints(); ++bridx) {
        std::string bridxstr(std::to_string(bridx));
        ERRHERE(".conv."+bridxstr);
        section_Converter sC(graph,log,bridx);
        if (!sC.render_section())
            ERRRETURNFALSE(__func__,"unable to render section ["+bridxstr+"] from Log chunk "+log.get_Breakpoint_first_chunk_id_str(bridx));
        
        ERRHERE(".store."+bridxstr);
        if (!string_to_file(TLdirectory+"/task-log."+log.get_Breakpoint_Ymd_str(bridx)+".html",sC.rendered_section))
            ERRRETURNFALSE(__func__,"unable to write section to file");
    }

    return true;
}
