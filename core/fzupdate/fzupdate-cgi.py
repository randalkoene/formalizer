#!/usr/bin/python3
#
# Randal A. Koene, 20201209
#
# CGI script to call fzupdate.

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
repeating = form.getvalue('repeating')
variable = form.getvalue('variable')
T_emulate = form.getvalue('T_emulate')


pagehead = '''Content-type:text/html

<html>
<head>
<link rel="stylesheet" href="/fz.css">
<title>fz: Update</title>
</head>
<body>
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>
'''

pagetail = '''<hr>
</body>
</html>
'''


def try_command_call(thecmd):
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


if __name__ == '__main__':

    print(pagehead)

    thisscript = os.path.realpath(__file__)
    print(f'<!--(For dev reference, this script is at {thisscript}.) -->')

    print("<!-- [Formalizer: fzupdate handler]\n<p></p> -->")
    #print("<table>")

    if repeating:
        thecmd = "./fzupdate -q -E STDOUT -r"
        try_command_call(thecmd)

    if variable:
        thecmd = "./fzupdate -q -E STDOUT -u"
        try_command_call(thecmd)

    print(pagetail)

    sys.exit(0)
