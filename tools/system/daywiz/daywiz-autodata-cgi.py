#!/usr/bin/python3
#
# daywiz-autodata-cgi.py
#
# Randal A. Koene, 20250922
#
# A CGI script that uses an FZ API fzserverpq call to run daywiz-autodata.py as
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
from json import load
from io import StringIO
from traceback import print_exc
#from subprocess import Popen, PIPE
#from pathlib import Path

from fzmodbase import *
from tcpclient import serial_API_request

daywiz_autodata_file = '/var/www/webdata/formalizer/daywiz_autodata.json'

FAILED='''<html>
<title>daywiz-autodata-cgi.py - Failed</title>
<body>
The daywiz-autodata-cgi.py script failed to retrieve data.
<P>
Does the user running fzserverpq have write permission in the directory?<BR>
Are they a member of the group that has write permission there?<BR>
Does the group have write permission in the directory?
</body>
</html>
'''

TEST_HTML='''<html>
<title>daywiz-autodata_cgi.py - Test</title>
<body>
Data:
<P>
Unread emails: %s
<P>
Chunks with open checkboxes: %s
<P>
Calendar events: %s
</body>
</html>
'''

CALENDAR_EVENT_LINE='''<tr>
<td>%s</td>
<td>%s</td>
<td>%s</td>
<td>%s</td>
</tr>
'''

form = cgi.FieldStorage()

dummy = form.getvalue('dummy')

def show_data(cgioutfile:str):
    with open(cgioutfile, 'r') as f:
        data = load(f)
    all_day_events = []
    events = []
    for entry in data['calendar_events']:
        if len(entry['start']) > 10:
            events.append(entry)
        else:
            all_day_events.append(entry)

    calendar_events_str = '<table></tbody>'
    for entry in events:
        calendar_events_str += CALENDAR_EVENT_LINE % (entry['start'], entry['end'], entry['event'], entry['location'])
    calendar_events_str += '</tbody></table>'

    calendar_events_str += '<table></tbody>'
    for entry in all_day_events:
        calendar_events_str += CALENDAR_EVENT_LINE % (entry['start'], entry['end'], entry['event'], entry['location'])
    calendar_events_str += '</tbody></table>'

    print(TEST_HTML % (str(data['unread_emails']), str(data['chunks_open_checkboxes']), str(calendar_events_str)))

def daywiz_autodata()->bool:
    if exists(daywiz_autodata_file):
        old_modification_timestamp = os.path.getmtime(daywiz_autodata_file)
    else:
        old_modification_timestamp = ''
    cgiprog='daywiz-autodata.py'
    cgiargs='F=json'
    cgioutfile=daywiz_autodata_file

    serial_API_request(f'CGIbg_run_as_user({cgiprog},{cgiargs},{cgioutfile})', running_on_server=True, error_exit_pause=False)

    # 1. Beware, the command is run in the background, i.e. asynchronously.
    #    The server call can return before output writing is completed.
    #    So, even if the cgioutfile has been cleared and is open for writing,
    #    the API calls may not have finished writing into it.

    # 2. Wait for an updated modification time to indicate that the background process is done.
    timeout_s=3*60 # 3 minutes
    for i in range(timeout_s):
        sleep(0.1)
        if exists(daywiz_autodata_file):
            new_modification_timestamp = os.path.getmtime(daywiz_autodata_file)
            if new_modification_timestamp != old_modification_timestamp:
                if os.path.getsize(daywiz_autodata_file) > 0:
                    return True

    print(FAILED)
    return False

if daywiz_autodata():
    show_data(daywiz_autodata_file)
