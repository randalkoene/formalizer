#!/usr/bin/python3
#
# Copyright 2020 Randal A. Koene
# License TBD

"""
This is the Python module for the Formalizer authoritative Postgres table structures
as copies directly from Graphpostgres.cpp. For more information about the layout of
the data structure, please see there and in Graphtypes.cpp/hpp.
"""


pq_enum_td_property = "('unspecified','inherit','variable','fixed','exact')"

pq_enum_td_pattern = "('patt_daily','patt_workdays','patt_weekly','patt_biweekly','patt_monthly','patt_endofmonthoffset','patt_yearly','OLD_patt_span','patt_nonperiodic')"

pq_nodelayout = [
    "id char(16) PRIMARY KEY,"
    "topics smallint[],"
    "topicrelevance real[],"
    "valuation real,"
    "completion real,"
    "required integer,"
    "text text,"
    "targetdate timestamp (0),"
    "tdproperty td_property,"
    "isperiodic boolean,"
    "tdperiodic td_pattern,"
    "tdevery integer,"
    "tdspan integer"
]

pq_edgelayout = [
    "id char(33),"
    "dependency real,"
    "significance real,"
    "importance real,"
    "urgency real,"
    "priority real"
]

pq_topiclayout = [
    "id smallint,"
    "supid smallint,"
    "tag text,"
    "title text,"
    "keyword text[],"
    "relevance real[]"
]
