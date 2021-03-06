#!/usr/bin/python3
#
# Randal A. Koene, 20200902

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
node_id = form.getvalue('node_id')
logchunk_id  = form.getvalue('logchunk_id')

print("Content-type:text/html\n\n")
print("<html>")
print("<head>")
print('<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">')
print("<title>Test Prototype: Request a Node and Log Chunk</title>")
print("</head>")
print("<body>")

thisscript = os.path.realpath(__file__)
print(f'(For dev reference, this script is at {thisscript}.)')

print("<h1>Test Prototype: Request a Node and Log Chunk</h1>\n<p></p>")
print("<h2>Node %s and Log Chunk %s</h2>" % (node_id, logchunk_id))
print("<table>")

if node_id:
    thecmd = "./fzquerypq -q -d formalizer -s randalk -n "+node_id+" -E STDOUT -F html"
    print('Using this command: ',thecmd)
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

if logchunk_id:
    thecmd = "./fzloghtml -q -d formalizer -s randalk -1 "+logchunk_id+" -D 1 -E STDOUT"
    print('Using this command: ',thecmd)
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

print("</table>")
print("</body>")
print("</html>")
