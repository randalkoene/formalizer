#!/usr/bin/python3
#
# Randal A. Koene, 20200921
#
# This CGI handler provides a near-verbatim equivalent access to fzloghtml via web form.
#
# Notes:
#
# 1. Checkboxes processing is now done through a pipe from fzloghtml to the checkboxes.py
#    script. To use the previous call via this script, call this script in the browser
#    with a URI such as: /cgi-bin/fzloghtml-cgi.py?weeksinterval=24&regex=%5B%3C%5Dinput%20type%3D%22checkbox%22%5B%20%5D%2A%5B%3E%5D
#    decoded, that URI is the equivalent of calling fzloghtml with weeksinterval=24&regex=[<]input type="checkbox"[ ]*[>]
#    and that is the equivalent of calling fzloghtml with -w 24 -x '[<]input type="checkbox"[ ]*[>]'.

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
topic = form.getvalue('topic')
nnl = form.getvalue('nnl')
frommostrecent = form.getvalue('frommostrecent')
mostrecentdata = form.getvalue('mostrecentdata')
mostrecentraw = form.getvalue('mostrecentraw')
searchtext = form.getvalue('searchtext')
regexpattern = form.getvalue('regex')
andall = form.getvalue('andall')
caseinsensitive = form.getvalue('caseinsensitive')
review = form.getvalue('review')
review_date = form.getvalue('reviewdate')
regen_index = form.getvalue('index')

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
<link rel="stylesheet" href="/copiedalert.css">
<link rel="stylesheet" href="/htmltemplatestocopy.css">
<link rel="stylesheet" href="/tooltip.css">
<meta http-equiv="cache-control" content="no-cache" />
<title>fz: Log interval</title>
<style>
.logstate {
position: fixed;
top: 120px;
right: 0px;
font-size: 34px;
font-family: calibri;
}
#protocol_tab {
position: fixed;
top: 185px;
right: 0px;
width: 300px;
height: 500px;
display: block;
text-align: right;
}
.prot_tip {
width: 300px;
top: 100%;
right: 50%;
}
</style>
</head>
<body>
<script type="text/javascript" src="/clock.js"></script>
<script type="text/javascript" src="/score.js"></script>
<script type="text/javascript" src="/fzuistate.js"></script>
<button id="timerBarText" class="button button2 logstate">_____</button>

<div id="protocol_tab">
<button class="button button1" onclick="window.open('/cgi-bin/schedule-cgi.py?c=true&num_days=7&s=20', '_blank');">Calendar Schedule</button><br>
<span class="alt_tooltip">Distractions off <input type="checkbox">
<span class="alt_tooltiptext prot_tip"><div>Turn off video distractions.</div></span>
</span><br>
<span class="alt_tooltip">Brush away emotion <input type="checkbox">
<span class="alt_tooltiptext prot_tip"><div>Literally, brush away emotions, especially anxiety.</div></span>
</span><br>
<span class="alt_tooltip">Just get started <input type="checkbox">
<span class="alt_tooltiptext prot_tip"><div>Just get started on a Node, you don't need to complete it right away, you just need to figure out what to do and take notes about that to get started.</div></span>
</span><br>
<button class="button button2" onclick="window.open('/cgi-bin/orderscore-cgi.py', '_blank');">OrderScore</button><br>
<button class="button button1" onclick="window.open('/cgi-bin/fzloghtml-cgi.py?review=today', '_blank');">Today Review</button><br>
<button class="button button2" onclick="window.open('/cgi-bin/nodeboard-cgi.py?D=week_main_goals&T=true&u=204512311159&r=100&U=true', '_blank');">Week Goals</button>
</div>

<script type="text/javascript" src="/stateoflog.js"></script>
<script type="text/javascript" src="/hoveropentab.js"></script>
<script>
const state_of_log = new logState(logstate_id="timerBarText");
const protocol_tab = new HoverOpenTab('timerBarText', 'protocol_tab');
</script>
'''

cgi_custom_tail = '''
<p>Enter HTML text here to append a Log Entry:
<form action="/cgi-bin/logentry-form.py" method="post"><input type="hidden" name="showrecent" value="on">
<textarea id="entrytext" rows="10" cols="100" name="entrytext"></textarea><br>
Add entry for <input type="submit" name="makeentry" value="Log Chunk Node" /> or <input type="submit" name="makeentry" value="Other Node" /> | <input type="submit" name="makeentry" value="Templates" /> | [<a href="/cgi-bin/metrictags.py" target="_blank">Show Metric Tags</a>].
</form>
</p>

<div id="after_cptplt"></div>
<div id="templates_html" style="display:none;">
<button class="button2" onclick="copyInnerHTMLToClipboard('chkbx_tpl');">copy</button> <span id="chkbx_tpl"><input type="checkbox" ></span>
<button class="button2" onclick="copyInnerHTMLToClipboard('lnkblank_tpl');">copy</button> <span id="lnkblank_tpl"><a href="URL" class="docpopupfunc" target="_blank">target</a></span>
<button class="button2" onclick="copyInnerHTMLToClipboard('lnklocaldoc_tpl');">copy</button> <span id="lnklocaldoc_tpl"><a href="@FZSERVER@/doc/" target="_blank">~/doc/path</a></span>
<button class="button2" onclick="copyInnerHTMLToClipboard('lnklocalwww_tpl');">copy</button> <span id="lnklocalwww_tpl"><a href="@FZSERVER@/" target="_blank">/var/www/html/path</a></span>
<button class="button2" onclick="copyInnerHTMLToClipboard('img_tpl');">copy</button> <span id="img_tpl"><img class="hoverdelayfunc" src="/favicon.png" width=32px height=32px></span>
<button class="button2" onclick="copyInnerHTMLToClipboard('acttype_tpl');">copy</button> <span id="acttype_tpl">@ACTTYPE=A100@</span>
<p></p>
</div>
<script type="text/javascript" src="/htmltemplatestocopy.js"></script>
<script>const htmltemplates = new htmlTemplates('after_cptplt', 'cptplt', 'templates', 'templates_html');</script>
<script type="text/javascript" src="/copiedalert.js"></script>
<script type="text/javascript" src="/copyinnerhtml.js"></script>

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
<input type="text" name="topic"> belongs to topic<br />
<input type="text" name="nnl"> belongs to NNL<br />
<input type="text" name="searchtext" size="48"> must contain search text
<input type="checkbox" name="caseinsensitive"> case insensitive
<input type="checkbox" name="andall"> must contain each<br />
<input type="text" name="regex" size="48"> must contain RegEx<br />
or<br />
<input type="checkbox" name="mostrecentdata"> show most recent Log data summary<br />

<input type="submit" value="Submit" />

<hr>

<p>[<a href="/index.html">fz: Top</a>] <span id="logautoupdate">_</span></p>

<script type="text/javascript" src="/delayedpopup.js"></script>
<script>
set_hover_delayed_function('.hoverdelayfunc', enlargeImage, 1000);
set_hover_delayed_function('.docpopupfunc', openPopup, 1000);
pastePopupLink('docpopupfunc', 'entrytext');
</script>
<script type="text/javascript" src="/logautoupdate.js"></script>
<script>
const global_autologupdate = new autoLogUpdate('logautoupdate', true, 'entrytext');
</script>
<!-- <script type="text/javascript" src="/logcheckbox.js"></script> -->
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

RAWTEMPLATE='''<html><body>
%s
</body></html>'''

def get_most_recent_raw():
    thecmd = "./fzloghtml -q -d formalizer -s randalk -o STDOUT -E STDOUT -R -F raw"
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout) = (p.stdin, p.stdout)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        print(RAWTEMPLATE % result)
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
    if topic:
        urioptions += get_uri_arg_separator(arg_i)+'topic='+topic
        arg_i += 1
    if nnl:
        urioptions += get_uri_arg_separator(arg_i)+'nnl='+nnl
        arg_i += 1
    if searchtext:
        urioptions += get_uri_arg_separator(arg_i)+'searchtext='+uri_safe(searchtext)
    if regexpattern:
        urioptions += get_uri_arg_separator(arg_i)+'regex='+uri_safe(regexpattern)
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
    if topic:
        cmdoptions += ' -g '+topic
    if nnl:
        cmdoptions += ' -l '+nnl
    if searchtext:
        cmdoptions += ' -f "'+searchtext+'"'
    if regexpattern:
        cmdoptions += " -x '"+regexpattern+"'"
    if andall:
        cmdoptions += ' -A '
    if caseinsensitive:
        cmdoptions += ' -C '

    return cmdoptions

def render_log_interval():
    print(log_interval_head)
    thisscript = os.path.realpath(__file__)
    print(f'<!-- (For dev reference, this script is at {thisscript}.) -->')
    print('<h1>Formalizer: HTML FORM interface to fzloghtml</h1>\n<p></p>\n<table id="LogInterval"><tbody>')
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
    if review=='today':
        review_arg = '-j'
    else:
        review_arg = '-i -D 2'
    if review_date:
        date_specific = '-a %s' % review_date
    else:
        date_specific = ''
    thecmd = f"./fzloghtml -q -d formalizer -s randalk {date_specific} {review_arg} -F html -o STDOUT -E STDOUT"
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

INDEXMISSINGARGS='''<html>
<head>
<meta charset="utf-8" />
<link rel="stylesheet" href="/fz.css">
<link rel="icon" href="/favicon-logentry-32x32.png">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Log Index</title>
</head>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>
%s
</body>
</html>
'''

def render_Log_index():
    cmdoptions = build_command_options()

    if cmdoptions:
        thecmd = "./fzloghtml -q -d formalizer -s randalk -o STDOUT -E STDOUT -I "+cmdoptions
        try:
            p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
            (child_stdin,child_stdout) = (p.stdin, p.stdout)
            child_stdin.close()
            result = child_stdout.read()
            child_stdout.close()
            print(result)

        except Exception as ex:                
            print(ex)
            f = StringIO()
            print_exc(file=f)
            a = f.getvalue().splitlines()
            for line in a:
                print(line)
    else:
        print(INDEXMISSINGARGS % '<b>Missing request arguments.</b>')

if __name__ == '__main__':

    if review:
        render_Log_review()
    elif regen_index:
        render_Log_index()
    else:
        if mostrecentraw:
            get_most_recent_raw()
        elif mostrecentdata:
            render_most_recent()
        else:
            render_log_interval()

    sys.exit(0)
