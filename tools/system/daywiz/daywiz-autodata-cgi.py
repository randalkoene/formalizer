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
from datetime import datetime
import traceback
from json import load
#from io import StringIO
#from traceback import print_exc
#from subprocess import Popen, PIPE
#from pathlib import Path
import base64


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

AUTODATA_HTML='''<!DOCTYPE html>
<html>
<HEAD>
<meta charset="utf-8" />
<link rel="icon" href="/favicon-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">

<TITLE>DayWiz AutoData</TITLE>
<style>
table, th, td {
  border: 1px solid gray;
  border-collapse: collapse;
  padding: 15px;
}
</style>
</HEAD>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>

<H1>DayWiz AutoData</H1>

<b>Unread emails</b>: %s
<P>
<b>Chunks with open checkboxes</b>: %s
<P>
<b>Calendar events</b>:
%s
<P>
%s

<script>
function sendData(data) {
    // Open a new browser window with the CGI script's URL and pass the data as a query parameter
    var cgiScriptUrl = "/cgi-bin/fzgraphhtml-cgi.py?edit=new&data=" + encodeURIComponent(data);
    window.open(cgiScriptUrl, '_blank');
}
</script>
</body>
</html>
'''

CALENDAR_A_EVENT_LINE='''<tr>
<td><button onclick="sendData('%s')">MkNode</button></td>
<td>%s</td>
<td>%s</td>
<td>%s</td>
<td>%s</td>
</tr>
'''

CALENDAR_B_EVENT_LINE='''<tr>
<td>%s</td>
<td>%s</td>
<td>%s</td>
<td>%s</td>
</tr>
'''

form = cgi.FieldStorage()

dummy = form.getvalue('dummy')

def encode_content(content:str, start_time:datetime, end_time:datetime):
    content += '@EXTRA_DATA@'
    content += end_time.strftime('%Y%m%d%H%M')
    rq_hint = (end_time - start_time).total_seconds()/3600.0
    content += ',%.2f' % rq_hint
    return base64.urlsafe_b64encode(content.encode()).decode()

def date_time_str(t:datetime)->str:
    return t.strftime("%Y%m%d %H:%M")

def show_data(cgioutfile:str):
    with open(cgioutfile, 'r') as f:
        data = load(f) # This is json.load().
    all_day_events = []
    events = []
    for entry in data['calendar_events']:
        if len(entry['start']) > 10:
            events.append(entry)
        else:
            all_day_events.append(entry)

    format_string = "%Y-%m-%dT%H:%M:%S%z"
    calendar_events_str = '<table></tbody>'
    if len(events)>0:
        start_time = datetime.strptime(events[0]['start'], format_string)
        calendar_events_str += "<p>Times expressed in GMT %s</p>" % start_time.strftime('%z')
    for entry in events:
        start_time = datetime.strptime(entry['start'], format_string)
        end_time = datetime.strptime(entry['end'], format_string)
        calendar_events_str += CALENDAR_A_EVENT_LINE % (encode_content(entry['event']+'\n'+entry['location'], start_time, end_time), date_time_str(start_time), date_time_str(end_time), entry['event'], entry['location'])
    calendar_events_str += '</tbody></table>'

    calendar_events_str += '<table></tbody>'
    for entry in all_day_events:
        calendar_events_str += CALENDAR_B_EVENT_LINE % (entry['start'], entry['end'], entry['event'], entry['location'])
    calendar_events_str += '</tbody></table>'

    if 'email_details' in data:
        email_details_str = '<b>Emails received in the last 24 hours (%d)</b>:\n<table></tbody>\n' % len(data['email_details'])
        for email_details in data['email_details']:
            email_details_str += '<tr><td>'+email_details['from']
            email_details_str += '</td><td>'+email_details['subject']+'</td></tr>\n'
        email_details_str += '</tbody></table>\n'
    else:
        email_details_str = ''

    print(AUTODATA_HTML % (str(data['unread_emails']), str(data['chunks_open_checkboxes']), str(calendar_events_str), str(email_details_str)))

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
