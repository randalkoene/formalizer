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

def show_subtree_board():
    thecmd = f"./nodeboard -D {subtree_list} {include_threads} {include_progress_analysis} {include_show_completed} -q -o /var/www/webdata/formalizer/subtree_list_kanban.html"
    res = try_command_call(thecmd, print_result=False)
    print(REDIRECT % "/subtree_list_kanban.html")

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

    if (node_dependencies != ''):
        show_node_dependencies_board()
        sys.exit(0)

    show_help()
    sys.exit(0)
