#!/usr/bin/python3
#
# Randal A. Koene, 20201026
#
# This CGI handler provides a near-verbatim equivalent access to fzgraph via web form.

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os, stat
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

cmdoptions = ""

# Get data from fields
nodetext = form.getvalue('text')
valuation = form.getvalue('valuation')
required = form.getvalue('required')
targetdate = form.getvalue('targetdate')
tdproperty = form.getvalue('tdproperty')
tdevery = form.getvalue('tdevery')
tdpatterns = form.getvalue('tdpattern')
tdspan = form.getvalue('tdspan')
topics = form.getvalue('topics')
superiors = form.getvalue('superiors')
dependencies = form.getvalue('dependencies')

if valuation:
    cmdoptions += ' -a '+valuation
if required:
    cmdoptions += ' -H '+required
if targetdate:
    cmdoptions += ' -t '+targetdate
if tdproperty:
    cmdoptions += ' -p '+tdproperty
if tdevery:
    cmdoptions += ' -e '+tdevery
if tdpatterns:
    cmdoptions += ' -r '+tdpatterns
if tdspan:
    cmdoptions += ' -s '+tdspan
if topics:
    cmdoptions += ' -g "'+topics+'"'
if superiors:
    cmdoptions += ' -S "'+superiors+'"'
if dependencies:
    cmdoptions += ' -D "'+dependencies+'"'

nodetextdir = "/var/www/webdata/formalizer/"
if os.path.isfile(nodetextdir+"nodetext.html"):
    os.remove(nodetextdir+"nodetext.html")

if nodetext:
    try:
        # making sure the files are group writable
        with open(os.open(nodetextdir+"nodetext.html", os.O_CREAT | os.O_WRONLY, 0o664),"w") as nodetextfile:
            nodetextfile.write(nodetext)
        os.chmod(nodetextdir+"nodetext.html",stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IWGRP | stat.S_IROTH )

        #print("Node content received. Sending request to fzgraph.<br>\n")
        cmdoptions += ' -f '+nodetextdir+'nodetext.html'

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)


thecmd = "./fzgraph -q -E STDOUT -M node "+cmdoptions
try:
    p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
    (child_stdin,child_stdout) = (p.stdin, p.stdout)
    child_stdin.close()
    result = child_stdout.read()
    child_stdout.close()
    print(result)

    # GET THE NODE ID in id, THEN DISPLAY RESULTS WITH THE FOLLOWING
    print("Content-type:text/html\n\n")

    print("<html>")
    print("<head>")
    print('<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">')
    print("<title>Formalizer: HTML FORM interface to fzloghtml</title>")
    print("</head>")
    print("<body>")
    print('<style type="text/css">')
    print('.chktop { ')
    print('    background-color: #B0C4F5;')
    print('}')
    #print("table tr.chktop { background: #B0C4F5; }")
    print("</style>")

    thisscript = os.path.realpath(__file__)
    print(f'(For dev reference, this script is at {thisscript}.)')

    print("<h1>Formalizer: fzgraph Add Node result</h1>\n<p></p>")
    print("<table><tbody>")

    if id:
        thecmd = "./fzgraphhtml -q -E STDOUT -o STDOUT -n "+id
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
    print("</body>")
    print("</html>")


except Exception as ex:                
    print(ex)
    f = StringIO()
    print_exc(file=f)
    a = f.getvalue().splitlines()
    for line in a:
        print(line)

sys.exit(0)
