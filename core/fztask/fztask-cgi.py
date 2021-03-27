#!/usr/bin/python3
#
# Randal A. Koene, 20201125
#
# This CGI handler provides a web interface to fztask.py.

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
from subprocess import Popen, PIPE

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

non_local = form.getvalue('n')

cmdoptions = ""

# Get data from fields
T_emulated = form.getvalue('T')
if (T_emulated == '') or (T_emulated == 'actual'):
    fzlog_T = ''
else:
    fzlog_T = '&T='+T_emulated


fztask_webpage = f"""Content-type:text/html

<html>
<link rel="stylesheet" href="/fz.css">
<head>
<title>fz: Task</title>
</head>
<body>
<h1>fz: Task</h1>

<p>T = {T_emulated}</p>

<ol>
<li>[<a href="/formalizer/logentry-form_fullpage.template.html" target="_blank">Make Log entry</a>]</li>
<li>[<a href="/cgi-bin/fzlog-cgi.py?action=close{fzlog_T}" target="_blank">Close Log chunk</a>]</li>
<li>[<a href="cgi-bin/fzgraphhtml-cgi.py" target="_blank">Update Schedule</a>]</li>
<li>[<a href="/select.html" target="_blank">Select Node for Next Log chunk</a>]</li>
<li>[<a href="/cgi-bin/fzlog-cgi.py?action=open{fzlog_T}" target="_blank">Open New Log chunk</a>]</li>
</ol>

<hr>
[<a href="/index.html">fz: Top</a>]

</body>
</html>
"""

if (non_local == 'on'):
    print(fztask_webpage)
    sys.exit(0)

#thecmd = "./fztask"+cmdoptions
#nohup env -u QUERY_STRING urxvt -rv -title "dil2al daemon" -geometry +$xhloc+$xvloc -fade 30 -e dil2al -T$emulatedtime -S &
thecmd = "nohup urxvt -rv -title 'fztask' -fn 'xft:Ubuntu Mono:pixelsize=14' -bd red -e fztask"+cmdoptions+" &"

# Let's delete QUERY_STRING from the environment now so that dil2al does not accidentally think it was
# called from a form if dil2al is called for synchronization back to Formalizer 1.x.
del os.environ['QUERY_STRING']


print("Content-type:text/html\n\n")

print("<html>")
print("<head>")
print("<title>fztask-cgi.py</title>")
print("</head>")
print("<body>")
print(f'\n<!-- Primary command: {thecmd} -->\n')
try:
    p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
    (child_stdin,child_stdout) = (p.stdin, p.stdout)
    child_stdin.close()
    result = child_stdout.read()
    child_stdout.close()
    print(result)
    print('fztask call completed.')

except Exception as ex:                
    print(ex)
    f = StringIO()
    print_exc(file=f)
    a = f.getvalue().splitlines()
    for line in a:
        print(line)

print('</body></html>')
sys.exit(0)
