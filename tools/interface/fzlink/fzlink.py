#!/usr/bin/python3
#
# Randal A. Koene, 20200921
#
# CGI script to forward Node, Log chunk and Log entry ID references to a handler.

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

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
id = form.getvalue('id')
alt = form.getvalue('alt')

print("Content-type:text/html\n\n")
print("<html>")
print("<head>")
print('<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">')
print("<title>Formalizer: fzlink handler</title>")
print("</head>")
print("<body>")
print('<style type="text/css">')
print('.chktop { ')
print('    background-color: #B0C4F5;')
print('}')
#print("table tr.chktop { background: #B0C4F5; }")
print("</style>")


thisscript = os.path.realpath(__file__)
print(f'<!--(For dev reference, this script is at {thisscript}.) -->')

#print("<h3>Formalizer: fzlink handler</h3>\n<p></p>")
print("[Formalizer: fzlink handler]\n<p></p>")
#print("<table>")

thecmd = ""
altcmd = ""
if id:
    idlen = len(id)
    if (idlen==12):
        # Log chunk ID
        thecmd = "./fzloghtml -q -d formalizer -s randalk -E STDOUT -D 1 -1 "+id
    else:
        if (idlen>12):
            if (id[12]=='.'):
                # Log entry ID (show the chunk)
                chunkid = id[0:12]
                thecmd = "./fzloghtml -q -d formalizer -s randalk -E STDOUT -D 1 -1 "+chunkid
            else:
                if (idlen==16):
                    # Node ID
                    thecmd = "./fzgraphhtml -q -E STDOUT -o STDOUT -n "+id
                    #thecmd = "./fzquerypq -q -d formalizer -s randalk -E STDOUT -F html -n "+id
                    if alt:
                        if (alt=="hist50"):
                            altcmd = "./fzloghtml -q -d formalizer -s randalk -o STDOUT -E STDOUT -N -c 50 -n "+id
                        else:
                            if (alt=="histfull"):
                                altcmd = "./fzloghtml -q -d formalizer -s randalk -o STDOUT -E STDOUT -N -n "+id
    

if thecmd:
    print(f'\n<!-- Primary command: {thecmd} -->\n')
    print('<br>\n<table><tbody>\n')
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

    print("</tbody></table>")

if altcmd:
    print(f'\n<!-- Secondary command: {altcmd} -->\n')
    print('<br>\n<table><tbody>\n')
    print('<br>\n\n')
    try:
        p = Popen(altcmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
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
    
    print("</tbody></table>")

    if (alt=="hist50"):
        print(f'\n<br>\n<p>\n[<a href="/cgi-bin/fzlink.py?id={id}&alt=histfull">full history</a>]\n</p>\n<br>\n')

print("<hr>\n</body>\n</html>")
