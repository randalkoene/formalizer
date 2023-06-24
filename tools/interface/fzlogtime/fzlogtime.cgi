#!/usr/bin/python3
#
# Randal A. Koene, 20210326
#
# This CGI handler provides a web interface to fzlogtime.

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os, stat
sys.stderr = sys.stdout
from time import strftime
import traceback
from io import StringIO
from traceback import print_exc
import subprocess
import json

try:
    with open('/home/randalk/.formalizer/config/fzlogtime/config.json', 'r') as f:
        config_json = json.load(f)
except Exception as e:
    config_json = {}
#print('config_json='+str(config_json))

# Create instance of FieldStorage 
form = cgi.FieldStorage()

source = form.getvalue('source')
verbositystr = form.getvalue('verbosity')
if verbositystr:
    try:
        verbosity = int(verbositystr)
    except:
        verbosity = 0
else:
    verbosity = 0 # quiet by default

logtime_failure = """Content-type:text/html

<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>fz: Logtime</title>
</head>
<body>
<h3>fz: Logtime</h3>

<p><b>ERROR: Unable to generate Logtime page.</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

</body>
</html>
"""

results = {}

def try_subprocess_check_output(thecmdstring: str, resstore: str, verbosity: 1) -> int:
    if verbosity > 1:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
    try:
        res = subprocess.check_output(thecmdstring, shell=True)
    except subprocess.CalledProcessError as cpe:
        if verbosity > 0:
            print('Subprocess call caused exception.')
            print('Error output: ',cpe.output.decode())
            print('Error code  : ',cpe.returncode)
            if (cpe.returncode>0):
                print(f'Formalizer error return code: {cpe.returncode}')
        return cpe.returncode
    else:
        if resstore:
            results[resstore] = res
        if verbosity > 1:
            print('Result of subprocess call:', flush=True)
            print(res.decode(), flush=True)
        return 0

def extra_cmd_args(verbosity = 1) -> str:
    global config_json

    #print('GOT HERE')

    extra = ''
    if verbosity < 1:
        extra += ' -q'
    else:
        if verbosity > 1:
            extra += ' -V'

    if 'hours_offset' in config_json:
        #print('OFFSETTING HOURS')
        extra += ' -H '+str(config_json['hours_offset'])
    return extra

def local_fzlogtime() -> bool:
    thecmd = './fzlogtime' + extra_cmd_args(verbosity)
    #print("thecmd="+thecmd)
    retcode = try_subprocess_check_output(thecmd, 'logtime', verbosity)
    if (retcode == 0):
        try:
            print(results['logtime'].decode())
            return True
        except:
            print(logtime_failure)
            return False
    else:
        print(logtime_failure)
        return False

def nonlocal_fzlogtime() -> bool:
    thecmd = './fzlogtime -E STDOUT -W STDOUT -n ' + extra_cmd_args(verbosity)
    #print('thecmd='+thecmd)
    retcode = try_subprocess_check_output(thecmd, 'logtime', verbosity)
    if (retcode == 0):
        try:
            print(results['logtime'].decode())
            return True
        except:
            print(logtime_failure)
            return False
    else:
        print(logtime_failure)
        return False

if __name__ == '__main__':
    if (source == 'nonlocal'):
        if nonlocal_fzlogtime():
            sys.exit(0)
        else:
            sys.exit(1)
    else:
        if local_fzlogtime():
            sys.exit(0)
        else:
            sys.exit(1)
