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

<hr>

<form action="/cgi-bin/fztimezone-cgi.py" method="get">
Set all to: <input id="setto" name="setto" type="number"> <input type="submit" value="Set" />
</form>
<p>
<b>Remember: </b> After changing the time zone offset, <b>stop</b> fzserverpq and <b>restart</b> it so that it will reread its configuration file.
</p>

<hr>

<button id="darkmode" class="button button2" onclick="switch_light_or_dark();">Light / Dark</button>
<script type="module" src="/fzuistate.js"></script>
<script type="text/javascript" src="/clock.js"></script>
</body>
</html>
'''

FAILED_SIGNAL='''
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

FAILED_DATA='''
The fztimezone call failed to return valid data.
<P>
Error string was: %s
<P>
Does the user running fzserverpq have write permission in the director?<BR>
Are they a member of the group that has write permissions there?<BR>
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
setto = form.getvalue('setto')

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

# Note:
# As fztimezone is called in 'quiet' mode, the output will
# contained either pure JSON or an error string.
def show_result(cgioutfile:str, expect_json=True)->dict:
    res = {}
    try:
        with open(cgioutfile,'r') as f:
            cgioutstr = f.read()
    except:
        filereaderror = "ERROR: Unable to read %s" % cgioutfile
        res['ERROR'] = FAILED_DATA % filereaderror
    if expect_json:
        try:
            res = json.loads(cgioutstr)
        except:
            res['ERROR'] = FAILED_DATA % cgioutstr
    return res

def run_as_server_user_and_await(cgiprog:str, cgiargs:str, cgioutfile:str, signalfile:str, expect_json=True, timeout_s=600)->dict: # 1 minute default time-out
    startvalue=get_start_value()

    try:
        serial_API_request(f'CGIbg_run_as_user({cgiprog},{cgiargs},{cgioutfile})', running_on_server=True, error_exit_pause=False)
    except Exception as e:
        return { "ERROR": "serial_API_request() failed: "+str(e) }

    for i in range(timeout_s):
        sleep(0.1)
        signalvalue, error_str = get_signal_value(signalfile)
        if int(signalvalue) >= int(startvalue):
            return show_result(cgioutfile, expect_json)

    return { "ERROR": FAILED_SIGNAL % (signalfile, signalvalue, error_str) }

def get_tzinfo_as_server_user()->dict:
    signalfile='/var/www/webdata/formalizer/fztimezone.signal'
    cgiprog='fztimezone'
    cgiargs='s=&q=&S=%s' % signalfile
    cgioutfile='/var/www/webdata/formalizer/fztimezone.out'

    return run_as_server_user_and_await(cgiprog, cgiargs, cgioutfile, signalfile)

REDIRECT='''
<html>
<meta http-equiv="Refresh" content="0; url='%s'" />
</html>
'''

def set_tzinfo_as_server_user(setto):
    try:
        tzhours = int(setto)
    except Exception as e:
        print('<html><body>Exception: %s</body></html>' % str(e))
        return

    signalfile='/var/www/webdata/formalizer/fztimezone.signal'
    cgiprog='fztimezone'
    cgiargs='z=%d&q=&S=%s' % (tzhours, signalfile)
    cgioutfile='/var/www/webdata/formalizer/fztimezone-set.out'
    res_dict = run_as_server_user_and_await(cgiprog, cgiargs, cgioutfile, signalfile, expect_json=False)
    if 'ERROR' in res_dict:
        print('<html><body>Error: %s</body></html>' % str(res_dict['ERROR']))
        return

    print(REDIRECT % '/cgi-bin/fztimezone-cgi.py?show=true')
    

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

    elif setto:
        set_tzinfo_as_server_user(setto)
        sys.exit(0)

    elif (len(thecmd)==0):
        page_content += '<p><b>Unrecognized time zone request.</b></p>'

    print(page_template % page_content)

    sys.exit(0)
