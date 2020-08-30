// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares Log Postgres types for use with the Formalizer.
 * These define the authoritative version of the data structure for storage in PostgreSQL.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __LOGPOSTGRES_HPP.
 */

/**
 * The functions and classes in this header are used to prepare Log chunk, Log entry and
 * related data of a Log for storage in a Postgres database, and they are used to retrieve
 * that data from the Postgres database.
 * 
 * The data structure in C++ is defined in Logtypes.hpp. The data structure in
 * Postgres is defined here.
 * 
 * Functions are also provided to initialze Log data in Postgres by converting a
 * complete Log with all Chunks and Entries from another storage format to Postgres
 * format. At present, the only other format supported is the original HTML storage
 * format used in dil2al.
 * 
 * Even though this is C++, we use libpq here (not libpqxx) to communicate with the Postgres
 * database. The lipbq is the only standard library for Postgres and is perfectly usable
 * from C++.
 * 
 * This version is focused on simplicity and human readibility, not maximum speed. A
 * complete Log is intialized relatively rarely. Unless automated tools generate new
 * Chunks and Entries very quickly, writing speed should not be a major issue.
 * 
 */

#ifndef __LOGPOSTGRES_HPP
#include "coreversion.hpp"
#define __LOGPOSTGRES_HPP (__COREVERSION_HPP)

#include "Logtypes.hpp"

/**
 * On Ubuntu, to install the libpq libraries, including the libpq-fe.h header file,
 * do: sudo apt-get install libpq-dev
 * You may also have to add /usr/include or /usr/include/postgresql to the CPATH or
 * to the includes in the Makefile, e.g. -I/usr/include/postgresql.
 */
#include <libpq-fe.h>

namespace fz {

//*** Right now, this is just a stub.

} // namespace fz

#endif // __LOGPOSTGRES_HPP
