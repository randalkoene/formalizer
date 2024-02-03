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

HELP='''
<h1>fzupdate-cgi API</h1>

<p>
Main modes:
<ul>
<li><code>help=true</code>: This Help page.
<li><code>update=breakeps</code>: Break up an EPS group with a specified target date. (The T_pass option is required for this mode.)
<li><code>update=repeating</code>: Update repeating Nodes.
<li><code>update=variable</code>: Update Nodes with variable or unspecified target dates.
<li><code>update=both</code>: Update both repeating and variable/unspecified target date Nodes.
<li><code>update=passedfixed</code>: Prepare an NNL with passed fixed/exact target date Nodes for conversion.
<li><code>update=convert_passedfixed</code>: Convert Nodes in passed_fixed NNL to variable target date Nodes.
</ul>
</p>

Options:
<ul>
<li><code>T_emulate=YYYYmmddHHMM</code>: Use emulated time instead of actual current time.
<li><code>map_days=NUM</code>: Do updates with a map of size NUM days. Default: 14 days.
<li><code>T_pass=YYYYmmddHHMM</code>: Update up to and including this date. This argument is also used as the specified target date when breaking up an EPS group.
<li><code>verbose=true</code>: Be verbose.
</ul>

<h3>Breaking up an EPS group</h3>

<p>
The Earliest Possible Scheduling (EPS) method keeps groups of Nodes with identical variable target dates together, assuming that they are intended to be treated as a group.
Breaking up an EPS group changes the variable target dates of the Nodes in such a group and converts them to successive target dates, so that
updates will modify and allocate the time needed for the Nodes independently.
</p>

<p>
A link to carry out EPS group breaking is included in the Edit page of each Node.
</p>

<h3>Updating repeating Nodes</h3>

<p>
If passed, the target dates of Nodes that have fixed or exact repeating target dates are updated to their next not-passed occurrence.
</p>

<h3>Updating Nodes with variable or unspecified target dates</h3>

<p>
Target dates are updated to indicate by when these Nodes can be completed by filling available time (not already filled by exact and
fixed target date Nodes) starting at current or emulated time. The order of Nodes with variable or unspecified target dates is not
changed. EPS groups are updated together so that they remain a group with identical target dates.
</p>

<p>
<b>fzupdate/config.json dolater_endofday and doearlier_endofday</b>: These are two reference end-of-day times by which work on variable
target date Nodes should be completed. Which reference time to use in adjusting proposed updated target dates should be determined by
a Node parameter. In the Formalizer 1.x this was based on the 'urgency' parameter, which is now Edge specific, not Node specific.
At present, only the dolater_endofday setting is used. (<b>This is an area of pending improvement, and using a boolean flag tag would
make sense.</b>) These time of day limits are adjusted in accordance with the timezone_offset_hours configuration parameter.
</p>

<p>
<b>fzupdate/config.json timezone_offset_hours</b>: Apply this offset to the update. This is in essence like applying an emulated time that
is the current time plus the offset. This offset is shown and managed through fztimezone.
</p>

<h3>Preparing an NNL with passed fixed/exact target date Nodes for conversion</h3>

<p>
A NNL is filled with Nodes that have non-repeating fixed or exact target dates that have been passed according to the current or
emulated time. This NNL is used for inspection and manual updates as well as automated conversion to variable target dates.
</p>

<p>
A link to call this NNL preparation mode is in the DayWiz page.
</p>

'''

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
showhelp = form.getvalue('help')


pagetail = '''<hr>
</body>
</html>
'''

show_passedfixed_steps = """<p>(Actually, you should just list the NNL here with fzgraphhtml.)</p>
<p>Manually update Nodes that should not be converted: <a href="/cgi-bin/fzgraphhtml-cgi.py?srclist=passed_fixed">passed_fixed NNL</a></p>
<p><b>Reload</b> to see how many remain.</p>
<p><a href="/cgi-bin/fzupdate-cgi.py?update=convert_passedfixed">Update the rest</a>.</p>
"""


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
    # Note that Graphaccess uses the subprocess call in fzcmdcalls.py, which does not include a <pre> block.
    # Add this here if needed:
    if config['verbose']:
        print('<div class="map"><pre>')
    if not Graphaccess.clear_NNL('passed_fixed', config):
        if config['verbose']:
            print('</pre></div>')
        return -1
    if (filterstr == ''):
        filterstr = make_filter_passed_fixed()
    num = Graphaccess.select_to_NNL(filterstr,'passed_fixed')
    if (num < 0):
        if config['verbose']:
            print('</pre></div>')
        return -2
    if config['verbose']:
        print('</pre></div>')
    return num


# This is based on the process carred out in fztask.py:update_passed_fixed().
def prepare_convert_passed_fixed():
    get_server_address('.')
    num = get_passed_fixed()
    print(f"<p>Number of passed Fixed or Exact Target Date Nodes: {num}</p>")
    print(show_passedfixed_steps)


def convert_passed_fixed():
    get_server_address('.')
    num_fixed_converted = Graphaccess.edit_nodes_in_NNL('passed_fixed','tdproperty','variable')
    if (num_fixed_converted > 0):
        print(f'<p><b>Converted {num_fixed_converted} Fixed or Exact Target Date Nodes to Variable Target Date Nodes.</b></p>')

def show_help():
    print(HELP)
    print(pagetail)

if __name__ == '__main__':

    thisscript = os.path.realpath(__file__)
    print(f'<!--(For dev reference, this script is at {thisscript}.) -->')
    print("<!-- [Formalizer: fzupdate handler]\n<p></p> -->")

    if showhelp:
        show_help()
        exit(0)

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
