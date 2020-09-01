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

#include <filesystem>
#include <ostream>

// needs these for the HTML output test samples of converted Log data
//#include <nlohmann/json.hpp>
//#include <inja/inja.hpp>

#include "error.hpp"
#include "general.hpp"
#include "templater.hpp"
#include "Logtypes.hpp"
#include "Graphtypes.hpp"
#include "log2tl.hpp"

using namespace fz;

/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string test_template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string test_template_dir("./templates");
#endif

enum section_status { section_ok, section_out_of_bounds, section_fist_chunk_not_found };

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
    index_temp,
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
    "entry_withnode",
    "lists"};

/// A container in which to cache the template files to be used.
typedef std::map<template_id_enum,std::string> section_templates;

bool load_templates(section_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(test_template_dir + "/TL_" + template_ids[i] + ".template.html", templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

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
 * Note: The `_templates` can be pre-loaded or they will be loaded the
 *       first time they are checked for use.
 * 
 * section_Converter constructor:
 * @param _env a templates rendering environment.
 * @param _templates a set of templates to apply.
 * @param _graph a complete in-memory Graph.
 * @param _log a complete in-memory Log.
 * @param _bridx index to the Breakpoint for which to convert a section. 
 */
struct section_Converter {
protected:
    section_status status;

public:
    render_environment & env;
    section_templates & templates;

    Graph & graph;
    Log & log;
    Log_chunk_ID_key_deque::size_type bridx;

    std::string yyyymmdd;
    Log_chunk_ptr_deque::size_type chunk_begin_idx; // index of first chunk in section
    Log_chunk_ptr_deque::size_type chunk_end_limit; // one past index of last chunk in section
    bool has_prev_section;
    bool has_next_section;
    std::string prev_yyyymmdd;
    std::string next_yyyymmdd;

    std::string rendered_section; /// Holds the result of section rendering process.

    section_Converter() = delete; // explicitly forbid the default constructor, just in case
    section_Converter(render_environment &_env, section_templates &_templates, Graph &_graph, Log &_log,
                      Log_chunks_Deque::size_type _bridx) : status(section_ok), env(_env), templates(_templates),
                      graph(_graph), log(_log), bridx(_bridx) {

        if (bridx >= log.num_Breakpoints()) {
            status = section_out_of_bounds;
            ADDERROR(__func__, "section out of bounds at breakpoint index " + std::to_string(bridx));

        } else {
            yyyymmdd = log.get_Breakpoint_Ymd_str(bridx);
            chunk_begin_idx = log.get_chunk_first_at_Breakpoint(bridx);

            if (chunk_begin_idx>=log.num_Chunks()) {
                status = section_fist_chunk_not_found;
                ADDERROR(__func__,"section first chunk not found "+log.get_Breakpoint_first_chunk_id_str(bridx));

            } else {
                has_prev_section = bridx > 0;
                if (has_prev_section) {
                    prev_yyyymmdd = log.get_Breakpoint_Ymd_str(bridx-1);
                }

                has_next_section = (bridx+1) < log.num_Breakpoints();
                if (has_next_section) {
                    chunk_end_limit = log.get_chunk_first_at_Breakpoint(bridx+1);
                    next_yyyymmdd = log.get_Breakpoint_Ymd_str(bridx+1);

                } else {
                    chunk_end_limit = log.num_Chunks();

                }
            }
        }
    }

    std::string prevsection;
    std::string nextsection;
    bool have_looked_for_chunks = false; // empty chunks is possible
    std::string chunks;

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
            if (!load_templates(templates))
                ERRRETURNFALSE(__func__,"unable to load templates");
        }

        ADDERRPING("#1");
        template_varvalues sectiondata;
        sectiondata.emplace("yyyymmdd",yyyymmdd);

        if (prevsection.empty()) {
            if (!render_prevsection())
                ERRRETURNFALSE(__func__,"unable to generate prevsection");
        }
        ADDERRPING("#2");
        sectiondata.emplace("prevsection",prevsection);

        if (nextsection.empty()) {
            if (!render_nextsection())
                ERRRETURNFALSE(__func__,"unable to generate nextsection");
        }
        ADDERRPING("#3");
        sectiondata.emplace("nextsection",nextsection);

        if (!have_looked_for_chunks) {
            if (!render_chunks())
                ERRRETURNFALSE(__func__,"unable to generate chunks");
        }
        ADDERRPING("#4");
        sectiondata.emplace("chunks",chunks);

        ADDERRPING("#5");
        rendered_section = env.render(templates[section_temp],sectiondata);
        return true;
    }

    /**
     * Before calling this function:
     * 1. Determine if the section `has_prev_section` from the list of Breakpoints.
     * 2. Make sure that `prev_yyyymmdd` has been derived from prev Breakpoint (if there is one).
     */
    bool render_prevsection() {
        template_varvalues prevsectiondata;
        ADDERRPING("#1");
        prevsectiondata.emplace("prev_yyyymmdd",prev_yyyymmdd);
        if (has_prev_section) { // *** find this!
            prevsection = env.render(templates[section_prev_temp],prevsectiondata);
        } else {
            prevsection = env.render(templates[section_noprev_temp],prevsectiondata);
        }
        return true;
    }

    /**
     * Before calling this function:
     * 1. Determine if the section `has_next_section` from the list of Breakpoints.
     * 2. Make sure that `next_yyyymmdd` has been derived from next Breakpoint (if there is one).
     */
    bool render_nextsection() {
        template_varvalues nextsectiondata;
        ADDERRPING("#1");
        nextsectiondata.emplace("next_yyyymmdd",next_yyyymmdd);
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

        ADDERRPING("#1");
        template_varvalues ALprevdata;
        if ((c - 1) < chunk_begin_idx) {
            ALprevdata.emplace("ALprevsectionyyyymmdd",log.get_Breakpoint_Ymd_str(bridx - 1));
        } else {
            ALprevdata.emplace("ALprevsectionyyyymmdd",yyyymmdd);
        }
        Log_chunk *pcptr = log.get_chunk(c - 1);
        if (!pcptr) {
            ADDERROR(__func__,"previous chunk (idx="+std::to_string(c-1)+") returned nullptr");
        } else {
            ALprevdata.emplace("ALprevchunkstartyyyymmddhhmm",pcptr->get_tbegin_str());
        }
        return env.render(templates[chunk_ALprev_temp], ALprevdata);
    }

    std::string render_chunk_ALnext(unsigned long c) {
        if ((c + 1) >= log.num_Chunks()) {
            return templates[chunk_ALnonext_temp]; // no need to render
        }

        ADDERRPING("#1");
        template_varvalues ALnextdata;
        if ((c + 1) >= chunk_end_limit) {
            ALnextdata.emplace("ALnextsectionyyyymmdd","task-log."+log.get_Breakpoint_Ymd_str(bridx + 1)+".html#");
        } else {
            ALnextdata.emplace("ALnextsectionyyyymmdd","#"); // All but the last in section are relative!
        }
        Log_chunk * ncptr = log.get_chunk(c + 1);
        if (!ncptr) { // always be careful
            ADDERROR(__func__,"next chunk (idx="+std::to_string(c+1)+") returned nullptr");
        } else {
            ALnextdata.emplace("ALnextchunkstartyyyymmddhhmm",ncptr->get_tbegin_str());
        }
        return env.render(templates[chunk_ALnext_temp], ALnextdata);
    }

    std::string render_chunk_nodeprev(Log_chunk & chunk) {
        if (chunk.node_prev_isnullptr()) {
            return templates[chunk_nodenoprev_temp]; // no need to render
        }

        ADDERRPING("#1");
        template_varvalues nodeprevdata;
        // find the section of the previous chunk belonging to the same node
        const Log_chain_target & chainprev = chunk.get_node_prev();
        auto node_prev_bridx = log.find_Breakpoint_index_before_chaintarget(chainprev);
        if (node_prev_bridx != bridx) {
            nodeprevdata.emplace("nodeprevsectionyyyymmdd",log.get_Breakpoint_Ymd_str(node_prev_bridx));
        } else {
            nodeprevdata.emplace("nodeprevsectionyyyymmdd",yyyymmdd);
        }
        nodeprevdata.emplace("nodeprevchunkstartyyyymmddhhmm",chainprev.str());
        return env.render(templates[chunk_nodeprev_temp], nodeprevdata);
    }

    std::string render_chunk_nodenext(Log_chunk & chunk) {
        if (chunk.node_next_isnullptr()) {
            return templates[chunk_nodenonext_temp]; // no need to render
        }

        ADDERRPING("#1");
        template_varvalues nodenextdata;
        // find the section of the next chunk belonging to the same node
        const Log_chain_target & chainnext = chunk.get_node_next();
        auto node_next_bridx = log.find_Breakpoint_index_before_chaintarget(chainnext);
        if (node_next_bridx != bridx) {
            nodenextdata.emplace("nodenextsectionyyyymmdd","task-log."+log.get_Breakpoint_Ymd_str(node_next_bridx)+".html#");
        } else {
            nodenextdata.emplace("nodenextsectionyyyymmdd","#"); // all that are in the same section!
        }
        nodenextdata.emplace("nodenextchunkstartyyyymmddhhmm",chainnext.str());
        return env.render(templates[chunk_nodenext_temp], nodenextdata);
    }

    /**
     * Render all Log chunks within the section into the `chunks` string variable.
     */
    bool render_chunks() {
        chunks.clear();

        for (unsigned long c = chunk_begin_idx; c < chunk_end_limit; ++c) {

            ADDERRPING("chunk#"+std::to_string(c));
            Log_chunk * cptr = log.get_chunk(c);
            if (!cptr) { // always be careful around pointers
                ADDERROR(__func__,"chunk (idx="+std::to_string(c)+") was nullptr (should never happen!)");
                continue;
            }

            Log_chunk & chunk = *cptr;
            std::string chunkidstr(chunk.get_tbegin_str());
            ERRHERE(chunkidstr);
            template_varvalues chunkdata;

            chunkdata.emplace("chunkbeginyyyymmddhhmm",chunkidstr);

            chunkdata.emplace("ALprev",render_chunk_ALprev(c));

            chunkdata.emplace("ALnext",render_chunk_ALnext(c));

            if (graph.num_Topics()>0) {
                Topic * maintopic = main_topic(graph,chunk);
                if (maintopic) {
                    chunkdata.emplace("nodetopicid", maintopic->get_tag());
                    chunkdata.emplace("nodetopictitle", maintopic->get_title());
                }
            } else {
                chunkdata.emplace("nodetopicid", "{missing-topic}");
                chunkdata.emplace("nodetopictitle", "{missing-title}");
            }
            ADDERRPING("#1");
            chunkdata.emplace("nodeid",chunk.get_NodeID().str());

            chunkdata.emplace("nodeprev",render_chunk_nodeprev(chunk));

            chunkdata.emplace("nodenext",render_chunk_nodenext(chunk));

            time_t chunkclose = chunk.get_close_time();
            if (chunkclose<0) {
                chunkdata.emplace("chunkendIyyyymmddhhmm","");
            } else {
                chunkdata.emplace("chunkendIyyyymmddhhmm","<I>"+TimeStampYmdHM(chunkclose)+"</I>");
            }

            ADDERRPING("#2");
            // Needs valid rapid-access `entries` vector.
            entry.clear();
            auto entries = chunk.get_entries();
            for (unsigned long e_idx = 0; e_idx < entries.size(); ++e_idx) {

                Log_entry * e = entries[e_idx];
                if (!e) {
                    ADDERROR(__func__,"nullptr encountered at entry "+std::to_string(e_idx)+" of chunk "+chunkidstr);
                    entry += "[<B>"+std::to_string(e_idx+1)+"</B>] {missing-entry}\n\n";
                    continue;
                }

                Node * n = e->get_Node();
                if (n) {
                    render_entry_withnode(*e,*n);
                } else {
                    render_entry(*e);
                }

            }
            chunkdata.emplace("entries",entry);

            chunks += env.render(templates[chunk_temp],chunkdata);
        }
        return true;
    }

    bool render_entry(Log_entry & e) {
        ADDERRPING("#1");
        template_varvalues entrydata;
        entrydata.emplace("entryid",e.get_id_str());
        entrydata.emplace("entryminorid",std::to_string(e.get_minor_id()));
        entrydata.emplace("entrytext",e.get_entrytext());
        entry += env.render(templates[entry_temp],entrydata);
        return true;
    }

    // ??? same templates as for chunks ???
    std::string render_entry_nodeprev(Log_entry & entry) {
        if (entry.node_prev_isnullptr()) {
            return templates[chunk_nodenoprev_temp]; // no need to render
        }

        ADDERRPING("#1");
        template_varvalues nodeprevdata;
        // find the section of the previous chunk or entry belonging to the same node
        const Log_chain_target & chainprev = entry.get_node_prev();
        auto node_prev_bridx = log.find_Breakpoint_index_before_chaintarget(chainprev);
        if (node_prev_bridx != bridx) {
            nodeprevdata.emplace("nodeprevsectionyyyymmdd",log.get_Breakpoint_Ymd_str(node_prev_bridx));
        } else {
            nodeprevdata.emplace("nodeprevsectionyyyymmdd",yyyymmdd);
        }
        nodeprevdata.emplace("nodeprevchunkstartyyyymmddhhmm",chainprev.str());
        return env.render(templates[chunk_nodeprev_temp], nodeprevdata);
    }

    // ??? same templates as for chunks ???
    std::string render_entry_nodenext(Log_entry & entry) {
        if (entry.node_next_isnullptr()) {
            return templates[chunk_nodenonext_temp]; // no need to render
        }

        ADDERRPING("#1");
        template_varvalues nodenextdata;
        // find the section of the next chunk or entry belonging to the same node
        const Log_chain_target & chainnext = entry.get_node_next();
        auto node_next_bridx = log.find_Breakpoint_index_before_chaintarget(chainnext);
        if (node_next_bridx != bridx) {
            nodenextdata.emplace("nodenextsectionyyyymmdd","task-log."+log.get_Breakpoint_Ymd_str(node_next_bridx)+".html#");
        } else {
            nodenextdata.emplace("nodenextsectionyyyymmdd","#"); // all that are in the same section!
        }
        nodenextdata.emplace("nodenextchunkstartyyyymmddhhmm",chainnext.str());
        return env.render(templates[chunk_nodenext_temp], nodenextdata);
    }

    bool render_entry_withnode(Log_entry & e, Node & n) {
        ADDERRPING("#1");
        template_varvalues entrydata;
        entrydata.emplace("entryid",e.get_id_str());
        entrydata.emplace("entryminorid",std::to_string(e.get_minor_id()));
        entrydata.emplace("entrytext",e.get_entrytext());
        ADDERRPING("#4-"+std::to_string((unsigned long) &n));
        entrydata.emplace("nodeid",n.get_id_str());

        ADDERRPING("#6");
        Topic * maintopic = main_topic(graph,n);
        if (maintopic) {
            entrydata.emplace("nodetopicid",maintopic->get_tag());
            entrydata.emplace("nodetopictitle",maintopic->get_title());
        }

        entrydata.emplace("nodeprev",render_entry_nodeprev(e));
        entrydata.emplace("nodenext",render_entry_nodenext(e));
        entry += env.render(templates[entry_withnode_temp],entrydata);
        return true;
    }

};

/**
 * Progress indicator that prints a row of '=' if the counter has reached
 * another 10th of the total.
 * 
 * @param n is the total number that equals 100%.
 * @param ncount is the number reached so far.
 * @return string containing a bar of proportional length (or empty if
 *         another 10th has not yet been reached).
 */
std::string all_sections_progress_bar(unsigned long n, unsigned long ncount) {
    if (ncount==0) {
        return "+----+----+----+---+\n"
               "|    :    :    :   |\n";
    }

    if (ncount >= n) {
        return "====================";
    }

    unsigned long tenth = n / 10;
    if ((n % 10) > 0)
        tenth++;

    if (ncount < tenth)
        return "";

    if ((ncount % tenth) == 0) {
        return std::string(2 * ncount / tenth,'=');
    }

    return "";
}

struct section_IndexBuilder {
public:
    render_environment & env;
    section_templates & templates;
    std::string sectionsdir;

    Log & log;
 
    std::string rendered_index; /// Holds the result of rendering process.

    section_IndexBuilder() = delete; // explicitly forbid the default constructor, just in case
    section_IndexBuilder(render_environment &_env, section_templates &_templates, Log & _log, std::string _sectionsdir) : env(_env), templates(_templates), sectionsdir(_sectionsdir), log(_log) {
    }

    bool render_Index() {

        template_varvalues indexvars;
        std::string indexlinksstr;
        for (Log_chunk_ID_key_deque::size_type bridx = 0; bridx < log.num_Breakpoints(); ++bridx) {
            std::string sectionymd = log.get_Breakpoint_Ymd_str(bridx);
            std::string sectionlinestr = "<LI><A HREF=\""+sectionsdir+"/task-log."+sectionymd+".html\">Task Log: section initiation date mark "+sectionymd+"</A>\n";
            indexlinksstr += sectionlinestr;
        }
        indexvars.emplace("sectionlinks",indexlinksstr);
        rendered_index = env.render(templates[index_temp],indexvars);

        return true;
    }

    bool save_rendered_Index(std::string indexpath) {
        if (indexpath.empty())
            return false;

        if (rendered_index.empty())
            ERRRETURNFALSE(__func__,"empty section index, nothing to save");

        if (!string_to_file(indexpath,rendered_index))
            return false;

        return true;
    }

};

/**
 * Convert an entire Log into a dil2al backwards compatible set of
 * Task Log files.
 * 
 * The Graph and Log must already be fully in memory.
 * 
 * This function attempts to create the `TLdirectory` specified if it does not
 * exist already. A symbolic link `task-log.html` is also created in that
 * directory, for easy access to the most recent of the files generated.
 * 
 * @param graph the Graph.
 * @param log the fully in-memory Log.
 * @param TLdirectory is the directory path where TL files should be created.
 * @param IndexPath is the file path where a TL index should be created (empty to skip).
 * @param o points to an optional output stream for progress report (or nullptr).
 * @return true if successfully converted.
 */
bool interactive_Log2TL_conversion(Graph & graph, Log & log, const Log2TL_conv_params & params) {
    ERRHERE(".top");

    Log_chunk_ID_key_deque::size_type from_idx = 0;
    Log_chunk_ID_key_deque::size_type to_idx = log.num_Breakpoints()-1;
    if (params.to_idx<to_idx)
        to_idx = params.to_idx;
    if (params.from_idx>from_idx)
        from_idx = params.from_idx;
    if (to_idx < from_idx) {
        ERRRETURNFALSE(__func__,"empty setion interval from "+std::to_string(from_idx)+" to "+std::to_string(to_idx));
    }

    std::error_code ec; // we need to check for error codes to distinguish from existing directory
    if (!std::filesystem::create_directories(params.TLdirectory, ec)) {
        if (ec)
            ERRRETURNFALSE(__func__,"unable to create the output directory "+params.TLdirectory+"\nError code ["+std::to_string(ec.value())+"]: "+ec.message());
        
        ADDWARNING(__func__,"using a previously existing directory - any existing files in the directory may be affected");
    }

    render_environment env;
    section_templates templates;
    load_templates(templates);
    std::string latest_TLfile;
    for (Log_chunk_ID_key_deque::size_type bridx = from_idx; bridx <= to_idx; ++bridx) {

        std::string bridxstr(std::to_string(bridx));
        ERRHERE(".conv."+bridxstr);

        section_Converter sC(env,templates,graph,log,bridx);
        if (!sC.render_section()) {
            ADDERROR(__func__,"unable to render section ["+bridxstr+"] from Log chunk "+log.get_Breakpoint_first_chunk_id_str(bridx));
            continue;
        }

        if (sC.rendered_section.empty()) {
            ADDERROR(__func__,"empty rendered section ["+bridxstr+"] from Log chunk "+log.get_Breakpoint_first_chunk_id_str(bridx));
            continue;
        }

        ERRHERE(".store."+bridxstr);
        latest_TLfile = params.TLdirectory+"/task-log."+log.get_Breakpoint_Ymd_str(bridx)+".html";
        if (!string_to_file(latest_TLfile,sC.rendered_section))
            ERRRETURNFALSE(__func__,"unable to write section to file");

        if (params.o) {
            std::string progbar = all_sections_progress_bar(log.num_Breakpoints(), bridx);
            if (!progbar.empty())
                (*params.o) << progbar << '\n';
        }
    }

    ERRHERE(".result");
    if (latest_TLfile.empty()) {
        ERRRETURNFALSE(__func__,"there appear to be no created files to point a task-log.html symbolic link to");

    } else {
        std::filesystem::path TLsymlink(params.TLdirectory+"/task-log.html");
        std::filesystem::remove(TLsymlink);
        std::filesystem::create_symlink(latest_TLfile,TLsymlink,ec);
        if (ec)
            ERRRETURNFALSE(__func__,"Conversion completed, but unable to create symbolic link to task-log.html\nError code ["+std::to_string(ec.value())+"]: "+ec.message());

        if (!params.IndexPath.empty()) {
            ERRHERE(".index");
            std::filesystem::path graph2dildir(params.TLdirectory);
            std::string sectionsdir = graph2dildir.filename(); // get the list part of the path (e.g. lists)
            section_IndexBuilder sIB(env,templates,log,sectionsdir);
            if (!sIB.render_Index()) {
                ERRRETURNFALSE(__func__,"unable to render section index");
            } else {
                if (!sIB.save_rendered_Index(params.IndexPath)) {
                    ERRRETURNFALSE(__func__,"unable to save rendered section index at "+params.IndexPath);
                }
            }
        }
    }

    return true;
}
