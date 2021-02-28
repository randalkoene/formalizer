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
searchstring = form.getvalue('searchstring')

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
    searchcmd = f"./fzgraphsearch -q -E STDOUT -s '{searchstr}' -l '{listname}'"
    return try_command_call(searchcmd)


def Call_Error(msg: str):
    print('-->')
    print(f'<b>Error: {msg} See page source view for call output.</b>')
    print(graphsearch_results_tail)
    sys.exit(0)   


def render_search_results():
    print(graphsearch_results_head)

    print(f'<!-- Clear_NNL({searchresultsNNL}) output')
    if not clear_NNL(searchresultsNNL):
        Call_Error('Unable to clear Named Node List.')
    else:
        print('-->')

    print(f'<!-- Graph_search("{searchstring}",{searchresultsNNL}) output')
    if not Graph_search(searchstring, searchresultsNNL):
        Call_Error('Search returned error.')
    else:
        print('-->')

    print('<!-- Render search results Named Node List -->')
    print('<table class="blueTable"><tbody>')
    rendercmd = f"./fzgraphhtml -q -e -L '{searchresultsNNL}' -o STDOUT -E STDOUT"
    try_command_call(rendercmd)
    print('</tbody></table>')

    print(graphsearch_results_tail)


if __name__ == '__main__':

    render_search_results()

    sys.exit(0)
