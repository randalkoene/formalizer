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
#repeating = form.getvalue('repeating')
#variable = form.getvalue('variable')
update = form.getvalue('update')
T_emulate = form.getvalue('T_emulate')
map_days = form.getvalue('map_days')
verbose = form.getvalue('verbose')
T_pass = form.getvalue('T_pass')


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
    # Let's comment out the following for now to get some feedback
    # else:
    #     add_to_cmd += ' -q'

    thecmd = ''
    if ((update=="breakeps") and T_pass):
        print(f'<p>Breaking EPS group with target date {T_pass}.</p>')
        thecmd = "./fzupdate -E STDOUT -b -T "+T_pass
        try_command_call(thecmd)
        print('<p><b>To see the new target dates assigned, see this <a href="/cgi-bin/fzgraphhtml-cgi.py?num_elements=256&all=on&max_td=202106242138&num_days=&norepeats=on">Schedule of Nodes</a>.</b></p>')

    if ((update=='repeating') or (update=='both')):
        print('<p>Updating repeating Nodes.</p>')
        thecmd = "./fzupdate -E STDOUT -r"+add_to_cmd
        try_command_call(thecmd)
        print('<p><b>To see which Nodes were modified, see the <a href="/cgi-bin/fzgraphhtml-cgi.py?srclist=repeating_updated">repeating_updated</a> Named Node List.</b></p>')

    if ((update=='variable') or (update=='both')):
        if map_days:
            add_to_cmd += ' -D '+map_days
        print('<p>Updating variable and unspecified target date Nodes.</p>')
        thecmd = "./fzupdate -E STDOUT -u"+add_to_cmd
        try_command_call(thecmd)
        print('<p><b>To see which Nodes were modified, see the <a href="/cgi-bin/fzgraphhtml-cgi.py?srclist=batch_updated">batch_updated</a> Named Node List.</b></p>')

    if (len(thecmd)==0):
        print('<p><b>Unrecognized update request.</b></p>')

    print(pagetail)

    sys.exit(0)
