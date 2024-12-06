#!/usr/bin/python3
#
# Randal A. Koene, 20200921
#
# CGI script to forward Node, Log chunk and Log entry ID references to a handler.
# This handler can also forward fzserverpq API file serving requests from
# any HTML page.

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

from fzmodbase import *
#from tcpclient import client_socket_request
from tcpclient import get_server_address

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
id = form.getvalue('id')
alt = form.getvalue('alt')
fz = form.getvalue('FZSERVER')

http_response = 'Content-type:text/html\n'

REDIRECT='''<html>
<meta http-equiv="Refresh" content="0; URL=%s" />
</html>
'''

# This page header is used when showing Nodes.
page_header = '''<html>
<head>
<link rel="icon" href="/favicon-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<link rel="stylesheet" href="/clock.css">
<link rel="stylesheet" href="/tooltip.css">
<title>Formalizer: fzlink handler</title>
</head>
<body>
'''

page_tail = '''<hr>
<script type="text/javascript" src="/fzuistate.js"></script>
<script type="text/javascript" src="/clock.js"></script>
<script type="text/javascript" src="/delayedpopup.js"></script>
<script>
set_hover_delayed_function('.docpopupfunc', openPopup, 1000);
</script>
</body>
</html>
'''

thecmd = ""
altcmd = ""
addframe = False

cut_from_page_header = '''
<style type="text/css">
.chktop { 
    background-color: #B0C4F5;
}
#darkmode {
position: fixed;
bottom: 0px;
right: 0px;
}
</style>
<button id="darkmode" class="button button2" onclick="switch_light_or_dark();">Light / Dark</button>
'''

def try_the_command(thecommand: str):
    try:
        p = Popen(thecommand,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
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


def build_command(id, alt):
    global thecmd
    global altcmd
    global addframe
    if id:
        idlen = len(id)
        if (idlen==12): # This refers to a Log Chunk!
            # Log chunk ID
            thecmd = "./fzloghtml -q -d formalizer -s randalk -o STDOUT -E STDOUT -D 1 -1 "+id
        else:
            if (idlen>12):
                if (id[12]=='.'): # This refers to a Log Entry!
                    # Log entry ID (show the chunk)
                    chunkid = id[0:12]
                    thecmd = "./fzloghtml -q -d formalizer -s randalk -o STDOUT -E STDOUT -D 1 -1 "+chunkid
                else:
                    if (idlen==16): # This refers to a Node!
                        # Node ID
                        addframe = True
                        thecmd = "./fzgraphhtml -q -E STDOUT -o STDOUT -n "+id
                        #thecmd = "./fzquerypq -q -d formalizer -s randalk -E STDOUT -F html -n "+id
                        if alt:
                            if (alt=="hist50"):
                                altcmd = "./fzloghtml -q -d formalizer -s randalk -o STDOUT -E STDOUT -N -c 50 -n "+id
                            else:
                                if (alt=="histfull"):
                                    altcmd = "./fzloghtml -q -d formalizer -s randalk -o STDOUT -E STDOUT -N -t -n "+id


def render_output():
    print(http_response)
    if addframe:
        print(page_header)
    thisscript = os.path.realpath(__file__)
    print(f'<!--(For dev reference, this script is at {thisscript}.) -->')
    print("<!-- [Formalizer: fzlink handler]\n<p></p> -->")

    if thecmd:
        print(f'\n<!-- Primary command: {thecmd} -->\n')
        if addframe:
            print('<br>\n<table><tbody>\n')
        try_the_command(thecmd)
        if addframe or altcmd:
            print("</tbody></table>")

    if altcmd:
        print(f'\n<!-- Secondary command: {altcmd} -->\n')
        print('<br>\n<table><tbody>\n')
        print('<br>\n\n')
        try_the_command(altcmd)
        if addframe:
            print("</tbody></table>")

        if (alt=="hist50"):
            #print(f'\n<br>\n<p>\n[<a href="/cgi-bin/fzlink.py?id={id}&alt=histfull">full history</a>]\n</p>\n<br>\n')
            print(f'\n<br>\n<p>\n<button class="button button2" onclick="location.href=\'/cgi-bin/fzlink.py?id={id}&alt=histfull\';">full history</button>\n</p>\n<br>\n')

    if addframe:
        print(page_tail)

# def forward_file_serving():
#     print(http_response)
#     data = client_socket_request('GET %s' % fz, running_on_server=True, expect_http=True)
#     print(data)

def forward_file_serving():
    print(http_response)
    server_address = get_server_address('.')
    url = 'http://%s%s' % (server_address, fz)
    print(REDIRECT % url)

if __name__ == '__main__':

    if fz:
        forward_file_serving()
        sys.exit(0)

    build_command(id, alt)

    render_output()

    sys.exit(0)
