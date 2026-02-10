#!/usr/bin/python3
#
# fzbackup-from-web-cgi.py
#
# Randal A. Koene, 20231225
#
# A CGI script that uses an FZ API fzserverpq call to run a backup as
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
from time import strftime, sleep
import datetime
import traceback
from json import loads, load
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE
from pathlib import Path

from fzmodbase import *
from tcpclient import serial_API_request
from TimeStamp import TimeStamp, ActualTime

home = str(Path.home())

# THESE SHOULD BE SET BY SETUP CONFIGURATION
# (Or they should be obtained somehow, e.g. via FZ API or from
# a standard location, e.g. ~/.formalizer/webdata_path.)
webdata_path = "/var/www/webdata/formalizer"

logfile = webdata_path + '/fzbackup-from-web-cgi.log'

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
sysmet_file = form.getvalue('f')
node_dependencies = form.getvalue('n')
filter_string = form.getvalue('F')
show_completed = form.getvalue('I')
subtree_list = form.getvalue('D')
threads = form.getvalue('T')

if filter_string:
    include_filter_string = ' -F %s' % filter_string
else:
    include_filter_string = ''
if show_completed == 'true':
    include_show_completed = ' -I'
else:
    include_show_completed = ''
if threads:
    include_threads = ' -T'
else:
    include_threads = ''

def log(logstr):
    with open(logfile, 'w') as f:
        f.write(logstr)

# def try_command_call(thecmd, print_result=True)->str:
#     try:
#         log(thecmd)
#     except:
#         pass
#     try:
#         #print(thecmd, flush=True)
#         p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
#         (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
#         child_stdin.close()
#         result = child_stdout.read()
#         error = child_stderr.read()
#         child_stdout.close()
#         child_stderr.close()
#         if print_result:
#             print(result)
#             return ''
#         if len(error)>0:
#             print(error)
#         #print(result.replace('\n', '<BR>'))
#         return result

#     except Exception as ex:                
#         print(ex)
#         f = StringIO()
#         print_exc(file=f)
#         a = f.getvalue().splitlines()
#         for line in a:
#             print(line)
#         return ''

HELP='''
<html>
<title>fzbackup-from-web-cgi.py - Help</title>
<body>
The fzbackup-from-web-cgi.py script received no recognized form data.
</body>
</html>
'''

FAILED='''
<html>
<title>fzbackup-from-web-cgi.py - Failed</title>
<body>
The fzbackup-from-web-cgi.py script failed to receive a signal from fzbackup-mirror-to-github.sh.
<P>
Last signal time stamp read from %s was: %s
<P>
Error string was: %s
<P>
Does the user running fzserverpq have write permission in the directory?<BR>
Are they a member of the group that has write permission there?<BR>
Does the group have write permission in the directory?
</body>
</html>
'''

BACKUP_RESULT='''
<html>
<head>
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>FZ: fzbackup-from-web-cgi.py - Result</title>
<style>
td {
	padding: 15px;
	vertical-align: top;
}
</style>
<body>
<pre>
%s
</pre>
<table>
<tbody>
<tr>
<td>
<pre>
%s
</pre>
</td>
<td>
<pre>
%s
</pre>
</td>
</tr>
</tbody>
</table>
<script type="module" src="/fzuistate.js"></script>
</body>
</html>
'''

def show_help():
    print(HELP)

def get_start_value()->str:
	return TimeStamp(ActualTime()) # time stamp in seconds (not the same as NowTimeStamp())

def get_signal_value(signalfile:str)->tuple:
	try:
		with open(signalfile,'r') as f:
			signalvalue=f.read()
		return (signalvalue, "")
	except Exception as e:
		return ("0", str(e))

def get_result_parts(full_result_string:str)->tuple:
	part2_start = full_result_string.find("List of Database")
	part3_start = full_result_string.find("List of DayWiz")
	if part2_start < 0:
		return (full_result_string, '', '')
	if part3_start < 0:
		return (full_result_string[0:part2_start], full_result_string[part2_start:], '')
	return (full_result_string[0:part2_start], full_result_string[part2_start:part3_start], full_result_string[part3_start:])

def show_backup_result(cgioutfile:str):
	try:
		with open(cgioutfile,'r') as f:
			cgioutstr=f.read()
	except:
		cgioutstr="ERROR: Unable to read %s" % cgioutfile
	result_parts = get_result_parts(cgioutstr)
	print(BACKUP_RESULT % result_parts)

def backup_with_mirror_to_github():
	#1. Launch fzbackup-mirror-to-github.sh on the server as the right user in the background.
	# NOTE: In other scripts, e.g. fzupdate-cgi.py FZ API calls are made by calling fzgraph -C.
	#       Here, we try to do this more directly by making a TCP API call.
	#       In both cases, this is running on the server, i.e. references are to localhost.
	#signalfile='/dev/shm/fzbackup-mirror-to-github.signal'
	signalfile='/var/www/webdata/formalizer/fzbackup-mirror-to-github.signal'
	cgiprog='fzbackup-mirror-to-github.sh'
	cgiargs='S=%s' % signalfile
	#cgioutfile='/dev/shm/fzbackup-mirror-to-github.out'
	cgioutfile='/var/www/webdata/formalizer/fzbackup-mirror-to-github.out'
	startvalue=get_start_value()

	serial_API_request(f'CGIbg_run_as_user({cgiprog},{cgiargs},{cgioutfile})', running_on_server=True, error_exit_pause=False)

	# *** If fzbackup-mirror-to-github.sh is updated to update a progress state file
	#     then here is where we could (mostly) replace the loop below with a call to fzbgprogress.py:make_background_progress_monitor()
	#     as already used in nodeboard-cgi.py.

	# 2. Wait for an updated signal value to indicate that the background process is done.
	timeout_s=3*60 # 3 minutes
	for i in range(timeout_s):
		sleep(1)
		signalvalue, error_str = get_signal_value(signalfile)
		if int(signalvalue) >= int(startvalue):
			show_backup_result(cgioutfile)
			return

	print(FAILED % (signalfile, signalvalue, error_str))

tmp_always_runs=True

if __name__ == '__main__':
    if tmp_always_runs:
        backup_with_mirror_to_github()
        sys.exit(0)

    show_help()
    sys.exit(0)
