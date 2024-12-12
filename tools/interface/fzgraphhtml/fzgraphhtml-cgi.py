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

from fzmodbase import *
from tcpclient import get_server_address

# *** THESE SHOULD BE SET BY SETUP CONFIGURATION
# *** Perhaps read the following from ~/.formalizer/webdata_path
webdata_path = "/var/www/webdata/formalizer"
TOPICSTATSFILE='/var/www/webdata/formalizer/topic_stats.json'

logfile = webdata_path+'/fzgraphhtml-cgi.log'

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Print this early to catch errors:
print("Content-type:text/html\n")

# Get data from fields
#startfrom = form.getvalue('startfrom')

def log(msg):
    with open(logfile,'w') as f:
        f.write(msg)

# statevars
def addstate(v) -> str:
    if v:
        return '&'+v
    else:
        return ''
SPA = form.getvalue('SPA')
statevars = addstate(SPA)

# local
help = form.getvalue('help')
id = form.getvalue('id')
srclist = form.getvalue('srclist')
list_pos = form.getvalue('list_pos')
edit = form.getvalue('edit')
topics = form.getvalue('topics') # this is used with edit=new
prop = form.getvalue('prop') # this is used with edit=new
topicslist = form.getvalue('topics')
topics_alt = form.getvalue('topics_alt') # uses a custome template
topic = form.getvalue('topic')
tonode = form.getvalue('to-node')
norepeats = form.getvalue('norepeats')
btf = form.getvalue('btf')

# extra arguments for generating Next Nodes Schedule
num_elements = form.getvalue('num_elements')
num_unlimited = form.getvalue('all')
max_td = form.getvalue('max_td')
num_days = form.getvalue('num_days')
sort_by = form.getvalue('sort_by')

# optional extra arguments for edit=new
tosup = form.getvalue('tosup')
todep = form.getvalue('todep')
template = form.getvalue('template')

subtrees_list = form.getvalue('subtrees')

# The following should only show information that is safe to provide
# to those who have permission to connect to this CGI handler.
interface_options_help = '''
<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<link rel="stylesheet" href="/clock.css">
<title>fzgraphhtml-cgi API</title>
</head>
<body>
<script type="text/javascript" src="/clock.js"></script>
<script type="text/javascript" src="/fzuistate.js"></script>

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

<p>
The edit new option can take an additional argument, either 'tosup=NODE_ID' or
'todep=NODE_ID', in which case the corresponding Node is added to the superiors
or dependencies NNL before the new Node page is opened.
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

modify_template = '''<tr><td style="width:10em;text-align:center;"><a href="http://{fzserverpq}/fz/graph/namedlists/{{{{ list_name }}}}?add={node_id}">[add]</a></td><td>[<a href="/cgi-bin/fzgraphhtml-cgi.py?srclist={{{{ list_name }}}}">{{{{ list_name }}}}</a>]</td></tr>
'''

listpagehead = '''<html>
<head>
<meta charset="utf-8">
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/bluetable.css">
<link rel="stylesheet" href="/fzuistate.css">
<link rel="stylesheet" href="/clock.css">
<title>fz:modify Named Node List</title>
</head>
<body>
<script type="text/javascript" src="/clock.js"></script>
<script type="text/javascript" src="/fzuistate.js"></script>
<h3>fz:modify Named Node List</h3>
<p>List element is Node {node_id} at list position {list_pos}.</p>

<form action="/cgi-bin/fzgraph-cgi.py" method="post"><input type="hidden" name="action" value="addtoNNL"><input type="hidden" name="add" value="{node_id}">
<p>Add to any new or existing List: <input type="text" name="namedlist" size="40" maxlength="60"> <input type="submit" value="add"></p>
</form>

'''

listpagehead_nomodif = '''<html>
<head>
<meta charset="utf-8">
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/bluetable.css">
<link rel="stylesheet" href="/fzuistate.css">
<link rel="stylesheet" href="/clock.css">
<title>Named Node List: {list_name}</title>
</head>
<body>
<script type="text/javascript" src="/clock.js"></script>
<script type="text/javascript" src="/fzuistate.js"></script>
<h3>Named Node List: {list_name}</h3>

{form_head}
<table class="blueTable"><tbody>
'''

listpagehead_alllists = '''<html>
<head>
<meta charset="utf-8">
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/bluetable.css">
<link rel="stylesheet" href="/fzuistate.css">
<link rel="stylesheet" href="/clock.css">
<title>Named Node Lists</title>
</head>
<body>
<script type="text/javascript" src="/clock.js"></script>
<script type="text/javascript" src="/fzuistate.js"></script>
<h3>Named Node Lists</h3>

<table class="blueTable"><tbody>
<tr><th>Name</th><th>Size</th><th>Max. Size</th><th>Features</th><th>Actions</th></tr>
'''

listpagetail = '''</tbody></table>
{form_tail}

<hr>

<p>[<a href="/index.html">fz: Top</a>]</p>

</body>
</html>
'''

#editpagehead = '''Content-type:text/html
#
editpagehead = '''<html>
<head>
<meta charset="utf-8">
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/bluetable.css">
<link rel="stylesheet" href="/fzuistate.css">
<link rel="stylesheet" href="/clock.css">
<link rel="stylesheet" href="/tooltip.css">
<link rel="stylesheet" href="/htmltemplatestocopy.css">
<link rel="stylesheet" href="/copiedalert.css">
<title>Formalizer: Edit Node</title>
</head>
<body>
<script type="text/javascript" src="/clock.js"></script>
<script type="text/javascript" src="/fzuistate.js"></script>
<style type="text/css">
td.paramtitle {
    vertical-align: top;
    font-weight: bold;
}
</style>
<br>
<table><tbody>
'''

COPYTOCLIPBOARDENTRY='''<td><button class="button2" onclick="copyInnerHTMLToClipboard('%s');">copy</button> <span id="%s">%s</span></td>
'''

editpagetail = '''</tbody></table>
<div id="after_cptplt"></div>
<div id="templates_html" style="display:none;">
<table><tbody>
<tr>%s</tr>
</table></tbody>
</div>
<script type="text/javascript" src="/htmltemplatestocopy.js"></script>
<script>const htmltemplates = new htmlTemplates('after_cptplt', 'cptplt', 'templates', 'templates_html');</script>
<script type="text/javascript" src="/copiedalert.js"></script>
<script type="text/javascript" src="/copyinnerhtml.js"></script>
<hr>
<script>
function edge_update(event) {
    var edge_id = event.target.id.substring(4);
    var modtype = event.target.id.substring(0,3);
    var value = event.target.value;
    //window.open('http://%s/fz/);
    window.open('/cgi-bin/fzedit-cgi.py?edge='+edge_id+'&edgemod='+modtype+'&modval='+value);
}
</script>
<script type="text/javascript" src="/delayedpopup.js"></script>
<script>
pastePopupLink('docpopupfunc', 'text');
</script>

</body>
</html>
'''

new_node_templates = {
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

#topicspagehead = '''Content-type:text/html
#
topicspagehead = '''<html>
<head>
<meta charset="utf-8">
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<link rel="stylesheet" href="/bluetable.css">
<link rel="stylesheet" href="/clock.css">
<title>fz: Topics</title>
</head>
<body>
<script type="text/javascript" src="/clock.js"></script>
<script type="text/javascript" src="/fzuistate.js"></script>
<h1>fz: Topics</h1>
<br>
<table class="blueTable"><tbody>
'''

topicspagetail = '''</tbody></table>
<hr>

<p>[<a href="/index.html">fz: Top</a>]</p>

</body>
</html>
'''

CUSTOM_TOPICS_TEMPLATE='''<a href="/cgi-bin/fzgraphhtml-cgi.py?topic={{ topic_id }}">{{ tag }}</a> [<a href="/cgi-bin/fzgraphhtml-cgi.py?edit=new&topics={{ tag }}&prop=%s">add to NEW</a>] _SPLIT_'''

# *** OBTAIN THIS SOMEHOW!
#with open(dotformalizer_path+'/server_address','r') as f:
#    fzserverpq_addrport = f.read()
with open('./server_address','r') as f:
    fzserverpq_addrport = f.read()
# OR: fzserverpq_addrport = get_server_address('.')
#fzserverpq_addrport = 'aether.local:8090'
custom_template_file = webdata_path+'/modify_NNL_template.html'


def try_command_call(thecmd, printhere = True) -> str:
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        error = child_stderr.read()
        child_stderr.close()
        if error:
            if len(error)>0:
                print(error)
        if printhere:
            print(result)
            return ''
        else:
            return result

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        return ''


NODEBTF_HTML_TEMPLATE = '''<html>
<head>
<meta charset="utf-8">
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">

<title>fz: Node Inferred Boolean Tag Flags</title>
</head>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>

<h3>fz: Node Inferred Boolean Tag Flags</h3>

<table><tbody>
%s
</tbody></table>

<hr>

<p>[<a href="/index.html">fz: Top</a>]</p>

</body>
</html>
'''

# Calling this requires the CGI arguments id=<node-id>&btf=<anything>.
def generate_Node_inferred_BTF_page():
    thecmd = "./fzgraphhtml -q -o STDOUT -E STDOUT -n %s -B -S threads" % id
    htmlcontent = try_command_call(thecmd, printhere=False)
    print(NODEBTF_HTML_TEMPLATE % htmlcontent)

def generate_embeddable_list_of_NNLs_to_add_Node_to():
    # Make the command for fzgraphhtml with custom template file instead of named_node_list_in_list_template.html
    modify_template_content = modify_template.format(node_id=id, fzserverpq=fzserverpq_addrport)
    with open(custom_template_file,'w') as f:
        f.write(modify_template_content)
    thecmd = "./fzgraphhtml -q -e -L '?' -o STDOUT -E STDOUT -T 'named="+custom_template_file+"'"
    if SPA:
        thecmd += ' -j' # no Javascript
    # Make page head, including form input for new Named Node List to add to
    #print("Content-type:text/html\n\n")
    print(listpagehead.format(node_id=id, list_pos=list_pos))
    # Include a remove-from-srclist button if srclist was not empty
    print(f'<p>Add to [<a href="http://{fzserverpq_addrport}/fz/graph/namedlists/superiors?add={id}&unique=true">superiors</a>] list / add to [<a href="http://{fzserverpq_addrport}/fz/graph/namedlists/dependencies?add={id}&unique=true">dependencies</a>] list.</p>\n')
    if srclist:
        print(f'\n<p>Or, <a href="http://{fzserverpq_addrport}/fz/graph/namedlists/{srclist}?remove={id}">[remove from {srclist}]</a></p>\n')
        print(f'\n<p>Or, <a href="http://{fzserverpq_addrport}/fz/graph/namedlists/{srclist}?move={list_pos}&up=">[move up within {srclist}]</a></p>\n')
        print(f'\n<p>Or, <a href="http://{fzserverpq_addrport}/fz/graph/namedlists/{srclist}?move={list_pos}&down=">[move down within {srclist}]</a></p>\n')
        print(f'\n<p><form action="http://{fzserverpq_addrport}/fz/graph/namedlists/{srclist}" method="get">'
            +f'<input type="hidden" name="move" value="{list_pos}">'
            +f'Move to position: <input type="text" name="to" size="12" maxlength="12"> <input type="submit" value="move"></p></form>')

    print('Or, add to one of the Named Node Lists below:\n\n<table class="blueTable"><tbody>')
    try_command_call(thecmd)

    print(listpagetail.format(form_tail=""))


NNLFORMHEAD='''
<form action="/cgi-bin/fzeditbatch-cgi.py" method="post">
<input type="submit" name="action" value="batchmodify"><br>
Select All<input onclick="toggle(this);" type="checkbox" /><br>
filter <input type="checkbox" name="filter"><br>
unspecified <input type="checkbox" name="unspecified"><br>
unique target dates <input type="checkbox" name="uniqueTD"><br>
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

def generate_NNL_page(include_checkboxes=True):
    # NOTE: Switched to showing checkboxes as well, could make that optional.
    if include_checkboxes:
        thecmd = "./fzgraphhtml -q -e -L '"+srclist+"' -N all -c -o STDOUT -E STDOUT"
        formhead=NNLFORMHEAD
        formtail="</form>\n"+CHECKALLJS
    else:
        thecmd = "./fzgraphhtml -q -e -L '"+srclist+"' -N all -o STDOUT -E STDOUT"
        formhead=""
        formtail=""
    if SPA:
        thecmd += ' -j' # no Javascript
    if sort_by == 'targetdate':
        thecmd += ' -s targetdate'
    #print("Content-type:text/html\n\n")
    if srclist == '?':
        print(listpagehead_alllists)
    else:
        print(listpagehead_nomodif.format(list_name=srclist, form_head=formhead))
    print(f'<!-- thecmd = {thecmd} -->')

    try_command_call(thecmd)

    print(listpagetail.format(form_tail=formtail))


NEW_NODE_INIT_PAGE='''<html>
<head>
<meta charset="utf-8">
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<link rel="stylesheet" href="/clock.css">
<link rel="stylesheet" href="/tooltip.css">
<title>fz: New Node - Init Page</title>
</head>
<body>
<script type="text/javascript" src="/clock.js"></script>
<script type="text/javascript" src="/fzuistate.js"></script>
<h3>New Node - Init Page</h3>
<button class="button button1" onclick="window.open('/cgi-bin/fzgraphhtml-cgi.py?topics_alt=?&to-node=NEW&VTDdefault=on', '_self');">Today VTD defaulting Node - Topic</button>
<button class="button button2" onclick="window.open('/cgi-bin/fzgraphhtml-cgi.py?topics_alt=?&to-node=NEW&UTDdefault=on', '_self');">Other Day UTD defaulting Node - Topic</button>
</body>
</html>
'''

def embed_in_textarea(html_content, textarea_content)->str:
    textarea_pos = html_content.find('<textarea')
    if textarea_pos < 0:
        return '<B>No textarea in fzgraphhtml output.</B>'
    else:
        textarea_start = html_content.find('>', textarea_pos+9)
        if textarea_start < 0:
            return '<B>Textarea in fzgraphhtml output has incomplete tag.</B>'
        else:
            textarea_end = html_content.find('</textarea>', textarea_start+1)
            if textarea_end < 0:
                return '<B>Textarea is missing closing tag.</B>'
            else:
                retstr = html_content[:textarea_start+1]
                retstr += textarea_content
                retstr += html_content[textarea_end:]
                return retstr

def generate_New_Node_init_page():
    print(NEW_NODE_INIT_PAGE)

def generate_Node_edit_form_page():
    # Ensure that the 'mkwsup' buttons work:
    if edit=='new':
        if tosup:
            #thecmd = "./fzgraph -q -L add -l superiors -S "+tosup
            thecmd = f'./fzgraph -q -C "/fz/graph/namedlists/superiors?add={tosup}&unique=true"'
            try_command_call(thecmd, printhere=False)
        elif todep:
            #thecmd = "./fzgraph -q -L add -l dependencies -D "+todep
            thecmd = f'./fzgraph -q -C "/fz/graph/namedlists/dependencies?add={todep}&unique=true"'
            try_command_call(thecmd, printhere=False)

    if edit=='new' and not topics:
        generate_New_Node_init_page()
        return

    template_content = None
    if edit=='new' and template:
        template_content = new_node_templates[template]

    # This is reached when fzgraphhtml-cgi.py?edit=new&topics=<something>
    thecmd = "./fzgraphhtml -q -E STDOUT -o STDOUT -m "+edit
    if topics:
        thecmd += " -t '"+topics+"'"

        # Try to find recommended required time based on median in topic statistics
        try:
            import json
            with open(TOPICSTATSFILE, "r") as f:
                topicstats = json.load(f)
        except:
            topicstats = {}
        if topics in topicstats:
            req_median = topicstats[topics]['median']
            thecmd += " -R "+str(req_median)

        if prop:
            thecmd += " -p "+prop

    if SPA:
        thecmd += ' -j' # no Javascript
    print(editpagehead)
    embeddable_html = try_command_call(thecmd, printhere=False)
    if not template_content:
        print(embeddable_html)
    else:
        print(embed_in_textarea(embeddable_html, template_content))
    copy_templates = COPYTOCLIPBOARDENTRY % ('chkbx_tpl', 'chkbx_tpl', '<input type="checkbox" >')
    for templatekey in new_node_templates:
        template_id = templatekey+'_tpl'
        template_html = new_node_templates[templatekey]
        copy_templates += COPYTOCLIPBOARDENTRY % (template_id, template_id, template_html)
    print(editpagetail % (copy_templates, fzserverpq_addrport))


def generate_topics_page():
    thecmd = "./fzgraphhtml -q -t '?' -E STDOUT -o STDOUT"
    if tonode:
        thecmd += " -i " + tonode
    if SPA:
        thecmd += ' -j' # no Javascript
    try_command_call(thecmd)

def generate_alternative_topics_page():
    VTDdefault = form.getvalue('VTDdefault')
    UTDdefault = form.getvalue('UTDdefault')
    if VTDdefault:
        TDdefault = 'variable'
    else:
        TDdefault = 'unspecified'

    thecmd = "./fzgraphhtml -q -e -t '?' -E STDOUT -o STDOUT"
    if tonode:
        thecmd += " -i " + tonode
    custom_topics_template = CUSTOM_TOPICS_TEMPLATE % TDdefault
    thecmd += f" -T 'topics=STRING:{custom_topics_template}'"
    if SPA:
        thecmd += ' -j' # no Javascript
    print(topicspagehead)
    print('<tr>')
    resstr = try_command_call(thecmd, False)
    resvec = resstr.split('_SPLIT_')
    for i in range(0,len(resvec)):
        if (i % 4) == 0:
            print('</tr>\n<tr>')
        print(f'<td>{resvec[i]}</td>')
    print('</tr>')
    print(topicspagetail)

def generate_topic_nodes_page():
    thecmd = "./fzgraphhtml -q -t '"+topic+"' -E STDOUT -o STDOUT"
    if SPA:
        thecmd += ' -j' # no Javascript
    #print("Content-type:text/html\n\n")
    try_command_call(thecmd)


def generate_Next_Nodes_Schedule_page():
    global max_td
    global num_unlimited
    global num_days
    global subtrees_list
    
    #print("Content-type:text/html\n\n")

    if subtrees_list:
        subtrees_par = " -S "+subtrees_list
    else:
        subtrees_par = ""

    thecmd = f"./fzgraphhtml -q -I -r{subtrees_par} -o STDOUT -E STDOUT"
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

    if SPA:
        thecmd += ' -j' # no Javascript

    log(thecmd)

    try_command_call(thecmd)


def generate_Incomplete_Nodes_list():
    global max_td
    global num_unlimited
    global num_days
    
    #print("Content-type:text/html\n\n")

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

    if SPA:
        thecmd += ' -j' # no Javascript

    log(thecmd)

    try_command_call(thecmd)


def show_interface_options():
    #print("Content-type:text/html\n\n")
    print(interface_options_help)


if __name__ == '__main__':
    if help:
        show_interface_options()
        sys.exit(0)

    if btf:
        generate_Node_inferred_BTF_page()
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

    if topics_alt:
        generate_alternative_topics_page()
        sys.exit(0)
    
    if topic:
        generate_topic_nodes_page()
        sys.exit(0)

    if norepeats:
        generate_Incomplete_Nodes_list()
        sys.exit(0)

    generate_Next_Nodes_Schedule_page()
    sys.exit(0)
