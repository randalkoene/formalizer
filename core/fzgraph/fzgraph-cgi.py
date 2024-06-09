#!/usr/bin/python3
#
# Randal A. Koene, 20210311
#
# This CGI handler provides processing of the fzgraphhtml generated Node Edit page web form when
# the Node ID is "new" or "NEW" and calls fzgraph to add a new Node.
#
# Also note the 'action=generic' fzgraph call option.
#
# The handler is purposely as similar to the fzedit-cgi.py handler as possible. Note that
# the fzedit-cgi.py handler is also currently able to handle new Node specification, and
# will call fzgraph when the Node ID is "new" or "NEW".

start_CGI_output = '''Content-type:text/html
'''

# This helps spot errors by printing to the browser.
print(start_CGI_output)

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

resultdict = {
    'stdout': '',
    'stderr': '',
}

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

edit_result_page_head = '''<html>
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

edit_success_page_tail = f'''<p class="success"><b>Node added. To review or edit more, follow this link: <a href="/cgi-bin/fzgraphhtml-cgi.py?edit={id}">{id}</a>.</b></p>
<hr>
<button id="closing_countdown" class="button button1" onclick="Keep_or_Close_Page('closing_countdown');">Keep Page</button>
<script type="text/javascript" src="/fzclosing_window.js"></script>
</body>
</html>
'''

edit_fail_page_tail = '''<hr>
</body>
</html>
'''

NNL_edit_result_page_head = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>fz: Graph - Add to Named Node List</title>
</head>
<body>
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>
'''

NNL_edit_success_page_tail = f'''<hr>
<button id="closing_countdown" class="button button1" onclick="Keep_or_Close_Page('closing_countdown');">Keep Page</button>
<script type="text/javascript" src="/fzclosing_window.js"></script>
</body>
</html>
'''

missing_action_request_html = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>fz: Graph - Missing Graph Action Request</title>
</head>
<body>
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>
<p class="fail"><b>ERROR: Missing Graph action request.</b></p>
<hr>
[<a href="/index.html">fz: Top</a>]

</body>
</html>
'''

unrecognized_action_request_page_head = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>fz: Graph - Unrecognized Graph Action Request</title>
</head>
<body>
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>
'''

missing_addtoNNL_arguments = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>fz: Graph - Missing Arguments in Add to NNL Request</title>
</head>
<body>
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>
<p class="fail"><b>ERROR: Missing 'add_id' or 'namedlist' form arguments in 'addtoNNL' Graph action request.</b></p>
<hr>
[<a href="/index.html">fz: Top</a>]

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


def try_call_command(thecmd: str, print_result=True)->bool:
    global resultdict
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        err = child_stderr.read()
        child_stderr.close()
        try:
            resultdict['stdout'] = result.decode()
        except:
            resultdict['stdout'] = '(failed to decode stdout)'
        try:
            resultdict['stderr'] = err.decode()
        except:
            resultdict['stderr'] = '(failed to decode stderr)'
        if print_result:
            print('<!-- begin: call output --><pre>')
            print(result)
            print('<!-- end  : call output --></pre>')
        #print(result.replace('\n', '<BR>'))
        return True

    except Exception as ex:
        if print_result:
            print(ex)
            f = StringIO()
            print_exc(file=f)
            a = f.getvalue().splitlines()
            for line in a:
                print(line)
        else:
            resultdict['stderr'] += str(ex)
            f = StringIO()
            print_exc(file=f)
            a = f.getvalue().splitlines()
            for line in a:
                resultdict['stderr'] += str(line)
        return False

def get_int_or_None(cgiarg: str):
    vstr = form.getvalue(cgiarg)
    if vstr:
        try:
            return int(vstr)
        except:
            return None
    return None

def get_float_or_None(cgiarg: str):
    vstr = form.getvalue(cgiarg)
    if vstr:
        try:
            return float(vstr)
        except:
            return None
    return None

def add_node():
    #print(start_CGI_output) # very useful, because CGI errors are printed from here on if they occur

    text = form.getvalue('text')

    comp = get_float_or_None('comp')
    comp_code = get_float_or_None('comp_code')
    if comp_code:
        comp = comp_code # precedence
    set_complete = form.getvalue('set_complete')

    req_mins_typical = get_int_or_None('req_mins_typical')
    req_hrs = get_float_or_None('req_hrs')
    req_mins = get_int_or_None('req_mins')
    if req_hrs or req_mins:
        # if a specific value was entered that takes precedence
        if not req_mins:
            req_mins = 0
        if req_hrs:
            req_mins += int(60*req_hrs)
    else:
        req_mins = req_mins_typical

    add_hrs = get_float_or_None('add_hrs')
    add_mins = get_int_or_None('add_mins')

    val_typical = get_float_or_None('val_typical')
    val = get_float_or_None('val')
    if not val:
        val = val_typical

    targetdate = form.getvalue('targetdate')
    alt_targetdate = form.getvalue('alt_targetdate')
    alt2_targetdate = form.getvalue('alt2_targetdate')
    alt2_targettime = form.getvalue('alt2_targettime')

    prop = form.getvalue('prop')

    repeats = form.getvalue('repeats')
    if repeats:
        repeats = True
    else:
        repeats = False
    patt = form.getvalue('patt')
    every = get_int_or_None('every')
    span = get_int_or_None('span')

    topics=form.getvalue('topics')

    orig_mins = get_int_or_None('orig_mins')
    if not orig_mins:
        orig_mins = 0
    orig_td = form.getvalue('orig_td')

    if (set_complete=='on'):
        req_mins = int(float(orig_mins)*comp)
        comp = 1.0

    # add_hrs and add_mins are combined
    if add_hrs:
        add_mins += int(add_hrs*60.0)

    if add_mins:
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
        if topics:
            thecmd += " -g '"+topics+"'"
    else:
        print('<p class="fail"><b>Expected "id=NEW" or "id=new".</b></p>')
        print(edit_fail_page_tail)
        return

    print(f'<!-- Call command: {thecmd} -->')

    if try_call_command(thecmd):
        print(edit_success_page_tail)
    else:
        print('<p class="fail"><b>Call to fzgraph returned error. (Check if a Node was created or not.)</b></p>')
        print(edit_fail_page_tail)


def add_to_NNL():
    #print(start_CGI_output) # very useful, because CGI errors are printed from here on if they occur

    add_id = form.getvalue('add')
    namedlist = form.getvalue('namedlist')

    if not add_id or not namedlist:
        print(missing_addtoNNL_arguments)
        return

    thecmd = f"./fzgraph {verbosearg} -E STDOUT -L add -l '{namedlist}' -S '{add_id}'"

    print(NNL_edit_result_page_head)
    thisscript = os.path.realpath(__file__)
    print(f'<!--(For dev reference, this script is at {thisscript}.) -->')
    print('<!-- [Formalizer: fzgraph handler]\n<p></p> -->')
    print(f'<!-- Call command: {thecmd} -->')

    if try_call_command(thecmd):
        print(f"<p class=\"success\"><b>Node {add_id} added to NNL '{namedlist}'.</b></p>")
        print(NNL_edit_success_page_tail)
    else:
        print(f"<p class=\"fail\"><b>Call to fzgraph returned error. (Check if a Node was added to Named Node List '{namedlist}' or not.)</b></p>")
        print(edit_fail_page_tail)


def show_interface_options():
    #print("Content-type:text/html\n\n")
    print(interface_options_help)


def missing_action_request():
    #print("Content-type:text/html\n\n")
    print(missing_action_request_html)


def unrecognized_action_request(action: str):
    #print("Content-type:text/html\n\n")
    print(unrecognized_action_request_page_head)
    print(f'<p class="fail"><b>Unrecognized Node add action: {action}</b><p>')
    print(edit_fail_page_tail)

GENERIC_ERROR_PAGE = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>fz: Graph - Generic Call Failed</title>
</head>
<body>
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>

<b>Attempted command</b>:
<p>
<pre>
%s
</pre>

<p>
<b>Stdout</b>:
<p>
<pre>
%s
</pre>

<p>
<b>Stderr</b>:
<p>
<pre>
%s
</pre>

<hr>
</body>
</html>
'''

GENERIC_SUCCESS_PAGE = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>fz: Graph - Generic Call Succeeded</title>
</head>
<body>
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>

<b>Command</b>:
<p>
<pre>
%s
</pre>

<p>
<b>Stdout</b>:
<p>
<pre>
%s
</pre>

<hr>
</body>
</html>
'''

def generic_fzgraph_call(form):
    # 1. Collect all variables and their values and transform them to fzgraph arguments.
    cgi_keys = list(form.keys())
    argpairs = []
    for key_str in cgi_keys:
        argpairs.append( (key_str, form.getvalue(key_str)) )
    argstr = ''
    for argpair in argpairs:
        arg, argval = argpair
        if arg != 'action':
            if argval=='true':
                argstr += ' -%s' % arg
            else:
                argstr += " -%s '%s'" % (arg, argval)

    # 2. Carry out an fzgraph call.
    thecmd = './fzgraph '+argstr
    if not try_call_command(thecmd, print_result=False):
        print(GENERIC_ERROR_PAGE % (thecmd, resultdict['stdout'], resultdict['stderr']))
        sys.exit(0)

    # 3. Present the result.
    print(GENERIC_SUCCESS_PAGE % (thecmd, resultdict['stdout']))

if __name__ == '__main__':
    if help:
        show_interface_options()
        sys.exit(0)

    action = form.getvalue('action')
    if not action:
        missing_action_request()
        sys.exit(0)

    if (action=='generic'):
        generic_fzgraph_call(form)
        sys.exit(0)

    global verbosearg
    verbosity = form.getvalue('verbosity')
    if (verbosity == "verbose"):
        verbosearg = '-V'
    else:
        verbosearg = '-q'

    if (action=='modify'):
        add_node()
        sys.exit(0)

    if (action=='addtoNNL'):
        add_to_NNL()
        sys.exit(0)

    unrecognized_action_request(action)
    sys.exit(0)
