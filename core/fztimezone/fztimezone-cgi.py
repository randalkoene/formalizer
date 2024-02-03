#!/usr/bin/python3
#
# Randal A. Koene, 20240201
#
# CGI script to call fztimezone.

# Do this immediately to catch bugs:
print('Content-type:text/html\n\n')

page_template = '''<html>
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta http-equiv="X-UA-Compatible" content="ie=edge">
<link rel="icon" href="/favicon-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/clock.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: TimeZone</title>
</head>
<body>
<button id="clock" class="button button2">_____</button>
<h1>fz: TimeZone</h1>

%s

<button id="darkmode" class="button button2" onclick="switch_light_or_dark();">Light / Dark</button>
<script type="text/javascript" src="/fzuistate.js"></script>
<script type="text/javascript" src="/clock.js"></script>
</body>
</html>
'''

FAILED='''
The fztimezone call failed to produce a signal.
<P>
Last signal time stamp read from %s was: %s
<P>
Error string was: %s
<P>
Does the user running fzserverpq have write permission in the directory?<BR>
Are they a member of the group that has write permission there?<BR>
Does the group have write permission in the directory?
'''

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
from time import strftime, sleep
import traceback
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE
import json

from fzmodbase import *
#from fzcmdcalls import try_subprocess_check_output
from tcpclient import serial_API_request
from TimeStamp import TimeStamp, ActualTime

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
show = form.getvalue('show')
verbose = form.getvalue('verbose')

# --- We run as the fzserverpq user in order to find the correct config files:

def get_start_value()->str:
    return TimeStamp(ActualTime()) # time stamp in seconds (not the same as NowTimeStamp())

def get_signal_value(signalfile:str)->tuple:
    try:
        with open(signalfile,'r') as f:
            signalvalue=f.read()
        return (signalvalue, "")
    except Exception as e:
        return ("0", str(e))

def show_result(cgioutfile:str)->dict:
    res = {}
    try:
        with open(cgioutfile,'r') as f:
            cgioutstr=f.read()
            res = json.loads(cgioutstr)
    except:
        res['ERROR'] = "ERROR: Unable to read %s" % cgioutfile
    return res

def get_tzinfo_as_server_user()->dict:
    signalfile='/var/www/webdata/formalizer/fztimezone.signal'
    cgiprog='fztimezone'
    cgiargs='s=&q=&S=%s' % signalfile
    cgioutfile='/var/www/webdata/formalizer/fztimezone.out'
    startvalue=get_start_value()

    try:
        serial_API_request(f'CGIbg_run_as_user({cgiprog},{cgiargs},{cgioutfile})', running_on_server=True, error_exit_pause=False)
    except Exception as e:
        return { "ERROR": "serial_API_request() failed: "+str(e) }

    timeout_s=1*60*10 # 1 minute
    for i in range(timeout_s):
        sleep(0.1)
        signalvalue, error_str = get_signal_value(signalfile)
        if int(signalvalue) >= int(startvalue):
            return show_result(cgioutfile)

    return { "ERROR": FAILED % (signalfile, signalvalue, error_str) }

# -----------------------------------------------------------------

TZLINE_TEMPLATE='<tr><td>%s</td><td><input id="%s" name="%s" value="%s"></td></tr>\n'

def render_tz_dict(tzdict:dict)->str:
    out_str = '<table>\n'
    for tzprog in tzdict:
        out_str += TZLINE_TEMPLATE % (str(tzprog), str(tzprog), str(tzprog), str(tzdict[tzprog]))
    out_str += '</table>\n'
    return out_str

if __name__ == '__main__':

    page_content = ''

    thisscript = os.path.realpath(__file__)

    page_content += f'<!--(For dev reference, this script is at {thisscript}.) -->'
    page_content += "<!-- [Formalizer: fztimezone handler]\n<p></p> -->"

    add_to_cmd = ''
    if verbose:
        add_to_cmd += ' -V'

    thecmd = ''

    if show:
        res_dict = get_tzinfo_as_server_user()
        if "ERROR" in res_dict:
            page_content += res_dict["ERROR"]+'\n'
        else:
            page_content += render_tz_dict(res_dict)
    elif (len(thecmd)==0):
        page_content += '<p><b>Unrecognized time zone request.</b></p>'

    print(page_template % page_content)

    sys.exit(0)
