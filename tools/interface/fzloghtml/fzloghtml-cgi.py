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
from datetime import datetime, date
from dateutil.relativedelta import relativedelta, SU
import traceback
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE

from fzmodbase import *
from TimeStamp import add_days_to_TimeStamp, add_weeks_to_TimeStamp, NowTimeStamp

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

def convert_datetime_format(datetime_str):
    date_part, time_part = datetime_str.split('T')
    date_numeric = date_part.replace('-', '')
    time_numeric = time_part.replace(':', '')
    return date_numeric + time_numeric

def get_upcoming_Sunday():
    return date.today() + relativedelta(weeks=+1, weekday=SU(0))

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
help = form.getvalue('help')
alloflog = form.getvalue('alloflog')
startfrom = form.getvalue('startfrom')
alt_startfrom = form.getvalue('alt_startfrom')
if alt_startfrom:
    startfrom = convert_datetime_format(alt_startfrom)
endbefore  = form.getvalue('endbefore')
alt_endbefore = form.getvalue('alt_endbefore')
if alt_endbefore:
    endbefore = convert_datetime_format(alt_endbefore)
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
btf = form.getvalue('btf')
review = form.getvalue('review')
review_date = form.getvalue('reviewdate')
if not review_date:
    review_date = datetime.now().strftime("%Y%m%d")
regen_index = form.getvalue('index')
selectchunks = form.getvalue('selectchunks')

json_output = form.getvalue('json')

#if not review:
#    print("Content-type:text/html\n\n")
print("Content-type:text/html\n")

if alloflog:
    startfrom = "199001010000"
    endbefore = NowTimeStamp()

try:
    if searchtext:
        with open('/var/www/webdata/formalizer/searchtext.test','w') as f:
            f.write(searchtext)
except:
    pass

interface_options_help = '''<html>
<head>
<title>fzloghtml-cgi API</title>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
</head>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>

<h1>fzloghtml-cgi API</h1>

<p>
Operations:
<ul>
<li><code>review!=None</code>: Generate and render Log review.
<li><code>index!=None</code>: Generate and render Log index.
<li><code>mostrecentraw!=None</code>: Retrieve raw content of most recent Log chunk.
<li><code>mostrecentdata!=None</code>: Retrieve data for most recent Log chunk.
<li>Otherwise: Render specified Log interval.
</ul>
</p>

<h3>Log review options</h3>

<p>
<code>review=today</code><br>
Interpret the current day for review.
</p>

<p>
Otherwise, interpret for day review with Log interval size of 2 days.
</p>

<p>
If <code>reviewdate=yyyyMMDD</code>, then interpret the Log around that date,
otherwise around the end of Log.
</p>

</body>
</html>
'''

#<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">
log_interval_head_part1 = '''<html>
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
/* Define the flashing sequence */
@keyframes warning-flash {
  0% { background-color: #ff0000; color: white; }    /* Bright Red */
  50% { background-color: #8b0000; color: #cccccc; } /* Dark Red */
  100% { background-color: #ff0000; color: white; }   /* Back to Bright */
}

/* The class that triggers the flash */
.flashing {
  animation: warning-flash 0.8s infinite; /* Cycles every 0.8 seconds forever */
  border: 2px solid white;
  box-shadow: 0 0 10px rgba(255, 0, 0, 0.8); /* Optional glow effect */
}

/* Basic button styling */
#my-button {
  padding: 10px 20px;
  background-color: #ccc;
  cursor: pointer;
  transition: all 0.3s;
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
<span class="alt_tooltip">Can an RA do this or help? <input type="checkbox">
<span class="alt_tooltip">Just get started <input type="checkbox">
<span class="alt_tooltiptext prot_tip"><div>Just get started on a Node, you don't need to complete it right away, you just need to figure out what to do and take notes about that to get started.</div></span>
</span><br>
<button class="button button2" onclick="window.open('/cgi-bin/orderscore-cgi.py', '_blank');">OrderScore</button><br>
<button class="button button1" onclick="window.open('/cgi-bin/fzloghtml-cgi.py?review=today', '_blank');">Today Review</button><br>
<button id="WeekGoals" class="button button2" onclick="window.open('/cgi-bin/nodeboard-cgi.py?D=week_main_goals&T=true&u='''

log_interval_head_part2 = '''&r=100&U=true', '_blank');">Week Goals</button><br>
<button id="DayWiz" class="button button1" onclick="window.open('/cgi-bin/daywiz.py', '_blank');">DayWiz</button>
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
Specify interval:<br />
<input type="checkbox" name="alloflog"> all of the Log (overrides interval start/end)<br />
<!-- value="2025-05-22T20:00" -->
<input type="text" name="startfrom"> <input type="datetime-local" id="alt_startfrom" name="alt_startfrom" min="1990-01-01T00:00:00"> start at this YYYYmmddHHMM time stamp (default: 24 hours before end of interval)<br />
<input type="text" name="endbefore"> <input type="datetime-local" id="alt_endbefore" name="alt_endbefore" min="1990-01-01T00:00:00"> end before this YYYYmmddHHMM time stamp (default: 1 second after most recent Log entry)<br />
<input type="text" name="daysinterval"> interval size in days<br />
<input type="text" name="weeksinterval"> interval size in weeks<br />
<input type="text" name="hoursinterval"> interval size in hours<br />
<input type="text" name="numchunks"> number of Log chunks (interval takes precedence)<br />
<input type="checkbox" name="frommostrecent"> from most recent (interval takes precedence)<br />
Specify filters:<br />
<input type="text" name="node"> belongs to node, node history<br />
<input type="text" name="topic"> belongs to topic<br />
<input type="text" name="nnl"> belongs to NNL<br />
<input type="text" name="searchtext" size="48"> must contain search text
<input type="checkbox" name="caseinsensitive"> case insensitive
<input type="checkbox" name="andall"> must contain each<br />
<input type="text" name="regex" size="48"> must contain RegEx<br />
<input type="text" name="btf"> must belong to BTF category<br />
<input type="checkbox" name="selectchunks"> enable selecting of Log chunks<br />
Or:<br />
<input type="checkbox" name="mostrecentdata"> show most recent Log data summary<br />

<input type="submit" value="Apply" />
</form>

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
<script type="module">
// import { fzCGIRequest, setupfzCGIListener, pollfzServer } from '/fzCGIRequest.js';
const btn = document.getElementById('WeekGoals');
function startWarning(btn_id) {
    document.getElementById(btn_id).classList.add('flashing');
}
function stopWarning(btn_id) {
    document.getElementById(btn_id).classList.remove('flashing');
}
/* Commented out "direct" approach:
function responseHandler(data) {
    let logdata = JSON.parse(data);
    if ('chunks_rendered' in logdata) {
        //console.log(`Number of chunks returned: ${logdata['chunks_rendered']}`);
        if (logdata['chunks_rendered'] == 1) {
            //console.log(`Last chunk in history of Node at epoch: ${logdata['chunks'][0][0]}`);
            const unixtime_ms = logdata['chunks'][0][0] * 1000;
            const now = Date.now();
            const elapsed_ms = now - unixtime_ms;
            const elapsed_days = elapsed_ms / (24*60*60*1000);
            //console.log(`Days elapsed: ${elapsed_days}`);
            if (elapsed_days > 7.0) {
                startWarning('WeekGoals');
            } else {
                stopWarning('WeekGoals');
            }
        }
    }
}
pollfzServer('/cgi-bin/fzloghtml-cgi.py', [['json','on'], ['node', '20240603105017.1'], ['frommostrecent', 'on'], ['numchunks', '1']], responseHandler, 60000);
*/
async function getIndicatorsData(url) {
  try {
    const response = await fetch(url);
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    const data = await response.json();
    
    if (data['WeekGoals_Overdue']['state']) {
        startWarning('WeekGoals');
    } else {
        stopWarning('WeekGoals');
    }
    if (data['DayWiz_Overdue']['state']) {
        startWarning('DayWiz');
    } else {
        stopWarning('DayWiz');
    }
    
    //console.log(`The data is:`, data);
    //return data;

  } catch (error) {
    console.error("Failed to fetch or parse JSON:", error);
  }
}
const IndicatorsDataUrl = '/formalizer/indicators.json';
setInterval(() => {
  getIndicatorsData(IndicatorsDataUrl);
}, 10000); // every 10 seconds
</script>
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
    if btf:
        urioptions += get_uri_arg_separator(arg_i)+'btf='+uri_safe(btf)
        arg_i += 1
    if selectchunks:
        urioptions += get_uri_arg_separator(arg_i)+'selectchunks='+uri_safe(selectchunks)

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
    if btf:
        cmdoptions += " -B '"+btf+"'"
    if selectchunks:
        cmdoptions += ' -S selectchunks.py'

    return cmdoptions

def render_log_interval():
    upcoming_Sunday_str = get_upcoming_Sunday().strftime("%Y%m%d%H%M")
    print(log_interval_head_part1+upcoming_Sunday_str+log_interval_head_part2)
    if selectchunks:
        print('<form action="/cgi-bin/selectchunks.py" method="post">')
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
    if selectchunks:
        print('Send selected Log chunks to <input type="submit" name="time_reporting" value="Time Reporting" />')
        print('or to <input type="submit" name="selected_display" value="Selected Log chunks display" />.')
        print('</form>')

    if endbefore:
        if is_before_now(endbefore):
            later_uri = build_uri_options(diff_endbefore=10)
            print(f"<button class=\"button button1\" onclick=\"window.open('/cgi-bin/fzloghtml-cgi.py{later_uri}','_self');\">Later (10 days)</button>")
            evenlater_uri = build_uri_options(diff_weeks_endbefore=5)
            print(f"<button class=\"button button2\" onclick=\"window.open('/cgi-bin/fzloghtml-cgi.py{evenlater_uri}','_self');\">Later (5 weeks)</button>")

    print('<a name="END">&nbsp;</a>')
    print(cgi_custom_tail)
    print("</body>\n</html>")

def json_log_interval():
    cmdoptions = build_command_options()
    if cmdoptions:
        thecmd = "./fzloghtml -q -d formalizer -s randalk -o STDOUT -E STDOUT -N -F json "+cmdoptions
        try:
            p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
            (child_stdin,child_stdout) = (p.stdin, p.stdout)
            child_stdin.close()
            result = child_stdout.read()
            child_stdout.close()
            print(result)

        except Exception as ex:                
            print('{"error": "')
            f = StringIO()
            print_exc(file=f)
            a = f.getvalue().splitlines()
            for line in a:
                print(line)
            print('"}')
    else:
        print('{"error": "Missing command options."}')

def show_rendered_Log_review(rendered:str, thecmd:str):
    part1 = rendered.find('<html>')
    if part1 >= 0:
        part1 += len('<html>')
        print(rendered[:part1])
        print('<!-- generated by command: %s -->' % str(thecmd))
        print(rendered[part1:])
    else:
        print(rendered)

def render_Log_review():
    if review=='today':
        review_arg = '-j'
    else:
        review_arg = '-i %s' % review_date
    thecmd = f"./fzloghtml -q -d formalizer -s randalk {review_arg} -F html -o STDOUT -E STDOUT"
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout) = (p.stdin, p.stdout)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        show_rendered_Log_review(result, thecmd)
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

    if help:
        print(interface_options_help)
        sys.exit(0)

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
            if json_output:
                json_log_interval()
            else:
                render_log_interval()

    sys.exit(0)
