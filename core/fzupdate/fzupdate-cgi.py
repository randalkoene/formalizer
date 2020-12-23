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
verbose = form.getvalue('verbose')


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
.map {
    font-family: "Lucida Sans Typewriter";
    font-size: 16px;
    font-style: normal;
    font-variant: normal;
    font-weight: 400;
    line-height: 17.6px;
    background-color: #e6e6fa;
}
</style>
'''

pagetail = '''<hr>
</body>
</html>
'''


def try_command_call(thecmd):
    print(f'<!-- thecmd = {thecmd} -->')
    print('<div class="map"><pre>')
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

    print('</pre></div>')


if __name__ == '__main__':

    print(pagehead)

    thisscript = os.path.realpath(__file__)
    print(f'<!--(For dev reference, this script is at {thisscript}.) -->')

    print("<!-- [Formalizer: fzupdate handler]\n<p></p> -->")
    #print("<table>")

    add_to_cmd = ''
    if T_emulate:
        add_to_cmd = ' -t '+T_emulate
    if verbose:
        add_to_cmd += ' -V'

    if repeating:
        thecmd = "./fzupdate -q -E STDOUT -r"+add_to_cmd
        try_command_call(thecmd)
        print('<p><b>Repeating Nodes updated. To see which Nodes were modified, see the <a href="/cgi-bin/fzgraphhtml-cgi.py?srclist=repeating_updated">repeating_updated</a> Named Node List.</b></p>')

    if variable:
        thecmd = "./fzupdate -q -E STDOUT -u"+add_to_cmd
        try_command_call(thecmd)
        print('<p><b>Variable or unspecified target date Nodes updated. To see which Nodes were modified, see the <a href="/cgi-bin/fzgraphhtml-cgi.py?srclist=batch_updated">batch_updated</a> Named Node List.</b></p>')

    print(pagetail)

    sys.exit(0)
