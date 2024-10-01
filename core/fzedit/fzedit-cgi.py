#!/usr/bin/python3
#
# Randal A. Koene, 20201209
#
# CGI script to forward Node editing data to fzedit.
#
# This handler is currently also able to handle new Node specification, and
# will call fzgraph when the Node ID is "new" or "NEW". For a very similar
# implementation aimed specifically at adding new Nodes, please see fzgraph-cgi.py.

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

print('Content-type:text/html\n\n');

textfile = '/var/www/webdata/formalizer/node-text.html'

REDIRECT='''
<html>
<meta http-equiv="Refresh" content="0; url='%s'" />
</html>
'''

# The following should only show information that is safe to provide
# to those who have permission to connect to this CGI handler.
interface_options_help = '''
<html>
<head>
<title>fzedit-cgi API</title>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
</head>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>

<h1>fzedit-cgi API</h1>

<p>
Main modes:
<ul>
<li><code>action=modify</code>: Modify Node or Add Node.
<li><code>action=update</code>: Update repeating Node past specified date-time.
<li><code>action=skip</code>: Skip repeating Node N times.
</ul>
</p>

<h3>Modify Node or Add Node</h3>

<p>
<code>fzedit-cgi.py?action=modify</code><br>
Expects an array of parameters:
</p>
<ul>
<li>verbosity: Verbose if "=on".
<li>id: Node ID. If "id=new" or "id=NEW" then add new Node.
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

<h3>Update repeating Node past specified date-time</h3>

<p>
<code>fzedit-cgi.py?action=update</code><br>
Expects the following parameters:
</p>
<ul>
<li>id: Node ID.
<li>tpass: Update repeating Node past date-time in YYYY-mm-ddTHH:MM:SS format.
</ul>

<h3>Skip repeating Node N times</h3>

<p>
<code>fzedit-cgi.py?action=skip</code><br>
Expects the following parameters:
</p>
<ul>
<li>id: Node ID.
<li>num_skip: Skip 'num_skip' instances of repeating Node.
</ul>

</body>
</html>
'''

#testingoutputstart='''Content-type:text/html
#
testingoutputstart='''<html>
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
edge = form.getvalue('edge')
edgemod = form.getvalue('edgemod')
modval = form.getvalue('modval')

#start_CGI_output = '''Content-type:text/html
#''
start_CGI_output = ''

edit_result_page_head = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Edit</title>
</head>
<body onload="do_if_opened_by_script('Keep Page','Go to Topics','/cgi-bin/fzgraphhtml-cgi.py?topics=?');">
<script type="text/javascript" src="/fzuistate.js"></script>
'''

edit_success_page_tail = f'''<p class="success"><b>Node modified. To review or edit more, follow this link: <a href="/cgi-bin/fzgraphhtml-cgi.py?edit={id}">{id}</a>.</b></p>
<hr>
<script type="text/javascript" src="/fzclosing_window.js"></script>
</body>
</html>
'''

create_success_page_tail = '''<p class="success"><b>Node created: <a href="/cgi-bin/fzlink.py?id=%s">%s</a></b><button class="button" onclick="copyValueToClipboard();">copy</button></p>
<hr>
<input id="node_id" type="hidden" value="%s">
<script type="text/javascript" src="/fzclosing_window.js"></script>
<script>
function copyValueToClipboard() {
  var copyValue = document.getElementById('node_id');
  var value_content = '---';
  if (copyValue == null) {
    console.log('Did not find object with id node_id.');
  } else {
    value_content = copyValue.value;
    console.log(`Value of node_id object is ${value_content}.`);
  }
  copyValue.select();
  //copyValue.setSelectionRange(0, 99999); // For mobile devices
  navigator.clipboard.writeText(value_content);
  alert("Copied: " + value_content);
}
</script>
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


def try_call_command(thecmd: str, return_result=False):
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout) = (p.stdin, p.stdout)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        if return_result:
            return result
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

def get_node_id_from_result(result_str:str)->str:
    identifier = 'New Node: '
    id_start = result_str.find(identifier)
    if id_start < 0:
        return ''
    id_len =  len(identifier)
    id_start += id_len
    id_end = result_str.find('\n',id_start)
    if id_end < 0:
        return ''
    return result_str[id_start:id_end]

def modify_node():
    print(start_CGI_output) # very useful, because CGI errors are printed from here on if they occur

    create_new = ((id == 'NEW') or (id == 'new'))

    text = form.getvalue('text')

    comp = get_float_or_None('comp')
    comp_code = get_float_or_None('comp_code')
    if comp_code:
        comp = comp_code # precedence
    set_complete = form.getvalue('set_complete')

    req_mins_typical = get_int_or_None('req_mins_typical')
    req_hrs = get_float_or_None('req_hrs')
    req_mins = get_int_or_None('req_mins')

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

    if create_new: # *** currently, we use a different interpretation in the two cases
        if req_hrs or req_mins:
            # if a specific value was entered that takes precedence
            if create_new: 
                if not req_mins:
                    req_mins = 0
                if req_hrs:
                    req_mins += int(60*req_hrs)
        else:
            req_mins = req_mins_typical
    else:
        if (req_mins_typical == None) or (req_mins_typical == ''):
            if (req_mins == None) and (req_hrs == None):
                req_mins = orig_mins
                req_hrs = orig_mins / 60.0
            else:
                if req_hrs == None:
                    req_hrs = req_mins / 60.0
                if req_mins == None:
                    req_mins = int(60*req_hrs)

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
    print('<!-- [Formalizer: fzedit handler]\n<p></p> -->')
    #print("<table>")

    with open(textfile,'w') as f:
        f.write(text)

    if create_new:
        # topics = form.getvalue('topics')
        # superiors = form.getvalue('superiors')
        # dependencies = form.getvalue('dependencies')
        #thecmd = f'./fzgraph {verbosearg} -E STDOUT -M node -f {textfile} -H {req_hrs:.5f} -a {val:.5f} -t {targetdate} -p {prop} -r {patt} -e {every} -s {span}'
        thecmd = f'./fzgraph -E STDOUT -M node -f {textfile} -H {req_hrs:.5f} -a {val:.5f} -t {targetdate} -p {prop} -r {patt} -e {every} -s {span}'
        if topics:
            thecmd += " -g '"+topics+"'"
        print(f'<!-- Call command: {thecmd} -->')
        result_str = try_call_command(thecmd, return_result=True)
        if isinstance(result_str, bool):
            print('<p class="fail"><b>Call to fzgraph returned error. (Check state of Nodes in database.)</b></p>')
            print(edit_fail_page_tail)
        else:
            node_id = get_node_id_from_result(result_str)
            print(create_success_page_tail % (node_id, node_id, node_id))
    else:
        thecmd = f"./fzedit {verbosearg} -E STDOUT -M {id} -f {textfile} -c {comp:.5f} -H {req_hrs:.5f} -a {val:.5f} -t {targetdate} -p {prop} -r {patt} -e {every} -s {span}"
        print(f'<!-- Call command: {thecmd} -->')
        if try_call_command(thecmd):
            print(edit_success_page_tail)
        else:
            print('<p class="fail"><b>Call to fzedit returned error. (Check state of Nodes in database.)</b></p>')
            print(edit_fail_page_tail)

def update_node():
    print(start_CGI_output) # very useful, because CGI errors are printed from here on if they occur
    tpass = form.getvalue('tpass')
    tpass_YmdHM = convert_to_targetdate(tpass)

    print(edit_result_page_head)
    thisscript = os.path.realpath(__file__)
    print(f'<!--(For dev reference, this script is at {thisscript}.) -->')
    print('<!-- [Formalizer: fzedit handler]\n<p></p> -->')

    thecmd = f"./fzgraph {verbosearg} -E STDOUT -C '/fz/graph/nodes/{id}?skip=toT&T={tpass_YmdHM}'"

    print(f'<!-- Call command: {thecmd} -->')
    
    if try_call_command(thecmd):
        print(f'<p class="success">Skipping all instances of Node {id} past {tpass_YmdHM}.</p>')
        print(edit_success_page_tail)
    else:
        print(f'<p class="fail">Call to `fzgraph -C` returned an error.</p>')
        print(edit_fail_page_tail)


def skip_node():
    print(start_CGI_output) # very useful, because CGI errors are printed from here on if they occur
    num_skip = get_int_or_None('num_skip')
    print(edit_result_page_head)
    if not num_skip:
        print('<p class="fail"><b>Missing number of instances to skip.</b></p>')
        print(edit_fail_page_tail)
        return

    thisscript = os.path.realpath(__file__)
    print(f'<!--(For dev reference, this script is at {thisscript}.) -->')
    print('<!-- [Formalizer: fzedit handler]\n<p></p> -->')

    thecmd = f"./fzgraph {verbosearg} -E STDOUT -C '/fz/graph/nodes/{id}?skip={num_skip}'"

    print(f'<!-- Call command: {thecmd} -->')
    
    if try_call_command(thecmd):
        print(f'<p class="success">Skipping {num_skip} instances of Node {id}.</p>')
        print(edit_success_page_tail)
    else:
        print(f'<p class="fail">Call to `fzgraph -C` returned an error.</p>')
        print(edit_fail_page_tail)


def show_interface_options():
    print(start_CGI_output) # very useful, because CGI errors are printed from here on if they occur
    print(interface_options_help)


fzedit_arg = {
    'dep': 'Y',
    'sig': 'G',
    'imp': 'I',
    'urg': 'U',
    'pri': 'P',
}

def modify_edge_parameters():
    print(edit_result_page_head)

    thecmd = f"./fzedit {verbosearg} -E STDOUT -M '{edge}' -{fzedit_arg[edgemod]} {float(modval):.5f}"

    print(f'<!-- Call command: {thecmd} -->')

    if try_call_command(thecmd):
        print(edit_success_page_tail)
    else:
        print('<p class="fail"><b>Call to fzedit returned error. (Check state of Edges in database.)</b></p>')
        print(edit_fail_page_tail)
    #print('Content-type:text/html\n\n');
    #print('<html><body>')
    #print('Edge: '+str(edge))
    #print('Modtype: '+str(edgemod))
    #print('Value: '+str(modval))
    #print('</body></html>')

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

    if edge:
        modify_edge_parameters()
        sys.exit(0)

    if (action=='modify') or (action=='create'):
        modify_node()
    else:
        if (action=='update'):
            update_node()
        else:
            if (action=='skip'):
                skip_node()
            else:
                print(edit_result_page_head)
                print(f'<p class="fail"><b>Unrecognized Node edit action: {action}</b><p>')
                print(edit_fail_page_tail)

    sys.exit(0)
