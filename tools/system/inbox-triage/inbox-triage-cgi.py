#!/usr/bin/python3
#
# inbox-triage-cgi.py
#
# Randal A. Koene, 20261014
#
# A CGI script for the dashboard's Inbox Triage button. If the inbox-triage
# server is not running yet, it uses an FZ API fzserverpq call to start it as
# the right user, waits for it to come up, then sends the browser there.
#
# The server must not run as www-data: it writes its cache in the source
# tree and its jobs read the Gmail tokens in ~/.formalizer/config.
#
# NOTE: For this to work, inbox-triage-start.py must be included in the
#       fzserverpq configuration variable 'predefined_CGIbg'.

import sys, os, socket, html
from time import sleep
sys.stderr = sys.stdout

from fzmodbase import *
from tcpclient import serial_API_request

print("Content-type:text/html\n")

triage_url = 'http://127.0.0.1:8711/'
start_outfile = '/dev/shm/inbox-triage-start.out'
timeout_s = 25

def listening() -> bool:
    try:
        with socket.create_connection(('127.0.0.1', 8711), timeout=0.5):
            return True
    except OSError:
        return False

def page(title: str, body: str, refresh=False):
    meta = f'<meta http-equiv="refresh" content="0; url={triage_url}">' if refresh else ''
    print(f'''<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
{meta}
<link rel="stylesheet" href="/fz.css">
<title>Inbox Triage</title>
</head>
<body>
<h3>{title}</h3>
{body}
</body>
</html>''')

if listening():
    page('Opening Inbox Triage...', f'<p><a href="{triage_url}">{triage_url}</a></p>', refresh=True)
    sys.exit(0)

if os.path.exists(start_outfile):
    old_modification_timestamp = os.path.getmtime(start_outfile)
else:
    old_modification_timestamp = None

cgiprog = 'inbox-triage-start.py'
cgiargs = ''
serial_API_request(f'CGIbg_run_as_user({cgiprog},{cgiargs},{start_outfile})', running_on_server=True, error_exit_pause=False)

for i in range(timeout_s*10):
    sleep(0.1)
    if listening():
        page('Starting Inbox Triage...', f'<p><a href="{triage_url}">{triage_url}</a></p>', refresh=True)
        sys.exit(0)

report = ''
if os.path.exists(start_outfile) and os.path.getmtime(start_outfile) != old_modification_timestamp:
    with open(start_outfile, 'r', errors='replace') as f:
        report = f.read()
if not report:
    report = ('No report from inbox-triage-start.py. Is it listed in predefined_CGIbg\n'
              'in the fzserverpq config, and was fzserverpq restarted since?')
page('Inbox Triage did not start', f'<pre>{html.escape(report)}</pre>')
sys.exit(1)
