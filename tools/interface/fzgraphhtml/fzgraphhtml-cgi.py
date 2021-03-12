#!/usr/bin/python3
#
# Randal A. Koene, 20201013
#
# This CGI handler provides a near-verbatim equivalent access to fzgraphhtml via web form.
#
# Expects POST or GET data of the following forms:
#   id=<node-id>[&srclist=<list-name>] to generate a List of Named Node Lists with links to add the specified Node.
#   srclist=<list-name>  to generate a page that shows the content of a Named Node List.
#   srclist=? to generate a page that shows all Named Node Lists.
#   edit=<node-id> to generate the form page with which to edit Node data.
#   otherwise, generate a page that shows the Next Nodes Schedule.
#

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
from pathlib import Path
home = str(Path.home())

# *** THESE SHOULD BE SET BY SETUP CONFIGURATION
# *** Perhaps read the following from ~/.formalizer/webdata_path
webdata_path = "/var/www/webdata/formalizer"

logfile = webdata_path+'/fzgraphhtml-cgi.log'

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
#startfrom = form.getvalue('startfrom')
help = form.getvalue('help')
id = form.getvalue('id')
srclist = form.getvalue('srclist')
edit = form.getvalue('edit')
topicslist = form.getvalue('topics')
topic = form.getvalue('topic')
tonode = form.getvalue('to-node')
norepeats = form.getvalue('norepeats')

# extra arguments for generating Next Nodes Schedule
num_elements = form.getvalue('num_elements')
num_unlimited = form.getvalue('all')
max_td = form.getvalue('max_td')
num_days = form.getvalue('num_days')

# The following should only show information that is safe to provide
# to those who have permission to connect to this CGI handler.
interface_options_help = '''
<html>
<link rel="stylesheet" href="/fz.css">
<head>
<title>fzgraphhtml-cgi API</title>
</head>
<body>
<h1>fzgraphhtml-cgi API</h1>

<p>
Main modes:
<ul>
<li><code>edit</code>: Node Edit Page
<li><code>id</code>: List of Named Node Lists
<li><code>srclist</code>: List of Nodes in a specified Named Node List
<li><code>topics</code>: List of Topics
<li><code>topic</code>: List of Nodes in a specified Topic
<li><code>norepeats</code>: List of Incomplete Nodes in order of Effective Target Date
<li>By default: Schedule of Incomplete Nodes with Repeats of Repeating Nodes
</ul>
</p>

<p>
Note that <code>fzlink.py</code> generates the single Node Info Page (without editing form elements).
</p>

<h3>Node Edit Page</h3>

<p>
<code>fzgraphhtml-cgi.py?edit=NODE_ID</code><br>
Genereates the Node Edit form page for the Node identified by NODE_ID.
</p>

<p>
Note that this is also used to create the form with which tot define a new Node.
Simply set NODE_ID to the string 'new'.
</p>

<h3>List of Named Node Lists</h3>

<p>
<code>fzgraphhtml-cgi.py?id=NODE_ID</code><br>
Generates the list of all existing Named Node Lists with action links. The NODE_ID is applied
to these action links, for example, to add the specified Node to a Named Node List.
</p>

<p>
(more info to come)
</p>

</body>
</html>
'''

modify_template = '''<tr><td>[<a href="/cgi-bin/fzgraphhtml-cgi.py?srclist={{{{ list_name }}}}">{{{{ list_name }}}}</a>]</td><td><a href="http://{fzserverpq}/fz/graph/namedlists/{{{{ list_name }}}}?add={node_id}">[add]</a></td></tr>
'''

listpagehead = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/bluetable.css">
<title>fz:modify Named Node List</title>
</head>
<body>
<h3>fz:modfy Named Node List</h3>
<p>List element is Node {node_id}</p>

<form action="/cgi-bin/fzgraph-cgi.py" method="post"><input type="hidden" name="add" value="{node_id}">
<p>Add to any new or existing List: <input type="text" name="namedlist" size="40" maxlength="60"> <input type="submit" value="add"></p>
</form>

'''

listpagehead_nomodif = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/bluetable.css">
<title>Named Node List: {list_name}</title>
</head>
<body>
<h3>Named Node List: {list_name}</h3>

<table class="blueTable"><tbody>
'''

listpagehead_alllists = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/bluetable.css">
<title>Named Node Lists</title>
</head>
<body>
<h3>Named Node Lists</h3>

<table class="blueTable"><tbody>
<tr><th>Name</th><th>Size</th><th>Max. Size</th><th>Features</th><th>Actions</th></tr>
'''

listpagetail = '''</tbody></table>
</body>
</html>
'''

editpagehead = '''Content-type:text/html

<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>Formalizer: Edit Node</title>
</head>
<body>
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>
<br>
<table><tbody>
'''

editpagetail = '''</tbody></table>
<hr>
</body>
</html>
'''

topicspagehead = '''Content-type:text/html

<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>Formalizer: Topics</title>
</head>
<body>
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>
<br>
<table><tbody>
'''

topicspagetail = '''</tbody></table>
<hr>
</body>
</html>
'''

# *** OBTAIN THIS SOMEHOW!
#with open(dotformalizer_path+'/server_address','r') as f:
#    fzserverpq_addrport = f.read()
with open('./server_address','r') as f:
    fzserverpq_addrport = f.read()
#fzserverpq_addrport = 'aether.local:8090'
custom_template_file = webdata_path+'/modify_NNL_template.html'


def try_command_call(thecmd):
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


def log(msg):
    with open(logfile,'w') as f:
        f.write(msg)


def generate_embeddable_list_of_NNLs_to_add_Node_to():
    # Make the command for fzgraphhtml with custom template file instead of named_node_list_in_list_template.html
    modify_template_content = modify_template.format(node_id=id, fzserverpq=fzserverpq_addrport)
    with open(custom_template_file,'w') as f:
        f.write(modify_template_content)
    thecmd = "./fzgraphhtml -q -e -L '?' -o STDOUT -E STDOUT -T 'named="+custom_template_file+"'"
    # Make page head, including form input for new Named Node List to add to
    print("Content-type:text/html\n\n")
    print(listpagehead.format(node_id=id))
    # Include a remove-from-srclist button if srclist was not empty
    print(f'<p>Add to [<a href="http://{fzserverpq_addrport}/fz/graph/namedlists/superiors?add={id}">superiors</a>] list / add to [<a href="http://{fzserverpq_addrport}/fz/graph/namedlists/dependencies?add={id}">dependencies</a>] list.</p>\n')
    if srclist:
        print(f'\n<p>Or, <a href="http://{fzserverpq_addrport}/fz/graph/namedlists/{srclist}?remove={id}">[remove from {srclist}]</a></p>\n')

    print('Or, add to one of the Named Node Lists below:\n\n<table class="blueTable"><tbody>')
    try_command_call(thecmd)

    print(listpagetail)


def generate_NNL_page():
    thecmd = "./fzgraphhtml -q -e -L '"+srclist+"' -o STDOUT -E STDOUT"
    print("Content-type:text/html\n\n")
    if srclist == '?':
        print(listpagehead_alllists)
    else:
        print(listpagehead_nomodif.format(list_name=srclist))
    print(f'<!-- thecmd = {thecmd} -->')

    try_command_call(thecmd)

    print(listpagetail)


def generate_Node_edit_form_page():
    thecmd = "./fzgraphhtml -q -E STDOUT -o STDOUT -m "+edit
    print(editpagehead)
    try_command_call(thecmd)
    print(editpagetail)


def generate_topics_page():
    thecmd = "./fzgraphhtml -q -t '?' -E STDOUT -o STDOUT"
    if tonode:
        thecmd += " -i " + tonode
    print("Content-type:text/html\n\n")
    #print(topicspagehead)
    try_command_call(thecmd)
    #print(topicspagetail)


def generate_topic_nodes_page():
    thecmd = "./fzgraphhtml -q -t '"+topic+"' -E STDOUT -o STDOUT"
    print("Content-type:text/html\n\n")
    try_command_call(thecmd)


def generate_Next_Nodes_Schedule_page():
    global max_td
    global num_unlimited
    global num_days
    
    print("Content-type:text/html\n\n")

    thecmd = "./fzgraphhtml -q -I -r -o STDOUT -E STDOUT"
    #thecmd = "./fzgraphhtml -q -I -o STDOUT -E STDOUT"

    if max_td:
        if len(max_td)==8:
            max_td += '2359'
        thecmd += ' -M '+max_td

    if (num_unlimited == 'on'):
        thecmd += ' -N all '
    else:
        if num_elements:
            thecmd += ' -N '+num_elements

    if num_days:
        thecmd += ' -D '+num_days

    log(thecmd)

    try_command_call(thecmd)


def generate_Incomplete_Nodes_list():
    global max_td
    global num_unlimited
    global num_days
    
    print("Content-type:text/html\n\n")

    thecmd = "./fzgraphhtml -q -I -o STDOUT -E STDOUT"

    if max_td:
        if len(max_td)==8:
            max_td += '2359'
        thecmd += ' -M '+max_td

    if (num_unlimited == 'on'):
        thecmd += ' -N all '
    else:
        if num_elements:
            thecmd += ' -N '+num_elements

    if num_days:
        thecmd += ' -D '+num_days

    log(thecmd)

    try_command_call(thecmd)


def show_interface_options():
    print("Content-type:text/html\n\n")
    print(interface_options_help)


if __name__ == '__main__':
    if help:
        show_interface_options()
        sys.exit(0)

    if id:
        generate_embeddable_list_of_NNLs_to_add_Node_to()
        sys.exit(0)
    
    if srclist:
        generate_NNL_page()
        sys.exit(0)

    if edit:
        generate_Node_edit_form_page()
        sys.exit(0)

    if topicslist:
        generate_topics_page()
        sys.exit(0)
    
    if topic:
        generate_topic_nodes_page()
        sys.exit(0)

    if norepeats:
        generate_Incomplete_Nodes_list()
        sys.exit(0)

    generate_Next_Nodes_Schedule_page()
    sys.exit(0)
