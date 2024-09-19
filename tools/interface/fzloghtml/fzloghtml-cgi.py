#!/usr/bin/python3
#
# Randal A. Koene, 20200921
#
# This CGI handler provides a near-verbatim equivalent access to fzloghtml via web form.

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

from fzmodbase import *
from TimeStamp import add_days_to_TimeStamp, add_weeks_to_TimeStamp, NowTimeStamp

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
alloflog = form.getvalue('alloflog')
startfrom = form.getvalue('startfrom')
endbefore  = form.getvalue('endbefore')
around = form.getvalue('around')
daysinterval  = form.getvalue('daysinterval')
weeksinterval  = form.getvalue('weeksinterval')
hoursinterval  = form.getvalue('hoursinterval')
numchunks = form.getvalue('numchunks')
node = form.getvalue('node')
frommostrecent = form.getvalue('frommostrecent')
mostrecentdata = form.getvalue('mostrecentdata')
searchtext = form.getvalue('searchtext')
andall = form.getvalue('andall')
caseinsensitive = form.getvalue('caseinsensitive')
review = form.getvalue('review')
review_date = form.getvalue('reviewdate')

#if not review:
#    print("Content-type:text/html\n\n")
print("Content-type:text/html\n\n")

if alloflog:
    startfrom = "199001010000"
    endbefore = NowTimeStamp()

try:
    if searchtext:
        with open('/var/www/webdata/formalizer/searchtext.test','w') as f:
            f.write(searchtext)
except:
    pass

#<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">
log_interval_head = '''<html>
<head>
<meta charset="utf-8" />
<link rel="stylesheet" href="/fz.css">
<link rel="icon" href="/favicon-logentry-32x32.png">
<link rel="stylesheet" href="/fzuistate.css">
<link rel="stylesheet" href="/clock.css">
<link rel="stylesheet" href="/score.css">
<meta http-equiv="cache-control" content="no-cache" />
<title>fz: Log interval</title>
</head>
<body>
<script type="text/javascript" src="/clock.js"></script>
<script type="text/javascript" src="/score.js"></script>
<script type="text/javascript" src="/fzuistate.js"></script>
'''

cgi_custom_tail = '''
<p>Enter HTML text here to append a Log Entry:
<form action="/cgi-bin/logentry-form.py" method="post"><input type="hidden" name="showrecent" value="on">
<textarea rows="10" cols="100" name="entrytext"></textarea><br>
Add entry for <input type="submit" name="makeentry" value="Log Chunk Node" /> or <input type="submit" name="makeentry" value="Other Node" /> | <input type="submit" name="makeentry" value="Templates" /> | [<a href="/cgi-bin/metrictags.py" target="_blank">Show Metric Tags</a>].
</form>
</p>

<hr>

Select another part of the Log:
<form action="/cgi-bin/fzloghtml-cgi.py" method="post">
<input type="checkbox" name="alloflog"> all of the Log<br />
<input type="text" name="startfrom"> start at this YYYYmmddHHMM time stamp (default: 24 hours before end of interval)<br />
<input type="text" name="endbefore"> end before this YYYYmmddHHMM time stamp (default: 1 second after most recent Log entry)<br />
<input type="text" name="daysinterval"> interval size in days<br />
<input type="text" name="weeksinterval"> interval size in weeks<br />
<input type="text" name="hoursinterval"> interval size in hours<br />
<input type="text" name="numchunks"> number of Log chunks (interval takes precedence)<br />
<input type="checkbox" name="frommostrecent"> from most recent (interval takes precedence)<br />
<input type="text" name="node"> belongs to node, node history<br />
<input type="text" name="searchtext" size="48"> must contain search text
<input type="checkbox" name="caseinsensitive"> case insensitive
<input type="checkbox" name="andall"> must contain each<br />
or<br />
<input type="checkbox" name="mostrecentdata"> show most recent Log data summary<br />

<input type="submit" value="Submit" />

<hr>

<p>[<a href="/index.html">fz: Top</a>] <span id="logautoupdate">_</span></p>

<script type="text/javascript" src="/logautoupdate.js"></script>
'''


def render_most_recent():
    thecmd = "./fzloghtml -q -d formalizer -s randalk -o STDOUT -E STDOUT -R"
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


def get_uri_arg_separator(arg_i:int)->str:
    if arg_i==0:
        return '?'
    else:
        return '&'

def is_before_now(endbefore:str)->bool:
    d_now = datetime.now()
    if len(endbefore)==8:
        endbefore += "0000"
    d_endbefore = datetime.strptime(endbefore, '%Y%m%d%H%M')
    return d_endbefore < d_now

def uri_safe(arg:str)->str:
    safe_arg=''
    for i in range(len(arg)):
        if arg[i]==' ':
            safe_arg += '%20'
        else:
            safe_arg += arg[i]
    return safe_arg

def build_uri_options(diff_numchunks=0, diff_startfrom=0, diff_weeks_startfrom=0, diff_endbefore=0, diff_weeks_endbefore=0)->str:
    urioptions = ""

    arg_i=0
    if around:
        urioptions += get_uri_arg_separator(arg_i)+'around='+around
        arg_i += 1
    if startfrom:
        new_startfrom=startfrom
        if diff_startfrom!=0:
            new_startfrom = add_days_to_TimeStamp(tstamp=startfrom, numdays=diff_startfrom)
        if diff_weeks_startfrom!=0:
            new_startfrom = add_weeks_to_TimeStamp(tstamp=startfrom, numweeks=diff_weeks_startfrom)
        urioptions += get_uri_arg_separator(arg_i)+'startfrom='+new_startfrom
        arg_i += 1
    if endbefore:
        new_endbefore=endbefore
        if diff_endbefore!=0:
            new_endbefore = add_days_to_TimeStamp(tstamp=endbefore, numdays=diff_endbefore)
        if diff_weeks_endbefore!=0:
            new_endbefore = add_weeks_to_TimeStamp(tstamp=endbefore, numweeks=diff_weeks_endbefore)
        urioptions += get_uri_arg_separator(arg_i)+'endbefore='+new_endbefore
        arg_i += 1
    if daysinterval:
        urioptions += get_uri_arg_separator(arg_i)+'daysinterval='+daysinterval
        arg_i += 1
    if weeksinterval:
        urioptions += get_uri_arg_separator(arg_i)+'weeksinterval='+weeksinterval
        arg_i += 1
    if hoursinterval:
        urioptions += get_uri_arg_separator(arg_i)+'hoursinterval='+hoursinterval
        arg_i += 1
    if numchunks:
        new_numchunks=int(numchunks)
        if diff_numchunks!=0:
            new_numchunks += diff_numchunks
        urioptions += get_uri_arg_separator(arg_i)+'numchunks='+str(new_numchunks)
        arg_i += 1
    if frommostrecent:
        urioptions += get_uri_arg_separator(arg_i)+'frommostrecent=on'
        arg_i += 1
    if node:
        urioptions += get_uri_arg_separator(arg_i)+'node='+node
        arg_i += 1
    if searchtext:
        urioptions += get_uri_arg_separator(arg_i)+'searchtext='+uri_safe(searchtext)
    if andall:
        urioptions += get_uri_arg_separator(arg_i)+'andall=on'
        arg_i += 1
    if caseinsensitive:
        urioptions += get_uri_arg_separator(arg_i)+'caseinsensitive=on'
        arg_i += 1

    return urioptions

def build_command_options()->str:
    cmdoptions = ""

    if around:
        cmdoptions += ' -a '+around
    if startfrom:
        cmdoptions += ' -1 '+startfrom
    if endbefore:
        cmdoptions += ' -2 '+endbefore
    if daysinterval:
        cmdoptions += ' -D '+daysinterval
    if weeksinterval:
        cmdoptions += ' -w '+weeksinterval
    if hoursinterval:
        cmdoptions += ' -H '+hoursinterval
    if numchunks:
        cmdoptions += ' -c '+numchunks
    if frommostrecent:
        cmdoptions += ' -r '
    if node:
        cmdoptions += ' -n '+node
    if searchtext:
        cmdoptions += ' -f "'+searchtext+'"'
    if andall:
        cmdoptions += ' -A '
    if caseinsensitive:
        cmdoptions += ' -C '

    return cmdoptions

def render_log_interval():
    print(log_interval_head)
    thisscript = os.path.realpath(__file__)
    print(f'<!-- (For dev reference, this script is at {thisscript}.) -->')
    print('<h1>Formalizer: HTML FORM interface to fzloghtml</h1>\n<p></p>\n<table><tbody>')
    if numchunks:
        earlier_uri = build_uri_options(diff_numchunks=50)
        print(f"<button class=\"button button1\" onclick=\"window.open('/cgi-bin/fzloghtml-cgi.py{earlier_uri}','_self');\">Earlier (50 Chunks)</button>")
        evenearlier_uri = build_uri_options(diff_numchunks=250)
        print(f"<button class=\"button button2\" onclick=\"window.open('/cgi-bin/fzloghtml-cgi.py{evenearlier_uri}','_self');\">Earlier (250 Chunks)</button>")
    if startfrom:
        earlier_uri = build_uri_options(diff_startfrom=-10)
        print(f"<button class=\"button button1\" onclick=\"window.open('/cgi-bin/fzloghtml-cgi.py{earlier_uri}','_self');\">Earlier (10 days)</button>")
        evenearlier_uri = build_uri_options(diff_weeks_startfrom=-5)
        print(f"<button class=\"button button2\" onclick=\"window.open('/cgi-bin/fzloghtml-cgi.py{evenearlier_uri}','_self');\">Earlier (5 weeks)</button>")

    cmdoptions = build_command_options()

    if cmdoptions:
        thecmd = "./fzloghtml -q -d formalizer -s randalk -o STDOUT -E STDOUT -N "+cmdoptions
        print(f'<!-- Using this command: {thecmd} -->')
        print('<br>\n')
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
    else:
        print('<tr><td><b>Missing request arguments.</b></td></tr>')

    print('</tbody></table>')

    if endbefore:
        if is_before_now(endbefore):
            later_uri = build_uri_options(diff_endbefore=10)
            print(f"<button class=\"button button1\" onclick=\"window.open('/cgi-bin/fzloghtml-cgi.py{later_uri}','_self');\">Later (10 days)</button>")
            evenlater_uri = build_uri_options(diff_weeks_endbefore=5)
            print(f"<button class=\"button button2\" onclick=\"window.open('/cgi-bin/fzloghtml-cgi.py{evenlater_uri}','_self');\">Later (5 weeks)</button>")

    print('<a name="END">&nbsp;</a>')
    print(cgi_custom_tail)
    print("</body>\n</html>")

def render_Log_review():
    if review_date:
        date_specific = '-a %s' % reviewdate
    else:
        date_specific = ''
    thecmd = f"./fzloghtml -q -d formalizer -s randalk {date_specific} -D 2 -i -F html -o STDOUT -E STDOUT"
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


if __name__ == '__main__':

    if review:
        render_Log_review()
    else:
        if mostrecentdata:
            render_most_recent()
        else:
            render_log_interval()

    sys.exit(0)
