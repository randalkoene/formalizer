#!/usr/bin/python3
#
# Randal A. Koene, 20241027
#
# CGI interface to the fzmetricspq core program.

try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
import re
import json
import traceback
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE

form = cgi.FieldStorage()

action = form.getvalue('action')
tablename = form.getvalue('tablename')
index = form.getvalue('index')
data = form.getvalue('data')
type = form.getvalue('type')

resultdict = {
    'stdout': '',
    'stderr': '',
}

print("Content-type: text/html\n\n")

def try_call_command(thecmd: str, print_result=True)->bool:
    global resultdict
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        err = child_stderr.read()
        child_stderr.close()
        try:
            resultdict['stdout'] = result.decode()
        except:
            try:
                resultdict['stdout'] = str(result)
            except Exception as e:
                resultdict['stdout'] = '(failed to decode stdout) : '+str(e)
        try:
            resultdict['stderr'] = err.decode()
        except:
            try:
                resultdict['stderr'] = str(err)
            except Exception as e:
                resultdict['stderr'] = '(failed to decode stderr) : '+str(e)
        if print_result:
            print('<!-- begin: call output --><pre>')
            print(result)
            print('<!-- end  : call output --></pre>')
        #print(result.replace('\n', '<BR>'))
        return True

    except Exception as ex:
        if print_result:
            print(ex)
            f = StringIO()
            print_exc(file=f)
            a = f.getvalue().splitlines()
            for line in a:
                print(line)
        else:
            resultdict['stderr'] += str(ex)
            f = StringIO()
            print_exc(file=f)
            a = f.getvalue().splitlines()
            for line in a:
                resultdict['stderr'] += str(line)
        return False

STORE_ERROR='''<html>
<body>
Error while attempting to store data.
<p>
%s
</body>
</html>
'''

STORE_RESULT='''<html>
<body>
Data stored to database.
<pre>
%s
</pre>
</body>
</html>
'''

READ_ERROR='''<html>
<body>
Error reading data.
<p>
%s
</body>
</html>
'''

def store_data_to_database(index:str, tablename:str, type:str):
    datafile = '/var/www/webdata/formalizer/database_data.json'
    try:
        if type == 'text':
            with open(datafile, 'w') as f:
                f.write('"'+data+'"')
        else: # assumes JSON
            with open(datafile, 'w') as f:
                f.write(data)
    except Exception as e:
        print(STORE_ERROR % str(e))
        return

    thecmd = './fzmetricspq -V -E STDOUT -S -i %s -G %s -g file:%s -o STDOUT' % (index, tablename, datafile)
    if not try_call_command(thecmd, print_result=False):
        print(STORE_ERROR % resultdict['stderr'])
    else:
        print(STORE_RESULT % resultdict['stdout'])
    os.remove(datafile)

def read_from_database(index:str, tablename:str):
    thecmd = './fzmetricspq -q -E STDOUT -R -i %s -G %s -F json -o STDOUT' % (index, tablename)
    if not try_call_command(thecmd, print_result=False):
        print(READ_ERROR % resultdict['stderr'])
    else:
        print(resultdict['stdout'])

if __name__ == '__main__':
    if action == 'store':
        store_data_to_database(index, tablename, type)
        sys.exit(0)

    if action == 'read':
        read_from_database(index, tablename)
        sys.exit(0)

    sys.exit(0)
