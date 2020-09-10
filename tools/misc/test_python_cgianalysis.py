#!/usr/bin/python3

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


if __name__ == '__main__':
    print("Content-type: text/html")
    print("")

    form = cgi.FieldStorage()
    # the following line expects the command to be provided from form input
    thecmd = form.getvalue('the_cmd')
    #thecmd = "./test_python_cgisubprocess.py" # this is your command

    if thecmd:
        print('<HR><BR><BR>')
        print('<B>Command : ', thecmd, '<BR><BR>')
        print('Result : <BR><BR><PRE>')
        try:
            p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
            (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
            child_stdin.close()
            result = child_stdout.read()
            child_stdout.close()
            errresult = child_stderr.read()
            child_stderr.close()
            print(result.replace('<', '&lt;').replace('>','&gt;'))
            print('</PRE>\n<HR>\nSTDERR output:\n<BR>\n<PRE>\n')
            print(errresult) # .replace('\n', '<BR>'))

        except Exception as ex:                
            print(ex)
            f = StringIO()
            print_exc(file=f)
            a = f.getvalue().splitlines()
            for line in a:
                print(line)

        print('\n</PRE>\n<HR>\n')

