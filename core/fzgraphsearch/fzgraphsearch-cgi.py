#!/usr/bin/python3
#
# Randal A. Koene, 20210227
#
# This CGI handler provides web form access to fzgraphsearch.

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

searchresultsNNL = 'fzgraphsearch_cgi'

# Create instance of FieldStorage 
form = cgi.FieldStorage()

# Get data from fields
verbose = form.getvalue('verbose')
searchstring = form.getvalue('searchstring')
case_sensitive = form.getvalue('case_sensitive')
t_created_from = form.getvalue('t_created_from')
t_created_through = form.getvalue('t_created_through')
created_today = form.getvalue('today')
if created_today:
    from datetime import date
    t_created_from = date.today().strftime('%Y%m%d')
    t_created_through = t_created_from

completion_lower = form.getvalue('completion_lower')
completion_upper = form.getvalue('completion_upper')
hours_lower = form.getvalue('hours_lower')
hours_upper = form.getvalue('hours_upper')
TD_lower = form.getvalue('TD_lower')
TD_upper = form.getvalue('TD_upper')
tdprop_lower = form.getvalue('tdprop_lower')
tdprop_upper = form.getvalue('tdprop_upper')
repeats = form.getvalue('repeats')
tdpatt_lower = form.getvalue('tdpatt_lower')
tdpatt_upper = form.getvalue('tdpatt_upper')

tdbinpatt_unspecified = form.getvalue('tdbinpatt_unspecified')
tdbinpatt_inherit = form.getvalue('tdbinpatt_inherit')
tdbinpatt_variable = form.getvalue('tdbinpatt_variable')
tdbinpatt_fixed = form.getvalue('tdbinpatt_fixed')
tdbinpatt_exact = form.getvalue('tdbinpatt_exact')

sup_self = form.getvalue('sup_self')
sup_none = form.getvalue('sup_none')
sup_min = form.getvalue('sup_min')

subtree = form.getvalue('subtree')
nnltree = form.getvalue('nnltree')

btf = form.getvalue('btf')
btf_nnl = form.getvalue('btf_nnl')

excerpt_size = form.getvalue('excerpt_size')
countby = form.getvalue('countby')

draggable = form.getvalue('draggable')

def append_to_str(strval: str) -> str:
    parval = eval(strval)
    if parval:
        return strval+'='+parval+'&' # you should NOT put double quotes around this, because a GET URL string does not want those (for more see url_encode() in fzdashboard:render.cpp)
    else:
        return ''

search_recreate_str = append_to_str('searchstring')
search_recreate_str += append_to_str('case_sensitive')
search_recreate_str += append_to_str('t_created_from')
search_recreate_str += append_to_str('t_created_through')
search_recreate_str += append_to_str('completion_lower')
search_recreate_str += append_to_str('completion_upper')
search_recreate_str += append_to_str('hours_lower')
search_recreate_str += append_to_str('hours_upper')
search_recreate_str += append_to_str('TD_lower')
search_recreate_str += append_to_str('TD_upper')
search_recreate_str += append_to_str('tdprop_lower')
search_recreate_str += append_to_str('tdprop_upper')
search_recreate_str += append_to_str('repeats')
search_recreate_str += append_to_str('tdpatt_lower')
search_recreate_str += append_to_str('tdpatt_upper')
search_recreate_str += append_to_str('sup_self')
search_recreate_str += append_to_str('sup_none')
search_recreate_str += append_to_str('sup_min')
search_recreate_str += append_to_str('subtree')
search_recreate_str += append_to_str('nnltree')
search_recreate_str += append_to_str('btf')
search_recreate_str += append_to_str('btf_nnl')
search_recreate_str += append_to_str('excerpt_size')
search_recreate_str += append_to_str('countby')
if search_recreate_str[-1] == '&':
    search_recreate_str = search_recreate_str[:-1]

#<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">
graphsearch_results_head = '''Content-type:text/html

<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<link rel="stylesheet" href="/bluetable.css">
<title>FZ: Graph Search - Results</title>
<style>
table.batchmodify {
  border-collapse: collapse;
  border: 1px solid #AAAAAA;
}

table.batchmodify th, table.batchmodify td {
  border: 1px solid #AAAAAA;
  padding: 8px;
}
</style>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>
'''

graphsearch_results_tail = '''

<hr>

<p>[<a href="/index.html">fz: Top</a>]</p>

</body>
</html>
'''

def try_command_call(thecmd) -> bool:
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout) = (p.stdin, p.stdout)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        print(result)
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


def clear_NNL(listname: str) -> bool:
    clearcmd = f"./fzgraph -q -E STDOUT -L delete -l '{listname}'"
    return try_command_call(clearcmd)


def Graph_search(searchstr: str, listname: str) -> bool:
    searchcmd = f"./fzgraphsearch -q -E STDOUT -l '{listname}'"
    tdpropbinpatlist = ''
    if tdbinpatt_unspecified:
        tdpropbinpatlist += 'u,'
    if tdbinpatt_inherit:
        tdpropbinpatlist += 'i,'
    if tdbinpatt_variable:
        tdpropbinpatlist += 'v,'
    if tdbinpatt_fixed:
        tdpropbinpatlist += 'f,'
    if tdbinpatt_exact:
        tdpropbinpatlist += 'e,'
    if len(tdpropbinpatlist)>0:
        tdpropbinpatlist = tdpropbinpatlist[:-1]
        searchcmd += f" -b '{tdpropbinpatlist}'"

    if searchstr:
        searchcmd += f" -s '{searchstr}'"
    if (case_sensitive != "on"):
        searchcmd += " -z"
    if t_created_from:
        searchcmd += f" -i '{t_created_from}0000'"
    if t_created_through:
        searchcmd += f" -I '{t_created_through}2359'"
    if completion_lower:
        try:
            searchcmd += f" -c {float(completion_lower):.3f}"
        except:
            print(f'Unable to convert completion_lower ({completion_lower}) to float.')
            return False
    if completion_upper:
        try:
            searchcmd += f" -C {float(completion_upper):.3f}"
        except:
            print(f'Unable to convert completion_upper ({completion_upper}) to float.')
            return False
    if hours_lower:
        try:
            searchcmd += f" -m {int(float(hours_lower) * 60)}"
        except:
            print(f'Unable to convert hours_lower ({hours_lower}) to integer minutes.')
            return False
    if hours_upper:
        try:
            searchcmd += f" -M {int(float(hours_upper) * 60)}"
        except:
            print(f'Unable to convert hours_upper ({hours_upper}) to integer minutes.')
            return False
    if TD_lower:
        searchcmd += f" -t {TD_lower}"
    if TD_upper:
        searchcmd += f" -T {TD_upper}"
    if tdprop_lower:
        searchcmd += f" -p {tdprop_lower}"
        if not tdprop_upper:
            searchcmd += f" -P {tdprop_lower}"
    if tdprop_upper:
        searchcmd += f" -P {tdprop_upper}"
        if not tdprop_lower:
            searchcmd += f" -p {tdprop_upper}"
    if repeats:
        if (repeats == "true"):
            searchcmd += " -r"
        else:
            if (repeats == "false"):
                searchcmd += " -R"
    if tdpatt_lower:
        searchcmd += f' -d {tdpatt_lower}'
        if not tdpatt_upper:
            searchcmd += f' -D {tdpatt_lower}'
    if tdpatt_upper:
        searchcmd += f' -D {tdpatt_upper}'
        if not tdpatt_lower:
            searchcmd += f' -d {tdpatt_upper}'

    if sup_self:
        searchcmd += ' -S self'
    if sup_none:
        searchcmd += ' -S 0'
    if sup_min:
        if int(sup_min)>0:
            searchcmd += f' -S {sup_min}+'

    if subtree:
        searchcmd += f' -B {subtree}'
    if nnltree:
        searchcmd += f' -N {nnltree}'

    if btf:
        searchcmd += f' -F {btf}'
    if btf_nnl:
        searchcmd += f' -f {btf_nnl}'

    if excerpt_size:
        searchcmd += f' -X {excerpt_size}'
        if countby:
            searchcmd += f':{countby}'

    if verbose:
        searchcmd += ' -V'

    print(f'thecmd = {searchcmd}')
    return try_command_call(searchcmd)


def Call_Error(msg: str):
    print('-->')
    print(f'<b>Error: {msg} See page source view for call output.</b>')
    print(graphsearch_results_tail)
    sys.exit(0)   

# TODO: Perhaps filter, unspecified and unique target date should be radio buttons
#       for clarity. As it is, filter is prioritized, and once it is handled
#       in fzeditbatch-cgi.py, the other two are ignored. The unspecified
#       checkbox is prioritized over unique target dates, and once it is handled
#       in fzeditbatch-cgi.py, the rest is ignored. Therefore, only one checkbox
#       request is carried out.
#       All of these apply only to the "batchmodify" button, while the
#       "Apply TD Updates" button ignores all three.
NNLFORMHEAD='''
<form action="/cgi-bin/fzeditbatch-cgi.py" method="post">
<table class="batchmodify"><tbody><tr><td>
Select All<input onclick="toggle(this);" type="checkbox" /><br>
filter <input type="checkbox" name="filter"> - show selected as filtered list<br>
unspecified <input type="checkbox" name="unspecified"> - convert selected to unspecified target date Nodes<br>
unique target dates <input type="checkbox" name="uniqueTD"> - modify selected to ensure unique target dates<br>
<input type="submit" name="action" value="batchmodify"><br>
</td></tr></tbody></table>
'''

CHECKALLJS='''
<script language="JavaScript">
function toggle(source) {
var checkboxes = document.querySelectorAll('input[type="checkbox"]');
for (var i = 0; i < checkboxes.length; i++) {
if (checkboxes[i] != source)
checkboxes[i].checked = source.checked;
}
}
</script>
'''

DRAGGABLEJS='''<script type="text/javascript" src="/draggable_rows.js"></script>
'''

# Note: The apply_td_updates() function is in draggable_rows.js. There,
#       after confirmation, it calls fzeditbatch-cgi.py with the
#       action 'targetdates'.
TDUPDATEBUTTON='''
<button id="tdupdatebtn" class="button button1" onclick="apply_td_updates();">Apply TD Updates</button>
Drag up and down within Node description column to update target date order.
'''

def render_search_results():
    print(graphsearch_results_head)

    print(f'<!-- Search filter expression used: {search_recreate_str} -->')

    print(f'<!-- Clear_NNL({searchresultsNNL}) output')
    if not clear_NNL(searchresultsNNL):
        Call_Error('Unable to clear Named Node List.')
    else:
        print('-->')

    # *** should add t_created_from and t_created_through here
    # Note: This step just creates a list of Nodes in searchresultsNNL, this does not render.
    print(f'<!-- Graph_search("{searchstring}",{searchresultsNNL}) output')
    if not Graph_search(searchstring, searchresultsNNL):
        Call_Error('Search returned error.')
    else:
        print('-->')

    print('<!-- Render search results Named Node List -->')
    if draggable:
        print(TDUPDATEBUTTON)
    print(NNLFORMHEAD)
    print('<table id="nodedata" class="blueTable"><tbody>')
    # *** This next command needs to be able to adjust to btf_nnl
    if btf_nnl:
        rendercmd = f"./fzgraphhtml -q -e -L '{searchresultsNNL}' -N all -S '{btf_nnl}' -c -o STDOUT -E STDOUT"
    else:
        rendercmd = f"./fzgraphhtml -q -e -L '{searchresultsNNL}' -N all -c -o STDOUT -E STDOUT"
    try_command_call(rendercmd)
    print('</tbody></table>')
    print('</form>')
    print(CHECKALLJS)

    if draggable:
        print(DRAGGABLEJS)

    print(graphsearch_results_tail)


if __name__ == '__main__':

    render_search_results()

    sys.exit(0)
