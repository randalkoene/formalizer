#!/usr/bin/python3
#
# Randal A. Koene, 20201209
#
# CGI script to forward Node editing data to fzedit.

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
form = cgi.FieldStorage() 

# Get data from fields
id = form.getvalue('id')
text = form.getvalue('text')
comp = form.getvalue('comp')
req_hrs = form.getvalue('req_hrs')
req_mins = form.getvalue('req_mins')
val = form.getvalue('val')
targetdate = form.getvalue('targetdate')
alt_targetdate = form.getvalue('alt_targetdate')
prop = form.getvalue('prop')
patt = form.getvalue('patt')
every = form.getvalue('every')
span = form.getvalue('span')

textfile = '/var/www/webdata/formalizer/node-text.html'

print("Content-type:text/html\n\n")
print("<html>")
print("<head>")
print('<link rel="stylesheet" href="/fz.css">')
print("<title>fz: Edit</title>")
print("</head>")
print("<body>")
print('<style type="text/css">')
print('.chktop { ')
print('    background-color: #B0C4F5;')
print('}')
#print("table tr.chktop { background: #B0C4F5; }")
print("</style>")


thisscript = os.path.realpath(__file__)
print(f'<!--(For dev reference, this script is at {thisscript}.) -->')

print("<!-- [Formalizer: fzedit handler]\n<p></p> -->")
#print("<table>")

with open(textfile,'w') as f:
    f.write(text)

thecmd = f"./fzedit -q -E STDOUT -M {id} -f {textfile} -H {req_hrs} -a {val} -t {targetdate} -p {prop} -r {patt} -e {every} -s {span}"

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

print('<b>This still needs to be changed to somehow modify only what is being changed.</b>')

print("<hr>\n</body>\n</html>")
