#!/usr/bin/python3
#
# Randal A. Koene, 20201013
#
# This CGI handler provides a near-verbatim equivalent access to fzgraphhtml via web form.

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

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
#startfrom = form.getvalue('startfrom')
id = form.getvalue('id')
srclist = form.getvalue('srclist')

modify_template = '''<tr><td>{{ list_name }}</td></tr><td><a href="http://{fzserverpq}/fz/graph/namedlists/{{ list_name }}?add={node_id}">[add]</a></td>
'''

listpagehead = '''<html>
<head>
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
<title>Named Node List: {list_name}</title>
</head>
<body>
<h3>Named Node List: {list_name}</h3>

'''

listpagehead_alllists = '''<html>
<head>
<title>Named Node Lists</title>
</head>
<body>
<h3>Named Node Lists</h3>

'''

listpagetail = '''</tbody></table>
</body>
</html>
'''

# *** OBTAIN THIS SOMEHOW!
fzserverpq_addrport = 'aether.local:8090'

if id:
    # Make the command for fzgraphhtml with custom template file instead of named_node_list_in_list_template.html
    modify_template_content = modify_template.format(node_id=id, fzserverpq=fzserverpq_addrport)
    with open('/tmp/modify_NNL_template.html','w') as f:
        f.write(modify_template_content)
    thecmd = "./fzgraphhtml -q -L '?' -o STDOUT -E STDOUT -T 'named=/tmp/modify_NNL_template.html'"
    # Make page head, including form input for new Named Node List to add to
    print("Content-type:text/html\n\n")
    print(listpagehead.format(node_id=id))
    # Include a remove-from-srclist button if srclist was not empty
    if srclist:
        print(f'\n<p>Or, <a href="http://{fzserverpq_addrport}/fz/graph/namedlists/{srclist}?remove={id}">[remove from {srclist}]</a></p>\n')
    # Call fzgraphhtml (try-except)
    print('Or, add to one of the Named Node Lists below:\n\n<table><tbody>')
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout) = (p.stdin, p.stdout)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        print(result)

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)

    # Make page tail
    print(listpagetail)

else:
    if srclist:
        thecmd = "./fzgraphhtml -q -e -L '"+srclist+"' -o STDOUT -E STDOUT"
        print("Content-type:text/html\n\n")
        if srclist == '?':
            print(listpagehead_alllists)
        else:
            print(listpagehead_nomodif.format(list_name=srclist))
        print(f'<!-- thecmd = {thecmd} -->')
        # Include a remove-from-srclist button if srclist was not empty
        try:
            p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
            (child_stdin,child_stdout) = (p.stdin, p.stdout)
            child_stdin.close()
            result = child_stdout.read()
            child_stdout.close()
            print(result)

        except Exception as ex:                
            print(ex)
            f = StringIO()
            print_exc(file=f)
            a = f.getvalue().splitlines()
            for line in a:
                print(line)

        # Make page tail
        print(listpagetail)

    else:
        print("Content-type:text/html\n\n")

        #thisscript = os.path.realpath(__file__)
        #print(f'(For dev reference, this script is at {thisscript}.)')

        #cmdoptions = ""

        #if startfrom:
        #    cmdoptions += ' -1 '+startfrom

        #if cmdoptions:

        thecmd = "./fzgraphhtml -q -I -o STDOUT -E STDOUT"
        #print('Using this command: ',thecmd)
        #print('<br>\n')

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

#if "name" not in form or "addr" not in form:
#    print("<H1>Error</H1>")
#    print("Please fill in the name and addr fields.")
#    return

#print("</table>")
#print("</body>")
#print("</html>")
