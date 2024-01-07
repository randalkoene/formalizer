#!/usr/bin/python3
#
# Randal A. Koene, 20230326
#
# This CGI handler displays nodeboard output in an HTML page.
#

# Do this immediately, so that any errors are visible:
print("Content-type:text/html\n\n")

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
from os.path import exists
sys.stderr = sys.stdout
from time import strftime
import datetime
import traceback
from json import loads, load
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE
from pathlib import Path

from fzmodbase import *
from fzbgprogress import *

home = str(Path.home())

# *** THESE SHOULD BE SET BY SETUP CONFIGURATION
# *** Perhaps read the following from ~/.formalizer/webdata_path
webdata_path = "/var/www/webdata/formalizer"

intentionscategoriesfile = "/var/www/webdata/formalizer/categories_a2c.json"
workcategoriesfile = "/var/www/webdata/formalizer/categories_work.json"
main2023categoriesfile = "/var/www/webdata/formalizer/categories_main2023.json"

logfile = webdata_path + '/nodeboard-cgi.log'

# HTML_HEAD='''<?xml version="1.0" encoding="UTF-8" ?>
# <!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
# <html xmlns="http://www.w3.org/1999/xhtml">
# <head>
# <meta http-equiv="Content-Type" content="application/xml+xhtml; charset=UTF-8"/>
# <title>System Metrics - Extract</title>
# </head>
# <body style="color:white; background-color:black">
# <pre>
# '''

# HTML_TAIL='''</pre>
# </body>
# </html>
# '''

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
sysmet_file = form.getvalue('f')
node_dependencies = form.getvalue('n')
node_dependencies_tree = form.getvalue('G')
node_superiors_tree = form.getvalue('g')
filter_string = form.getvalue('F')
show_completed = form.getvalue('I')
subtree_list = form.getvalue('D')
threads = form.getvalue('T')
progress_analysis = form.getvalue('P')
calendar_file = form.getvalue('c')
header_arg = form.getvalue('H')
vertical_multiplier = form.getvalue('M')

if filter_string:
    include_filter_string = ' -F %s' % filter_string
else:
    include_filter_string = ''
if show_completed == 'true':
    include_show_completed = ' -I'
else:
    include_show_completed = ''
if threads:
    include_threads = ' -T'
else:
    include_threads = ''
if progress_analysis:
    include_progress_analysis = ' -P'
else:
    include_progress_analysis = ''

# *** OBTAIN THIS SOMEHOW!
#with open('./server_address','r') as f:
#    fzserverpq_addrport = f.read()

def log(logstr):
    with open(logfile, 'w') as f:
        f.write(logstr)

def try_command_call(thecmd, print_result=True)->str:
    try:
        log(thecmd)
    except:
        pass
    try:
        #print(thecmd, flush=True)
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
        child_stdin.close()
        result = child_stdout.read()
        error = child_stderr.read()
        child_stdout.close()
        child_stderr.close()
        if print_result:
            print(result)
            return ''
        if len(error)>0:
            print(error)
        #print(result.replace('\n', '<BR>'))
        return result

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        return ''

REDIRECT='''
<html>
<meta http-equiv="Refresh" content="0; url='/formalizer/data%s'" />
</html>
'''

TEST='''
<html>
<body>
Output was: %s
Click here: <a href="/formalizer/data%s">%s</a>
</body>
</html>
'''

def show_sysmet_board(sysmet_json_path:str, sysmet_output_path:str):
    thecmd = f"./nodeboard -f {sysmet_json_path} {include_filter_string} {include_show_completed} -q -o /var/www/webdata/formalizer{sysmet_output_path}"
    #thecmd = f"./nodeboard -f {sysmet_json_path} -o STDOUT"
    res = try_command_call(thecmd, print_result=False)
    #print(res)
    print(REDIRECT % sysmet_output_path)
    #print(TEST % (res, sysmet_output_path, sysmet_output_path))

def show_main2023_board():
    show_sysmet_board(main2023categoriesfile, '/main2023categories-kanban.html')

def show_node_dependencies_board():
    if node_dependencies_tree:
        thecmd = f"./nodeboard -G -n {node_dependencies} {include_filter_string} {include_show_completed} {include_threads} {include_progress_analysis} -q -o /var/www/webdata/formalizer/node_dependencies_kanban.html"
    elif node_superiors_tree:
        thecmd = f"./nodeboard -g -n {node_dependencies} {include_filter_string} {include_show_completed} {include_threads} {include_progress_analysis} -q -o /var/www/webdata/formalizer/node_dependencies_kanban.html"
    else:
        thecmd = f"./nodeboard -n {node_dependencies} {include_filter_string} {include_show_completed} -q -o /var/www/webdata/formalizer/node_dependencies_kanban.html"
    res = try_command_call(thecmd, print_result=False)
    print(REDIRECT % "/node_dependencies_kanban.html")

ANALYSIS_PROGRESS_PAGE = '''
<html>
<body>
<h3>Running progress analysis...</h3>
%s
<script>
%s

progressloop();

</script>
</body>
</html>
'''

# If I want this to show progress:
# 1. Launch it in the background with >logfile 2>&1 &.
# 2. Print the progress page (see test_progress_indicator FZ method).
# 3. In nodeboard, use the -p /var/www/webdata/formalizer/nodeboard-progress.state option
#    and have nodeboard update that file as it works through cards.
# 4. The done-page will be  /formalizer/data/subtree_list_kanban.html.
def show_subtree_board():
    nodeboard_logfile = '/var/www/webdata/formalizer/nodeboard.log'
    progress_state_file = 'nodeboard_progress.state'
    progress_state_path = '/var/www/webdata/formalizer/%s' % progress_state_file
    result_file = 'subtree_list_kanban.html'
    result_page_path = '/var/www/webdata/formalizer/%s' % result_file
    result_page_url = '/formalizer/data/%s' % result_file
    if progress_analysis:
        thecmd = f"./nodeboard -D {subtree_list} {include_threads} {include_progress_analysis} {include_show_completed} -p {progress_state_path} -q -o {result_page_path} >{nodeboard_logfile} 2>&1 &"
        res = try_command_call(thecmd, print_result=False)
        embed_in_html, embed_in_script = make_background_progress_monitor(progress_state_file, result_page_url)
        print(ANALYSIS_PROGRESS_PAGE % (embed_in_html, embed_in_script))
    else:
        thecmd = f"./nodeboard -D {subtree_list} {include_threads} {include_show_completed} -q -o {result_page_path}"
        res = try_command_call(thecmd, print_result=False)
        print(REDIRECT % "/subtree_list_kanban.html")

def show_calendar_schedule_board():
    if vertical_multiplier:
        vertical_multiplier_arg = ' -M %s' % vertical_multiplier
    else:
        vertical_multiplier_arg = ''
    if header_arg:
        header_arg_str = " -H '%s'" % header_arg
    thecmd = f"./nodeboard -c '{calendar_file}'{header_arg_str}{vertical_multiplier_arg} -q -o STDOUT"
    res = try_command_call(thecmd, print_result=True)

HELP='''
<html>
<body>
The nodeboard-cgi.py script received no recognized form data.
</body>
</html>
'''

def show_help():
    print(HELP)

if __name__ == '__main__':
    if (sysmet_file == 'main2023'):
        show_main2023_board()
        sys.exit(0)

    if subtree_list:
        show_subtree_board()
        sys.exit(0)

    if node_dependencies: #(node_dependencies != ''):
        show_node_dependencies_board()
        sys.exit(0)

    if calendar_file:
        show_calendar_schedule_board()
        sys.exit(0)

    show_help()
    sys.exit(0)
