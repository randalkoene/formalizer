#!/usr/bin/env python3
# metrics.py
# Copyright 2022 Randal A. Koene
# License TBD
#
# Metrics data access.
#
# For details, see: https://trello.com/c/JssbodOF .
#
# This can be launched as a CGI script.

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout

from datetime import datetime
from time import time
import json
from os.path import exists

from fzhtmlpage import *

# Create instance of FieldStorage 
# NOTE: Only parameters that have a "name" (not just an "id") are submitted.
#       We do not always supply a name, because it is faster if we only receive
#       the par_changed and par_newval values to parse.
form = cgi.FieldStorage()
#cgitb.enable()

t_run = datetime.now() # Use this to make sure that all auto-generated times on the page use the same time.

# ====================== Data store:

# TODO: *** Switch to using the database.
#JSON_DATA_PATH='/home/randalk/.formalizer/.daywiz_data.json' # Permission issues.
JSON_DATA_PATH='/var/www/webdata/formalizer/.daywiz_data.json'

