#!/usr/bin/python3
#
# md2html-cgi.py
#
# Randal A. Koene, 20250827
#
# CGI interface to md2html.

try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
#import traceback
#from io import StringIO
#from traceback import print_exc
import subprocess

form = cgi.FieldStorage()

data = form.getvalue('markdown_text')

mdfilepath = '/var/www/webdata/formalizer/mdfile.html'
with open(mdfilepath, 'w') as f:
    f.write(data)

# Print the HTTP header
print("Content-Type: text/plain\n")

# Copied from fzcmdcalls.py to simplify using this as CGI script.
results = {}
def try_subprocess_check_output(thecmdstring, resstore, config:dict=None):
    if config is None:
        config= {
            'verbose': False,
            'logcmdcalls': False,
            'cmdlog': '',
            'logcmderrors': False,
            'cmderrlog': '',
        }

    results['error'] = ''
    if config['verbose']:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
    if config['logcmdcalls']:
        with open(config['cmdlog'],'a') as f:
            f.write(thecmdstring+'\n')
    try:
        res = subprocess.check_output(thecmdstring, shell=True)

    except Exception as e:
        try:
            results['error'] = str(e)
        except:
            pass
        return 1
    else:
        if resstore:
            results[resstore] = res
        if config['verbose']:
            print('Result of subprocess call:', flush=True)
            print(res.decode(), flush=True)
        return 0


thecmd = "./md2html --github "+mdfilepath
retcode = try_subprocess_check_output(thecmd,'html_content')
if retcode != 0:
    print('Failed to run md2html: '+str(results['error']))
    sys.exit(0)

print(results['html_content'].decode())
