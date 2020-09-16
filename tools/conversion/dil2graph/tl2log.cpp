// Copyright 2020 Randal A. Koene
// License TBD

/**
 * The tl2log component of dil2graph handles conversion from the dil2al HTML Task Log
 * format to Formalizer Log in a Postgres database.
 * 
 * For more development details, see the Trello card at https://trello.com/c/NNSWVRFf.
 */

// std
#include <ctime>
#include <tuple>
#include <iomanip>

// dil2al compatibility
#include "dil2al.hh"

// core
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "TimeStamp.hpp"
#include "Graphtypes.hpp"
#include "Logtypes.hpp"
#include "Logpostgres.hpp"

// local
#include "tl2log.hpp"
#include "dil2graph.hpp"
#include "logtest.hpp"

#ifdef __DIRECTGRAPH2DIL__
    #include "../graph2dil/log2tl.hpp"
#endif // __DIRECTGRAPH2DIL__

using namespace fz;

const Node_ID Lost_and_Found_node("20200820215834.1");
//const Node_ID Null_node("19000101000000.1");
const Node_ID Null_node("20200820215834.1");

bool manual_decisions = false;
unsigned int auto_eliminate_duration_threshold = 20; // in minutes

unsigned long num_fixes_applied = 0;

//----------------------------------------------------
// Definitions of file-local scope functions:
//----------------------------------------------------

/**
 * Prepare Task_Log for iterative access to its records.
 * 
 * Note that the Task Log is typically perused from its most recent entry
 * (the Task Log head) back to earlier records.
 * 
 * Access starts with the HTML Task Log file at `tasklog` and with the
 * search cursor at its end (chunkseekloc=-1).
 * 
 * @param o an optional message stream for information about the process.
 * @return pointer to Task Log object, or NULL if unsuccessful.
 */
Task_Log * get_Task_Log(ostream * o) {
    Task_Log * tl = new Task_Log();

    if (o) {
        (*o) << "Task Log ready for access at " << tl->chunktasklog << "\n";
        (*o) << "\nNote that the Task Log is scanned iteratively, not loaded into memory in full.\n";
    }

    return tl;    
}

//----------------------------------------------------
// Definitions of functions declared in tl2log.hpp:
//----------------------------------------------------

/**
 * Search for Task Log entries within the text of a Task Log chunk and
 * convert those to Log_entry objects.
 * 
 * There are several possibilities:
 * 1. A fully-specified entry.
 *    - starts with '<!-- entry Begin'
 *    - has entry ID after '<A NAME="'
 *    - has Node ID after '<!-- entry Context --><A HREF="'
 *    - text follows '</FONT>\n<BR>\n'
 *    - ends at the next '<!-- entry Begin' OR at the end of string
 * 2. A chunk-relative entry:
 *    - starts with '<!-- entry Begin'
 *    - has entry ID after '<A NAME="'
 *    - text follows '</A>]</FONT>\n'
 *    - ends at the next '<!-- entry Begin' OR at the end of string
 * 3. Something that is neither 1 nor 2, possibly end of file.
 *    - there is no '<!-- entry Begin', OR,
 *    - there is no '<A NAME="'
 *    - or basically any situation where 1 or 2 don't parse correctly
 * 
 * Optionally, as long as you can find an entry ID, you can always toss
 * whatever else you find into an entry string.
 * 
 * The Log_chunk of which the text will be transformed into a set of
 * Log_entry objects should already be attached in log.
 * 
 * @param log the Log being generated.
 * @param chunktext the HTML text content of the Task Log chunk.
 * @return the number of entries that were extracted and added to the Log.
 */
unsigned int convert_TL_Chunk_to_Log_entries(Log & log, std::string chunktext) {
    std::vector<std::size_t> candidates;

    // find all candidate Log entry start positions
    for (std::size_t entrypos = chunktext.find("<!-- entry Begin",0); entrypos!=std::string::npos; entrypos = chunktext.find("<!-- entry Begin",entrypos+32)) {
        candidates.push_back(entrypos);
    }

    if (candidates.size()<1)
        return 0;

    Log_chunk * chunk = (log.get_Chunks().front()).get();
    if (!chunk)
        return 0;

    for (unsigned int i = 0; i<candidates.size(); ++i) {

        // find Log entry ID
        std::size_t entryidstart = chunktext.find("<A NAME=\"",candidates[i]+16);
        if (entryidstart==std::string::npos) {
            ADDERROR(__func__,"skipping Log entry with malformed A-NAME tag in Log chunk ["+chunk->get_tbegin().str()+']');
            continue;
        }
        entryidstart += 9; // to start of entry ID
        std::size_t entryidend = chunktext.find('"',entryidstart);
        if ((entryidend==std::string::npos) || ((entryidend-entryidstart)<14) || ((entryidend-entryidstart)>20)) {
            ADDERROR(__func__,"skipping Log entry with malformed ID tag in Log chunk ["+chunk->get_tbegin().str()+']');
            continue;
        }
        std::string entryid_str(chunktext.substr(entryidstart,entryidend-entryidstart));

        // find possible specific Node context
        entryidend += 12;
        std::size_t nodecontext = chunktext.find("<!-- entry Context --><A HREF=\"",entryidend);
        std::string nodeid_str;
        std::size_t maxpos = chunktext.size();
        if ((i+1)<candidates.size()) maxpos = candidates[i+1];

        if ((nodecontext!=std::string::npos) && (nodecontext<maxpos)) {
            nodecontext = chunktext.find('#',nodecontext+28);
            if ((nodecontext!=std::string::npos) && (nodecontext<maxpos)) {
                nodecontext++;
                std::size_t nodecontextend = chunktext.find('"',nodecontext);
                if ((nodecontextend==std::string::npos) || ((nodecontextend-nodecontext)<16) || ((nodecontextend-nodecontext)>20)) {
                    ADDERROR(__func__,"malformed Node context ID at Log entry ["+entryid_str+"], treating as chunk-relative");
                } else {
                    nodeid_str = chunktext.substr(nodecontext,nodecontextend-nodecontext);
                    entryidend = nodecontextend+20;
                }
            } else {
                ADDERROR(__func__,"malformed Node context at Log entry ["+entryid_str+"], treating as chunk-relative");
            }
        }

        // find entry content text start
        std::size_t entrytextpos = chunktext.find("</FONT>\n", entryidend);
        if ((entrytextpos != std::string::npos) && (entrytextpos < maxpos)) {
            entrytextpos += 8;
        } else {
            ADDWARNING(__func__, "missing </FONT> tag in Log entry [" + entryid_str + "], entry text may contain front-end rubble");
            entrytextpos = entryidend;
        }
        std::string entrytext(chunktext.substr(entrytextpos, maxpos - entrytextpos));

        // attempt to build a Log_entry_ID object
        try {
            const Log_entry_ID entryid(entryid_str);

            std::unique_ptr<Log_entry> entry;
            if (nodeid_str.empty()) { // make Log_entry object without Node specifier
                entry = std::make_unique<Log_entry>(entryid.key().idT, entrytext, chunk);
                chunk->add_Entry(*entry); // add to chunk.entries
                log.get_Entries().insert({entryid.key(),std::move(entry)}); // entry is now nullptr

            } else {
                // attempt to build a Node_ID object
                try {
                    const Node_ID nodeid(nodeid_str);

                    // make Log_entry object with Node specifier
                    entry = std::make_unique<Log_entry>(entryid.key().idT, entrytext, nodeid.key(), chunk);
                    chunk->add_Entry(*entry);
                    log.get_Entries().insert({entryid.key(),std::move(entry)}); // entry is now nullptr

                } catch (ID_exception idexception) {
                    ADDERROR(__func__, "invalid Node ID (" + nodeid_str + ") at TL entry [" + entryid_str + "], " + idexception.what() + ",\ntreating as chunk-relative");

                    entry = std::make_unique<Log_entry>(entryid.key().idT, entrytext, chunk);
                    chunk->add_Entry(*entry);
                    log.get_Entries().insert({entryid.key(),std::move(entry)}); // entry is now nullptr

                }
            }
        } catch (ID_exception idexception) {
            ADDERROR(__func__, "skipping entry with invalid Log entry ID (" + entryid_str + ") in Log chunk [" + chunk->get_tbegin().str() + "], " + idexception.what());
        }
    }

    return chunk->get_entries().size();
}

char manual_fix_choice(std::string chunkid_str, std::string nodeid_str, const char chunk_content[], int mins_duration) {
    FZOUT("Chunk with invalid Node ID encountered. (interactive mode)\n");
    FZOUT("  Chunk at: "+chunkid_str+" ["+std::to_string(mins_duration)+" minutes]\n");
    FZOUT("  Invalid Node ID string: {"+nodeid_str+"}\n\n");
    FZOUT("  -- Chunk content: --------------------------------------------------------------------\n");
    FZOUT(chunk_content+'\n');
    FZOUT("  --------------------------------------------------------------------------------------\n");
    FZOUT("Options:\n\n");
    FZOUT("  L - connect the chunk to the Lost-and-Found Node [20200820215834.1]\n");
    FZOUT("  S - specify a Node ID manually\n");
    FZOUT("  E - eliminate the chunk\n");
    FZOUT("  X - exit\n\n");
    FZOUT("Your choice: ");
    std::string enterstr;
    std::getline(cin, enterstr);
    char decision = enterstr[0];
    switch (decision) {
        case 'L': case 'l':
            decision = 'l';
            break;

        case 'S': case 's':
            decision = 's';
            break;
        
        case 'E': case 'e':
            decision = 'e';
            break;

        default:
            decision = 'x';
    }
    return decision;
}

/**
 * Print summary information about the conversion through basic metrics support in Logtypes.hh.
 * 
 * @param log the log data structure.
 * @param o an output stream such a cout or a file stream.
 * @param indent an optional string of characters to prepend each line with.
 */
void print_Log_metrics(Log & log, ostream & o, std::string indent) {
    o << indent << "Log breakpoints (TL files): " << log.num_Breakpoints() << "\n\n";
    o << indent << "Log chunks                : " << log.num_Chunks() << '\n';

    if (log.num_Chunks() > 0) {
        o << indent << "\t(" << num_fixes_applied << " chunks fixed, " << to_precision_string(100.0 * (double)num_fixes_applied / (double)log.num_Chunks()) << "%)\n";
        if (log.num_Breakpoints() > 0)
            o << indent << "\t(average chunks per breakpoint: " << to_precision_string((double)log.num_Chunks() / (double) log.num_Breakpoints()) << ")\n";
        o << '\n';
        o << indent << "Log entries               : " << log.num_Entries() << "\n";
        o << indent << "\t(average entries per chunk: " << to_precision_string((double)log.num_Entries() / (double)log.num_Chunks()) << ")\n\n";
        
        std::time_t firstchunk_t = log.oldest_chunk_t();
        std::time_t lastchunk_t = log.newest_chunk_t();
        double span_days = Log_span_in_days(log);
        auto [years, months, days] = Log_span_years_months_days(log);
        o << indent << "Log span in days          : " << span_days << " (" << years << " years, "<< months << " months, " << days << " days)\n";
        o << indent << "\tFrom [" << std::put_time(std::localtime(&firstchunk_t),"%F %T") << "] to [" << std::put_time(std::localtime(&lastchunk_t),"%F %T") << "]\n\n";
    }

    unsigned long total_minutes = Chunks_total_minutes(log.get_Chunks());
    auto [logged_years, logged_months, logged_days] = static_cast<ymd_tuple>(years_months_days(0,total_minutes*60));
    o << indent << "Total time logged         : " << total_minutes << " minutes (" << logged_years << " years, " << logged_months << " months, " << logged_days << " days)\n";
    if (log.num_Chunks() > 0) {
        double av_duration = (double) total_minutes / (double) log.num_Chunks();
        o << indent << "\tAverage Chunk duration : " << to_precision_string(av_duration) << " minutes\n";

        if (log.num_Entries() > 0) {
            unsigned long total_characters = Entries_total_text(log.get_Entries());
            o << '\n' << indent << "Total characters logged: " << total_characters << '\n';
            o << indent << "\t(approx. " << to_precision_string((double)total_characters / 3000.0) << " letter sized written pages, or " << to_precision_string((double)total_characters / (150. * 3000.0)) << " books)\n";
            o << indent << "\t(" << to_precision_string((double)total_characters / log.num_Entries()) << " chars per entry, " << to_precision_string((double)total_characters / log.num_Chunks()) << " chars per chunk)\n";
            o << indent << "\t(" << to_precision_string((double)total_characters / (total_minutes/60)) << " chars per hour, " << to_precision_string((double)total_characters / (total_minutes/(24*60))) << " chars per day)\n";
        }
    }

    o << '\n';
}

const Node_ID convert_TL_DILref_to_Node_ID(TL_entry_content &TLentrycontent, Log & log, std::string chunkid_str, int &nodeid_result) {
    std::string nodeid_str(TLentrycontent.dil.title.chars());
    try {
        const Node_ID nodeid(nodeid_str);

        nodeid_result = 1;
        return nodeid;

    } catch (ID_exception idexception) {

        unsigned int mins_duration = 0;
        if (!(log.get_Chunks().empty())) {
            time_t thischunk = time_stamp_time(chunkid_str);
            time_t laterchunk = log.get_Chunks().front()->get_open_time();
            mins_duration = (laterchunk - thischunk) / 60;
        }

        char decision('x');
        if (manual_decisions) {
            decision = manual_fix_choice(chunkid_str, nodeid_str, TLentrycontent.htmltext.chars(), mins_duration);

        } else {
            FZOUT("Chunk with invalid Node ID encountered. Applying automatic fix.\n");
            std::string chunkcontent(TLentrycontent.htmltext.chars());
            std::size_t entrypos = chunkcontent.find("<!-- entry Begin");
            if (entrypos != std::string::npos) {
                decision = 'l';
            } else {
                if (mins_duration < auto_eliminate_duration_threshold)
                    decision = 'e';
                else
                    decision = 'l';
            }
        }

        if (decision == 'l') {
            ADDWARNING(__func__, "invalid Node ID in Log chunk [" + chunkid_str + "] replaced with Lost-and-Found Node");
            ++num_fixes_applied;
            nodeid_result=1;
            return Lost_and_Found_node;

        } else {
            if (decision == 's') {
                FZOUT("\nAlternative Node ID: ");
                std::string enterstr;
                std::getline(cin, enterstr);
                try {
                    const Node_ID alt_nodeid(enterstr);
                    ++num_fixes_applied;
                    nodeid_result=1;
                    return alt_nodeid;

                } catch (ID_exception idexception) {
                    FZOUT("\nUnforuntately, that one was invalid as well.\n");
                    ADDERROR(__func__, "invalid alternate Node ID (" + enterstr + ") provided by user at TL chunk [" + chunkid_str + "], " + idexception.what());
                    nodeid_result=-1;
                    return Null_node;

                }
            } else {
                if (decision == 'e') {
                    ADDWARNING(__func__, "invalid Node ID in Log chunk [" + chunkid_str + "] eliminated per user instruction");
                    FZOUT("\nEliminating that node.\n");
                    ++num_fixes_applied;
                    nodeid_result=0;
                    return Null_node;

                } else {
                    FZOUT("\nExiting.\n");
                    ADDERROR(__func__, "invalid Node ID (" + nodeid_str + ") at TL chunk [" + chunkid_str + "], " + idexception.what());
                    nodeid_result=-1;
                    return Null_node;

                }
            }
        }
    }
    // never gets here
}

/**
 * Convert a complete Task Log to Log format.
 * 
 * Note A: This function carries out the entire conversion in memory.
 * If the Task Log is too large to hold all its entries in memory during
 * conversion then a different, stream-based or paging implementation of
 * this conversion will need to be used.
 * 
 * Note B: The `Tlentrycontent->dilprev` and `->dilnext` data is not used
 * to set up `node_prev_chunk_id` and `node_next_chunk_id`, because
 * before all chunks are in the Log it is impossible to verify if those
 * point to existing Log chunks. Instead, the `Log::setup_Chunk_nodeprevnext()`
 * function is called at the end of conversion to set up those references.
 * 
 * Note C: This function does not make assumptions about the proper order
 * and completeness of a Log. It simply converts from the Task Log to a
 * v2.x Log. The Log_integrity_test() function can be called on the result.
 * 
 * This function can be run in the absence of a Graph, but note that the
 * rapid-access `node` caches are not set up here for that reason. Run
 * `log->setup_Entry_node_caches(graph)` upon successful return from this
 * function to set up rapid-access.
 * 
 * ***Possible future improvement: By providing a valid Graph to this
 * function it would be possible to test if Nodes referenced by chunks
 * and entries actually exist.
 * 
 * @param tl pointer to a valid Task_Log.
 * @return pointer to Log, or nullptr.
 */
std::unique_ptr<Log> convert_TL_to_Log(Task_Log * tl) {
    ERRHERE(".top");
    if (!tl)
        ERRRETURNNULL(__func__, "unable to build Log from NULL Task_Log");

    // Start an empty Log
    std::unique_ptr<Log> log = make_unique<Log>();
    if (!log)
        ERRRETURNNULL(__func__, "unable to initialize Log");

    ERRHERE(".revparse");
    // Add all the TL_entry_content chunks to the Log while extracting Log_entry objects
    TL_entry_content * tlec; // just a momentary holder
    std::string TLfilename;
    while ((tlec = tl->get_previous_task_log_entry()) != NULL) {

        // This unique_ptr will be automatically deleting chunk objects on each iteration, as
        // well as if the function returns by error.
        std::unique_ptr<TL_entry_content> TLentrycontent(tlec); // captures ownership

        // detect TL file change
        // previously processed chunk, which should now be at the front of the chunks
        // deque becomes breakpoint start for previously parsed TL file
        if (TLentrycontent->source.file != TLfilename.c_str()) {
            // test here in case a lower-bound constraint was placed with the "-1" argument
            if (d2g.proc_from!=0) {
                if (TLfilename == (std::string(basedir.chars())+"/lists/"+std::to_string(d2g.proc_from)+".html")) {
                    // we just passed the last (earliest) section that was within the constraints, we're done here
                    break;
                }
            }
            if (!TLfilename.empty()) {
                if (log->get_Chunks().empty())
                    ERRRETURNNULL(__func__,"unable to assign Log breakpoint without any Log chunks");

                log->get_Breakpoints().add_earlier_Breakpoint(*log->get_Chunks().front());

            }
            TLfilename = TLentrycontent->source.file.chars();
        }

        // convert TL_entry_content into Log_chunk
        const Log_chunk_ID chunkid(TimeStampYmdHM(TLentrycontent->_chunkstarttime)); // Use Log_integrity_test() to gaurantee uniqueness.

        ERRHERE(".revparse."+chunkid.str());
        int nodeid_result = 0; // 0 = skip chunk, 1 = keep chunk, -1 = fail conversion
        const Node_ID nodeid(convert_TL_DILref_to_Node_ID(*TLentrycontent,*log, chunkid.str(), nodeid_result));

        if (nodeid_result<0)
            ERRRETURNNULL(__func__,"undefined Log conversion directive, exiting");

        // Note that node_prev_chunk_id and node_next_chunk_id are assigned at the end of the
        // function by the call to log->setup_Chunk_nodeprevnext().

        if (nodeid_result>0) {
            log->add_earlier_Chunk(chunkid.key().idT,nodeid,TLentrycontent->chunkendtime);
        } else {
            continue; // This skips parsing the chunk for entries as well.
        }

        if (convert_TL_Chunk_to_Log_entries(*log,TLentrycontent->htmltext.chars())<1)
            ADDWARNING(__func__,"no Log entries found in Log chunk ["+chunkid.str()+']');

    }

    // Collect the first-TL-file as breakpoint as well
    ERRHERE(".earliest");
    if (log->get_Chunks().empty())
        ERRRETURNNULL(__func__, "no Log chunks found");

    if (TLfilename.empty())
        ERRRETURNNULL(__func__, "no Task Log files found");

    log->get_Breakpoints().add_earlier_Breakpoint(*log->get_Chunks().front());

    ERRHERE(".chainbynode");
    log->setup_Chain_nodeprevnext();

    return log;
}

typedef std::vector<Log_chunk*> Problem_Chunks;
/// Check for possible logical problems in the Log and propose or apply fixes.
#define EPOCH1999 (time_t)915177600
#define MEGACHUNK (time_t)(7*24*60*60)
void Log_Integrity_Tests(Log & log) {

    // Deal with missing Log chunk close times.
    time_t actualtime = ActualTime();
    unsigned long topen_future = 0;
    Problem_Chunks topenfuture;
    unsigned long tclose_future = 0;
    Problem_Chunks tclosefuture;
    unsigned long topen_problems = 0;
    Problem_Chunks topenproblems;
    unsigned long tclose_problems = 0;
    Problem_Chunks tcloseproblems;
    unsigned long zero_duration = 0;
    Problem_Chunks zeroduration;
    unsigned long mega_chunk = 0;
    Problem_Chunks megachunk;
    Log_chunks_Deque & chunks = log.get_Chunks();
    for (const auto& chunkptr : chunks) {
        time_t t_open = chunkptr->get_open_time();
        time_t t_close = chunkptr->get_close_time();
        if (t_open<EPOCH1999) {
            ++topen_problems;
            topenproblems.push_back(chunkptr.get());
        }
        if (t_close<t_open) {
            ++tclose_problems;
            tcloseproblems.push_back(chunkptr.get());
        } else {
            if (t_close == t_open) {
                ++zero_duration;
                zeroduration.push_back(chunkptr.get());
            } else {
                if ((t_close-t_open)>MEGACHUNK) {
                    ++mega_chunk;
                    megachunk.push_back(chunkptr.get());
                }
            }
        }
        if (t_open > actualtime) {
            ++topen_future;
            topenfuture.push_back(chunkptr.get());
        }
        if (t_close > actualtime) {
            ++tclose_future;
            tclosefuture.push_back(chunkptr.get());
        }
    }
    FZOUT("Log chunks open and close times summary:\n");
    FZOUT("  open times before 1999   : "+std::to_string(topen_problems)+'\n');
    FZOUT("  close time < open time   : "+std::to_string(tclose_problems)+'\n');
    FZOUT("  zero duration chunks     : "+std::to_string(zero_duration)+'\n');
    FZOUT("  unreasonably large chunks: "+std::to_string(mega_chunk)+'\n');
    FZOUT("  open times in the future : "+std::to_string(topen_problems)+'\n');
    FZOUT("  close times in the future: "+std::to_string(topen_problems)+'\n');
    key_pause();
    if (tclose_problems>0) { // Let's have a look at some
        FZOUT("Close time < Open time examples:\n");
        FZOUT("  Chunk ID: "+tcloseproblems[0]->get_tbegin_str()+'\n');
        FZOUT("    open time : "+TimeStampYmdHM(tcloseproblems[0]->get_open_time())+'\n');
        FZOUT("    close time: "+TimeStampYmdHM(tcloseproblems[0]->get_close_time())+'\n');
        FZOUT("  Chunk ID: "+tcloseproblems[1]->get_tbegin_str()+'\n');
        FZOUT("    open time : "+TimeStampYmdHM(tcloseproblems[1]->get_open_time())+'\n');
        FZOUT("    close time: "+TimeStampYmdHM(tcloseproblems[1]->get_close_time())+'\n');
        key_pause();
    }
    // Deal with duplicate Log chunk IDs.

    // Deal with Log chunk IDs out of temporal order.

}

/**
 * This function can be run in the absence of a Graph, but note that the
 * rapid-access `node` caches are not set up here for that reason. Run
 * `log->setup_Entry_node_caches(graph)` upon successful return from this
 * function to set up rapid-access.
 */
std::pair<Task_Log *, std::unique_ptr<Log>> interactive_TL2Log_conversion() {
    ERRHERE(".top");
    key_pause();

    FZOUT("Let's prepare the Task Log for parsing:\n\n");
    ERRHERE(".prep");

    Task_Log * tl;
    if (!(tl = get_Task_Log(base.out))) {
        FZERR("\nSomethihg went wrong! Unable to peruse Task Log.\n");
        standard.exit(exit_DIL_error);
    }

    if (!manual_decisions) {
        FZOUT("\nPlease note that this conversion will attempt to apply fixes where those\n");
        FZOUT("are feasible:\n");
        FZOUT("  a. Log chunks with missing Node references will be attached to a\n");
        FZOUT("     special 'Lost and Found' Node at id [20200820215834.1].\n");
        FZOUT("  b. If such chunks do not contain Log entry tags, and if the chunk\n");
        FZOUT("     duration is less than "+std::to_string(auto_eliminate_duration_threshold)+" then the chunk is eliminated.\n");
        FZOUT('\n');
    }

    key_pause();

    FZOUT("Now, let's convert the Task Log to Log format:\n\n");
    if (base.out) base.out->flush();
    if ((d2g.proc_from!=0) || (d2g.proc_to<99991231)) {
        FZOUT("Please note, section constraints have been applied:\n");
        FZOUT("  from: "+std::to_string(d2g.proc_from)+'\n');
        FZOUT("  to  : "+std::to_string(d2g.proc_to)+'\n');
        if (d2g.proc_to<99991231) {
            tl->chunktasklog = basedir+("/lists/task-log."+std::to_string(d2g.proc_to)+".html").c_str();
        }
    }
    ERRHERE(".convert");
    //ConversionMetrics convmet;
    std::unique_ptr<Log> log(convert_TL_to_Log(tl));
    if (log == nullptr) {
        FZERR("\nSomething went wrong! Unable to convert to Log.\n");
        standard.exit(exit_conversion_error);
    }
    FZOUT("\nTask Log converted to Log with "+std::to_string(log->num_Entries())+" entries.\n\n");

    FZOUT("Carrying out Log integrity tets with proposed corrections.\n\n");
    Log_Integrity_Tests(*log);

    FZOUT("Summary of Log metrics:\n\n");
    print_Log_metrics(*log,*(base.out),"\t");

    key_pause();

    ERRHERE(".test");
    if (!test_Log_data(*log)) {
        FZERR("\nConverted Log data sampling test did not complete.\n");
        standard.exit(exit_conversion_error);
    }

    FZOUT("Finally, let's store the Log as Postgres data.\n\n");
    ERRHERE(".store");
    FZOUT("+----+----+----+---+\n");
    FZOUT("|    :    :    :   |\n");
    if (base.out)
        base.out->flush();
    d2g.ga.access_initialize();
    if (!store_Log_pq(*log, d2g.ga, node_pq_progress_func)) {
        FZERR("\nSomething went wrong! Unable to store (correctly) in Postgres database.\n");
        standard.exit(exit_database_error);
    }
    FZOUT("\nLog stored in Postgres database.\n\n");

    if (SimPQ.SimulatingPQChanges()) {
        FZOUT("Postgres database changes were SIMULATED. Writing Postgres call log to file at:\n/tmp/dil2graph-Postgres-calls.log\n\n");
        std::ofstream pqcallsfile;
        pqcallsfile.open("/tmp/dil2graph-Postgres-calls.log");
        pqcallsfile << SimPQ.GetLog() << '\n';
        pqcallsfile.close();
    }

    key_pause();

    return std::make_pair(tl, std::move(log));
}

void direct_graph2dil_Log2TL_test(Log * logptr, Graph * graphptr) {
#ifdef __DIRECTGRAPH2DIL__
    COMPILEDPING(std::cout,"PING-main.g2dtest\n");
    if (logptr) {
        VOUT << "\nNow, let's try converting the Log right back into Tak Log files.\n\n";
        key_pause();

        ERRHERE(".graph2dil");
        if (!graphptr) {
            ERRHERE(".loadGraph");
            graphptr = d2g.ga.request_Graph_copy().get();
            if (!graphptr) {
                ADDERROR(__func__,"unable to load Graph");
                standard.exit(exit_database_error);
            }
            COMPILEDPING(std::cout,"PING-main.g2d-Logcaches\n");
            if ((logptr != nullptr) && (graphptr != nullptr)) {
                logptr->setup_Entry_node_caches(*graphptr); // here we can do this!
                logptr->setup_Chunk_node_caches(*graphptr);
            }
        }

        COMPILEDPING(std::cout,"PING-main.g2d-Log2TL\n");
        Log2TL_conv_params params;
        params.TLdirectory = DIRECTGRAPH2DIL_DIR;
        params.IndexPath = DIRECTGRAPH2DIL_DIR "/../graph2dil-lists.html";
        params.o = &VOUT;
        params.from_idx = d2g.from_section;
        params.to_idx = d2g.to_section;
        if (!interactive_Log2TL_conversion(*graphptr, *logptr, params)) {
            EOUT << "\nDirect conversion test back to Task Log files did not complete..\n";
            standard.exit(exit_general_error);
        }
        VOUT << "\nDirect conversion test back to Task Log files written to " << DIRECTGRAPH2DIL_DIR << '\n';
        VOUT << "Hint: Try viewing it http://aether.local/formalizer/graph2dil/task-log.html\n\n";
    }
#else
    FZOUT("Note: Direct graph2dil test not included. Please use the graph2dil program.\n");
#endif // __DIRECTGRAPH2DIL__
}

void interactive_TL2Log_validation(Task_Log * tl, Log * log, Graph * graph) {
    ERRHERE(".top");
    if ((!tl) || (!log)) {
        FZERR("Unable to validate due to tl==NULL or log==NULL\n");
        standard.exit(exit_general_error);
    }
    FZOUT("Now, let's validate the database by reloading the Log and comparing it with the one that was stored.\n");

    ERRHERE(".reloadLog");
    Log reloaded;
    if (!load_Log_pq(reloaded, d2g.ga)) { // Not using d2g.ga.request_Log_copy(), because there might be no running server when we just created the database.
        FZERR("\nSomething went wrong! Unable to load back from Postgres database.\n");
        standard.exit(exit_database_error);
    }
    FZOUT("Log re-loading data test:\n");
    FZOUT("  Number of Entries      = "+std::to_string(reloaded.num_Entries())+'\n');
    FZOUT("  Number of Chunks       = "+std::to_string(reloaded.num_Chunks())+'\n');
    FZOUT("  Number of Breakpoints  = "+std::to_string(reloaded.num_Breakpoints())+"\n\n");
    if (graph != nullptr) {
        reloaded.setup_Entry_node_caches(*graph); // here we can do this!
        reloaded.setup_Chunk_node_caches(*graph);
    }

    key_pause();

    if (d2g.TL_reconstruction_test) {
        direct_graph2dil_Log2TL_test(&reloaded,graph);
    }
}
