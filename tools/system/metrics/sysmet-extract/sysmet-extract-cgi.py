#!/usr/bin/python3
#
# Randal A. Koene, 20210308
#
# This CGI handler displays the simple terminal System Metrics extractions in an HTML page.
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

showmetricsfile = webdata_path+'/sysmet-extract.html'
intentionscategoriesfile = "/var/www/webdata/formalizer/categories_a2c.json"
workcategoriesfile = "/var/www/webdata/formalizer/categories_work.json"

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
categories = form.getvalue('categories')

# *** OBTAIN THIS SOMEHOW!
#with open('./server_address','r') as f:
#    fzserverpq_addrport = f.read()

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


# def log(msg):
#     with open(logfile,'w') as f:
#         f.write(msg)


def show_intentions_categories():
    print("Content-type:text/html\n\n")
    thecmd = f"./fzlogmap -D 7 -r -o STDOUT -q -C -f {intentionscategoriesfile} | ./aha -b > {showmetricsfile}"
    try_command_call(thecmd)
    print(f"<!-- The command: {thecmd}  -->")
    with open(showmetricsfile, "r") as f:
        filecontent = f.read()
    print(filecontent)


def show_work_categories():
    print("Content-type:text/html\n\n")
    thecmd = f"./fzlogmap -D 7 -r -o STDOUT -q -f {workcategoriesfile} | ./aha -b > {showmetricsfile}"
    try_command_call(thecmd)
    print(f"<!-- The command: {thecmd}  -->")
    with open(showmetricsfile, "r") as f:
        filecontent = f.read()
    print(filecontent)


if __name__ == '__main__':
    if (categories == 'work'):
        show_work_categories()
        sys.exit(0)

    show_intentions_categories()
    sys.exit(0)
