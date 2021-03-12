#!/usr/bin/python3
#
# Randal A. Koene, 20210311
#
# This CGI handler provides processing of the fzgraphhtml generated Node Edit page web form when
# the Node ID is "new" or "NEW" and calls fzgraph to add a new Node.
#
# The handler is purposely as similar to the fzedit-cgi.py handler as possible. Note that
# the fzedit-cgi.py handler is also currently able to handle new Node specification, and
# will call fzgraph when the Node ID is "new" or "NEW".

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

textfile = '/var/www/webdata/formalizer/node-text.html'

# The following should only show information that is safe to provide
# to those who have permission to connect to this CGI handler.
interface_options_help = '''
<html>
<link rel="stylesheet" href="/fz.css">
<head>
<title>fzgraph-cgi API</title>
</head>
<body>
<h1>fzgraph-cgi API</h1>

<p>
Main modes:
<ul>
<li><code>action=modify</code>: Add Node.
</ul>
</p>

<h3>Add Node</h3>

<p>
<code>fzedit-cgi.py?action=modify&id=NEW</code><br>
The value of 'id' can be either "id=NEW" or "id=new". Expects an array of parameters:
</p>
<ul>
<li>verbosity: Verbose if "=on".
<li>text: Node description text.
<li>comp: Node completion value.
<li>set_complete: if "=on" then set Node completion to 1.0 and adjust required to match.
<li>req_hrs: Node required time, expressed in hours.
<li>req_mins: Node required time, expressed in minutes.
<li>add_hrs: Add hours to Node required time.
<li>add_mins: Add minutes to Node required time.
<li>val: Node valuation.
<li>targetdate: Node target date in YYYYmmddHHMM format.
<li>alt_targetdate: Node target date in YYYY-mm-ddTHH:MM:SS format.
<li>alt2_targetdate: Node target date calendar date in YYYY-mm-dd format.
<li>alt2_targettime: Node target date clock time in HH:MM:SS format.
<li>prop: Node target date property ("=unspecified", "=variable", "=inherit", "=fixed", "=exact").
<li>patt: Node target date pattern ("=daily", "=workdays", "=weekly", "=biweekly", "=monthly", "=endofmonth", "=yearly").
<li>every: Repeating Node every K instances of pattern.
<li>span: Repeating Node with N instances ("=0" means indefinite).

<li>orig_mins: Original Node minutes required. Used with 'add_mins', 'add_hrs' and 'set_complete'.
<li>orig_td: Used to determine whether 'targetdate' or 'alt_targetdate' should be applied if both are given.
</ul>

</body>
</html>
'''

testingoutputstart='''Content-type:text/html

<html>
'''

testingoutputend='''Testing
</html>
'''

def cgi_testing_start():
    print(testingoutputstart)

def cgi_testing_end():
    print(testingoutputend)
    sys.exit(0)


# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

help = form.getvalue('help')
id = form.getvalue('id')

edit_result_page_head = '''Content-type:text/html

<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>fz: Graph - Add Node</title>
</head>
<body>
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>
'''

edit_success_page_tail = f'''<b>Node added. To review or edit more, follow this link: <a href="/cgi-bin/fzgraphhtml-cgi.py?edit={id}">{id}</a>.</b>
<hr>
</body>
</html>
'''

edit_fail_page_tail = '''<hr>
</body>
</html>
'''

def convert_to_targetdate(alttargetdate: str):
    if (len(alttargetdate)<16):
        return ''
    atd = alttargetdate.split('T')
    atd_date = atd[0].split('-')
    atd_time = atd[1].split(':')
    atd_YmdHM = f'{atd_date[0]}{atd_date[1]}{atd_date[2]}{atd_time[0]}{atd_time[1]}'
    return atd_YmdHM


def convert_date_and_time_to_targetdate(alt2_targetdate: str, alt2_targettime: str):
    if ((len(alt2_targetdate)<10) or (len(alt2_targettime)<5)):
        return ''
    atd_date = alt2_targetdate.split('-')
    atd_time = alt2_targettime.split(':')
    atd_YmdHM = f'{atd_date[0]}{atd_date[1]}{atd_date[2]}{atd_time[0]}{atd_time[1]}'
    return atd_YmdHM


def try_call_command(thecmd: str):
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
        return True

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        return False


def add_node():
    text = form.getvalue('text')
    comp = '0.0'
    req_hrs = float(form.getvalue('req_hrs'))
    req_mins = int(form.getvalue('req_mins'))
    add_hrs = float(form.getvalue('add_hrs'))
    add_mins = int(form.getvalue('add_mins'))
    val = float(form.getvalue('val'))
    targetdate = form.getvalue('targetdate')
    alt_targetdate = form.getvalue('alt_targetdate')
    alt2_targetdate = form.getvalue('alt2_targetdate')
    alt2_targettime = form.getvalue('alt2_targettime')
    prop = form.getvalue('prop')
    patt = form.getvalue('patt')
    every = int(form.getvalue('every'))
    span = int(form.getvalue('span'))

    orig_mins = int(form.getvalue('orig_mins'))
    orig_td = form.getvalue('orig_td')

    # add_hrs and add_mins are combined
    if (add_hrs != 0):
        add_mins += int(add_hrs*60.0)

    if (add_mins != 0):
        if (comp >= 0.0):
            completed_mins = int(float(orig_mins)*comp)
        req_mins = orig_mins + add_mins
        if (req_mins<0):
            req_mins = 0
        if (comp >= 0.0):
            if (completed_mins >= req_mins):
                comp = 1.0
            else:
                comp = float(completed_mins)/float(req_mins)
        
    if (orig_mins != req_mins):
        # if the value changed then we assume that req_mins is being used to set required
        req_hrs = float(req_mins)/60.0
        #req_hrs = '{:.5f}'.format(req_hrs_float)

    atd_YmdHM = convert_to_targetdate(alt_targetdate)
    if (atd_YmdHM != orig_td):
        # if the value changed then we assume that atd_YmdHM is being used to set targetdate
        targetdate = atd_YmdHM
    atd_YmdHM = convert_date_and_time_to_targetdate(alt2_targetdate,alt2_targettime)
    if (atd_YmdHM != orig_td):
        # if the value changed then we assume that atd_YmdHM is being used to set targetdate
        targetdate = atd_YmdHM

    print(edit_result_page_head)
    thisscript = os.path.realpath(__file__)
    print(f'<!--(For dev reference, this script is at {thisscript}.) -->')
    print('<!-- [Formalizer: fzgraph handler]\n<p></p> -->')
    #print("<table>")

    with open(textfile,'w') as f:
        f.write(text)

    if ((id == 'NEW') or (id == 'new')):
        # topics = form.getvalue('topics')
        # superiors = form.getvalue('superiors')
        # dependencies = form.getvalue('dependencies')
        thecmd = f'./fzgraph {verbosearg} -E STDOUT -M node -f {textfile} -H {req_hrs:.5f} -a {val:.5f} -t {targetdate} -p {prop} -r {patt} -e {every} -s {span}'
    else:
        print('<b>Expected "id=NEW" or "id=new".</b>')
        print(edit_fail_page_tail)
        return

    print(f'<!-- Call command: {thecmd} -->')

    if try_call_command(thecmd):
        print(edit_success_page_tail)
    else:
        print(edit_fail_page_tail)


def show_interface_options():
    print("Content-type:text/html\n\n")
    print(interface_options_help)


if __name__ == '__main__':
    if help:
        show_interface_options()
        sys.exit(0)

    global verbosearg
    action = form.getvalue('action')
    verbosity = form.getvalue('verbosity')
    if (verbosity == "verbose"):
        verbosearg = '-V'
    else:
        verbosearg = '-q'

    if (action=='modify'):
        add_node()
    else:
        print(edit_result_page_head)
        print(f'<p><b>Unrecognized Node add action: {action}</b><p>')
        print(edit_fail_page_tail)

    sys.exit(0)
