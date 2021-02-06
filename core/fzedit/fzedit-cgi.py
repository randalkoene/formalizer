#!/usr/bin/python3
#
# Randal A. Koene, 20201209
#
# CGI script to forward Node editing data to fzedit.

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

textfile = '/var/www/webdata/formalizer/node-text.html'

testingoutputstart='''Content-type:text/html

<html>
'''

testingoutputend='''Testing
</html>
'''

def cgi_testing_start():
    print(testingoutputstart)

def cgi_testing_end():
    print(testingoutputend)
    sys.exit(0)


# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

id = form.getvalue('id')

edit_result_page_head = '''Content-type:text/html

<html>
<head>
<link rel="stylesheet" href="/fz.css">
<title>fz: Edit</title>
</head>
<body>
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>
'''

edit_success_page_tail = f'''<b>Node modified. To review or edit more, follow this link: <a href="/cgi-bin/fzgraphhtml-cgi.py?edit={id}">{id}</a>.</b>
<hr>
</body>
</html>
'''

edit_fail_page_tail = '''<hr>
</body>
</html>
'''

def convert_to_targetdate(alttargetdate: str):
    atd = alttargetdate.split('T')
    atd_date = atd[0].split('-')
    atd_time = atd[1].split(':')
    atd_YmdHM = f'{atd_date[0]}{atd_date[1]}{atd_date[2]}{atd_time[0]}{atd_time[1]}'
    return atd_YmdHM


def convert_date_and_time_to_targetdate(alt2_targetdate: str, alt2_targettime: str):
    atd_date = alt2_targetdate.split('-')
    atd_time = alt2_targettime.split(':')
    atd_YmdHM = f'{atd_date[0]}{atd_date[1]}{atd_date[2]}{atd_time[0]}{atd_time[1]}'
    return atd_YmdHM


def try_call_command(thecmd: str):
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout) = (p.stdin, p.stdout)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        print('<!-- begin: call output --><pre>')
        print(result)
        print('<!-- end  : call output --></pre>')
        #print(result.replace('\n', '<BR>'))
        return True

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        return False


def modify_node():
    text = form.getvalue('text')
    comp = float(form.getvalue('comp'))
    set_complete = form.getvalue('set_complete')
    req_hrs = float(form.getvalue('req_hrs'))
    req_mins = int(form.getvalue('req_mins'))
    add_hrs = float(form.getvalue('add_hrs'))
    add_mins = int(form.getvalue('add_mins'))
    val = float(form.getvalue('val'))
    targetdate = form.getvalue('targetdate')
    alt_targetdate = form.getvalue('alt_targetdate')
    alt2_targetdate = form.getvalue('alt2_targetdate')
    alt2_targettime = form.getvalue('alt2_targettime')
    prop = form.getvalue('prop')
    patt = form.getvalue('patt')
    every = int(form.getvalue('every'))
    span = int(form.getvalue('span'))

    orig_mins = int(form.getvalue('orig_mins'))
    orig_td = form.getvalue('orig_td')

    if (set_complete=='on'):
        req_mins = int(float(orig_mins)*comp)
        comp = 1.0

    # add_hrs and add_mins are combined
    if (add_hrs != 0):
        add_mins += int(add_hrs*60.0)

    if (add_mins != 0):
        if (comp >= 0.0):
            completed_mins = int(float(orig_mins)*comp)
        req_mins = orig_mins + add_mins
        if (req_mins<0):
            req_mins = 0
        if (comp >= 0.0):
            if (completed_mins >= req_mins):
                comp = 1.0
            else:
                comp = float(completed_mins)/float(req_mins)
        
    if (orig_mins != req_mins):
        # if the value changed then we assume that req_mins is being used to set required
        req_hrs = float(req_mins)/60.0
        #req_hrs = '{:.5f}'.format(req_hrs_float)

    atd_YmdHM = convert_to_targetdate(alt_targetdate)
    if (atd_YmdHM != orig_td):
        # if the value changed then we assume that atd_YmdHM is being used to set targetdate
        targetdate = atd_YmdHM
    atd_YmdHM = convert_date_and_time_to_targetdate(alt2_targetdate,alt2_targettime)
    if (atd_YmdHM != orig_td):
        # if the value changed then we assume that atd_YmdHM is being used to set targetdate
        targetdate = atd_YmdHM

    print(edit_result_page_head)
    thisscript = os.path.realpath(__file__)
    print(f'<!--(For dev reference, this script is at {thisscript}.) -->')
    print('<!-- [Formalizer: fzedit handler]\n<p></p> -->')
    #print("<table>")

    with open(textfile,'w') as f:
        f.write(text)

    thecmd = f"./fzedit {verbosearg} -E STDOUT -M {id} -f {textfile} -c {comp:.5f} -H {req_hrs:.5f} -a {val:.5f} -t {targetdate} -p {prop} -r {patt} -e {every} -s {span}"

    print(f'<!-- Call command: {thecmd} -->')

    if try_call_command(thecmd):
        print(edit_success_page_tail)
    else:
        print(edit_fail_page_tail)


def update_node():
    tpass = form.getvalue('tpass')
    tpass_YmdHM = convert_to_targetdate(tpass)

    print(edit_result_page_head)
    thisscript = os.path.realpath(__file__)
    print(f'<!--(For dev reference, this script is at {thisscript}.) -->')
    print('<!-- [Formalizer: fzedit handler]\n<p></p> -->')

    thecmd = f"./fzgraph {verbosearg} -E STDOUT -C '/fz/graph/nodes/{id}?skip=toT&T={tpass_YmdHM}'"

    print(f'<!-- Call command: {thecmd} -->')
    
    print(f'Skipping instances of Node {id} past {tpass_YmdHM}.')

    if try_call_command(thecmd):
        print(edit_success_page_tail)
    else:
        print(edit_fail_page_tail)


def skip_node():
    num_skip = form.getvalue('num_skip')

    print(edit_result_page_head)
    thisscript = os.path.realpath(__file__)
    print(f'<!--(For dev reference, this script is at {thisscript}.) -->')
    print('<!-- [Formalizer: fzedit handler]\n<p></p> -->')

    thecmd = f"./fzgraph {verbosearg} -E STDOUT -C '/fz/graph/nodes/{id}?skip={num_skip}'"

    print(f'<!-- Call command: {thecmd} -->')
    
    print(f'Skipping {num_skip} instances of Node {id}.')

    if try_call_command(thecmd):
        print(edit_success_page_tail)
    else:
        print(edit_fail_page_tail)


if __name__ == '__main__':

    global verbosearg
    action = form.getvalue('action')
    verbosity = form.getvalue('verbosity')
    if (verbosity == "verbose"):
        verbosearg = '-V'
    else:
        verbosearg = '-q'

    if (action=='modify'):
        modify_node()
    else:
        if (action=='update'):
            update_node()
        else:
            if (action=='skip'):
                skip_node()
            else:
                print(edit_result_page_head)
                print(f'<p><b>Unrecognized Node edit action: {action}</b><p>')
                print(edit_fail_page_tail)

    sys.exit(0)
