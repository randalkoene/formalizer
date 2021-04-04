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
verbositymap = {
    0 : "quiet",
    1 : "normal",
    2 : "very verbose"
}
verbositycmdmap = {
    0 : " -q",
    1 : "",
    2 : " -V"
}
verbosity = 1

pagehead = '''Content-type:text/html

<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Refresh Node Histories</title>
</head>
<body>
<h3>fz: Refresh Node Histories</h3>

'''

pagetail = '''
<hr>

<p>[<a href="/index.html">fz: Top</a>]</p>

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

print(pagehead)

thisscript = os.path.realpath(__file__)
print(f'<!-- (For dev reference, this script is at {thisscript}.) -->')

#cmdoptions = ""

#if startfrom:
#    cmdoptions += ' -1 '+startfrom

#if cmdoptions:
print(f'Verbosity is set to: {verbositymap[verbosity]}')

thecmd = "./fzquerypq -d formalizer -s randalk -E STDOUT -R histories" + verbositycmdmap[verbosity]

print(f'<!-- Using this command: {thecmd} -->')
#print('<br>\n')

print('<pre>')

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

print('</pre>')
print(pagetail)
