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
if search_recreate_str[-1] == '&':
    search_recreate_str = search_recreate_str[:-1]

#<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">
graphsearch_results_head = '''Content-type:text/html

<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/bluetable.css">
<title>FZ: Graph Search - Results</title>
</head>
<body>
<style type="text/css">
.chktop { 
    background-color: #B0C4F5;
}
table tr.chktop { background: #B0C4F5; }
</style>
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
            searchcmd += f" -m {int(hours_lower * 60)}"
        except:
            print(f'Unable to convert hours_lower ({hours_lower}) to integer minutes.')
            return False
    if hours_upper:
        try:
            searchcmd += f" -M {int(hours_upper * 60)}"
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
    if verbose:
        searchcmd += ' -V'

    print(f'thecmd = {searchcmd}')
    return try_command_call(searchcmd)


def Call_Error(msg: str):
    print('-->')
    print(f'<b>Error: {msg} See page source view for call output.</b>')
    print(graphsearch_results_tail)
    sys.exit(0)   


def render_search_results():
    print(graphsearch_results_head)

    print(f'<!-- Search filter expression used: {search_recreate_str} -->')

    print(f'<!-- Clear_NNL({searchresultsNNL}) output')
    if not clear_NNL(searchresultsNNL):
        Call_Error('Unable to clear Named Node List.')
    else:
        print('-->')

    # *** should add t_created_from and t_created_through here
    print(f'<!-- Graph_search("{searchstring}",{searchresultsNNL}) output')
    if not Graph_search(searchstring, searchresultsNNL):
        Call_Error('Search returned error.')
    else:
        print('-->')

    print('<!-- Render search results Named Node List -->')
    print('<table class="blueTable"><tbody>')
    rendercmd = f"./fzgraphhtml -q -e -L '{searchresultsNNL}' -N all -o STDOUT -E STDOUT"
    try_command_call(rendercmd)
    print('</tbody></table>')

    print(graphsearch_results_tail)


if __name__ == '__main__':

    render_search_results()

    sys.exit(0)
