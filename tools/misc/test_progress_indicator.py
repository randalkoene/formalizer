#!/usr/bin/python3
#
# Randal A. Koene, 20240106
#
# This CGI handler tests displaying a progress indicator via REST API.
#

# Do this immediately, so that any errors are visible:
print("Content-type:text/html\n\n")

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
from os.path import exists
sys.stderr = sys.stdout
from time import strftime
import datetime
import traceback
from json import loads, load
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE
from pathlib import Path

from fzmodbase import *
from fzbgprogress import *

home = str(Path.home())

form = cgi.FieldStorage()

launch = form.getvalue('launch')
progress = form.getvalue('progress')
done = form.getvalue('done')
method = form.getvalue('method')

#if not method:
#    method='reloads'

progress_file = '/var/www/webdata/formalizer/test_progress.state'
bg_logfile = '/var/www/webdata/formalizer/test_background_process.log'

def try_command_call(thecmd, print_result=True)->str:
    try:
        #print(thecmd, flush=True)
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
        child_stdin.close()
        result = child_stdout.read()
        error = child_stderr.read()
        child_stdout.close()
        child_stderr.close()
        if print_result:
            print(result)
            return ''
        if len(error)>0:
            print(error)
        #print(result.replace('\n', '<BR>'))
        return result

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        return ''

REDIRECT_TO_SELF='''
<html>
<meta http-equiv="Refresh" content="%s; url='/cgi-bin/test_progress_indicator.py?%s'" />
</html>
'''

UNRECOGNIZED_METHOD='''
<html>
<body>
Unrecognized method: %s
</body>
</html>
'''

DONE='''
<html>
<body>
Done.
</body>
</html>
'''

UNIMPLEMENTED='''
<html>
<body>
Unimplemented.
</body>
</html>
'''

def do_launch():
    thecmd = f'./test_background_process.py >{bg_logfile} 2>&1 &'
    res = try_command_call(thecmd, print_result=False)
    first_progress_request = 'method=%s&progress=0' % method
    print(REDIRECT_TO_SELF % ('0', first_progress_request))

def get_progress()->str:
    try:
        with open(progress_file,'r') as f:
            return f.read()
    except:
        return progress # previous progress value

def clear_progress_file():
    try:
        with open(progress_file,'w') as f:
            f.write('0')
    except:
        pass

def do_reloads_progress():
    progress_value = get_progress()[0:3]
    if progress_value == '100':
        print(REDIRECT_TO_SELF % ('0', 'done=true'))
        return
    refresh_wait='1'
    PROGRESS=f'''
<html>
<meta http-equiv="Refresh" content="{refresh_wait}; url='/cgi-bin/test_progress_indicator.py?method=reloads&progress={progress_value}'" />
<body>
<h3>Processing in background (method: reloads)</h3>
<progress value="{progress_value}" max="100">{progress_value} %</progress>
</body>
</html>
'''

    print(PROGRESS)
    #print(PROGRESS % ('1', progress_value, progress_value, progress_value))

JAVASCRIPT_PROGRAM='''
<html>
<body>
<h3>Processing in background (method: javascript)</h3>
<div id='result'>
<progress id="progress" value="0" max="100">0 %</progress>
</div>
<script>
function loadFile(filePath) {
  var result = null;
  var xmlhttp = new XMLHttpRequest();
  xmlhttp.open("GET", filePath, false);
  xmlhttp.send();
  if (xmlhttp.status==200) {
    result = xmlhttp.responseText;
  }
  return result;
}

const sleep = (delay) => new Promise((resolve) => setTimeout(resolve, delay));

const progressloop = async () => {
  var result=0;
  var progress = 0;
  /* for (var i=0; i<100; i+=10) { */
  var progressbar_ref = document.getElementById('progress');
  while (progress < 100) {
      await sleep(1000);
      var new_progress = loadFile('/formalizer/data/test_progress.state');
      if (new_progress) {
        progress = new_progress;
      }
      progressbar_ref.value = `${progress}`;
      progressbar_ref.innerHTML = `${progress} %`;
  }
  document.getElementById('result').innerHTML = "Done.";
}

progressloop();

</script>
</body>
</html>
'''

def do_javascript_progress():
    print(JAVASCRIPT_PROGRAM)

FZ_PROGRESS_PAGE='''
<html>
<body>
<h3>Processing in background (method: fz)</h3>
%s
<script>
%s

progressloop();

</script>
</body>
</html>
'''

def do_fz_progress():
    embed_in_html, embed_in_script = make_background_progress_monitor('test_progress.state', '/cgi-bin/test_progress_indicator.py?done=true')
    print(FZ_PROGRESS_PAGE % (embed_in_html, embed_in_script))

def do_progress():
    if method == 'reloads':
        do_reloads_progress()
        return
    elif method == 'javascript':
        do_javascript_progress()
        return
    elif method == 'fz':
        do_fz_progress()
        return
    print(UNRECOGNIZED_METHOD % method)

def do_done():
    clear_progress_file()
    print(DONE)

if __name__ == '__main__':

    if launch:
        do_launch()
        sys.exit(0)

    if progress:
        do_progress()
        sys.exit(0)

    if done:
        do_done()
        sys.exit(0)

    sys.exit(0)
