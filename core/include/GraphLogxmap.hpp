// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Useful mapping classes and functions for data determined via cross-association
 * between Graph and Log.
 * 
 * For example, this containing mapping structures used by:
 * - fzlogmap
 * - fzupdate (soon, `EPS_map` is still in local files there)
 * - Graphinfo where projection onto time is needed
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __GRAPHLOGXMAP_HPP.
 */

#ifndef __GRAPHLOGXMAP_HPP
#include "coreversion.hpp"
#define __GRAPHLOGXMAP_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
#include <vector>

// core
//#include "error.hpp"


namespace fz {

/**
 * Comparing map types
 * -------------------
 * 
 * ## `EPS_map`
 * 
 * Originally developed for EPS group mapping in fzupdate, based on developments reaching back
 * to the Formalizer 1.x code in `dil2al`.
 * 
 * The purpose is to map time in intervals called `slots`. The default size of a `slot` is
 * 5 minutes. Each `slot` is mapped as a pair that associates a UNIX epoch time with a pointer
 * to a Node.
 * 
 * If `time_t` is a `long` on a 64 bit system and pointers are also 64 bits then each slot
 * theoretically consumes 16 bytes. Mapping a year composed of 364*24*60 = 525600 minutes in 5
 * minute slots then requires at least 1681920 bytes (about 1.6 MBytes).
 * 
 * Additionally: `EPS_map` also creates a meta-data map (`node_vector_index`) from Node_ID_key
 * to vector index to align with a vector of `eps_data` for each Node that is mapped into the
 * temporal map.
 * 
 * ## `Minute_Record_Map`
 * 
 * Originally developed to map how time was actually spent, as far as available data in the Log
 * is sufficiently accurate and complete. Of course, the same data type could be used to map
 * a (candidate) Schedule into the future.
 * 
 * As the name suggests, the default interval size is 1 minute. The temporal mapping is a simple
 * vector or pointers to Nodes, each corresponding to a specific minute, between specified
 * `t_start` and `t_end`. Methods are provided to easily map a Log interval and to work with
 * pointer elements at any time within that interval.
 * 
 * If a pointer is represented by 64 bits then each minute consumes 8 bytes. Mapping a year
 * then requires at least 4204800 bytes (about 4.2 MBytes).
 * 
 * Additionally: A separately defined `Node_Category_Cache_Map` can be used to flexibly and
 * with high specificity categorize mapped Nodes, and then, optionnaly, to translate the
 * categorized inspection of the `Minute_Record_Map` even further by translating the output
 * though a second `cat_translation_map`. In summary, this provides a combination of filtering,
 * mapping, inspection, and transformation.
 * 
 * ## `Byte_Map`
 * 
 * The `Byte_Map` is intended for temporal mapping that combines Graph data with either Log
 * or Schedule data, but where it is not necessary to be able to track back from a data point
 * in the map to the Node that produced it.
 * 
 * Instead, a `Byte_Map` is meant to be used to carry out calculations over collections of
 * time points while being relatively conservative in terms of memory consumption and CPU
 * effort devoted to translating data points (hence no bit map).
 * 
 * For example, each byte (`uint8_t`) can be associated with 1 minute of a time span, a specific
 * type of Node (e.g. repeating) can be associated with the value 1 and mapped. Summing the
 * value content of each byte in the map then produces a count of the number of minutes that are
 * associated with such Nodes.
 * 
 * A year of 525600 minutes consumes 525600 bytes in a `Byte_Map` (i.e. about 0.5 MBytes).
 * 
*/

typedef std::vector<uint8_t> byte_vector_t;

struct Byte_Map {
    byte_vector_t temporalrecord;
    time_t t_start;
    time_t t_end;
    time_t interval_seconds;

    /**
     * @param from_t The earliest time point to include in the map.
     * @param before_t The first time point beyond the map (if `invlusive==false`) or the last time point included.
     * @param inclusive Determines the interpretation of `before_t`.
     * @param construct_empty Determines if `init()` is called during construction or not.
     */
    Byte_Map(time_t from_t, time_t before_t, time_t secstep = 60, bool inclusive = false, bool construct_empty = false) : t_start(from_t), t_end(before_t), interval_seconds(secstep) { if (!construct_empty) init(inclusive); }
    void init(bool inclusive = false);
    void set(uint8_t val, time_t from_t, time_t before_t);
    unsigned long sum();
};


} // namespace fz

#endif // __GRAPHLOGXMAP_HPP
