// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <algorithm>

// core
//#include "error.hpp"
#include "GraphLogxmap.hpp"


namespace fz {

void Byte_Map::init(bool inclusive) {
    if (inclusive) {
        t_end += interval_seconds;
    }
    size_t approx_size = ((t_end - t_start) / interval_seconds) + 1; // This hopefully hops over leap seconds and includes an extra minute.
    temporalrecord.resize(approx_size, 0);
}

void Byte_Map::set(uint8_t val, time_t from_t, time_t before_t) {
    // Is there an overlap with the temporalrecord interval?
    if (from_t >= t_end) {
        return;
    }
    if (before_t <= t_start) {
        return;
    }
    // What are the overlapping start and end points?
    if (from_t < t_start) {
        from_t = t_start;
    }
    if (before_t > t_end) {
        before_t = t_end;
    }
    // What are the corresponding indices?
    size_t from_idx = (from_t - t_start) / interval_seconds;
    size_t before_idx = (before_t - t_start) / interval_seconds;
    if (from_idx >= before_idx) {
        return;
    }
    std::fill (temporalrecord.begin()+from_idx, temporalrecord.begin()+before_idx, val);
}

unsigned long Byte_Map::sum() {
    unsigned long thesum = 0;
    for (const auto & val : temporalrecord) {
        thesum += val;
    }
    return thesum;
}

} // namespace fz
