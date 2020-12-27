#!/usr/bin/python3
#
# Randal A. Koene, 20200921
#
# This CGI handler provides a near-verbatim equivalent access to fzloghtml via web form.

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
startfrom = form.getvalue('startfrom')
endbefore  = form.getvalue('endbefore')
daysinterval  = form.getvalue('daysinterval')
weeksinterval  = form.getvalue('weeksinterval')
hoursinterval  = form.getvalue('hoursinterval')
numchunks = form.getvalue('numchunks')
node = form.getvalue('node')
frommostrecent = form.getvalue('frommostrecent')
mostrecentdata = form.getvalue('mostrecentdata')

print("Content-type:text/html\n\n")

if mostrecentdata:
    thecmd = "./fzloghtml -q -d formalizer -s randalk -o STDOUT -E STDOUT -R"
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

    sys.exit(0)


print("<html>")
print("<head>")
#print('<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">')
print('<link rel="stylesheet" href="/fz.css">')
print("<title>Formalizer: HTML FORM interface to fzloghtml</title>")
print("</head>")
print("<body>")
print('<style type="text/css">')
print('.chktop { ')
print('    background-color: #B0C4F5;')
print('}')
#print("table tr.chktop { background: #B0C4F5; }")
print("</style>")

thisscript = os.path.realpath(__file__)
print(f'<!-- (For dev reference, this script is at {thisscript}.) -->')

print("<h1>Formalizer: HTML FORM interface to fzloghtml</h1>\n<p></p>")
print("<table><tbody>")

cmdoptions = ""

if startfrom:
    cmdoptions += ' -1 '+startfrom
if endbefore:
    cmdoptions += ' -2 '+endbefore
if daysinterval:
    cmdoptions += ' -D '+daysinterval
if weeksinterval:
    cmdoptions += ' -w '+weeksinterval
if hoursinterval:
    cmdoptions += ' -H '+hoursinterval
if numchunks:
    cmdoptions += ' -c '+numchunks
if frommostrecent:
    cmdoptions += ' -r '
if node:
    cmdoptions += ' -n '+node

if cmdoptions:
    thecmd = "./fzloghtml -q -d formalizer -s randalk -o STDOUT -E STDOUT -N "+cmdoptions
    print(f'<!-- Using this command: {thecmd} -->')
    print('<br>\n')
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

print("</tbody></table>")
print('<a name="END">&nbsp;</a>')

cgi_custom_tail = '''
<hr>

<form action="/cgi-bin/fzloghtml-cgi.py" method="post">
<input type="text" name="startfrom"> start at this YYYYmmddHHMM time stamp (default: 24 hours before end of interval)<br />
<input type="text" name="endbefore"> end before this YYYYmmddHHMM time stamp (default: 1 second after most recent Log entry)<br />
<input type="text" name="daysinterval"> interval size in days<br />
<input type="text" name="weeksinterval"> interval size in weeks<br />
<input type="text" name="hoursinterval"> interval size in hours<br />
<input type="text" name="numchunks"> number of Log chunks (interval takes precedence)<br />
<input type="checkbox" name="frommostrecent"> from most recent (interval takes precedence)<br />
<input type="text" name="node"> belongs to node, node history<br />
or<br />
<input type="checkbox" name="mostrecentdata"> show most recent Log data summary<br />

<input type="submit" value="Submit" />

<hr>

<p>[<a href="/index.html">fz: Top</a>]</p>

'''

print(cgi_custom_tail)
print("</body>")
print("</html>")
