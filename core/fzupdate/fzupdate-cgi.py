#!/usr/bin/python3
#
# Randal A. Koene, 20201209
#
# CGI script to call fzupdate.

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

print(pagehead)

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

from fzmodbase import *
from tcpclient import get_server_address
import Graphaccess

Graphaccess.fzmodulebasedir='./' # for use from CGI script

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


pagetail = '''<hr>
</body>
</html>
'''

show_passedfixed_steps = ("""<p>Number of passed Fixed or Exact Target Date Nodes: ???</p>
<p>(Actually, you should just list the NNL here with fzgraphhtml.)</p>
<p>Manually update Nodes that should not be converted: <a href="/cgi-bin/fzgraphhtml-cgi.py?srclist=passed_fixed">passed_fixed NNL</a></p>
<p><b>Reload</b> to see how many remain.</p>
<p><a href="/cgi-bin/fzupdate-cgi.py?update=convert_passedfixed">Update the rest</a>.</p>
""")


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


def make_filter_passed_fixed():
    completionfilter = 'completion=[0.0-0.999]'
    hoursfilter = 'hours=[0.001-1000.0]'
    if T_emulate:
        targetdatesfilter = f'targetdate=[MIN-{T_emulate}]'
    else:
        targetdatesfilter = 'targetdate=[MIN-NOW]'
    tdpropertiesfilter = 'tdproperty=[fixed-exact]'
    return f'{completionfilter},{hoursfilter},{targetdatesfilter},{tdpropertiesfilter},repeats=false'


filterstr = ''
def get_passed_fixed() ->int:
    global filterstr
    config = {}
    config['verbose'] = True
    config['logcmdcalls'] = False
    config['logcmderrors'] = False
    if not Graphaccess.clear_NNL('passed_fixed', config):
        return -1
    if (filterstr == ''):
        filterstr = make_filter_passed_fixed()
    num = Graphaccess.select_to_NNL(filterstr,'passed_fixed')
    if (num < 0):
        return -2
    return num


# This is based on the process carred out in fztask.py:update_passed_fixed().
def prepare_convert_passed_fixed():
    get_server_address('.')
    num = get_passed_fixed()
    print(show_passedfixed_steps)


def convert_passed_fixed():
    get_server_address('.')
    num_fixed_converted = Graphaccess.edit_nodes_in_NNL('passed_fixed','tdproperty','variable')
    if (num_fixed_converted > 0):
        print(f'<p><b>Converted {num_fixed_converted} Fixed or Exact Target Date Nodes to Variable Target Date Nodes.</b></p>')


if __name__ == '__main__':

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

    if (update=='passedfixed'):
        prepare_convert_passed_fixed()
        thecmd="passedfixed"
    
    if (update=='convert_passedfixed'):
        convert_passed_fixed()
        thecmd="convert_passedfixed"

    if (len(thecmd)==0):
        print('<p><b>Unrecognized update request.</b></p>')

    print(pagetail)

    sys.exit(0)
