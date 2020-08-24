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

#include "dil2al.hh"

#include "general.hpp"
#include "TimeStamp.hpp"
#include "tl2log.hpp"

// special non-header include to keep compile time teamplates out of the way
#include "logtest_templates.cpp"

using namespace fz;

/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string test_template_path(DEFAULT_TEMPLATE_DIR "/test_page.template.html");
#else
    std::string test_template_path("./test_page.template.html");
#endif

std::string testfilepath("/var/www/html/formalizer/dil2graph-testlogdata.html");

/**
 * Generate simple HTML snippet for the content of a specified Log chunk.
 * 
 * @param env is the inja Environment in use while rendering.
 * @param chunk the specified Log chunk.
 * @return a string containing the HTML snippet.
 */
std::string sample_Chunk_HTML(inja::Environment & env, Log_chunk & chunk) {
    auto entrypointers = chunk.get_entries();
    VOUT << "Adding sample chunk with " << entrypointers.size() << " entries.\n";

    std::string htmlentries;
    for (auto entryptr_it = entrypointers.begin(); entryptr_it != entrypointers.end(); ++entryptr_it) {
        nlohmann::json entrydata;
        entrydata["entry"] = (*entryptr_it)->get_entrytext();
        entrydata["minor_id"] = (*entryptr_it)->get_id().key().idT.minor_id;
        htmlentries += env.render(testentry,entrydata);
    }

    nlohmann::json chunkdata;
    chunkdata["entries"] = htmlentries;
    chunkdata["chunk"] = chunk.get_tbegin_str();
    chunkdata["node"] = chunk.get_NodeID().str();

    return env.render(testchunk,chunkdata);
}

/**
 * Add a Breakpoint's data and that of its first Log chunk to a cumulative
 * render string.
 * 
 * This struct's operator() function is called for_each() element of the
 * sampleset in test_Log_data().
 * 
 * render_Breakpoint::render_Breakpoint():
 * @param _log reference to a fully in-memory Log object.
 * @param _rendered reference to a string that is accumulating the render results.
 * @param _env reference to the inja::Environment for rendering.
 * 
 * render_Breakpoint::operator():
 * @param sample_idx the index in deque of the Log breakpoint being sampled.
 */
struct render_Breakpoint {
    Log & log;
    std::string & rendered;
    inja::Environment & env;

    render_Breakpoint(Log & _log, std::string & _rendered, inja::Environment & _env): log(_log), rendered(_rendered), env(_env) {}

    void operator()(unsigned long sample_idx) {
        Log_chunk_ID_key & chunk_key = log.get_Breakpoint_first_chunk_id_key(sample_idx);

        std::string ymdstr(log.get_Breakpoint_Ymd_str(sample_idx));
        std::string TLfilename = "task-log."+ymdstr+".html";
        VOUT << "Adding sample Breakpoint " << TLfilename << " entries.\n";

        nlohmann::json breakpointdata;
        breakpointdata["TLfile"] = TLfilename;

        std::string firstchunk_rendered;
        auto chunk_idx = log.get_Chunks().find(chunk_key);
        if (chunk_idx<log.get_Chunks().size()) {
            Log_chunk * chunk = log.get_Chunks().at(chunk_idx).get();
            firstchunk_rendered += sample_Chunk_HTML(env,*chunk);
        } else {
            firstchunk_rendered += testchunk_notfound;
        }
        breakpointdata["firstchunk"] = firstchunk_rendered;

        rendered += env.render(testbreakpoint,breakpointdata);
    }
};

/**
 * This samples Log data that was converted and generated HTML snippets for
 * inspection.
 * 
 * This is not intended as a full-scale interface for HTML-based access to
 * the Log. For that, see `fzloghtml` and other access tools.
 * 
 * @param log is the Log that was generated, all chunks still in memory.
 * @return true if the test was able to complete.
 */
bool test_Log_data(Log & log) {
    nlohmann::json testpagedata = {
        {"title","Log data test"},
        {"header","Log data test"},
        {"breakpoints_header","Task Log backward compatibility Log Breakpoints test"},
        {"chunks_header","Log chunks test"}
    };

    inja::Environment env;
    VOUT << "Building test page.\n";

    ERRHERE(".chunks");
    if (log.num_Chunks()>=1) {
        Log_chunk * chunk = log.get_Chunks().back().get();
        testpagedata["chunks"] = sample_Chunk_HTML(env,*chunk);
    } else {
        testpagedata["chunks"] = zerochunks;
    }

    ERRHERE(".breakpoints");
    if (log.num_Breakpoints()>=3) {
        std::string testbreakpointsstr;
        render_Breakpoint rB(log,testbreakpointsstr,env);
        std::set<unsigned long> sampleset = {0, log.num_Breakpoints()/2, log.num_Breakpoints()-1 };
        std::for_each(sampleset.begin(), sampleset.end(), rB);
        testpagedata["breakpoints"] = testbreakpointsstr;
    } else {
        testpagedata["breakpoints"] = zerobreakpoints;
    }

    VOUT << "Using test page template at: " << test_template_path << '\n';
    //std::string testhtml = env.render_file(test_template_path,testpagedata);  // *** appears to be broken
    std::ifstream::iostate template_rdstate;
    std::string testhtml = env.render(string_from_file(test_template_path,&template_rdstate),testpagedata);

    if ((template_rdstate & std::ifstream::failbit & std::ifstream::badbit) == 0) {
        VOUT << "Writing test page to " << testfilepath << '\n';
        VOUT << "Hint: Try viewing it http://aether.local/formalizer/dil2graph-testlogdata.html\n";
        std::ofstream testfile(testfilepath, std::ofstream::out);
        testfile << testhtml;
        testfile.close();
    }

    return true;
}
