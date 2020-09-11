// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This is a snippet that belongs in logtest.cpp.
 * 
 * This file exists only to separate the raw string definitions for inja tempaltes
 * that are included at compile time.
 * 
 * Make sure to include this file in logtest.cpp.
 * 
 * Defines __LOGTEST_TEMPLATES_CPP to prevent being multiply defined.
 *
 * For more development details, see the Trello card at https://trello.com/c/NNSWVRFf.
 */

#ifndef __LOGTEST_TEMPLATES_CPP
#include "version.hpp"
#define __LOGTEST_TEMPLATES_CPP (__VERSION_HPP)

// Used in: sample_Chunk_HTML()
const std::string testentry
(R"(    <tr><td class="entrycell">[<b>{{ minor_id }}</b>]
    {{ entry }}
    </td></tr>
)");

const std::string testchunk
(R"(    <tr><td>
    {{ chunk }}: @{{ node }}
    <table>
    {{ entries }}
    </table>
    </td></tr>
)");

// Used in: render_Breakpoint()
const std::string testbreakpoint
(R"(    <tr><td>
    <b>Log Breakpoint:</b> <a href="lists/{{ TLfile }}">{{ TLfile }}</a><br>
    <table>
    {{ firstchunk }}
    </table>
    </td></tr>
)");

const std::string testchunk_notfound
(R"(    <tr><td>
    <b>Chunk not found!</b>
    </td></tr>
)");

/* The following is now in the file test_page.template.
// Used in: test_Log_data()
const std::string testpage
(R"(<!DOCTYPE html>
<html>

  <head>
    <title>{{ title }}</title>
  </head>

  <style>
  body {
    font-family: Arial, Helvetica, sans-serif;
    font-size:12px
  }
  table, th, td {
    border: 1px solid black;
    border-collapse: collapse;
  }
  </style>

  <body>

    <h1>{{ header }}</h1>

    This test page was generated as a test of Task Log to Log conversion with <code>dil2graph</code>.

    <h2>{{ breakpoints_header }}</h2>

    <table>
    {{ breakpoints }}
    </table>

    <h2>{{ chunks_header }}</h2>

    <table>
    {{ chunks }}
    </table>

  </body>

</html>
)");
*/

const std::string zerochunks
(R"(
<tr><td>
<b>0 chunks in the Log!</b>
</td></tr>
)");

const std::string zerobreakpoints
(R"(
<tr><td>
<b>0 Breakpoints in the Log!</b>
</td></tr>
)");

#endif // _TEMPLATES_CPP