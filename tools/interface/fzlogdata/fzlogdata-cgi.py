#!/usr/bin/python3
#
# Randal A. Koene, 20240418
#
# This CGI handler provides a near-verbatim equivalent access to fzlogdata via web form.

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
from time import strftime
from datetime import datetime
import traceback
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE

config = {
    'verbose': False,
    'logcmdcalls': False,
    'cmdlog': '/var/www/webdata/formalizer/fzlogdata-cgi.log'
}

results = {}

print("Content-type:text/html\n\n")

def try_subprocess_check_output(thecmdstring, resstore):
    if config['verbose']:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
        print('<pre>')
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
            print('</pre>')
        return 0

    except Exception as ex:
        print('<pre>')
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        print('</pre>')
        return 1

def show_full_Log_data():
    thecmd = './fzlogdata -q -E STDOUT -d formalizer -s randalk -I -F html -o STDOUT'
    res = try_subprocess_check_output(thecmd, resstore='logdata_html')
    #print('<html><body>res = %s</body></html>' % str(res))
    if res!=0:
        print('Call to fzlogdata failed.')
        sys.exit(1)
    if 'logdata_html' not in results:
        print('Missing logdata_html result.')
        sys.exit(1)
    if isinstance(results['logdata_html'], str):
        print(results['logdata_html'])
    else:
        try:
            print(results['logdata_html'].decode())
        except Exception as e:
            print('Exception: '+str(e))
            sys.exit(1)

if __name__ == '__main__':
    form = cgi.FieldStorage()
    full = form.getvalue("full")
    summary = form.getvalue("summary")

    if full:
        show_full_Log_data()

    sys.exit(0)
