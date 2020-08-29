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
//#include <nlohmann/json.hpp>
//#include <inja/inja.hpp>

#include "dil2al.hh"

#include "general.hpp"
#include "TimeStamp.hpp"
#include "tl2log.hpp"
#include "templater.hpp"

// special non-header include to keep compile time teamplates out of the way
#include "logtest_templates.cpp"

using namespace fz;

// The Makefile attempts to provide this at compile time based on the source
// file directory location.
#ifndef DEFAULT_TEMPLATE_DIR
    #define DEFAULT_TEMPLATE_DIR "."
#endif
// The Makefile can specify this as desired.
#ifndef DEFAULT_LOGTEST_FILE
    #define DEFAULT_LOGTEST_FILE "/tmp/dil2graph-logtest.html"
#endif

std::string test_template_path(DEFAULT_TEMPLATE_DIR "/test_page.template.html");
std::string testfilepath(DEFAULT_LOGTEST_FILE);

/**
 * Generate simple HTML snippet for the content of a specified Log chunk.
 * 
 * @param env is the inja Environment in use while rendering.
 * @param chunk the specified Log chunk.
 * @return a string containing the HTML snippet.
 */
std::string sample_Chunk_HTML(render_environment & env, Log_chunk & chunk) {
    auto entrypointers = chunk.get_entries();
    VOUT << "Adding sample chunk with " << entrypointers.size() << " entries.\n";

    std::string htmlentries;
    for (auto entryptr_it = entrypointers.begin(); entryptr_it != entrypointers.end(); ++entryptr_it) {
        template_varvalues entrydata;
        entrydata.emplace("entry",(*entryptr_it)->get_entrytext());
        entrydata.emplace("minor_id",(*entryptr_it)->get_id().key().idT.minor_id);
        htmlentries += env.render(testentry,entrydata);
    }

    template_varvalues chunkdata;
    chunkdata.emplace("entries",htmlentries);
    chunkdata.emplace("chunk",chunk.get_tbegin_str());
    chunkdata.emplace("node",chunk.get_NodeID().str());

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
    render_environment & env;

    render_Breakpoint(Log & _log, std::string & _rendered, render_environment & _env): log(_log), rendered(_rendered), env(_env) {}

    void operator()(unsigned long sample_idx) {
        Log_chunk_ID_key & chunk_key = log.get_Breakpoint_first_chunk_id_key(sample_idx);

        std::string ymdstr(log.get_Breakpoint_Ymd_str(sample_idx));
        std::string TLfilename = "task-log."+ymdstr+".html";
        VOUT << "Adding sample Breakpoint " << TLfilename << " entries.\n";

        template_varvalues breakpointdata;
        breakpointdata.emplace("TLfile",TLfilename);

        std::string firstchunk_rendered;
        auto chunk_idx = log.get_Chunks().find(chunk_key);
        if (chunk_idx<log.get_Chunks().size()) {
            Log_chunk * chunk = log.get_Chunks().at(chunk_idx).get();
            firstchunk_rendered += sample_Chunk_HTML(env,*chunk);
        } else {
            firstchunk_rendered += testchunk_notfound;
        }
        breakpointdata.emplace("firstchunk",firstchunk_rendered);

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
    std::map<std::string,std::string> tempinit {
        {"title", "Log data test"},
        {"header","Log data test"},
        {"breakpoints_header","Task Log backward compatibility Log Breakpoints test"},
        {"chunks_header","Log chunks test"}
    };
    template_varvalues testpagedata(tempinit);

    render_environment env;
    VOUT << "Building test page.\n";

    ERRHERE(".chunks");
    if (log.num_Chunks()>=1) {
        Log_chunk * chunk = log.get_Chunks().back().get();
        testpagedata.emplace("chunks",sample_Chunk_HTML(env,*chunk));
    } else {
        testpagedata.emplace("chunks",zerochunks);
    }

    ERRHERE(".breakpoints");
    if (log.num_Breakpoints()>=3) {
        std::string testbreakpointsstr;
        render_Breakpoint rB(log,testbreakpointsstr,env);
        std::set<unsigned long> sampleset = {0, log.num_Breakpoints()/2, log.num_Breakpoints()-1 };
        std::for_each(sampleset.begin(), sampleset.end(), rB);
        testpagedata.emplace("breakpoints",testbreakpointsstr);
    } else {
        testpagedata.emplace("breakpoints",zerobreakpoints);
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
