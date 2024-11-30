#!/usr/bin/python3
#
# logentry-form.py
#
# Randal A. Koene, 20240412
#
# Generate a page from which to select a template for a new node page.

try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os, stat
sys.stderr = sys.stdout
import time
from time import strftime
import traceback
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE

#template_out_path = '/var/www/webdata/formalizer/addnode-template.html'

# Print this early to enable seeing errors.
print("Content-type:text/html\n\n")

form = cgi.FieldStorage()
topics = form.getvalue("topics")
selection = form.getvalue('selection')

# optional extra arguments for edit=new
extra_args=''
# tosup = form.getvalue('tosup') # By the time it gets here Superiors was already updated.
# if tosup:
#     extra_args += '&tosup='+tosup
# todep = form.getvalue('todep')
# if todep:
#     extra_args += '&todep='+todep
prop = form.getvalue('prop')
if prop:
    extra_args += '&prop='+prop

config = {
    'verbose': False,
    'logcmdcalls': False,
    'cmdlog': '/var/www/webdata/formalizer/addnode-template.log'
}
results = {}

with open('./server_address','r') as f:
    fzserverpq_addrport = f.read()

def try_subprocess_check_output(thecmdstring:str, resstore):
    if config['verbose']:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
    if config['logcmdcalls']:
        with open(config['cmdlog'],'a') as f:
            f.write(thecmdstring+'\n')
    try:
        p = Popen(thecmdstring,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout) = (p.stdin, p.stdout)
        child_stdin.close()
        result = child_stdout.read()
        if resstore:
            results[resstore] = result
        child_stdout.close()
        if result and config['verbose']:
            print(result)
        return 0

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        print('<pre>')
        for line in a:
            print(line)
        print('</pre>')
        return 1

templates = {
    "Milestone": '''MILESTONE: <b>[(label)]</b> (description of achievement)
<P>
@PREREQS:(prereqs)@
@PROVIDES:(provides)@
''',
    "Contact": '''<b>(name)</b>. https://contacts.google.com/u/1/person/(link-code)
<P>
<b>Description</b>:
<P>
(description)
<P>
<b>Next interaction</b>:
<P>
(next)
<P>
@TZADJUST@
@SELFWORK@
''',
    "Check List": '''<ul>
<li><input type="checkbox" > </li>
<li><input type="checkbox" > </li>
<li><input type="checkbox" > </li>
</ul>
'''
}

# TEMPLATE_LINE='''<tr><td>
# <button class="button button1" onclick="window.open('/cgi-bin/addnode-template.py?topics=%s&selection=%s','_self');">%s</button>
# <P>
# %s
# <hr>
# </td>
# </tr>
# '''

TEMPLATE_LINE='''<tr><td>
<button class="button button1" onclick="window.open('/cgi-bin/fzgraphhtml-cgi.py?edit=new&topics=%s&template=%s%s','_self');">%s</button>
<P>
%s
<hr>
</td>
</tr>
'''

SELECT_TEMPLATE_PAGE='''<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Add Node - Select Template</title>
</head>
<body>
<h1>fz: Add Node - Select Template</h1>

<table><tbody>
%s
</tbody></table>

<hr>

<p>[<a href="/index.html">fz: Top</a>]</p>

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

# editpagehead = '''<html>
# <head>
# <meta charset="utf-8">
# <link rel="icon" href="/favicon-nodes-32x32.png">
# <link rel="stylesheet" href="/fz.css">
# <link rel="stylesheet" href="/bluetable.css">
# <link rel="stylesheet" href="/fzuistate.css">
# <link rel="stylesheet" href="/clock.css">
# <title>Formalizer: Edit Node</title>
# </head>
# <body>
# <style type="text/css">
# td.paramtitle {
#     vertical-align: top;
#     font-weight: bold;
# }
# .chktop {
#     background-color: #B0C4F5;
# }
# </style>
# <button id="clock" class="button button2">_____</button>
# <br>
# <table><tbody>
# '''

# editpagetail = '''</tbody></table>
# <hr>
# <script>
# function edge_update(event) {
#     var edge_id = event.target.id.substring(4);
#     var modtype = event.target.id.substring(0,3);
#     var value = event.target.value;
#     //window.open('http://%s/fz/);
#     window.open('/cgi-bin/fzedit-cgi.py?edge='+edge_id+'&edgemod='+modtype+'&modval='+value);
# }
# </script>
# <script type="text/javascript" src="/fzuistate.js"></script>
# <script type="text/javascript" src="/clock.js"></script>
# <script>
#     var clock = new floatClock('clock');
# </script>
# </body>
# </html>
# '''

# def template_selected(topics:str, selection:str):
#     try:
#         # # Save the selected template to a file.
#         # with open(template_out_path, 'w') as f:
#         #     f.write(templates[selection])

#         # Generate New Node page with template content.
#         thecmd = "./fzgraphhtml -q -E STDOUT -o STDOUT -m new -t '"+topics+"'"
#         retval = try_subprocess_check_output(thecmd, resstore='newnodepage')
#         if retval == 0:
#             textarea_pos = results['newnodepage'].find('<textarea')
#             if textarea_pos < 0:
#                 print('<html><body>No textarea in fzgraphhtml output.</body></html>')
#             else:
#                 textarea_start = results['newnodepage'].find('>', textarea_pos+9)
#                 if textarea_start < 0:
#                     print('<html><body>Textarea in fzgraphhtml output has incomplete tag.</body></html>')
#                 else:
#                     textarea_end = results['newnodepage'].find('</textarea>', textarea_start+1)
#                     if textarea_end < 0:
#                         print('<html><body>Textarea is missing closing tag.</body></html>')
#                     else:
#                         print(editpagehead)
#                         print(results['newnodepage'][:textarea_start+1])
#                         print(templates[selection])
#                         print(results['newnodepage'][textarea_end:])
#                         print(editpagetail % fzserverpq_addrport)


#     except Exception as e:
#         print('<html><body>Failed to save selected template. Exception: %s</body></html>' % str(e))

def make_select_template_page(topics:str):

    templates_list_str = ""

    for template_key, template_content in templates.items():
        templates_list_str += TEMPLATE_LINE % (topics, template_key, extra_args, template_key, template_content)

    print(SELECT_TEMPLATE_PAGE % templates_list_str)

if __name__ == '__main__':
    # if selection:
    #     template_selected(topics, selection)
    # else:
    #     make_select_template_page(topics)

    make_select_template_page(topics)

    sys.exit(0)
