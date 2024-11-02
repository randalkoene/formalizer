#!/usr/bin/env python3
# score.py
# Copyright 2023 Randal A. Koene
# License TBD
#
# Display Node specific metrics.
#
# This can be launched as a CGI script.

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
from io import StringIO
from subprocess import Popen, PIPE
import traceback

import plotly.express as px
import plotly.io as pxio

fig = px.scatter(score_data, x=score_days, y=score_data)
self.content = pxio.to_html(fig, full_html=False)

if __name__ == '__main__':


# This will use fzlogmap, which needs an -n option much like fzloghtml has.
