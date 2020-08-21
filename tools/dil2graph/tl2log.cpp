// Copyright 2020 Randal A. Koene
// License TBD

/**
 * The tl2log component of dil2graph handles conversion from the dil2al HTML Task Log
 * format to Formalizer Log in a Postgres database.
 * 
 * For more development details, see the Trello card at https://trello.com/c/NNSWVRFf.
 */

// dil2al compatibility
#include "dil2al.hh"

// Formalizer core
#include "error.hpp"
#include "general.hpp"
#include "Graphtypes.hpp"
#include "Logtypes.hpp"

// Tool specific
#include "tl2log.hpp"
#include "dil2graph.hpp"

using namespace fz;

const Node_ID Lost_and_Found_node("20200820215834.1");

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
 * @return pointer to Task Log object, or NULL if unsuccessful.
 */
Task_Log * get_Task_Log() {
    Task_Log * tl = new Task_Log();

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

        try {
            const Log_entry_ID entryid(entryid_str);

            std::unique_ptr<Log_entry> entry;
            if (nodeid_str.empty()) {
                entry = std::make_unique<Log_entry>(entryid.key().idT, entrytext, chunk);
                chunk->add_Entry(*entry);
                log.get_Entries().insert({entryid.key(),std::move(entry)}); // entry is now nullptr

            } else {
                try {
                    const Node_ID nodeid(nodeid_str);

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

/**
 * Convert a complete Task Log to Log format.
 * 
 * Note that this function carries out the entire conversion in memory.
 * If the Task Log is too large to hold all its entries in memory during
 * conversion then a different, stream-based or paging implementation of
 * this conversion will need to be used.
 * 
 * ***Possible future improvement: By providing a valid Graph to this
 * function it would be possible to test if Nodes referenced by chunks
 * and entries actually exist.
 * 
 * @param tl pointer to a valid Task_Log.
 * @return pointer to Log, or nullptr.
 */
std::unique_ptr<Log> convert_TL_to_Log(Task_Log * tl) {
    ERRHERE(".1");
    if (!tl)
        ERRRETURNNULL(__func__, "unable to build Log from NULL Task_Log");

    // Start an empty Log
    std::unique_ptr<Log> log = make_unique<Log>();
    if (!log)
        ERRRETURNNULL(__func__, "unable to initialize Log");

    ERRHERE(".2");
    // Add all the TL_entry_content chunks to the Log while extracting Log_entry objects
    TL_entry_content * tlec; // just a momentary holder
    std::string TLfilename; 
    while ((tlec = tl->get_previous_task_log_entry()) != NULL) {

        // This unique_ptr should be automatically deleting chunk objects on each iteration, as
        // well as if the function returns by error.
        std::unique_ptr<TL_entry_content> TLentrycontent(std::make_unique<TL_entry_content>(*tlec));

        // detect TL file change
        // previously processed chunk, which should now be at the front of the chunks
        // deque becomes breakpoint start for previously parsed TL file
        if (TLentrycontent->source.file != TLfilename.c_str()) {
            if (!TLfilename.empty()) {
                if (log->get_Chunks().empty())
                    ERRRETURNNULL(__func__,"unable to assign Log breakpoint without any Log chunks");

                log->get_Breakpoints().add_earlier_Breakpoint(*log->get_Chunks().front());
            }
            TLfilename = TLentrycontent->source.file.chars();
        }

        // convert TL_entry_content into Log_chunk
        const Log_chunk_ID chunkid(TimeStampYmdHM(TLentrycontent->_chunkstarttime));
        try {
            const Node_ID nodeid(TLentrycontent->dil.title.chars());
            log->add_earlier_Chunk(chunkid.key().idT,nodeid);

        } catch (ID_exception idexception) {
            VOUT << "Chunk with invalid Node ID encountered. (interactive mode)\n";
            VOUT << "  Chunk at: " << chunkid.str() << '\n';
            VOUT << "  Invalid Node ID string: {" << std::string(TLentrycontent->dil.title.chars()) << "}\n\n";
            VOUT << "Options:\n\n";
            VOUT << "  L - connect the chunk to the Lost-and-Found Node [20200820215834.1]\n";
            VOUT << "  S - specify a Node ID manually\n";
            VOUT << "  X - exit\n\n";
            VOUT << "Your choice: ";
            std::string enterstr;
            std::getline(cin, enterstr);
            if ((enterstr[0]=='L') || (enterstr[0]=='l')) {
                ADDWARNING(__func__,"invalid Node ID in Log chunk ["+chunkid.str()+"] replaced with Lost-and-Found Node");
                log->add_earlier_Chunk(chunkid.key().idT,Lost_and_Found_node);

            } else {
                if ((enterstr[0]=='S') || (enterstr[0]=='s')) {
                    VOUT << "\nAlternative Node ID: ";
                    std::getline(cin,enterstr);
                    try {
                        const Node_ID alt_nodeid(enterstr);
                        log->add_earlier_Chunk(chunkid.key().idT,alt_nodeid);

                    } catch (ID_exception idexception) {
                        VOUT << "\nUnforuntately, that one was invalid as well.\n";
                        ERRRETURNNULL(__func__,"invalid alternate Node ID ("+enterstr+") provided by user at TL chunk ["+chunkid.str()+"], "+idexception.what());

                    }
                } else {
                    VOUT << "\nExiting.\n";
                    ERRRETURNNULL(__func__,"invalid Node ID ("+std::string(TLentrycontent->dil.title.chars())+") at TL chunk ["+chunkid.str()+"], "+idexception.what());
                }
            }

        }
        
        if (convert_TL_Chunk_to_Log_entries(*log,TLentrycontent->htmltext.chars())<1)
            ADDWARNING(__func__,"no Log entries found in Log chunk ["+chunkid.str()+']');

    }

    // Collect the first-TL-file as breakpoint as well
    if (log->get_Chunks().empty())
        ERRRETURNNULL(__func__, "no Log chunks found");

    if (TLfilename.empty())
        ERRRETURNNULL(__func__, "no Task Log files found");

    log->get_Breakpoints().add_earlier_Breakpoint(*log->get_Chunks().front());

    return log;
}

std::pair<Task_Log *, std::unique_ptr<Log>> interactive_TL2Log_conversion() {
    ERRHERE(".1");
    key_pause();

    VOUT << "Let's prepare the Task Log for parsing:\n\n";
    ERRHERE(".2");

    Task_Log * tl;
    if (!(tl = get_Task_Log())) {
        EOUT << "\nSomethihg went wrong! Unable to peruse Task Log.\n";
        Exit_Now(exit_DIL_error);
    }

    key_pause();

    VOUT << "Please note that this conversion will attempt to apply fixes where those\n";
    VOUT << "are feasible:\n";
    VOUT << "  a. Log chunks with missing Node references will be attached to a\n";
    VOUT << "     special 'Lost and Found' Node at id [20200820215834.1].\n";
    VOUT << '\n';
    VOUT << "Now, let's convert the Task Log to Log format:\n\n";
    ERRHERE(".3");
    //ConversionMetrics convmet;
    std::unique_ptr<Log> log(convert_TL_to_Log(tl));
    if (log == nullptr) {
        EOUT << "\nSomething went wrong! Unable to convert to Log.\n";
        Exit_Now(exit_conversion_error);
    }
    VOUT << "\nTask Log converted to Log with " << log->num_Entries() << " entries.\n\n";

    key_pause();

    return std::make_pair(tl, std::move(log));
}
