#!/usr/bin/python3
#
# email-info-cgi.py
#
# Randal A. Koene, 20260107
#
# A CGI script that uses an FZ API fzserverpq call to run email-info.py as
# the right user and to return the results as an HTML page.
#
# NOTE: For this to work, the user running fzserverpq must be a
#       member of the www-data group and the directory at
#       /var/www/webdata/formalizer must be writable by members
#       of the www-data group.

import sys, os
from os.path import exists
sys.stderr = sys.stdout
from time import sleep
import traceback

from fzmodbase import *
from tcpclient import serial_API_request

print("Content-type:text/html\n")

email_autodata_file = '/var/www/webdata/formalizer/email_autodata.log'

if exists(email_autodata_file):
    old_modification_timestamp = os.path.getmtime(email_autodata_file)
else:
    old_modification_timestamp = ''
cgiprog='email-info.py'
cgiargs=''
cgioutfile=email_autodata_file

serial_API_request(f'CGIbg_run_as_user({cgiprog},{cgiargs},{cgioutfile})', running_on_server=True, error_exit_pause=False)

timeout_s=3*60 # 3 minutes
for i in range(timeout_s):
    sleep(0.1)
    if exists(email_autodata_file):
        new_modification_timestamp = os.path.getmtime(email_autodata_file)
        if new_modification_timestamp != old_modification_timestamp:
            if os.path.getsize(email_autodata_file) > 0:
                try:
                    with open(email_autodata_file, 'r') as f:
                        data = f.read()
                    print(str(data))
                    sys.exit(0)
                except Exception as e:
                    print('Data file read error: '+str(e))
                    sys.exit(1)

print('FAILED')
sys.exit(1)
