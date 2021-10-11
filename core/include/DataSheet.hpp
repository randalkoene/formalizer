// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Definitions and methods for Data sheets. These are used the way spreadsheets are used.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __DATASHEET_HPP.
 */

#ifndef __DATASHEET_HPP
#include "coreversion.hpp"
#define __DATASHEET_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
#include <any>
#include <vector>

// core
//#include "error.hpp"


namespace fz {

struct DataCell {
    std::any data;
    int row;
    int col;
};

struct DataColumns {
    std::vector<DataCell> columns;
    int row;
};

struct DataSheet {
    std::vector<DataColumns> rows;
    std::string sheet_id;
};

} // namespace fz

#endif // __DATASHEET_HPP
