#!/usr/bin/python3
#
# Randal A. Koene, 20241114
#
# Use this to refresh Node histories locally so that the update
# is run through fzquerypq, and then web user www-data is again
# given reading rights to the new table.
# This is not needed when the refresh is call through the web
# interface using fzquerypq-cgi.py, since www-data is then the
# one who creates the table.

from sys import exit
from subprocess import Popen, PIPE
from io import StringIO
from traceback import print_exc

USERNAME='randalk'
CGIUSER='www-data'
DATABASE='formalizer'
SCHEMANAME=USERNAME
USERHOME='/home/'+USERNAME
FZQUERYPQ=USERHOME+'/.formalizer/bin/fzquerypq'

def try_call_command(thecmd: str, return_result=False):
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout) = (p.stdin, p.stdout)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        if return_result:
            return result
        print(result)
        return True

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        return False

refreshcmd=f"{FZQUERYPQ} -d {DATABASE} -s {SCHEMANAME} -E STDOUT -R histories -q"
if not try_call_command(refreshcmd):
	exit(1)

# This may print 'GRANT' to STDOUT
wwwaccesscmd=f"psql -d {DATABASE} -c 'GRANT SELECT ON {SCHEMANAME}.histories TO \"{CGIUSER}\";'"
try_call_command(wwwaccesscmd)
