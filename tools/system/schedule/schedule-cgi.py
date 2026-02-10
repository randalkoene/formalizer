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

print("Content-type:text/html\n\n")

# *** THESE SHOULD BE SET BY SETUP CONFIGURATION
# *** Perhaps read the following from ~/.formalizer/webdata_path
webdata_path = "/var/www/webdata/formalizer"

logfile = webdata_path+'/schedule-cgi.log'
schedulename = '/fzschedule.html'
schedulefile = webdata_path+schedulename
redirectpath = '/formalizer/data'+schedulename

schedulecsvfile = webdata_path+'/fzschedule.csv'

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
num_days_str = form.getvalue('num_days')
calendar_schedule = form.getvalue('c')
min_block_size = form.getvalue('s')
vertical_multiplier = form.getvalue('M')

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
<script type="module" src="/fzuistate.js"></script>

<h1>schedule-cgi API</h1>

Generate a proposed schedule.

<p>
Options:
<ul>
<li><code>num_days</code>: Number of days
<li><code>c</code>: Make calendar
<li><code>s</code>: Minimum block size in minutes
</ul>
</p>

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
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
        child_stdin.close()
        result = child_stdout.read()
        error = child_stderr.read()
        child_stdout.close()
        child_stderr.close()
        if printhere:
            print(result)
            return ''
        if len(error)>0:
            print(error)
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
    if min_block_size:
        #min_block_size_arg = '-s %s' % min_block_size
        min_block_size_arg = '-b %s' % min_block_size
    else:
        min_block_size_arg = ''
    #thecmd = "./schedule.py -w -d %s %s" % (num_days, min_block_size_arg)
    thecmd = "./schedule -w -S f_late_v_early_2 -D %s %s" % (num_days, min_block_size_arg)
    log(try_command_call(thecmd, printhere=False))
    #print("Content-type:text/html\n\n")
    #print('Done')
    print(REDIRECT % redirectpath)

def generate_calendar_schedule():
    if min_block_size:
        #min_block_size_arg = '-s %s' % min_block_size
        min_block_size_arg = '-b %s' % min_block_size
    else:
        min_block_size_arg = ''
    if vertical_multiplier:
        vertical_multiplier_arg = '-M %s' % vertical_multiplier
    else:
        vertical_multiplier_arg = ''
    #thecmd = "./schedule.py -W -d %s %s" % (num_days, min_block_size_arg)
    thecmd = "./schedule -W -S f_late_v_early_2 -D %s %s" % (num_days, min_block_size_arg)
    log(try_command_call(thecmd, printhere=False))
    thecmd = f"./nodeboard -c {schedulecsvfile} -H 'Proposed Schedule' {vertical_multiplier_arg} -q -o {schedulefile}"
    log(try_command_call(thecmd, printhere=False))
    #print("Content-type:text/html\n\n")
    print(REDIRECT % redirectpath)

def show_interface_options():
    #print("Content-type:text/html\n\n")
    print(interface_options_help)

if __name__ == '__main__':
    if help:
        show_interface_options()
        sys.exit(0)

    if num_days_str:
        num_days = int(num_days_str)
    
    if calendar_schedule:
        generate_calendar_schedule()
        sys.exit(0)

    generate_schedule()
    sys.exit(0)
