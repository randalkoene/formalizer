#!/usr/bin/python3
#
# logentry-form.py
#
# Randal A. Koene, 20201226
#
# This is the version of the script that was created for use with the dil2al polldaemon
# during the early transition phase from Formalizer 1.x to Formalizer 2.0.
#
# Generate Log entry forms for stand-alone or embedded use. This can generate the
# initial form, as well as its updates as Node is selected, etc.

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


if __name__ == '__main__':
    print("Content-type:text/html\n\n")
    print("<html>")
    print("<head>")
    print('<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">')
    print("<title>Test Prototype: Request a Node and Log Chunk</title>")
    print("</head>")
    print("<body>")

    form = cgi.FieldStorage()
    entrytext = form.getvalue("entrytext")


    if entrytext:
        try:
            polldaemondir = "/var/www/webdata/dil2al-polldaemon/"
            if os.path.isfile(polldaemondir+"dil2al_poller_response"):
                os.remove(polldaemondir+"dil2al_poller_response")

            # making sure the files are group writable
            with open(os.open(polldaemondir+"dil2al_poller_data", os.O_CREAT | os.O_WRONLY, 0o664),"w") as dil2alpollerdata:
                dil2alpollerdata.write(entrytext)
            os.chmod(polldaemondir+"dil2al_poller_data",stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IWGRP | stat.S_IROTH )

            with open(os.open(polldaemondir+"dil2al_poller_request", os.O_CREAT | os.O_WRONLY, 0o664), "w") as dil2alpollerfile:
                dil2alpollerfile.write('makenote')
            os.chmod(polldaemondir+"dil2al_poller_request",stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IWGRP | stat.S_IROTH )

            print("Log entry text received. Sending request to dil2al polled daemon.<br>\n")
            seconds = 0
            while not (os.path.isfile(polldaemondir+"dil2al_poller_response") or (seconds > 9)):
                time.sleep(1)
                print("Waiting for response...<br>")
                seconds += 1

            if os.path.isfile(polldaemondir+"dil2al_poller_response"):
                print("The response by dil2al polled daemon is:<br>\n")
                with open(polldaemondir+"dil2al_poller_response") as f:
                    response = f.read()
                    print(response)

            else:
                print("Request timed out. The dil2al polled daemon did not respond.<br>\n")

        except Exception as ex:                
            print(ex)
            f = StringIO()
            print_exc(file=f)
            a = f.getvalue().splitlines()
            for line in a:
                print(line)

    print("</body>")
    print("</html>")
