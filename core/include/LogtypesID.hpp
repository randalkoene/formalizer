// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares Log ID key and related types for use with the Formalizer.
 * These define the authoritative version of the Log data structure for use in C++ code.
 * 
 * This is a logical subsection of Logtypes.hpp.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __LOGTYPESID_HPP.
 */

/**
 * For more information see:
 * - The development Trello card at https://trello.com/c/NNSWVRFf.
 */

#ifndef __LOGTYPESID_HPP
#include "coreversion.hpp"
#define __LOGTYPESID_HPP (__COREVERSION_HPP)

#include <memory>
#include <map>
#include <ctime>

#include "error.hpp"
#include "TimeStamp.hpp"
#include "Graphtypes.hpp"
// Don't put "Logtypes.hpp" here! In the header tree, this comes before that.

namespace fz {

#define LOG_NULLKEY_STR "{null-key}"

// forward declarations to classes from Logtypes.hpp
class Log;
class Log_data;
class Log_entry;
class Log_chunk;

// forward declarations to classes in here
class Log_TimeStamp;

std::string Log_entry_ID_TimeStamp_to_string(const Log_TimeStamp idT);

std::string Log_chunk_ID_TimeStamp_to_string(const Log_TimeStamp idT);

std::string Log_TimeStamp_to_Ymd_string(const Log_TimeStamp idT);

bool valid_Log_entry_ID(const Log_TimeStamp &idT, std::string &formerror);

bool valid_Log_chunk_ID(const Log_TimeStamp &idT, std::string &formerror);

bool valid_Log_entry_ID(std::string id_str, std::string &formerror, Log_TimeStamp *id_timestamp = NULL);

bool valid_Log_chunk_ID(std::string id_str, std::string &formerror, Log_TimeStamp *id_timestamp = NULL);

/**
 * Timestamp IDs in the format required for Log IDs.
 * These include all time components from year to minute, as well as an additional
 * minor_id (this is NOT a decimal, since .10 is higher than .9).
 * The minor_id is used only in Log Entry IDs, not in Log Chunk IDs.
 * Note that the time formatting is not the same as in the C time structure 'tm'.
 * Days and months count from 1. The year is given as is, not relative to the
 * UNIX epoch. (By contrast, tm time subtracts 1900 years.)
 * 
 * These structures are mainly used as unique IDs, but conversion to UNIX time
 * is provided through member functions.
 * Time conversion to UNIX time is carried out by the get_epoch_time() function.
 * 
 * Note: A non-standard Log time stamp can be created and used. The quick
 *       isnullstamp() test can detect the special case where non-standard
 *       values are used to create a null-stamp, so that the get_local_time()
 *       and get_epoch_time() functions return well defined results for those.
 *       For greater assurance, the Log_entry_ID_key and Log_chunk_ID_key
 *       classes call specific thorough `valid_...` test functions.
 */
struct Log_TimeStamp {
    uint8_t minor_id;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    int16_t year;

    /// Initializes as LOG_NULL_IDSTAMP.
    Log_TimeStamp(): minor_id(0), minute(0), hour(0), day(0), month(0), year(0) {}
    Log_TimeStamp(std::time_t t, bool testvalid = false, uint8_t _minorid = 0);

    /// standardization functions and operators
    bool isnullstamp() const { return (month == 0) || (year<1900); }
    bool operator< (const Log_TimeStamp& rhs) const {
        return std::tie(year,month,day,hour,minute,minor_id)
             < std::tie(rhs.year,rhs.month,rhs.day,rhs.hour,rhs.minute,rhs.minor_id);
    }
    bool operator== (const Log_TimeStamp& rhs) const {
        return std::tie(year,month,day,hour,minute,minor_id)
             == std::tie(rhs.year,rhs.month,rhs.day,rhs.hour,rhs.minute,rhs.minor_id);
    }
    std::tm get_local_time() const;
    time_t get_epoch_time() const;
};

/**
 * Standardized Formalizer Log entry ID key.
 * 
 * The principal constructors (all but the default constructor) each
 * call a validity test for the key format and can throw an
 * ID_exception.
 * 
 * In various containers, the ordering of Log entry ID keys is determined
 * by the provided operator<(). It calls its equivalent in Log_TimeStamp,
 * where std::tie() enables lexicographical comparison, from the largest
 * temporal component to the smallest.
 * 
 * The default constructor is provided for various special use cases such
 * as initialization of containers. The `isnullkey()` function can test
 * for this special state.
 */
struct Log_entry_ID_key {
    Log_TimeStamp idT;

    Log_entry_ID_key(): idT() {} /// Try to use this one only for container element initialization and such.

    Log_entry_ID_key(const Log_TimeStamp& _idT);
    Log_entry_ID_key(std::time_t t, uint8_t _minorid = 1): idT(t,true,_minorid) {}
    Log_entry_ID_key(std::string _idS);

    bool isnullkey() const { return idT.month == 0; }

    std::string str() const { return (isnullkey() ? LOG_NULLKEY_STR : Log_entry_ID_TimeStamp_to_string(idT)); }

    bool operator< (const Log_entry_ID_key& rhs) const { return (idT < rhs.idT); }
    bool operator== (const Log_entry_ID_key& rhs) const { return (idT == rhs.idT); }

};

/**
 * Standardized Formalizer Log chunk ID key.
 * 
 * The principal constructors (all but the default constructor) each
 * call a validity test for the key format and can throw an
 * ID_exception.
 * 
 * In various containers, the ordering of Log chunk ID keys is determined
 * by the provided operator<(). It calls its equivalent in Log_TimeStamp,
 * where std::tie() enables lexicographical comparison, from the largest
 * temporal component to the smallest.
 * 
 * The default constructor is provided for various special use cases such
 * as initialization of containers. The `isnullkey()` function can test
 * for this special state.
 */
struct Log_chunk_ID_key {
    Log_TimeStamp idT;

    Log_chunk_ID_key(): idT() {} /// Try to use this one only for container element initialization and such.

    Log_chunk_ID_key(const Log_TimeStamp& _idT);
    Log_chunk_ID_key(std::time_t t): idT(t,true) {}
    Log_chunk_ID_key(std::string _idS);

    bool isnullkey() const { return idT.month == 0; }

    std::time_t get_epoch_time() const { return idT.get_epoch_time(); }
    std::string str() const { return (isnullkey() ? LOG_NULLKEY_STR : Log_chunk_ID_TimeStamp_to_string(idT)); }

    bool operator< (const Log_chunk_ID_key& rhs) const { return (idT < rhs.idT); }
    bool operator== (const Log_chunk_ID_key& rhs) const { return (idT == rhs.idT); }
};

/**
 * Log objects are principally identified by their ID key, but when used
 * as a linked target, e.g. by-Node chaining, rapid-access pointers are
 * maintained when possible for faster traveral. This Log_target template
 * formalizes that concept.
 * 
 * *** Note: In a future update, this template can be the basis for added
 *           pointer safety with regards to rapid-access pointers. See the
 *           description on the Trello card at https://trello.com/c/CZ8XIw4j.
 */
template <class Log_ID_key, class Log_component>
struct Log_target {
    Log_ID_key key;
    Log_component *ptr;

    Log_target() : ptr(nullptr) {}
    Log_target(const Log_ID_key &k, const Log_component *p = nullptr) : key(k), ptr(p) {}
    Log_target(const Log_TimeStamp &t_stamp, const Log_component *p = nullptr) : key(t_stamp), ptr(p) {}
};

typedef Log_target<Log_chunk_ID_key, Log_chunk> Log_chunk_target;
typedef Log_target<Log_entry_ID_key, Log_entry> Log_entry_target;

typedef std::pair<bool, Log_data *> Log_ptr_pair;
typedef std::tuple<bool, Log_chunk_ID_key, Log_entry_ID_key> Log_key_tuple;
typedef std::tuple<bool, Log_chunk_target, Log_entry_target> Log_target_tuple;

/**
 * This data structure is used to follow a chain by-Node through the Log that
 * can lead to Log chunks or Log entries.
 * 
 * The two possible IDs and two possible pointers are in unions to share the
 * same memory space.
 * 
 * @param ischunk flag is true if the target is a Log chunk, false if Log entry.
 * @param chunk identifies and points to a Log chunk (or is nullkey & nullptr).
 * @param entry identifies and points to a Log entry (or is nullkey & nullptr).
 */
struct Log_chain_target {
    bool ischunk;
    union {
        Log_chunk_target chunk;
        Log_entry_target entry;
    };

    Log_chain_target(): ischunk(true) {}
    Log_chain_target(const Log_chunk_ID_key & chunkkey, const Log_chunk * cptr = nullptr): ischunk(true), chunk(chunkkey,cptr) {}
    Log_chain_target(const Log_chunk & _chunk);
    Log_chain_target(const Log_entry_ID_key & entrykey, const Log_entry * eptr = nullptr);
    Log_chain_target(const Log_entry & _entry);
    Log_chain_target(const Log_TimeStamp & t_stamp, bool _ischunk, Log_data * dptr = nullptr);
    Log_chain_target(const Log_chunk_ID_key & chunkkey, const Log_chunk * cptr): ischunk(true), chunk(chunkkey,cptr) {}
    Log_chain_target(const Log_chunk & _chunk): ischunk(true), chunk(_chunk.get_tbegin_key(),&_chunk) {}
    Log_chain_target(const Log_entry_ID_key & entrykey, const Log_entry * eptr): ischunk(false), entry(entrykey,eptr) {}
    Log_chain_target(const Log_entry & _entry): ischunk(false), entry(_entry.get_id_key(),&_entry) {}
    Log_chain_target(const Log_TimeStamp & t_stamp, bool _ischunk, Log_data * dptr) { set_any_target(t_stamp,_ischunk, dptr); }

    void set_to_nulltarget() { ischunk = true; chunk.key = Log_chunk_ID_key(); chunk.ptr = nullptr; }
    void set_chunk_target(const Log_chunk_target _chunk) { ischunk = true; chunk = _chunk; }
    void set_chunk_target(const Log_chunk & _chunk) { ischunk = true; chunk.key = _chunk.get_tbegin_key(); chunk.ptr = const_cast<Log_chunk *>(&_chunk); }
    void set_chunk_target(const Log_chunk_ID_key & chunkkey, Log_chunk * cptr = nullptr) { ischunk = true; chunk.key = chunkkey; chunk.ptr = cptr; }
    void set_chunk_target(const Log_TimeStamp & t_stamp, Log_chunk * cptr = nullptr) { ischunk = true; chunk.key = Log_chunk_ID_key(t_stamp); chunk.ptr = cptr; }
    void set_entry_target(const Log_entry_target _entry) { ischunk = false; entry = _entry; }
    void set_entry_target(const Log_entry & _entry) { ischunk = false; entry.key = _entry.get_id_key(); entry.ptr = const_cast<Log_entry *>(&_entry); }
    void set_entry_target(const Log_entry_ID_key & entrykey, Log_entry * cptr = nullptr) { ischunk = false; entry.key = entrykey; entry.ptr = cptr; }
    void set_entry_target(const Log_TimeStamp & t_stamp, Log_entry * cptr = nullptr) { ischunk = false; entry.key = Log_entry_ID_key(t_stamp); entry.ptr = cptr; }
    void set_any_target(const Log_TimeStamp & t_stamp, bool _ischunk, Log_data * dptr = nullptr);

    Log_chunk_target get_chunk_target();
    Log_entry_target get_entry_target();
    Log_chunk_ID_key get_chunk_key();
    Log_entry_ID_key get_entry_key();
    Log_data * get_ptr(); /// Definitely check `ischunk` when using `get_ptr()`.
    Log_ptr_pair get_data_ptr();
    Log_key_tuple get_any_ID_key();
    Log_target_tuple get_any_target();

    bool isnulltarget_byID();
    bool isnulltarget_byptr() { return get_ptr() == nullptr; }

    bool same_target(Log_chain_target & target);
    bool same_target(Log_chunk & chunkref);
    bool same_target(Log_entry & entryref);

    // helper function dealing with next/prev-in-chain, instead of this object
    Log_chain_target * go_next_in_chain();
    Log_chain_target * go_prev_in_chain();
    void bytargetptr_set_Node_next_ptr(Log_chunk * _next);
    void bytargetptr_set_Node_next_ptr(Log_entry * _next);
    void bytargetptr_set_Node_prev_ptr(Log_chunk * _prev);
    void bytargetptr_set_Node_prev_ptr(Log_entry * _prev);
};

class Log_entry_ID {
    friend class Log_entry;
protected:
    Log_entry_ID_key idkey;
    std::string idS_cache; // cached string version of the ID to speed things up

    Log_entry_ID() {} // to set up a null-ID in very specific cases
public:
    Log_entry_ID(std::string _idS): idkey(_idS), idS_cache(_idS) {}
    Log_entry_ID(const Log_TimeStamp _idT);
    //Log_entry_ID() = delete; // explicitly forbid the default constructor, just in case

    bool isnullID() const { return idkey.isnullkey(); }
    Log_entry_ID_key key() const { return idkey; }
    std::string str() const { return (isnullID() ? idkey.str() : idS_cache); }
    time_t get_epoch_time() const { return idkey.idT.get_epoch_time(); }
};

class Log_chunk_ID {
    friend class Log_chunk;
protected:
    Log_chunk_ID_key idkey;
    std::string idS_cache; // cached string version of the ID to speed things up

    Log_chunk_ID() {} // to set up a null-ID in very specific cases
    // the following are only meant for careful administrative use, for example in
    // Log chunk chaining in Log::setup_Chunk_nodeprevnext().
    bool _reassign(std::string _idS);
    bool _reassign(const Log_TimeStamp _idT);
    void _nullID();
public:
    Log_chunk_ID(std::string _idS): idkey(_idS), idS_cache(_idS) {}
    Log_chunk_ID(const Log_TimeStamp _idT);
    //Log_chunk_ID() = delete; // explicitly forbid the default constructor, just in case

    bool isnullID() const { return idkey.isnullkey(); }
    Log_chunk_ID_key key() const { return idkey; }
    std::string str() const { return (isnullID() ? idkey.str() : idS_cache); }
    time_t get_epoch_time() const { return idkey.idT.get_epoch_time(); }
};

// +----- begin: inline functions -----+

inline void Log_chain_target::set_any_target(const Log_TimeStamp & t_stamp, bool _ischunk, Log_data * dptr) {
    ischunk = _ischunk;
    if (ischunk) {
        chunk = Log_chunk_target(t_stamp, (Log_chunk *)dptr);
    } else {
        entry = Log_entry_target(t_stamp, (Log_entry *)dptr);
    }
}

inline Log_chunk_target Log_chain_target::get_chunk_target() {
    if (ischunk)
        return chunk;

    ADDERROR(__func__, "chunk target requested from entry target (returning null-target)");
    return Log_chunk_target();
}

inline Log_entry_target Log_chain_target::get_entry_target() {
    if (!ischunk)
        return entry;
    
    ADDERROR(__func__, "entry target requested from chunk target (returning null-target)");
    return Log_entry_target();
}

inline Log_chunk_ID_key Log_chain_target::get_chunk_key() {
    if (ischunk)
        return chunk.key;

    ADDERROR(__func__, "chunk key requested from entry target (returning null-key)");
    return Log_chunk_ID_key();
}

inline Log_entry_ID_key Log_chain_target::get_entry_key() {
    if (!ischunk)
        return entry.key;

    ADDERROR(__func__, "entry key requested from chunk target (returning null-key)");
    return Log_entry_ID_key();
}

/// Definitely check `ischunk` when using `get_ptr()`.
inline Log_data * Log_chain_target::get_ptr() {
    return (ischunk) ? (Log_data *)chunk.ptr : (Log_data *)entry.ptr;
}

inline Log_ptr_pair Log_chain_target::get_data_ptr() {
    return std::make_pair(ischunk, get_ptr());
}

inline Log_key_tuple Log_chain_target::get_any_ID_key() {
    if (ischunk) {
        return std::make_tuple(ischunk, chunk.key, Log_entry_ID_key());
    } else {
        return std::make_tuple(ischunk, Log_chunk_ID_key(), entry.key);
    }
}

inline Log_target_tuple Log_chain_target::get_any_target() {
    if (ischunk) {
        return std::make_tuple(ischunk, chunk, Log_entry_target());
    } else {
        return std::make_tuple(ischunk, Log_chunk_target(), entry);
    }
}

inline bool Log_chain_target::isnulltarget_byID() {
    if (ischunk) {
        return chunk.key.isnullkey();
    } else {
        return entry.key.isnullkey();
    }
}

inline void Log_chain_target::bytargetptr_set_Node_next_ptr(Log_chunk * _next) {
    if (ischunk) {
        chunk.ptr->set_Node_next_ptr(_next);
    } else {
        entry.ptr->set_Node_next_ptr(_next);
    }
}

inline void Log_chain_target::bytargetptr_set_Node_next_ptr(Log_entry * _next) {
    if (ischunk) {
        chunk.ptr->set_Node_next_ptr(_next);
    } else {
        entry.ptr->set_Node_next_ptr(_next);
    }
}

inline void Log_chain_target::bytargetptr_set_Node_prev_ptr(Log_chunk * _prev) {
    if (ischunk) {
        chunk.ptr->set_Node_prev_ptr(_prev);
    } else {
        entry.ptr->set_Node_prev_ptr(_prev);
    }
}

inline void Log_chain_target::bytargetptr_set_Node_prev_ptr(Log_entry * _prev) {
    if (ischunk) {
        chunk.ptr->set_Node_prev_ptr(_prev);
    } else {
        entry.ptr->set_Node_prev_ptr(_prev);
    }
}

// +----- end  : inline functions -----+

} // namespace fz

#endif // __LOGTYPESID_HPP