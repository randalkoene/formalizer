#!/usr/bin/python3
#
# Randal A. Koene, 20210304
#
# This CGI handler provides access to specific fzquerypq operations.

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

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
#form = cgi.FieldStorage() 

# Get data from fields
#startfrom = form.getvalue('startfrom')

print("Content-type:text/html\n\n")

#thisscript = os.path.realpath(__file__)
#print(f'(For dev reference, this script is at {thisscript}.)')

#cmdoptions = ""

#if startfrom:
#    cmdoptions += ' -1 '+startfrom

#if cmdoptions:

thecmd = "./fzquerypq -q -d formalizer -s randalk -E STDOUT -R histories"

#print('Using this command: ',thecmd)
#print('<br>\n')

try:
    p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
    (child_stdin,child_stdout) = (p.stdin, p.stdout)
    child_stdin.close()
    result = child_stdout.read()
    child_stdout.close()
    print(result)
    #print(result.replace('\n', '<BR>'))

except Exception as ex:                
    print(ex)
    f = StringIO()
    print_exc(file=f)
    a = f.getvalue().splitlines()
    for line in a:
        print(line)

#if "name" not in form or "addr" not in form:
#    print("<H1>Error</H1>")
#    print("Please fill in the name and addr fields.")
#    return

#print("</table>")
#print("</body>")
#print("</html>")
