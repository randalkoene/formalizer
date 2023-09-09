#!/usr/bin/python3
#
# Randal A. Koene, 20230812
#
# This CGI handler access to schedule.py via web form.
#
# Expects POST or GET data of the following forms:
#   num_days=<num-days>
#

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
from time import strftime
import traceback
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE
from pathlib import Path
home = str(Path.home())

#print("Content-type:text/html\n\n")

# *** THESE SHOULD BE SET BY SETUP CONFIGURATION
# *** Perhaps read the following from ~/.formalizer/webdata_path
webdata_path = "/var/www/webdata/formalizer"

logfile = webdata_path+'/schedule-cgi.log'
schedulename = '/fzschedule.html'
schedulefile = webdata_path+schedulename
redirectpath = '/formalizer/data'+schedulename

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
num_days_str = form.getvalue('num_days')

# local
help = form.getvalue('help')

num_days = 1

# The following should only show information that is safe to provide
# to those who have permission to connect to this CGI handler.
interface_options_help = '''
<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>schedule-cgi API</title>
</head>
<body>
<h1>schedule-cgi API</h1>

Generate a proposed schedule.

<p>
Options:
<ul>
<li><code>num_days</code>: Number of days
</ul>
</p>

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

REDIRECT='''
<html>
<meta http-equiv="Refresh" content="0; url='%s'" />
</html>
'''

def try_command_call(thecmd, printhere = True) -> str:
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout) = (p.stdin, p.stdout)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        if printhere:
            print(result)
            return ''
        else:
            return result

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        return ''

def log(msg):
    with open(logfile,'w') as f:
        f.write(msg)

def generate_schedule():
    thecmd = "./schedule.py -w -d %s" % num_days
    log(try_command_call(thecmd, printhere=False))
    print("Content-type:text/html\n\n")
    #print('Done')
    print(REDIRECT % redirectpath)

def show_interface_options():
    print("Content-type:text/html\n\n")
    print(interface_options_help)

if __name__ == '__main__':
    if help:
        show_interface_options()
        sys.exit(0)

    if num_days_str:
        num_days = int(num_days_str)
    
    generate_schedule()
    sys.exit(0)
