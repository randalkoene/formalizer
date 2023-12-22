#!/usr/bin/python3
#
# Randal A. Koene, 20231212
#
# This CGI handler displays fzvismilestones output in an HTML page.
#

# Do this immediately, so that any errors are visible:
print("Content-type:text/html\n\n")

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
from os.path import exists
sys.stderr = sys.stdout
from time import strftime
import datetime
import traceback
from json import loads, load
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE
from pathlib import Path
home = str(Path.home())

# *** THESE SHOULD BE SET BY SETUP CONFIGURATION
# *** Perhaps read the following from ~/.formalizer/webdata_path
webdata_path = "/var/www/webdata/formalizer"

logfile = webdata_path+'/fzvismilestones.log'

#DEFAULT_OUTPUT_FILE = webdata_path+"/test_node_graph.sif"

#DEFAULT_REDIRECT_TARGET = "/data/cytoscape_graph.html"
DEFAULT_REDIRECT_TARGET = "/data/cytowebapp"

SUCCESSFUL_OUTPUT_SIGNAL_FILE = "/dev/shm/fzvismilestones.webapp.updated"

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
filter_string = form.getvalue('F')
show_completed = form.getvalue('I')
show_only_labels = form.getvalue('L')

if filter_string != '':
    include_filter_string = " -F '%s'" % filter_string
else:
    include_filter_string = ''
if show_completed == 'true':
    include_show_completed = ' -I'
else:
    include_show_completed = ''
if show_only_labels == 'true':
    include_show_only_labels = ' -L'
else:
    include_show_only_labels = ''

# *** OBTAIN THIS SOMEHOW!
#with open('./server_address','r') as f:
#    fzserverpq_addrport = f.read()

def try_command_call(thecmd, print_result=True)->str:
    try:
        #print(thecmd, flush=True)
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
        child_stdin.close()
        result = child_stdout.read()
        error = child_stderr.read()
        child_stdout.close()
        child_stderr.close()
        if print_result:
            print(result)
            return ''
        if len(error)>0:
            print(error)
        #print(result.replace('\n', '<BR>'))
        return result

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        return ''

REDIRECT='''
<html>
<meta http-equiv="Refresh" content="0; url='/formalizer%s'" />
</html>
'''

TEST='''
<html>
<body>
Output was: %s
Click here: <a href="/formalizer/data%s">%s</a>
</body>
</html>
'''

FAILED_UPDATE='''
<html>
<body>
Failed to update web app network.js file.
</body>
</html>
'''

def show_graph():
    thecmd = f"./fzvismilestones -O webapp {include_filter_string} {include_show_completed} {include_show_only_labels} -q" # -o {DEFAULT_OUTPUT_FILE}"

    with open(logfile,'w') as f:
        f.write(thecmd)

    if exists(SUCCESSFUL_OUTPUT_SIGNAL_FILE):
        os.remove(SUCCESSFUL_OUTPUT_SIGNAL_FILE)

    res = try_command_call(thecmd, print_result=False)

    # Check successful update.
    if exists(SUCCESSFUL_OUTPUT_SIGNAL_FILE):
        print(REDIRECT % DEFAULT_REDIRECT_TARGET)
    else:
        print(FAILED_UPDATE)

HELP='''
<html>
<body>
The fzvismilestones-cgi.py script received no recognized form data.
</body>
</html>
'''

def show_help():
    print(HELP)

tmp_always_runs = True

if __name__ == '__main__':

    if tmp_always_runs:
        show_graph()
        sys.exit(0)

    show_help()
    sys.exit(0)
