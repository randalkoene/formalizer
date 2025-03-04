#!/usr/bin/python3
#
# dayreview-plot-cgi.py
#
# Randal A. Koene, 20250303
#
# A CGI script that uses an FZ API fzserverpq call to run dayreview-plot.py as
# the right user and to return the results as an HTML page.
#
# NOTE: For this to work, the user running fzserverpq must be a
#       member of the www-data group and the directory at
#       /var/www/webdata/formalizer must be writable by members
#       of the www-data group.

# Do this immediately, so that any errors are visible:
print("Content-type:text/html\n")

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
from os.path import exists
sys.stderr = sys.stdout
from time import sleep
from datetime import date, timedelta
import traceback
#from json import loads, load
from io import StringIO
from traceback import print_exc
#from subprocess import Popen, PIPE
#from pathlib import Path

from fzmodbase import *
from tcpclient import serial_API_request
#from TimeStamp import TimeStamp, ActualTime

plot_html_file = '/var/www/webdata/formalizer/dayreview_scores.html'

FAILED='''
<html>
<title>dayreview-plot-cgi.py - Failed</title>
<body>
The dayreview-plot-cgi.py script failed to detect a modified output file created by dayreview-plot.sh.
<P>
Does the user running fzserverpq have write permission in the directory?<BR>
Are they a member of the group that has write permission there?<BR>
Does the group have write permission in the directory?
</body>
</html>
'''

form = cgi.FieldStorage()

workweeks = form.getvalue('workweeks')

def show_plot(cgioutfile:str):
    with open(plot_html_file, 'r') as f:
        html_plot = f.read()
    print(html_plot)

def dayreview_plot():
    old_modification_timestamp = os.path.getmtime(plot_html_file)
    cgiprog='dayreview-plot.py'
    if workweeks:
        numweeks = int(workweeks)
        today = date.today()
        one_week_ago = today - timedelta(weeks=numweeks)
        cgiargs='c=on&e=%s&l=%s&w=on' % (one_week_ago.strftime('%Y.%m.%d'), today.strftime('%Y.%m.%d'))
    else:
        cgiargs='c=on'
    cgioutfile='/var/www/webdata/formalizer/dayreview-plot-cgi.out'

    serial_API_request(f'CGIbg_run_as_user({cgiprog},{cgiargs},{cgioutfile})', running_on_server=True, error_exit_pause=False)

    # 2. Wait for an updated modification time to indicate that the background process is done.
    timeout_s=3*60 # 3 minutes
    for i in range(timeout_s):
        sleep(1)
        new_modification_timestamp = os.path.getmtime(plot_html_file)
        if new_modification_timestamp != old_modification_timestamp:
            show_plot(cgioutfile)
            return

    print(FAILED)

dayreview_plot()
