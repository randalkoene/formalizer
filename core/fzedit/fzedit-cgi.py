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

verbosity = form.getvalue('verbosity')

orig_mins = form.getvalue('orig_mins')
orig_td = form.getvalue('orig_td')

if (orig_mins != req_mins):
    # if the value changed then we assume that req_mins is being used to set required
    req_hrs_float = float(req_mins)/60.0
    req_hrs = '{:.5f}'.format(req_hrs_float)

atd = alt_targetdate.split('T')
atd_date = atd[0].split('-')
atd_time = atd[1].split(':')
atd_YmdHM = f'{atd_date[0]}{atd_date[1]}{atd_date[2]}{atd_time[0]}{atd_time[1]}'
if (atd_YmdHM != orig_td):
    # if the value changed then we assume that atd_YmdHM is being used to set targetdate
    targetdate = atd_YmdHM

textfile = '/var/www/webdata/formalizer/node-text.html'

edit_result_page_head = '''Content-type:text/html

<html>
<head>
<link rel="stylesheet" href="/fz.css">
<title>fz: Edit</title>
</head>
<body>
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>
'''

edit_result_page_tail = f'''<b>Node modified. To review or edit more, follow this link: <a href="/cgi-bin/fzgraphhtml-cgi.py?edit={id}">{id}</a>.</b>
<hr>
</body>
</html>
'''

print(edit_result_page_head)

thisscript = os.path.realpath(__file__)
print(f'<!--(For dev reference, this script is at {thisscript}.) -->')

print("<!-- [Formalizer: fzedit handler]\n<p></p> -->")
#print("<table>")

with open(textfile,'w') as f:
    f.write(text)

if (verbosity == "verbose"):
    thecmd = f"./fzedit -V -E STDOUT -M {id} -f {textfile} -H {req_hrs} -a {val} -t {targetdate} -p {prop} -r {patt} -e {every} -s {span}"
else:
    thecmd = f"./fzedit -q -E STDOUT -M {id} -f {textfile} -H {req_hrs} -a {val} -t {targetdate} -p {prop} -r {patt} -e {every} -s {span}"

print(f'<!-- Call command: {thecmd} -->')

try:
    p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
    (child_stdin,child_stdout) = (p.stdin, p.stdout)
    child_stdin.close()
    result = child_stdout.read()
    child_stdout.close()
    print('<!-- begin: call output --><pre>')
    print(result)
    print('<!-- end  : call output --></pre>')
    #print(result.replace('\n', '<BR>'))

except Exception as ex:                
    print(ex)
    f = StringIO()
    print_exc(file=f)
    a = f.getvalue().splitlines()
    for line in a:
        print(line)

print(edit_result_page_tail)
