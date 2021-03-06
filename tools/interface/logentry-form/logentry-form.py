#!/usr/bin/python3
#
# logentry-form.py
#
# Randal A. Koene, 20210304
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

cgiwritabledir = "/var/www/webdata/formalizer/"

config = {}
config['verbose'] = True
config['logcmdcalls'] = False

results = {}

# We need this everywhere to run various shell commands.
def try_subprocess_check_output(thecmdstring, resstore):
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
        if result:
            print(result)
        #print(result.replace('\n', '<BR>'))
        return 0

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        return 1


def send_to_fzlog(logentrytmpfile: str, node = None):
    thecmd=f"./fzlog -e -E STDOUT -d formalizer -s randalk -f {logentrytmpfile}"
    #thecmd='./fzlog -h -V -E STDOUT -W STDOUT'
    # *** Probably add this as in logentry.py: node = check_same_as_chunk(node)
    if node:
        thecmd += f" -n {node}"
    retcode = try_subprocess_check_output(thecmd, 'fzlog_res')
    if (retcode != 0):
        print('<p><b>Attempt to add Log entry via fzlog failed.</b></p>')
    else:
        print('<p><b>Entry added to Log.</b></p>')


if __name__ == '__main__':
    print("Content-type:text/html\n\n")
    print("<html>")
    print("<head>")
    print('<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">')
    print("<title>Prototype: Submitting Log Entry to fzlog</title>")
    print("</head>")
    print("<body>")

    form = cgi.FieldStorage()
    entrytext = form.getvalue("entrytext")

    if not entrytext:
        print('<p><b>No Log entry text submitted.</b></p>')
        print("</body>")
        print("</html>")
        sys.exit(0)

    if entrytext:
        # making sure the files are group writable
        logentrytextfile = cgiwritabledir+"logentry-text.html"
        with open(os.open(logentrytextfile, os.O_CREAT | os.O_WRONLY, 0o664),"w") as f:
            f.write(entrytext)
        os.chmod(logentrytextfile,stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IWGRP | stat.S_IROTH )

        send_to_fzlog(logentrytextfile)

    print("</body>")
    print("</html>")

    sys.exit(0)
