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

cmdoptions = ""

# Get data from fields
T_emulated = form.getvalue('T')
if T_emulated:
    cmdoptions += f" -T {T_emulated}"

#thecmd = "./fztask"+cmdoptions
#nohup env -u QUERY_STRING urxvt -rv -title "dil2al daemon" -geometry +$xhloc+$xvloc -fade 30 -e dil2al -T$emulatedtime -S &
thecmd = "nohup urxvt -rv -title 'fztask' -e fztask"+cmdoptions+" &"

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
